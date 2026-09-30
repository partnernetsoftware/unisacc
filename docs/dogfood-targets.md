# 狗粮靶子清单：自家组件从 cosmocc / tinycc 切到 unisacc（R14-7 落点 B，2026-09-30）

> 盘点范围：`~/repos/minicon`、`~/repos/agenterm`（含 archive/research 目录）。方法：检索 `cosmocc|tinycc|tcc`，逐个读引用它们的构建脚本与 C 源，列出依赖的语言面与库面，对照本仓 README 限制表与 [C99 条款账本](../tests/c99/clauses.tsv)。

## 1　盘点结果

| 组件 | 位置 | 现用工具链 | 规模 | 作用 |
|---|---|---|---|---|
| **minicon 六格 APE 启动器** | `minicon/loader/loader.c`（`pack.sh` 调 `cosmocc -Os -static`） | cosmocc | 355 行 C | 一个 `minicon.com` 内嵌六个原生格（ZipOS `/zip/cells/…`），运行时按 `{os}-{isa}` 取出、落盘、`posix_spawn` 执行 |
| **agenterm 六格 APE 启动器** | `agenterm/research/agenterm-com-loader/loader.c` | cosmocc | 与上同构 | `agenterm-ape.com`，同一设计的复制 |
| 其余 C 源 | agenterm `examples/c/*`（libagenterm C ABI 探针）、`crates/agenterm-platform/native/process_inspection.c`、minicon `archive/research/*` | 系统 cc / cargo cc | — | 不经 cosmocc/tinycc；不是本项的切换对象（可作为后续“替换系统 cc”的候选） |

两个项目里 **tinycc 没有被使用**；**cosmocc 只用于同一个组件**：六格 APE 启动器。

## 2　启动器依赖面 vs 本产品（0.0.13 + R14-0/R14-1）

| 依赖 | 启动器用法 | unisacc 现状 |
|---|---|---|
| 语言 | C99 常规写法（结构体、snprintf、数组、指针） | 覆盖（语言条款 96%） |
| `<stdio.h> <stdlib.h> <string.h> <errno.h> <signal.h> <dirent.h>` | snprintf、getenv、strerror、`opendir/readdir` | 有头；`kill`/`signal` 语义未验证 |
| `<unistd.h> <fcntl.h> <sys/stat.h> <sys/types.h> <sys/wait.h>` | `open/read/close/unlink/rename/chmod/stat/lstat/getpid/waitpid` | **`unistd.h`/`fcntl.h`/`sys/stat.h` 缺**（限制表已列）；`sys/types.h` 有 |
| `<spawn.h>` | `posix_spawn(&pid, dst, NULL, NULL, argv, environ)` | **缺**（Windows 上需 CreateProcess 映射） |
| `<cosmo.h>` | `IsWindows/IsXnu/IsLinux/IsAarch64`；ZipOS（`/zip/cells/<cell>` 当文件读） | **无对应物**：平台判定可用编译期宏（本产品按目标选宏）或运行时探测；内嵌负载需要自有方案 |

## 3　首个靶子与切换方案

**靶子：minicon 六格 APE 启动器**（agenterm 同构，切完一份即复用）。理由：唯一的 cosmocc 使用者；规模小；功能单一可验收；正好对应 unisacc “一个 .com 跑六平台”的定位。

**缺口（切换前必须补）**，按依赖顺序：
1. **POSIX 最小面（FX-5 L2 的第一批）**：`unistd.h`（open/read/write/close/unlink/rename/getpid）、`fcntl.h`（O_* 常量）、`sys/stat.h`（stat/lstat/chmod/mkdir）、`sys/wait.h`（waitpid）、`spawn.h`（posix_spawn）——lnx/osx 走现有 `.sys/.sys6` 系统调用门，win 映射到 Win32（CreateFile/CreateProcess/WaitForSingleObject）。每个函数一个红例、六目标对拍。
2. **内嵌负载**：替代 ZipOS 的最小方案——APE 写出器支持把若干命名数据块附在镜像尾部并提供 `unisacc_blob(name, &len)` 取回（格式写进 docs，内容哈希记入 build.json）；或者保留外部目录模式（启动器已支持 `MINICON_COM_CELLS` 环境变量）先切编译器、后切内嵌。
3. **平台判定**：用本产品已有的目标宏（`__APPLE__/__linux__/_WIN32/__aarch64__/__x86_64__`）替代 `IsXnu()` 等——六个目标各落一个镜像；若坚持“一个 .com 内运行时判定”，需要 APE 层暴露运行时平台号（与 origin_target 同一枚举）。

**切换步骤**：① 在 minicon 仓加 `loader/pack-unisacc.sh`：`unisacc.com loader.c -b <t> -o dist/minicon-loader-<t>` × 6（外部目录模式）；② 六格门禁（minicon 已有 `six-cell-smoke`）对 unisacc 产物跑一遍；③ 缺口 2 落地后切内嵌模式，与 cosmocc 产物做功能对拍（同一组 cells、同一组参数、同退出码/输出）；④ 两种产物并存一个版本，门禁同绿后移除 cosmocc 路径。

**验收**：六个目标上启动器用 unisacc 构建、`six-cell-smoke` 全绿；构建产物字节可复现（两次构建同 sha，sha 记入 minicon 的 build receipt）；cosmocc 路径保留为对照直到 ③ 通过。

## 4　排期

本版（0.0.14）只交付本清单（R14-7 落点 B 的验收即“定靶 + 列清单 + 写步骤”）。切换本身依赖缺口 1、2，工作量明显超过 1 天，进入 0.0.15 计划：**R15 候选：POSIX 最小面 L2 + 内嵌负载 + minicon 启动器切换**。
