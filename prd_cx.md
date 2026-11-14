# prd_cx.md —— cx 侧结构（流、网、编译器模块、包层）

要用 cx 写工具或脚本，先读 [prd_cx_spec.md](prd_cx_spec.md)（使用规范：运行方式、语言要点、标准库速查、替代脚本对照、实测例子）；本文只管结构与路线。

本文件是 [prd.md](prd.md) 的子文档，是 **cx 侧结构点的唯一权威定义**：CX0 流、cx 侧网模块、`cx/compiler` 结构、cx 包层、语言与所有权现状。产品整体结构（五层、六片、七流、金表网、构造链、测试）在 prd.md §3，这里不重复。条款在 [cx/spec.md](cx/spec.md)，pragma 扩展与目录约定在 [cx/prd.md](cx/prd.md)，本文只写结构、规模、现状与路线。

**定位**：cx 是**编译器无关的语言 / 模块层**（模块化 `#pragma unisa module / use`、派生接口、运行期所有权与带界接口），不是 unisacc.com 专有。现有三个宿主：

| 宿主 | 入口 | 怎么处理 `.cx` | 测试入口 | 现状 |
|---|---|---|---|---|
| unisacc.com | `sh ./unisacc.com FILE.cx -- ARGS`（默认 -run）、`-cx-plan` | 产品内 `cx/compiler/cxmod.cx` 规划，七流编译 | `cx0-unit`、`cx0-bundle`、`cx0-e2` 批次（daily），`check-cx-layers`、三个 cx 棘轮 | 日常使用；工具 733 个 .cx 都走它 |
| gcc_cx | `sh ./gcc_cx.com FILE.cx -o OUT`；源 `tools/gcc_cx.cx`（960 B）+ `tools/cxcc.cx`，入口由 `tools/build-cxcc-host.cx` 在本机生成 | 驱动内展开 `use`（链接 `src/cxmod.c`，不调 unisacc.com），强制 `-std=c99 -x c -include include/cx.h` 后交 gcc | 用例 `tests/cxcc_one.cx` 等 5 个；`tests/batches.tsv` 里 grep 不到 cxcc / gcc_cx 批次（未登记） | 仓内与已装根都没有 `gcc_cx.com`（`ls` 实测），本次未跑 |
| clang_cx | `sh ./clang_cx.com FILE.cx -o OUT`；源 `tools/clang_cx.cx`（1001 B） | 同上，交 clang | 同上 | 同上 |

gcc / clang 产物是普通 C 产物，「not a checked .cx build」（gcc_cx.cx 头注）：模块可见性由驱动展开保证，运行期所有权来自 cx 库本身，不来自编译器。三宿主的深度对比分析另文，本文只写现状与索引。

取数口径：2026-11-11，HEAD `6a4fa1fa6f` 私有克隆，已装 0.0.52 APE（sha `1bffabba…`）；每个数后写命令；「未测」即本次没有命令证据。

## 0. cx 核心 PRD(定稿 2026-11-12)

本节是 cx 核心库面(`cx.*` 与 `ext.*`)的需求定稿。逐区设计与依据在 [docs/cx/design-core.md](docs/cx/design-core.md),稳定性与版本策略在 [docs/cx/stability.md](docs/cx/stability.md),四份参照调研在 `docs/cx/ref-rust.md`、`ref-node.md`、`ref-quickjs.md`、`ref-cpython.md`。第 1 节起是 cx 侧结构(CX0 流、网模块、包层、路线),与本节并行有效,节号不变。

取数口径:2026-11-12 对 `build/std-surface.tsv`(v1,412 行)的只读统计。本轮未运行任何检查工具,凡写「现状」均指表内文字,不指实跑结果。

### 0.1 目标

- (1) 上层(产品、工具、测试、应用)写系统程序只用 cx 核心面,不直接碰 libc 与平台 `#if`。
- (2) 六目标(`lnx`、`osx`、`win` 乘 `x86_64`、`arm64`)同面同签名;做不到的返回 `-ENOSYS`,符号不消失。
- (3) 面由事实表定义,变更由工具判类;何时稳定由工具判定,不由项目所有者或任何人裁量。
- (4) 核心保持小:只收别的包要在其上搭建、且需求不再变的接口;其余进 `ext.*`。
- 非目标:线程与事件循环、TLS/HTTP、时区、泛型容器、编译期所有权(CX-W-4 未落地,本面不声称内存安全)。

### 0.2 层与包

| 包 | 高度 | 内容 | 阶段 |
|---|---|---|---|
| `cx.sys` | 系统调用 | fd、管道、轮询、信号观察、tty、平台识别、错误码 | 预稳定 |
| `cx.os` | 便捷 | 文件系统、进程派生、环境、时间与 deadline | 预稳定 |
| `cx.path` | 纯计算 | 词法路径,posix 与 win 双风格 | 预稳定 |
| `cx.mem` | 纯计算 | 堆、带界字节、vec、map;并入 `mg_*`、`ms_*` | 预稳定 |
| `cx.text` | 纯计算 | 带界文本、UTF-8、进制、hex/base64;并入 `tx_snprintf` | 预稳定 |
| `cx.stream` | 组合 | reader/writer 句柄、拷贝、有界管线 | 预稳定 |
| `ext.net` | 系统调用 | 地址、解析、TCP、UDP、Unix 域;目标 `cx.net` | 孵化 |
| `ext.procx`、`ext.memx`、`ext.textx` | 便捷与纯计算 | 输出采集与进程树;arena;字符串构造器与 deflate | 孵化 |
| `ext.crypto`、`ext.tsvx` | 纯计算 | 密码与 TLS 1.3 记录层;TSV 三方合并 | 扩展 |

对项目所有者假设「按 QuickJS 把 sys + os + net 做完整即够」的裁定:成立一半。系统调用高度的面确实可以很小;但 QuickJS 没有套接字,且它不需要 path、text、mem、stream 是因为 JS 自带,所以核心是 sys + os 加四个补位层,net 另从 Rust 与 Node 推导并先孵化。

tlib 侧现状与此不符:`cx.sys`、`cx.os`、`cx.mem`、`cx.text` 仍合在 `cx.base`(连同 `cx/core`、`cx/owned.cx`);`cx.stream` 依赖 `ext.memx`。拆包与并入是稳定声明的前置。

### 0.3 各区面与现状

| 区 | 表内行数 | new | 含拒绝的行 | 含 unverified 的行 | 定稿要点 |
|---|---|---|---|---|---|
| fs | 52 | 3 | 22 | 15 | 整操作函数为首选;流式目录遍历;路径约定 UTF-8;`*at` 族不进承诺面 |
| process | 40 | 1 | 7 | 18 | 唯一入口 `sy_spawn_t` 加 `sy_proc_*`;只收 argv;超时是退出状态字段 |
| os-env | 19 | 7 | 0 | 4 | 平台识别三个入口;能力用 `sy_cap` 查询;改环境只在启动阶段 |
| time | 22 | 5 | 0 | 5 | 单调钟与墙钟分开;一个 `sy_deadline_t`;日历只做 UTC |
| sys | 46 | 6 | 23 | 16 | fd 是整数、默认不继承、EINTR 在 L0 重试;`sy_poll` 是唯一多路入口 |
| path | 26 | 0 | 0 | 0 | 纯词法、双实现;已最接近稳定判据 |
| text | 66 | 20 | 0 | 0 | 字节加长度;只认 UTF-8;可失败转换不静默替换 |
| buffer | 63 | 17 | 0 | 0 | `mm_*`、`mg_buf`、`ms_slice`;容器只有 vec 与字符串键 map |
| stream | 29 | 16 | 0 | 0 | reader/writer 加上限加句柄 deadline;关闭不吞错 |
| net | 31 | 30 | 2 | 10 | 阻塞 TCP/UDP/Unix 域加地址值类型;每个等待带 deadline;留在 ext |

全表合计:exists 308、new 108、别名 3;`unverified` 单元格 71(osx 3、win 68);拒绝单元格 58。设计对表的要求是 129 处行编辑(改 122、增 7)加表级升 v2,清单在 design-core.md 第 14 节,本轮未落表。

### 0.4 错误与资源模型

- 错误只有一种约定:返回值负码(`-errno` 风格),不做带载荷的错误对象,不用全局 `errno`。码在 L0 归一成一套;`sy_err_kind` 按处理方式分类,类清单可增长,调用者须留兜底分支。码是合同,消息不是。「不支持」只有 `-ENOSYS`。
- 资源:裸 fd 是整数,不声称保护;reader、writer、map、子进程、目录是带检查的句柄或调用者拥有的结构,关闭后使用与重复关闭确定返回 `-EBADF`;每个返回资源的行在表里写明配对的关闭函数;`sy_res_live` 让泄漏可断言。
- 阻塞:每个可能无限期等待的入口带 `sy_deadline_t`(绝对单调时刻),到期 `-ETIMEDOUT`;例外只有名字解析与普通文件读写。
- 平台:公开函数体无平台分支,差异在 L0 的 `_os` 静态帮手与表里;拒绝分「按能力拒绝」(`cap=NAME`,封闭词表)与「未实现」,后者阻塞稳定。
- 以上都是运行期检查或调用方合同,不是编译期所有权。

### 0.5 版本与稳定性摘要

- 阶段:孵化(`ext.*`)-> 预稳定(`cx.*`,版本只走 Z,打印本应的 y 或 x)-> 稳定(`1.0.0` 起,单向门)-> 弃用 -> 移除。
- 稳定判据 S1 至 S13:面封口、全部实现、错误合同合文法、零未核、拒绝有名、用例覆盖成功失败与拒绝、六目标同候选实跑、面连续 3 个公开版且 30 天未变、有消费者且产品层 libc 计数为 0、依赖已稳定、所有权合同、机制绿、独立评审入账。
- 判定者是 `tools/stable-eval.cx`(待建):全部 ok 输出 `STABLE-READY`,集成者才提交翻转 `build/tlib-policy.tsv` 的包行并记账本;不请示项目所有者。
- 弃用三段:legacy(可永久)-> deprecated 带不可变编号 `CXDEPnnnn`(至少 3 个公开版且 30 天)-> 移除(X 升级,仓内调用点为 0)。安全例外由集成者决定并入账。
- 新函数在面封口后只能先进 `ext.*`,满足提升判据再进 `cx.*`。

### 0.6 验收门

| 门 | 命令 | 状态 |
|---|---|---|
| 面与导出一致 | `sh ./unisacc.com tools/check-std-surface.cx` | 已有;须随 v2 列扩展 |
| 接口清单与依赖闭包 | `sh ./unisacc.com tools/layer-api.cx -- --check` | 已有 |
| 版本与内容冻结 | `sh ./unisacc.com tools/check-tlib-frozen.cx` | 已有 |
| 棘轮(含上层禁裸 libc) | `sh ./unisacc.com tools/ratchets.cx` | 已有 |
| 各区面批次(每行成功、失败、拒绝用例,输出 `SURF` 行) | `check-cx-sys*`、`check-cx-path` 等 | 已有批次;`SURF` 输出待加 |
| 六目标同候选 | `release/candidate.tsv` 所指候选的 CI 证据 | 已有矩阵;按包的面批次待接入 |
| 稳定判定 | `sh ./unisacc.com tools/stable-eval.cx` | 待建 |

未实跑不记 PASS;证据只绑定产生它的候选。

### 0.7 路线(按依赖排序)

每步是一类改动,按「一个候选只含一类大改」各占候选;与第 6、8 节的 CX0 路线(0.0.58 至 0.0.60)并行排期,具体版本由集成者按 `agentwork/dag.tsv` 定。

| 序 | 内容 | 前置 | 触产品输入 |
|---|---|---|---|
| C1 | 表升 v2:四列、能力词表、错误文法归一、129 处行编辑落表;`check-std-surface` 同步 | 无 | 否(表与工具) |
| C2 | 建 `tools/stable-eval.cx` 与夹具,先跑出各包差距清单 | C1 | 否 |
| C3 | 纯计算层收口:`mg_*`、`ms_*` 并入 `cx.mem`,`tx_snprintf` 并入 `cx.text`,失败合同改负码;`text`、`buffer` 两区的 new 行实现 | C1 | 是 |
| C4 | 拆 `cx.base` 为 `cx.sys`、`cx.os`、`cx.mem`、`cx.text` 加内部包 | C3 | 否(包表;发布链冷构造不读 tlib) |
| C5 | 错误码归一与 `sy_err_kind`、`sy_cap`、`sy_res_live`;64 位与 deadline 签名改动;`time`、`os-env` 的 new 行 | C4 | 是 |
| C6 | win 补齐:21 行未实现的拒绝与 68 格 unverified 逐格实跑;路径 UTF-8 | C5 | 是 |
| C7 | `stream` 的 reader/writer 实现与句柄 deadline | C5、C6 的 `sy_poll` | 是 |
| C8 | 各区面批次输出 `SURF` 行并接入六目标 CI | C1 | 否 |
| C9 | `ext.net` 实现与孵化;`np_exec` 孵化 | C5、C6 | 否(ext 不随产品) |
| C10 | 首批稳定声明,预计顺序 `cx.path`、`cx.mem`、`cx.text`,再 `cx.sys`、`cx.os`、`cx.stream` | 判据全 ok | 否(策略表) |

C10 没有日期。它在判据成立时发生,最早也要等面静止满 3 个公开版且 30 天。

#### C5 调用者清单(2026-11-13,`sh ./unisacc.com tools/refactor.cx sites NAME,... --tracked` 实跑)

11 行破坏性签名:`sy_stat_t`、`sy_truncate`、`sy_utimes`、`sy_ts_t`、`sy_spawn_t`、`sy_child_t`、`sy_tcp_accept_from`、`sy_tcp_connect_to`、`sy_udp_recv_from`、`sy_unix_connect`、`sy_tcp_dial`。`sites` 只匹配函数名后带括号的形式,共 38 处;类型名(`sy_stat_t`、`sy_ts_t`、`sy_spawn_t`、`sy_child_t`)作类型使用的位置与结构体字段访问(`.size`、`.sec`、`.deadline_ms`)未被覆盖,同候选改前须另用 `refactor.cx find` 补查,本清单不算完整。

- `apps/csih`(只列出,不改,归 csih 车道,须先发信封):`apps/csih/net.cx:194` 的 `sy_tcp_dial(host, port)`(第三参数增 `sy_deadline_t`)。
- 定义处:`cx/sys/fs.cx:243`(`sy_truncate`)、`cx/sys/posix.cx:369`(`sy_utimes`);`ext/sysx/net.cx` 的 201、510、558、675、715(`sy_tcp_dial`、`sy_tcp_accept_from`、`sy_tcp_connect_to`、`sy_udp_recv_from`、`sy_unix_connect`)。这些在 cx/sys 冻结与 ext 孵化范围,走冻结解除流程或 CXS-C9。
- `tools/check-cx-sys-net.cx`:133、143、160、163(三处)、177(`sy_tcp_dial`);244、246、262、268、269、282、285、287、295、296、300、303(`*_to`、`*_from`、`sy_unix_connect`,末参 ms 常量)。
- `tools/check-cx-sys.cx`:140、141、142(两处)(`sy_truncate`);477、478(两处)、479(`sy_utimes`)。
- `tools/check-procguard.cx:227`、`tools/procguard.cx:473`(`sy_truncate`);`tools/tls13-get.cx:142`(`sy_tcp_dial`)。

#### C3 逐函数返回码表(S3 已落地,S4 待办)

前提事实:冻结 L0-CORE-FREEZE-049 已由账本 CXS-C3-STEP1-FREEZE-NARROW 收窄为 `cx/sys cx/core`,`cx/mem`、`cx/text` 不在冻结前缀内,S3/S4 不需要解除流程。码取 prelude 的 errno 常量,与 `cx/mem/vec.cx`、`cx/mem/map.cx`、`cx/text/iter.cx` 同一约定。成功值全部不变(0);`ms_find`、`ms_rfind` 的 `miss:-1` 是「未找到」而非错误,不变。

| 函数 | 现值(失败) | 新值 | 负例断言(`tools/check-cx-grow.cx`、`check-cx-slice.cx`) |
|---|---|---|---|
| `mg_reserve` | -1 | 空指针或 n<0 -> `-EINVAL`;容量翻倍不可表示 -> `-EOVERFLOW`;分配失败 -> `-ENOMEM` | reserve-negative-refused、reserve-null-einval、reserve-huge-eoverflow、reserve-huge-unchanged |
| `mg_push` | -1 | 空指针 -> `-EINVAL`;扩容失败 -> `-ENOMEM` | null-buf-refused |
| `mg_append` | -1 | 空指针、n<0、n>0 且 p 空、内部复制被拒 -> `-EINVAL`;长度和溢出或扩容失败 -> `-ENOMEM` | append-null-nonzero-refused、append-negative-len-einval、null-buf-refused |
| `mg_move` | -1 | 空指针 -> `-EINVAL` | move-null-refused |
| `mg_clear` | -1 | 空指针 -> `-EINVAL` | clear-null-einval |
| `mg_pop` | -1 | 空指针 -> `-EINVAL`;空缓冲 -> `-ENOENT` | pop-empty-refused、pop-null-einval |
| `mg_truncate` | -1 | 空指针 -> `-EINVAL`;n 不在 0..=len -> `-ERANGE` | truncate-over-refused、truncate-negative-erange、truncate-null-einval |
| `mg_insert` | -1 | 空指针 -> `-EINVAL`;下标不在 0..=len -> `-ERANGE`;扩容失败 -> `-ENOMEM` | insert-oob-refused、insert-null-einval |
| `mg_remove` | -1 | 空指针 -> `-EINVAL`;下标不在 0..len-1(含空缓冲)-> `-ERANGE` | remove-empty-erange、remove-oob-erange、remove-null-einval |
| `mg_take` | -1 | 空指针 -> `-EINVAL` | take-null-refused |
| `ms_at`、`ms_mut_at` | -1 | 空指针 -> `-EINVAL`;越界 -> `-ERANGE` | at-out-of-range、at-neg-off、at-null-einval、mut-at-out-of-range、mut-at-null-einval |
| `ms_mut_fill` | -1 | 坏切片 -> `-EINVAL` | mut-fill-null-einval |
| `ms_mut_copy` | -1 | 空指针或负长 -> `-EINVAL`;长度不等 -> `-ERANGE` | mut-copy-len-mismatch、mut-copy-null-einval |
| `tx_snprintf`、`tx_vsnprintf` | C99:返回完整长度,截断不报错,编码错误为负 | 不改(见下) | - |

调用者检查点(S3,全仓 14 个调用文件,`apps/csih` 为 0):

- 写死 `== -1` 的只有 `tools/check-cx-grow.cx`、`tools/check-cx-slice.cx`、`tools/practice-grow.cx`,已改为精确码并加负例;`archive/` 不动。
- `ext/textx/build.cx` 的 `sb_clear`、`sb_reserve`、`sb_truncate`、`sb_putn`、`sb_putc`、`sb_puts` 原样透传 mg 返回值;`sb_*` 属 `ext.textx`,其 0 或 -1 合同本步不变,透传处归一为 `? -1 : 0`(`check-cx-build`、`practice-build` 的 `== -1` 断言原样通过)。
- 其余调用者只判真假或不看返回值(`tests/stream/xs-conform.cx`、`tools/read-all.cx` 等),不受影响。
- `build/std-surface.tsv` 的 errors 列与实现仍有出入(如 `mg_truncate`、`mg_move`、`mg_clear`、`mg_take`、`ms_mut_fill` 记 `-ENOMEM`,`mg_pop` 只记 `-ENOENT`),该表由 CXS-C8 持表者按本表逐行改,本步不动。

S4(`tx_snprintf`)对 csih 零破坏的过渡,未实施:

- (1) `tx_snprintf`、`tx_vsnprintf` 保留原符号与 C99 语义不动(`apps/csih` 有 64 个调用文件,归 csih 车道)。
- (2) 在 `cx/text/fmt.cx` 新增 `tx_fmt(char *dst, size_t cap, const char *fmt, ...)` 与 `tx_vfmt`:成功返回写入长度(不含 NUL);`dst` 空且 cap>0、`fmt` 空 -> `-EINVAL`;输出放不下 -> `-EMSGSIZE`(此时 dst 仍截断并 NUL 结尾,不静默成功);编码错误 -> `-EILSEQ`。只新增符号,`cx.text` 升 Y 位。
- (3) 非 csih 调用者由 `refactor.cx sites` 列出后分批迁移,每批跑 affected;csih 侧发信封由维护者按自己节奏迁移。
- (4) 全仓 `tx_snprintf` 调用点清零后,旧符号在面表改标 legacy,再过 3 个公开版才可删;删除是破坏性改动,另占候选。

### 0.8 未决风险

- (1) win 是最大缺口:核心 21 行未实现、68 格未核,且表内 win 路径入口是 A 结尾函数(按系统代码页而非 UTF-8,据单元格文字推断,未读源码)。
- (2) 行编辑里有破坏性签名改动(`sy_stat_t`、`sy_ts_t`、`sy_spawn_t`、`sy_child_t` 等 11 行)。预稳定期允许,但仓内调用者须同候选改完;`csih` 车道依赖 `sys.net`,net 的改动先发信封。
- (3) `cx/mem`、`cx/text` 与 `ext/memx`、`ext/textx` 的互指符号链接(第 3 节已记)会让 C3 的迁包出现双身份,须先收敛为单向。
- (4) 标 internal 的 12 行(`rl_*`、`*at` 族、映射四项、`sy_fork_session`)的调用点数未查;若工具广泛使用,须改标 legacy。
- (5) S6、S7 需要的证据格式(`SURF` 行、按包的六目标面批次)尚不存在,`win/arm64` 复用 x86_64 二进制的那一格在实际执行前只能记 UNKNOWN。
- (6) 判据里的数值(3 个公开版、30 天、提升需 3 个调用单元)参照 CPython 与 Node 的做法按本仓节奏取定,无本仓数据支撑。
- (7) 三宿主:gcc_cx 与 clang_cx 下 15 个 L0 模块不能编译(第 7 节 A6),本核心面的稳定判据只对 unisacc.com 宿主求值,不声称宿主驱动下可用。

## 1. cx 流：CX0（在 e2 之前）

**定位**：`.cx` 源 -> 模块规划与导出接口 -> 每单元一份「平铺 C + 强制前导头」，再交给 e2。它是七流之外的第 8 个处理步，但**现在不是模型流**：由手写规划器代码执行，没有网、没有路由行。prd.md §3.3 写的「7 + 1」中的 1 即它。

| 项 | 现状 | 出处（命令） |
|---|---|---|
| 权威源码 | `cx/cx0/plan.cx` 73,295 B / 1329 行（解析器只在这里维护） | `stat`、`wc -l` |
| 产品实际读的 | `src/cxmod.c` 65,288 B / 1235 行，由 `tools/cx0-extract.cx` 从 plan.cx 投影，禁止手改 | `cx0-extract -- --check` 实跑 1.8 s，`CX0_EXTRACT EXACT bytes=65288` |
| 束 | `cx/cx0/bundle.cx` 12,485 B、`bundle.h` 1,146 B（封存规划时观察到的源 / 接口 / 资源） | `stat` |
| 产品侧实现 | `cx/compiler/cxmod.cx`（`src/drive.cx`、`driveargs.cx`、`driveunits.cx` 直接 use；条款 CX-G-4 要求它与 plan.cx 同文法）。产品没有改接 plan.cx 的 `cx0_plan` 接口；cx0 README 的说法是「已装产品驱动仍调用原 C 规划器」，三份实现（plan.cx / 投影 cxmod.c / compiler.cxmod）在产品二进制里各占多少本次未核 | `git grep compiler.cxmod -- src`、cx/spec.md CX-G-4 |
| 包图 | `cx/workspace/packages.tsv`（CXWORK1：P 6 / I 49 / D 5 / T 5 / G 6 / C 1 条记录）、`cx/workspace/driver.cx` 25,611 B | 按首列计数 |
| 前导资源 | `include/cx-flat.h` 源 409,158 B / 9165 行；包内 `@hdr/cx-flat.h` 101,204 B | `stat`、`com-tree` |

**三种输出格式**（协议见 [cx/cx0/README.md](cx/cx0/README.md)、[cx/cx0/BUNDLE.md](cx/cx0/BUNDLE.md)）：`CX0PLAN1`（逐单元 TSV：序号、路径、强制头，即 `unisacc -cx-plan` 的内容）、`CX0IF0`（CXIF v0 导出声明文本）、`CX0BND1`（不可变编译单元束，带模型身份、目标、ABI、资源）。

**已验证**：plan.cx 与投影逐字一致（本次实跑）；批次 `cx0-unit`、`cx0-bundle`、`cx0-e2` 已登记为 daily 档（本次未跑）；模块循环、公开冲突、条件化依赖、模块定义 main 均具名拒绝（cx0 README 所列证据，本次未重跑）。

**未完成（U1 / U2，见 [cx/spec.md](cx/spec.md) CX-W 节）**：已装产品驱动仍调用原 C 规划器；规范逻辑 ID、完整配置 / 依赖映射、完整 CX0 / E2 合同、CX0 模型化部署都没有做。CXIF v0 与 e2 强制接口仍是文本包含兼容桥。

**与 units 的区别**：units 是包内已有的模型阶段（模型 35，`multi/*` 路由 144 行），在 e1 之后把各单元的 `tokens.typed` 帧合并交给 e3；CX0 决定「有哪些单元、每单元前导什么头」。两者都只在多单元编译里出现，units 已是模型，CX0 还不是。

**实测耗时**：`-cx-plan` 9 单元 28-39 ms、19 单元 106-108 ms（各 2 次）；最小 2 单元 `.cx`（`use text.text`）冷编总墙钟 874-884 ms（3 次），其中 e2 两次共 243 ms、e3 228 ms、e1 56 ms、units 45 ms；9 单元工具冷编 6.7 s，其中 e2 九次共 1568.5 ms，帧材料 3,783,271 B（约每单元 420 KB，与 cx-flat.h 源 409 KB 同量级）。

## 2. cx 侧网模块

`cx/compiler/net/` 有 **20** 个模块（`ls` 计数）= 18 张金表网 + lexcls、lexword 两张声明表；`cx/compiler/stream/` 7 个、`cx/compiler/image/` 4 个（elf、macho、object、pe）。这 31 个文件全部是生成的**骨架**，函数体只有 `return 0`：18 个金表网与 7 个流由 `tools/surface-gen.cx` 生成（`grep -l surface-gen` 命中 25 个，头注「Skeleton only. Behavior is later TDD」），lexcls、lexword 由 `tools/declared-gen.cx` 生成，image 的 4 个由 `tools/image-gen.cx` 生成；20 个网模块 `grep -l 'return 0'` 全中。

定位：它们是 cx 模块树里给「流 / 网 / 映像格式」预留的**接口位置**，不承载行为，产品执行的网全部在 P4 包里（prd.md §3.4）。名字真源是 `cx/compiler/netname.cx`（18）、`goldname.cx`（20）、`streamname.cx`（7），都由生成器从 `exec/pipeline/*.tsv` 产生。

与决策表的关系：真正已进表并被七流读取的 cx 时代决策表是 `weights/stmt/stmt.tsv`（107 行）、`weights/abi/dispatch.tsv`（523 行）、`weights/type/context.tsv`（61 行）以及金表里的 type（4276 行）、peep（1633 行）；`weights/arity/arity.tsv`（244 行）已有表与规则对拍，产品 parse 尚未读它（行数均为非注释行）。这些表经 `tools/decide-gen.cx` 进 `exec/` 的网行，不经过骨架模块。骨架模块是 0.0.53 计划 M1-M4（决策进表）将来落行为的位置，目前没有任何一个有行为。

## 3. cx/compiler 其余结构与包层

`cx/compiler` 共 78 个文件、397,580 B（`git ls-files` + `stat`）：

| 组 | 成员 | 作用 |
|---|---|---|
| 命名真源 `*name.cx` | 25 个（streamname、netname、goldname、routename、targetname、osname、archname 等） | 全部由生成器产生，手改无效（条款 CX-S-8） |
| 模块规划 | `cxmod.cx` | CX0 的产品侧接线 |
| 帧与多单元 | `frames.cx`、`frameparts.cx`、`fwdstub.cx`、`fwdmerge.cx`、`fwdsig.cx`、`fwdincrement.cx` | 单元帧、转发桩、合并（units 与方案 X / Y 的基础） |
| 门 `*gate.cx` | `boundgate`、`codecgate`、`libgate`、`tapegate` | 边界、编解码、库合同、tape 域的接受 / 拒绝 |
| tape | `tapebin.cx`、`tapelink.cx`、`tapeshape.cx` | tapebin 格式与链接 |
| 库与宿主绑定 | `libc99bind.cx`（由 `libc99posix-gen` 生成）、`hostbind.cx`、`libsig.cx`、`libtype.cx` | libc99posix 六格绑定、宿主符号 |
| 汇编文本 | `asmtext.cx` | `as` / `view` 子命令 |
| 骨架 | `net/` 20、`stream/` 7、`image/` 4 | 见 §2 |

**模块层（cx-L0..cx-L3）与 EXT**（定义在 `tools/check-cx-layers.cx` 头注；字节为 `git ls-files` + `stat` 实测）：

| 层 | 目录 | 文件 / 字节 | 进源 SID |
|---|---|---|---|
| cx-L0 宿主对接 | `cx/sys`、`cx/mem`、`cx/text` | 27 / 141,992；6 / 20,987；7 / 34,117 | 是 |
| cx-L1 机制 | `cx/core`、`cx/runtime` | 11 / 47,409；90 / 271,130 | 是 |
| cx-L2 驱动与编译器 | `cx/driver`、`cx/compiler`、`src/`（25 个 .cx 203,081 B） | 14 / 100,918；78 / 397,580 | 是 |
| cx-L3 工具、测试、应用 | `tools`（733 个 .cx）、`tests`、`apps` | 未逐项计 | 否（tools 仅 10 个支撑文件进 SID） |
| AMB（未定层） | `cx/cx0`、`cx/workspace` | 7 / 101,017；6 / 49,394 | 是 |
| EXT 包层 | `ext/sysx`、`memx`、`textx`、`crypto`、`tsvx` | 13 / 56,844；5 / 18,680；5 / 24,403；15 / 157,845；3 / 5,851 | 否；memx、textx 经 `cx/mem`、`cx/text` 里的符号链接间接参与 |

规则与入口：
- `use` 的包根由仓根 `cxroot.tsv`（CXROOT1，13 条 R 行）按最长前缀映射（条款 CX-U-7）。
- 层向检查 `tools/check-cx-layers.cx`：编号层向上越层即红，产品层 `use` 到 `ext/` 即红。**本次实跑 20.9 s，`failed=1`**：`cx/runtime/holdframe.cx:5 use compiler.frames`（cx-L1 向上用 cx-L2；edges=4599、up=1、amb=21、out=79、ext=410）。未查它在 main 上是否已登记为已知项。
- core 模块冻结表 `tests/cx-module-freeze.tsv` 26 行，只许减；导出面棘轮 `tests/coreapi.cx`。
- 三个只降不升的棘轮：`tests/cxinclude.cx`（禁新 `#include`）、`tests/cxlibc.cx`（上层禁裸 libc）、`tests/cxraw.cx`。
- `src/` 下有 7 个目录符号链接（compiler、core、driver、runtime、sys、sysx、text；`git ls-files -s src` 的 120000 项）。
- 源 SID 与 ext 的已核事实（2026-11-08 更正，自 prd.md §5 迁入，本次未重核）：`ext/` 目录本身不在 `seed/ident.c` 的目录集内（`exec src kernel include weights seed cx` + `iterate/kernel/typekw.tsv` + `tools/` 10 文件，共 1586 文件），但 `cx/mem`、`cx/text` 里有 8 个符号链接指向 `ext/memx`、`ext/textx`（反向亦有互指），`ident` 跟随文件链接，故这两个 ext 目录的内容经链接参与源 SID；`ext/sysx`、`ext/crypto` 不参与。`ext/` 不随 APE 发布（产品二进制无其符号）。W3 判定 `arena`、`slice`、`grow`、`num`、`build`、`fmt`、`deflate` 七模块产品链路引用为 0、归 ext、原路径留别名，当前状态即该终态；`memx/textx` 经别名使用（`use mem.*`/`use text.*`，`use text.fmt` 169 处），不是孤儿。`sysx`/`crypto` 由 75/14 个检查守住。风险：互指符号链接让同一文件有两个身份、Windows 无符号链接不安全；冻结路径须含 `ext/memx ext/textx`（已补）。待办（原定 0.0.49，未做）：把真身 / 别名收敛为单向并去掉互指。内部包接口摘要见 [build/std-surface.tsv](build/std-surface.tsv)。原文说「`cxroot.tsv` 只映射 sysx、crypto」已过期：现表 13 条 R 行，ext 侧映射 sysx、crypto、tsvx。

## 4. 语言与运行期所有权：现状与未声称的性质

- cx 是参照 Rust 设计的模块化 C 方言：`#pragma unisa module / use`，新 `.cx` 不手写 `#include`，上层只用 cx-L0 的 `sy_*`、`mm_*`、`tx_*`。
- 所有权与边界检查是**运行期**的（句柄、容量、`mm_copy` 类带界接口）。**没有**编译期借用检查，不声称 Rust 式静态内存安全。
- 源码级所有权扫描 `tools/cx-own.cx` 只看函数内局部 owner（DOUBLE-FREE / USE-AFTER-FREE / LEAK，保守）；产品的所有权多在结构体字段与包装分配器里，文本分析覆盖不到。
- CX0 只面向单线程 POSIX 调用者；Windows、任意线程并发未获资格。
- 模块循环硬拒绝；跨模块错误只走返回值 `-errno`（CX-Y-4）。
- 包里的 contract（模型 38）、ownership（模型 39）、symbols（模型 37）是库 / 宿主合同的协议模型，不是 cx 语言的所有权检查器。

## 5. 速度与开发效率点

| 点 | 现状实测 | 做法 | 是否触产品输入 | 状态 |
|---|---|---|---|---|
| CX0 规划结果按 CX0BND1 身份缓存 | `-cx-plan` 28-108 ms（9-19 单元），占冷编不到 2% | 以束身份为键缓存规划与 CXIF | 是（驱动） | 收益小，不单独立项 |
| cx-flat.h 裁剪 | 源 409,158 B；每单元帧约 420 KB；e2 占 9 单元冷编 23%（1568.5 / 6705 ms） | 按单元实际用到的声明裁前导 | 是（头与 e2 输入） | 收益未测；会改所有 e2 输出字节 |
| e2 / e1 结果按内容身份缓存 | 同上；e1 另占 259.5 ms | 键 = 单元内容 sha + 规范化前导顺序；共享前导只展开一次 | 是（runcache / driveunits） | 0.0.52 A2、0.0.53 C6 立项，完成度未核；建议作 0.0.54 唯一一项产品输入改动 |
| 热路径 | 运行缓存命中 0.13-0.8 s（H7 与本次 com-tree） | 已有 | - | 已落地 |
| CX0 模型化为第 8 流 | 未做 | 见 §6 | 是（大改） | 顺延：55-4 在 0.0.60（P60），55-1 在 0.0.58（P1），55-2 在 0.0.59（P59） |
| 骨架网落行为 | 31 个骨架全为 `return 0` | 见 §6 | 是 | 顺延 0.0.56/0.0.57 |
| 层向检查提速 | `check-cx-layers` 20.9 s | 复用 symindex 索引 | 否 | 未立项 |

## 6. 路线：cx 流网设计与实现（0.0.58 至 0.0.60）

版本划分（集成者 2026-11-11 决定，账本 `VER054-055-SPLIT`；后经 `VER055-SPEED` 改判，2026-11-12 再按 0.0.58 计划重排）：0.0.54 只做梳理与提速；cx 流网的步骤按每版一项产品改动重排：55-1（规划器单源）= 0.0.58 的 P1，55-2（解析状态实例化）= 0.0.59 的 P59，55-4（CX0 事实表入库与第 8 流）= 0.0.60 的 P60，55-3（cx-tri 门）视矩阵结果随其后（见 [prd.md](prd.md) §9.7、[plans/v0.0.59-unisacc.md](plans/v0.0.59-unisacc.md)）。下文凡写 0.0.55 处，55-1 按 0.0.58、55-2 按 0.0.59、55-4 及以后按 0.0.60 理解。范围、前置、验收与分步的唯一权威是下文「进化计划」B1（0.0.54 前置）与 B2（0.0.55 主线），B4 为贯穿各版的不变式。补充两条前置：`research/cx2-module-design.md` 没有进表或成网的设计（其 §7 把「现在就用完整 E3 解析器推导 CXIF」列为与约束冲突的做法），0.0.55 须先写 CX0 事实表划分与流格式（B1 54-6）；`check-cx-layers` 的 1 条向上边（`cx/runtime/holdframe.cx:5`）先处置。

以下两节（架构分析、进化计划）由独立分析写成，基线同上：HEAD `6a4fa1fa6f`，已装产品 sha256 前缀 `1bffabbaf45ee349`，gcc 11.4.0，clang 15.0.7，实测日 2026-11-11；探针与脚本不入库。

## 7. 架构分析

### A0 定位:cx 是编译器无关的语言/模块层,unisacc.com 只是宿主之一

cx = C99 受检查子集 + `#pragma unisa` 配置(`cx/spec.md` CX-L、CX-G)。它由四件与具体编译器无关的东西组成:

- (1) 模块文法与规划:`#pragma unisa module/use`、包根 `cxroot.tsv`(CX-U-7)、闭包/环/冲突拒绝。产物是逐单元计划(`-cx-plan` 的 TSV)。
- (2) 接口投影:CXIF v0 文本(类型加原型),公开集由 `cxpack.tsv` 收窄(CX-I、CX-P)。
- (3) 前导与库:`include/cx.h` 前导(`cx_*`),L0 库 `cx/sys`、`cx/mem`、`cx/text`,包层 `ext/*`。
- (4) 运行期所有权合同:`mg_*`/`ms_*`(CX-O-1 至 5)、`cm_*` 句柄(CX-O-6)、冻结原型 `cx/owned.cx`(CX-O-7)。

三个宿主消费同一层:

| 宿主 | 入口 | 规划器实际编入的源 | 前导来源 | 后端 |
|---|---|---|---|---|
| unisacc.com | `sh ./unisacc.com FILE.cx`(默认 -run)、`-cx-plan`、`-cx-interface` | `cx/compiler/cxmod.cx`(`src/drive.cx:14` 等 5 处 `use compiler.cxmod`) | 包内资源 `@hdr/cx-flat.h`(盘上 `include/cx-flat.h` 409,158 B,由 `tools/cx-flatten.cx` 生成) | 七模型流 e2 e1 e3 e4 prune lower elf |
| gcc_cx | `tools/gcc_cx.cx`(19 行)-> `tools/cxcc.cx`(1366 行) | `src/cxmod.c`(`-DCXM_HOST_REALPATH`,由 `cx/cx0/plan.cx` 投影) | 盘上 `include/cx.h` -> `cx-nested.h`(17,519 B) | 系统 gcc,普通 C 产物 |
| clang_cx | `tools/clang_cx.cx`(19 行)-> 同一 `cxcc.cx` | 同上 | 同上 | 系统 clang,普通 C 产物 |

结论:语言/模块层(规划、CXIF、cxpack、拒绝)在设计上是三宿主共用的前端;现状是「一份权威加两份实现」,而不是一份实现(见 A7 问题 1)。宿主驱动的产物是普通 C,不授予已检查身份(`cx/spec.md` CX-W 节末句)。

### A1 模块体系

- 文法:`#pragma unisa module NAME`、`use NAME`、`use NAME::sym [as alias]`、`dock`;只在文件作用域、首个声明之前、条件之外;未知词具名拒绝。三宿主实测同拒:`out/cxarch/bad.cx` 三入口均输出 `cx: out/cxarch/bad.cx:1: unknown unisa pragma nonsense`,rc=1。
- 解析:先找使用方同目录,再按最近的 `cxroot.tsv` 前缀映射(根表 13 行前缀:sys mem text core runtime driver compiler workspace cx0 sysx crypto cxlib tsvx)。
- 闭包:命令行源在前,广度优先追加模块,同 realpath 且依赖一致则合并,不一致具名拒绝;环、被 use 模块定义 main、公开名冲突均在编译前拒绝。
- 接口:CXIF v0 是文本,写到 `$TMPDIR/ua-cxif-<sha16>.h`,作为强制包含追加到使用方单元(实测 `-cx-plan out/cxarch/h5.cx`:根单元带 5 个 `/tmp/ua-cxif-*.h`,5 个模块单元各只带 `cx.h`)。直接 use 给完整接口,传递 use 只给类型部分。
- 公开集:同目录 `cxpack.tsv` 有匹配行则只导出该行函数;无匹配行导出全部非 static(兼容行为)。
- 链接模型:整程序 `-run`/`-o` 把闭包内每个模块各嵌一份并合成一个程序(无链接器);`-c -funit` 的链接单元语义(CX-W-1)已裁定未落地。
- 未生效项(CX-W-1 至 8):`.cx` 内 `#include` 硬拒绝、编译期所有权、层感知前导、CXIF v1 结构化字段、宿主 `-run` 正式资格等,均不得记 PASS。

### A2 包层与层向规则

规模(实测,`git ls-files` 加 `.cx` 行数):

| 层 | 目录 | 文件 / .cx 行 |
|---|---|---|
| L0 | `cx/sys` `cx/mem` `cx/text` | 27/3499、6/488、7/807 |
| L1 | `cx/core` `cx/runtime` | 11/634、90/6683 |
| L2 | `cx/driver` `cx/compiler`(及 `src/`) | 14/2381、78/8686 |
| AMB | `cx/cx0` `cx/workspace` | 7/1434、6/102 |
| EXT | `ext/sysx` `ext/memx` `ext/textx` `ext/crypto` `ext/tsvx` | 13/1258、5/488、5/610、15/3432、3/72 |
| 工具库 | `tools/cxlib` | 44/7814 |

层向由 `tools/check-cx-layers.cx` 审计(5 条永久断言:编号层上行边拒绝;未解析只许白名单 ENOENT;边分区求和;产品层不得 use `ext/`;核心模块集按 `tests/cx-module-freeze.tsv` 冻结,只缩不扩)。注意:`cx/cx0`、`cx/workspace` 在该检查里是 AMB(无层号),AMB/EXT/OUT 边只报 UNKNOWN 不算通过;符号链接入口不在审计域(`tools/check-cx-layers.cx:8-14` 头注)。`src/` 下有 7 个指向 `cx/`、`ext/` 的目录软链(compiler core driver runtime sys sysx text),同一文件两个词法身份。

### A3 CX0 流:输入、输出与合同

- 输入:源路径数组、强制头数组、调用方显式给出的接口目录(不读 TMPDIR)。
- 输出:`status=0` 时 `CX0PLAN1\n` 加逐单元 TSV,或接口模式 `CX0IF0\n` 加 CXIF v0;`status=1` 规划拒绝;`status=2` API/IO/工作进程协议故障;失败不返回成功计划字节。只有 ENOENT 归缺失,ENOTDIR/EACCES 单列 IO 错误(`cx/cx0/README.md`)。
- 封存:`cx/cx0/bundle.cx` 把规划时观察到的源字节、CXIF、显式资源、单元顺序封成 `CX0BND1`(头 96 字节含 E2 模型容器 SHA256,尾 64 字节摘要;上限 64 MiB、64 单元、4096 资源)。读取器不访问文件系统。
- 消费:`tools/cx0-e2.c` 只从束内取源与头,跑真正的 E2 网,输出 UNIPP1;仅实现 `lnx/x86_64`。
- 工作区:`cx/workspace/packages.tsv`(CXWORK1)登记 6 个包(cx0-plan、cx0-bundle、cx0-e2、pipeline-stage、pipeline、source-driver)与 5 条依赖边;无自动缓存,只有显式 `resume`。
- 边界:U1/U2 未完成,已装产品驱动不走这条链;CX0 没有事实表、生成器、网,不在 `exec/pipeline/image-stages.tsv`(该表 7 行),不在 P4 包内。隔离方式是 POSIX fork 工作进程加管道(`cx/cx0/plan.cx` 的 `pipe`/`fork` 段),只面向单线程 POSIX 调用者;Windows 与线程并发未获资格。

### A4 cx/compiler 子结构与耦合

78 个文件 8686 行(实测),分五类:

| 类 | 文件 | 行数 | 性质 | 产品是否使用 |
|---|---|---|---|---|
| 模块规划器 | `cxmod.cx` | 1348 | 手写机制,与 `plan.cx` 同文法的第二份实现 | 是(src 5 处 use) |
| 汇编文本 | `asmtext.cx` | 2268 | 手写机制 | 是(1 处) |
| tape 与转发 | `tapegate` 686、`tapelink` 521、`fwdsig` 512、`tapebin` 378、`fwdmerge` 351、`frameparts` 195、`fwdstub` 174、`fwdincrement` 112、`frames` 99、`*gate`、`lib*`、`hostbind`、`bindstage`、`leword`、`winprocess` | 约 3900 | 手写机制 | 部分(frames 3、tapegate 2、fwdsig 2 等) |
| 命名真源 | `*name.cx` 23 个 | 每个 15 至 84 | 生成物(各 `tools/*-gen.cx`),从事实表列出名字 | `cellname`、`rtsharedname` 被 src 使用,其余主要被检查工具使用 |
| 骨架 | `net/` 20 个、`stream/` 7 个、`image/` 4 个 | 每个 7 至 16 | `tools/surface-gen.cx`、`declared-gen.cx`、`image-gen.cx` 生成,函数体 `return 0` | 否;调用方只有 `tools/check-cx-surface.cx`、`check-cx-decl.cx`、`check-cx-declared.cx`、`check-cx-image.cx` 与生成器自身 |

耦合要点:`src/` 对 `compiler.*` 的 use 共 20 处,其中 5 处是 `cxmod`;骨架三目录(31 文件)与产品零耦合。`cx/runtime` 下同样有 `net/` 20 个、`stream/` 7 个骨架与一套同名 `*name.cx`(surface-gen 头注:compiler 与 runtime 各得一份),即同一事实表投影成两套镜像模块;带 `Generated by` 标记的文件在 `cx/compiler` 加 `cx/runtime` 合计 114 个头注(其中 surface-gen 50 个)。

### A5 与七流、18 网、42 构造网、P4 的关系

- 七流是字节管道(`image-stages.tsv`),cx 源在产品内的路径是:cxmod 规划(经典代码,编进 ISA 切片)-> 每单元强制包含 `cx.h` 加 CXIF -> e2 -> e1 ->(units 合并)-> e3 -> prune -> lower -> elf。cx 语义此刻完全表现为「e2 之前的一段经典代码加一个 409 KB 的头」。
- 18 张金表网(`weights/gold`,实测行数含表头:reloc 11、pfconv 13、lexword 21、regmap 21、tyinfo 22、pp 23、prec 23、scope 35、binsel 37、opinfo 79、lex 149、isel 180、lexcls 262、parse 345、irsel 425、enc 528、abi 540、combo 542、peep 1638、type 4281)没有任何一张回答 cx 的决策;cx/compiler/net 的 20 个骨架只镜像了网名,不持有表,也不调用网。
- P4 包的 40 模型、42 构造网、77 资源(prd.md 第 3 节,0.0.47 取数,未重测)里与 cx 相关的只有资源 `@hdr/cx-flat.h`(包内 101,204 B,同上未重测)与 `@hdr/cx.h`、`@hdr/cx-nested.h`。
- 所以按 prd.md 0.1 节的范式度量,cx 前端的「决策进表比例」为 0:规划拒绝、公开集、可见性、上限全是机制代码。

### A6 三宿主:共享点、分叉点、合同、测试入口

共享(实测成立):

- 计划字节:`-cx-plan out/cxarch/h1.cx`、`h5.cx` 在 unisacc、gcc_cx、clang_cx 三入口输出逐字节相同(`cmp` 通过),含 `/tmp/ua-cxif-<sha16>.h` 名字,即 CXIF 内容地址一致。
- 未知 pragma 拒绝行与 rc 三入口相同(见 A1)。
- 运行期所有权检查:`cx/check_memory.cx` 三入口均 `assertions=138 failed=0`;`cx/check_owned.cx` 三入口均 `assertions=551 failed=0`。

分叉(实测或有行号证据):

| 分叉点 | unisacc.com | gcc_cx / clang_cx | 证据 |
|---|---|---|---|
| 规划器实现 | `cx/compiler/cxmod.cx`(88 个 `cxm_*` 函数名) | `src/cxmod.c`(66 个,等于 `plan.cx` 投影) | `cx0-extract --check` 只核 plan.cx 与 src/cxmod.c(实测 `EXACT bytes=65288`);cxmod.cx 无逐字门,`diff` 两者 423 行差异 |
| 单元上限 | `CXM_MAXU = 128`(`cx/compiler/cxmod.cx:28`) | `CXM_MAXU 96`(`src/cxmod.c:27`、`cx/cx0/plan.cx:78`) | 97 至 128 单元的程序预期产品接受、宿主拒绝(由常量推断,行为未测) |
| 前导 | `cx-flat.h`(平铺,409,158 B) | `cx-nested.h` 加宿主 libc 头 | `include/cx.h` 按 `__UNISA__` 选择;`tools/cx-flatten.cx --check` 守平铺一致 |
| L0 库可用性 | 全部可用 | 38 个可解析探针模块中 15 个编译失败 | 实测 `out/cxarch/hostsweep.cx`:gcc_cx 与 clang_cx 均 ok=23 fail=16(含 1 个不存在的 `tsvx.tsv`);失败集:sys.fs sys.proc sys.time sys.posix sys.mem sys.poll sys.tty sys.dl sys.ffi sys.sig sys.kernel sys.process sys.net sysx.poll cxlib.netpack;首因是裸 `__syscall6`(例 `cx/sys/fs.cx:208`、`:268`)与 `unisacc_ffi.h` 缺失(`cx/sys/ffi.cx:15`) |
| 隐式声明 | 历史上默认运行接受跨单元隐式调用(历史记录;本轮未重测) | `-Werror=implicit-function-declaration` 拒绝 | `cx/spec.md` CX-W |
| 缓存 | `-run` 结果级缓存(`src/runcache.cx`,键含产品、路由、目标、源) | 单元对象缓存 `out/cxcc-cache`(键含编译器、旗标、前导、源、头闭包) | 头注 |
| 宿主入口引导 | 不适用 | `tools/build-cxcc-host.cx` 头注给的 gcc 引导命令在 HEAD 失败:`implicit declaration of function 'tx_eqz'`(文件首行 `use text.text`,裸 gcc 不展开) | 实测;`tests/cx-host-run.cx` 的私有四步配方可用(实测 gcc 759 ms、clang 597 ms 建成) |
| 平台 | 六目标 | 仅 Linux 实跑;Windows、macOS 未执行 | `tools/gcc_cx.cx` 头注 |

测试入口:三宿主一致性只有 `tests/cx-host-run.cx`(批次 `cx-host-run`,对拍语料 ms1 ms2 ms3 cxpack/use 四个正例加 unk cond cmt1 splice1 四个反例)与 `tests/cxcc/units.tsv`(10 个单元,无一使用 `cx/sys` 模块)。产品侧有 `cx0-unit`、`cx0-bundle`、`cx0-e2`、`tests/cxif.cx`、`tests/cxpack.cx`、`check-cx-layers`。没有一个门同时约束 cxmod.cx 与 plan.cx 的语义相等。

所有权与安全性质分宿主:

| 性质 | unisacc.com | gcc_cx | clang_cx |
|---|---|---|---|
| 模块私有(static 不出接口)、cxpack 公开集 | 已有(规划期) | 已有(同一 CXIF) | 已有 |
| 未声明跨模块调用拒绝 | 规划器 `cxm_checkcalls` 对本程序内定义的函数拒绝;其余情形未声称 | 已有(编译器 -Werror) | 已有 |
| `mg_*`/`ms_*` 运行期合同 | 已有,调用方合同,不强制 | 同左(库代码相同) | 同左 |
| `cm_*` 句柄世代/借用/越界 | 实测 138/0 | 实测 138/0 | 实测 138/0 |
| `owned` 冻结原型 | 实测 551/0 | 实测 551/0 | 实测 551/0 |
| L0 `sys.*` 资源生命周期合同 | 已有(检查在产品上跑) | 未声称(15 个模块不能编译) | 未声称 |
| 编译期所有权/借用(CX-W-4) | 未声称 | 未声称 | 未声称 |
| 线程安全、Windows/macOS 上述性质 | 未声称 | 未声称(未执行) | 未声称(未执行) |
| 已检查 .cx 身份 | 仅产品路径 | 不授予 | 不授予 |

### A7 问题清单(每项带证据)

1. 规划器三源、两份实现。`cx/cx0/plan.cx`(1329 行,权威)、`src/cxmod.c`(1235 行,投影,逐字门 `cx0-extract --check`)、`cx/compiler/cxmod.cx`(1348 行,产品实际编入,靠人工同步)。事故记录:`agentwork/ledger.tsv:73-74`(DOCK1 只改了前两份,候选 strings 命中 0)。现存漂移:`CXM_MAXU` 128 对 96;`cxm_*` 函数名 88 对 66。`cx/spec.md` CX-G-4 只要求「实现同一文法」,无门。
2. 权威与产品旁路。README 写「已装产品驱动仍调原 C 规划器」;准确说是产品编入 cxmod.cx,而 plan.cx 的全部调用方是工具链 6 包(`cx/workspace/packages.tsv`)与宿主驱动。权威源不在产品里运行,产品源不在宿主里运行。
3. C 投影是活依赖。`src/cxmod.c` 被 `tests/cx-host-run.cx:1031`、`tests/check-source-bom.cx:113`、`tools/build-cxcc-host.cx` 读取,退役前须给宿主驱动另一条取得规划器的路。
4. 骨架网无表无行为。`cx/compiler/net/*.cx` 20 个(18 金表网加 lexcls、lexword)与 `stream/` 7、`image/` 4,函数体全是 `return 0`;`cx/runtime` 再镜像一套。调用方只有检查工具;它们证明的是「名字与事实表一致」,不是行为。
5. CX0 不是模型流。无事实表、无生成器、无网、不在 `image-stages.tsv`、不在 P4;CX0BND1 链只在工具侧运行,E2 适配器仅 lnx/x86_64。
6. 冷启动开销随 use 数近线性放大(详见 A8):5 个 use 的 8 行程序冷跑 3.6 至 3.7 s,其中规划只占约 17 ms;同样 5 个 use 换一个根文件(h5b)仍 3570 ms,即模块单元无跨程序复用。
7. 每单元重复解包同一模型。h5 冷跑 6 个单元里 e2 模型 inflate 6 次共 77.4 ms、e1 模型 inflate 6 次共 68.4 ms(单进程内同一模型)。
8. CX0 工作进程隔离只支持单线程 POSIX(`cx/cx0/README.md` 第 27 行附近);Windows 无 fork,产品若直接切到 plan.cx 的现形态会丢 win 两格。
9. L0 不是宿主无关。15 个模块在 gcc/clang 下不能编译(A6),「上层不写 `__syscallN`」只约束了上层,L0 自己把 unisacc 内建当成唯一后端;`tests/cxcc/units.tsv` 未覆盖 L0。
10. 宿主引导文档失效。`tools/build-cxcc-host.cx` 头注命令在 HEAD 不能用(A6);根目录无 `gcc_cx.com`、`clang_cx.com`(共享检出与克隆均无,需现建)。
11. 层审计盲区。`cx/cx0`、`cx/workspace` 无层号(AMB);软链入口不在 symindex 域;`src/` 7 个目录软链加 `cx/mem`、`cx/text` 与 `ext/memx`、`ext/textx` 互指链接(prd.md 第 5 节已记),Windows 无软链不安全。
12. 生成名表膨胀。约 30 个单用途 `*-gen.cx` 生成 114 处标记文件,compiler 与 runtime 各一份同名表;一类事实改一次要过两套生成与两套检查。
13. 接口仍是文本包含桥。CXIF v0 无效果/布局/所有权字段(CX-I-2),强制头进入 e2 重新预处理,接口不变也不能免编使用方。
14. 上限常量分散。`CXM_MAXUSE 48`、`CXM_MAXDECL 48`、`CXM_MAXU` 写在三份源里;`exec/facts/limits.tsv` 只登记了 `UNIT_MAX 128` 指向 cxmod.cx,未覆盖 plan.cx 的 96。

### A8 速度实测(Linux x86_64,已装产品,单次墙钟,毫秒)

小程序(`out/cxarch/`):h0.c 空 main;h0.cx 同内容;h1.cx 一个 `use text.text`;h5.cx 五个 use(sys.io sys.fs sys.env text.text mem.heap);h5b.cx 与 h5 同 use、main 不同。

| 用例 | unisacc 冷(`UNISACC_RUN_CACHE=0`,三次) | unisacc 热(运行缓存命中) | gcc_cx `-o` 冷 / 热 | clang_cx `-o` 冷 / 热 |
|---|---|---|---|---|
| h0.c | 110 / 109 / 114 | 8 | 不适用 | 不适用 |
| h0.cx | 364 / 371 / 359 | 7 至 9 | 70 / 31 至 32 | 148 / 65 至 68 |
| h1.cx(2 单元) | 886 / 868 / 881 | 13 至 15 | 136 / 41 至 46 | 209 / 67 至 71 |
| h5.cx(6 单元) | 3681 / 3617 / 3731 | 29 至 32 | 编译失败 rc=1(sys.fs) | 编译失败 rc=1 |
| h5b.cx(新根,同模块) | 3570(首次,缓存开) | 30 | 未测 | 未测 |

其它:`sh unisacc.com --version` 8 至 9 ms(启动下限);`-cx-plan h5.cx` 17 至 23 ms;`-cx-plan tools/check-cx-layers.cx`(11 个 use)93 至 97 ms;宿主 `-run` h0 gcc 34 ms、clang 67 ms,h1 gcc 38 ms、clang 75 ms;编出的宿主二进制运行 2 ms。工具级:`cx/check_memory.cx` 产品冷 2953 ms、热 131 ms,gcc_cx -run 132 ms,clang_cx -run 208 ms;`cx/check_owned.cx` 产品 1335 ms(首次),gcc 140 ms,clang 218 ms;`tools/cx0-extract.cx -- --check` 冷 1824 ms、热 38 ms。

h5.cx 冷跑分相位(`UNISACC_RR_TIME=1`,单次,纳秒折毫秒):

| 相位 | 次数 | 合计 ms | 占相位和 3354 ms |
|---|---|---|---|
| e2 inference | 6 | 1120.4 | 33.4% |
| e3 inference(合并后整程序一次) | 1 | 1002.9 | 29.9% |
| lower inference | 1 | 276.8 | 8.3% |
| prune inference | 1 | 271.6 | 8.1% |
| e1 inference | 6 | 173.3 | 5.2% |
| units inference | 1 | 163.2 | 4.9% |
| elf inference | 1 | 148.1 | 4.4% |
| 模型 inflate(e2 6 次 77.4、e1 6 次 68.4、其余 48.6) | 17 | 194.4 | 5.8% |
| construction | 1 | 3.7 | 0.1% |

h0.cx 冷跑同法:e2 112.8、e3 99.3、e1 26.6、lower 12.9、prune 10.8、elf 7.7,inflate 合计约 71;对照 h0.c 全程 110 ms,可知空 `.cx` 的约 250 ms 增量基本是前导 `cx-flat.h` 过 e2/e3 的成本。h5 产出 110,974 B,h0.cx 产出 8,192 B。

读数结论:

- (1) CX0 规划本身不是瓶颈(17 ms 对 3600 ms);按 CX0BND1 身份缓存「规划结果」收益上限约 0.5%,不值得单做。值得做的是用同一身份缓存规划之后的每单元产物。
- (2) 最大两项是每单元 e2(33%)与整程序 e3(30%)。前者可按「单元源字节加强制头字节加 e2 模型 sha 加目标」做内容寻址缓存(正是 CX0BND1 单元记录的身份),对 L0 模块这类稳定单元命中率高;后者要等链接单元(CX-W-1)才能分单元缓存。
- (3) 同进程重复 inflate 同一模型 145.8 ms(e2 加 e1)是纯浪费,属机制修复。
- (4) 现有运行缓存只对「同一根源重复跑」有效(热 30 ms);工具开发的常态是改根文件,模块不变,正好落在无复用区。
- (5) 宿主冷编译(含链接)比产品冷跑快约 2.5 至 6.5 倍(h0:364 对 70 / 148;h1:880 对 136 / 209),但 L0 一用到 sys 层就不能编;三宿主体验差距的主因是这条,而不是 PCH。
- 未测:CX0 fork 工作进程的单独开销(`tools/cx0.cx plan` 对 `-cx-plan` 的差);`cx-flat.h` 裁剪后的 e2/e3 时间;多单元并行;Windows、macOS、arm64 全部数字;P4 包内 cx-flat.h 现值。

## 8. 进化计划

### B0 目标形态

一句话:cx 前端(CX0)成为三宿主共用的、由事实表驱动的第 8 流;表是唯一真源,unisacc.com 把表构造成网放进 P4,`gcc_cx` 与 `clang_cx` 把同一张表生成的判定代码编进驱动;同一 `.cx` 在三宿主得到相同的计划字节、接口字节与拒绝行。

(1) 哪些进表(一步决策,有限定义域),哪些留机制:

| 决策 | 键(先由机制分类成有限键) | 标签 | 归属 |
|---|---|---|---|
| pragma 词判定 | 词类(module use dock 其它) x 位置(首声明前 / 后) x 条件嵌套(内 / 外) x 文件类(.cx / .c) | 接受为 module / use / dock、忽略、具名拒绝 id | 拟建表 `pragma`(待建) |
| 模块名文法 | 状态(段首 段中 点后) x 字符类(小写 数字 下划线 点 其它 结束) | 后继状态或拒绝 | 拟建表 `name`(待建) |
| 文件作用域条目的接口去向 | static 与否 x 种类(函数定义 原型 对象 typedef 标签类型 匿名 enum) x 条件内 x 引用模块宏 x 保留前缀 x cxpack 行(无 / 列入 / 未列入) | 导出原型、导出类型、私有、具名拒绝 id | 拟建表 `export`(待建) |
| 接口可见性 | use 关系(直接 传递 无) x 部分(类型 函数) x 符号导入(无 匹配 不匹配) | 给出、省略 | 拟建表 `visible`(待建) |
| 读失败分类 | 错误类(ENOENT ENOTDIR EACCES 超限 其它) x 角色(源 模块 cxpack cxroot 接口发布) | status 1 / 2 与诊断 id | 拟建表 `iofail`(待建) |
| 同 realpath 合并 | 依赖集相等 x 有效公开集相等 | 合并、具名拒绝 | 拟建表 `merge`(待建) |
| 名字冲突 | 定义方对(模块-模块 单元-模块 类型-类型) x 字节相同与否 | 接受、拒绝 id | 拟建表 `collide`(待建) |
| 上限 | UNIT_MAX USE_MAX DECL_MAX 等 | 常量 | `exec/facts/limits.tsv`,动作只写 `["constant",NAME]` |

留在机制或生成器:源扫描循环(续行、注释、字符串)、括号深度跟踪、广度优先闭包与环检测、文件读写与无覆盖发布、SHA、CXIF 文本拼装、别名改写、CX0BND1 封存与读取。这与 prd.md 第 1 节「一步决策进表,循环/递归/带副作用序列留机制」一致。表的规模预计每张 10 至 200 键,合计低于 1000 键(估计,未测)。

(2) 三宿主共享真源:上述表加 `limits.tsv` 是唯一真源。拟建生成器 `cx0-gen`(待建,带 `--check`)同时产出两种等价形式:(a) 给产品的网行(经 `tools/decide-gen.cx` 的 STATE KEY LABEL 行,T1 逐键核「网 = 表」);(b) 给宿主与工具的平铺判定函数(拟生成模块 `cx0.decide`,纯 switch,无 libc),由 `plan.cx` use,并随 `cx0-extract` 投影进 `src/cxmod.c`。再加一条逐键门核「生成判定函数 = 表」。这样 `gcc_cx` 与 `clang_cx` 不需要执行器也与产品同表;0.0.55 的流网化不会只在 unisacc.com 内生效。

(3) CX0 作为第 8 模型流进 P4:`image-stages.tsv` 增一行 `cx0  src.cx  units.bundle  cx0.net  plan`,输出格式即 CX0BND1;e2 的输入从「源路径加强制头列表」改为「束加单元序号」(`tools/cx0-e2.c` 已证明可行的形态)。非 `.cx` 输入不过 cx0(保持 `.c` 逐字节红线)。执行器内无 fork:解析状态改为按实例拥有(句柄化),去掉工作进程,才能覆盖 win 两格。

(4) 骨架网落行为的顺序:骨架的目标形态是「该网的 cx 侧 ask 接口」,即 `int skn_NAME(键分量..., int *label)`,函数体由生成器从同一张金表生成(与网逐键相等),供 cx 写的工具、检查与将来的 cx 侧阶段调用。顺序按「出货主脊优先、键少优先、已有决策表优先」:第一批 reloc(11 行)regmap(21)tyinfo(22)pp(23)prec(23)binsel(37)lexword(21);第二批 opinfo(79)lex(149)lexcls(262);第三批 irsel(425)enc(528)abi(540)peep(1638,沿用 `decide-gen --each --select` 两级式)type(4281);对照臂 pfconv scope isel parse combo 最后做,并在接口头注标「对照臂,七流不读」。每张落地时 compiler 与 runtime 两份镜像收敛为一份(runtime 侧保留为同文件别名或删去,由调用点普查决定)。stream/ 7 个与 image/ 4 个骨架不落行为,只保留名字与格式访问器,步函数 `return 0` 删去(它不表示任何事实)。

(5) `src/cxmod.c` 投影退役条件(全部满足才退役):宿主驱动改为直接编 `plan.cx` 的一个不依赖 unisacc 的引导形态(或由 `cxcc` 自举:先用上一版驱动展开 plan.cx);`tests/cx-host-run.cx`、`tests/check-source-bom.cx`、`tools/build-cxcc-host.cx` 不再读它;历史 C 参考对拍不再需要它或改读生成副本。预计不早于 0.0.56。在此之前它保持「投影、逐字门」身份,禁止手改。`cx/compiler/cxmod.cx` 的退役更早:0.0.55 第一步即由产品直接 use `cx0.plan`(或由生成器从 plan.cx 投影出 cxmod.cx 并加逐字门),消灭人工同步。

### B1 0.0.54(只做不触产品输入的梳理、测量、设计、提速前置)

本段所有步骤都不改 `seed/ident.c` 目录集内的文件(`exec src kernel include weights seed cx` 与登记的 `tools/` 10 文件),源 SID 不变。每步单次操作低于 5 分钟,每条命令低于 60 秒。

| 步 | 内容 | 触产品输入 | 验收 gate(命令) | 前置 | 风险 | 规模(估) |
|---|---|---|---|---|---|---|
| 54-1 | 三源差异清单:拟写 `cx0-diff`(待建),按函数对齐 plan.cx 与 cxmod.cx,输出差异表 `cx0-three-source-diff`(函数、差异类、是否语义)。带 `check-cx0-diff` 并登记批次 | 否 | `sh ./unisacc.com tools/cx0-extract.cx -- --check`;`sh ./unisacc.com tools/ratchets.cx` | 无 | 对齐靠函数名,改名函数需手工标注 | 工具约 300 行,表约 90 行 |
| 54-2 | 三宿主一致性矩阵:扩 `tests/cx-host-run.cx` 的对拍为拟建工具 `cx-tri`(待建):对语料目录每个 `.cx` 在三入口取 `-cx-plan` 字节、`-cx-interface` 字节、拒绝行与 rc,输出 MATCH/DIFF/UNKNOWN,DIFF 不记 PASS。语料先含:无 use、单 use、多符号、整模块加符号、alias、注释/续行/条件、cxpack、双软链、环、缺模块、未知 pragma、97 单元与 129 单元(暴露 MAXU 分叉) | 否(测试与工具) | `sh ./unisacc.com tests/cx-host-run.cx` | 宿主驱动可建(54-4) | 语料生成 129 单元要控制在 55 秒内,必要时拆两条命令 | 工具约 400 行,语料约 30 文件 |
| 54-3 | L0 宿主可编译普查入库:把 `out/cxarch/hostsweep.cx` 的做法做成拟建工具 `cx-host-l0`(待建),遍历 `cx/sys cx/mem cx/text ext/*` 全部模块,输出 `cx-host-l0` 清单(模块、gcc、clang、首条错误)。只记录,不设绿线 | 否 | `sh ./unisacc.com tests/cx-host-run.cx` | 54-4 | 模块数约 80,每个 0.1 至 0.4 秒,须分片 | 约 200 行 |
| 54-4 | 修宿主引导说明:`tools/build-cxcc-host.cx` 头注命令与实际不符。本版只更正文档与 `cx/cx0/README.md` 的复现节,并在 `agentwork/items.tsv` 立修复项(改代码属 `tools/`,不在源 SID 的 10 文件内则可在本版修:让构建器不依赖 `text.text`,或由 `tests/cx-host-run.cx` 的四步配方抽成共用模块) | 否(先核该文件不在 ident 的 tools 清单) | 文档命令逐条实跑;`sh ./unisacc.com tests/cx-host-run.cx` | 无 | 若该文件在 ident 清单内则顺延到 0.0.55 | 约 60 行 |
| 54-5 | 相位基线入账:把 A8 的做法做成 `tools/cx-phase.cx`(读 `UNISACC_RR_TIME` 输出,按 stage 汇总冷/热、单元数 1/2/6/12),结果写 `agentwork/phase-times.tsv` 的 cx 行;未测相位标 UNMEASURED | 否 | `sh ./unisacc.com tools/cx-phase.cx -- out/cxphase-N`;`tools/check-cx-phase.cx` | 无 | 12 单元冷跑预计超 7 秒,仍在 60 秒内 | 约 250 行 |
| 54-6 | CX0 事实表设计稿:拟写 `cx0-tables` 设计稿与 7 张表的初稿(B0 表格所列),每张表每个键标注它对应 plan.cx 的哪一行判断;写「表 = 现行实现」的逐键探针方案。不接入任何构建 | 否(草表不进 `exec/`) | 设计评审;`sh ./unisacc.com tools/decide-gen.cx -- --check ...` 对草表可生成(只验证表形合法) | 54-1 | 键分类不全会在 0.0.55 才暴露;用 54-2 语料反查 | 文档约 300 行,表约 600 行 |
| 54-7 | 单元级缓存设计与离线原型:在工具侧(`tools/cx-pipeline.cx` 链,不动产品)实现「按 CX0BND1 单元身份缓存 e2 输出 UNIPP1」的原型,量化命中时的节省;键 = 单元源 sha、强制头 sha 序列、E2 模型容器 sha、目标、ABI;命中仍核内容身份,只记 PREPARED | 否 | `sh ./unisacc.com tests/preprocess/pipeline.cx -- ...` 不回退;新增 `tools/check-unit-frontcache.cx`(命中、失效、损坏、键变更四类) | 工作区 6 包可建 | 工具链适配器每单元进程隔离,原型数字不等于产品内数字,须标注 | 约 350 行 |
| 54-8 | 骨架与名表普查:拟写 `cx-surface-census`(待建)列出 114 处生成文件的生成器、事实表、调用点数;产出候选清理表(不自动删) | 否 | `sh ./unisacc.com tools/surface-gen.cx -- --check` | 无 | 无 | 约 200 行 |
| 54-9 | 层号裁定稿:给 `cx/cx0`、`cx/workspace` 定层(建议 cx0 为 L2 前端、workspace 为 L3),列出裁定后 `check-cx-layers` 会新报的边;只出补丁不合入(改 `tools/check-cx-layers.cx` 与规格属 0.0.55) | 否 | 补丁在仓外克隆跑 `sh ./unisacc.com tools/check-cx-layers.cx` 的结果附后 | 无 | 可能暴露真实上行边 | 补丁约 40 行 |

0.0.54 的提速只做不触产品输入的部分:54-5 测量、54-7 工具侧原型、以及开发习惯上的两条(同一根源重复跑才命中运行缓存;分析脚本尽量少 use 以降低冷启动,A8 的每 use 约 0.5 至 0.7 秒可作估算)。产品内的缓存与 inflate 修复都触 `src/` 或 `cx/runtime`,不在本版。

### B2 55-1..55-6(原定 0.0.55;现 55-1 在 0.0.58,55-2 在 0.0.59,55-4 起在 0.0.60;设计与实现主线:CX0 前端单源、入表、入流)

候选的一类大改 = 「cx 前端 CX0 单源化并入表入流」。下列步骤同属这一类,但按可恢复批次推进,每批独立绿;若慢车道评估认为 55-1 与 55-4 以后不宜同候选,则 55-1 作为 0.0.58 的 P1,55-2 作为 0.0.59 的 P59,55-4 至 55-6 作为 0.0.60 的 P60 主线(已按此排期)(判据:55-3 结束时三宿主矩阵 DIFF 是否为 0)。

| 步 | 内容 | 触产品输入 | 验收 gate(命令) | 前置 | 风险 | 规模(估) |
|---|---|---|---|---|---|---|
| 55-1 | 产品规划器单源:`cx/compiler/cxmod.cx` 改为由 `plan.cx` 投影生成(扩 `tools/cx0-extract.cx` 增第二投影目标,带 `--check` 逐字门),或产品直接 `use cx0.plan`。先抹平 54-1 清单里的语义差(MAXU 统一取 `limits.tsv` 的值,三处同数) | 是(`cx/`) | `sh ./unisacc.com tools/cx0-extract.cx -- --check`(两目标均 EXACT);`sh ./unisacc.com tests/preprocess/cx0.cx -- ROOT OUT`;字节等价回归:改前改后对 54-2 语料的 `-cx-plan`、`-cx-interface` 逐字节相同;`sh ./unisacc.com tools/affected.cx --files cx/compiler/cxmod.cx cx/cx0/plan.cx --run` | 54-1、54-2 | plan.cx 的 fork 隔离不能进产品(win 两格);投影须保留产品现有的注入回调形态(`cxm_rd`/`cxm_wr`),即产品形态与工具形态是同一体的两个适配壳 | 生成器约 200 行;cxmod.cx 变生成物 |
| 55-2 | 解析状态实例化:把 `cxm_*` 文件级全局收进一个实例结构,去掉对工作进程隔离的依赖(工具入口仍可选 fork 作为额外防线) | 是 | 同 55-1 全部;`cx0-unit` 批次;新增同进程连续两次规划、失败后再规划的恢复用例 | 55-1 | 机械改写面大(约 90 个函数);用 `tools/refactor.cx sweep` 分文件验证 | 约 1300 行改写 |
| 55-3 | 三宿主矩阵转绿并成为门:54-2 的 `cx-tri`(待建)在 lnx/x86_64 上 DIFF=0;登记批次 `cx-tri`,release 档必跑 | 否(测试) | `sh ./unisacc.com tests/cx-host-run.cx`;`sh ./unisacc.com tests/run.cx -- --smoke` | 55-1 | 历史分叉(隐式声明、公开集)需逐项裁定是修产品还是修规格;裁定记账本 | 语料增补 |
| 55-4 | 事实表入库与双形态生成:7 张草表移入 `exec/` 下拟建 `cx0` 子目录;`cx0-gen --write/--check`(待建)生成网行与 `cx0.decide`;plan.cx 的对应 if 链改为「机制分类成键 -> 调 decide -> 落动作」。此步后产品与宿主仍执行编译出的判定函数(尚未走网) | 是(`exec/`、`cx/`) | 逐键门 `check-cx0-decide`(待建,判定函数 = 表,全键);`tests/check-literal.cx`、`tests/check-limits.cx`;55-1 的字节等价回归;`cx-tri` 仍 DIFF=0 | 54-6、55-2 | 键分类遗漏导致行为变化;用「每个拒绝 id 至少一个语料」的覆盖表守住 | 表约 600 行;生成器约 400 行;plan.cx 改约 300 行 |
| 55-5 | CX0 网与第 8 流:`image-stages.tsv` 增 cx0 行;网行经现有构造链进 P4(模型数加 1);驱动对 `.cx` 输入先跑 cx0 流产出 CX0BND1,e2 从束取字节。执行器内的机制部分(扫描、闭包、IO)以生成程序或资源适配壳承担,分两小步:(a) 仅判定走网、机制仍是切片内代码;(b) 束作为 e2 的正式输入格式 | 是(`exec/`、`weights/`、`src/`、`kernel/` 生成物) | T1 逐键(批次 `check-decide` 同款);`sh ./unisacc.com tools/gold-table.cx -- --check`;`sim --cov`;`.c` 红线:同一 `.c` 语料改前改后产物 sha 相同;`.cx` 语料产物字节等价;慢车道正规 context、PC、两代 GEN、安装与全批;同候选六目标 CI | 55-4 | 这是本线最重的一步:改流表会波及 pp/lower/generator/facts(第 14 号门 net-deps);启动开销可能上升(多一次模型装载,按 A8 约 12 至 30 ms),须用 54-5 基线对比;win 两格无 fork 路径必须先由 55-2 保证 | 流表 1 行;驱动改约 400 行;构造链按相位各低于 5 分钟 |
| 55-6 | 骨架网第一批落行为(B0 第 4 点第一批 7 张)并合并 compiler/runtime 镜像;`surface-gen` 改为从金表生成 ask 函数体,`--check` 逐字;新增逐键门「ask 函数 = 金表」 | 是(`cx/` 生成物) | `sh ./unisacc.com tools/surface-gen.cx -- --check`;`tools/check-cx-surface.cx`、`check-cx-decl.cx`、`check-cx-gold.cx`;三宿主各跑一遍逐键门(生成函数无 libc,应三宿主可编) | 54-8 | 与 55-5 同候选会使「一类大改」边界变宽;若慢车道时间超 2 小时则本步移到 0.0.56 | 生成器改约 300 行 |
| 55-7 | 产品内提速(与主线同输入域,独立小批):(a) 同进程模型 inflate 去重(A8 第 7 项,预期每多一单元省约 24 ms);(b) 按 CX0BND1 单元身份的 e2/e1 结果缓存(预期对 h5 类程序省 5 个模块单元的 e2 加 e1,约 1.0 s 中的大部分;以实测为准) | 是(`src/`、`cx/runtime`) | `tools/cx-phase.cx` 冷/热两组对比入 `phase-times.tsv`;缓存负例:键变更、内容损坏、截断、并发写,均回退重算且结果字节相同;`UNISACC_RUN_CACHE=0` 路径不变 | 54-5、54-7、55-5(b) | 缓存结果不得替代执行结论;命中仍核内容身份;跨进程并发写需无覆盖发布(沿用 link 发布法) | 约 500 行 |

0.0.55 的整体迭代(冻结到可发布)须低于 2 小时;55-5 的构造按既有相位工具分片,任何单相位超 5 分钟先分解再入库,并登记 `agentwork/phase-times.tsv`。发布说明如实写:CX0 流仅对 `.cx` 生效;三宿主一致性只在 lnx/x86_64 实跑;宿主 L0 缺口清单(54-3)原样列出。

### B3 0.0.56 及以后

| 步 | 内容 | 触产品输入 | 验收 gate | 前置 | 风险 | 规模(估) |
|---|---|---|---|---|---|---|
| 56-1 | L0 宿主后端:`cx/sys` 内把裸 `__syscallN` 收进 `_os` 静态帮手的宿主分支(`__UNISA__` 下走内建,否则走宿主 libc),按 54-3 清单逐模块转绿;目标是 L0 三宿主可编 | 是 | `cx-host-l0`(待建)失败数只降不升(棘轮);`tools/check-cx-sys.cx` 在三宿主各跑 | 54-3 | 导出函数不得包进平台 `#if`(CX-Y-3);产品字节须等价(`__UNISA__` 分支不变) | 15 个模块,每个 20 至 80 行 |
| 56-2 | 链接单元(CX-W-1)落地:每模块按 realpath 编一只对象;随后 e3 及以后各流的每单元结果可按内容缓存(A8 中 e3 30% 加 prune/lower/elf 约 21% 才可分摊) | 是 | `cx/spec.md` CX-W-1 的验收组;`cx-phase` 对比 | 55-7 | 改变 `-funit` 对象语义;一类大改,独占一个候选 | 大 |
| 56-3 | `src/cxmod.c` 退役:宿主驱动改由 plan.cx 直接引导(B0 第 5 点条件全部满足后),删除投影与 `compat-prefix.c.in`,`cx0-extract` 降为只守 cxmod 生成物或一并退役 | 是 | `tests/cx-host-run.cx`、`tests/check-source-bom.cx`、`cx-tri` 全绿;全仓无 `src/cxmod.c` 读者(`tools/refactor.cx sites`) | 55-1、56-1 | 宿主自举出现鸡生蛋:保留一份上一版驱动产物作引导并记录依赖 | 小 |
| 56-4 | 骨架网第二、三批与对照臂(B0 第 4 点);之后 cx 侧可用 ask 接口写阶段级检查 | 是 | 各张逐键门;`surface-gen --check` | 55-6 | type 4281 行生成函数体较大,注意单元编译时间(每命令低于 60 秒) | 中 |
| 56-5 | 前导裁剪:层感知前导(CX-W-5),上层单元只见 L0 接口声明;或对 `cx-flat.h` 的 e2/e1 结果做身份缓存。先用 54-5 数据判断哪条收益大(空 `.cx` 的约 250 ms 增量是上限) | 是 | `tools/cx-flatten.cx -- --check ROOT`;`tests/cxlibc.cx`;`.cx` 语料产物字节等价 | 55-7 | 改前导波及全部 `.cx`;独占候选 | 中 |
| 56-6 | CXIF v1(CX-W-8)与编译期所有权(CX-W-4)的设计:接口带效果/布局/所有权字段,由 cx0 流输出结构化记录直接进 e3;v0 文本须可由 v1 逐字再生。三宿主同一检查器(检查在 cx0 流内完成,宿主驱动复用同一判定表),否则不声称 | 是 | 设计评审后另立 | 55-5 | 最大的不确定项;未实现前任何文档不得写「内存安全」 | 大 |
| 56-7 | 层与软链收敛:cx0/workspace 定层入规格(54-9 补丁);`src/` 7 个目录软链与 mem/text 互指链接按 cxroot 解析淘汰(CX-W-6) | 是 | `tools/check-cx-layers.cx` AMB 计数归零、软链入口计数归零 | 已装产品含 CX-U-7 | Windows 路径行为;csih 仍依赖 `sys.net` 别名,须先发信封给 csih 车道 | 中 |
| 56-8 | 宿主平台资格:`gcc_cx` 与 `clang_cx` 在 macOS、Windows 实跑 `cx-tri` 与所有权检查 | 否 | 同候选 CI 的宿主作业 | 56-1 | CI 时长;每作业须低于 5 分钟 | 小 |

### B4 一致性与不变式(贯穿各版)

- 三宿主同语义门:`cx-tri`(54-2 建,55-3 起为门)。判据是同一 `.cx` 的计划字节、接口字节、拒绝行与 rc 三入口相同;DIFF 与 UNKNOWN 都不记 PASS;未执行的平台写未执行。
- 单一真源链:拟建 CX0 事实表与 `exec/facts/limits.tsv` -> `cx0-gen --check`(待建)->(网行;`cx0.decide`)-> `cx/cx0/plan.cx` ->(投影 `cx/compiler/cxmod.cx`、`src/cxmod.c`,逐字门 `tools/cx0-extract.cx -- --check`)。任何一环手改都被 `--check` 报红。
- `.c` 红线:cx0 流不扫描 `.c`;`.c` 语料的产物 sha 在每一步前后相同。
- 缓存纪律:只缓存有测量依据的构造产物(单元 e2/e1 结果、模型解包);键含全部输入身份;命中仍核内容;不缓存测试结论;冷/热两组数据与回退负例同时入账。
- 所有权措辞:运行期合同与句柄检查三宿主可复验(A6 表);编译期所有权在 CX-W-4 落地前三宿主都写「未声称」。
- 分治时限:本计划内没有预计超过 5 分钟的单次操作;55-5 的慢车道若实测超限,先登记 `agentwork/phase-times.tsv` 并分解,不放宽时限。
- 车道:`apps/csih/`、`ujs/` 不改;涉及它们依赖的别名(如 `sys.net`)先发信封。

## 9. 并入:分层、模块与 use、dock、别名与再导出、L0 fmt 与 sig 合同(2026-11-14 并入,批 B3)

来源:plans/cx-layers.md、plans/cx-modules.md、plans/cx-dock-pragma.md、plans/cx-use-alias-reexport.md、plans/cx-l0-fmt-2026-10-30.md、plans/cx-l0-sig-2026-10-30.md、cx/prd.md、docs/cx/prd.md(cx/prd.md 与 docs/cx/prd.md 原文已归档到 archive/plans-cx-2026-11/;六篇 plans/cx-*.md 仍被 cx/sys、cx/core、include/、ARCHITECTURE.md、prd_cx_spec.md 等写域外文件引用,check-archive-refs 会失败,故本批暂不归档,待引用改完再动;历史施工记录与 closure-once 设计过程留在原文,本节只收仍有效的规则)。原则:每条标已核对的源码路径;核不到的不并入。

### 9.1 分层与层向规则
来源 plans/cx-layers.md「层」「只有底层能用 include」「三个检查表」。已核对:cx/sys、cx/mem(arena bytes grow heap map slice vec)、cx/text(build codec conv deflate edit fmt iter num text utf8) 目录存在;build/layer-api/cx.sys.tsv 等为包面登记。
- 层就是目录:cx/sys、cx/mem、cx/text 整体是 L0(宿主对接,可 include 与直调 libc/POSIX);cx/core、cx/runtime 是 L1;cx/driver、cx/compiler、src/ 是 L2;tools、tests、apps 是 L3。
- 上层不写 `#include`、不直调 libc/POSIX(走 sy_* mm_* tx_*)、不写平台 `#if` 与 `__syscallN`;跨模块错误只返回 `-errno`(spec CX-Y-1..4,cx/spec.md)。产品代码严格按此;tools/tests 脚本输出允许 printf(见 9.3 判据)。
- 许可表现状:原文的两张目录白名单(tests/cxraw-layers.tsv、tests/cxinclude-boundary.tsv)现仍在库中,但 tests/cxraw.cx、cxlibc.cx、cxplatform.cx、cxinclude.cx 已改为先读文件内 dock 声明(核对:这四个文件的注释均写 "Per plans/cx-dock-pragma.md");以 9.3 为准,原文"只看 TSV"已过时。
- 棘轮扫描面:`git ls-files` 下 `*.cx`、`cx/**/*.c|.h|.c.in`,排除 archive;src/、exec/、tools/ 的宿主 C 不在分层叙事内。一次跑 `tools/ratchets.cx`。
- 不并入:原文 closure-once 施工图、cli.cx 收敛阻塞链、各「已落地」流水(过时或为历史;见归档)。

### 9.2 模块与 use 语法
来源 plans/cx-modules.md 前部(需求、产品面、阶段 CX0..CX4、红线)。已核对:cx/compiler/cxmod.cx(cxm_directive 解析 module/use/dock)、cx/spec.md(CX-G-1、CX-W-3)。
- `.cx` 逐步改造为无 `#include` 的模块,API 由 unisacc 提供;`#pragma unisa module NAME`、`#pragma unisa use a.b`(NAME 是模块路径 seg(.seg)*,seg=[a-z_][a-z0-9_]*,不是符号名)。pragma 之外不新增 C 关键字,宿主 gcc/clang 忽略整行(gcc_cx/clang_cx 的前提)。
- 命名空间:两个被 use 的模块导出同名符号 -> 拒绝(tests/cx-namespace.cx 锁定);模块循环拒绝。
- 不并入:原文第 133..164 节为 2026-10-30 入口候选的逐日构造记录(历史,见归档);阶段表以本文件第 8 节为准。

### 9.3 dock pragma
来源 plans/cx-dock-pragma.md。已核对:cx/compiler/cxmod.cx 第 535 行识别 `dock`;cx/sys/sig.cx 头部有 `#pragma unisa dock`;tests/cxraw.cx、cxlibc.cx、cxplatform.cx、cxinclude.cx 读 dock;tools/dock-map.cx 引用判据。
- 语法 `#pragma unisa dock [libc|include]`:裸 dock 允许 `#include` 与裸 libc/POSIX;`dock include` 只允许 include;`dock libc` 只允许裸调。
- 规则:无 dock 的单元出现 `#include` 或裸 libc 符号 -> 拒绝(`dock required for ...`)。上层模块可 use dock 模块。产品路径里出现 dock 须人工审。
- 判据(2026-11-04 已定):只有声明了 `#pragma unisa module`、或被他单元 `use` 的单元才是"模块",才受 dock 约束;tools/tests 里既非模块也未被 use 的纯脚本不受约束,printf 自由。tools/cxlib/*、tools/cxcc.cx、tools/cxproc.cx 等被 use 的仍按模块处理。
- 三源一致:cx/cx0/plan.cx(权威)、src/cxmod.c(投影)、cx/compiler/cxmod.cx;改一处须 cx0-extract --check 报 EXACT。
- 未并入:原文"落点/迁移"计划表(已执行或已演变),待办中的"白名单 TSV 废弃"未核到完成,仍视为未完成项。

### 9.4 use 别名、符号级导入与再导出
来源 plans/cx-use-alias-reexport.md。已核对:cx/compiler/cxmod.cx 第 523 行解析 `as`、第 862 行输出 `sym as alias`;cx/spec.md 第 252 行 CX-W-3。
- 已落地:`#pragma unisa use NAME::sym [as alias]`(符号级导入与别名),`::` 只在 pragma 行内;同名函数经别名或符号级导入消解冲突;三源(plan.cx、src/cxmod.c、cxmod.cx)一致。
- 未落地:`export`(重导出,CX-W-3)。cxmod.cx 无 `"export"` 识别,与 spec 一致。
- 已知边界:多单元用不同别名 use 同一模块时只存一份改名源码(单版本假设);未导入函数体仍被编译,仅改成唯一名避免链接冲突;传递 use 的符号级过滤不追踪。
- 未核:原文"产品 unisacc.com 尚未重编因而两个用例 FAIL"是 2026-10-31 状态,已过时,未并入。

### 9.5 L0 fmt、num、io 合同(2026-10-30 csih 车道需求)
来源 plans/cx-l0-fmt-2026-10-30.md。已核对:cx/text/fmt.cx(tx_snprintf)、cx/text/num.cx(tx_atol、tx_strtol、tx_strtoul、tx_strtod 与 _r 版)、build/layer-api/cx.sys.tsv(sy_printf、sy_isatty、sy_fd_cloexec);tools/check-cx-fmt.cx 与批次 cx-l0-fmt 存在。
- text.fmt:tx_snprintf、tx_vsnprintf 与 snprintf 逐字节等价(截断、NUL 结尾、返回完整长度、cap 为 0 时 dst 可空);cx_fmt 溢出会清空 dst,不能替代。
- text.num:tx_atol、tx_strtol、tx_strtoul、tx_strtod 对齐 libc;errno 每单元一份、不跨模块,要判越界必须用 `_r` 版(0、-EINVAL、-ERANGE 经返回值)。迁移 `errno=0; strtol; errno==ERANGE` 写法须改 `_r`,不能只换函数名。
- sys.io:sy_printf、sy_eprintf、sy_vprintf、sy_veprintf、sy_puts、sy_fd_nonblock、sy_fd_cloexec(0 或 -errno,Windows -ENOSYS)、sy_isatty(1 或 0)、sy_strerror;sys.env:sy_euid。标准流前导不缓冲,sy_* 与残留 printf 交错顺序一致。
- 未并入:原文检查数字(38 条、candidate 哈希)为当日实测,不并入;现状以 `tools/check-cx-fmt.cx` 实跑为准(本批未实跑,标 UNMEASURED)。

### 9.6 L0 sig、atexit、symlink、setpgid 合同
来源 plans/cx-l0-sig-2026-10-30.md。已核对:build/layer-api/cx.sys.tsv 含 sy_sig_handle、sy_sig_default、sy_raise、sy_atexit、sy_exit;cx/sys/sig.cx 头部说明、cx/sys/fs.cx(sy_symlink)、cx/sys/proc.cx(sy_setpgid);tools/check-cx-sys-sig.cx 存在。
- 前导 signal/sigaction 只填进程内表,不进内核;sy_sig_handle 经 rt_sigaction 真正注册(SA_RESTART),返回 0 或 -errno(越界、SIGKILL/SIGSTOP、空 fn 为 -EINVAL),macOS 与 Windows 为 -ENOSYS;sy_sig_default、sy_sig_ignore、sy_raise 同为 0 或 -errno。
- 处理函数入口:tape ABI 与平台 C ABI 不同,不能直接把函数地址交内核;运行时映射一页 thunk 再转 syg_dispatch;arm64 用 sigaltstack(256 KiB)加 SA_ONSTACK。macOS 不提供处理函数。
- sy_atexit/sy_exit:模块自带表,最多 32 项,后进先出;只在 main 返回(模块析构链)或 sy_exit 时运行;别的单元 exit、_exit、信号致死不运行;表满 -ENOMEM。
- sy_symlink:0 或 -errno,空参 -EINVAL,已存在 -EEXIST,允许悬空,Windows -ENOSYS。sy_setpgid:原始系统调用,不是前导转发;-EINVAL/-ESRCH/-EACCES/-EPERM 语义见原文归档。
- 限制:errno 每单元一份,处理函数调用本单元前导函数应自存自恢复;arm64 备用栈只设给第一个注册线程。未实跑项:osx 与 win 只编到 tape/二进制,未执行。

### 9.7 扩展约定与宿主 .com 要求(并入自 cx/prd.md 与 docs/cx/prd.md 的有效部分)
已核对:tools/build-cxcc-ape.cx、tools/cxproc.cx 存在。
- 语言扩展继续只走 `#pragma`(宿主按标准忽略不认识的 pragma);声明级事实将来可加 C23 `[[unisa::...]]` 属性,现在不加;旁路文件只放层号、生成表、条款,不放 `use`;新关键字、宏库、注释契约不能作本树约定。
- gcc_cx.com 与 clang_cx.com 必须是同一份六格 APE(win/lnx/osx x x86_64/arm64),打包五份切片(win/arm64 复用 win/x86_64 的 PE);构建入口 `sh ./unisacc.com -run tools/build-cxcc-ape.cx`。没有 use 时直接交系统编译器(`-std=c99 -x c -include include/cx.h`);宿主不实现 `-run`。尺寸与字节数为旧快照,不并入。
- tools/cxproc.cx 是按需模块(导出 cxproc_run);包清单 cxpack.tsv 把导入身份钉到路径;CXIF v0 文法见 archive/plans/2026-10-28/cxif-v0.md。
- 未实现:编译期所有权(spec 第 5 节)、非本机切片未在 Windows/OSX 上执行。

## 10. cx 2.0 结构草案(2026-11-14,cx 设计负责人;草案,未实现项一律不得记 PASS)

立场:模块、接口、包边界是一等公民。判据只有一个:改一处,需要重编、重跑、重审的范围是否随边界缩小。本节数字除注明外均为 2026-11-14 在 HEAD 4367e1b0e5、已装产品、lnx/x86_64 上实测;已装产品不含 4aee1dfb4a、e576000457、4367e1b0e5 的修复,凡涉及它们的行为都标 UNVERIFIED-需新产品。

### 10.1 边界问题清单(证据与可测度量)

| 号 | 问题 | 证据 | 度量(现值 -> 目标) |
|---|---|---|---|
| E1 模块身份 | 同一文件经软链两条路径曾被当成两个模块;`src/` 下 7 个目录软链使同一文件有两个词法身份 | 修复 e576000457(`cxm_bind_real`)、4367e1b0e5(`cxm_deps_same`);`tests/module/units.cx` 软链夹具;A2 节 | 软链入口数 7 -> 0(`tools/check-cx-layers.cx` 口径);新产品上 `osx-sentinel -funit` 实编:UNVERIFIED |
| E2 导出面 | 默认导出全部非 static;只有带 `cxpack.tsv` 的目录才收窄 | `git ls-files` 中 `cxpack.tsv` 7 个(5 个是 `tests/cxpack/` 夹具),产品侧只有 `cx/driver`、`cx/stream` 两个;带 `module` 行的 `.cx` 587 个 | 有显式公开集的产品包 2/11 -> 11/11(`build/layer-api` 的 cx.* 11 包) |
| E3 重导出 | `#pragma unisa export` 未实现,门面模块写不出来,拆模块必然破坏全部使用方 | 实测 `-cx-plan`:`unknown unisa pragma export`,rc=1;`cx/spec.md` CX-W-3 | 拆一个模块需改的使用方文件数:text.text 为 826 -> 0 |
| E4 通配 | `use sys.*` 从未设计,落到模块名文法拒绝 | 实测:`bad module name "sys.*"`,rc=1;`use text.text::*` 被当成普通名解析 | 两种写法的拒绝文案具名且被批次锁定:0 -> 2 |
| E5 接口哈希 | 哈希粒度是包(18 行),不是模块或符号;窄化门是全局的,任一包的表过期就整体关闭 | `build/iface-hash.tsv` 18 行;实测 `affected --files cx/text/text.cx --iface-narrow` 输出 `IFACE_NARROW state=off reason=iface-hash-check`(起因是别的窗口未提交的 `build/layer-api/cx.sys.tsv`),选批 903/1069 未降 | 窄化可用率(按包判定)与选批数:903 -> 见 10.2 P5 |
| E6 波及面 | 改 `cx/text/text.cx` 选中 903/1069 批次(84%);直接 `use text.text` 的 `.cx` 826 个 | 实测 `AFFECTED_SUMMARY changed=1 batches=903 scanned=1069`(1.6 s) | 单函数体改动的选批数:903 -> 该函数的可达批次数 |
| E7 包拆面 | text.text 的 20 个导出里,826 个直接使用者中 552 个(67%)只用长度/比较族,用查找族 205、切分族 46、`tx_copy` 136 | `git grep -lw` 逐符号计数(tx_eqz 393 文件、tx_len 274、tx_find 138、tx_copy 117、tx_trim 2) | 改查找族时直接使用者波及 826 -> 约 205(传递闭包另计,见 P4) |
| E8 前导 | `include/cx-flat.h` 9388 行每单元重付;空 `.cx` 冷跑 359 至 371 ms 对空 `.c` 109 至 114 ms | A8 节;`wc -l include/cx-flat.h` | 空 `.cx` 冷跑增量约 250 ms -> 层感知前导后的实测值(CX-W-5) |
| E9 命名空间 | 可见名靠全局前缀(`tx_`、`sy_`、`mm_`)而不是模块限定;冲突只能拒绝或逐符号别名 | CX-N-1..3;`tests/cx-namespace.cx` | 仓内别名使用文件 6 个、符号级导入文件 3 个:语义未稳定前不扩大 |
| E10 符号级导入 | 未导入的函数体仍被编译(改唯一名);多单元不同别名时只存一份改名源码;传递 use 的符号过滤不追踪 | 9.4 节已知边界;修复 4aee1dfb4a | 同一模块被 N 种别名导入时的单元数与函数体份数:应恒为 1 份 |
| E11 可见性层级 | 只有两级:static 私有、其余全公开;没有「包内可见、包外不可见」 | `cxm_exportok` 只判 static 与 cxpack 行(`cx/cx0/plan.cx:681`) | 包内帮手被包外直接调用的点数(用 `tools/refactor.cx sites` 统计):未测 |
| E12 封装与对接 | 允许裸 libc 与 include 的 dock 模块 56 个(cx 下 35 个),没有逐模块的审计表 | `git grep -l '^#pragma unisa dock'` | dock 模块数只降不升;每个 dock 模块有一行审计(见 11.6) |
| E13 上限分叉 | `CXM_MAXU` 产品 128、权威 96;`CXM_MAXUSE 48` 三处各写 | `cx/compiler/cxmod.cx:28`、A7 第 14 项 | 三源常量差异 1 -> 0 |

### 10.2 设计提案

每条标:产品输入类别(cxmod 三源 | 运行库头 | 模型装载 | 非产品输入的工具与表)与版本。

**P1 export 重导出(cxmod 三源;0.0.64)**

- 语法:`#pragma unisa export NAME` 与 `#pragma unisa export NAME::sym`。只许出现在带 `module` 行的单元里;位置规则与 use 相同(首声明前、条件外)。
- 语义:(1) 对本模块等于 `use NAME`;(2) 直接 use 本模块的单元,视同也直接 use 了 NAME(给完整接口,而不是传递 use 的类型部分);(3) 符号身份仍属 NAME,函数体只有一份,不复制、不改名;(4) 链式重导出只沿显式 export 行传递,普通 use 仍不传递(CX-U-5 不变)。
- 冲突:使用方同时直接 use NAME 与经门面得到 NAME,是同一模块,按规范路径合并,不算冲突;两个不同模块导出同名仍报 `exported by both`(CX-N-1 不放宽)。
- 首版明确拒绝:`export NAME::sym as alias`(文案 `export alias unsupported`);非模块单元写 export(`export outside module`);export 自己(`module M exports itself`);export 形成环(沿用 `module cycle`)。
- 层向:export 算一条 use 边,门面的层号不得低于被导出模块(`tools/check-cx-layers.cx` 同口径)。
- 验收实验:`sh ./unisacc.com tests/cx-namespace.cx -- ROOT ROOT/unisacc.com OUT` 新增门面正例 2 条、拒绝 4 条;三源 `sh ./unisacc.com tools/cx0-extract.cx -- --check` EXACT。

**P2 通配 use 的窄语义(cxmod 三源;0.0.64,排在 P1 之后)**

- 语法:只有 `#pragma unisa use PFX.*` 一种;`*` 只能是最后一段且独占一段。`PFX.*::sym`、`PFX.*.x`、`*`、`use NAME::*` 一律具名拒绝。
- 语义:纯展开。把 PFX 解析成目录(旁边优先,其次包根),取该目录下直接子文件 `<seg>.cx` 中带 `#pragma unisa module <seg>` 的那些,按字节序展开成逐个 `use PFX.<seg>`;不递归子目录。展开后与手写完全等价:冲突仍拒、上限仍按 `CXM_MAXUSE` 计、空展开拒绝(`wildcard PFX.* matches no module`)。
- 限制:只许 L3(tools、tests、apps)使用;产品层(`src/`、`cx/`、`ext/`)出现即棘轮拒绝。理由:通配让闭包依赖目录列表,新增一个文件就改变所有通配使用方的编译输入,与「下游只依赖用到的面」相反。
- 代价如实:规划器要新增列目录回调(类比 `cxm_bind_real`),CX0BND1 束要记录目录列表身份;计划字节须把展开结果逐条写出,保证三宿主同字节。推荐做法仍是 P1 的显式门面,通配只是脚本便利。
- 验收实验:同上批次,正例 1 条(展开数与目录实数相等)、拒绝 5 条。

**P3 符号级导入的稳定语义(cxmod 三源;缺陷修复已并入 0.0.62,语义收敛在 0.0.64)**

- (1) 可见性:`use M::sym` 只给 sym 的原型与 M 的全部类型;类型永不改名。(2) 身份:一个模块在一个程序里只有一份函数体,与被多少单元、以多少种别名导入无关;别名是使用方单元的属性。(3) 同一单元对同一模块既整模块导入又符号导入:取并集,不报错;同一 sym 两个不同别名:拒绝。(4) sym 不在 M 的公开集:具名拒绝(`module M does not export sym`)。(5) 经 P1 门面得到的模块也可写 `use FACADE::sym`,解析到真实属主。
- 验收实验:`sh ./unisacc.com tests/module/check.cx`(批次 module-units)断言「N 种别名时单元数不变」;运行断言 UNVERIFIED-需新产品。

**P4 text.text 拆面(运行库头;0.0.64,依赖 P1。先行部分为非产品输入,立即可做)**

- 目标面(按 E7 实测的使用分布,而不是按文件大小):扫描面 `text.cmp`(tx_len tx_same tx_eq tx_eqz tx_cmp tx_prefix tx_prefixz tx_suffix)与 `text.find`(tx_find tx_rfind tx_chr tx_rchr tx_contains tx_count);切分 `text.split`(tx_fields tx_field tx_trim tx_trimc);构造面 `tx_copy` 归 `text.build`;格式化面即现有 `text.fmt`、`text.num`、`text.conv`,不动。`text.text` 保留为只含 export 行的门面,826 个使用方零改动,之后按需逐步改成只 use 用到的面。
- 为什么必须等 P1:没有 export 时,新模块与 text.text 同时定义同名函数会触发 `exported by both`,只能一次改完 826 个文件,违反分治。
- 先行(非产品输入):在只读副本里做拆面实验并用 `tools/affected.cx` 量化;预估见 10.4。

**P5 接口哈希驱动的选批与增量(非产品输入的工具与表;立即可做,不等版本)**

- (a) 窄化门按包判定:只要求被改文件所属包的 `layer-api` 与哈希是当前的,别的包过期不再关掉全局窄化(E5 实测的失效原因)。
- (b) 哈希粒度加一级:`build/iface-hash.tsv` 增加模块行(`pkg/module`),符号行已有 `sig_sha`(`build/layer-api/*.tsv` 第 3 列)。
- (c) 符号级选批:被改文件的改动行映射到文件作用域条目;若只落在若干导出函数体内,则只选「闭包内引用了这些函数(含同模块内调用它们的导出函数)」的批次;改动落在 static 数据、宏、类型或无法归属时退回整模块。未选中的批次打印为 DEFERRED_BY_SYMBOL,计数、可用 `--full` 重跑,不静默丢弃;release 档永远全选。
- 验收实验:`sh ./unisacc.com tools/affected.cx -- --files cx/text/text.cx --list-only --iface-narrow` 的 `IFACE_NARROW state=on`;符号级对 `tx_trimc` 的选批数远小于 903(预估见 10.4)。

**P6 前导分层(运行库头;不早于 0.0.64 末位,超 2 小时即移 0.0.65)与零解码(模型装载;0.0.63)**

- 层感知前导(CX-W-5):上层单元只见 L0 接口声明,不再每单元付 9388 行;与 0.0.63 的零解码互不依赖,分两个候选,各自用 `tools/cx-phase.cx` 冷热两组入账。
- 验收实验:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com /tmp/h0.cx`(空 main),e2 相位下降量入 `build/phase-times.tsv`。

**P7 包内可见性(非产品输入的表先行;cxmod 三源在 0.0.64 之后)**

- 不加语法。先把 `cxpack.tsv` 扩到 11 个 cx.* 包(E2),公开集 = `build/layer-api` 的行;包内帮手不进 cxpack 即包外不可见。表与检查可立即做;规划器强制(包外调用未列入符号即拒绝)已有 `tests/cxpack/bad-call` 路径,扩面属产品输入。

版本映射汇总:0.0.62(库包与运行库)只并入 E1、E10 的缺陷修复与 P5、P7 表、P4 先行实验;0.0.63(模型与网)只做零解码;0.0.64(cx 设计)做 P1 -> P2 -> P3 收敛 -> P4 物理拆面 -> P6 前导(末位)。

### 10.3 与冻结层、三源、棘轮的冲突与兼容

- 冻结层:`cx/sys`、`cx/core` 只可新增文件(`tests/cx-module-freeze.tsv` 33 行,只缩不扩)。P1/P2 不改这两层的任何文件;P4 只动 `cx/text`(不在冻结集),新面是新增文件,`text.text` 改成门面属于改现有文件,须在 0.0.64 候选内一次完成并做字节等价回归(826 个使用方的 `-cx-plan` 单元集只增不减、程序输出不变)。
- 三源:每条语法改动的顺序固定为 `cx/cx0/plan.cx`(权威)-> `tools/cx0-extract.cx` 投影 `src/cxmod.c`(`--check` EXACT)-> `cx/compiler/cxmod.cx` 手工对齐并在 `tests/module/units.cx` 加同名断言。4367e1b0e5 证明手工对齐会漏(`cxm_deps_same` 晚了一轮);P1 起每个新拒绝文案都要同时出现在三份源里,用一条 `git grep -c` 断言守住,直到 55-1(cxmod.cx 改为投影)落地。归一方案与先后关系见 10.5:决定为先归一(D0)再加语法(D1 起),D0 落地后本条的「手工对齐」一步取消。
- 棘轮:srcshape 不许 `src/` 增行,P2 的列目录回调绑定须与 e576000457 同法(与现有语句同行或放 `cx/driver`);cxinclude、cxlibc、cxplatform 不受影响;通配的「产品层禁用」做成 cxapis 类棘轮的一条规则,不新开第 14 个棘轮。
- 一版一类:P1、P2、P3 同属 cxmod 三源,可同候选;P4 物理拆面属运行库头,与前者不同类,须是 0.0.64 内的第二个候选或顺延,不得与 P1 同候选。
- 宿主:gcc_cx、clang_cx 经 `src/cxmod.c` 自动得到同一语义;export 与通配对宿主编译器仍是被忽略的 pragma 行,C99 兼容不变。
- 不做:不引入新关键字、不引入按路径字符串的导入、不做隐式遮蔽(后 use 覆盖先 use)、不做传递可见。

### 10.4 先行实验与预估(本节数字由段 2 实测后原位更新;未测项标 UNMEASURED)

- text.text 拆面波及面(2026-11-14 实测,A11,只读):当前改 text.cx 任一符号,`affected.cx --files cx/text/text.cx --list-only --iface-narrow --symbol-narrow SYM` 对 tx_find、tx_rfind、tx_copy、tx_trim、tx_fields、tx_eq、tx_len、tx_prefix、tx_count 全部给 batches=769(scanned=1073),符号级收窄对 text.text 内符号等于没收窄;只有 tx_upper(定义不在 text.cx)给 4。拆面后预计批次数的算法:`symindex.cx calls SYM --tracked` 取各族调用点文件(去 archive/ 与 text.cx 自身),再 `affected.cx --files <这些文件> --list-only` 数 use 闭包波及批次(上界:调用方文件变动波及,而非调用方使用该族):

| 族(符号) | 调用文件数 | 当前选中批次 | 拆面后预计批次 |
|---|---|---|---|
| find(tx_find tx_rfind tx_chr tx_rchr tx_contains) | 201 | 769 | 385 |
| copy(tx_copy) | 130 | 769 | 459 |
| trim(tx_trim tx_trimc tx_is_ws) | 2 | 769 | 1 |
| split(tx_fields tx_field tx_count) | 44 | 769 | 207 |
| cmp(tx_eq tx_eqz tx_same tx_cmp) | 487 | 769 | 596 |
| prefix(tx_prefix tx_prefixz tx_suffix) | 93 | 769 | 266 |
| len(tx_len) | 308 | 769 | 420 |
| upper(tx_upper,对照) | 2 | 4 | 15 |

  结论:trim、split、prefix 拆成独立模块收益大(769 -> 1/207/266),find 减半,cmp 与 len 收益有限(调用面广)。未测:真实拆面后的 use 闭包(未改源码,为调用文件反推估算)、拆面对 tx_* 内部互相调用的影响;upper 估算(15)大于符号收窄(4),说明该估算偏保守。
- 符号级选批对单函数的选批数:text.text 内符号 769,text.text 外 tx_upper 4(见上);其它模块未测,UNMEASURED。
- export 与通配原型:UNMEASURED(段 3,只出补丁,不入主仓)。

### 10.5 cxmod 三源归一(2026-11-14 设计;未实现,不得记 PASS;即 B2 的 55-1 细化)

结论:`cx/compiler/cxmod.cx` 可以全部变成 `cx/cx0/plan.cx` 的生成物,不需要长期保留「共享核 + 手写差异层」;差异层收成一张投影事实表加两个宿主适配片段。前提是先把只在产品源的语义上移到权威(两条 GAP),这一步改变宿主侧行为,属产品输入。

**现状证据(2026-11-14,HEAD 98e5adbc09 工作区实测)**

- 三源门:`sh ./unisacc.com tools/check-cxmod-tri.cx` 末行 `CXMOD_TRI passed=10 failed=0 gaps=2`(派工方实跑;本节未重跑)。两条 GAP 的发出处:`tools/check-cxmod-tri.cx:121`(cxm_checkcalls)、`:122`(CXM_MAXU,未跑 97 夹具)。
- 差异规模:`git diff --no-index --numstat cx/cx0/plan.cx cx/compiler/cxmod.cx` 为 +301 -221,68 个差异块;`src/cxmod.c` 对 `cx/compiler/cxmod.cx` 为 +253 -79。即产品源与 C 投影的距离比与权威的距离近,现有投影步(去 static)已覆盖大部分机械差异。
- 68 块归成六类(逐块入口用上面的 diff 命令加 `-U0`):
  - (a) 可见性:plan.cx 全 `static`,产品源 `cxm_*` 函数与表为外部可见(约 50 个单行块)。`tools/cx0-extract.cx:29-41` 的 `classic_visibility` 已做同一变换。
  - (b) 宿主接线头:plan.cx 第 1 至 76 行是 `cx0_*` 的直接读写与快照(含平台 `#if` 与 `__syscall6`);产品源第 28 至 63 行是 `cxm_bind` 五回调加 `cxm_bind_real`(`cx/compiler/cxmod.cx:46`、`:58`)。
  - (c) 临时目录与哈希来源:plan.cx 用 `cx0_directory`、`cx_sha256_hex`;产品源用 `cxm_tmpdir`(`:44`、`:958`、`:976`、`:1001`)与 `cxm_digest`。`tools/cx0-extract.cx:54-60` 已有 7 条 `replace_one` 唯一锚替换做同类事。
  - (d) 模块身份判同:`cxm_samefile` 权威为文本比较,C 投影经锚替换成 `CXM_HOST_REALPATH`,产品源走 `cxm_reali` 回调(`cx/compiler/cxmod.cx:347` 起)。
  - (e) 只在产品源的语义:`cxm_checkcalls` 及其状态 `cxm_locn`、`cxm_badcalls`、`cxm_funit`(`cx/compiler/cxmod.cx:1273`、调用点 `:1351`,连续 138 行块);拒绝文案 `call to undeclared function` 在三源的计数为 0、0、1(`grep -c`)。
  - (f) 上限:`CXM_MAXU` 产品为 enum 128 且带 `DV_MAXU`(`cx/compiler/cxmod.cx:28`),权威与投影为 `#define 96`(`cx/cx0/plan.cx:78`、`src/cxmod.c:27`);文案也分叉:产品用 `cxm_md(CXM_MAXU)` 拼数字(`:508`、`:1301`),权威写死字面量 96(`cx/cx0/plan.cx:520`、`:1178`)。`exec/facts/limits.tsv:2` 只登记 `UNIT_MAX 128` 指向产品源。
  - 尾部:plan.cx 在 `CX0_COMPAT_BODY_END`(`:1315`)之后是 `cx0_plan`、`cx0_capture_plan` 适配 53 行,产品源对应 9 行。
- 其中 (a)(b)(c)(d) 与尾部是宿主差异,可以表达为投影;(e)(f) 是语义分叉,不属于任何宿主,必须先消掉。

**(1) 全生成的形状:事实表 -> 生成器 -> 生成物**

- 事实表(新增,待建)`cx/cx0/projection.tsv`,一行一条投影规则,列:`target`(`src/cxmod.c` 或 `cx/compiler/cxmod.cx`)、`step`(序号)、`kind`(`prefix` | `suffix` | `visibility` | `region` | `limit`)、`key`(区名或常量名)、`value`(片段文件路径、`public`/`static`、或取自 `exec/facts/limits.tsv` 的键)。现有 7 条 `replace_one` 的旧新文本对从工具源码搬成片段文件,放 `cx/cx0/proj/`,表里只留路径,避免在 TSV 里塞多行文本。
- 权威源内的区标:把今天靠「整段旧文本唯一匹配」的锚,改成 plan.cx 体内成对注释 `/* CX0_HOST_BEGIN id */` 与 `/* CX0_HOST_END id */`(与现有 `CX0_COMPAT_BODY_BEGIN/END` 同法,`cx/cx0/plan.cx:77`、`:1315`)。区内是权威宿主的写法,投影按表把整区换成目标片段。区 id 初定五个:`rd-wr-say`、`tmpdir`、`digest`、`samefile`、`entry`。
- 宿主适配片段(手写,但只含接线,不含文法与拒绝):`cx/cx0/compat-prefix.c.in`(已有)、`cx/cx0/proj/cxmod-prefix.cx.in`(产品头:module 行、`cxm_bind`、`cxm_bind_real`、四个转发函数)、`cx/cx0/proj/cxmod-suffix.cx.in`(产品尾 9 行)。
- 生成器:扩 `tools/cx0-extract.cx`(名字已有历史,保留原合同,不另起名;`git log --all --oneline -- tools/cx0-extract.cx` 先核)。新增 `--target PATH`,缺省仍只处理 `src/cxmod.c`,新增 `--all` 处理表中全部目标。
- 生成器写域:只写 `src/cxmod.c` 与 `cx/compiler/cxmod.cx` 两个文件(经现有 `publish` 的临时文件加 rename,`tools/cx0-extract.cx:42-47`);不写 plan.cx、不写事实表、不写片段。
- `--check` 合同:(1) 永不写盘;(2) 每个目标输出一行 `CX0_EXTRACT<TAB>EXACT<TAB>bytes=N<TAB>sha256=H<TAB>output=PATH<TAB>authority=cx/cx0/plan.cx`,行首 17 字节保持不变,因为 `tools/check-cxmod-tri.cx:89` 按 `CX0_EXTRACT\tEXACT` 匹配;(3) 任一目标不逐字相等即 rc=2 并报 `generated-classic-projection-differs` 加目标路径;(4) 区标缺失、重复、嵌套、表中区 id 在源里找不到、源里有表中没有的区,各自具名 rc=2(沿用 `bad()` 的文案风格);(5) 不带 `--all` 时行为与今天逐字节相同,保证 `tools/ratchets.cx` 与现有批次不用同步改。
- 生成物头部第一行写 `GENERATED by tools/cx0-extract.cx from cx/cx0/plan.cx; do not edit`,并把 `cx/compiler/cxmod.cx` 加进禁止手改的检查(与 `src/cxmod.c` 同口径,prd_cx.md 第 1 节表「禁止手改」)。

**(2) 若全生成受阻时的最小切分(备选,不是首选)**

- 受阻点只有一个可预见:`cxm_checkcalls` 上移到权威后 `src/cxmod.c` 增约 138 行,而 srcshape 棘轮不许 `src/` 增行(10.3 棘轮条)。`src/cxmod.c` 不计入 srcshape 口径(T4 实测:`tests/srcshape.cx` 第 101 行只取 `git ls-files -s -z -- 'src/*.cx'`;基线 `tests/srcshape.tsv`;`src/cxmod.c` 现 1271 行,不在其中)。故无需豁免;若仍需对 `src/cxmod.c` 减行,则 `checkcalls` 区对 `src/cxmod.c` 目标按表 `drop`,对产品目标 `keep`;这仍是一份源,不是手写第二份。
- 必须共享(决定接受或拒绝、决定计划字节或 CXIF 字节的函数,任何一个分叉都会让三宿主结果不同):`cxm_directive`、`cxm_adduse`、`cxm_scan`、`cxm_exportok`、`cxm_typenames`、`cxm_project`、`cxm_text`、`cxm_emit`、`cxm_collisions`、`cxm_usechk`、`cxm_dfs`、`cxm_deps_same`、`cxm_rootfind`、`cxm_rewrite`、`cxm_rewrite_multi`、`cxm_plan`、`cxm_planprint`、`cxm_interface`、`cxm_checkcalls`,以及全部 `CXM_*` 上限与全部拒绝文案。函数名入口:`grep -n '^int cxm_\|^static int cxm_' cx/compiler/cxmod.cx`。
- 允许留在差异层的只有:`cxm_rd`、`cxm_wr`、`cxm_say`、`cxm_digest` 的实现来源,临时目录从哪里来,`cxm_samefile` 的规范路径来源,对外入口(`cxm_bind` 对 `cx0_plan`),可见性。差异层不得出现 `cxm_m(` 调用(即不得发出任何拒绝文案),这条可做成 `--check` 的一条断言。

**两条 GAP 的处置(归一的前置,产品输入)**

- `cxm_checkcalls`:上移进 plan.cx 体内,不设区标(全宿主同语义)。效果是 cx0 与 gcc_cx/clang_cx 宿主从接受变为拒绝未导入符号的调用,行为收紧;`tools/check-cxmod-tri.cx:121` 的 gap 分支届时会按设计报 `gap cxm_checkcalls closed, revisit`(`:76`),须同提交把该用例改成普通断言。
- `CXM_MAXU`:统一为 128。依据:产品是已发布行为,`exec/facts/limits.tsv:2` 已登记 128;降到 96 等于对用户缩限。代价:宿主静态表变大(`cxm_reach[MAXU][MAXU]`、`cxm_renpool[MAXU*CXM_MAXDECL][CXM_PATHW]`,`cx/cx0/plan.cx:104`、`:112`),增量未测,标 UNMEASURED。文案统一用 `cxm_md(CXM_MAXU)` 拼数字,不再写死。数值改由 `limits.tsv` 经表中 `limit` 行注入,关闭 A7 第 14 项与 E13。

**(3) 与 0.0.64 export、通配的先后:先归一(D0),再加语法(D1 起)**

- 依据一(仓内事实):手工对齐已漏过两次。`cxm_deps_same` 晚一轮(4367e1b0e5);57c7427f6f 的提交说明写「plan.cx 与 cxmod.cx 同构」,但同一提交加的 `call to undeclared function` 只进了产品源(计数 0、0、1)。P1 加 4 条拒绝、P2 加 5 条、P3 再加若干,每条三处同现,按已有漏率至少再漏一次。
- 依据二(成本):D0 是一次性机械变换,验收是逐字节(生成物等于现手写件,或差异逐行归到上面六类);D1 至 D3 是语义新增,验收是行为。先做 D0,D1 至 D3 每条只改 plan.cx 一处,`--check --all` 一条命令守两份生成物;反过来则 D0 要在更大的差异面上做,且 D1 至 D3 的手工对齐劳动全部作废。
- 依据三(候选规则):D0 与 D1 至 D3 同属 cxmod 三源一类(10.3 一版一类),可同候选,不增加整链次数(F9)。
- 类比,均为 [忆] 未核:
  - Python [忆]:导入机制只有一份实现 `importlib._bootstrap`,解释器内置的那份是由它冻结生成的产物,不是另写的 C 版本;早年 C 版 import 与 Python 版并存时行为不一致,后来收成单源再冻结。对应本方案:plan.cx 单源,产品件与宿主件都是冻结投影。
  - Node.js [忆]:`package.json` 的 `exports` 解析在 CJS 加载器与 ESM 加载器之间共用同一套解析函数,新增条件导出时两种加载器同时生效;没有为两套加载器各写一份 exports 解析。对应:先有共用解析核,再加 export 语法。
  - Rust [忆]:名字解析的权威实现在 rustc 的解析阶段;rust-analyzer 另有一份独立实现,新语法(如新版次的导入规则、宏相关可见性)在两者之间出现过滞后与结果不一致。对应:第二份手写实现在加语法期间必然滞后,即 F3。
- 反方理由与回应:归一动产品输入,会不会拖住 0.0.64?回应:D0 的今天可做部分(T1 至 T4)不动产品输入;动产品输入的部分(T5 至 T8)各自 15 分钟内,且排在 D1 之前同一候选。

**(4) 任务拆分(每项 15 分钟内,写域互不相交;简报按五段式另写)**

| 号 | 内容 | 写域 | 产品输入 | 今天可开工 | 验收命令与预期值 |
|---|---|---|---|---|---|
| T1 | 三源差异表:按函数对齐 plan.cx 与 cxmod.cx,输出函数、差异类(a 至 f)、是否语义(即 B1 的 54-1 `cx0-diff`) | `tools/cx0-diff.cx`、`tools/check-cx0-diff.cx`、`build/cx0-three-source-diff.tsv`、批次行 | 否 | 是 | `sh ./unisacc.com tools/check-cx0-diff.cx` rc=0;表中「是否语义=是」的行恰为 (e)(f) 两类,其余为 0;68 块全部归类,未归类 0 |
| T1 实测更正 | tools/cx0-diff.cx 已落(62901f0fc7,check 27 条 failed=0):plan.cx 对产品 68 块与上文相符;但 4 块落在六类之外(类 x),含三处 10.5 未列的语义分叉:cxm_load 的 `if (n == -3) return 1;` 只在 plan.cx;cxm_text 的 CXIF 防重复宏名后缀 `_sym` 只在 plan.cx;`cxm_rootn = n;` 只在产品。T5/T6 上移范围须含这三处;真实三源 `--check` 因此 rc=2 属有意 | - | - | - | - |
| T2 | MAXU GAP 实跑:生成 97 与 129 个单元夹具,对 cx0 与产品各跑一次,把 `:122` 的静态 GAP 改成实测 GAP | `tools/check-cxmod-tri.cx` | 否 | 是 | 末行仍 `failed=0 gaps=2`;GAP 行带实测 rc(预期 97 单元:cx0 rc=1 文案 `more than 96 units`,产品 rc=0;129 单元:两者均 rc=1) |
| T3 | 第二投影原型:用现有 `--output` 把产品目标生成到仓外,量与手写件的剩余差异 | 仅 `$CLAUDE_JOB_DIR/tmp`(不入库);结论原位更新本节 | 否 | 是 | `sh ./unisacc.com tools/cx0-extract.cx -- --output /tmp/cxmod-proto.c` 输出 `PREPARED`;`git diff --no-index --numstat /tmp/cxmod-proto.c cx/compiler/cxmod.cx` 与本节 +253 -79 相符,并给出去掉 (b)(e)(f) 后的剩余行数 |
| T4 | 棘轮核实:`src/cxmod.c` 增行是否触发 srcshape;`cx/compiler/cxmod.cx` 成为生成物是否触发 cxapis、cxlibc 等 | 只读;结论原位更新本节 UNVERIFIED 项 | 否 | 是 | `sh ./unisacc.com tools/ratchets.cx` 末行 13 项现值入账;给出 srcshape 对 `src/cxmod.c` 的判定源码行 |
| T5 | 上移 `cxm_checkcalls` 及其状态到 plan.cx,重生成 `src/cxmod.c`,tri 用例改普通断言 | `cx/cx0/plan.cx`、`src/cxmod.c`(生成)、`tools/check-cxmod-tri.cx` 第 121 行 | 是 | 否(等 0.0.62 新产品已装;T2 先完成,避免与其同文件) | `cx0-extract -- --check` 为 `EXACT`;`check-cxmod-tri` 末行 `passed=11 failed=0 gaps=1` |
| T6 | MAXU 统一 128 并由 `limits.tsv` 注入,文案改拼数字 | `cx/cx0/plan.cx`(与 T5 串行)、`exec/facts/limits.tsv`、`src/cxmod.c`(生成) | 是 | 否(在 T5 之后) | `check-cxmod-tri` 末行 `passed=12 failed=0 gaps=0`(T2 的两个夹具转为普通断言);`grep -c 'CXM_MAXU' exec/facts/limits.tsv` 覆盖三个消费者 |
| T7 | 区标与投影表:plan.cx 加五对 `CX0_HOST` 区标,建 `cx/cx0/projection.tsv` 与 `cx/cx0/proj/` 片段,`cx0-extract` 改读表,`src/cxmod.c` 逐字节不变 | `cx/cx0/plan.cx`(与 T6 串行)、`cx/cx0/projection.tsv`、`cx/cx0/proj/`、`tools/cx0-extract.cx` | 是(区标是注释,投影后须从 C 目标剥掉,保证字节不变) | 否(在 T6 之后) | `cx0-extract -- --check` 的 sha256 与 T6 结束时相同;工具内不再有 `replace_one(body,"` 字面量(`grep -c` 为 0) |
| T8 | 产品目标切换:`--all` 生成 `cx/compiler/cxmod.cx`,删手写件的手工对齐义务,登记批次 | `cx/compiler/cxmod.cx`(生成)、`tools/check-cx0-extract-all.cx`、批次行、`tests/module/units.cx` 只增断言 | 是 | 否(在 T7 之后) | `sh ./unisacc.com tools/cx0-extract.cx -- --check --all` 两行均 `EXACT`、rc=0;`sh ./unisacc.com tools/affected.cx -- --files cx/compiler/cxmod.cx --run` 全绿;`sh ./unisacc.com tests/module/check.cx` 断言数不减 |

- 并发关系:T1、T2、T3、T4 写域互不相交,可同时派;T5 至 T8 都写 plan.cx,串行。T5 至 T8 合计四个 15 分钟片,排在 D1 之前。
- 遍历消费者(限时与字面量改读表的铁律):T6 的消费者清单是 `cx/cx0/plan.cx`、`src/cxmod.c`、`cx/compiler/cxmod.cx`、`DV_MAXU` 的使用方(`grep -rn DV_MAXU src cx`)、`exec/facts/limits.tsv:2`、`tools/check-cxmod-tri.cx:122`;T8 的消费者是 15 个 `use compiler.cxmod` 的文件(`grep -rln 'compiler.cxmod' src cx tools tests`,其中 `src/` 6 个)。
- 未验证与风险:(1) 生成的产品件能否与现手写件逐字节相等:UNMEASURED,T3 给数;不等时以「差异逐行归入六类且 `tests/module/units.cx` 全绿」为准,不强求字节相等。(2) srcshape 对 `src/cxmod.c`:已核实不计(T4,见上;cxapis、cxlibc 对生成物 `cx/compiler/cxmod.cx` 的影响仍 UNVERIFIED,未跑 `tools/ratchets.cx`)。(3) MAXU 升到 128 后宿主静态内存增量:UNMEASURED。(4) `cxm_checkcalls` 上移后宿主侧语料新拒绝:T4 实测对 `tests/cxcc/units.tsv` 全部 10 个源与 `tests/cx-host-run.cx` 共 11 个文件逐个跑产品 `cxm_plan`(`sh ./unisacc.com tools/check-cxmod-tri.cx -- --prod FILE`):因 "call to undeclared function" 被拒 0 个;被拒 3 个均为表内 refuse 正例(cxcc_missing、cxcycle/ping、cxstd_unknown,原因分别为缺模块、环、未知 std 模块),其余 8 个 rc=0。产品源本就含 `cxm_checkcalls`,上移不改变这批结论。未覆盖:`tests/cx-host-run.cx` 内联字符串夹具(运行时才生成)与其它批次语料:UNVERIFIED。T5 验收仍须加跑批次 `cx-host-run`。(5) 本节全部为设计,T1 至 T8 均未开工。

## 11. 参考系对照与取舍(2026-11-14,项目所有者补充要求)

资料:`docs/cx/ref-node.md`、`ref-rust.md`、`ref-cpython.md`、`ref-quickjs.md` 已读;这四篇侧重标准库面与稳定性策略,模块系统与内存安全的机制部分(下文带 [忆] 的)出自通用知识,本轮未联网核对一手文档,落合同前须再查。仓内没有 Bun 的参考篇(`git grep -i bun -- docs/cx` 只命中 QuickJS 篇的无关行)。不取的理由统一按四条约束写:C99 兼容(宿主 gcc/clang 须能忽略全部扩展)、单文件 APE(无运行期模块装载器、无文件系统上的包目录)、三源构造(规划器一处改三处核)、no-libc(上层不直调 libc,前导自带)。

### 11.1 Node.js(ESM 与 package.json exports)[忆]

- 解决的问题:包的内部文件被外部深层导入,重构即破坏;同一包要给不同环境不同入口。
- 机制:`exports` 字段把包的可导入路径封死成一张表,表外路径导入即报错;条件导出(`import`/`require`/`node`/`browser`)按环境选入口;`imports` 字段给包内私有别名(`#name`);ESM 静态解析、环允许但绑定延迟;`node:` 前缀隔离核心名。
- cx 差距:公开集只在 2 个产品包有表(E2);包内任一模块都能被包外 use(没有「表外路径即拒绝」)。
- 取:(1) 导出面封死成表 -> P7(cxpack 扩到全部 cx.* 包,表外 use 具名拒绝);(2) 核心名带命名空间前缀,`ref-node.md` 第 6 节(9) 已采纳;(3) 错误码是合同、消息不是,同篇(4) 已采纳,P1/P2 的拒绝合同按「文案关键词」而非全文断言。
- 改造:条件导出对应六目标,但不做成导入期选择。cx 的做法是符号不消失、目标差异收在 L0 的 `_os` 表里并返回 `-ENOSYS`(`build/std-surface.tsv` 的 lnx/osx/win 列);条件导出会让同一 use 在不同目标解析到不同文件,计划字节随目标变,破坏「同一 .cx 三宿主同计划字节」。
- 不取:循环导入的延迟绑定(cx 整程序静态合成,环一律拒绝,CX-M-3);运行期动态 `import()`(单文件 APE 无装载器);CommonJS/ESM 双形态。

### 11.2 Bun [忆]

- 解决的问题:解析与启动慢;单仓多包的依赖去重。
- 机制:解析结果与转译产物缓存、一个二进制内置运行时与打包器、workspace 把仓内包按名字互相链接、`bun build --compile` 出单文件可执行。
- cx 差距:冷启动每单元重付前导与模型解码(E8);单元产物无跨程序复用(A8 读数 4)。
- 取:(1) 按内容寻址缓存每单元前端结果(55-7 b 已立项),键含源、强制头、模型、目标;(2) workspace 的「按名字解析、不靠相对路径与软链」即 CX-U-7 包根,已落地,继续淘汰 7 个目录软链(E1)。
- 不取:自动安装与 lockfile(cx 无外部包源,包版本在 `build/tlib-packages.tsv`);宽松解析(自动补扩展名、目录 index):与「解析唯一、拒绝具名」相反。

### 11.3 Python(import 系统)[忆]

- 解决的问题:包作为命名空间、公开面声明、初始化顺序。
- 机制:`__init__.py` 作为包入口并可重导出子模块;`__all__` 限定 `from m import *` 的面;`sys.modules` 以名字为键缓存模块对象(同一文件经两个名字导入会得到两份状态);循环导入得到半初始化模块;下划线前缀是约定而非强制。
- cx 差距:没有包入口文件(E3);通配无定义(E4)。
- 取:(1) 包入口 = 只含 export 行的门面模块(P1),对应 `__init__` 的重导出用途,但没有初始化代码;(2) 通配必须受显式面约束:cx 的 P2 只展开带 `module` 行的直接子模块且冲突仍拒,等价于强制的 `__all__`。
- 引以为戒:按名字而非规范路径判同导致双份模块状态,正是 E1 的缺陷类;cx 取规范路径判同(e576000457)。
- 不取:导入期执行代码与半初始化模块(cx 模块无顶层执行,环直接拒绝);约定式私有(cx 用 static 与 cxpack 强制);运行期可变的搜索路径(`sys.path`):计划必须可由输入字节复算。

### 11.4 Rust(模块树、可见性、Cargo features)[忆,所有权部分另见 `ref-rust.md` 3.2]

- 解决的问题:可见性分层、重导出稳定门面、按特性裁剪编译面、增量编译。
- 机制:`mod` 树与 `pub`/`pub(crate)`/`pub(super)`;`pub use` 重导出;crate 是编译与版本单元,crate 间无环;features 是加法式条件编译;增量编译按查询结果的指纹判断是否重算。
- cx 差距:可见性只有两级(E11);无重导出(E3);哈希只到包(E5)。
- 取:(1) `pub use` -> P1,语义按 Rust:重导出不复制定义、身份属原主;(2) `pub(crate)` -> P7,以 cxpack 表表达「包内可见」而不加语法;(3) 指纹驱动增量 -> P5,符号级 `sig_sha` 已在表里;(4) 包间无环已有(CX-M-3、层向规则)。
- 改造:features 不做成源码内条件编译(上层禁平台 `#if`);cx 的等价物是「不稳定项默认不可用,显式开门」(`ref-rust.md` 第 6 节(2)),落在表与棘轮。
- 不取:glob 导入的遮蔽规则(显式名遮蔽 glob 名):cx 冲突一律拒绝;泛型与 trait 带来的单态化边界;edition(`ref-rust.md` 第 6 节(11))。

### 11.5 QuickJS

- 取其结论(`ref-quickjs.md` 第 6 节):系统面可以小,但 cx 没有宿主语言内建,text/buffer/stream 必须自己成核心层;模块以 C 侧注册表静态登记的做法与 cx 的整程序静态合成同向。不取其「单一 FILE 抽象」。

### 11.6 主线 B:内存安全的诚实清单与增量

cx 现有(均为运行期或约定,不是编译期证明):模块私有状态(static 不出接口,规划期强制);`mg_*`/`ms_*` 所有权合同(CX-O-1..5,调用方合同,不强制);`cm_*` 句柄世代、共享/独占借用、越界拒绝(CX-O-6,`cx/check_memory.cx` 138 条);cxraw、cxtrunc、cxlibc 棘轮(扫描期);`mm_*` 分配器与 arena(`cx/mem/arena.cx`);带长度切片 `ms_slice`(`cx/mem/slice.cx:26`)。

对照:Rust 在编译期证明无释放后使用、无数据竞争、无悬垂借用,运行期没有句柄检查层,`unsafe` 把未证明的部分圈成可审计区块;Python 与 Node 靠引用计数或 GC 保证对象存活,靠运行期边界检查拒绝越界,代价是运行时与停顿,且不防逻辑上的资源泄漏(文件未关)[忆]。cx 没有 GC、没有借用检查器,也不打算把 C99 改成别的语言。

cx 现在能在编译(规划、扫描)期保证的:(1) 模块私有符号不被包外引用(static 与 cxpack);(2) 未声明的跨模块调用被拒(`cxm_checkcalls`,限本程序内定义的函数;该函数只在产品源 `cx/compiler/cxmod.cx:1247`,权威 `cx/cx0/plan.cx` 中 `git grep` 无此名,属三源差异,宿主侧靠编译器 -Werror 兜住);(3) 无 dock 的单元不出现 `#include` 与裸 libc(棘轮加规划器);(4) 模块图无环、公开名不冲突;(5) 层向不上行。以上都是名字与结构性质,不是内存性质。

只能运行期检查的:(1) 过期句柄使用(世代号,概率性隔离,句柄可伪造可复制);(2) 越界(仅经 `cm_*`、`ms_*`、`mg_*` 接口的访问;裸指针运算不受检);(3) 重复释放(仅 `mg_free` 幂等路径);(4) 借用冲突(仅 `cm_*` 的共享/独占计数)。

不声称的:释放后使用的一般性阻止、数据竞争免疫、裸指针别名安全、编译期所有权与借用(CX-W-4 未落地)、线程安全。任何文档不得写「内存安全」。

可落地的增量(每条:类别、版本、一条命令的最小验证):

| 号 | 增量 | 取自 | 类别与版本 | 最小验证实验 |
|---|---|---|---|---|
| M1 | unsafe 边界显式化与审计表:dock 模块即 cx 的 unsafe 区;建 `build/dock-audit.tsv`(模块、dock 种类、裸调用符号数、审计人、理由),dock 模块数与裸调用数只降不升 | Rust unsafe 区块 | 非产品输入的工具与表;立即 | `sh ./unisacc.com tools/dock-map.cx` 的模块数与表行数相等(现 56) |
| M2 | 拥有者标注 pragma:`#pragma unisa owns sym`(返回值由调用方拥有、须交给指定释放函数),先只进接口表列与文档,由 lint 检查「标注了的返回值在同函数内被释放、转移或返回」这一条流不敏感规则 | Rust 所有权、`ref-rust.md` 第 6 节(9) | 表与 lint 为非产品输入(0.0.62 可做);pragma 进规划器属 cxmod 三源,0.0.64 之后;CXIF v1 字段属 CX-W-8 | 对 `cx/mem` 的 `mg_take` 调用点跑 lint,漏放点数入表 |
| M3 | 句柄代际号推广:`cm_*` 的世代检查模式推广到 `sy_*` 返回的 fd、进程、套接字句柄的库面包装(关闭后使用得到确定的 `-EBADF` 而不是命中被复用的号) | Python/Node 的「已关闭即确定错误」、Rust OwnedFd | 运行库头(新增文件,不改冻结的 cx/sys 既有文件);0.0.62 候选或其后 | 新增 check:关闭后读写 10 类调用全部返回 `-EBADF` |
| M4 | arena 作用域:`mem.arena`(现有 `ar_new`/`ar_alloc`/`ar_drop`)增加成对的 enter/leave 标记检查(leave 时回滚并使其间句柄世代失效),替代逐块释放 | Rust thread::scope 的「括住生命周期」 | 运行库头(cx/mem,非冻结);0.0.62 候选或其后 | `sh ./unisacc.com cx/check_memory.cx` 新增作用域外使用被拒的断言 |
| M5 | 带长度字符串视图作为库面:以 `ms_slice` 为准,`tx_*` 新增接口一律取(指针,长度)或切片,不新增以 NUL 结尾为唯一边界的接口;不加语法 | Rust &str、Node Buffer | 表规则为非产品输入(立即);新接口属运行库头 | `build/std-surface.tsv` 新增行中无长度参数的指针形参数为 0(lint) |
| M6 | 编译期可证明性质的扩面只走规划器的结构性拒绝(如「标注为独占的句柄不得出现在两个模块的静态存储里」),不做流敏感借用检查 | Rust 借用检查的可负担子集 | cxmod 三源;不早于 0.0.65,先出设计 | 设计稿每条性质配一个被拒的最小 `.cx` |

不取:垃圾回收与引用计数运行时(单文件 APE 与 no-libc 下要自带运行时,且改变 C/ASM 内存合同:零写占槽、覆盖前扩容);流敏感借用检查与生命周期标注(需要新语法或属性,宿主 C99 编译器无法忽略后仍同义,且检查器要在三源各实现一遍);把 `cx/owned.cx` 扩成通用所有权系统(CX-O-7 冻结)。
