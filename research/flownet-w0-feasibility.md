# flownet W0 可行性裁定（三个探针，实测）

> 需求：`plans/m4-flownet-cx-module.md` §4 W0 · 执行：cc · 2026-10-23 · 基线 `b7fea757`，root 产品 `unisacc.com`（m9，sha256 前缀 `3b17e191a8c6d2e3`）
> 结论一句话：**三问皆「是」**。`.cx` 能以表驱动形式表达并穷举验收 10 张网全部 3677 个键（10/10 exact）；能读外部 `.tsv` 且错误可具名；两个 `.cx` 可编译为一个程序互相调用，缺了模块会被具名拒绝（rc 127）。§8 裁定 3 的降级不触发，W2 选**运行时读外部 `.tsv`**，生成 `.cx` 表作为已实测的备选。
> 实测期间另得两条产品行为（非本需求修复范围，作为 flownet 的设计约束记录）：未定义的外部**对象**不被拒绝、按 0 读出；`cx_read` 把目录读成空缓冲。

所有命令都在仓库根目录跑，每条前缀 `timeout 58`（D6）。下文 `$S` = `/home/devali/.hermes/cache/scratch/claude-1001/-home-devali-repos/41bdac58-fc66-4720-85ce-d4853b027c7b/scratchpad/flownet/w0`（本会话的 scratch，探针 1、2 的源在这里；探针 3 已入树、入闸）。

## 探针 1：`.cx` 能否表达 10 张网的键空间（表驱动 + acc）

**做法。** `$S/p1/export.py`（只在 scratch，作 oracle 用：`m4.nets.build_nets()` + `ujs.seed.construct.gold.STAGES`）把 10 张 IntNet 原样导出为 C 数组，写成生成源码 `$S/p1/gen/fnw0tab.cx`（18046 字节，105 行，sha256 前缀 `890a2e721f29be7c`，连跑两次相同）：每网 `radix`（各字段取值数）、`offs`、`b1`、`feeds`（CSR：`fptr`/`fidx`）、`W2`（CSR：`wptr`/`wk`/`ww`）、以及每个键的 gold 类下标。执行器 `$S/p1/acc.cx`（2257 字节）对每个键按 `itertools.product` 次序做混合进制分解，跑与 `unisa/intnet.py:IntNet.predict` 相同的整数前向（命中计数 → `hit+b1>0` → 累加 W2 → 严格大于的 argmax），与 gold 比较。

**命令与实测输出。**

```
$ python3 $S/p1/export.py $S/p1/gen                       # 1.3 s
nets 10 keys 3677 lex:100/H9/c9 parse:330/H38/c34 type:1863/H40/c10 scope:48/H6/c5 shape:54/H15/c15 irsel:117/H19/c17 ic:975/H27/c19 isel:42/H16/c18 enc:144/H10/c7 reloc:4/H2/c4
$ sh ./unisacc.com $S/p1/acc.cx $S/p1/gen/fnw0tab.cx -o $S/p1/acc      # 2.85 s, rc 0
$ $S/p1/acc                                                             # 0.006 s, rc 0
lex      100/100   ok
parse    330/330   ok
type    1863/1863  ok
scope     48/48    ok
shape     54/54    ok
irsel    117/117   ok
ic       975/975   ok
isel      42/42    ok
enc      144/144   ok
reloc      4/4     ok
acc: 10/10 nets exact
$ sh ./unisacc.com -run $S/p1/acc.cx $S/p1/gen/fnw0tab.cx -- lex parse isel enc     # acc 子集（4 张决策网），2.9 s
...  acc: 4/4 nets exact
```

与 oracle `python3 -m m4 acc` 的逐网 (exact,total) 相同（lex 100、parse 330、type 1863、scope 48、shape 54、irsel 117、ic 975、isel 42、enc 144、reloc 4，合计 3677）。

**不是摆设（变异）。** `--mutate NET` 给 NET 的每个单元加一条 W2 边（某一类 +1000）：`lex 10/100 FAIL`、`type 21/1863 FAIL`、`isel 3/42 FAIL`、`reloc 1/4 FAIL`，均 rc 1；不变异时 rc 0。（第一版变异只改 W2 第一个权重，四网全仍 exact——那个单元只为本类点火，改它不改 argmax；已换成上面的强变异，如实记下。）

**规模上限（编译进去的表要付编译时间）。** 单个 `static const int big[N]` 初始化器，`-run` 墙钟：N=20000 5.2 s、50000 10.5 s、65000/66000 13.1 s、100000 20.1 s、150000 29.3 s（均正确求和）；**N=200000 rc 1，`reject: not covered: a branch to an undefined label`**（36 s，具名拒绝，不是崩溃）。约 0.2 ms/元素，线性。10 网全部参数加 gold 只有约 5K 个整数，离上限两个数量级；但 W2 若把 28 个语料的 gold 字节也编译进去，就要按这个尺子算。

**结论 1：是。** 表驱动的 `.cx` 直接表达 3677 键，在 m9 上编译 + 运行，10/10 exact（不止子集），无需「按需构造 + 稀疏表」或降级为纯函数。

## 探针 2：`.cx` 能否读外部 gold 表（`.tsv`），路径与错误处理

**做法。** 同一个 export 另写 20 个 `.tsv`（`$S/p1/gen/tsv/`，合计 70887 字节）：`NET.tsv` 每键一行 `key<TAB>字段下标...<TAB>gold 类名`；`NET.net.tsv` 把 IntNet 写成文本（`radix`/`offs`/`b1`/`classes` 各一行，`feed c units...`、`w2 j k:w...` 每行一项）。两个读表探针：

- `$S/p2/tsvgold.cx`（与 `fnw0tab.cx` 一起编译）：读 `DIR/NET.tsv`，逐行核对键序、字段下标 = 混合进制位、类名 ↔ 编译进去的类下标一一对应。
- `$S/p2/tsvacc.cx`（**单文件，无任何编译进去的表**）：读 `NET.net.tsv` 重建网、读 `NET.tsv` 取 gold，前向后比较**类名**。

**命令与实测输出。**

```
$ sh ./unisacc.com -run $S/p2/tsvacc.cx -- $S/p1/gen/tsv lex parse type scope shape irsel ic isel enc reloc     # 2.4 s, rc 0
lex      100/100   ok   ...   reloc      4/4     ok
acc from tsv: 10/10 nets exact, 3677 keys
$ $S/p2/tsvgold $S/p1/gen/tsv                                                                               # rc 0
lex      100 rows   9 labels  agrees   ...   reloc      4 rows   2 labels  agrees
tsv: 10/10 tables agree
```

错误处理（每例 rc 1、stdout 0 字节、stderr 点名；`…` 为 `$S/p2/bad` 或 `$S/p2/mut`）：

| 情形 | stderr |
|---|---|
| 目录不存在 | `tsvgold: …/nope/lex.tsv: cannot stat (missing?)` |
| 同名目录占位 | `tsvgold: …/dir/lex.tsv: not a regular file` |
| parse 第 5 行标签改成 BOGUS | `tsvgold: …/parse.tsv:5: label disagrees with compiled gold` |
| type 第 7 行多一个字段 | `tsvgold: …/type.tsv:7: wrong field count` |
| reloc 少最后一行 | `tsvgold: …/reloc.tsv: row count differs from key space` |
| scope 第 3 行键乱序 | `tsvgold: …/scope.tsv:3: key out of order or out of range` |
| 表内 NUL 字节 | `tsvgold: …/lex.tsv: NUL byte in table` |
| 权限 000 | `tsvgold: …/enc.tsv: cannot read` |
| enc.net.tsv 第 2 行 `offs 0 x` | `tsvacc: …/enc.net.tsv:2: not a list of integers` |
| reloc.net.tsv 截掉末尾换行 | `tsvacc: …/reloc.net.tsv:10: no final newline` |
| scope.net.tsv 缺失 | `tsvacc: …/scope.net.tsv: cannot read` |
| isel.net.tsv 每单元 +1000 到类 1（权重变异） | `isel 3/42 FAIL`，rc 1 |
| lex.tsv 第 10 行 gold 改为 none（标签变异） | `lex 99/100 FAIL`，rc 1 |

路径：相对路径按进程 cwd 解析（`cd $S/p2 && ./tsvgold rel isel` → `1/1 tables agree`）；参数经 `-run X.cx -- ARGS` 传入。

**附带测得的产品行为：** `cx_read` 对**目录**返回非 NULL、长度 0（与空文件无法区分）；对不存在的路径返回 NULL。所以读表方必须先 `stat` 判 `S_ISREG`，并拒绝零行表——入树探针已这样做。

**结论 2：是。** `.cx` 有文件读写（`cx_read`/`cx_write`，prelude 内建），能读外部 `.tsv` 并完全从文本重建 10 张网、穷举 exact；错误可做成「路径[:行]: 原因」的具名拒绝。

## 探针 3：`.cx` 有无模块引用机制（A.cx 用 B.cx）

**机制。** `.cx` 没有 import 语法；模块引用就是 C 的多翻译单元：`sh ./unisacc.com A.cx B.cx -o out` 或 `sh ./unisacc.com -run A.cx B.cx -- ARGS` 把几个 `.cx` 编成一个程序，A 用原型（或 `#include "b.h"`，按包含文件所在目录解析）声明 B 的外部函数。每个单元都隐式带 prelude，`static` 的 `cx_*` 与各单元自己的 `static` 同名不冲突。

**scratch 实测（`$S/p3/`）。**

```
$ sh ./unisacc.com $S/p3/a.cx $S/p3/b.cx -o $S/p3/ab && $S/p3/ab $S/p3/b.cx     # 编译 1.8 s
sum=100 oob=-1 len=320                                                          # rc 0
$ sh ./unisacc.com -run $S/p3/a.cx $S/p3/b.cx -- $S/p3/b.cx                     # 同上，rc 0
$ sh ./unisacc.com -run $S/p3/a2.cx $S/p3/b.cx      # a2 #include "b.h"，且与 b.cx 有同名 static tab
get2=30 tab=2                                                                   # rc 0
$ sh ./unisacc.com -run $S/p3/a.cx                  # 不带 B
unisacc: no host function fnb_get                                               # rc 127
$ sh ./unisacc.com $S/p3/a.cx -o $S/p3/aonly        # rc 0（不在链接期拒绝）
$ $S/p3/aonly x
unisacc: no host function fnb_get                                               # rc 127（首次调用时具名拒绝）
$ sh ./unisacc.com -run $S/p3/a.cx $S/p3/b.cx $S/p3/c.cx   # c.cx 重复定义 fnb_get
reject: multiple definitions of this function                                   # rc 1（-o 同）
```

**产品行为（设计约束，重要）：未定义的外部对象不被拒绝。** `$S/p3/ext.cx` 只有 `extern const int fw0x[2];` 而无定义：`-run` 打印 `x0=0 x1=0`、rc 0；`-o` rc 0、运行 rc 0；`extern int fw0y;` 写 5 再读出 5。即缺失的外部**对象**被当作零初始化的定义，而缺失的外部**函数**在首次调用时 rc 127 具名拒绝。入树探针第一版就踩到了：A 读 B 导出的数组 `fw0_radix[]`，不带 B 时循环 0 次、打印了 `FAIL …` 才在下一次函数调用处 rc 127。对 flownet 的后果：**模块 API 只导出函数，不导出数据对象**；否则 D3（移除调用必须红）与 D5（未覆盖必须具名拒绝）可能被「缺模块却读到全 0」悄悄绕过。这是编译器语义，按 §5 红线 4 不在本需求内改，只作约束记录。

**入树探针（经 commitgate）。** `tests/flownetw0.cx`（A）+ `tests/flownetw0mod.cx`（B，`fw0_` 前缀、只导出函数：查表、键空间、带具名错误的 `.tsv` 读器）。两个闸步：

| 闸步 | 命令 | 实测 |
|---|---|---|
| `flownet-w0-module` | `tests/bound 58 sh ./unisacc.com -run tests/flownetw0.cx tests/flownetw0mod.cx -- .` | rc 0，5 s：`flownet-w0: module reference ok (2 units, 9 checks)`（跨单元查表、键外拒绝、好表 3 行和 42、缺文件/目录/零行表/键乱序/多字段/缺末尾换行各自点名路径与行） |
| `flownet-w0-module-negative` | 同上 `-- --negative .` | rc 0，7 s：子进程 `sh ./unisacc.com -run tests/flownetw0.cx`（不带 B）rc 127、输出 `unisacc: no host function fw0_radix`、无裁决行 → `removing the module is red` |

登记：`tests/gate.sh` 两行 job，`tests/gatelayers.py` contract 层，`tests/stagemap.c` meta，`tests/INVENTORY.md` 两行（两文件均 `gate: flownet-w0-module, flownet-w0-module-negative`）。未进 `tests/gatedeps.json`（未声明的闸步按全局输入处理，`gatequeue` 支持）与 `release/derived-map.tsv`（它要求套件已在 gatedeps 中声明，W0 不需要派生路由）。

**inventory 与共享检出。** 提交时工作树里有另一会话正在移植的、未跟踪的 `tests/*check.cx`（`copycontractcheck.cx` 等 8 个以上，非本波文件），树内 `script-inventory` 因它们是 ORPHAN 而红。本波的 inventory 因此在 HEAD 的干净导出（`git archive HEAD tests exec .github release` 加本波 5 个文件）里跑：`--check` rc 0，`scripts 619 gated/reached 556 library 9 listed 54 orphan 0`；`tests/INVENTORY.md` 取自该导出，相对 HEAD 只多本波两行。commitgate 点名 `flownet-w0-module`、`flownet-w0-module-negative`、`gate-layers`。

**结论 3：是。** 多单元编译就是模块机制，能过 commitgate；不需要「合并为单文件 + 命名前缀分区」的降级。但命名前缀（`fw0_`、日后 `fn_`）仍要用：所有单元共享一个外部命名空间，重名外部函数被 `reject: multiple definitions` 拒绝。

## 裁定（按 §8 裁定 3）

1. **表达力为「是」，§8-3 的降级不触发。** 不做 `.cx` 语言特性工作，也不需要稀疏构造/纯函数。
2. **W2 的输入机制：运行时读外部 `.tsv`（首选）。** 理由：(a) 缺表 = 具名拒绝（`stat`+`cx_read` 失败），而编译进去的表若以外部数据对象跨单元引用，缺单元会静默读 0（探针 3 的产品行为）；(b) flownet 的编译时间与表规模无关（探针 2b 单文件 2.4 s），不碰 ~150K 元素的编译上限；(c) 「标签源」本来就要做成注入面（§2-1、W3），文件路径是最薄的注入面；(d) `.tsv` 可 diff、可被 W1 基线逐字节固化。表由边界适配器（`m4/` 或 `adapter/`，允许 Python，作 oracle 导出）写出并入树，`flownet/` 只读不导，D1/D4 都成立。
3. **生成 `.cx` 表为已实测的备选**（探针 1：18 KB 源、2.85 s 编译、10/10 exact）。只在需要单文件、无外部表的场合用，且必须以**函数**而非数据对象暴露，例如 `int fn_tab_gold(int net, int key)`。
4. **flownet API 约束（由本波实测得出，W2 起执行）：** 只导出函数；外部名一律 `fn_` 前缀；读表先 `stat` 判普通文件、拒零行；错误一律「路径[:行]: 原因」，rc 1，stdout 空。

## 复现清单

| 探针 | 位置 | 命令（仓库根） | 墙钟 |
|---|---|---|---|
| 1 导出（oracle） | `$S/p1/export.py` | `python3 $S/p1/export.py $S/p1/gen` | 1.3 s |
| 1 表驱动 acc | `$S/p1/acc.cx` + `$S/p1/gen/fnw0tab.cx` | `sh ./unisacc.com -run $S/p1/acc.cx $S/p1/gen/fnw0tab.cx [-- NET... \| -- NET --mutate NET]` | 2.9 s |
| 1 规模 | `$S/p1/scale/bigN.cx` | `sh ./unisacc.com -run $S/p1/scale/big100000.cx` | 5–36 s |
| 2 读 gold 表 | `$S/p2/tsvgold.cx` | `sh ./unisacc.com $S/p2/tsvgold.cx $S/p1/gen/fnw0tab.cx -o $S/p2/tsvgold && $S/p2/tsvgold $S/p1/gen/tsv` | 3.6 s |
| 2b 纯文本 acc | `$S/p2/tsvacc.cx` | `sh ./unisacc.com -run $S/p2/tsvacc.cx -- $S/p1/gen/tsv lex parse type scope shape irsel ic isel enc reloc` | 2.4 s |
| 3 两单元 | `$S/p3/{a,a2,b,c,ext}.cx` | 见探针 3 | 1.8 s |
| 3 入树 | `tests/flownetw0.cx`、`tests/flownetw0mod.cx` | `./tests/gate.sh --suite flownet-w0-module --suite flownet-w0-module-negative` | 5 s + 7 s |

scratch 源的 sha256 前缀：`export.py 1be5cfaafbb43047`、`acc.cx 72f31ebe114a52f2`、`fnw0tab.cx 890a2e721f29be7c`、`tsvgold.cx 84916f1186803f99`、`tsvacc.cx 3a819e4ebf2cb3c0`、`a.cx 73fd20cb561a0de2`、`b.cx 12ba95b478b94fa5`、`ext.cx 9b28b39b7fc51357`。scratch 不随仓库走；探针 1/2 的导出在 W1/W2 由边界适配器入树重做（新 Python 文件不进本波）。
