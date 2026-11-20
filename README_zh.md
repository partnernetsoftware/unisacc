[English](README.md) | 中文

<!--
maintenance: keep README.md and README_zh.md in lockstep: same section ids in the same order, same link targets, same table rows; edit both in the same commit.
section correspondence (id -> English heading -> Chinese heading):
  what     -> unisacc            -> unisacc
  builtin  -> Built-in capabilities -> 内置能力
  download -> Download and verify -> 下载与校验
  commands -> Common commands    -> 常用命令
  index    -> Index              -> 索引
  targets  -> Targets            -> 目标
-->

<a id="what"></a>
# unisacc

C99 编译器,六个目标 {Linux, macOS, Windows} x {x86-64, arm64}:自己的编码器直接写出 ELF、Mach-O(ad-hoc 签名)、PE,能自举,整个产品是一个文件 `unisacc.com`。默认产品是模型驱动的:事实表 -> 生成器 -> 通用 VM 上执行的转移/动作程序,不是训练所得。当前版本、资格与已知缺口不在本页,见下表的 prd 系列文档与发布页。

<a id="builtin"></a>
## 内置能力

以 0.0.84 公开件实测(`unisacc 0.0.84`,APE 8417722 字节,Linux x86-64 宿主;POSIX 与 asm 面数字为 0.0.82 实测,0.0.83 与 0.0.84 未重测,0.0.83 修复项见 [release/notes-0.0.83.md](release/notes-0.0.83.md),0.0.84 见 [release/notes-0.0.84.md](release/notes-0.0.84.md))。详见 [prd.md 1](prd.md#1-unisacccom-是什么)、[docs/readme-detail.md 二](docs/readme-detail.md#二使用手册命令选项头文件限制)。

| 能力 | 是什么 | 实测现状与限制 |
|---|---|---|
| (1) 内置 libc | C99 头文件与运行库随 `unisacc.com` 自带,不依赖宿主 libc 头;`-ftrim-libc`(默认开)只保留程序用到的库体。 | 缺 `regex.h`、`strings.h`。Linux 上库直接走内核调用。 |
| (2) 内置 POSIX 子集 | Linux 与 macOS 上:`unistd.h`、`sys/stat.h`、`fcntl.h`、`poll.h`、`termios.h`、`dirent.h`、进程与 `sys/wait.h`、IPv4 套接字、无 DNS 的 `netdb.h`、时间函数。Windows 对这些头具名拒绝。 | POSIX 实测(46 探针对 gcc 逐字节对拍):完全通过 17/29,2 项行为缺陷(`sigprocmask(SIG_BLOCK)` 未屏蔽;`extern char **environ` 为 NULL),10 项拒绝或缺失。线程、同步、原子与 TLS 未实现。asm 面 4/17;以上为 0.0.82 实测。0.0.83 已修 sigprocmask 屏蔽、environ 与 wchar_t 守卫三项,基本 `__asm__` 与白名单外属性改为具名拒绝(原静默忽略);修后 POSIX 与 asm 面未整体重测。 |
| (3) cx | C 语言扩展:用 `#pragma unisa module NAME` 与 `#pragma unisa use MOD[::sym [as alias]]` 代替 `#include`;分层库 `sy_*`、`mm_*`、`tx_*`;所有权与边界为运行期检查,不是编译期。 | 规范见 [prd_cx_spec.md](prd_cx_spec.md)、[prd_cx.md](prd_cx.md)。模块名必须等于文件名主干。 |
| (4) 脚本式运行 | 默认即 `-run`:`.c` 或 `.cx` 在内存中编译并运行,不落盘;程序参数放在 `--` 之后。 | 只有显式 `-o` 才写文件。 |

另:一个 APE 文件在 Linux 与 macOS(x86-64、arm64)上直接运行;为六目标写出 ELF、Mach-O、PE;`-c` 与 `-S` 输出目标文件与汇编。

实跑(0.0.84):

```text
$ sh ./unisacc.com --version
unisacc 0.0.84
$ sh ./unisacc.com h.c a b          # h.c 打印 argc
hello c argc=3
$ sh ./unisacc.com hello.cx -- a b  # #pragma unisa use text.text::tx_len
hello cx argc=3 len=5
$ sh ./unisacc.com h.c -o h && ./h x
hello c argc=2
$ sh ./unisacc.com h.c -b win/x86_64 -o h.exe   # file: PE32+ executable (console) x86-64
```

<a id="download"></a>
## 下载与校验

发布页:<https://github.com/partnernetsoftware/unisacc/releases>。下载 `unisacc.com` 与同页的 `SHA256SUMS`,校验后使用:

```sh
shasum -a 256 -c SHA256SUMS --ignore-missing   # 只核对已下载的文件
```

卸载即删除下载的文件;回退即恢复上一版 Release 的文件。macOS 与 Windows 的启动方式和已测范围写在各版发布说明里。

<a id="commands"></a>
## 常用命令

```sh
sh ./unisacc.com hello.c [args]               # 默认 -run:内存中编译并运行
sh ./unisacc.com hello.c -o hello             # 写本机可执行文件
sh ./unisacc.com hello.c -b osx/arm64 -o hello # 指定目标
sh ./unisacc.com -E file.c                    # 只预处理;-c/-S 的目标文件与 tape 行为见使用手册
sh ./unisacc.com tools/FILE.cx -- ARGS        # 工具与脚本都是 .cx,同样默认 -run
sh ./unisacc.com tests/run.cx -- --smoke      # 测试入口;另有 --all、--list
```

<a id="index"></a>
## 索引

新来者阅读顺序(按角色;其余文件按需查,不是首页):

- 使用者:本页 -> 条款与限制查 [prd_spec.md](prd_spec.md)、[prd_release.md](prd_release.md)。
- 贡献者:本页 -> [prd_arch.md](prd_arch.md) -> [prd.md 2 能力速查](prd.md#2-有什么功能已实现能力速查) -> 写 `.cx` 读 [prd_cx_spec.md](prd_cx_spec.md),结构疑难查 [prd_cx.md](prd_cx.md) -> 术语查 [docs/glossary.md](docs/glossary.md) -> 当前版本查 [plans/INDEX.md](plans/INDEX.md)。
- 协作 agent:[AGENTS.md](AGENTS.md) -> [prd_mempalace.md](prd_mempalace.md) -> [prd_cx_spec.md](prd_cx_spec.md) -> [plans/devsvc.md](plans/devsvc.md);[prd_hist.md](prd_hist.md) 与 [docs/readme-detail.md](docs/readme-detail.md) 的数字已冻结,不当现行状态。
- 发布者:[prd_mempalace.md](prd_mempalace.md) -> [prd_release.md](prd_release.md) -> prd.md 2 发布工具行 -> [plans/INDEX.md](plans/INDEX.md) 现行版本计划。

每行一跳到达。

| 主题 | 去哪里 |
|---|---|
| 当前状态、思维树(每轮先读) | [prd.md 9.1 思维树](prd.md#work-tree) |
| 记忆宫殿(全部入口的唯一索引) | [prd.md 9.2 记忆宫殿](prd.md) |
| 构造与链工具、能力速查 | [prd.md 2 能力速查](prd.md#2-有什么功能已实现能力速查) |
| 测试、棘轮(13 项)、批次 | [prd.md 2.2 测试](prd.md#22-测试棘轮门禁批次)、[prd.md 7 测试套件设计](prd.md#7-测试套件设计)、[tests/README.md](tests/README.md) |
| 结构五层、六片、七流、十八网、执行器 | [prd.md 3 结构组成](prd.md#3-结构组成unisacccom-的内部) |
| 架构与文件关系 | [prd_arch.md](prd_arch.md) |
| 仓库目录与"我想做 X 去哪里" | [prd.md §8.1](prd.md#repo-map) |
| cx 语言与 cx 侧结构 | [prd_cx.md 0 cx 核心 PRD](prd_cx.md#0-cx-核心-prd定稿-2026-11-12)、[cx/spec.md](cx/spec.md)、[docs/cx/prd.md](docs/cx/prd.md) |
| cx 设计文档 | [design-core](docs/cx/design-core.md)、[stability](docs/cx/stability.md)、[ref-cpython](docs/cx/ref-cpython.md)、[ref-node](docs/cx/ref-node.md)、[ref-quickjs](docs/cx/ref-quickjs.md)、[ref-rust](docs/cx/ref-rust.md) |
| 发布流程与 CI 手册 | [prd_release.md 1 发布链](prd_release.md#1-发布链顺序即依赖)、[release/README.md](release/README.md) |
| 往期发布与历史记录 | [prd.md 9.3 往期发布](prd.md)、[prd_hist.md](prd_hist.md) |
| 工具链内部(目标文件、汇编、链接) | [docs/toolchain.md](docs/toolchain.md)、[tools/kernels.md](tools/kernels.md)、[tools/packages.md](tools/packages.md) |
| 格式与互操作 | [docs/tapebin-v1.md](docs/tapebin-v1.md)、[docs/host-interop.md](docs/host-interop.md)、[docs/glossary.md](docs/glossary.md) |
| 任务看板、账本、工作流 | [prd_mempalace.md](prd_mempalace.md)(唯一协同入口;`archive/agentwork-retire-2026-11/` 已退役,仅作历史) |
| agent 工作规则 | [AGENTS.md](AGENTS.md#unisacc-工作规则)(`CLAUDE.md` 是其软链) |
| 当前版本计划与发布说明 | [plans/INDEX.md](plans/INDEX.md)(现行版本计划行)、[release/README.md](release/README.md) |
| 先验工作综述 | [research/prior-art.md](research/prior-art.md) |
| 详细使用手册(中文)与已知限制:全部选项、头文件、`-c`/`-S`/对象与归档 | [docs/readme-detail.md 二](docs/readme-detail.md#二使用手册命令选项头文件限制)、[七](docs/readme-detail.md#七已知限制) |
| 冻结的历史说明(中文):决策网表、旧测试套件表、语料观测、C99 覆盖表 | [docs/readme-detail.md](docs/readme-detail.md) |

<a id="targets"></a>
## 目标

| 系统 | x86-64 | arm64 |
|---|---|---|
| Linux | 是 | 是 |
| macOS | 是 | 是 |
| Windows | 是 | 是(经 `slice-win-arm64` 与 `pkg.bin`,见发布说明) |

各目标的资格与未测范围只以发布页与 prd_release.md 为准;本页不声称 Windows 原生自举。
