# cwd `include/` 遮蔽系统头：范围、证据、修法（只读调查）

日期：2026-11-02，omp-ds。冻结期只读；所有样本跑 `/tmp`，未改 `tools tests cx src include weights exec`。

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

strace（`-e trace=file`）显示产品按**相对路径** `stat("include/stdio.h")`、`open("include/stdio.h", O_RDONLY)`：
cwd 有 `include/` 就命中该文件（`st_size=73454`，正是树里 `include/stdio.h` 的大小），cwd 没有则 ENOENT 后走内嵌。
`include/cx.h`、`include/cx-flat.h` 走同一机制。

## 二、搜索顺序（实测）与 C99/常见编译器对照

| 顺序 | 实测 | 证据 |
|---|---|---|
| 1 | 源文件目录（仅 `"..."`） | `/tmp/sh5/stdio.h` 空、`/tmp/sh5/include/stdio.h` 为 `#error`：`#include "stdio.h"` rc=0（本地空文件胜），`#include <stdio.h>` rc=1（角括号不搜源目录） |
| 2 | `-I` 目录 | 遮蔽 cwd + `-I /tmp/sh4`（其中 `stdio.h` 为空）→ rc=0，说明 `-I` 先于 cwd/include |
| 3 | **cwd 相对 `include/`**（缺陷点） | 本文件第一节 |
| 4 | 内嵌头 | 干净 cwd rc=0，且 strace 无其它 include 打开 |

C99 6.10.2：`"..."` 搜实现定义的"当前文件目录"，`<...>` 只搜实现定义的系统目录；两者之后才是实现定义的实现目录。
GCC/Clang 的系统目录是**编译期固化的绝对路径**（`/usr/include` 等），从不包含 cwd。产品把相对 `include/` 当成系统目录，
因此用户项目根下只要有同名头（`include/stdio.h`、`include/cx.h`、`include/unisacc_ffi.h` 等），`<...>` 与 `"..."` 都会
静默读到它——行为与所有常见编译器不同，属产品缺陷。

## 三、范围数字

| 项 | 数字 | 来源 |
|---|---|---|
| 注册批次总行 | 369 | `awk 'NR>1&&$0!~/^#/' tests/batches.tsv \| wc -l` |
| mode=SOURCE 且 IMPLEMENTED（affected.cx 口径） | **355** | `tools/affected.cx` 的 `scanned=355` |
| 直接写 `#include <...>` 的 `tests/*.cx` | 10 个文件 | `grep -l '#include <' tests/*.cx` |
| 直接写 `#include <...>` 的 `tests/cases/**` | 15 个文件 | `grep -rl '#include <' tests/cases` |
| 每次批次运行里 `stat("include/stdio.h")` | 4 次（1 次 ENOENT + 3 次命中树文件），`include/cx.h` 1 次 | `strace -f -e trace=file` 跑 `check-stdio`（3924 行日志） |

结论：受影响面不是"那 10+15 个文件"，而是**全部 355 个 SOURCE 批次**——产品对每个编译单元都会先探测
`include/{cx.h,cx-flat.h,stdio.h}`，树里存在即用树文件。cwd 判定另见第四节。

## 四、验证失真（两条证据，均已实测）

### 4.1 树外副本：改头不装产品，批次结论照样变

```sh
mkdir -p /tmp/r2 && cd /home/devali/repos/unisacc
git archive HEAD -- include tests tools cx exec seed | tar -x -C /tmp/r2
git archive HEAD -- ext | tar -x -C /tmp/r2          # tools/cxlib/reaper.cx -> cx/sys/reaper.cx -> ext/sysx/reaper.cx
cp unisacc.com /tmp/r2/ && cp -r unisacc.com.dev /tmp/r2/ && mkdir -p /tmp/r2/out
printf '\n#error SHADOWED_BY_TREE_COPY\n' >> /tmp/r2/include/stdio.h
cd /tmp/r2 && sh /tmp/r2/unisacc.com /tmp/r2/tools/check-stdio.cx -- /tmp/r2
# rc=1 reject: not covered: a preprocessing diagnostic inside an included header
git -C /home/devali/repos/unisacc show HEAD:include/stdio.h > /tmp/r2/include/stdio.h
cd /tmp/r2 && sh /tmp/r2/unisacc.com /tmp/r2/tools/check-stdio.cx -- /tmp/r2
# rc=0 CHECK_STDIO failed=0
```

同一枚现装产品、只换 root 里的 `include/stdio.h`，批次从绿变红再变绿。证明批次验的是 `ROOT/include`（树里的副本），
不是候选包内的头；"改头、没装产品"时检查结果已经变了。

### 4.2 外层 cwd 也会被读（cc 的前提要修正）

仓内批次由 `tests/run-batch.cx:54` 与各 `tools/check-*.cx` 以 `np_run(args, root, ...)` 启动，子编译进程 cwd=root
（strace 里 pid 3016173 读 `include/stdio.h` 命中 73454 字节即树文件）。但**外层 cwd 并非无关**：

```sh
mkdir -p /tmp/include && printf '#error OUTER_CWD\n' > /tmp/include/stdio.h
cd /tmp && sh $R/unisacc.com $R/tools/check-stdio.cx -- $R   # rc=1，读到 17 字节的 /tmp/include/stdio.h
rm -rf /tmp/include
cd /tmp && sh $R/unisacc.com $R/tools/check-stdio.cx -- $R   # rc=0
```

strace 显示 rc=1 那次是单个 pid 3022052 在 cwd=/tmp 相对打开 `include/stdio.h`（17 字节）后即拒绝，未进入子编译。
即产品在驱动/预备阶段**也**按继承 cwd 读头。机制未完全定位（是驱动自身读还是 np_run 的 chdir 时序），
但足以说明：批次的 include 来源有两个 cwd（外层 cwd + root），"仓外 cwd 运行"并不能让批次验到包内副本。

### 4.3 已知受影响的结论

| 批次 | 状态 |
|---|---|
| `check-stdio` | **实测受影响**：外层 cwd 与 root 两处都能翻转结果（4.1/4.2）；仓内正常跑 `failed=0` |
| `check-errno`、`check-libc-probes`、`check-ffi-bridge`、`check-budget` 等所有 `np_run(..., root, ...)` 的 check-* | **未逐条实测**，但同一 spawn 口径与同一 include 解析，属同一风险面；按规则不记 PASS，标 `[未核实]` |
| 全部 355 个 SOURCE 批次 | 同一口径 `[推论]`，未逐条实跑 |

## 五、修法

### 5.1 缺陷本体（优先，快车道）

搜索顺序里第 3 项"cwd 相对 `include/`"应删除。三档改法：

| 档 | 改哪层 | 内容 | 等级 |
|---|---|---|---|
| A1 | 驱动/阶段层（`src/stages.cx` 的 `st_dirs` 与 `dv_cxh` 一带） | 把默认头目录从相对 `include` 改成**可执行文件所在目录的绝对路径**，exe 旁没有 `include/` 时静默回落内嵌；`-I` 仍最先 | 快车道 S；不触模型表，属路径解析 |
| A2 | 事实表 → 生成器 → 网 | 默认系统目录列表进事实表（`exec/facts/*` 或 `exec/pp/*` 的 std-dir 行），生成器产出，模型按表搜索；同时删 cwd 相对项 | **慢车道 M**：改表要重生成与纪元换标 |
| A3 | 仅批次侧 | 把各 `check-*.cx`/`run-batch.cx` 的 `np_run(v, root, ...)` 改成 `np_run(v, tmpdir, ...)`，产品用绝对路径调用 | 治标：只让仓内批次验到内嵌头，**不修用户侧缺陷**；且与 4.2 的外层 cwd 同源，治不了驱动侧读取 |

推荐：A1 先止血（用户项目不再被 cwd 遮蔽），A2 作为 0.0.44 的正式合同（可跨六目标、可 `--check`）。A3 只作为
A1/A2 落地前的批次隔离手段。

注意边界：`include/` 作为相对路径同时被 `include/cx.h`（用户可覆盖 cx.h）、`include/unisacc_ffi.h` 命中——
A1 之后这些覆盖能力只保留给人显式 `-I include` 的场景，不隐式生效。

### 5.2 验证方案

1. **新探针** `tools/check-include-shadow.cx`（修复后登记为批次）：
   - 临时目录建 `include/stdio.h`（`#error`）→ 期望产品 rc=0 且 6 个断言全 PASS（即不再遮蔽）；
   - 同目录 `-I include` 显式给出 → 期望 rc=1（显式 `-I` 仍可遮蔽，是用户选择）；
   - 干净 cwd 编译 → rc=0；`"..."` 源目录优先（本地空 `stdio.h` + `include/stdio.h` 为 `#error` → rc=0）。
   修复前该探针应为 `failed=5` 左右（本文件第一节的现场），可先作为红证据留存，不登记批次。
2. **批次隔离回归**：A1/A2 落地前，用 4.1 的树外副本法抽查 `check-stdio`、`check-errno`、`check-libc-probes`：
   改副本头 → 现状红；修复后应仍绿（证明批次不再读 `ROOT/include`，或证明读的是候选包内嵌）。
3. **字节等价**：修法只动搜索路径，不动编码。固定输入 `int main(void){return 3;}` 用 `-o` 的镜像 SHA
   与现装相同；`-S` 产物对固定输入逐字节相同；棘轮 9/9 与 `cxapis` 319 / `coreapi` 193 不变。
4. **跨目标**：A2 落地时，lnx/x86_64、lnx/arm64、osx×2、win×2 的默认目录行必须一致（表驱动），
   不允许在 `drive.cx` 里按平台写特判。

## 六、未核实

- 4.2 的驱动侧读取机制（是产品主进程自身读，还是 np_run 的 chdir 时序）未定位到具体函数。
- `check-errno`/`check-libc-probes`/`check-ffi-bridge` 在污染 cwd 下的红/绿未逐条实跑，仅按同口径推断。
- 内嵌头的实际来源（二进制内嵌 vs 模型表）未在源码中定位；strace 只证明"树文件缺失时仍成功"。
