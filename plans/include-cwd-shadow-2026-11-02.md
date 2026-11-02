# cwd `include/` 遮蔽系统头：范围、证据、修法（只读调查，2026-11-02）

日期：2026-11-02，omp-ds。冻结期只读；所有样本跑 `/tmp`，未改 `tools tests cx src include weights exec`。
cc 裁定（2026-11-02）：默认**不再搜 cwd/include，也不再搜 exe 目录**，系统头默认只用内嵌；开发要用树头必须显式
`-I`（或等价专用开关），不留隐式路径；批次要显式声明"验树头/验包内头"并写进证据行，默认包内。本文件按此改写。

## 一、最小复现（现装产品，实测）

```sh
R=/home/devali/repos/unisacc
mkdir -p /tmp/sh1/include && printf '#error SHADOWED\n' > /tmp/sh1/include/stdio.h
printf '#include <stdio.h>\nint main(void){return 0;}\n' > /tmp/sh1/a.cx
printf '#include "stdio.h"\nint main(void){return 0;}\n' > /tmp/sh1/q.cx
cd /tmp/sh1 && sh $R/unisacc.com a.cx      # rc=1 reject: not covered: a preprocessing diagnostic inside an included header
cd /tmp/sh1 && sh $R/unisacc.com q.cx      # rc=1 同上
mkdir -p /tmp/sh2 && cp /tmp/sh1/a.cx /tmp/sh2/
cd /tmp/sh2 && sh $R/unisacc.com a.cx      # rc=0（cwd 无 include/，改用内嵌头）
```

strace（`-e trace=file`）显示产品按相对路径 `stat("include/stdio.h")`、`open("include/stdio.h", O_RDONLY)`：
cwd 有 `include/` 就命中（`st_size=73454`，正是树里 `include/stdio.h`），没有则 ENOENT 后走内嵌。
`include/cx.h`、`include/cx-flat.h`、`include/unisacc_ffi.h` 走同一机制。

## 二、搜索顺序（实测）与 C99/常见编译器对照

| 顺序 | 实测 | 证据 |
|---|---|---|
| 1 | 源文件目录（仅 `"..."`） | `/tmp/sh5/stdio.h` 空、`/tmp/sh5/include/stdio.h` 为 `#error`：`#include "stdio.h"` rc=0，`#include <stdio.h>` rc=1 |
| 2 | `-I` 目录 | 遮蔽 cwd + `-I /tmp/sh4`（其中 `stdio.h` 空）→ rc=0，`-I` 先于 cwd/include |
| 3 | **cwd 相对 `include/`**（缺陷点） | 第一节 |
| 4 | 内嵌头 | 干净 cwd rc=0，strace 无其它 include 打开 |

C99 6.10.2：`"..."` 搜"当前文件目录"，`<...>` 只搜实现定义的系统目录；GCC/Clang 的系统目录是编译期固化绝对路径，
从不含 cwd。产品把相对 `include/` 当系统目录 → 用户项目下的同名头静默顶替系统头，属产品缺陷。

## 三、范围数字

| 项 | 数字 | 来源 |
|---|---|---|
| 注册批次总行 | 369 | `awk 'NR>1&&$0!~/^#/' tests/batches.tsv \| wc -l` |
| mode=SOURCE 且 IMPLEMENTED | **355** | `tools/affected.cx` 的 `scanned=355` |
| 直接写 `#include <...>` 的 `tests/*.cx` | 10 个文件 | `grep -l '#include <' tests/*.cx` |
| 直接写 `#include <...>` 的 `tests/cases/**` | 15 个文件 | `grep -rl '#include <' tests/cases` |
| 每次批次运行 `stat("include/stdio.h")` | 4 次（1 ENOENT + 3 命中树文件），`include/cx.h` 1 次 | `strace -f -e trace=file` 跑 `check-stdio`（3924 行日志） |

受影响面是**全部 355 个 SOURCE 批次**：产品对每个编译单元都先探测 `include/{cx.h,cx-flat.h,stdio.h}`。

## 四、验证失真（全部实测）

### 4.1 树外副本：改头不装产品，批次结论照样变

```sh
mkdir -p /tmp/r2 && cd /home/devali/repos/unisacc
git archive HEAD -- include tests tools cx exec seed | tar -x -C /tmp/r2
git archive HEAD -- ext | tar -x -C /tmp/r2          # tools/cxlib/reaper.cx -> cx/sys/reaper.cx -> ext/sysx/reaper.cx
cp unisacc.com /tmp/r2/ && cp -r unisacc.com.dev /tmp/r2/ && mkdir -p /tmp/r2/out
printf '\n#error SHADOWED_BY_TREE_COPY\n' >> /tmp/r2/include/stdio.h
cd /tmp/r2 && sh /tmp/r2/unisacc.com /tmp/r2/tools/check-stdio.cx -- /tmp/r2   # rc=1
git -C /home/devali/repos/unisacc show HEAD:include/stdio.h > /tmp/r2/include/stdio.h
cd /tmp/r2 && sh /tmp/r2/unisacc.com /tmp/r2/tools/check-stdio.cx -- /tmp/r2   # rc=0 CHECK_STDIO failed=0
```

### 4.2 外层 cwd 也会被读

```sh
mkdir -p /tmp/include && printf '#error OUTER_CWD\n' > /tmp/include/stdio.h
cd /tmp && sh $R/unisacc.com $R/tools/check-stdio.cx -- $R   # rc=1（读到 17 字节的 /tmp/include/stdio.h）
rm -rf /tmp/include
cd /tmp && sh $R/unisacc.com $R/tools/check-stdio.cx -- $R   # rc=0
```

strace 显示 rc=1 那次是单 pid 3022052 在 cwd=/tmp 相对打开 `include/stdio.h`（17 字节）即拒绝，未进子编译。

### 4.3 未核实项已逐条实测（cc 第 4 条，全部在 `/tmp/r2`，用现装产品）

| 批次 | 基线（树头完好） | 污染 `include/stdio.h`（追加 `#error`） | 污染 `include/unisacc_ffi.h` |
|---|---|---|---|
| `check-errno` | `CHECK_ERRNO failed=0` | rc=1 `reject: not covered: a preprocessing diagnostic inside an included header` | 未跑（与 ffi 无关） |
| `check-libc-probes` | `CHECK_LIBC_PROBES failed=2`（putc 两行，已知） | rc=1 同上 | 未跑 |
| `check-ffi-bridge` | `FFI_BRIDGE assertions=8 failed=0` | rc=1 同上 | rc=1 同上 |

三条都随 `ROOT/include` 的头内容翻转 → **头来源确实是树**，与产品内嵌头无关。三批次在仓内正常跑的结果不变
（`failed=0` / `failed=2` / `assertions=8 failed=0`）。

## 五、修法（按 cc 裁定重写）

目标：系统头默认**只用内嵌**；不搜 cwd/include，也不搜 exe 目录；开发用树头必须显式 `-I`。

### 5.1 改哪几处

| # | 位置 | 改动 | 车道/等级 |
|---|---|---|---|
| 1 | `src/stages.cx`（`st_dirs`、`dv_cxh`、`st_pub` 的 `cli/include-dir`/`cli/nostdinc` 一带） | 默认头目录列表置空；只把显式 `-I`/`-iquote` 交给模型；`nostdinc` 语义简化为"默认目录本就为空" | 快车道 S（路径解析层，不写模型特判） |
| 2 | 默认标准头目录的**来源**（事实表 `exec/facts/*` 或 `exec/pp/*` 的 std-dir 行） | 删掉相对 `include` 项；只保留内嵌头表 | 慢车道 M（表→生成器→网，跨六目标一致） |
| 3 | `include/cx.h`、`include/cx-flat.h` 的探测（同一 `dv_cxh` 路径） | 与 stdio 同处理：不搜 cwd、不搜 exe 目录；需要时用 `-include cx.h` 显式给出 | 快车道 S |
| 4 | exe 目录回落 | **不新增** exe 目录搜索（cc 理由：exe 旁通常没有 include/，多一条路径只是多一个遮蔽来源） | — |
| 5 | 文档/手册 | `include/` 的隐式可用性从文档删除；写明"树头用 `-I <root>/include`" | 快车道 S |

开发便利：可以给 `--tree-headers` 糖，等价于显式 `-I <ROOT>/include`，但必须出现在命令行里、可见可审计，
不做任何隐式默认。

### 5.2 不属于本次修法

- 不改 `"..."` 的源目录优先（C99 正确，实测也正确：本地空 `stdio.h` 胜过 `include/` 的 `#error`）。
- 不改 `-I` 的优先位置（实测 `-I` 已先于 cwd/include）。
- 不在 `drive.cx`/`host.cx` 写平台特判；六目标差异只允许出现在表里。

## 六、批次头来源声明（cc 第 2 条）

### 6.1 注册表字段

`tests/batches.tsv` 增加一列 `headers`，取值两个：`product`（缺省，缺列即 product）、`tree`。

- `cxlib.testplan` 同步解析：未知值报 `unknown headers` 并使 registry 校验失败；缺列按 `product`。
- 该列参与 `--check`，避免手写错值静默通过。

### 6.2 `tests/run-batch.cx` 的行为

| headers | spawn 形态 | 证明什么 |
|---|---|---|
| `product`（默认） | 产品绝对路径调用；cwd = `out/headers-product-<id>/`（仓外临时目录）；**不传** `-I <root>/include` | 只可能读内嵌头 |
| `tree` | 产品绝对路径调用；cwd 同样为仓外临时目录；显式传 `-I <root>/include` | 显式读树头，且 cwd 不参与 |

两种模式 cwd 一致（都仓外），避免 cwd 成为第三个变量。

### 6.3 证据行

- run-batch 每批打印：`BATCH_HEADERS<TAB>id<TAB>product|tree`。
- `BATCH_RUN` 行追加 `headers=` 字段。
- `tests/run.cx` 汇总追加 `headers_product=N headers_tree=M`。
- 候选验收规则：`headers=tree` 的批次**不计入**候选验收结论；候选验收清单里的头相关批次必须是 `product`，
  且 `BATCH_HEADERS` 行可查。

### 6.4 哪些批次必须 `tree`（显式树头）

| 批次/工具 | 原因 |
|---|---|
| `tests/cxinclude.cx`（棘轮 `cxinclude`） | 直读树 `include/` 统计 include 行 |
| `carried-header-export`（工具）与读 `include/` 的 libneed 表导出 | 产物是树头哈希与 libneed 表，输入就是树头 |
| `pp-cli-includes`、`pp-header-locations` | 预处理域批次，输入包含树头文件位置 |
| 新的 `check-include-shadow.cx` 的 `tree` 模式用例（第 7.2 节断言 5、6） | 验证"显式 -I/源目录优先"必须真读树/本地头 |
| 开发期"改头验证"定点批次（改 `include/stdio.h` 后手工复跑的任何批次） | 需要看新头内容 |

其余全部（含 `stdio`、`stdio-close`、`library-abi`、`cx-ffi-bridge`、`check-errno`、`check-libc-probes`、
`check-ffi-bridge`、`check-stdio`）默认 `product`：它们验的是**产品对头的处理**，必须验包内副本。

## 七、`tools/check-include-shadow.cx` 断言表

形态：`sh ./unisacc.com tools/check-include-shadow.cx -- ROOT`，全部用例在 `out/check-include-shadow/` 建临时目录，
逐例 `PASS/FAIL<tab>名称`，末行 `CHECK_INCLUDE_SHADOW	failed=N`，rc=1 当 failed>0。
本探针的 `tree` 类用例（5、6）登记批次时标 `headers=tree`，其余 `product`。

| # | 断言名 | 布置 | 期望（修复后） | 修复前 |
|---|---|---|---|---|
| 1 | `shadow-ignored-angle` | cwd 有 `include/stdio.h`=`#error`，源用 `<stdio.h>` | rc0 | 红（rc1） |
| 2 | `shadow-ignored-quote` | 同上，源用 `"stdio.h"` | rc0 | 红（rc1） |
| 3 | `shadow-ignored-cx` | cwd 有 `include/cx.h`=`#error`，编译任意程序 | rc0 | 红 |
| 4 | `shadow-ignored-ffi` | cwd 有 `include/unisacc_ffi.h`=`#error` | rc0 | 红 |
| 5 | `explicit-I-still-shadows` | 同 1，但显式 `-I include` | rc1（具名拒绝） | 绿 |
| 6 | `source-dir-first` | 源目录 `stdio.h` 空 + `include/stdio.h`=`#error`，源用 `"stdio.h"` | rc0 | 绿 |
| 7 | `clean-cwd` | 无 `include/` | rc0 | 绿 |
| 8 | `no-exe-dir-search` | 产品复制到临时目录，旁边放 `include/stdio.h`=`#error`，cwd 干净 | rc0 | 红（当前会搜 exe 目录/继承路径） |

修复前预期 `failed≥5`（1、2、3、4、8），修复后 `failed=0`。修复前可作为红证据留存，**不登记批次**。

## 八、落地时机

- 全部改在 **W5 之后的下一候选**，不进当前 W5 纪元（W5 只做 lower 卡点与 64→128 上限，不叠加头路径变更，
  避免两个身份扰动同批）。
- 顺序：A1 快车道（`src/stages.cx` 默认目录置空）→ 六目标表行修正（慢车道）→ run-batch 的 `headers` 列与证据行
  → 各批次的 `headers` 值登记 → `check-include-shadow.cx` 转绿并登记批次。
- 每步都要跑：固定输入镜像 SHA 不变、`-S` 逐字节相同、棘轮 9/9（`cxapis` 319 / `coreapi` 193）、
  `tests/run.cx -- --smoke`。

## 九、未核实

- 4.2 驱动侧读取的具体函数（产品主进程自身读 vs `np_run` 的 chdir 时序）未定位。
- 内嵌头在源码中的存放形式（二进制内嵌 vs 模型表）未定位；strace 只证明"树文件缺失时仍成功"。
- `check-errno` 等三批次已在 `/tmp/r2` 实测随树头翻转（第 4.3 节），但"仓内跑等于验树头"这一说法对**每一**批次
  未逐条验证，属同一 spawn 口径的推论。
