# unisacc 0.0.45 —— 发布说明

> **预发布（prerelease），不是 Latest。** 六个平台的 identity、version、hello、run、smoke 都有同候选原生证据；其中 stdio-crossunit 与 stream-pipe 在 macOS 和 Windows 上有部分未跑，全批会话也没有对本候选重跑。详见下文，不夸大。

## 是什么

unisacc 是一个 C99 编译器，自己写出可执行文件（ELF、Mach-O、PE），能编译自身，整个工具链打成一个文件 `unisacc.com`（APE：加载头、ISA 切片、一个模型包）。

本版：
- APE `unisacc.com`：sha256 `ae0b12d8c822efa44d6724782f32a275b4d3fb2cdb3387178cbd76541972c6f0`，6,128,709 字节
- 源码：`fa0a747c7f22ba20c0a875183108106afff21f4a`（tag 所在提交与它的产品输入相同，之间只有工具、测试和文档提交）
- `sh ./unisacc.com --version` 打印 `unisacc 0.0.45`

## 支持哪些平台

在 GitHub 原生 runner 上实测（workflow `candidate-native`，运行 42191390455，分支 `ci/c45-2f`）。汇总：`pass=36 fail=0 not_run=6`。

| 目标 | identity | version | hello | run | smoke | stdio-crossunit | stream-pipe |
|---|---|---|---|---|---|---|---|
| Linux x86-64 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| Linux arm64 | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| macOS x86-64 | PASS | PASS | PASS | PASS | PASS | NOT_RUN | PASS |
| macOS arm64 | PASS | PASS | PASS | PASS | PASS | NOT_RUN | PASS |
| Windows x86-64 | PASS | PASS | PASS | PASS | PASS | NOT_RUN | NOT_RUN |
| Windows arm64 | PASS | PASS | PASS | PASS | PASS | NOT_RUN | NOT_RUN |

未跑的检查及原因：
- macOS 的 stdio-crossunit：runner 没有 `/dev/full`，该用例需要它构造写失败。
- Windows 的 stdio-crossunit 与 stream-pipe：这两个批次需要 POSIX `fork` 和进程组，Windows 上还没有对应的树看门狗与路径适配。

**Windows arm64 的执行体是随发的独立切片文件，不是 `unisacc.com` 自身。** 该格原生运行的是资产 `slice-win-arm64`（sha256 `380e28700a1a2595b7fda3cff563452efeb40b361c22c4829fc3f4a2eda85b25`，3,299,840 字节），配合资产 `pkg.bin`（sha256 `15d8af5844459de26e0ad7feec1564deb1cd1e501d58ab6a964097e7087208ec`，1,662,149 字节），通过环境变量 `UNISA_CONTAINER` 指向它。APE 内部还没有 win/arm64 的分发，所以 Windows arm64 用户目前要用这个独立切片文件。

另外，Windows x86-64 上的 FFI 探针 `strdup` 与 `pread` 的输出与 Linux 逐字节相同。

## 本版相对 0.0.44-rc3 的主要变化

- Windows：修复启动即崩溃（未初始化变量、`long` 在 LLP64 下装指针、PE 头缺 `DYNAMIC_BASE`）；默认 `-run` 改走临时 exe 加 CreateProcess；临时目录统一走 `sy_tmpdir`；`#include` 的盘符绝对路径（`"C:/…"`）可用；`open`、`read`、`strdup` 等 POSIX 名对 UCRT 下划线导出名的映射改为事实表驱动（`exec/facts/libc99posix.tsv`）。
- Windows arm64：第一次有同候选原生证据。
- 修复 `_Static_assert` 之后字符串字面量取错（rc3 的已知限制）。用 `tests/cases/sa-pool.cx` 在本候选上实测五条全部通过。
- 版本号 0.0.45。
- 工具：`tools/install-ape.cx`，把 APE 原子安装到仓根并保留旧脚本，可回滚。

## 已知限制

- **全批会话没有对本候选重跑。** 最近一次完整的全批会话属于前一个候选 c45-1（lnx/x86_64）：418 批已实现，292 PASS，58 FAIL，1 UNKNOWN，67 MISSING，另有 68 批因材料未登记而未判定。失败项均有登记原因（`tests/waivers-0.0.44.tsv`）。本版因此只作预发布，原生 runner 证据仅覆盖上表列出的检查，不是通过率。
- 部分非法 C 会被静默接受，产物不可信：22 种 C99 约束违例不报错，其中 12 种可能产出错误代码。约束检查请另用 `gcc` 或 `clang -std=c99 -pedantic-errors`。（沿用 rc3，本轮未复跑。）
- `-c` 分开编译再链接：运行库状态不跨对象共享；两个及以上带 stdio 的对象一起链接只告警。要共享 `FILE` 请一次编译所有源。（沿用 rc3。）
- 手工拼 `unisacc stream` 管道：tape 无结束标记，上游截断时下游可能接受不完整输入；请用 `set -o pipefail`。正常 `-o` 与默认运行不受影响。（沿用 rc3。）
- 预处理器接受两种非法输入（宏名不是标识符、`#include` 头名未闭合）。（沿用 rc3。）
- `-c -b osx/*|win/*` 文档写拒绝，实际会编出对象。（沿用 rc3。）
- 运行库在每个对象里各带一份（约 264KB），未去重。（沿用 rc3。）
- `#include <C:/x.h>`（尖括号）不按盘符绝对路径处理，只有引号形式 `"C:/x.h"` 走绝对路径。
- 对负载敏感的检查 `cx-procguard`：并发争用时曾假红，现在对同一探针最多重试 5 次，断言未放宽。
- APE 内尚无 win/arm64 分发（见上）。

## 怎么验证

1. 核哈希：`sha256sum unisacc.com` 应为 `ae0b12d8c822efa44d6724782f32a275b4d3fb2cdb3387178cbd76541972c6f0`。
2. `sh ./unisacc.com --version` 输出 `unisacc 0.0.45`。
3. 冒烟：`printf 'int main(void){return 7;}\n' > t.c && sh ./unisacc.com t.c; echo $?` 应为 7。
4. 各平台原生证据：`candidate-native` 的运行 42191390455，每格证据表在其 `evidence-<os>-<arch>` 产物里。
5. Windows arm64：设 `UNISA_CONTAINER` 指向 `pkg.bin`，运行 `slice-win-arm64`（需先核两者的 sha256）。
