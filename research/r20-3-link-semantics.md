# R20-3 链接语义：tape 标记、C99 6.9.2 规则、符号顺序（研究稿）

状态标注：**[跑过]** = 2026-10-02 在本机 osx/arm64 用 `/tmp/ua_ref` 实测（注意：该二进制可能由未提交树构建，见 memory「Shared ua_ref is not committed」，正式验收须用同源 UA 重跑 `tests/linkunits/r20-3/run.sh`）；**[设计]** = 未实现、未运行。

## 1. 现状（读码 + 实测）

### 1.1 tape 里全局对象长什么样 [跑过，`-c -funit -S -o -`]
- `int g = 7;`（-funit）→ `.global g_g` / `.bss g_g 4`，初值**不在数据里**，而是 `__init_u:` 里的 `imm r0,7; .lea r1,g_g; .st [r1+0],r0,4`。
- `int g;` → 同样的 `.global g_g` / `.bss g_g 4`，`__init_u` 里无写入。
- `extern int g;` → `.extern g_g`，无 `.bss`（src/front_parse.c:5465-5473）。
- `.bss` 在该名**第一次**声明时写出（`gdup==0`，front_parse.c:5487），此时还不知道同单元后面是否有 `int g = 5;` 补全。

所以 tape 上「带初值定义」与「暂定定义」**逐字节相同**，区别只藏在 `__init_u` 的代码里。

### 1.2 tapelink 的合并（src/tapelink.c:127-196）
- pass0 收 `.global` 名（kind 2），pass1 收本单元定义（`.bss`/`.str`/标签，kind 1）。
- `.bss NAME` 且 NAME 是 `.global`：若之前单元已见（`tl_gseen`）就**跳过**（第一个 `.bss` 胜）。
- 导出标签重复 → `link: multiple definitions of NAME`（R19-8）。
- `.global/.extern` 行丢弃；各单元 `__init_u` 改名 `__init_u<k>`，按链接顺序调用。

### 1.3 实测矩阵 [跑过]（`run.sh /tmp/ua_ref`，cc = Apple clang -std=c99）

| 例 | 单元 | unisacc 链接 | unisacc 一步 | cc |
|---|---|---|---|---|
| 两个暂定 | tent_a + tent_b | `5` rc0 | `5` rc0 | `5` rc0（本机 clang 实测接受） |
| 初值+暂定 | init1 + tentm / tentm + init1 | `7` rc0 | `7` rc0 | `7` rc0 |
| **两个初值** | initm + init2 | **`9` rc0（错：后一个 `__init_u` 覆盖）** | refuse rc1 | `duplicate symbol '_g'` |
| 两个初值（反序） | init2 + initm | **`3` rc0（错）** | refuse rc1 | duplicate |
| 两个 main | main_a + main_b | `link: multiple definitions of main` rc1 | `multiple definitions of this function` rc1 | duplicate |
| **只有 extern** | ext_a + ext_b | **rc=138（SIGBUS，无诊断）** | **`0` rc0（错：静默造出存储）** | `Undefined symbols … _g` |
| static 同名 | stat_a + stat_b | `1 2` rc0 | `1 2` rc0 | `1 2` rc0 |

结论：R20-3 已知缺口（两个初值被合并）实测成立，且比描述更糟——不是「第一个胜」，而是**两份初值都执行、后者覆盖**，结果随链接顺序变。另有**新发现**：只有 `extern` 无定义时，链接路径崩溃（138），一步路径静默分配并读到 0。两者都应报未定义。

## 2. 提议的 tape 标记 [设计]

只加**一个**新指令，行级、独立成行：

```
.gdef NAME
```

含义：本单元对已 `.global` 的对象 NAME 给出了**带初值的定义**（外部定义，6.9.2p1 意义上的 external definition with initializer）。三态由已有记录组合表达：

| 源码 | tape |
|---|---|
| `int g = 7;` | `.global g_g` `.bss g_g 4` `.gdef g_g` |
| `int g;`（暂定） | `.global g_g` `.bss g_g 4` |
| `extern int g;` | `.extern g_g` |
| `static int s = 1;` | `.bss g_s…`（无 `.global`，不写 `.gdef`；按现规则改名为单元私有） |

为什么是独立一行而不是 `.bss NAME SIZE init`：
1. `.bss` 在首次声明时已写出，`int g; … int g = 5;` 的初值在之后才知道；独立行可在解析到 `=` 时写出，位置任意，tapelink 在 pass0 收集即可。
2. 向后兼容：`.bss` 语法不变；`.gdef` 只在 `-funit`（`unitmode`）下写出，tapelink 与 `.global/.extern` 一样把它丢弃，后端**永远看不到**。旧对象（无 `.gdef`）按「全是暂定」处理 = 今天的行为，不会新拒绝任何旧对象。
3. 函数不需要：函数定义就是导出标签，R19-8 已按标签查重。

写出点（cdx 产品与参考各一处）：参考在 front_parse.c 全局声明分支解析到 `=` 且 `declstatic==0 && unitmode` 时 `es(".gdef g_"); etok(t); ec(10);`。产品 δ 在对应的“全局初值”决策处写同一行。

## 3. 规则（参考将实现，产品须一致）[设计]

C99 6.9.2 + 传统 common 约定（unisacc 选择**接受** common，与 tapelink 现注释和 errno 合并一致；GCC ≥10 默认 -fno-common 会拒绝两个暂定，这里明确不跟随）：

对每个 `.global` 对象名 N，跨全部单元统计 D = 带 `.gdef N` 的单元数，T = 有 `.bss N` 无 `.gdef` 的单元数，X = 只有 `.extern N` 的单元数：

1. D ≥ 2 → 拒绝：`link: multiple definitions of N\n`，rc 1（与 R19-8 同一消息，名字为 C 名，去掉 `g_` 前缀——见下注）。
2. D = 1 → 一个对象；保留那个单元的 `__init_u<k>` 写入，其余单元的 `.bss N` 丢弃。尺寸取各 `.bss` 的最大值（6.9.2 不允许不一致，取最大避免越界，与 common 约定一致）。
3. D = 0, T ≥ 1 → 一个零初始化对象（common），尺寸取最大。
4. D = 0, T = 0, X ≥ 1 → 拒绝：`link: undefined reference to N\n`，rc 1。一步编译路径（front_parse）同样须拒绝：`error: undefined reference to this object (declared extern, never defined)`，rc 1。
5. 无 `.global` 的名字（static）每单元改名私有，从不参与 1–4。
6. 对象位置（地址顺序）由第 4 节规则决定，**不**由“第一个出现的单元”决定。

注：现有 `link: multiple definitions of main` 打印的是标签名；对象的标签是 `g_g`。为让用户看到 C 名，打印前剥去 `g_` 前缀。probes 期望值按剥去后写。

## 4. 唯一的符号顺序规则 [设计]

> **数据记录（`.bss` / `.str` / 复合字面量 `__cl*`）不得出现在指令记录之间。每个单元的数据记录集中在该单元的数据段头，按「定义点在源中的首次出现顺序」排列；复合字面量按其所在顶层声明的位置、在该声明的对象之后、以单元内序号 `__cl<k>.<n>`（n 从 0 起，按出现顺序）命名，不以 token 下标命名。链接后的程序按单元链接顺序拼接各单元数据头。**

“定义点”= 带初值的声明；若无则第一个暂定定义；`extern` 声明不算定义点。

## 5. 三处分叉分别被这条规则改变什么 [设计；分叉身份来自记录]

- **b_compound**（exec/c/chain.knownfail：“.bss definitions move across instruction records; six images differ”）：参考在函数体中遇到复合字面量时把 `.bss __cl<tok>` 就地写进指令流（front_parse.c:1124），产品把它放在别处。规则的“数据记录不得在指令之间、集中到单元数据头”使两侧位置相同，镜像地址随之一致。
- **b_pp2**（knownfail：“product uses a distinct compound literal symbol address”，文件域复合字面量）：参考名字是 `__cl` + token 下标（随预处理展开而变），产品用另一套编号；规则改为 `__cl<k>.<n>` 单元内出现序号，且放在所属顶层对象之后，名字与地址两侧同一。
- **fb12-13**（tests/c/fb12-13-extern-incomplete-array.c：`extern const char ident[]; extern int table[];` 先于定义）：分叉出在符号表从“首次声明”（extern）还是“定义”排位。规则规定以定义点排位、`extern` 不占位，两侧顺序一致；同时第 2 节中 extern 不写 `.bss`，定义时才写，与“定义点”一致。

（三项的具体字节差异本轮**未复现**，以上是按 knownfail 文字与读码作出的对应，需 cdx 在产品侧用 `exec/pipeline/run.py` 对拍确认。）

## 6. 逐字节 probes

源码在 `tests/linkunits/r20-3/`，驱动 `run.sh UA`（每单元 `UA X.c -c -b HOST -funit -o X.o`，再 `UA a.o b.o`，以及一步 `UA a.c b.c`；每步 `tests/bound.py 20`）。期望值（实现后）：

| 例 | 单元 | stdout | stderr | rc | 今天 [跑过] |
|---|---|---|---|---|---|
| 两暂定合并 | tent_a.c + tent_b.c | `5\n` | 空 | 0 | 同 |
| 初值+暂定 | init1.c + tentm.c，及反序 | `7\n` | 空 | 0 | 同 |
| 两初值拒绝（链接） | initm.c + init2.c，及反序 | 空 | `link: multiple definitions of g\n` | 1 | `9`/`3` rc0（错） |
| 两初值拒绝（一步） | 同上 `.c` | 空 | `…/init2.c:1:5: error: multiple definitions of this object across units (each has an initialiser)` + 源行 + 插入符 + `1 error generated.` | 1 | 同 |
| 两 main | main_a.c + main_b.c | 空 | `link: multiple definitions of main\n` | 1 | 同 |
| 只 extern（链接） | ext_a.c + ext_b.c | 空 | `link: undefined reference to g\n` | 1 | rc138（错） |
| 只 extern（一步） | 同上 `.c` | 空 | `error: undefined reference to this object (declared extern, never defined)`（带位置） | 1 | `0` rc0（错） |
| static 独立 | stat_a.c + stat_b.c | `1 2\n` | 空 | 0 | 同 |

建议接入 tests/linkunits.sh：现有 `pair` 只比较输出相等，需加一个 `refuse NAME A B WANT_STDERR` 帮手比对 stderr 与 rc；并在 `LINK_PRODUCT` 下对产品同跑（产品与参考须同一 stderr）。

## 7. 实际跑过 vs 仅设计

- 跑过：§1.1 tape 形状；§1.3 全表（unisacc 链接、一步、clang）。
- 仅设计：§2 `.gdef`、§3 规则与消息文本、§4 顺序规则、§5 对三处分叉的影响、§6 的“实现后”期望列。
- 未改：src/、exec/ 均未动，未提交。

## 8. cdx 审核后的契约（2026-10-02，与上文冲突处以本节为准）

批准 `.gdef NAME`：表示“本单元给出了该外部对象的带初值定义”。它是独立的 tape 记录，可以出现在首个 `.bss` 之后。分工：cdx 负责 shape、tapebin、C 与 Python 的读写、产品 E3 和对象路由；cc 负责参考侧的标记、tapelink 的 D/T/X 计数和红例。

1. **旧对象不按“全是暂定定义”处理。** 旧的带初值 .o 同样没有 `.gdef`，两个这样的旧对象仍会静默地后者覆盖前者。所以新语义要求 unit tape 带一个**必需的版本标记**，缺版本的对象被 tapelink 拒绝，提示重新编译。不宣称向后兼容，§2 里“旧对象 = 全暂定”的说法作废。具体拼写与 cdx 一起定，在 tapebin 一侧落地。
2. **“只有 extern”时的拒绝**，只适用于只链接本工具对象的路径：所有输入都查完以后，再按名报 `link: undefined reference to NAME`（已实现：src/tapelink.c 的 `tl_check_undef`，产品驱动调用同一个函数）。一步编译的全程序路径报 `unisacc: error: undefined reference to NAME (declared extern, defined in no unit)`。将来链接 cc 产出的 .o/.a 数据符号时，要在那些输入也查完之后才能下结论。这不写成适用于所有外部链接的一般 C99 规则。
3. **§4 的统一顺序和 `__cl` 重新编号拆成 R20-3b**：先逐个复现 b_compound、b_pp2、fb12-13 的首处字节差异，再定统一顺序；没有逐字节证据之前，不把三处差异归为同一个根因。
4. 正式探针使用当前 HEAD 的私有 UA，不用 /tmp/ua_ref。
