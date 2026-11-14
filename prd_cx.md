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
| 首次与重复之差分相(A12,0.0.63 零解码前置) | load1 约 3.8-4.2,Linux x86_64,已装产品;首次 e2e n=3 中位 747.7 ms(run-startup-bench),UNISACC_RR_TIME 另 3 次新源首次 e2e 中位 671 ms;重复 n=3 中位 28.9 ms。首次减重复约 642 ms 的分相(RR_TIME 3 次中位):六流模型推理 530.8 ms(82.7%,e2 146 + e3 143 + prune 102 + lower 91 + elf 47 + e1 8);六流模型 inflate 解码 49.3 ms(7.7%);构造 3.7 ms(0.6%);其余启动/装载/输出/子进程约 58 ms(9%,残差;N1 2026-11-14 用 strace 细分,结果:未闭合,仍 UNMEASURED,见本行后附注)。N1 附注(load1 5.3 至 6.1,n=3 中位,命令 `strace -f -ttt -T -o F.txt sh ./unisacc.com f.c`,每次源码末尾注释不同,另跑 `strace -c -f`):strace 下总墙钟 954 ms(无 strace 同口径 151 ms,故 strace 膨胀约 6.3 倍,且本次首次 e2e 未复现 671 ms,推理仅约 12 ms,因 E3 缓存已热);strace 下四项(毫秒):启动与 exec(sh 前奏含 uname、chmod、探测子进程)13.5;读文件(open/lstat/fstat/close 234.0 加 read 64.9 共 298.9,含对 /tmp/unisacc-e3cache-1001 的约 3881 次 open/lstat/fstat/read/close,每类约 3.9 千次,是新发现的主体);子进程 clone/wait/execve 1.6;输出 write 9.5;合计约 323,与 58 ms 相差远超 20%,不闭合。缺:(1) 无 strace 的分相(speed-probe 无首次运行分相,见 build/pkg-gaps.tsv A12);(2) 复现 671 ms 的冷 E3 缓存状态;(3) 3881 次逐文件探测是否为每次首次运行固有,待查是 e3cache 目录扫描还是逐条目验证。E3TRIM 附注(2026-11-14,M5 残差定因,load1 4.6 至 5.1,xfs,n=3 中位):第 (3) 项已查清,是 `cx/runtime/modelcache.cx:239` mc_trim 的预算修剪,每次成功发布(:301)全量扫一遍,每条目 lstat/open/fstat/read/close 各一次;新源首次运行发布 2 次(strace 计数:4017 项目录下 lstat 8061、read 8081、fstat 8041、rename 2),故 3881 约等于 2 乘当时约 1940 项。耗时随条目数(`UNISACC_E3CACHE` 指向假 uc- 项目录):0 项首次 515.6、重复 8.1;2000 项 573.9、8.2;4000 项 649.9、8.2;16000 项 962.9、8.8(毫秒);斜率约 28 微秒每条目,现目录 1938 项折合约 54 ms,与残差约 58 ms 相当,即 M5 残差主体是这次修剪扫描(推断,未在真实目录做有无扫描的对照,M5 四项分摊仍 UNMEASURED)。判定为可优化点,修法与类别见 prd_mempalace.md 的 E3TRIM 行。重复路径只剩 lookup 约 2.9 ms + exec-prepare 约 0.07 ms | 结论:差的主体是推理而非解码;零解码至多消灭约 49 ms(7.7%),单做达不到 500 ms 门(671 -> 约 620 ms),C3 验收需同时消灭推理(编译结果按内容命中)或改口径 | 是(产品输入) | A12 已测,反馈 C3 口径;0.0.63 方案已按此重排,见下五行 L1 至 L5 |
| L1 推理随程序规模:运行库闭包复用(0.0.63 主杠杆) | 2026-11-14 单次实测(n=1,load1 约 3.0,非安静机):新源 `int main(void){return 7;}` 的 .cx 首次,六流推理合计 152.2 ms(e2 89.7、e3 28.2、lower 13.0、prune 10.6、elf 6.8、e1 4.0),对照 A12 基准程序(一行 printf)530.8 ms;差约 379 ms,集中在 e3(143 对 28)、prune(102 对 11)、lower(91 对 13)、elf(47 对 7)。推断:差来自 printf 拉入的运行库闭包每次从头走后四流。M1 取数(n=3 中位,ms,单位均取自 RR_TIME inference/model 行;load1 10.1 至 12.0;每次换源注释避免整程序命中;列 e2/e1/e3/prune/lower/elf/推理合计/model 相):(a) 空 main 105/4/29/10/12/7/168/54;(b) return 7 93/4/29/10/12/7/157/51;(c) 一行 printf 158/9/145/105/94/51/567/52;(d) printf+malloc+strcmp 322/12/299/189/170/93/1074/79;(e) 约 90 行(45 个小函数加 d 的调用)679/15/439/226/254/117/1816/137。每组 3 次原始值抖动大(d 的 e2 317 至 412、e 的 elf 74 至 261),机器负载高。结论:部分成立。后四流从 (b) 到 (c) 只因引入 printf 就由 58 增到 395 ms,且 (c) 到 (d) 再加调用库函数继续增(395 到 751),说明后四流主要随运行库闭包大小增长;但 (d) 到 (e) 同闭包下仍再增(751 到 1036,负载也更高),说明用户代码也有一份,e2 随行数增长最明显(322 到 679);故闭包复用只能吃掉库部分,上界按闭包大小而非固定 379 ms,且闭包随调用的库函数集合变,键须含闭包单元集合 | 把运行库闭包的后段结果按内容身份缓存或随产品预制;键 = 产品身份(rc_hex_ident) + 闭包单元内容 sha + 目标 + 级别,不含路径;现成样板是 `cx/runtime/unitcache.cx`(键 UNIUC3,只盖 e2 与 e1)。上界约 379 ms(671 -> 约 292 ms,过 500 门),前提是推断成立;成因 UNMEASURED,取数见 prd_mempalace 的 M1。风险:缓存正确性(闭包随用户宏与 `\0cli` 状态变化则不可跨程序复用;命中须与未命中逐字节相同) | 是(驱动与运行缓存:`src/driveroute.cx`、`cx/runtime/unitcache.cx`) | 设计;M1 部分成立、M3 已定因(只读读码加实跑):单文件默认 -run 不经 `dv_units`,故无 unitcache 行;`src/driveunits.cx:99` 的 `uc_span` 只在多单元(两个及以上 .cx)路上调用,`uc_open` 在 `src/runcache.cx:628` 启用(门控:非 tape/deps/warnings、目标 lnx/x86_64、`UNISACC_UNIT_CACHE` 不为 0、`ex_flow_pinned()` 为假)。实跑(load1 约 9.1 至 10.3,printf+malloc+strcmp,UNISACC_RR_TIME=1):单文件 unitcache 行数 0;两文件出现 miss/store 各 2 行、二次 hit。六流推理合计 n=3(ms):单文件 646/653/742;双文件冷 737/786/765;双文件其中一单元 hit 539/527/593。即命中只省 e2/e1 前端,约 200 至 240 ms,后四流(e3/prune/lower/elf)仍整程序重跑。结论:机制现成但只盖单元前端(UNIUC3,键含单元内容与产品身份)且不在单文件路径;L1 要复用后四流的闭包结果,缺 (1) 单文件路入口(把单文件也切成 用户单元+运行库闭包单元 走 `dv_units`),(2) 后四流结果的存取与闭包单元集合键,(3) 命中与未命中逐字节等价检查;e2 前端复用已有,可作为 L5(a) 的现成样板,可定形 |
| L2 整程序结果按内容命中与缓存根解耦(原 C2) | 已有:`src/runcache.cx` USE33/USE34,重复路径 lookup 约 0.7 至 2.9 ms。现键含 cwd、`--` 之前的 argv、源路径与源字节(`rc_hex_sources`),同内容换路径或换 cwd 即未命中;缓存根为 `$UNISACC_E3CACHE`,否则 `$TMPDIR/unisacc-e3cache-<uid>`(`mc_cache_dir`),即根已可用环境变量解耦,缺的是默认根与路径无关键 | (a) 默认根改为不随 TMPDIR 变的用户缓存目录,`UNISACC_CACHE_DIR` 作 `UNISACC_E3CACHE` 的正名;(b) 键去 cwd 与路径。对 first_e2e(新内容首次)收益为 0,这条只解 F6(并发子代理全冷);同内容换路径时上界为全部约 642 ms。风险:去路径后 `__FILE__`、诊断文本、相对 include 解析与 argv[0] 会随路径变而字节不同,须先证这些不进产物或把它们留在键里;换路径命中率(M4 取数):同内容不同目录与文件名,第二次 lookup_hit=0(现状未命中,符合预期);换 cwd 相对路径同样 0;`UNISACC_E3CACHE` 同目录一组未取;**N3 实测(0.0.63,夹具在 $CLAUDE_JOB_DIR/tmp,`UNISACC_RR_TIME=1 sh ./unisacc.com X.cx`)**:键全部输入(`src/runcache.cx`):(1)产品身份 rc_hex_ident,影响编译;(2)target(仅 lnx/x86_64)、level、mode(310-328 行),影响编译;(3)cwd(`rc_hex_sources` 252 行 sy_cwd),不影响编译本身,但相对 include 与运行期相对路径依赖它;(4)`--` 之前的 argv[1..](260-264 行),影响 `__FILE__`/argv[0];(5)源路径串(`sources[j]`),同(4);(6)源字节。E1 `__FILE__|argv[0]|__LINE__` 在两目录均输出 `f.cx|f.cx|1`,绝对路径调用输出绝对路径,`./g.cx` 输出 `./g.cx`:二者等于命令行写的路径串,不是 cwd 绝对路径;E2 改名 renamed.cx 输出 `renamed.cx`;E3 d1 跑过后 d2 同内容同名首次 lookup_hit=0;E4 未定义函数诊断两目录逐字节同为 `e.cx:1:25: error: ...`(只含调用串);E5 相对 `#include "inc.h"` 有头两处输出 7,删头后报 `no such file for #include`(更正:头字节虽不在上面列的键输入里,但整程序缓存另有依赖记录,调度者实测同一目录改头 `#define V 7` -> 9 后 rc 7 -> 9,命中走 lookup_hit=2 即依赖校验后重编,不是陈旧命中;所以这不是正确性缺口,只是 K' 设计时要把依赖闭包字节纳入键或沿用依赖校验);E6 裸声明 sy_cwd 报 `no host function`,cwd 相关程序行为靠 cwd 入键保证。结论:源路径串与 argv 须保留,除非程序不引用 `__FILE__`/argv[0];cwd 仅在无相对 include/use 且运行结果不依赖 cwd 时可去;只缓存成功产物时诊断路径不进缓存。合同草案(路径无关键 K'):K'=H(product,target,level,mode,源字节,include/use 闭包字节),仅当(i)源与闭包不含 `__FILE__`、argv[0],(ii)无相对 include/use,才用 K',否则回退现键;检查:每夹具两目录各冷跑、再热跑,断言热 stdout/退出码与冷逐字节同、落盘产物 sha256 同、K' 命中时两目录互命中且 stdout 与各自冷跑逐字节同;含 `__FILE__` 夹具须断言 K' 不适用。未验证:产物 sha 未取(只取 stdout 与 lookup 行);osx/win 不缓存 | 是(`cx/runtime/modelcache.cx` 与 `src/runcache.cx`) | 设计 |
| L3 推理阶段并行 | 依赖图是一条链:`st_span(first,last)` 顺序执行 e2 -> e1 -> e3 -> prune(级别大于 0 时为 e4)-> lower -> elf,每流吃上一流的带(`src/driveroute.cx` 407、432 至 445 行;RR_TIME 行序同)。单单元程序流间无可并发段。`build/graph.tsv` 是发布构造图,不含运行期流图,运行期流图事实表不存在 | 可并发的只有两处:(a) 后五流的模型装载与 e2 推理重叠,上界 = 后五流 model 相之和,本次 n=1 为 40.4 ms(e1 8.2 + e3 20.8 + prune 1.2 + lower 7.6 + elf 2.7),A12 口径下不超过 49.3 ms;(b) 多单元程序的单元级前段并行,对单单元基准为 0。风险:产品运行库引入线程或子进程,六目标都要有合同 | 是(`cx/runtime/routestep.cx`) | 收益小于 50 ms,不单独立项;并入 L4 一起评 |
| L4 模型装载零解码(原 C1/C3) | 模型缓存已存在且本次全部 hit(RR_TIME 六行 `modelcache result=hit`),但 model 相仍合计 49.0 ms(n=1;A12 为 49.3 ms)。即这 49 ms 是命中后的读入、sha 校验与建布局,不是 inflate 本身;行里的 `source=inflate` 字样与 hit 并存,口径待核 | mmap 已解码布局,上界 49 ms(7.7%),单做 671 -> 约 620 ms;降为次要项,排在 L1 之后。M2 取数:命中组(六行 hit)model 相合计 50 至 54 ms(空 main/return 7/printf,n=3);`UNISACC_RUN_CACHE=0` 组 106 与 193 ms(n=2,load 高);N2 三分(2026-11-14,load1 6 至 8.4 noisy,n=3 model 相合计 52.8/53.3/48.9 ms 中位 52.8;RR_TIME 只有每流合计,源码无分段,故用 `UNISACC_RR_TIME=1 strace -f -ttt -e trace=openat,read,mmap,munmap,write` n=1 看系统调用间隔,strace 放大数字):读入(开缓存文件并读 71 字节头加整体)每流 0.4 至 0.6 ms,六流合计约 2 至 3 ms;校验(sha 经管道交子进程,write(4)/read(4) 往返,含键哈希 0.4 至 0.9 ms 与内容哈希 1.5 至 3.1 ms)六流合计约 8 至 10 ms;建布局为余量约 38 至 40 ms(e3 流 20.7 ms 最大,e2 8.9、e1 8.0、lower 7.9),读入非瓶颈,主体是读入后的计算与分配而非 IO。三项为间隔估计,不是分段打点,严格分段仍 UNMEASURED(缺口:modelcache.cx 加 read/verify/build 三个 RR_TIME 字段,属产品源码变更) | 是(`cx/runtime/modelcache.cx`、`cx/runtime/routestep.cx`) | 降级,不再作 500 门的载体 |
| L5 更小的流、剪枝与小程序直通 | 最小程序推理下限 152 ms(n=1),其中 e2 89.7 ms 占 59%,是与程序规模无关的固定部分的主体;e2 固定成本的构成 UNMEASURED(推断为 cx-flat 前导约 409 KB 每次过 e2,见本表第二行) | (a) 前导裁剪或前导的 e2 结果预制,上界不超过 89.7 ms;(b) 小程序直通(跳过 prune)上界 10.6 ms(最小程序)至 102 ms(基准程序),但改变产物字节,属模型与网类;(c) speed12b 重建 e2 网,收益 UNMEASURED | 是(头、e2 输入或网) | (a) 与 L1 同属缓存类可同候选;(b)(c) 属模型与网类,不与 L1 同候选 |
| CX0 模型化为第 8 流 | 未做 | 见 §6 | 是（大改） | 顺延：55-4 在 0.0.60（P60），55-1 在 0.0.58（P1），55-2 在 0.0.59（P59） |
| 骨架网落行为 | 31 个骨架全为 `return 0` | 见 §6 | 是 | 顺延 0.0.56/0.0.57 |
| 层向检查提速 | `check-cx-layers` 20.9 s | 复用 symindex 索引 | 否 | 未立项 |

### 5.1 L1 设计稿:运行库闭包结果复用(任务 C1,2026-11-14;设计,未实现,不得记 PASS)

标注约定:[读码] 为本次只读读源码得到;[实测] 引自上表 L1 行 M1/M3;[推断] 未证实;[未验证] 附取数命令。本稿没有新跑任何计时。

(1) 现状事实与缺口

- [读码] 单文件 -run 的后段入口是 `src/driveroute.cx:360` 的 `dv_run_lower`:e3 整段一次 `st_span(... "e3" ...)`(:407),随后把 e3 带子整段交给后续流,`cont = level ? "e4" : "prune"`(:432),即 prune 起的后续流是对整条带子的一次 `st_span`(:448),没有按单元的切口。
- [读码] 现有两份跨进程缓存都是整程序键:`rc_lookup_e3`/`rc_store_e3`(`src/runcache.cx:660`、`:668`,kind 0)与 `rc_lookup_elf`/`rc_store_elf`(`:656`、`:664`,kind 1);`dv_run_lower` 只在 `nsrc != 1` 时查 e3 回执(`src/driveroute.cx:402`)。换一个注释即整程序未命中。
- [读码] 单元缓存 `uc_span`(`cx/runtime/unitcache.cx:394`)键 = sha256("UNIUC3"、产品身份 rc_hex_ident、单元路由、已发布 `\0cli` 状态的 sha、单元字节 sha),不含单元序号与个数(`cx/runtime/unitcache.cx:28` 至 `:33`);只包 e2/e1。
- [读码] e3 带子层已有一个"分开编译再合并"的先例:`fwm_merge`(`cx/compiler/fwdmerge.cx:233`)把单独编译的转发桩带子按行并入第一条 e3 带子(带子是行文本,有 `.unit 2` 行,:240),形状不能证明安全就退回整程序重放(`cx/compiler/fwdmerge.cx:2` 至 `:5`;`fwd_incremental` 返回 0 即全量重放,`cx/compiler/fwdincrement.cx:3` 至 `:6`)。合并后才进 prune(`src/driveroute.cx:268` 至 `:270`)。
- [已测,见 E1] 运行库闭包随头文件文本进同一 `.unit 0` 单元,按来源标记交错,不是独立 `.unit` 段。原问题:是随头文件文本进同一单元,还是独立 `.unit` 段。这决定闭包单元的切法,是本稿最大的未知。取数:对 M1 的 (b)(c) 两个源各取一次 e3 带子,数 `.unit` 行数、总行数、来自用户源与非用户源的行数(`fwm_location` 的 source 标记,`cx/compiler/fwdmerge.cx:232`);带子落盘入口未核实,先查 `sh ./unisacc.com tools/refactor.cx sites rc_store_kind` 与 `tools/tape-gap.cx` 的读带子方式,见分期 E1。

(2) 粒度与缓存键

- 粒度定为"闭包单元集合"而不是单个库函数:一次运行中除用户单元以外、被拉入后四流的全部运行库内容,作为一个整体缓存一份结果。理由:[实测] 闭包随调用的库函数集合变((c) 395 ms 到 (d) 751 ms),按集合整体做键最简单且不需要证明单元间可拼;代价是不同程序调用不同库函数集合时各占一个条目。
- 键 UNICL1 = sha256 of:(a) 标签 "UNICL1";(b) 产品身份 rc_hex_ident(覆盖模型、打包头、解释器、ABI 表,沿用 `cx/runtime/unitcache.cx:28` 的口径;后四流模型身份是否已在 rc_hex_ident 内 [未验证],取数:读 `src/runcache.cx` 的 `rc_hex_ident` 定义,核它是否列入 e3/prune/lower/elf 四个模型的 sha);(c) 目标与级别(路由串,如 `fwi_route` 拼的 `<target>/tape/O<level>`,`cx/compiler/fwdincrement.cx:17`);(d) 闭包单元集合:每个闭包单元内容 sha 的有序列表(按单元进入顺序,顺序改变即视为不同键,因为顺序会进带子与 ELF 布局);(e) 已发布 `\0cli` 状态 sha(宏定义、include 目录、extras;用户 `-D` 会改库头展开,必须入键);(f) 用户单元对闭包的引用面(见下条,仅方案 B 需要)。
- 键不含路径。与 L2(C2b 路径无关键)的关系:UNICL1 天生路径无关,因此不依赖 C2b 落地;但闭包内容若含 `__FILE__` 或诊断路径,路径会进带子字节,此时必须把该路径串入键或拒绝入缓存。[未验证] 运行库闭包内是否出现 `__FILE__`:取数用 `tools/refactor.cx find` 在打包头与 `cx/` 运行库源里搜 `__FILE__`。
- 条目格式、锁、字节预算、条目自校验 sha 沿用 UNIUC3 的写法(`cx/runtime/unitcache.cx:35` 至 `:40`),放在同一模型缓存目录。

(3) 后四流能否按单元拆分后拼接且逐字节等于整程序结果

- e3:可以,有先例。[读码] `fwm_merge` 已经在产品里做"单独过 e3 的带子并入主带子",并带有不可证明即回退的合同。方案 A 把闭包的 e3 带子缓存下来,用户单元单独过 e3 后合并。[推断] 逐字节等于整程序 e3 带子并不保证:合并涉及局部名与全局名改写(`fwm_merge` 的 locals/globals,:234),行序与编号可能与整程序一次过不同。所以方案 A 的等价合同只能落在最终 ELF 字节与运行输出上,或者要求合并器输出与整程序 e3 逐字节相同并由检查强制。
- prune:需整程序。它是可达性裁剪,保留哪些闭包函数取决于用户单元引用了什么;同一闭包对不同用户程序的裁剪结果不同。[实测,见 E2 定因] 且带子含 `callr` 时整带原样输出。
- lower:[实测 E3,一对样本] 库函数体基本逐函数独立(共有 398 段中 385 段同字节,96.7%);差异在启动收尾桩、用户 main 与生成标号,@sym 地址只在头部。限度:仅 (c)/(d) 一对,源码行号 UNMEASURED(lower 在 exec/facts/lower-*.tsv 生成的网中,未逐行定位)。若 lower 对每个函数的输出只依赖该函数自身的带子段,则可按"裁剪后的闭包函数集合"缓存;若依赖全局编号或字符串池则不可拆。取数见 E3。 对每个函数的输出只依赖该函数自身的带子段,则可按"裁剪后的闭包函数集合"缓存;若依赖全局编号或字符串池则不可拆。取数见 E3。
- elf:需整程序。布局、符号地址、重定位都是全程序量;按单元产出再拼需要一个链接步,而现有流水线没有独立链接器,拼出的字节不会等于整程序 elf 流的输出。
- 结论:只有 e3 有现成的可拆证据;prune 与 elf 按语义需整程序;lower 待 E3。因此"后四流全部按单元拆分再拼接并逐字节等价"不成立,不走这条路。

(4) 采用的方案:两级,先 A 后 B

- 方案 A(e3 闭包带子复用,0.0.63 主体):单文件路把输入切成"用户单元 + 闭包单元集合";闭包的 e3 带子按 UNICL1 键(不含 (f))命中即取,用户单元单独过 e3,再按 `fwm_merge` 同样的保守合同合并,合并不可证明安全则整程序重跑。prune/lower/elf 仍整程序跑。上界:[实测 n=3] e3 从 145 降到约 29 ms,约省 116 ms;按 A12 基准 671 ms 算到约 555 ms,[推断] 单靠方案 A 过不了 500 ms 门。这一点与上表 L1 行"上界约 379 ms"的口径不同:379 ms 是后四流全部复用的上界,方案 A 只能拿到其中 e3 一份。
- 方案 B(闭包后段整体预制,条件成立才做):键加上 (f) 用户单元对闭包的引用面(用户带子里未定义外部名的有序集合;`uu_user_undef` 已在算这类集合,`src/driveroute.cx:459` 至 `:460`)。同一闭包单元集合加同一引用面,prune 对闭包部分的保留集相同,此时可缓存"闭包部分 prune 后再 lower 的结果",只对用户单元跑 prune/lower,最后整程序跑 elf。成立条件是 E2 与 E3 都为真(prune 对闭包的保留集只由引用面决定;lower 逐函数独立)。上界:再省 prune 与 lower 的闭包份,[实测] (b) 到 (c) 两流合计由 22 增到 199 ms,约 177 ms;A 加 B 约 293 ms,671 到约 378 ms。elf 的 44 ms 不动。
- 发布时预制随产品分发:A 与 B 的条目都可以在发布时对常见闭包(空、printf、printf+malloc+strcmp 等,清单进事实表)预先生成并放进产品,首次运行即命中;这只是缓存的预热来源,键与校验不变,不另立格式。预制清单与体积预算 [未定],等 E1 的闭包大小数据。
- 500 ms 门的口径:若 E2 或 E3 否定方案 B,则 L1 单做约 555 ms,需与 L5(a) 前导 e2 预制(上界 89.7 ms)同候选才可能过门;按铁律此处写明,不放宽不删门。

(5) 正确性合同、失效与回滚

- 等价合同:同一产品、同一源、同一参数下,关缓存(`UNISACC_RUN_CACHE=0`)、开缓存首次(未命中并存入)、开缓存再次(命中)三种运行的最终 ELF 字节、标准输出、标准错误、退出码全部相同。方案 A 另加一条:合并后带子进 prune 的输入若与整程序 e3 带子不同字节,必须由检查列出差异并证明 ELF 仍同字节,否则判红。
- 新检查 `tools/check-closure-reuse.cx` 的形状(新名,建前按规则跑 `git log --all --oneline -- tools/check-closure-reuse.cx`):夹具取 M1 的 (a) 至 (e) 五个源加三个反例(用户 `-D` 改库头展开、用户定义与库同名函数、含 `__FILE__` 的源);每个夹具在独立缓存目录下跑三次(关、未命中、命中),比较产物 sha 与输出;断言行 `CLOSURE fixture=... off=<sha> miss=<sha> hit=<sha> same=1`,并断言命中那次的 RR_TIME 确有 closure hit 行(防止检查因为根本没命中而假绿);反例必须未命中或结果仍同字节。登记批次用 `tools/register.cx`,样板是 `tools/check-unit-frontcache.cx`。
- 失效条件(任一成立即未命中):产品身份变;目标或级别变;任一闭包单元内容 sha 变;闭包单元集合或顺序变;`\0cli` 状态 sha 变;方案 B 下引用面变;条目自校验 sha 不符;条目依赖的头文件回执与现读不符(沿用 `uc_note` 回执,`cx/runtime/unitcache.cx:24` 至 `:26`)。不可入缓存的情形:闭包阶段产生了诊断输出且含路径;`ex_flow_pinned()` 为真;tape/deps/warnings 模式(与 `uc_open` 现有门控一致,见上表 M3)。
- 回滚:纯加速器。运行期关闭用 `UNISACC_RUN_CACHE=0`(整体)或沿用 `UNISACC_UNIT_CACHE=0` 的写法新增一个只关闭包复用的开关;代码回滚是撤 C3 的提交,缓存条目带新标签 UNICL1,旧产品不识别即忽略,不需要清缓存。

(6) 分期(每项 15 分钟内,写域互不相交)

今天可做的只读实验(不改产品输入,用已装产品,脚本用 .cx 写在 `$CLAUDE_JOB_DIR/tmp`,结果原位填回本小节):

- E1 [已测 2026-11-14,load1 6.4 至 7.0,noisy] 取带子入口:`sh ./unisacc.com X.cx -S > X.tape`(-S 输出 tape 文本,是 e3 之后的带子,`stream sidecar` 对它报 `forwarding side-car missing`,故它已是 sidecar 后形态);`.unit 0 4:<hex 文件名>` 是来源标记行,不是单元分隔,整程序始终只有 `.unit 0` 一个单元号。脚本 `$CLAUDE_JOB_DIR/tmp/seg2.cx`(按 `.unit` 标记归属累计行与字节)。结果(行/字节):(b) return 7 共 1463/24805,用户 b.cx 44/654,cx-flat.h 1418/24143;(c) 一行 printf 共 11539/194502,用户 c.cx 51/768,errno.h 7/127,stdio.h 10462/176166,sys/_exit.h 690/11681,cx-flat.h 328/5752;(d) printf+malloc+strcmp 共 12323/207702,用户 d.cx 94/1471,stdio.h 10463/176174,sys/_exit.h 690/11681,string.h 236/4007,stdlib.h 748/12800,errno.h 7/127,cx-flat.h 84/1434。结论:闭包不是独立 `.unit` 段,而是同一 `.unit 0` 内按头文件来源标记交错的文本;非用户源占比 (b) 97.3%/(c) 99.6%/(d) 99.3%(按行),(c) 单个 stdio.h 即约 91% 行;切单元须按来源标记切带子行而非按 `.unit` 号,合并器要自行解决函数编号与局部名(`fwm_merge` 的 locals/globals);预制体积:(c) 闭包 e3 带子约 194 KB,(d) 约 206 KB,按文本计,尚未压缩。注:(b) 空程序也有 1418 行 cx-flat.h 前导,即最小闭包不为零。
- E2 [已测 2026-11-14,load1 2.8 至 3.0] 结论:stream prune 对 -S 带子逐字节无变化,且该带子里没有函数级裁剪的迹象,方案 B 的"prune 保留集由引用面决定"前提无从成立或无从证明。证据:(1) 源 `static int unusedfn(...)` 加 `int unusedglob(...)` 各一个无人调用的函数,再接 c.cx(一行 printf),`sh ./unisacc.com u.cx -S > u.tape` 中两函数标号均在(11595 行),`stream prune --target lnx/x86_64 < u.tape | cmp - u.tape` 同字节;(2) `stream lower` 之后标号数不变(u.tape 404 对 u.low 404,c 400 对 400,d 424 对 424),未用函数仍在 lower 输出(grep unused 得 2)。读码:`src/driveroute.cx:268/432` 的 cont 为 `level ? "e4" : "prune"`,即 prune 是 e3 后默认续流,-S 带子已是 e3 后;prune 流(`exec/facts/prune-nametable.tsv` 是 op 名与字符表)在本输入上不删函数。未能区分:-S 带子已经是 prune 之后(则 prune 在默认级别不做死函数消除),还是 stream prune 单独喂 -S 带子即恒等;`stream --help` 无此选项(报 `no input`),`stream e3` 未试。故 prune 后带子取不到不影响结论:可观察的后四流不做可达性裁剪,prune 的 105 ms 花在别处(用途未测),不是"闭包随引用面裁剪"。
- E2 定因 [已测 2026-11-14,load1 2.4 至 3.0,已装产品,lnx/x86_64,材料在 `$CLAUDE_JOB_DIR/tmp` 不入库,未改源码] 结论:prune 在设计上是函数级可达性裁剪,但带保守回退;带子里只要有 `callr`(间接调用)就整带原样输出。凡带 stdio 闭包的程序都含 `exit` 里的 `callr r5`(atexit 回调分发),所以 hello 类程序上 prune 恒等。这是有意的保守合同加未做的优化,不是缺陷。
  - (1) 设计:`prd.md` 4.0"prune 按根 / 导出剪不可达整函数(政策在代码,不读金表)";`exec/prune/libraryroots.md` 写明根为 main/init、取址、数据,加库导出根;`exec/prune/prune-result.tsv` 有 30 行转 FALLBACK,第 393 行 FALLBACK 的动作是把输入整段原样输出(`SPAN2 zero size` 后 ACCEPT)。合同用例 `tools/prune-cases-gen.cx:111` 至 `:116` 的 `indirect-call-preserved`(callr-in-main、callm-in-main、callr-in-reachable-callee)与 `:117` 起的 `host-abi-preserved` 期望标志为 1(全保留),批次 `stream-prune-cases`。未做:没有定位到 callr 转 FALLBACK 的具体网行。
  - (2) -S 位置:-S 是 e3 之后、prune 之前。`stream e2 | stream e1 | stream e3` 的原始输出比 -S 多 16 字节头(`USLATTRT` 加 8 字节),去头后与 -S 只差 4 行 `.unit` 的源名(stdin.c 对 u.c),其余同字节(189,657 对 189,625 B)。直接证据:`chain.c`(`exec/prune/fixtures.json` 01-chain)的 -S 带子 97 行含 `unused:`,`stream prune` 后 81 行不含。`stream sidecar` 对该输出报 `forwarding side-car missing`,不是取 prune 输入的入口;`stream prune` 直接吃 -S 带子或 e3 原始输出都可以。
  - (3) 回退触发的二分:六个小程序里,只含 string.h 或无头文件的被裁(148 行到 132 行,91 到 59),含 stdio.h 或 unistd.h 的全部恒等(1171、11493、11244、1029 行不变)。向 chain.tape 的 main 注入一行:`.sys`、`eq`、`ne`、`sle64` 各自仍裁(98 到 82),`callr r5` 则恒等(98 到 98,`.lea r5, leaf` 加 callr 也恒等)。`fixtures.json` 03-addresses(函数指针)在已装产品上也是 210 行到 210 行,dead 仍在。带 stdio 的带子里 callr 只有一处,在 `exit`。
  - (4) 量级(每流 3 次取最小,`stream STAGE` 进程墙钟,含进程启动与模型装载,空输入启动约 5 ms;load1 2.9 至 3.0):hello(stdio 加一行 printf)对同程序加 200 个未用的小外部函数。prune 189 对 299 ms,lower 198 对 271 ms,elf 120 对 165 ms;带子 11,212 对 17,412 行,lower 输出 8,860 对 13,860 行;ELF 36,894 对 53,278 B(加 16,384 B),两者运行都输出 hi。合计约多 228 ms,约每个死函数 1.1 ms、82 B。
  - (5) 若回退解除能省多少:把 hello 带子里那一行 callr 删掉后再跑 prune(只作上界估计,该带子不可执行),11,211 行到 11,095 行,标号 388 到 383,只裁掉 `ferror` 一个函数;200 个未用函数的带子则裁到 11,126 行(199 个被裁,`unused0` 留下,原因未查)。即 stdio 闭包对 printf 程序几乎全可达,hello 的 36,894 B 与后三流耗时基本不受此影响;受影响的是用户自己的死代码和整模块引入却只用一部分的库。
  - (6) 对方案 B 的修订:上一条"前提无从成立"改为"在含可达 callr 的带子上平凡成立"。此时 prune 输出等于输入,闭包部分的保留集与用户引用面无关,闭包部分 prune 后的带子就是闭包的 e3 带子;不含 callr 的程序(如只用 string.h)会真裁,保留集随引用面变,方案 B 的键仍需带 (f) 或先判 callr。
  - (7) 下一步:列为 0.0.65 之后的优化项"prune 有 callr 时以取址集合为根继续裁剪,不整带回退"。类别是模型与网类(prune 的表与网:`exec/prune/`、`exec/facts/prune-nametable.tsv`、`prune.net`),不得与驱动与运行缓存类同候选。验收须保留 `indirect-call-preserved` 的现有断言或以等价的取址根用例替换并写明依据,不删断言。优先级低于 L1:对 hello 的收益约 1%。另:`prd.md` 4.0 的 prune 描述没写回退,需补一句(本条未改 prd.md,不在写域)。
- (原 E2 记录,保留) `sh ./unisacc.com stream prune --target lnx/x86_64 < c.tape` 返回 rc 0 且输出与输入同字节(cmp 相同,11539 行不变;原因未查清(可能 -S 带子已是 prune 后,也可能 stream prune 对该输入不裁剪;c.tape 有约 400 个标号)),所以不能用它取 prune 后带子;prune 后带子缺单独导出入口(整程序 -run 内不落盘)。要做 E2 须先找出喂 prune 的正确输入形态(`stream sidecar` 的 e3 原始输出需 `stream e3` 产出,未试)或给产品加诊断导出;本轮不硬做。方案 B 判断:未决,缺 prune 后带子。
- E3 [已测 2026-11-14,load1 2.8,脚本 `$CLAUDE_JOB_DIR/tmp/fn.cx` 不入库:按顶格 `X:` 标号切 lower 输出,略去 `@` 头行,逐段比较 (c) 与 (d)] 命令 `sh ./unisacc.com fn.cx -- c.low d.low`,输出:`labelsA=400 labelsB=424 common=398 onlyA=2 rawsame=385 normsame=385 bytes=159393 rawbytes=152103 normbytes=152103`。即共有标号段 398 个中 385 个函数体同字节,占 96.7%(按字节 95.4%);归一化(4 位以上数字换成 N)不增加同字节数,说明差异不是 @sym 数字地址。不同的 13 段:`_start`、`__init`、`__main_ret`、`main`、`atexit`、`write`、`_unisa_environ_init`、`__unisacc_R365`、`__unisacc_L367/370/371/372/374`;后一类是编译器生成的局部标号编号,随程序整体顺序变,另有仅在 (c) 的 2 个标号。含义:库函数体(占约 95% 字节)lower 后与具体程序无关,@sym 全局地址只出现在头部,函数体内不含;差异集中在启动与收尾桩、用户 main 与带全局编号的生成标号。所以"lower 逐函数独立"在库函数上基本成立,先前"至少全局布局依赖整程序"过强,只对头部 @sym 与带生成标号的少数段成立。限度:只比了 (c) 与 (d) 一对,(d) 为 (c) 加 strcmp/malloc;未含更多程序;未验证闭包函数顺序变化时同字节;lower 逐函数结果缓存后拼接还需处理 `@sym`、`@data`、`@relocs` 头与生成标号,未做。(原记录,保留)命令:`sh ./unisacc.com stream lower --target lnx/x86_64 < X.tape > X.low`(rc 0)。(c) 9135 行/166702 B,(d) 9766 行/178720 B;`diff c.low d.low` 有 733 个差异行;`@sym` 全局符号地址在 (c) 与 (d) 间不同(如 `g__unisa_freel` 66392 对另一值,35 个 @sym),即 lower 输出含全局布局地址,随程序整体变化。结论:lower 输出不是只依赖自身带子段的逐函数纯函数,至少全局数据布局依赖整程序;这是 (c)(d) 整体对比,没有按函数切段逐段比,所以不能据此判定函数体本身能否同字节,但已指向方案 B 的『lower 逐函数独立』条件需加布局无关证明才成立。按条件,方案 B 判断:未决偏否定;缺口:按标号切 lower 输出逐函数比较的脚本与 prune 后带子(见 E2)。
- E4 [未测] 本轮 12 分钟内只做完 E1 与 E3 部分,E4 未跑;因 E1 显示闭包是 stdio.h 等头文本而非独立单元,手工拆成两个 .cx 需先决定闭包单元的切法(把 `#include` 行放哪个文件),这个前置未定。
- 以上四项互相只读、可并行;取带子的落盘入口未核实,E1 先解决它并把命令写回这里供 E2/E3 复用。

需新产品的实现项,与任务树对齐:

- C2a(缓存根默认值与 `UNISACC_CACHE_DIR`,写域 `cx/runtime/modelcache.cx`)与 L1 无代码依赖;但 L1 的检查要用独立缓存目录,C2a 先落更好写。
- C2b(路径无关键,写域 `src/runcache.cx`)与 L1 独立:UNICL1 自带路径无关。两者同属驱动与运行缓存类,可同候选。
- C3 拆成三片,写域不相交:C3-1 单文件路切单元并进 `uc_span`(写域 `src/driveunits.cx` 与 `src/drive.cx` 的入口选择;注意任务树原写域只列了 `src/driveroute.cx`、`cx/runtime/unitcache.cx`,此片越出,需主调度改派工行);C3-2 UNICL1 条目存取(写域 `cx/runtime/unitcache.cx`);C3-3 e3 闭包带子合并与回退(写域 `src/driveroute.cx`,合并器若要改则加 `cx/compiler/fwdmerge.cx`)。C3-2 可先行,C3-1 与 C3-3 依赖 E1、E4。
- C3-4 方案 B(仅当 E2、E3 为真):写域与 C3-2、C3-3 重叠,串行在其后。
- C3-5 `tools/check-closure-reuse.cx` 加批次登记:写域 `tools/`、`tests/batches.tsv` 自己的行;可与 C3-1 至 C3-3 并行,先写成红的。
- C3-1 派工行修正(2026-11-14,只读读码):`dv_units` 只在 `nsrc > 1` 分支调用(`src/drive.cx:685,694`),其内 `uc_span` 在 `src/driveunits.cx:99`;单文件路径走 `st_span`(`src/drive.cx:~730 附近` else 分支)。故 C3-1 写域 = `src/driveunits.cx`(新增单文件入口或把 `nsrc>1` 门放宽)加 `src/drive.cx` 的 `:685` 入口选择;这两个文件都不在原任务树写域(`src/driveroute.cx`、`cx/runtime/unitcache.cx`)内,与 N15(同改 `src/drive.cx` 的 `:545` 附近)同文件,须在 N15 之后串行(不同行段,但同文件不并行提交)。

#### N15 实现前读码(2026-11-14,只读;读码加两条实测,未改源码,未验证项标 UNVERIFIED)

(1) mode 5 调用链:`src/drive.cx:496` `cxt_object(...)` 调 `cx/driver/route.cx:319`,`:343` `if (obj) *mode_out = 5`(obj = `-c` 且有目标且源非对象,或 asm `-S`;`:337-338`;`-funit` 无 obj 即拒 `:342`)。`:530` 仅 mode 0 的第二遍桩逻辑;`:538` 默认对象名赋给 `shown`;`:545` 缓存门 `(mode2 == 3 || mode2 == 0) && nsrc >= 1 && !dv_fwd_pass && !as_tape && !cxp_tape_path(src)` 不含 5,故 `rc_open` 从未对 mode 5 调用(`rc_on` 恒 0)。接着 `:568` `ex_ri_arm`、`:598` `st_gap`(`cx/runtime/runexec.cx:761-763`;`:763` `ex_obj_mode = mode == 2` 即 -S 而非 -c,mode 5 名 "object")、`:627` e2 模型、`:730` 因 mode 5 非 3 且 `image_fwd` 通常 0 跳过 lower_n;`:~751` `obj && opts.funit && !asmw && nsrc==1 ? rq_object(...) : st_span(...)` 跑 e2 至 elf 得 `body`;`:810` `dv_finish`,`:377` 条件 `mode2 == 5 && !asmw` 走 `:390` `ex_ri_emit(dest=shown, ...)` 写文件;同一行 `:390` 内已有 `rc_store_elf(body)`(只要 body 是 ELF 且写成功就调),但 `rc_on == 0` 故不存(`src/runcache.cx:664,412`)。
(2) 放置层:不新建机制,复用 `rc_`。查询点 = `src/drive.cx:545` 门加 `|| (mode2 == 5 && !asmw && !opts.funit)`(`asmw` 是 -S 文本不是 ELF,`funit` 走 `rq_object` 管线,字节另算,首切排除),`:561` 命中分支对 mode 5 用 `ex_ri_emit(shown, cached, cachedn, ...)`(不是 `opts.out ? : "a.out"`,mode 5 默认名在 `shown`,`:538`)。写入点 = 现成 `:390` 的 `rc_store_elf`,无需改。`rc_open(..., mode2)` 已把 mode 入键与路由(`src/runcache.cx:307-317` `rc_route` 得 `/image/O5<级别>`,`:319` `rc_key` 含 mode、prod、route、target、level、src),mode 5 与 mode 0 不互撞。`ex_ri_die(route, mode2)`(`runexec.cx:740-751`)对 mode 5 在 strict/capture 下返回 2,对 mode 5 须跳过(strict/capture 下不查缓存即可)。`runexec.cx:1014-1036` `ex_ri_artifact` 在 `ex_obj_mode`(mode==2)才发 `artifact_hit`,对 -c 是死代码;N15 之后 mode 5 要发真实戳需改 `ex_obj_mode = mode == 2 || mode == 5` 并让 hit/miss 由 `rc_` 的 `rc_report`(`runcache.cx:99`)而非 `UNISACC_E3CACHE/art` 文件产生;最小做法:phase-timing-stamps 的 artifact_hit 断言改读 `rc_report` 的 lookup 行(`rc_lookup_kind` 末尾 `:~596 rc_report(elf ? 2 : 1, ...)`),并删 `ex_ri_artifact` 死码。
(3) 键成分与路径实测(命令在 $CLAUDE_JOB_DIR/tmp,单命令约 0.3 s):同一源 `n15.cx` 内容 `int main(void){return 3;}`,`-c -b lnx/x86_64 n15.cx -o a.o` 与 `-o b/c.o`,两对象 sha 同为 `64d85a70...342e` -> 产物字节不含 -o 路径。内容相同、源路径不同:`m/other.cx` sha `3d16b59e...`、`m/n15.cx`(经 `m/` 前缀给出)sha `3569f9b0...`,在 `m/` 内以 `n15.cx` 给出得 `64d85a70...`(与 cwd 下相同)-> 对象字节含命令行给出的源路径字符串(字符串在对象内,具体段 UNVERIFIED,取数:`cmp -l a.o m/y.o | head` 与 `strings -a`)。现 `rc_hex_sources`(`runcache.cx:242-296`)的哈希已含 cwd、全部 argv 到 `--`(含 -c、-b、-o)、每个源的路径串与字节,故源路径入键已满足;-o 路径虽不影响字节,但已由 argv 入键,多做不一致(同源 -o 到不同路径会各存一份,浪费不影响正确):首切接受,后续可由 C2b(路径无关键)去掉。键还含产品身份 `rc_hex_ident`(`:608-650`)、目标(仅 `lnx/x86_64`,`rc_open` 门)、级别、mode。依赖闭包(include/头)字节:`rc_arm`(`:650`)注册 `uc_note_set(rc_on_file)` 记 include 收据,`rc_store_kind` 把收据写入条目、`rc_lookup_kind` 重核(沿用 mode 0 行为);头字节不在键内,但有依赖校验,不是陈旧命中(实测同目录改头文件 `#define V 7` 到 `9` 后 rc 7 -> 9,命中走 lookup_hit=2 依赖校验后重编,见 5 节 L2 行更正)。`-funit`、`-S` 不并入首切。
(4) 命中与未命中逐字节等价:命中的字节就是 `dv_finish` 写盘前 `body` 的存档,`rc_lookup_kind` 校验 `USE34` 魔数、内容摘要与收据后返回,再经同一 `ex_ri_emit` 写出(同一 strict 收尾 `ex_ri_finish` 与同一目标 id),故差异只可能来自键漏项;验收用 `UNISACC_RUN_CACHE=0` 对开缓存逐字节 cmp,再加改源后未命中、改 argv(-O 级别)后未命中、换 cwd 后未命中。失效条件:产品身份变、源字节变、argv 变、cwd 变、include 收据任一变、缓存条目魔数/摘要不符(按未命中处理)、`UNISACC_RUN_CACHE=0`、warnings/deps/tape、目标非 lnx/x86_64。
(5) 子任务拆分(各 15 分钟内,写域互不相交,均属产品输入,补丁先交集成者):
- N15-a 写域 `src/drive.cx`(仅 `:545` 门与 `:561` 命中写出两处):验收 `UNISACC_RR_TIME=1 sh ./unisacc.com -c -b lnx/x86_64 F.cx -o a.o` 第二次 `grep -c '^RR_TIME.inference'` 为 0,`cmp a.o b.o` 无差异,`grep RR_TIME.*lookup` 为 hit;改 F.cx 末尾加注释后第三次 inference 为 7 行。必须装后才算数(产品输入)。
- N15-b 写域 `cx/runtime/runexec.cx`(`:763` 与 `:1014-1036` 死码清理或接真实戳):验收 `sh ./unisacc.com tests/run.cx -- --smoke` 不退步,且 `grep -n ex_ri_artifact cx/runtime/runexec.cx` 与 `ex_obj_mode` 的定义同改一处。与 N15-a 写域不相交,可并行,但同候选。补丁草稿已备(2026-11-14,不入仓不应用):`$CLAUDE_JOB_DIR/tmp/n15.patch.txt`,对 HEAD `git apply --check` 通过;N15-a 门加 `mode2==5 && !asmw && !opts.funit && !ex_flow_pinned()`(strict/capture 下不查缓存,mode 5 不调 ex_ri_die,避免其对 mode 5 返回 2),命中用 `shown` 写出;N15-b 读码结论:`ex_ri_artifact` 对 mode 2 仍活,仅 -c 无戳,改为新增 `ex_ri_stamp5(hit)` 按 rc_ 查找结果发 `artifact_hit=N mode=5`,不动 `ex_obj_mode`。静态验证:已装产品跑 phase-timing-stamps PASS(仅基线,不含补丁);补丁本身 UNVERIFIED-需重建产品。重建后验收序列:(1) `UNISACC_RUN_CACHE=0 sh ./unisacc.com -c -b lnx/x86_64 F.cx -o a.o` 与开缓存 `-o b.o`,`cmp a.o b.o` 无差异;(2) `UNISACC_RR_TIME=1` 第二次 `grep -c '^RR_TIME.inference'` 为 0 且有 `artifact_hit=1 mode=5`;(3) F.cx 末尾加注释后第三次 inference 为 7 行且 `artifact_hit=0`;(4) `-o` 不同路径的对象 sha 相同;(5) 不带 -o 跑两次 `stat -c %a` 与文件名一致;(6) `--strict-resource-flow` 下 -c 不查缓存且行为同旧;(7) phase-timing-stamps 与 cx-driver-route 相关批 `affected --run` 通过。
- N15-c 写域 `tests/phase-timing-stamps.cx`(非产品输入,装后才恢复):把 STAMP artifact 的 UNMEASURED 改回 artifact_hit 0 然后 1,并恢复断言;验收 `sh ./unisacc.com tools/affected.cx --ids phase-timing-stamps --run` rc 0,断言数不低于 68。
- N15-d 写域 `src/runcache.cx`:首切不需改(`rc_route`/`rc_key` 已含 mode 与 level);仅当 N15-a 的实跑发现 mode 5 的 `rc_open` 门(`:620-628`)拒绝才动,此时与 C2b 同文件,先于 C2b。
- C3-1(写域 `src/driveunits.cx` 与 `src/drive.cx` 的 `:685` 入口选择,晚于 N15-a 同文件串行):验收 `UNISACC_RR_TIME=1` 单文件新源 `grep -c unitcache` 大于 0(现为 0),`UNISACC_RUN_CACHE=0` 与开缓存产物同字节。
(6) 类别判定:N15-a、N15-b、(N15-d)、C3-1 全是产品输入(`src/`、`cx/runtime/`),归 63A 驱动与运行缓存类,与 L1、C2a、E3TRIM 同候选;N15-c 是测试,不入产品输入。对 0.0.63 候选的影响:N15 是结果不变的加速器,只对 -c 路径省约 280 ms;不动 SID、PC、GEN 的模型输入(不触 weights 与 kernel);键漏项风险(相对 include 头、-o 多存)已在 (3) 写明。UNVERIFIED:(a) 命中后 `ex_ri_emit` 对对象写出的权限位与未命中是否一致(取数:`stat -c %a` 对两次产物);(b) strict/capture 下 mode 5 的行为(取数:`UNISACC_STRICT...` 的实际变量名查 `runexec.cx` `ex_strict` 来源,再试跑);(c) 命中路径 `shown` 默认名在 `-o` 缺省时是否与 `objname` 一致(取数:不带 -o 跑两次 `-c`)。
- C3z(零解码)与本项不相交,照旧。

(7) 大改类别

属"驱动与运行缓存"类产品输入变更(`src/driveroute.cx`、`src/driveunits.cx`、`src/runcache.cx`、`cx/runtime/unitcache.cx`),与 C2a、C2b、C3z、L5(a) 同类可同候选。不得混入同一候选的:模型与网类(speed12b 重建 e2 网、L5(b) 跳过 prune、任何改 e3/prune/lower/elf 网或表的动作)、库包与运行库类(0.0.62 的封网与扫描族)。边界提醒:C3-3 若需要改 `cx/compiler/fwdmerge.cx` 的合并规则,仍算驱动侧代码不算网;但若为了让 lower 可拆而改 lower 的表或网,就越界到模型与网类,必须另开候选。发布时预制闭包条目属于产品打包内容变化,归同一类,发布说明须写明预制清单与未覆盖的闭包。

### 5.2 nn 下一步:推理侧候选、e2 线性成因、与前导预制的分工(2026-11-14;设计,未实现,不得记 PASS)

标注约定同 5.1:[实测] 引自上表 A12、L1(M1)与 5.1 的 E1 至 E3;[读码] 为本次只读读源码或表;[推断] 未证实;[UNMEASURED] 附取数命令。本稿没有新跑任何计时(整链在跑)。权重由 gold 构造,本节没有任何训练动作;所谓"更小的网"一律指改事实表后由生成器重构造。

(0) 口径与单位成本

- [实测] 首次减重复约 642 ms,六流推理 530.8 ms(82.7%):e2 146、e3 143、prune 102、lower 91、elf 47、e1 8;inflate 解码 49.3 ms;最小程序推理约 152 至 168 ms,其中 e2 89.7 至 105 ms(调度口径 93.8 ms)。
- [推断,跨两次运行拼算,只作量级] 按 E1 的 (c) 带子 11,539 行与 A12 基准(同为一行 printf)折算每带子行成本:e3 约 12.4 微秒,prune 约 8.8,lower 约 7.9;elf 按 lower 输出 9,135 行约 5.1 微秒。同一程序同一次运行的逐流行数与耗时配对 [UNMEASURED],取数见 X6。
- [读码] `UNISACC_RR_TIME` 的 inference 行只有 stage、rc、ns 三个字段(`cx/runtime/routetime.cx:90` 至 `:115`),没有执行步数。所以"每步多少纳秒、每流多少步"现在取不到,下面凡涉及"缩网能省多少"的上界都只能给到"不超过该流现耗时",网内分布一律 [UNMEASURED];缺口是 inference 行加 steps 字段,属产品源码变更(`cx/runtime/routetime.cx`、`cx/runtime/routestep.cx`),归驱动与运行缓存类。

(1) 候选表:哪些阶段换小网、换逻辑或不动

| 号 | 阶段与做法 | 上界收益(只用实测数) | 字节等价风险 | 类别归属 |
|---|---|---|---|---|
| K1 | prune:带子含可达 `callr` 时整带恒等(5.1 E2 定因)。做法甲:网内提前回退,先扫 `callr` 再建可达集,命中即走 FALLBACK 原样输出;做法乙:驱动先扫再决定跳过 prune 流 | 不超过 prune 现耗时:基准 102 ms(占 642 的 15.9%),最小程序 10.6 ms。实得取决于现网在何处判回退 [UNMEASURED,见 X2];若现网已是先扫后退,则这 102 ms 是逐行原样输出的成本,甲无收益 | 甲:输出仍是输入原样,产物字节不变,风险在回退条件必须与现 30 行 FALLBACK(`exec/prune/prune-result.tsv`,[读码] `grep -c FALLBACK` 得 30)完全一致,哪一行由 callr 触发未定位(X3)。乙:字节同样不变,但违反"编译行为差异写成模型步,不在驱动写特判",不采用 | 甲属模型与网类(`exec/prune/` 表与 prune 网);乙属驱动类且违规,否决 |
| K2 | prune:有 `callr` 时以取址集合为根继续裁,不整带回退(5.1 E2 第 7 条) | 对 hello 约 1%(只裁掉 `ferror`);对带用户死代码的程序,每个小死函数约 1.1 ms、82 B,200 个约 228 ms [实测 5.1 E2 第 4 条] | 高:产物字节必变(本就是目的),`indirect-call-preserved` 断言须保留或以等价取址根用例替换 | 模型与网类;与 K1 甲同表同网,可同候选 |
| K3 | lower:库函数逐函数结果复用(5.1 方案 B) | 不超过 lower 的闭包份:基准 91 减最小程序 13 约 78 ms(M1 口径 94 减 12 为 82);[实测 E3] 398 个共有段中 385 段同字节(96.7%,按字节 95.4%) | 中高:头部 `@sym`、`@data`、`@relocs` 与 13 个带生成编号的段须重算;只比过 (c)(d) 一对 | 驱动与运行缓存类,不改网;已在 5.1,不重复立项 |
| K4 | e3:闭包带子复用(5.1 方案 A) | 约 116 ms(145 降到约 29,M1 n=3) | 中:合并后带子可能与整程序 e3 不同字节,合同落在最终 ELF | 驱动与运行缓存类;已在 5.1 |
| K5 | e2:固定份(前导)结果预制或前导裁剪(L5(a)、10.2 紧凑前导) | 不超过最小程序 e2:89.7 至 105 ms | 预制:低(命中与未命中逐字节比);裁剪:高(改所有 e2 输出字节) | 预制属驱动与运行缓存类;裁剪属 cx 设计类(0.0.65 已排紧凑前导) |
| K6 | e2:随用户源增长的那一份(M1 的 (d) 到 (e) 同闭包下 322 到 679 ms)。做法待 (2) 定因后选:若成因在网的每标识符或每行成本,则改 pp 事实表重构造(speed12b);若成因在驱动侧缓冲或逐行调用,则改逻辑 | 不超过 357 ms(679 减 322,n=3 中位,load1 10 至 12,噪声大);对 A12 基准程序不超过 e2 146 减固定 93.8 约 52 ms | 改网:中(e2 输出须逐字节不变,pp 金表 1,105,532 键全键验证已有工具 `tools/lab-loader/net-check.cx`);改驱动:低 | 成因未定前不归类;改 pp 表与网即模型与网类 |
| K7 | elf:编码是查表型,无需学习。把 elf 网换回确定性逻辑 | 不超过 47 ms(7.3%);替换后的逻辑自身耗时 [UNMEASURED],净收益可能接近 0 | 高:12 个模型覆盖六目标的 image 与 object,换实现等于重写编码器,且与"以网换逻辑"方向相反 | 模型与网类;不做,理由是上界最小而面最大 |
| K8 | e1:词法 | 不超过 8 ms | - | 不做 |
| K9 | 量化、蒸馏、剪枝(学习意义上的) | 不适用:六流网是由金表精确构造的有限表执行,不是近似学习结果,没有可牺牲的精度;等价物是状态合并与表压缩(改事实表后重构造)。模型字节(e2 474,912、e3 1,095,431、lower 12 个共 2,029,794、elf 12 个共 994,216)只影响装载的 49 ms,不直接决定推理耗时;推理耗时等于步数乘每步成本,二者 [UNMEASURED,缺 steps 字段] | 状态合并须全键全状态复验 | 模型与网类;等 steps 字段有了再评 |

结论:(a) 无需学习、可以回到逻辑或提前退出的只有 prune 的回退判定(K1 甲)与 elf(K7),K7 不值得;(b) 网侧唯一上界够大的是 prune(102 ms)与 e2 的用户份(K6),两者都缺一次定因实验才能给出实得而非上界;(c) e3、lower 的大头走 5.1 的缓存路线,不动网。

(2) e2 随源码近线性增长:成因假设与取数

[实测 M1] e2(ms):(b) return 7 为 93;(c) 加 `stdio.h` 与一行 printf 为 158;(d) 再加 malloc、strcmp 为 322;(e) 同闭包再加约 90 行(45 个小函数)为 679。反常点:(e) 比 (d) 只多约 90 行用户源却多 357 ms,折合约 4 ms 每行;而 (c) 引入的 `stdio.h` 在带子里有 10,462 行,只让 e2 多 65 ms。即用户源每行的 e2 成本比头文件文本高两个量级以上,"按字节线性"解释不了。

假设(互不排斥):
- H1 噪声:M1 的 load1 为 10.1 至 12.0,(d) 的 e2 三次为 317 至 412;357 ms 的差里有多少是负载 [UNMEASURED]。
- H2 每标识符查宏表:用户源每个标识符都要对已定义宏做查找,成本约等于标识符数乘宏数;预测:不含头文件时斜率小得多,纯注释行与纯数字行斜率近 0。
- H3 头文件份被复用而用户份全价:头展开结果有缓存或预制(UNIUC3 只在多单元路,单文件是否有别的复用 [未验证]),用户文本每次全价过网;预测:换注释使缓存失效的口径下,头文件份并不便宜。
- H4 每行固定开销:行标记、帧或逐行调用;预测:同字节数拆成更多行则更慢。
- H5 输出累加二次:覆盖前扩容时的拷贝随已有输出增长;预测:行数翻倍耗时超过翻倍,且有头文件(已有输出大)时斜率更陡。

取数(全部只读,用已装产品,夹具与脚本在 `$CLAUDE_JOB_DIR/tmp`,脚本用 .cx;属速度取数,须等整链结束且 load1 低于 3):
- 夹具族 `g-K-N-I.cx`:K 为行种类(c 注释行、n 纯数字语句行 `1;`、d 声明行 `int vN;`、f 小函数行、l 同字节的长行即把 N 行并成一行),N 取 0、50、100、200、400,I 为是否 `#include <stdio.h>`。生成器是一个十几行的 .cx。
- 耗时:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com g-K-N-I.cx 2>&1`,取 `RR_TIME inference stage=e2` 的 ns,每点 n=3 取中位。
- 规模自变量:源字节与行数直接数;pp 输出字节 `sh ./unisacc.com stream e2 --target lnx/x86_64 < g.c | wc -c`;记号流字节 `... | sh ./unisacc.com stream e1 --target lnx/x86_64 | wc -c`(`stream` 对 .cx 前导的口径未核,先用同文本的 .c 取规模,耗时仍取 .cx 的 RR_TIME)。
- 判读:每个 (K,I) 对 N 做线性拟合得斜率(ms 每行)与截距;H2 看 f 行在 I=1 对 I=0 的斜率比;H4 看 f 对 l;H5 看 N=400 对 N=200 的比值是否明显大于 2;H1 看同点三次的极差是否超过斜率乘 50 行。结果原位填回本小节,并据此给 K6 归类。

(3) 后四流"运行库前导预制"与网侧改动的互补关系

- 模型:单流耗时约等于 固定份 加 每行成本 乘 (用户行数 加 未命中的闭包行数)。预制与缓存(5.1 方案 A、B,L5(a))减的是"未命中的闭包行数";网侧改动减的是"每行成本"与"固定份"。[实测 E1] 带子里非用户行占 97.3% 至 99.6%,所以对 hello 类程序,预制先吃掉绝大部分行数,网侧改动只对剩下的行与预制覆盖不到的流有意义。
- 预制覆盖不到的流:prune 与 elf 按语义需整程序(5.1 第 3 条),lower 的头部与生成标号段也要重算。A 加 B 落地后基准程序剩余推理约为 e2 146 加 e1 8 加 e3 约 29 加 prune 102 加 lower 约 13 加 elf 47,合计约 345 ms [推断,由 M1 与 A12 拼算];其中 prune 102 与 e2 146 占 72%。所以网侧工作应只投在 prune(K1、K2)与 e2(K5、K6),不要投在 e3 与 lower 的库份上,那部分会被预制拿走,投了等于重复。
- 顺序约束:预制条目的键含产品身份(UNICL1 的 (b) 项),任何改网的候选都会让全部预制条目失效,须在同一次发布里重生成预制清单;反过来预制的等价检查(关、未命中、命中三次同字节)是同一产品内的比较,不受改网影响。因此先落预制(驱动与运行缓存类),后做改网候选,两者不同候选;改网候选的发布步骤里加一步"重生成预制清单并跑 `tools/check-closure-reuse.cx`"。
- 一处会互相抵消:K2 让 prune 真裁之后,闭包的保留集随用户引用面变,5.1 方案 B 的键必须带 (f) 引用面,预制命中率下降;K1 甲不改输出,没有这个副作用。所以先 K1 甲,K2 排在方案 B 的命中率数据出来之后。

(4) 路线(0.0.65 至 0.0.67)与今日可开工的只读实验

- 0.0.65(已定为 cx 设计类,含紧凑前导):nn 侧不进产品输入,只做取数与设计:X1 至 X6 跑完并原位回填;补运行期流图事实表的草案(L3 行已记"运行期流图事实表不存在");给 `RR_TIME` 加 steps 字段的补丁草稿交集成者(驱动与运行缓存类,随 0.0.63 或 0.0.65 的同类候选带,不单开候选)。紧凑前导落地时用 X1 的截距核对 K5 的实得。
- 0.0.67(nn 网化类槽;osx/win netraw 属库包与运行库类,顺延到不早于 0.0.69):nn 的模型与网类候选只含 prune 一类改动(K1 甲;K2 视 X2、X3 与方案 B 命中率决定是否同候选),与 netraw 不同类,不得同候选;按 F9,整链降到 2400 s 以内才允许同版两候选,否则顺延 0.0.68。K6 若 X1 判定成因在 pp 网,另开一个只含 e2 网的候选,排在 prune 候选之后。
- 改网候选的验收(预先写明,不放宽):`stream-prune-cases` 批次全绿且 `indirect-call-preserved` 断言不删;K1 甲另加逐字节合同:M1 的 (a) 至 (e) 五个源改网前后最终 ELF sha256 相同;耗时用同机同负载 n=3 中位对比,未测不记 PASS。

只读实验清单(单项 15 分钟内;写域均为本小节对应行的原位回填,夹具与脚本放 `$CLAUDE_JOB_DIR/tmp` 不入库;"今"表示不取耗时、整链在跑也可做,"链后"表示速度取数,须等整链结束):

| 号 | 目标 | 命令 | 验收与预期值 | 开工 |
|---|---|---|---|---|
| X1 | e2 耗时对行数、行种类、有无头文件的曲线,判 H1 至 H5 | 见 (2) 的取数三条 | 得到 5 种行乘 5 个 N 乘 2 的表,每点 n=3;预期 f 行在 I=1 下斜率与 M1 推出的约 4 ms 每行同量级(2 倍以内),否则记为 H1 噪声并写明实测斜率;c 行斜率预期小于 f 行的十分之一 | 链后 |
| X2 | prune 在何处判回退 | 取 `chain.c`(`exec/prune/fixtures.json` 01-chain)的 -S 带子,造三份:`callr r5` 插在首个函数、末个函数、不插;再对 hello 带子(11,212 行)取原样与删去 callr 行两份;各跑 `sh ./unisacc.com stream prune --target lnx/x86_64 < T > O`,n=3 取最小 | 输出行数预期:小带子 98 到 98、98 到 98、97 到 81;hello 11,212 到 11,212 与 11,211 到 11,095(复现 5.1 E2);耗时预期:若首末两份相差 20% 以内且 hello 原样约 189 ms,则判"全带扫完才回退",K1 甲的实得按 189 减空输入启动约 5 ms 再减原样输出成本估;否则记实测差 | 链后(行数部分今可做) |
| X3 | 定位 callr 触发 FALLBACK 的表行 | `grep -n FALLBACK exec/prune/prune-result.tsv`,对照 `tools/prune-cases-gen.cx:111` 至 `:116` 的三个 callr 用例与 `exec/prune/libraryroots.md` | 预期 FALLBACK 行数 30(本次已核为 30);回填由 callr 或 callm 触发的行号至少 1 个,定位不到则写明缺的是网行到表行的映射工具 | 今;[已做 2026-11-14,读表] FALLBACK 行数 30 复核。callr 与 callm(op id 31、32,`exec/facts/prune-nametable.tsv` =ops)的触发链为三行:`prune-result.tsv:329` `EDGEOP.b115 31,32 -> INDIRECT`;`:350` `INDIRECT` 动作 `STX u 19000000 one`(只在函数 u 上置间接标志,此处不回退);`:369` `CLOSEFACT.b126 1 -> FALLBACK`(闭包阶段对可达集里任一 u 读到该标志即整带回退)。另 `:91` `INSDONE.b27 33,37,34 -> FALLBACK` 是 .hostcall/.hostaddr/.librarycall(host-abi-preserved 用例)。结论:回退发生在建边与根集之后的闭包遍历,即整带扫完才判,与 X2 的"全带扫完才回退"假设一致,K1 甲(先扫 callr 再建可达集)有可做空间但收益上界仍待 X2 的耗时。网行到表行的反查靠人读,缺 `prune.net` 状态名到 `prune-result.tsv` 行号的映射工具(未造)。 |
| X4 | lower 逐函数同字节在更多程序对上是否成立 | `sh ./unisacc.com X.cx -S > X.tape`;`sh ./unisacc.com stream lower --target lnx/x86_64 < X.tape > X.low`;`sh ./unisacc.com fn.cx -- A.low B.low`,程序对取 (b)(c)、(c)(e)、(d) 与调换 include 顺序的 (d) | 每对输出一行 `labelsA= labelsB= common= rawsame=`;预期 (c)(e) 的 rawsame 对 common 不低于 95%;调换 include 顺序一对预期明显降低或不变,二者都如实记录,它决定 K3 的键要不要含顺序 | 今;[已测 2026-11-14,load1 约 6,已装产品,lnx/x86_64,脚本与材料在 `$CLAUDE_JOB_DIR/tmp` 不入库;(e)=(d) 的三个头文件加 45 个 `static int fN(int a)` 小函数(各 3 行)与 6 次调用,共 146 行] 输出:(b)(c) `labelsA=62 labelsB=400 common=50 onlyA=12 rawsame=7`(最小程序与 stdio 程序几乎不同字节,不适用);(c)(e) `labelsA=400 labelsB=514 common=398 onlyA=2 rawsame=385`(96.7%,达预期 95%);(d)(e) `labelsA=424 labelsB=514 common=424 rawsame=420`(99.1%);(d) 对头文件顺序换成 string/stdlib/stdio 的 (d2) `common=424 rawsame=420`,与原序完全相同。结论:同闭包下逐函数同字节成立,差异仍是 `_start`、`main` 等少数入口与生成标号;include 顺序在此对上不改变 lower 输出,K3 的键暂不需含顺序(仅一对样本,头文件内容不变的前提下)。 |
| X5 | 预制能拿走多少行:(e) 程序带子的用户行与非用户行 | `sh ./unisacc.com e.cx -S > e.tape`;`sh ./unisacc.com seg2.cx -- e.tape` | 输出各来源的行与字节;预期非用户行占比不低于 95%(E1 的 (d) 为 99.3%),用户行数回填后用于 (3) 的剩余推理估算 | 今;[已测 2026-11-14] 输出(行/字节):总 13,856/231,602;e.cx 1627/25373,stdio.h 10463/176174,sys/_exit.h 690/11681,stdlib.h 748/12800,string.h 236/4007,errno.h 7/127,cx-flat.h 84/1432,(pre) 1/8。非用户行 12,228 占 88.2%(低于预期 95%,因 (e) 用户 146 行展开成 1627 带子行,约 11 倍);用户行数 1627 回填 (3)。若按 (d) 的 94 行 1.2% 外推会高估预制收益,(e) 类用户代码偏多的程序预制只能拿走约 88%。 |
| X6 | 同一次运行的逐流行数与耗时配对,替换 (0) 的拼算值 | 对 (c) 与 (e) 各跑 `UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com X.cx 2>&1`,n=3 中位;行数用 `-S` 带子与 `stream lower` 输出的 `wc -l` | 得到四流的微秒每行;预期与 (0) 的 12.4、8.8、7.9、5.1 同量级(2 倍以内),超出则以实测为准改写 (0) | 链后 |
| X7 | 确认 RR_TIME 无步数字段并登记缺口 | `sh ./unisacc.com tools/refactor.cx sites rr_infer`;读 `cx/runtime/routetime.cx:90` | 预期 inference 行字段只有 stage、rc、ns(本次读码已核);在 `build/pkg-gaps.tsv` 记一行(need:每流执行步数;建议归属 cx/runtime) | 今;[已做 2026-11-14] `cx/runtime/routetime.cx:90` `rr_infer(stage, rc, ns)` 输出 `RR_TIME\tinference\tstage=..\trc=..\tns=..`,确无步数字段;`refactor.cx sites rr_infer` 需文件参数(单独调用只打 usage),改以 grep 得调用点:仅 `archive/reference-c/tools/check-compiler-units.cx:569` 起的检查与定义处。缺口已记 `build/pkg-gaps.tsv`(每流执行步数,归 cx/runtime)。 |

未做与限度:本节没有新实测;(0) 的每行成本是跨运行拼算;K1 甲的收益在 X2 之前只有上界;K6 在 X1 之前不归类;六目标里只有 lnx/x86_64 有上述数字,其余目标的推理耗时 [UNMEASURED]。

### 5.3 切片逻辑网化普查(2026-11-14;只读普查与排序,未改源码,未训练,不得记 PASS)

标注同 5.2:[实测] 只引已有数(A12、M1、E1 至 E3、X3 至 X5);[读码] 为本次只读,精度分两档:"读行"指读过该行内容,"计数"指只做了 `wc -l` 与 grep 计数、没有逐函数读;[推断] 未证实;[UNVERIFIED] 附取数命令。本次限时 12 分钟,计数档居多,凡计数档的候选在立项前必须先做 (5) 的只读实验。本节没有新跑任何计时。

(1) 现状地图

已是网的阶段(流表 `exec/pipeline/image-stages.tsv` 共 7 行,读行):

| 流 | 输入 -> 输出 | 网 | 固定成本与增长 [实测,5.2 (0)] |
|---|---|---|---|
| e2 preprocess | src.c -> pp.text | e2.net | 最小程序 89.7 至 105 ms(调度口径 93.8),基准 146 |
| e1 lex | pp.text -> tokens.typed | e1.net | 基准 8 |
| e3 parse | tokens.typed -> tape.text | e3.net | 基准 143,随闭包增长 |
| e4 opt | tape.text -> tape.text | e4.net | 仅 level 大于 0 时进(`src/driveroute.cx:268`),耗时 [UNVERIFIED:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com -O1 X.cx 2>&1`] |
| prune | tape.text -> tape.text | prune.net | 基准 102,最小程序 10.6 |
| lower | tape.text -> target.text | lower.net(12 个) | 基准 91 |
| elf encode | target.text -> image.bytes | elf.net(12 个,含 macho、pe、object) | 基准 47 |

- [计数] 流与封装在 cx 侧只剩薄壳:`cx/compiler/stream/*.cx` 7 个文件各 16 行,`cx/compiler/image/{elf,macho,pe,object}.cx` 各 10 行,`cx/compiler/net/*.cx` 20 个文件合计 148 行;对应事实表在 `exec/facts/enc-elf.tsv`、`enc-macho.tsv`、`enc-pe.tsv`、`enc-x86*.tsv`、`enc-arm*.tsv` 与 `exec/lower/*.tsv`。所以任务书所举"elf 头"与"lowering 的目标特判"在产品路径上已在表与网里,不是手写候选;薄壳内容未逐行读 [UNVERIFIED:读这 31 个文件,合计约 300 行]。
- 另有 tapecheck(`exec/tapecheck/`,mode 2 且 level 不大于 0 时的终点名,`cx/runtime/runexec.cx:767`)。

仍是手写确定性逻辑的相邻部分(行数为 `wc -l`;"IO 数"为 `grep -c 'sy_open\|sy_read\|sy_write\|fopen\|sy_stat\|sy_spawn\|hs_read\|sy_close'`,0 表示经注入回调输出、本体无文件与进程调用):

| 号 | 位置 | 行数 | IO 数 | 做什么(精度) |
|---|---|---|---|---|
| H1 | `cx/runtime/runexec.cx:760` 至 `:774` st_gap | 15 | 0 | mode 与 level 到终点流名的 7 路分支(读行):e2、tokens、object、tapecheck、e4、memory、elf;唯一调用 `src/drive.cx:598` |
| H2 | `src/drive.cx:749`、`src/driveroute.cx:268` | 2 处 | 0 | 同一判定的字面量副本:level 大于 0 取 "e4" 否则 "prune"(读行);另有 6 个生成的流名表各带 "e4" 行(`cx/runtime/endname.cx:14`、`routename.cx:13`、`codename.cx:15`、`streamname.cx:15`、`cx/compiler/streamname.cx:14`、`src/stream.cx:12`) |
| H3 | `cx/driver/route.cx` | 420 | 0 | 路线选择,文件头自述"不分配、不 exit、不打印、无模块状态"(读头 6 行,函数未读) |
| H4 | `cx/compiler/fwdmerge.cx` | 351 | 0 | 按行扫带子文本并合并基带、桩、单元三份(读函数表到 :175);行种类判定 `fwm_eq/fwm_prefix/fwm_metaword` 共 34 处;e3 可拆的先例在 `:233` |
| H5 | `cx/compiler/fwdstub.cx` | 174 | 0 | 转发桩文本生成,去重字面量 86 个(计数) |
| H6 | `cx/compiler/fwdsig.cx`、`fwdmark.cx`、`fwdincrement.cx` | 514、250、112 | 18、8、未数 | 转发签名侧车与标记,带文件读写(计数) |
| H7 | `src/driveroute.cx` | 470 | 未数 | 流间胶水:`dv_fw_take/front/range/merge`(`:61` 至 `:108`)调 st_span 取区间、塞 `cli/funit` 附加输入、调 fwm_merge;`dv_fwd_one`(`:111`)解侧车(读行到 :130) |
| H8 | `cx/compiler/tapebin.cx` | 378 | 0 | 带子文本到二进制编码 tb_encode(调用 `src/drive.cx:385`);计数 |
| H9 | `cx/compiler/tapegate.cx` | 686 | 0 | LEB、引号、容器头判定与规范记录文本(读头) |
| H10 | `cx/compiler/tapelink.cx` | 762 | 0 | 产品带子链接器,驱动注入字节输出与失败回调,一次一链(读头) |
| H11 | `cx/compiler/asmtext.cx` | 2268 | 0 | ELF 汇编文本,去重字面量 476 个,固定容量表(`AT_MAXS 262144` 等,读头 12 行);本组最大的手写块 |
| H12 | `cx/compiler/cxmod.cx`、`cx/cx0/plan.cx` | 1450、1512 | 0(经宿主回调) | 模块 plan:`cxm_plan`(`:1300`)、`cxm_planprint`(`:1411`);两份同文(`cxmod.cx:408` 注释自述与 cx/cx0/plan.cx 同文),即 10.5 的三源 |
| H13 | `cx/runtime/unitcache.cx:247` 至 `:256` uc_keymake | 约 10 | - | 拼 blob 后 sha256 得键(读行);`cx/runtime/modelcache.cx` 397 行、`src/runcache.cx` 670 行同类 |
| H14 | `src/drive.cx:358` 至 `:390` dv_finish | 约 33 | 有 | 按 mode 分支落盘、a.out 默认名、tb_encode、rc_store_elf(读行) |
| H15 | `cx/driver/cli.cx`、`args.cx` | 559、279 | 未数 | 选项解析(计数) |
| H16 | `tools/ape-pack.cx` | 456 | 有 | APE 打包,构造期工具,不在每次编译路径上 |
| H17 | 前导与头文件展开 | - | - | 展开本体在 e2 网内;名表 `cx/compiler/predefname.cx` 22 行、`hdrname.cx` 18 行为生成物;手写部分是 cxmod 的强制 include 列表(并入 H12) |

(2) 判据

适合变网(五条都要成立):(a) 有精确金表或可由现实现穷举导出金表,输入输出是有限状态或带子/字节序列;(b) 本体无外部副作用(IO 数为 0,输出经回调或缓冲);(c) 现实现可作逐字节对照的参考,新旧产物 sha256 可比;(d) 输入输出本来就是流表里的类型(pp.text、tokens.typed、tape.text、target.text、image.bytes),接进 `image-stages.tsv` 不需新容器;(e) 状态量有上界且可验证(固定容量表要么能证不溢出,要么保留具名拒绝)。

不适合:依赖文件系统、进程、时间、环境变量(H6、H13、H14、H16);开放世界输入且无法穷举验证(H15 的 argv、H12 的路径与闭包解析);本体是密码学哈希或键拼装(H13:`exec/facts/enc-sha256.tsv` 已有 sha 的表,但键的成分是文件身份,网化无意义);只是有限映射而没有序列(H1、H2:该改成"事实表 -> 生成器 -> 表",不该加一个网,加网要多一次模型装载,第 8 节 B2 的 55-5 行按 A8 记每个模型约 12 至 30 ms)。

(3) 候选清单

模型数代价的口径:现 42 个模型;每加一个模型的整链增量 [UNVERIFIED:取 `build/graph.tsv` 里 kind 为 net 与 wave 的行的 observed_wall_s,按模型分摊;整链现 3538 s(prd_mempalace F1)],运行期每多装一个模型约 12 至 30 ms(A8),所以"扩现有网"优先于"新增网"。

| 号 | 候选 | 收益(上界) | 风险 | 类别 | 依赖 |
|---|---|---|---|---|---|
| S1 | H1 加 H2:终点流名与 e4/prune 分叉改为读流表(`image-stages.tsv` 加 mode、level 列或新增一张路线事实表,生成器出表,三处手写分支改查表)。不加网 | 速度 0;结构收益:消掉 3 处同义特判,符合"差异写成模型步";速度不取数 | 低:7 路输出可穷举(mode 0 至 5 乘 level 0、1、2),改前改后逐格比;风险在"遍历所有消费者",已知 3 处加 6 个名表 | 驱动与运行缓存类(改 `src/`、`cx/runtime`);若只加表与生成器、不改消费者则是纯工具,不进产品输入 | 无;与 K1 甲不同类不得同候选 |
| S2 | prune 网内提前回退(5.2 K1 甲,已排项,列此为锚) | 不超过 102 ms [实测上界],实得待 X2 | 见 5.2 | 模型与网类 | X2 |
| S3 | H4 fwdmerge 并入 e3 之后的带子流:做成 tape.text -> tape.text 的合并步(扩 e3 或 prune 邻接的网,或新增第 8 流) | [UNMEASURED] 现耗时未单列:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com A.cx B.cx 2>&1`,看非 inference 行里有无 merge 相位;若 RR_TIME 无此行则缺口同 X7 | 中高:三输入合并不是单带子流,需把基带、桩、单元串成一条带或用 `st_extra_put` 附加输入(`driveroute.cx:99` 已有此机制);金表来源是现 fwm_merge 的输出,行种类 34 处判定须先枚举成有限集;模型数可能加 1 | 模型与网类 | Y3;5.1 方案 A(闭包 e3 带子复用也依赖合并语义,先后顺序须一致) |
| S4 | H8 tapebin:带子文本 -> 二进制,扩 elf/encode 一族的表 | [UNMEASURED] 只在 emitbin 且 mode 2 时走(`drive.cx:385`),不在默认 -run 路径,速度收益对 first_e2e 为 0 | 中:逐字节金表可由现 tb_encode 导出;受益面窄 | 模型与网类 | Y4 |
| S5 | H10 tapelink 与 H9 tapegate:多带子链接做成流 | [UNMEASURED];是否在默认路径 [UNVERIFIED:`sh ./unisacc.com tools/refactor.cx sites tl_bind,tl_emit --tracked`] | 高:符号解析是开放集合(符号名任意),状态量随输入增长,判据 (e) 存疑 | 模型与网类 | Y4;先确认调用面 |
| S6 | H11 asmtext:2268 行 ELF 汇编文本 | [UNMEASURED];调用面 [UNVERIFIED:`sh ./unisacc.com tools/refactor.cx sites at_bind --tracked`];若只服务 -S 输出则不在 -run 路径 | 高:476 个字面量说明规则面大;固定容量表 4 张须保留拒绝合同;与 elf 网可能重复同一批编码事实(`enc-x86.tsv`),若重复则应改成从同一事实表生成,而不是再造网 | 模型与网类 | Y4;先核与 `exec/facts/enc-*.tsv` 的重叠 |
| S7 | H12 plan 里"pragma 行识别与判定"一份:即第 8 节 55-5 (a)"仅判定走网、机制留在切片" | 速度可能为负(多一次模型装载 12 至 30 ms,55-5 行已记);收益是三源归一后判定单源 | 高:55-5 自评为该线最重一步,波及 pp/lower/generator/facts(net-deps 门) | 模型与网类 | 10.5 三源归一(55-1)先完成 |
| S8 | H3 路线选择入表 | 速度 0 | 低至中:文件头自述纯函数,适合出金表;函数体未读 | 驱动与运行缓存类(同 S1,可同候选) | S1;Y2 |
| - | H5(桩文本生成) | 随 S3 一并评;单做无意义 | - | - | S3 |
| - | H6、H7 的 IO 部分、H13、H14、H15、H16、H17 | 不做:按 (2) 不适合 | - | - | - |

(4) 排序与路线

- 排序依据:最小可证看金表能否今天穷举,最大收益只认已有实测。现有实测里网侧可拿的仍是 prune 102 ms 与 e2 用户份(5.2 K1、K6);本次普查新增的 S3 至 S6 全部没有耗时数,不能排到 K1 甲之前。
- 第一步(最小可证):S1。18 格可穷举,不加模型,不动网。只做表与生成器加 `--check` 的那一半今天即可开工(纯工具);改三处消费者属驱动与运行缓存类,随 0.0.63 同类候选或其后第一个同类候选带,不单开候选。S1 消费者补丁(2026-11-14,未入库,基 c5853f58b2):结论是 st_gap 可改为逐行查生成行表且 7 路输出不变。做法:`tools/st-gap-gen.cx` 的 `build/st-gap-chain.txt` 由 if 链改为 `st_gap_row(index,&mode,&le0,&name)` 的 7 行(沿用 `cx/runtime/endname.cx` 的生成 if 行形状,不引入数据重定位),`cx/runtime/runexec.cx` 的 st_gap 改成对行表取首个匹配行,签名不变,调用方 `src/drive.cx:600` 不改;`drive.cx:749` 与 `driveroute.cx:268` 未动。补丁 `~/.cache/unisacc/patches-0.0.64/s1.patch.txt`(4 个文件,加 38 减 23)。证据(`git archive` 解到 /tmp 的副本上,已装产品,load1 2.9):`git apply --check` rc 0;`st-gap-gen.cx -- --check` 输出 `ST_GAP_GEN ok rows=7`;`check-st-gap-gen.cx` 输出 `CHECK_ST_GAP_GEN ok=18 failed=0`;`-c cx/runtime/runexec.cx` rc 0;对拍探针把新旧 st_gap 源段原文各改名后同编,mode -1 至 9 乘 level -1 至 3 乘 cap 4 与 32 共 110 格,返回值、名字、ex_obj_mode 全同,输出 `ST_GAP_EQ cells=110 diff=0`,其中 18 格(7 路)逐行 SAME。UNMEASURED:带补丁重建产品后的整产品行为、六目标、棘轮与 derive 全闸;新增的无匹配行返回 1 分支在现表(有 `*` 行)下不可达,未实测。
- 第二步(最大已测收益):S2 即 K1 甲,落点不变:0.0.65 只取数(X1、X2、X6),0.0.67 模型与网类候选只含 prune,整链未降到 2400 s 以内则顺延 0.0.68。K1 甲现状(2026-11-14 实测,已装产品,lnx/x86_64,load1 2.9,未改源码):结论是 prune 本身已是网(`exec/pipeline/image-stages.tsv` 的 prune 行用 prune.net,cx 侧 `cx/runtime/stream/prune.cx` 只有 16 行),不存在"用 nn 替换"的剩余逻辑;而"先扫 callr 即回退"不能做,因为它不保字节且几乎无收益。证据:(1) 回退条件依赖可达性:`int dead(int(*f)(void)){return f();}` 加 `int main(void){return 7;}` 的 -S 带子 65 行含 1 个 `callr r5`(在不可达的 dead 内),`stream prune` 后 43 行、dead 与 callr 均被裁,与不含 dead 的同程序输出行数相同;先扫即回退会输出 65 行,产物变,违反 K1 甲"输出不变"的前提;这与 X3 的 `prune-result.tsv:369`(闭包阶段读到可达函数的间接标志才回退)一致。(2) X2 耗时部分:hello 带子 11,212 行,回退路(恒等,cmp 相同)3 次 194、197、197 ms;删去 callr 行后真裁路(到 11,095 行)3 次 206、210、200 ms;两路只差约 6 至 13 ms(3% 至 6%),即耗时主体是全带扫描与建边,回退判定之后可省的闭包余段与保留集输出至多这个量级,102 ms 的上界不成立。最小可做切片:只剩"闭包遍历中首次读到间接标志即回退"(现网已如此,`:369` 在 CLOSEFACT 循环内),无新增可做项;要拿到 prune 的耗时须降扫描与建边的每行成本(属 K2 之外的另一题,未立项)。建议 0.0.67 的 prune 候选撤下 K1 甲,待集成者裁定。UNMEASURED:进程启动与模型装载在 194 ms 中的占比(本次未扣)、扫描与建边的分相、n 大于 3 的安静机复测、其余五目标。
- 第三步:S3,条件是 Y1 取到合并相位耗时且 Y3 证明行种类有限。它与 5.1 方案 A 的关系:方案 A 把闭包 e3 带子缓存后仍要合并,合并步若进网,缓存键须含该网身份(UNICL1 的产品身份项已覆盖);所以先落方案 A(0.0.63,驱动类),S3 最早 0.0.68,且与 K1 甲同为模型与网类,可在 prune 候选发布后另开一个候选,不与之合并(一个候选只含一类大改,同类也按网分开以便定位字节差)。
- 与 L5(a) 前导 e2 预制的衔接:预制是缓存类,减 e2 固定份;本节没有新增 e2 侧候选,H17 的手写部分并入 S7。S7 排在 55-1 之后,不早于 0.0.68。
- S4、S5、S6 在 Y4 出调用面之前不排版本;若都不在默认 -run 路径,则只按"与事实表重复"处理(改成从 `exec/facts/enc-*.tsv` 生成),不立网化候选。
- 落点汇总:0.0.65 取数与 S1 的表半边;0.0.67 S2(prune);0.0.68 S3 或 K6 二选一(看 X1 与 Y1 谁的实得大),S7 视 55-1。

(5) 今天可做的只读实验(各 15 分钟内,不取耗时的可在整链运行时做;产物写 `$CLAUDE_JOB_DIR/tmp/Y-N.tsv`,互不相交;脚本用 .cx)

| 号 | 目标 | 命令 | 验收与预期值 |
|---|---|---|---|
| Y1 | 手写胶水在一次运行里占多少 | 链后:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com hello.cx 2>&1`,列出全部 RR_TIME 行种类并分 inference 与其余求和,n=3 中位 | inference 之和预期约 531 ms(A12);其余行之和即 S3 至 S8 的总上界;预期不超过 642 减 531 减 inflate 49 约 62 ms,若成立则本节所有驱动侧候选的速度上界合计不超过 62 ms,网化理由只剩结构而非速度 |
| Y2(2026-11-14 已做:st_gap 定义 runexec.cx:760,唯一调用 drive.cx:598;drive.cx:749 与 driveroute.cx:268 是另一映射 level>0?e4:prune,不是 st_gap 副本,S1 须另立事实行;18 格读码手算(st_gap 不可从探针直调),实际 7 个不同输出,见 build/st-gap.tsv;S1 表半边已做:exec/facts/st-gap.tsv、tools/st-gap-gen.cx、check-st-gap-gen.cx,消费者未改) | S1 的消费者全集与 18 格真值表 | `sh ./unisacc.com tools/refactor.cx sites st_gap --tracked`;`grep -rn '"e4"\|"prune"\|"tapecheck"' src cx/runtime cx/compiler cx/driver --include=*.cx`;按 `runexec.cx:760` 至 `:774` 手列 mode 0 至 5 乘 level 0 至 2 | st_gap 定义 1 处、调用 1 处(`drive.cx:598`,本次已核);"e4" 命中预期 11 行(本次 grep 得 10 个文件 11 行,含 `drive.cx:749` 与 `driveroute.cx:268`);真值表 18 格,7 个不同输出 |
| Y3 | fwdmerge 行种类是否有限 | 读 `cx/compiler/fwdmerge.cx:48` 至 `:130` 与 `:175` 至 `:351`,列出 `fwm_eq/fwm_prefix/fwm_metaword` 的全部字面量参数 | 判定调用 34 处(本次已数);去重字面量预期不超过 29 个(本次计数全文件 29 个);列出后回填 S3 行,若出现依赖符号名内容的分支(`fwm_hasname`、`fwm_isexport`,`:120`、`:124`)则记为"状态随输入增长",S3 风险升为高 |
| Y4 | S4、S5、S6 是否在默认 -run 路径 | `sh ./unisacc.com tools/refactor.cx sites tb_encode,tl_bind,at_bind --tracked`,对每个调用点读其守卫条件 | 回填三行的"调用面";预期 tb_encode 只在 emitbin 且 mode 2(本次已读 `drive.cx:385`);另两个未知,如实记录 |
| Y5 | 薄壳确认 | 读 `cx/compiler/stream/*.cx`、`image/*.cx`、`net/*.cx`(共 31 个文件约 300 行) | 预期全部只有 module/use 与转调,无分支逻辑;有例外则补进 (1) 的手写表 |

Y3 与 Y4 结果(2026-11-14,只读取证,`cx/compiler/fwdmerge.cx`、`src/drive*.cx` 原文):

Y3 fwdmerge 行种类表(行号为 `cx/compiler/fwdmerge.cx`):

| 行种类 | 识别形态 | 行号 | 行间依赖 |
|---|---|---|---|
| 结束行 | 整行 `.end`,且必须是最后一行 | `:99` `:100` | 无 |
| 单元头(基带) | 仅首行 `.unit 2` | `:103` `:105` | 无 |
| 来源行 | `.unit ID LEN:HEX`,ID 小于 128,十六进制对不含 00 | `:106`、`fwm_source :64` 至 `:86` | 有:记为后续各行的 source,影响 `.unit` 重发 |
| 全局声明 | 前缀 `.global ` | `:259` | 全局名表(上限 4096 个) |
| 桩区声明 | 前缀 `.global `、`.extern `、`.gdef ` | `:285` `:329` 等 | 跳过 |
| bss 名 | 前缀 `.bss ` 后接标识符 | `:288` | 收集局部名 |
| 标号 | 非空格开头、以 `:` 结尾 | `:290` | 收集局部名 |
| 锚点 | 整行 `main:`、`__init_u:`、`__main_ret:`(桩);`__init:`、`__main_ret:`(基带) | `:263` 至 `:270` | 全局位置:取各锚点首次出现的行号,须满足 body<main<sinit<sret、ainit<aret |
| ret 行 | 整行 `  ret` | `:276` `:280` | 位置依赖(最后一个 ret 为插入点) |
| 调用行 | 前缀 `  call ` | `:284` `:288` | 插入点向前回溯连续 call 行 |
| 退出行 | `  .exit r0`、`  call exit`、`.extern __init`(尾部只许这三类与 `.str `) | `:292` `:293` | 位置 |
| 字符串行 | 前缀 `.str `(引号内不改写) | `:206` `:297` | 无 |
| 系统调用行 | 前缀 `  .sys `、`  .sys6 `(逗号前原样) | `:200` | 无 |
| 标识符内的改名 | `__unisacc_L/R<数>`、`S<数>`、`ls<数>`、局部名 | `:214` `:217` `:219` `:223` | 偏移量由基带全表最大值决定 |

Y3 结论:行种类本身是有限集,识别只看行首前缀或整行相等,所以"行种类判定"可以进事实表。但合并语义不是有限状态转移:(1) 必须先整带切成行再全局找 7 个锚点(`:263` 至 `:270`);(2) 局部名集合依赖符号名内容,`fwm_hasname/fwm_isexport`(`:120` `:124`)查开放集合,上限 4096 名、1024 字节(`:131`);(3) 标号与字符串偏移取基带全带最大编号 labmax/strmax(`:296` 至 `:312`),是先扫后用,不是邻行转移;(4) 来源映射表 128 项(`:179` `:183`)。判定:行种类有限,合并整体需要"先全扫(锚点、名集合、最大值)后写(带偏移改名)"的两遍序列网,不能表示为事实表加有限状态转移;S3 风险维持"中高"。
默认 -run 路径:`fwm_merge` 经 `dv_fw_merge`(`src/driveroute.cx:107`)作为回调传入 `fwd_incremental`(`src/driveroute.cx:262` 至 `:264`),只有增量路径(fast>0)才执行;最小程序 `int main(void){return 0;}` 的 `UNISACC_RR_TIME=1` 输出只有 e1/e3/prune/lower/elf 五个 inference 行,无合并相位。多单元增量下是否调用 [UNVERIFIED:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com A.cx B.cx 2>&1 | grep -ci merge`]。

Y4 调用面分类(git grep 取数):

| 函数 | 调用点 | 守卫 | 分类 |
|---|---|---|---|
| `tb_encode` | `src/drive.cx:385` | `emitbin && mode2 == 2`(--tapebin 的 -S) | 仅 -S 路径;另有 `tools/check-tapebin.cx:28,44` |
| `tl_bind` | `src/drivelink.cx:193`(`dv_link_objects`);`tools/image-contrib.cx:59`;`tools/tape-origin.cx:85` | `dv_link_objects` 由 `src/drive.cx:229`(`dv_link_objects_route`)调用,`src/drive.cx:502` 守卫 `cxp_object_path(src)`,输入是 .o 才走 | 仅链接 .o 路径与工具;源 .cx 默认 -run 不走 |
| `at_bind` | `src/driveface.cx:125`(as)与 `:169`(nm/objdump);`tools/check-asmtext-tables.cx:104` | 入口是 as/nm/objdump/ar 子命令 | 仅工具型子命令,不在默认 -run |

三项均不是默认路径热点。回填:S4、S6 不排版本(按 5.3 既定,只按与事实表重复处理);S5 只在链接 .o 时用,收益面窄。UNVERIFIED:加载层面是否读到这三个单元(`.com` 是单一二进制,strace 的 openat 区分不出),取数 `sh ./unisacc.com tools/refactor.cx sites tb_encode,tl_bind,at_bind --tracked`(本次以 git grep 代替)。

未做与限度:H3、H5、H6、H8 至 H11、H15 只做了计数与读文件头;`src/driveunits.cx`(451 行)、`src/stages.cx`(424 行)、`cx/compiler/routepipe.cx`(190 行)、`tools/facts-gen.cx`(4343 行)、`tools/target-gen.cx`(422 行)、`build/graph.tsv`(211 行)只取了行数,没有读内容,其中的手写逻辑未进本表 [UNVERIFIED:按 (1) 的 IO 数与字面量计数口径补做];本节所有新候选没有任何耗时实测。

### 5.4 Bun 冷热启动技术借鉴(2026-11-14;只读研究,未改源码,未运行 Bun,不得记 PASS)

标注约定:[已核 URL] 为本次联网读到原文;[已核摘要] 为检索结果摘要里读到、正文页 404 未取到;[忆] 为记忆,未核,不得当事实引用;[实测] 只引本节上表与 5.1、5.2、prd_mempalace.md、build/speed-baseline.tsv 的已有数;[本次] 为本次只读实跑(n=1)。来源简称:U1 = https://bun.com/blog/behind-the-scenes-of-bun-install ;U2 = https://bun.com/blog/bun-v1.1 ;U3 = https://bun.com/blog/bun-v1.4.md ;U4 = https://bun.com/docs/bundler/bytecode ;U5 = https://bun.sh/docs/install/cache ;U6 = https://www.kvakil.me/posts/2023-05-23-nodejs-startup-series-comparing-to-deno-and-bun.html ;U7 = https://webkit.org/blog/10308/speculation-in-javascriptcore/ 。

(1) Bun 的手段(冷启动前五,加热启动)

| 号 | 手段 | 原理 | 代价 | 来源 |
|---|---|---|---|---|
| B1 | 引擎选 JavaScriptCore | 第三方实测 Bun 启动约为 Node 与 Deno 的 3 倍快,作者只猜测是 JSC 更适合快启动,没有给出机制;JSC 四层为 LLInt、Baseline、DFG、FTL,代码先解释、按热度升层 | 峰值吞吐与生态兼容另算;升层阈值与编译成本见 (5) | 3 倍与猜测 [已核 U6];四层 [已核 U7];"JSC 启动快因为 LLInt 不先编译" [忆] |
| B2 | 构建期字节码缓存(--bytecode) | 构建时把 JS 预编成字节码,运行时跳过解析;文档举例 tsc 启动约快 2 倍;1.4 起支持 ES 模块且须配 --compile | 字节码绑定确切的 JSC 版本,升级 Bun 必须重生成;.jsc 约为源的 2 至 8 倍;内建模块不走这份缓存 | [已核摘要 U4];ESM 一条 [已核 U3] |
| B3 | 单文件可执行(--compile) | 代码与资源嵌进可执行文件,--asset 可嵌文件或整个目录并保留原名;Linux x64 本体 77.0 MB | 体积;产物绑定本体版本 | --asset 与体积 [已核 U3];"从自身映像直接读、不解包落盘" [忆] |
| B4 | 运行期转译缓存 | 按内容寻址,只缓存大于 50 KB 的文件,避免重复转译;tsc 类命令行工具比 1.0 快到 2 倍 | 小文件不进缓存(阈值换条目数);存放位置与校验方式原文未写 | [已核 U2] |
| B5 | 少发系统调用、二进制清单、预分配 | 一次简单安装的系统调用数:bun 165,743,pnpm 456,930,npm 996,978,yarn 4,046,507;包清单存二进制;读 gzip 尾部 ISIZE 一次分配解压缓冲(16 B 至 64 MB 时);数组结构布局约 6 次大分配;Linux 用硬链接(109.4 对 copyfile 325.0 ms),macOS 用 clonefile | 二进制锁文件因不可审阅已弃用,改回文本;硬链接使缓存与工程共享同一份字节 | [已核 U1][已核 U5] |
| B6 | 启动本身(1.4:Linux hello 5.1 ms,1.3 为 10.9,Node 26 为 27.2;峰值内存 14.6 MB) | 发布文前 10 万字符没有写启动变快的机制 | - | 数字 [已核 U3];"内建模块惰性初始化、静态链接、mimalloc" [忆] |
| H1 | 热路径:先判已完成再退出 | node_modules 已齐即结束;缓存安装 9.1 ms(npm 缓存安装 363.1 ms);清单用 ETag,304 时不解析 | - | [已核 U1][已核 U5] |
| H2 | watch 与热重载 | - | - | [忆],本次未取到原文,不映射 |

(2) 映射到 unisacc

| 号 | 借鉴点 | unisacc 现状 | 已有等价物 | 具体改法 | 收益上界 | 风险 | 类别 |
|---|---|---|---|---|---|---|---|
| T1 | B5 少发系统调用、B5 二进制清单 | `cx/runtime/modelcache.cx:239` mc_trim 每次发布全量扫,每条目 lstat 加 open/fstat/read/close;K2 在 0.0.62 重测斜率约 102 微秒每条目,0/2000/4000 项首次 284/491/692 ms(build/speed-baseline.tsv 末三行);[本次] 目录现 2282 项 | 补丁已备未入候选:E3TRIM2 的 `.budget.acct` 总量账(prd_mempalace.md E3TRIM 行)正是 Bun 二进制清单的同形物 | 不新增设计;沿用 E3TRIM2。Bun 侧可补的只有一条:顶满预算时淘汰到低水位再停(Bun 无对应原文,[忆]),E3TRIM2 已把它列为未解 | 1938 项约 200 ms(102 乘 1938;按先前 28 的斜率只有约 54 ms,两次斜率差 3.6 倍未解释);E3TRIM2 副本候选实测账已在时 0/2000/4000 项 228/226/231 ms,斜率约 0(入口形态与基线不同,未隔离);目录顶满预算时仍全扫,该情形收益为 0 | 账与目录不一致时须回退全扫;并发发布写账要锁 | 驱动与运行缓存 |
| T2 | B4 的 50 KB 阈值 | 预算只限字节不限条数,均值约 550 KB(E3TRIM 行);条目大小分布 UNMEASURED | 无 | 小于阈值的条目不发布,或加条数上限(E3TRIM 修法 (4)) | UNMEASURED;取数:用 .cx 列 `$TMPDIR/unisacc-e3cache-<uid>` 各条目字节数并按 50 KB 分两档计数 | 小程序恰是首次运行基准,阈值设错会让它永不命中 | 同上,并入 T1 |
| T3 | B2 构建期预编、绑定版本、不符即回退 | L1 方案 A(e3 闭包带子复用,5.1)与 L5(a) 前导 e2 预制均为设计;键已含产品身份 rc_hex_ident | 设计上等价(5.1 (4) 已写发布时预制) | 采纳 Bun 的三条合同:键绑确切产品身份、不符静默回退整程序、发布说明写体积;(c) 闭包带子约 194 KB 文本(E1) | 方案 A 约 116 ms;L5(a) 不超过 89.7 ms;两者相加约 206 ms | 命中须与未命中同字节;预制体积进产品 | 驱动与运行缓存 |
| T4 | B3 嵌入后直接读、B5 按头里长度一次分配 | model 相合计约 53 ms,其中读入 2 至 3、校验 8 至 10、建布局约 38 至 40 ms(N2,间隔估计) | 无(C3z 已排) | mmap 已解码布局;若不做 mmap,先按条目头长度一次分配 | 不超过 49 ms;一次分配单做的份额 UNMEASURED(缺 read/verify/build 三个 RR_TIME 字段,属产品源码变更) | 六目标的映射合同;布局随产品身份失效 | 驱动与运行缓存 |
| T5 | H1 先判已完成 | -c 每次全跑约 280 ms(prd_mempalace.md 第 75 行);默认 -run 重复路径已是先查后退,约 8 至 29 ms | -run 有;-c 无(N15 已排,补丁草稿已备) | N15-a 把 mode 5 纳入 rc_open 门 | 每次重复 -c 约 280 ms | 键漏项(5.1 N15 (3) 已列) | 驱动与运行缓存 |
| T6 | B5 系统调用数(热路径) | [本次] 重复运行 hello 一次共 412 次系统调用,其中 135 次失败(close 98 次里 59 次失败,newfstatat 78 次里 60 次失败),mmap 31、openat 12、read 19 | 无 | 查出 59 次失败的 close 与 60 次失败的 newfstatat 属 sh 前奏还是产品,去掉无效探测 | 重复路径总共约 8.1 ms(空目录档),上界小于此;份额 UNMEASURED,取数:`strace -f -e trace=close,newfstatat -o F sh ./unisacc.com h.c` 按 PID 归属 | 收益小;前奏在打包头里,属产品输入 | 驱动与运行缓存(前奏)或库包与运行库(若在 L0) |
| T7 | B5 硬链接 | 缓存命中后把字节再写出一份 | 无 | 不取:硬链接使用户改产物即改缓存条目 | - | 缓存被污染 | - |
| T8 | B1 换引擎 | 推理是 nn 加流 | - | 不取,理由见 (5) | - | - | - |

(3) 与已排项的去重与互补

- E3TRIM 摊销账:T1、T2 与它是同一件事,Bun 只补了"总量账"的形状和阈值的做法,不另立项。
- N15:T5 即 N15,无新增。
- L1 方案 A 与 L5(a) 前导预制:T3 不改设计,只补三条合同措辞;二者互补(一个吃 e3 的闭包份,一个吃 e2 的固定份)。
- 零解码 mmap:T4 即 C3z;"一次分配"是它的最小前置片,可在 mmap 之前单独验证。
- prune 提前回退:Bun 侧没有对应物;与 (5) 的"分层按需"同向(恒等时不付全价),仍属模型与网类,不与以上同候选。
- 新增的只有 T6(热路径系统调用清点),量级小于 8 ms。

(4) 排序与落点

| 序 | 项 | 收益上界 | 风险 | 类别 | 依赖 | 落点 |
|---|---|---|---|---|---|---|
| 1 | T1 E3TRIM(含 T2) | 约 200 ms(1938 项,斜率 102);按斜率 28 为约 54 ms | 低,纯加速器 | 驱动与运行缓存 | 无;C2a 同文件串行在其后 | 0.0.63 最先做,也是最小可证(K2 曲线复测即验收) |
| 2 | T5 N15 | 每次重复 -c 约 280 ms | 低,补丁已过 apply --check | 驱动与运行缓存 | 无;C3-1 同文件在其后 | 0.0.63 |
| 3 | T3 方案 A | 约 116 ms | 中,合并器同字节 | 驱动与运行缓存 | E4 切法、C3-1、C3-2 | 0.0.63 |
| 4 | T3 L5(a) | 不超过 89.7 ms | 中,前导随 `\0cli` 状态变 | 驱动与运行缓存 | unitcache 单文件入口(C3-1) | 500 ms 探针不绿则 0.0.63,否则 0.0.65 |
| 5 | T4 零解码 | 不超过 49 ms | 中,六目标映射 | 驱动与运行缓存 | C2a 之后 | 0.0.63 末位或 0.0.65 |
| 6 | prune 提前回退 | 不超过 102 ms(基准程序 prune 全程,恒等) | 中,不得删 indirect-call-preserved 断言 | 模型与网 | 无 | 0.0.65 之后,单独候选 |
| 7 | T6 热路径系统调用 | 小于 8.1 ms | 低 | 驱动与运行缓存 | R2 归属结果 | 0.0.65,不单独立项 |

最大收益是 T1 与 T3 两行相加(约 200 加 206 ms,前者随目录条目数变);最先做与最小可证都是 T1。口径提醒:K2 的 0 项档首次 284 ms 与 run-startup-bench 的 715.9 ms 用的程序和目录不同,二者未对齐,见 R1。

(5) 单位成本对照(项目所有者 2026-11-14 观点:nn 流推导的单位成本不会比 JIT 差多少;本条不预设 JIT 更快)

- unisacc 每单位输入成本 [实测,拼算]:空 main 推理约 168 ms(固定份,其中 e2 约 90 至 105 ms);一行 printf 约 11.5 千带子行,六流约 567 ms;后四流每带子行 e3 约 12.4、prune 约 8.8、lower 约 7.9、elf 约 5.1 微秒,合计约 34.2 微秒每行;e2 约每源行 4 毫秒。
- JSC 公开数据 [已核 U7]:升层阈值 LLInt 到 Baseline 500 点、Baseline 到 DFG 1000 点、DFG 到 FTL 100000 点(每次调用 15 点、每次循环 1 点);每字节码执行耗时约 3.97(LLInt)、1.71(Baseline)、0.349(DFG)、0.225(FTL)纳秒;DFG 编译耗时约为 Baseline 的 4 倍,FTL 约为 DFG 的 6 倍;编译在并发线程上。解析、字节码生成、各层编译的绝对耗时(每行或每字节码多少微秒)本次没有查到,UNMEASURED,不拿记忆补。
- 可比的部分:只有"热路径端到端"一项勉强同口径,Bun hello 整进程 5.1 ms [已核 U3] 对 unisacc 重复运行约 8.1 ms(空目录档)至 29 ms(run-startup-bench);同一量级,且两边都没有做编译,靠的都是复用。
- 不可比的部分:(a) JSC 的纳秒数是执行成本,我们的微秒数是编译成本,单位不同;(b) JSC 编译的绝对单位成本缺公开数,所以"nn 单位成本比 JIT 差多少"在本次资料下不能下结论,既不能证实也不能证伪;(c) 产物不同,我们出完整 ELF,JSC 出内存里的分层代码。
- 结构差别 [已核 U7 的阈值可推出]:JSC 对只跑一次的函数只付解释,500 点以上才付 Baseline,十万点才付 FTL,且编译不占主线程;unisacc 每次首次运行对整条带子付全价,而带子里非用户行占 97.3% 至 99.6%(E1),即几乎全部成本花在每次重复的闭包上。
- 判断:收益来自摊销与复用(缓存、预制、增量、分层按需),不是把 nn 换成别的执行方式。依据是我们自己的数:同一套 nn,重复路径 8 至 29 ms 对首次约 716 ms;一行 printf 让后四流由 58 增到 395 ms,增量全是可复用的闭包。换执行方式动不了这 99% 的重复份。故 T8 不取。
- 分层按需的可借鉴形状:(a) 逐函数 lower 结果按内容缓存,E3 实测 398 个共有标号段里 385 个同字节(按字节 95.4%),这是静态版的"只对新函数付费",可行性较高,待 X4 在更多程序对上复核;(b) 真正的惰性 lower 与 elf(只对被执行到的函数走完后四流):出 ELF 文件的路径不可行,产物必须含全部可达函数;只在默认 -run 路径可设想(未执行函数留桩,首次调用时再编),但要求运行期进程内带编译器、可写可执行内存,六目标都要有合同,且 elf 流按语义需整程序布局(5.1 (3)),风险高。收益上界 = hello 实际执行到的函数占带子行的比例乘后四流 395 ms,该比例 UNMEASURED,且现无取"被执行函数集合"的入口(缺口,应记 build/pkg-gaps.tsv,本次写域不含);可先取的代替数:`sh ./unisacc.com X.cx -S > X.tape`、`sh ./unisacc.com stream lower --target lnx/x86_64 < X.tape > X.low`、`sh ./unisacc.com fn.cx -- A.low B.low`(X4,量逐函数同字节率);(c) prune 恒等时提前回退,同向。
- 反例,如实列出(nn 流相比 JIT 明显更差之处):(1) 固定成本:空 main 也要约 168 ms 推理加约 53 ms model 相,Bun 整进程 5.1 ms [已核 U3],差一个量级以上,虽然两边做的事不同,但用户感受到的首次等待就是这个数;(2) 不可增量:现键是整程序键,改一个注释即整程序重跑(5.1 (1)),JSC 按函数升层;(3) 批量依赖:prune 与 elf 按语义需整程序,不能逐函数产出再拼(5.1 (3));(4) e2 约每源行 4 毫秒且随行数线性(M1 的 (d) 到 (e) 322 到 679 ms),常规解析器的单位成本 [忆] 远低于此,未核;(5) 没有"便宜的第一层":JSC 有解释器兜底,我们没有低成本档,小程序直通 L5(b) 是唯一接近的候选,且属模型与网类。

(6) 今天可做的只读实验(各 15 分钟内,材料放 `$CLAUDE_JOB_DIR/tmp`,写域互不相交)

| 号 | 目的 | 命令 | 预期 |
|---|---|---|---|
| R1 | 对齐 K2 的 284 ms 与基准 715.9 ms 两个口径 | 同一基准源各跑 3 次首次:一次 `UNISACC_E3CACHE` 指向新建空目录,一次用默认目录;`UNISACC_RR_TIME=1 sh ./unisacc.com F.c`,每次换末尾注释 | 两组差约等于 102 微秒乘默认目录条目数(现 2282 项约 233 ms);不等则斜率或口径有误,如实记 |
| R2 | 热路径 135 次失败调用归属 | `strace -f -e trace=close,newfstatat -o F sh ./unisacc.com h.c`(先跑一次使其命中),按 PID 分 sh 与产品 | 已得总数 412、失败 135 [本次,n=1,load1 2.9];归属未知 |
| R3 | 模型读入是 read 还是 mmap | 新源首次:`strace -f -e trace=openat,read,pread64,mmap -o F sh ./unisacc.com new.c`,只看 mc- 条目的 fd | 预期六个模型条目都是 read 整读、无文件映射(N2 读到"71 字节头加整体");若已是 mmap 则 T4 上界要下调 |
| R4 | T2 阈值可行性 | 用 .cx 列缓存目录条目大小,分小于 50 KB 与其余两档计数与字节和 | 未知;小条目占条数多而字节少才值得做阈值 |
| R5 | T1 修法 (3) 的份额 | K2 同法造 2000 个假 uc- 项,对比 `strace -c -f` 里 lstat 与 open/read/close 的次数 | 每条目 lstat 1 次、open/fstat/read/close 各 1 次;只留 lstat 约去掉五分之四的调用数,耗时份额待测 |

R2 结果(2026-11-14,表中 R2 即任务称的"R1";已装 0.0.62,`strace -f -ttt -T`,hello.c 第二次运行,n=1,整程序缓存命中):全程 444 行,失败 135 次,按名与 errno 分类如下。strace 放大约 6 倍,真实值取 1/6 量级。

| 名 | errno | 次数 | 调用方 | 实测耗时(strace 内) |
|---|---|---|---|---|
| close(4..62) | EBADF | 59 | `src/host.cx` 约 422 行 memfd 启动后的 `while (c < 63)` 循环:对 3..62 逐个 `HS_CLOSE`,仅 `hs_fd_argv(argv)` 位图保留的 fd 跳过;fd 3 是 memfd 刚 dup3 到 63 后已关,4..62 全无效 | 0.335 ms(整程序 24.3 ms 的 1.4%) |
| newfstatat | ENOENT | 60 | 不在 unisacc 运行库,在 unisacc.com 的 APE sh 前奏:`u=$(uname -sm)` 与 `chmod 700 "$d"` 两个外部命令,sh 按 PATH 逐项探测(本机 PATH 前 31 项不含该命令,uname 与 chmod 各约 30 次失败后命中 /usr/bin);生成源 `tools/ape-pack.cx` 约 109 行 | 0.792 ms(3.3%) |
| arch_prctl 加 access ld.so.preload | EINVAL/ENOENT | 各 3 | 三个 sh 子进程(sh、uname、chmod)的动态装载 | 约 0.1 ms |
| wait4 WNOHANG | ECHILD | 5 | sh 自身 | 可忽略 |
| mkdir/open/symlink/memfd_create(旧标志)/ioctl | EEXIST/EINVAL/ENOTTY | 各 1 至 2 | unisacc 运行库:E3 缓存目录与 `.budget.lock`(正常竞争探测)、`host.cx` 先试 `MFD_CLOEXEC|0x10`(EINVAL 回退,内核 5.15 不认 MFD_NOEXEC_SEAL)、`runlaunch.cx` 约 164 行 symlink、isatty | 合计约 0.1 ms |

结论:135 次失败里 119 次(close 59、newfstatat 60)有两个独立来源,前者是 unisacc 自己的运行库(库包与运行库类),后者是 APE 头 sh 前奏(驱动与运行缓存类,生成物在 ape-pack)。两类合计 1.13 ms,占 strace 内 24.3 ms 的 4.7%;按 6 倍放大折合真实约 0.19 ms,上界很小,收益不在耗时而在系统调用条数(444 条里占 27%)。真正耗时在三次 fork+execve(sh、uname、chmod)与 gzip 外壳,不在失败调用本身。

最小修法(至多 3 条):
(1) close 循环改 `close_range(3, 62, 0)`(Linux 5.9+,一次系统调用),需保留位图中的 fd 时分段调用;ENOSYS 时退回现循环。写域 `src/host.cx` 一处,59 次降到 1 次,省约 58 次调用。类别:库包与运行库类。
(2) APE 头去掉 `uname -sm` 外部命令:Linux 路径先按 `$BASH_VERSINFO` 之外的纯 sh 手段难判架构,可改为按 `[ -x "$d/unisacc-$k" ]` 对两个已知 k 先后探测,命中即跳过 uname;`chmod 700 "$d"` 改为只在 `[ -O "$d" ]` 且权限位不是 700 时才调用(`[ -d "$d" ]` 已在前面做过,热路径目录必为 0700)。写域 `tools/ape-pack.cx` 头模板,省两次 fork+execve 与 60 次失败探测。类别:驱动与运行缓存类。注意 APE 头字节变动会改 unisacc.com 身份与 SID 以外的头校验,需重打包。
(3) 若不想动头:在 `memfd_create` 的 EINVAL 回退处缓存结果(进程内一次)已是现状,不再另改;留作不做。

没测到的:未在 PATH 短的环境对比 newfstatat 条数(PATH 越长越多,本机 31 项是用户环境,CI 上会少);真实(非 strace)耗时未单测。

未做与限度:U4 与单文件可执行文档页 404,B2 只有检索摘要;U3 与 U7 只读了前 10 万字符;没有读 Bun 源码;H2 无原文;[本次] 实测只有 R2 的总数一项且 n=1;T1 至 T6 没有任何修后数值。

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
| E12 封装与对接 | 允许裸 libc 与 include 的 dock 模块 89 个(docked=89,其中 module=1 的 84 个;依据 `build/dock-audit.tsv` 与 `tools/check-dock-audit.cx`,均 UNAUDITED),逐模块审计表已建但审计人未填 | `git grep -l '^#pragma unisa dock'` | dock 模块数只降不升;每个 dock 模块有一行审计(见 11.6) |
| E13 上限分叉 | `CXM_MAXU` 产品 128、权威 96;`CXM_MAXUSE 48` 三处各写 | `cx/compiler/cxmod.cx:28`、A7 第 14 项 | 三源常量差异 1 -> 0 |

### 10.2 设计提案

每条标:产品输入类别(cxmod 三源 | 运行库头 | 模型装载 | 非产品输入的工具与表)与版本。

**P1 export 重导出(cxmod 三源;0.0.65)**

- 语法:`#pragma unisa export NAME` 与 `#pragma unisa export NAME::sym`。只许出现在带 `module` 行的单元里;位置规则与 use 相同(首声明前、条件外)。
- 语义:(1) 对本模块等于 `use NAME`;(2) 直接 use 本模块的单元,视同也直接 use 了 NAME(给完整接口,而不是传递 use 的类型部分);(3) 符号身份仍属 NAME,函数体只有一份,不复制、不改名;(4) 链式重导出只沿显式 export 行传递,普通 use 仍不传递(CX-U-5 不变)。
- 冲突:使用方同时直接 use NAME 与经门面得到 NAME,是同一模块,按规范路径合并,不算冲突;两个不同模块导出同名仍报 `exported by both`(CX-N-1 不放宽)。
- 首版明确拒绝:`export NAME::sym as alias`(文案 `export alias unsupported`);非模块单元写 export(`export outside module`);export 自己(`module M exports itself`);export 形成环(沿用 `module cycle`)。
- 层向:export 算一条 use 边,门面的层号不得低于被导出模块(`tools/check-cx-layers.cx` 同口径)。
- 验收实验:`sh ./unisacc.com tests/cx-namespace.cx -- ROOT ROOT/unisacc.com OUT` 新增门面正例 2 条、拒绝 4 条;三源 `sh ./unisacc.com tools/cx0-extract.cx -- --check` EXACT。

**P2 通配 use 的窄语义(cxmod 三源;0.0.65,排在 P1 之后)**

- 语法:只有 `#pragma unisa use PFX.*` 一种;`*` 只能是最后一段且独占一段。`PFX.*::sym`、`PFX.*.x`、`*`、`use NAME::*` 一律具名拒绝。
- 语义:纯展开。把 PFX 解析成目录(旁边优先,其次包根),取该目录下直接子文件 `<seg>.cx` 中带 `#pragma unisa module <seg>` 的那些,按字节序展开成逐个 `use PFX.<seg>`;不递归子目录。展开后与手写完全等价:冲突仍拒、上限仍按 `CXM_MAXUSE` 计、空展开拒绝(`wildcard PFX.* matches no module`)。
- 限制:只许 L3(tools、tests、apps)使用;产品层(`src/`、`cx/`、`ext/`)出现即棘轮拒绝。理由:通配让闭包依赖目录列表,新增一个文件就改变所有通配使用方的编译输入,与「下游只依赖用到的面」相反。
- 代价如实:规划器要新增列目录回调(类比 `cxm_bind_real`),CX0BND1 束要记录目录列表身份;计划字节须把展开结果逐条写出,保证三宿主同字节。推荐做法仍是 P1 的显式门面,通配只是脚本便利。
- 验收实验:同上批次,正例 1 条(展开数与目录实数相等)、拒绝 5 条。

**P3 符号级导入的稳定语义(cxmod 三源;缺陷修复已并入 0.0.62,语义收敛在 0.0.65)**

- (1) 可见性:`use M::sym` 只给 sym 的原型与 M 的全部类型;类型永不改名。(2) 身份:一个模块在一个程序里只有一份函数体,与被多少单元、以多少种别名导入无关;别名是使用方单元的属性。(3) 同一单元对同一模块既整模块导入又符号导入:取并集,不报错;同一 sym 两个不同别名:拒绝。(4) sym 不在 M 的公开集:具名拒绝(`module M does not export sym`)。(5) 经 P1 门面得到的模块也可写 `use FACADE::sym`,解析到真实属主。
- 验收实验:`sh ./unisacc.com tests/module/check.cx`(批次 module-units)断言「N 种别名时单元数不变」;运行断言 UNVERIFIED-需新产品。

**P4 text.text 拆面(运行库头;0.0.65,依赖 P1。先行部分为非产品输入,立即可做)**

- 目标面(按 E7 实测的使用分布,而不是按文件大小):扫描面 `text.cmp`(tx_len tx_same tx_eq tx_eqz tx_cmp tx_prefix tx_prefixz tx_suffix)与 `text.find`(tx_find tx_rfind tx_chr tx_rchr tx_contains tx_count);切分 `text.split`(tx_fields tx_field tx_trim tx_trimc);构造面 `tx_copy` 归 `text.build`;格式化面即现有 `text.fmt`、`text.num`、`text.conv`,不动。`text.text` 保留为只含 export 行的门面,826 个使用方零改动,之后按需逐步改成只 use 用到的面。
- 为什么必须等 P1:没有 export 时,新模块与 text.text 同时定义同名函数会触发 `exported by both`,只能一次改完 826 个文件,违反分治。
- 先行(非产品输入):在只读副本里做拆面实验并用 `tools/affected.cx` 量化;预估见 10.4。

**P5 接口哈希驱动的选批与增量(非产品输入的工具与表;立即可做,不等版本)**

- (a) 窄化门按包判定:只要求被改文件所属包的 `layer-api` 与哈希是当前的,别的包过期不再关掉全局窄化(E5 实测的失效原因)。
- (b) 哈希粒度加一级:`build/iface-hash.tsv` 增加模块行(`pkg/module`),符号行已有 `sig_sha`(`build/layer-api/*.tsv` 第 3 列)。
- (c) 符号级选批:被改文件的改动行映射到文件作用域条目;若只落在若干导出函数体内,则只选「闭包内引用了这些函数(含同模块内调用它们的导出函数)」的批次;改动落在 static 数据、宏、类型或无法归属时退回整模块。未选中的批次打印为 DEFERRED_BY_SYMBOL,计数、可用 `--full` 重跑,不静默丢弃;release 档永远全选。
- 验收实验:`sh ./unisacc.com tools/affected.cx -- --files cx/text/text.cx --list-only --iface-narrow` 的 `IFACE_NARROW state=on`;符号级对 `tx_trimc` 的选批数远小于 903(预估见 10.4)。

**P6 前导分层(运行库头;不早于 0.0.65 末位,超 2 小时即移 0.0.67)与零解码(模型装载;0.0.63)**

- 层感知前导(CX-W-5):上层单元只见 L0 接口声明,不再每单元付 9388 行;与 0.0.63 的零解码互不依赖,分两个候选,各自用 `tools/cx-phase.cx` 冷热两组入账。
- 验收实验:`UNISACC_RUN_CACHE=0 UNISACC_RR_TIME=1 sh ./unisacc.com /tmp/h0.cx`(空 main),e2 相位下降量入 `build/phase-times.tsv`。

**P7 包内可见性(非产品输入的表先行;cxmod 三源在 0.0.65 之后)**

- 不加语法。先把 `cxpack.tsv` 扩到 11 个 cx.* 包(E2),公开集 = `build/layer-api` 的行;包内帮手不进 cxpack 即包外不可见。表与检查可立即做;规划器强制(包外调用未列入符号即拒绝)已有 `tests/cxpack/bad-call` 路径,扩面属产品输入。

版本映射汇总:0.0.62(库包与运行库)只并入 E1、E10 的缺陷修复与 P5、P7 表、P4 先行实验;0.0.63(模型与网)只做零解码;0.0.65(cx 设计)做 P1 -> P2 -> P3 收敛 -> P4 物理拆面 -> P6 前导(末位)。

### 10.3 与冻结层、三源、棘轮的冲突与兼容

- 冻结层:`cx/sys`、`cx/core` 只可新增文件(`tests/cx-module-freeze.tsv` 33 行,只缩不扩)。P1/P2 不改这两层的任何文件;P4 只动 `cx/text`(不在冻结集),新面是新增文件,`text.text` 改成门面属于改现有文件,须在 0.0.65 候选内一次完成并做字节等价回归(826 个使用方的 `-cx-plan` 单元集只增不减、程序输出不变)。
- 三源:每条语法改动的顺序固定为 `cx/cx0/plan.cx`(权威)-> `tools/cx0-extract.cx` 投影 `src/cxmod.c`(`--check` EXACT)-> `cx/compiler/cxmod.cx` 手工对齐并在 `tests/module/units.cx` 加同名断言。4367e1b0e5 证明手工对齐会漏(`cxm_deps_same` 晚了一轮);P1 起每个新拒绝文案都要同时出现在三份源里,用一条 `git grep -c` 断言守住,直到 55-1(cxmod.cx 改为投影)落地。归一方案与先后关系见 10.5:决定为先归一(D0)再加语法(D1 起),D0 落地后本条的「手工对齐」一步取消。
- 棘轮:srcshape 不许 `src/` 增行,P2 的列目录回调绑定须与 e576000457 同法(与现有语句同行或放 `cx/driver`);cxinclude、cxlibc、cxplatform 不受影响;通配的「产品层禁用」做成 cxapis 类棘轮的一条规则,不新开第 14 个棘轮。
- 一版一类:P1、P2、P3 同属 cxmod 三源,可同候选;P4 物理拆面属运行库头,与前者不同类,须是 0.0.65 内的第二个候选或顺延,不得与 P1 同候选。
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

**(3) 与 0.0.65 export、通配的先后:先归一(D0),再加语法(D1 起)**

- 依据一(仓内事实):手工对齐已漏过两次。`cxm_deps_same` 晚一轮(4367e1b0e5);57c7427f6f 的提交说明写「plan.cx 与 cxmod.cx 同构」,但同一提交加的 `call to undeclared function` 只进了产品源(计数 0、0、1)。P1 加 4 条拒绝、P2 加 5 条、P3 再加若干,每条三处同现,按已有漏率至少再漏一次。
- 依据二(成本):D0 是一次性机械变换,验收是逐字节(生成物等于现手写件,或差异逐行归到上面六类);D1 至 D3 是语义新增,验收是行为。先做 D0,D1 至 D3 每条只改 plan.cx 一处,`--check --all` 一条命令守两份生成物;反过来则 D0 要在更大的差异面上做,且 D1 至 D3 的手工对齐劳动全部作废。
- 依据三(候选规则):D0 与 D1 至 D3 同属 cxmod 三源一类(10.3 一版一类),可同候选,不增加整链次数(F9)。
- 类比,均为 [忆] 未核:
  - Python [忆]:导入机制只有一份实现 `importlib._bootstrap`,解释器内置的那份是由它冻结生成的产物,不是另写的 C 版本;早年 C 版 import 与 Python 版并存时行为不一致,后来收成单源再冻结。对应本方案:plan.cx 单源,产品件与宿主件都是冻结投影。
  - Node.js [忆]:`package.json` 的 `exports` 解析在 CJS 加载器与 ESM 加载器之间共用同一套解析函数,新增条件导出时两种加载器同时生效;没有为两套加载器各写一份 exports 解析。对应:先有共用解析核,再加 export 语法。
  - Rust [忆]:名字解析的权威实现在 rustc 的解析阶段;rust-analyzer 另有一份独立实现,新语法(如新版次的导入规则、宏相关可见性)在两者之间出现过滞后与结果不一致。对应:第二份手写实现在加语法期间必然滞后,即 F3。
- 反方理由与回应:归一动产品输入,会不会拖住 0.0.65?回应:D0 的今天可做部分(T1 至 T4)不动产品输入;动产品输入的部分(T5 至 T8)各自 15 分钟内,且排在 D1 之前同一候选。

**(4) 任务拆分(每项 15 分钟内,写域互不相交;简报按五段式另写)**

| 号 | 内容 | 写域 | 产品输入 | 今天可开工 | 验收命令与预期值 |
|---|---|---|---|---|---|
| T1 | 三源差异表:按函数对齐 plan.cx 与 cxmod.cx,输出函数、差异类(a 至 f)、是否语义(即 B1 的 54-1 `cx0-diff`) | `tools/cx0-diff.cx`、`tools/check-cx0-diff.cx`、`build/cx0-three-source-diff.tsv`、批次行 | 否 | 是 | `sh ./unisacc.com tools/check-cx0-diff.cx` rc=0;表中「是否语义=是」的行恰为 (e)(f) 两类,其余为 0;68 块全部归类,未归类 0 |
| T1 实测更正 | tools/cx0-diff.cx 已落(62901f0fc7,check 27 条 failed=0):plan.cx 对产品 68 块与上文相符;但 4 块落在六类之外(类 x),含三处 10.5 未列的语义分叉:cxm_load 的 `if (n == -3) return 1;` 只在 plan.cx;cxm_text 的 CXIF 防重复宏名后缀 `_sym` 只在 plan.cx;`cxm_rootn = n;` 只在产品。T5/T6 上移范围须含这三处;真实三源 `--check` 因此 rc=2 属有意 | - | - | - | - |
| T2 | MAXU GAP 实跑:生成 97 与 129 个单元夹具,对 cx0 与产品各跑一次,把 `:122` 的静态 GAP 改成实测 GAP | `tools/check-cxmod-tri.cx` | 否 | 是 | 末行仍 `failed=0 gaps=2`;GAP 行带实测 rc(预期 97 单元:cx0 rc=1 文案 `more than 96 units`,产品 rc=0;129 单元:两者均 rc=1) |
| T3 | 第二投影原型:用现有 `--output` 把产品目标生成到仓外,量与手写件的剩余差异 | 仅 `$CLAUDE_JOB_DIR/tmp`(不入库);结论原位更新本节 | 否 | 是 | `sh ./unisacc.com tools/cx0-extract.cx -- --output /tmp/cxmod-proto.c` 输出 `PREPARED`;`git diff --no-index --numstat /tmp/cxmod-proto.c cx/compiler/cxmod.cx` 与本节 +253 -79 相符,并给出去掉 (b)(e)(f) 后的剩余行数 |
| T3 实测 | 2026-11-14 已跑:`--output` 得 `PREPARED bytes=66794`;proto 对 `cx/compiler/cxmod.cx` numstat 为 +253 -79,33 块,与本节相符。逐块用 `cx0-diff --pair` 归类:(b) 3 块 -3 +9,(e) 5 块 -16 +197,(f) 3 块 -8 +3;去掉 (b)(e)(f) 后剩余 -52 +44(a 8 块、c 6 块、d 2 块、g 3 块、x 3 块,共 22 块)。(e) 一类即占新增 197 行,证实 checkcalls 上移前不可能全生成 | - | - | - | - |
| T1 落表 | `build/cx0-three-source-diff.tsv`(167 块:plan-vs-prod 68、plan-vs-c 66、c-vs-prod 33)由 `tools/cx0-diff.cx --out` 生成、`--verify` 校验;已在 `exec/facts/derive.tsv` 登记 `cx0-three-source`(order 8);`--check` 仍 rc=2(类 x 有意) | - | - | - | - |
| T4 | 棘轮核实:`src/cxmod.c` 增行是否触发 srcshape;`cx/compiler/cxmod.cx` 成为生成物是否触发 cxapis、cxlibc 等 | 只读;结论原位更新本节 UNVERIFIED 项 | 否 | 是 | `sh ./unisacc.com tools/ratchets.cx` 末行 13 项现值入账;给出 srcshape 对 `src/cxmod.c` 的判定源码行 |
| T5 | 上移 `cxm_checkcalls` 及其状态到 plan.cx,重生成 `src/cxmod.c`,tri 用例改普通断言 | `cx/cx0/plan.cx`、`src/cxmod.c`(生成)、`tools/check-cxmod-tri.cx` 第 121 行 | 是 | 否(等 0.0.62 新产品已装;T2 先完成,避免与其同文件) | `cx0-extract -- --check` 为 `EXACT`;`check-cxmod-tri` 末行 `passed=11 failed=0 gaps=1` |
| T5 已入库 [2026-11-14,产品输入,待下一轮整链验证] | 补丁已应用,cxmod.cx 补 `_sym` 与 n==-3;实跑:cx0-extract --check EXACT bytes=73271;check-cxmod-tri passed=15 failed=0 gaps=2(两个 gap 均为 MAXU 的 97/129 单元测量,计数口径较方案时增多);批次 module-units、cx-namespace、cx-host-run、cxmod-tri、layer-api、tlib、tlibpkg 均 PASS;bump cx.plan 0.1.2 -> 0.1.3、cx.compile 0.2.6 -> 0.2.7;ratchets ok=13 fail=0。UNVERIFIED-需新产品:cxmod.cx 的 `_sym`/-3 对齐字节效果 | - | - | - | - |
| T5 补丁方案已备 [2026-11-14,只读草稿,未改仓内源码] | 草稿 `$CLAUDE_JOB_DIR/tmp/t5.patch.txt`(plan.cx、src/cxmod.c、tools/check-cxmod-tri.cx,共 7 块,共 149 行增删),对 HEAD 的 `git archive` 解包副本 `git apply --check` 通过;在副本上用 `--source/--output` 重生成后 `cx0-extract -- --check` 为 `EXACT bytes=73271`(旧 66794 以后又增,以实跑为准)。(1) 产品源函数体:`cx/compiler/cxmod.cx` 第 1155 至 1292 行连续 138 行(cxm_declword、cxm_addloc、cxm_sees、cxm_owner、cxm_base、cxm_callerr、cxm_isskip、cxm_scanreg、cxm_checkcalls),状态 `CXM_LOCS`、`cxm_locn`、`cxm_locu`、`cxm_nloc`、`cxm_badcalls`(第 102 至 106 行);plan.cx 已有的依赖 cxm_tok、cxm_skipgrp、cxm_ismac、cxm_cpyn、cxm_idch、cxm_eqn、cxm_load、cxm_scan、cxm_start、cxm_fail、cxm_nuse/use/usym/ualias、cxm_sym 一族。(2) plan.cx 插入点:状态声明在第 123 行后(cxm_nmac 行与 cxm_tokb 之间);函数体在 cxm_collisions 结尾(第 1170 行)之后、`cxm_plan` 注释之前,函数前缀 `int` 改 `static int`;cxm_plan 内 `cxm_rootn = n;` 加在第 1175 行状态复位行后,`if (cxm_checkcalls()) { cxm_on = 0; return 1; }` 加在 cxm_collisions 调用(第 1227 行)后。(3) 三处同步分叉的取舍:cxm_load 的 `if (n == -3) return 1;` 引入于 70d1719cc1(2026-10-28,cx0 抽成私有模块时让宿主读失败 rc=2 上抛),以 plan 为准;`tools/cx0-extract.cx` 已对 src/cxmod.c 投影剔除该行,产品 `cxm_rdi` 回调不产生 -3,同步为死分支无害。cxm_text 的 CXIF 宏名 `_sym` 后缀引入于 972835632f(2026-11-01,同一模块多符号导入时每符号 CXIF 守卫唯一,否则第二次导入被 #ifndef 吞掉),以 plan 为准,产品源缺它是遗漏,属产品输入变更,须并入 cxmod.cx 才能让产品与 plan 同字节。`cxm_rootn = n;` 引入于 b393ac30bc(2026-10-31,-funit 只编命令行单元,cxm_count 读它),以产品为准,草稿只补声明与赋值进 plan.cx;cxm_funit、cxm_count、cxm_set_funit 仍只在产品(宿主不读),T5 后 cx0-diff 的 e 类尾块不会清零。草稿未含产品 cxmod.cx 的 `_sym` 与 -3 对齐(需新产品验证,见下)。(4) 验收命令序列(仓根逐条跑,每条 60 秒内):`sh ./unisacc.com tools/cx0-extract.cx -- --check` 预期 `EXACT`;`sh ./unisacc.com tools/check-cxmod-tri.cx` 预期末行 `passed=11 failed=0 gaps=1`(gaps 由 2 降为 1,passed 由 10 升为 11,仅剩 MAXU);`sh ./unisacc.com tests/run.cx -- --list` 取批次名后分别跑 `module-units`、`cx-namespace`、`cx-host-run`、`cxmod-tri`(经 `tools/affected.cx --files cx/cx0/plan.cx src/cxmod.c tools/check-cxmod-tri.cx --run`);`sh ./unisacc.com tools/layer-api.cx -- --bump` 预期 cx.plan 内容变而公开 API 不变(只加 static 内部,按 z 类补丁位升到 0.1.3),同步 `build/tlib-packages.tsv` 的 cx.plan 行与 `build/layer-api/cx.plan.tsv`,再跑批次 `layer-api`、`tlib`、`tlibpkg`;若产品 cxmod.cx 同提交补 `_sym`,cx.compile 同样 bump;`sh ./unisacc.com tools/cx0-diff.cx -- --check` 预期 e 类块减少 checkcalls 三块(状态、函数体、调用点)。(5) 需新产品才能验证:cxmod.cx 的 `_sym` 与 -3 对齐(产品输入);宿主 gcc_cx/clang_cx 路径新拒绝未导入符号调用对 `tests/cx-host-run.cx` 内联夹具的影响;`check-cxmod-tri` 在副本中因路径相对仓根跑出 rc=-1(13 项全 FAIL),属副本环境而非补丁判定,须在仓内应用后实跑,本轮 UNVERIFIED |
| T6 | MAXU 统一 128 并由 `limits.tsv` 注入,文案改拼数字 | `cx/cx0/plan.cx`(与 T5 串行)、`exec/facts/limits.tsv`、`src/cxmod.c`(生成) | 是 | 否(在 T5 之后) | `check-cxmod-tri` 末行 `passed=12 failed=0 gaps=0`(T2 的两个夹具转为普通断言);`grep -c 'CXM_MAXU' exec/facts/limits.tsv` 覆盖三个消费者(补丁草稿已备:`$CLAUDE_JOB_DIR/tmp/t6.patch.txt`,对 HEAD `git apply --check` 通过,未入仓未应用;git archive 副本实测:plan.cx MAXU 96 -> 128,两处文案改 `cxm_md(CXM_MAXU)`,funit 声明与注释并入 plan.cx 并删单独 cxm_rootn,limits.tsv 增 plan.cx 与 src/cxmod.c 两行 UNIT_MAX,check-cxmod-tri 夹具 97 改 cx0 期望 0、去 GAP;cx0-extract 重生成后 --check EXACT;check-cxmod-tri `passed=15 failed=0 gaps=0`;tests/module/check.cx MODULE_UNITS_CHECK PASS;tests/cx-namespace.cx checks=13 failed=0 unverified=5。UNVERIFIED:cx/compiler/cxmod.cx 无需改动(已 128);cxm_count/set_funit 取值器未并入 plan 宿主片段(归 T7);第 930 行注释 HEAD 的 plan.cx 已含;未在已安装产品上重建 src/cxmod.c 的 C 引导路径与 limits 全量 check-limits-gen)|
| T7 | 区标与投影表:plan.cx 加五对 `CX0_HOST` 区标,建 `cx/cx0/projection.tsv` 与 `cx/cx0/proj/` 片段,`cx0-extract` 改读表,`src/cxmod.c` 逐字节不变 | `cx/cx0/plan.cx`(与 T6 串行)、`cx/cx0/projection.tsv`、`cx/cx0/proj/`、`tools/cx0-extract.cx` | 是(区标是注释,投影后须从 C 目标剥掉,保证字节不变) | 否(在 T6 之后) | `cx0-extract -- --check` 的 sha256 与 T6 结束时相同;工具内不再有 `replace_one(body,"` 字面量(`grep -c` 为 0)(补丁草稿已备:`$CLAUDE_JOB_DIR/tmp/t7.patch.txt`,22 文件 +194 -22,对 HEAD `git apply --check` 通过,未入仓未应用。git archive 副本实测:plan.cx 体内加 9 对 `CX0_HOST_BEGIN/END`(head、bind、sha_out、sha_src、tmp_put、tmp_rewrite、tmp_multi、rd_err、samefile,比方案多 4 对,因 8 处 replace_one 锚点加两处头尾);`cx/cx0/projection.tsv` 三列(zone、c 片段、cx 片段,`@tail` 行为 cx 目标尾部),`cx/cx0/proj/` 19 个片段(c 版即原 replace_one 的替换文本,cx 版取自手写件);`tools/cx0-extract.cx` 删 replace_one 函数与 8 处字面量,新增 host_zones(区标成对、未闭合、嵌套、重复、表缺行、表有而体无均具名 rc=2)、`--all`(第二目标 `cx/compiler/cxmod.cx`,不带 --all 行为不变,--all 与 --output 互斥);classic_visibility 增 functions_only(cx 目标只剥函数的 static,变量保留)。实跑:不带 --all `--check` 为 `EXACT bytes=73271 sha256=462bde89...a59a`,与 HEAD 相同(src/cxmod.c 逐字节不变);`--all --check` 首行 EXACT、第二行 rc=2 `generated-projection-differs`(预期,手写件未替换,T8 才切);`--all` 生成的 cxmod.cx 对手写件 +26 -23,14 块(T3 原型为 +253 -79,33 块),与 HEAD 的 plan.cx 对手写件 72 块(e3 f3 x2)相比,生成件对手写件仅 14 块;check-cxmod-tri(副本内,需 unisacc.com 软链)`passed=15 failed=0 gaps=2`。剩余差异归类:(1) 5 个手写件保持 static 而生成为外部可见的函数(cxm_samefile、cxm_deps_same、cxm_rootbad、cxm_rootrow、cxm_rootfind),需每函数标记或 T8 接受可见性放宽;(2) funit 注释与 `cxm_funit` 声明、cxm_md(CXM_MAXU) 文案、符号导入注释两句、samefile 注释:属 T6 与 plan.cx 注释同步;(3) rewrite_multi 的 `char *td` 与 `p = cxm_cpy(out, td,...)` 两行;(4) cxm_count 等 8 个取值器已在 tail.cx。UNVERIFIED:区标对 `tests/module/check.cx`、cx-namespace、ratchets 的影响未跑;`--all` 生成件未用于实际编译;T6 补丁与本补丁的 plan.cx 合并顺序未试(head 区标包住 `#define CXM_MAXU 96`,T6 改 128 时须同步改 head.c/head.cx);checkcalls 取值器归属表第 1296 行 e 类仍靠 functions_only 可见性而非 export)|
| T8 | 产品目标切换:`--all` 生成 `cx/compiler/cxmod.cx`,删手写件的手工对齐义务,登记批次 | `cx/compiler/cxmod.cx`(生成)、`tools/check-cx0-extract-all.cx`、批次行、`tests/module/units.cx` 只增断言 | 是 | 否(在 T7 之后) | `sh ./unisacc.com tools/cx0-extract.cx -- --check --all` 两行均 `EXACT`、rc=0;`sh ./unisacc.com tools/affected.cx -- --files cx/compiler/cxmod.cx --run` 全绿;`sh ./unisacc.com tests/module/check.cx` 断言数不减 |

T6 归属表(N10,2026-11-14,只读取证;来源 `tools/cx0-diff.cx -- --pair plan.cx cxmod.cx` 实测 8 行 e/f/x 待判:6 行语义分叉加 2 行类 x 待核,不是 14 行;`build/cx0-three-source-diff.tsv` 另含 plan-vs-c 3 行、c-vs-prod 8 行,是同批分叉在 cxmod.cx 视角的重复投影,随下表同归属;未实跑修改,不记 PASS):

| plan 行 / prod 行 | 类 | 归属 | 依据 |
|---|---|---|---|
| 1-78 / 1-28 | f | 宿主片段 | 模块头(`#pragma unisa module/use`)与投影说明注释,只在 plan.cx 的 CX0 实际编译形态有意义,属 cx/cx0/proj/ 头片段 |
| 90 / 65-68 | e | 上移到 plan.cx | prod 有 `cxm_funit` 与 -funit 注释,plan 没有;引入提交 `git log -S cxm_funit`:b393ac30bc(-funit 只编命令行单元),权威源缺该能力 |
| 124 / (无) | x | 可删(已冗余) | plan 单独声明 `cxm_rootn`("宿主从不读"),上移 funit 后与 prod 同行 `cxm_funit; cxm_rootn` 合并,此行冗余 |
| 527 / 509 | f | 上移到 plan.cx | `cxm_n >= CXM_MAXU` 文案写死 "96",prod 为 `cxm_md(CXM_MAXU)`;即 T6 本体(MAXU 统一 128,文案拼数字) |
| 930 / 913 | x | 上移到 plan.cx | `cxm_text` 的符号级导入说明(SYM 非空只出该原型)只在 prod 注释;引入 e32df07ef4(`-S _sym`、符号级导入),plan 须同步,否则 D3 无权威注释 |
| 1296 / 1278 | e | 宿主片段 | `cxm_checkcalls` plan 为 static,prod 为外部可见;T5 已上移函数体(b2f2e45ef6),可见性是投影差,D1 export 落地后由 `export` 统一,此前留投影片段 |
| 1324 / 1306 | f | 上移到 plan.cx | `u >= CXM_MAXU` 文案写死 "96",同 527 行,属 T6 |
| 1460-1512 / 1442-1450 | e | 宿主片段(其中 `cxm_count` 的 funit 分支上移) | plan 为 CX0_COMPAT_BODY_END 后的 `cx0_*` 读写封装(read_all/invalid/execute,只宿主用);prod 为 `cxm_count/set_funit/unit_path/wants_*/set_on` 取值器,属 cx/cx0/proj/;`cxm_count` 返回 funit 时的 rootn,随第 90 行上移 |

对 D1 至 D3 的先后影响(上移项:90、527、930、1324 四行):(1) 90、527、1324 是 T6 与 funit 同批,须在 D1(export)之前落,否则 D1 的导出集在 plan.cx 与 prod 间仍差 funit 访问器与 MAXU 文案,三源 `--check` 无法收敛;(2) 930 是符号导入(D3)的前置,只需在 D3 前落,不挡 D1、D2;(3) D2(通配)不依赖任一上移行;(4) 1296 与 1460 的宿主片段在 D1 之后可再收缩为 `export` 标注,不阻塞 D1。排序:T6(90、527、1324)-> D1 -> D2 -> 930 -> D3。

- 并发关系:T1、T2、T3、T4 写域互不相交,可同时派;T5 至 T8 都写 plan.cx,串行。T5 至 T8 合计四个 15 分钟片,排在 D1 之前。
- 遍历消费者(限时与字面量改读表的铁律):T6 的消费者清单是 `cx/cx0/plan.cx`、`src/cxmod.c`、`cx/compiler/cxmod.cx`、`DV_MAXU` 的使用方(`grep -rn DV_MAXU src cx`)、`exec/facts/limits.tsv:2`、`tools/check-cxmod-tri.cx:122`;T8 的消费者是 15 个 `use compiler.cxmod` 的文件(`grep -rln 'compiler.cxmod' src cx tools tests`,其中 `src/` 6 个)。
- 未验证与风险:(1) 生成的产品件能否与现手写件逐字节相等:UNMEASURED,T3 给数;不等时以「差异逐行归入六类且 `tests/module/units.cx` 全绿」为准,不强求字节相等。(2) srcshape 对 `src/cxmod.c`:已核实不计(T4,见上;cxapis、cxlibc 对生成物 `cx/compiler/cxmod.cx` 的影响仍 UNVERIFIED,未跑 `tools/ratchets.cx`)。(3) MAXU 升到 128 后宿主静态内存增量:UNMEASURED。(4) `cxm_checkcalls` 上移后宿主侧语料新拒绝:T4 实测对 `tests/cxcc/units.tsv` 全部 10 个源与 `tests/cx-host-run.cx` 共 11 个文件逐个跑产品 `cxm_plan`(`sh ./unisacc.com tools/check-cxmod-tri.cx -- --prod FILE`):因 "call to undeclared function" 被拒 0 个;被拒 3 个均为表内 refuse 正例(cxcc_missing、cxcycle/ping、cxstd_unknown,原因分别为缺模块、环、未知 std 模块),其余 8 个 rc=0。产品源本就含 `cxm_checkcalls`,上移不改变这批结论。未覆盖:`tests/cx-host-run.cx` 内联字符串夹具(运行时才生成)与其它批次语料:UNVERIFIED。T5 验收仍须加跑批次 `cx-host-run`。(5) 本节全部为设计,T1 至 T8 均未开工。

### 10.6 cx.stream 与 tlib 重构(2026-11-14 设计;未实现,不得记 PASS)

口径:数字为 2026-11-14 在 HEAD 4966abbf9c、已装产品、lnx/x86_64 实测;行号以该提交为准。未核项标 UNVERIFIED 或 UNMEASURED 并给取数命令。

**(1) 现状**

| 号 | 事实 | 证据 |
|---|---|---|
| S1 包边界 | cx.stream 0.1.1 只含 `cx/stream` 三单元(item 46 行、pipe 319 行、rw 504 行),deps 为 cx.sys、cx.os、cx.mem、cx.text、ext.memx | `build/tlib-packages.tsv` cx.stream 行;`wc -l cx/stream/*.cx` |
| S2 导出面 | 已有显式公开集 30 名:item 4、pipe 9、rw 17;static 帮手不出包 | `cx/stream/cxpack.tsv`;定义行 `item.cx:18,24,30,43`、`pipe.cx:134,153,170,219,242,248,261,280,309`、`rw.cx:164,171,214,241,256,305,311,327,334,359,405,412,418,438,469,494,500`;`build/layer-api/cx.stream.tsv` 36 行(与 30 名的差为表头或类型行:UNVERIFIED,取数见 R0-1) |
| S3 依赖方 | 产品侧只有两个文件 use,且都只用 item 与 pipe:`cx/compiler/routepipe.cx:2-3`、`src/drivestream.cx:39-40`;rw 没有产品使用方,只有 `tests/stream` 与 `tools/check-cx-streamapi.cx` | `git grep -n '^#pragma unisa use stream\.' -- cx src ext` 共 4 行 |
| S4 波及面已分叉 | 改 pipe.cx 或 item.cx 选 126 批,改 rw.cx 选 1 批(scanned=1079):同包内已是两个互不依赖的面 | `sh ./unisacc.com tools/affected.cx -- --files cx/stream/rw.cx --list-only` 的 AFFECTED_SUMMARY |
| S5 名字三义 | "stream" 同时指 (a) `cx/stream` 的 xs_ 库,(b) `cx/compiler/stream` 与 `cx/runtime/stream` 各 7 个流单元(encode lex lowering opt parse preprocess prune,分属 cx.compile 与 cx.runtime 包),(c) 驱动的 `src/drivestream.cx` | `ls cx/compiler/stream cx/runtime/stream`;`build/tlib-packages.tsv` cx.compile、cx.runtime 行的 roots |
| S6 tlib 边界 | tlib 是工具侧机制:包表 `build/tlib-packages.tsv`(18 包),读表库 `tools/cxlib/tlibpkg.cx`(799 行),命令 `tools/tlib.cx`(1315 行),接口表 `tools/layer-api.cx`(451 行)生成 `build/layer-api/*.tsv`(18 个);发布链与 gen1/g2/g3 冷构造不读 tlib | 表头注释(`build/tlib-packages.tsv:1-6`);`exec/facts/derive.tsv:4`(layer-api 派生行) |
| S7 tlib 波及面 | 改 `tools/cxlib/tlibpkg.cx` 选 152 批;`tools/affected.cx:833` 起 `tools/tlib.cx kit`,`:1048` 起 `layer-api --check --pkg`,即选批器本身依赖 tlib | 同 S4 命令换文件 |

不干净的耦合:

- K1 合同倒挂:下层 `cx/stream/pipe.cx:15` 用上层 `src/stages.cx` 的 `st_span` 结果码定义自己的返回码 1 至 4;权威应在下层或表里,上层引用。
- K2 `st_span` 手抄原型:`src/driveroute.cx:68,225,378` 三处函数内 extern 原型,`src/drive.cx:441` 一处,定义在 `src/stages.cx:253`(13 个参数);`cx/runtime/unitcache.cx:392` 以函数指针接收同一签名。签名变一处须改五处,无模块接口守护。
- K3 重复实现:`pipe.cx:68-115` 自带 sha256(xs_rot、xs_block、xs_sha 与常量表),而包 deps 已含 cx.sys(`cx/sys/sha.cx` 在 cx.sys roots 内)。是否有意避开 kernel 哈希以保证跨目标同值:UNVERIFIED(取数:`git log --oneline -3 -- cx/stream/pipe.cx` 与 `git grep -n 'sys.sha' -- cx/stream`)。
- K4 类型擦除:stage 与 tap 以 `void *` 传 `xs_item *`,原因写在 pipe.cx 头注释(函数指针参数里的结构体类型不是已覆盖的导入接口声明形式,约 `pipe.cx:39-43`);这是规划器接口打印的缺口,不是流库的设计选择。
- K5 未实现面进了公开集:`xs_reader_deadline`、`xs_writer_deadline` 恒返回 -ENOSYS(`rw.cx:497,503`),却在 cxpack 白名单里。
- K6 最低版本漂移:cx.compile 的 deps 写 `cx.stream@0.1.0`,现版 0.1.1;18 包全部 deps 的"最低版本对现版本"差距未统计(UNMEASURED,取数见 R0-4)。
- K7 fwdmerge 与驱动:`cx/compiler/fwdmerge.cx:9` 的 FwmBuf 注明"与驱动布局一致",`src/driveroute.cx:108` 以强制转换传入,布局靠注释对齐;分配器经 `fwm_bind` 注入(`fwdmerge.cx:18`、`driveroute.cx:188`),这一半是干净的。改 fwdmerge.cx 选 123 批。
- K8 tlib 与 kernel:`tools/cxlib/tlibpkg.cx:86` 的块哈希取 sys.sha 的 kernel 哈希;包版本、api_sha 与 kernel 事实表之间是否还有别的读写关系:UNVERIFIED(取数见 R0-3)。任务书举例的"tlib 包版本与 kernel 事实表耦合"本轮未找到文件行证据,不作论断。

**(2) 目标结构**

参考系 [忆,未核]:Rust 的 `pub`/`pub(crate)` 与 crate 根重导出、Node `package.json` 的 `exports` 白名单、Python 的 `__all__` 与包 `__init__` 门面。取其三点:包根是唯一入口;公开集是显式白名单;包内可见与包外可见分开。

- 层与边界:cx.stream 拆成两个面,`stream.pipe`(含 item;有产品使用方,属产品输入)与 `stream.rw`(无产品使用方)。S4 证明两面已互不依赖,本条是把事实写进包表,不是新拆。包根门面 `stream`(只含 export 行)在 D1 之后才写得出。
- 公开集(保持 `cx/stream/cxpack.tsv` 为单一来源,`build/layer-api/cx.stream.tsv` 为派生):item 4 名、pipe 9 名、rw 15 名;两个 deadline 名移出白名单,等 cx.sys 非阻塞面就绪再加(加为 Y 位版本)。移出属破坏性变更(X 位),使用方数量未测(取数:`git grep -lw 'xs_reader_deadline\|xs_writer_deadline'`)。
- 内部:句柄表、槽位编码、sha 帮手、`xs_no` 一律 static;结果码 1 至 5 成为下层权威,`src/stages.cx` 引用(消 K1)。
- 改名消 S5:`cx/compiler/stream`、`cx/runtime/stream` 的目录名属另两个包,本重构不动;文档与批次名里以 "xs 库" 称 (a)、"七流" 称 (b)。是否改目录名另议,不在本节。
- 句柄生命周期合同(以现实现为准的成文,除标注外均有行号):
  - 句柄是 int,编码 `gen << 8 | kind << 5 | slot + 1`(`rw.cx:74`);表容量 XS_MAXRW 16(`rw.cx:52`)、XS_MAXPIPE 4 与 XS_MAXSTAGE 16(pipe.cx 头注释之后的 enum)。超限返回码 5 或负 errno,不截断(`pipe.cx:21-22`)。
  - 状态机:open -> (run 或读写)* -> close;close 后句柄恒被拒(`pipe.cx:38`),陈旧句柄靠 gen 位识别。gen 回绕后的行为:读码已定(`rw.cx` xs_rnew/xs_wnew 22 位掩码,`pipe.cx` xs_open 24 位),槽重开 4096 次 gen 逐次加 1 已由 `tools/check-cx-streamown.cx` 断言(批次 cx-streamown);回绕后旧句柄被接受:UNMEASURED(需 4194304 次重开,未跑)。
  - 重入:在运行中的 stage 内调用同一管线返回 4(`pipe.cx:20`)。
  - 所有权:管线运行后不保留字节,收据只存摘要与长度(`pipe.cx:39`);`xs_writer_close` 即使 flush 失败也释放句柄并返回首个错误(`rw.cx:41-42`);`xs_reader_mem` 借用调用方内存、`xs_writer_buf` 借用调用方 mg_buf。fd 的归属由内部 own 位决定(`rw.cx:130,147`),各入口取值:xs_reader_fd/xs_writer_fd 为 0(借用,关句柄不关 fd),xs_pipe_pair 为 1(拥有,关句柄即关 fd),已实跑断言(cx-streamown)。`xs_item_set` 接管 data,参数非法返回 4 时仍释放调用方的 data(KNOWN-RISK-DOUBLE-FREE-ON-ERROR:调用方若按"未转移"再释放即二次释放),现状已实跑钉住;stage 内重入 xs_run/xs_add/xs_close 均返回 4 已断言。
  - 未声称:无并发安全(模块级静态表);运行期句柄检查不等于编译期所有权(见第 4 节)。
- tlib 目标:tlibpkg.cx 按职责分三面(读表与校验、内容身份、冻结判定),使只用读表的工具不随身份算法重跑;纯工具改动,不进产品输入。分面前先量各面使用方(R0-5)。

**(3) 与 D0 至 D3 的依赖次序**

- 不依赖 D0 至 D3、可先行:公开集收窄(cxpack 表,规划器已强制,`tests/cxpack/bad-call` 路径)、K1/K3/K5 的下层修正、tlib 工具分面、全部 R0。
- 依赖 D1(export):包根门面 `stream` 与 tlibpkg 门面(零使用方改动的拆面)。D1 未落地时只做"不需要门面"的部分。
- 与 D2(通配)无关:产品层禁用通配(棘轮 cx-wildcard-product,4966abbf9c)。
- 与 D0 无写域交集:本重构不写 `cx/cx0/plan.cx`;K4 的根治(接口打印覆盖带结构体的函数指针参数)写 plan.cx,必须排在 D0 的 T8 之后,与 D1 至 D3 串行。

**(4) 分期**

| 期 | 内容 | 类别 | 版本 |
|---|---|---|---|
| R0 | 只读取数与文档(下表 7 项) | 纯工具/文档 | 今天 |
| R1 | tlibpkg 分面、包表登记 stream 两面、deps 最低版本检查 | 非产品输入的工具与表 | 不等版本 |

R1 状态(2026-11-14,已做,待提交后复核):包表增第十列 faces(FACE=UNIT),cx.stream 登记 pipe(item.cx、pipe.cx)与 rw(rw.cx),只是元数据,公开集仍以 cxpack.tsv 为准;tlibpkg 增 tp_face_of、tp_dep_ok、tp_deps_check(依赖按"现版本满足最低版本"判,不要求相等;tp_ver_compat 原本即是下限语义,tlib.cx 的依赖检查未用相等,漂移只是 R0-4 差距表的口径),并在头注释按表/身份/冻结三面标注函数归属(代码未物理拆分,等 D1 export 后做门面);layer-api set_row 保留第十列;check-tlibpkg 新增 7 断言(failed=0);tools.cxlib 0.1.26 -> 0.1.27。
| R2 | cx/stream 下层修正:K1 结果码归属、K3 去重复 sha、K5 白名单收窄 | 单类产品输入大改(库包源;routepipe 与 drivestream 是使用方) | 0.0.65 第二候选,与 D4 同类同候选;受 F9 门约束,不满足则 0.0.67。补丁草稿已备(未入仓未应用):`$CLAUDE_JOB_DIR/tmp/r2.patch.txt`(8 文件,对 HEAD `git apply --check` 通过)。副本(git archive HEAD)用已装产品实跑:check-cx-streamapi `ok=205 failed=0`(旧 208:deadline 段 11 条换成 8 条"不在公开名单且句柄不受影响"断言,净减 3,清单/覆盖表里的 208 要同步,未改);check-cx-streamown `ok=32 failed=0`(KNOWN-RISK 断言改为同强度反向断言 OWNERSHIP-ON-ERROR,xs_item_set 返回 4 不再 mg_free)。K1 实为 pipe.cx 注释里的出处说法,已改成 cx.stream 自有、仅数值与 st_span 相同;K3 未动。公开集变化:cxpack.tsv rw 面去掉 xs_reader_deadline/xs_writer_deadline,rw.cx 删两函数,layer-api 重生为 rows=32 bump=major。UNVERIFIED:build/tlib-packages.tsv 的 cx.stream 版本与 api_sha(d1a5dfff...)及内容 sha 未改(需正式 `layer-api --bump`,副本无 git);std-surface-v2.cx 里两行硬编码与派生的 build/std-surface.tsv、docs/cx/api/cx.stream.md 未重生;tests/manifests、coverage、batches 的 208 未改;routepipe.cx:70/122、drivestream.cx:411/438 三处调用 xs_item_set 未在返回 4 时释放(现实参恒合法,风险仅为理论泄漏,未改产品调用点);选批数 `affected --list-only` 129 批。 |
| R3 | K2 `st_span` 改为模块接口、K7 FwmBuf 共享类型 | 单类产品输入大改(驱动) | 0.0.67,不与 R2 同候选。T1 只读已测 2026-11-14(脚本 .cx 在 `$CLAUDE_JOB_DIR/tmp/w/`,不入库):(1) 五处 `int st_span(` 原型(stages.cx:253 定义;driveroute.cx:68/225/378 与 drive.cx:441 为声明)按空白归一后,四处声明逐字相同(sha 前 12 位 7d90adfa6af3,13 个无名形参,`extern` 只在 drive.cx),与定义的差只有形参名与行尾 `{`;形参类型序列完全一致,无签名漂移,故改签名需改 5 处是实情;(2) FwmBuf(fwdmerge.cx:9)= `{unsigned char *b; long long *at; int n, cap;}`,LP64 偏移 b=0、at=8、n=16、cap=20,大小 24;同布局另有 FwiBuf(fwdincrement.cx:8)、CxCoreBuffer(core/output.cx:5,int64_t)、Buf(core/execute.cx:16 与 sys/kernel.cx:15,I=int64_t),共 5 份各自定义;driveroute.cx:108 把 `void *` 强转 `FwmBuf *`,调用方实参为 FwiBuf(:213),两类型字段同序同宽,强转目前成立,但无编译期保证(按字面文本比对,未核各翻译单元实编尺寸);(3) 基线表 7 行(含表头共 8 行)`$CLAUDE_JOB_DIR/tmp/baseline-r3t1.tsv`,列 src、-o sha256 与字节、-S sha256 与字节,已装产品实跑 0 个 NA;M1 的 (a) 至 (e) 夹具找不到,改用 g-f0、g-f400 加 tests/cxcc_one.cx、all-aggregate.cx、bound-rc142.cx、cases/character_prefix.cx、cases/driver-default-platform.cx;未测:各 -o 的跨次是否同字节(只跑一次,非复现性证据) |
| R4 | K4 接口打印覆盖面、包根门面 | 单类产品输入大改(cxmod 三源) | D1 至 D3 之后 |

R0 任务(每项 15 分钟内,只读;各写各的 `$CLAUDE_JOB_DIR/tmp/R0-N.tsv`,结论由集成者原位填回本节):

| 号 | 目标 | 验收命令 | 预期值 |
|---|---|---|---|
| R0-1 | 核 cxpack 30 名与 layer-api 36 行的差集 | `sh ./unisacc.com tools/layer-api.cx -- --check --pkg cx.stream` | rc=0;差集逐行归类,函数行恰 30 |
| R0-2 | `st_span` 原型副本清点 | `git grep -c 'int st_span(' -- src` | driveroute.cx 3、drive.cx 1、stages.cx 1,合计 5 |
| R0-3 | tlib 对 kernel 的全部读点 | `git grep -n 'kernel' -- tools/tlib.cx tools/cxlib/tlibpkg.cx tools/layer-api.cx build/tlib-packages.tsv` | 至少含 `tlibpkg.cx:86`;每个命中标"读哈希/读表/无关" |
| R0-4 | 18 包 deps 最低版本对现版本差距表 | `sh ./unisacc.com tools/check-tlibpkg.cx` | rc=0;差距表至少含 cx.compile -> cx.stream 0.1.0 对 0.1.1 一行 |
| R0-5 | tlibpkg 各导出函数的使用文件数 | `sh ./unisacc.com tools/symindex.cx -- calls NAME --tracked`(逐导出名) | 每名一行;三面归属无交叉调用或列出交叉点 |
| R0-6 | 句柄合同四个 UNVERIFIED 项定值(gen 回绕、own 位、item_set 归属、deadline 使用方) | `sh ./unisacc.com tools/check-cx-streamapi.cx` | rc=0;四项各给文件行 |
| R0-7 | 18 包逐根选批数基线 | `sh ./unisacc.com tools/affected.cx -- --files FILE --list-only`(每包取一个代表文件) | pipe.cx 126、rw.cx 1、item.cx 126、tlibpkg.cx 152、fwdmerge.cx 123 复现(scanned 随批次表变) |

R0 实测(2026-11-14,HEAD 735a161e52,lnx/x86_64,已装产品;明细 TSV 在 `$CLAUDE_JOB_DIR/tmp/R0-N.tsv`,不入库;7 项中完成 5 项、部分 2 项):

- R0-1 完成:`sh ./unisacc.com tools/layer-api.cx -- --check --pkg cx.stream` rc=0,`LAYER_API cx.stream ok rows=34 units=3`。`build/layer-api/cx.stream.tsv` 36 行 = 2 行表头注释 + 34 数据行;34 行 = 30 个函数行(与 cxpack 30 名一致,差集为空)+ 1 行 typedef `xs_item` + 3 行枚举 `enum:XS_MAXPIPE`、`enum:XS_MAXRW`、`enum:XS_KR`(各带 XS_MAXSTAGE、XS_BUFDEF/XS_CHUNK、XS_KW)。S2 的 UNVERIFIED 解除。
- R0-2 完成:`git grep -c 'int st_span(' -- src` = drive.cx 1、driveroute.cx 3、stages.cx 1,合计 5,与预期一致。
- R0-3 部分:`git grep -n 'kernel' -- tools/tlib.cx tools/cxlib/tlibpkg.cx tools/layer-api.cx build/tlib-packages.tsv` 仅 2 行命中:`tlibpkg.cx:86`(读哈希:sys.sha 的 kernel 块哈希,注释)与 `build/tlib-packages.tsv:9`(cx.sys 行 roots 里的路径,读表,无关);tools/tlib.cx 与 tools/layer-api.cx 零命中。只查了字面词 kernel,未穷举 sha 调用点:K8 的"别的读写关系"仍 UNVERIFIED。
- R0-4 完成:`sh ./unisacc.com tools/check-tlibpkg.cx` rc=0,`CHECK_TLIBPKG failed=0`(该检查本身不输出差距表;差距表是读 `build/tlib-packages.tsv` 的 deps 列与版本列对拍得到,只读取数)。18 包 deps 共 44 条,最低版本不等于现版本的 42 条,仅 2 条相等;含预期行 `cx.compile -> cx.stream 0.1.0 对 0.1.1`。除 `ext.net -> cx.sys 0.1.3 对 0.1.9` 外,其余全是 0.1.0 对现版本。K6 定值:漂移是常态,最低版本检查应判"现版本是否满足最低版本",而不是相等。
- R0-5 完成(用 `git grep -lw` 取数,没用 symindex;统计 tools 与 tests 下除 tlibpkg.cx 自身的文件数):tp_put 5、tp_puts 5、tp_sha 5、tp_sha_file 4、tp_ver_parse 2、tp_ver_compat 2、tp_load 6、tp_find_opaque 2、tp_prep_forbidden 3、tp_names 5、tp_find_unit 5、tp_find_excl 5、tp_member_name 2、tp_closure 3、tp_api 2、tp_ar_next/canon/find 各 2、tp_rc_get 3、tp_is_frozen 1、tp_cache_dir 1、tp_unit_key 2、tp_kit_key 3、tp_kit_dir_for 1、tp_kit_dir 10(含 tests 下 tsv 的文本引用,偏高)。交叉点:身份面(tp_sha*、tp_unit_key、tp_kit_*、tp_cache_dir)与读表面(tp_load、tp_names、tp_find_*)被 tools/tlib.cx、layer-api.cx、tlib-cover.cx 同时调用;batch-packages-gen.cx 只用读表面加 tp_put 与 tp_sha 之外的输出帮手。三面归属的划分本轮未做,只给使用方分布。
- R0-6 部分:`sh ./unisacc.com tools/check-cx-streamapi.cx` rc=0,`CX_STREAMAPI ok=208 failed=0`;四项定值见"合同四项"。
- R0-7 部分(只复现本节引用的 5 个文件,18 包逐根未做,UNMEASURED):`affected.cx --files F --list-only` 的 AFFECTED_SUMMARY:pipe.cx batches=126、rw.cx 1、item.cx 126、tlibpkg.cx 152、fwdmerge.cx 123,scanned=1079,与基线完全一致。
- K3 定值:pipe.cx 私有 sha256 是有意的。引入提交 2afa486d68(0.0.58 cx.stream 核心包);`pipe.cx:91-92` 注释写明 L0 sys.sha 是 Linux 内核套接字调用,而收据必须在每个目标上同值。`git grep -n 'sys.sha' -- cx/stream` 无命中。后果:R2 的"K3 去重复 sha"不能直接改用 sys.sha,前提是 osx/win 上有同值的实现,否则 K3 应改判为"保留,并把理由写进接口文档"。
- K5 使用方数:`git grep -lw 'xs_reader_deadline\|xs_writer_deadline'` 共 11 个文件:实现与导出 3(`cx/stream/rw.cx`、`cx/stream/cxpack.tsv`、`build/layer-api/cx.stream.tsv`);派生表与文档 4(`build/std-surface.tsv`、`docs/cx/api/cx.stream.md`、`docs/cx/design-core.md`、`prd_cx.md`);测试与工具 4(`tools/check-cx-streamapi.cx`、`tests/manifests/cx-streamapi.tsv`、`tests/coverage.tsv`、`tools/std-surface-v2.cx`)。`src/`、`cx/compiler`、`cx/driver` 零使用方:移出白名单不破坏产品调用点,只需同步测试与派生表。
- K7 FwmBuf:`fwdmerge.cx:9` 的 `FwmBuf` 是 `{unsigned char *b; long long *at; int n, cap;}`;`driveroute.cx:108` 把 `const void *` 强转为 `const FwmBuf *`,该文件内没有同名结构定义(只见强转行),驱动侧传入对象的真实类型本轮未追到,布局一致性无编译期守护:风险成立,但未做布局字节对拍(UNMEASURED)。
- 合同四项:(1) gen 回绕:`rw.cx` 的 xs_rnew/xs_wnew 用 `(gen + 1) & 0x3fffff`(22 位),`gen << 8` 共 30 位不溢出 int;同一槽重开 4194304 次后 gen 回到 0,此时一个 4194304 代之前的旧句柄会被接受(陈旧句柄检测的最小周期),不是崩溃。(2) own 位:`xs_reader_fd`、`xs_writer_fd` 传 0(借用,关闭时不 close,`rw.cx:168,331`);`xs_pipe_pair` 对两端传 1(`rw.cx:477,479`);`xs_reader_close`、`xs_writer_close` 据 own 调 `sy_close`(`rw.cx:318,426`)。(3) `xs_item_set` 接管 data:是。`item.cx:30-42` 先 `xs_item_drop(it)`,再把 `{data,len,cap}` 直接装入 `it->buf`,不复制;参数非法(返回 4)时执行 `mg_free(&b)`,即也会释放调用方传入的 data,调用方须按"传入即交出"约定。(4) deadline 使用方:见上 K5 使用方数。

**(5) 度量**

- 已测(S4、S7):同包两面 126 对 1,即 rw 面的改动与 pipe 面的 125 批可并发、互不重跑;这是"面独立即批次独立"的现成证据,与 10.4 的 text.text find 族 769 -> 预计 385 同口径。
- 待测:R2 去掉私有 sha 后 pipe.cx 的选批数(预计不变,因闭包由使用方决定):UNMEASURED;tlibpkg 分面后只读表工具的选批数(现 152):UNMEASURED,由 R0-5 的使用方分布按 10.4 同法估算;R3 后改 `st_span` 签名需改文件数 5 -> 1:未实现。
- 取数命令统一为 `sh ./unisacc.com tools/affected.cx -- --files FILE --list-only [--iface-narrow --symbol-narrow SYM]`,读 AFFECTED_SUMMARY 的 batches 与 scanned。

### 10.7 export 语法与可见性合同(2026-11-14 设计;未实现,不得记 PASS;即 0.0.65 的 D1)

结论:`export` 分两种形状,首版(D1a)只做「本模块自有函数的公开名单」,门面重导出(D1b)排其后。有 export 的模块以源码内名单为准,`cxpack.tsv` 的公开名列在迁移期只作校验,终态改为由 export 生成;没有 export 的模块行为完全不变。本节未跑任何命令,所有行为论断来自读源码,新产品上的行为一律 UNVERIFIED。

**现状证据(读源码,HEAD 6c2b8e4776)**

- 导出集现在怎么推导:`cxm_project`(`cx/cx0/plan.cx:833`)先给每个文件级项分类,`typedef` 为 1(`:841`)、`static` 为 0(`:842`)、带函数体的非 static 为 2(`:843`)、无名字的 struct/union/enum 为 1(`:845`)、其余声明为 3(`:846`)。类 1(类型)无条件写进接口文本;类 2(函数)再过 `cxm_listed`(`:915`),返回 0 就跳过(`:917`),既不写原型也不 `cxm_addsym`。
- `cxm_listed`(`:778`)的三值:同目录没有 `cxpack.tsv` 或表里没有本文件的行,返回 2(全部非 static 函数公开);有行且名字在第三列,返回 1;有行而名字不在,返回 0。所以「公开名单」今天已经存在,只是写在包表里,不在源码里。
- 公开名单单一来源:`cx/stream/cxpack.tsv` 三行共 30 名(item 4、pipe 9、rw 17);仓内带 `cxpack.tsv` 的目录只有 `cx/driver`、`cx/stream` 与 `tests/cxpack/` 五个夹具(`git ls-files '*cxpack.tsv'`)。
- 指令解析:`cxm_directive`(`:570`)只认 `use`、`module`、`dock`,其余落到 `unknown unisa pragma`(`:623`);`export` 今天因此被拒,`tests/cx-namespace.cx:100`、`:102` 的 `planned-export-refused-by-name`、`planned-export-alias-refused-by-name` 断言的就是这条文案。位置约束已有:`unisa pragma after first declaration`(`:585`)、`unisa pragma inside conditional`(`:586`),export 自动继承。
- CX-U-2 已有拒绝:符号级导入点名未导出符号,`cxm_usechk`(`:1113`)发 `module M does not export SYM (used by U)`(`:1123`);调用未导入的函数,`cxm_callerr` 发 `call to undeclared function NAME (declared in M, which U does not use)`(`:1242`)。T5 之后三源都有这两条(`cx/compiler/cxmod.cx:1105`、`:1224`;`src/cxmod.c:1082`、`:1201`)。
- 接口表:`tools/layer-api.cx` 读产品自己的 `-cx-interface` 输出(头注释第 6 行),不读 `cxpack.tsv`;R0-1 实测 cx.stream 为 34 行(30 函数、1 typedef、3 枚举),函数行与 cxpack 30 名差集为空(10.6 R0 实测)。
- 与 `prd_cx_spec.md` 3.5 的冲突:3.5 把 `export NAME` 定为「重导出模块 NAME」;本任务书的 D1 是「公开名单 `export NAME[,NAME...]`」。同一写法不能两义,下面 (1) 给出取舍,3.5 须随 E0 改写(不在本节写域)。

**(1) 语法与 cxpack 的关系**

```
#pragma unisa export NAME[,NAME...]          D1a:NAME 是本单元定义的非 static 函数名;逗号分隔,逗号两侧可有空格;可写多行,取并集
#pragma unisa export use MODULE[::sym]       D1b:门面重导出(即 3.5 原 export NAME 的语义),排在 D1a 之后
```

- 取舍:裸名一律是自有符号,重导出必须带 `use` 关键字。依据:单段模块名(如 `rw`)与函数名在词法上无法区分,靠查表消歧要求在指令期就知道本单元的全部定义,而 `cxm_scan` 是先处理指令再收项(`:630` 的注释);带关键字则指令期即可定形。
- 操作数只许标识符(`cxm_idch`),不许 `*`、`.`、`::`、`as`。export 必须出现在本单元的 `#pragma unisa module` 之后。
- 只管函数。类型(类 1)首版仍无条件公开:类型不产生链接符号,且隐藏类型会让已公开原型不可写。名单里点类型名即拒。
- 导出集规则(替换 `:915` 至 `:917` 的判定,顺序即优先级):
  - (a) 单元有至少一条 export:公开集就是 export 名单。此时若 `cxm_listed` 对本文件有行(返回 0 或 1 的情形),两边集合必须相等,不等即拒;没有行(返回 2)则不校验。
  - (b) 单元没有 export:行为与今天逐字节相同(有 cxpack 行按行,否则全部非 static 函数公开)。这就是向后兼容规则,不设「默认变私有」的切换日。
- cxpack 的去向分三期:迁移期(0.0.65)export 与 cxpack 并存且互相校验;一个包的全部单元都写了 export 之后,该包 `cxpack.tsv` 第三列改为生成物(事实 export 行 -> 生成器带 `--check` -> 表),name 与 path 两列保留给包工具;「无 export 即全公开」只对产品层收紧,用棘轮计数(只减不增),不改规划器默认。
- 上限:每单元 export 名数上限登记到 `exec/facts/limits.tsv`,文案拼数字(T6 的做法),不写死字面量。

**(2) 错误模型(文案即断言子串;rc 一律 1)**

D0 的 T8 落地后三源只改 `cx/cx0/plan.cx` 一处,另两源是生成物;下表「落点」列按 plan.cx / cxmod.cx / src/cxmod.c 给现有函数的行,供 T8 未落地时核对投影。

| 编号 | 触发 | 文案 | 落点函数(三源行) |
|---|---|---|---|
| X1 | `export` 后无名字 | `unisa export takes a name` | `cxm_directive` 的 pragma 分支(`:623` / `:606` / `:579` 之前新增分支) |
| X2 | 非标识符、`*`、空项(连续或结尾逗号) | `bad export name "TEXT"` | 同上 |
| X3 | 单元无 `module` 行或 export 写在它之前 | `export outside module` | 同上 |
| X4 | 同一名字出现两次(同行或跨行) | `module M exports NAME twice` | 同上(登记名单时查重) |
| X5 | 名单超上限 | `module M has more than N exports` | 同上 |
| X6 | 名字在本单元无定义 | `module M exports NAME but does not define it` | `cxm_project`(`:833` / `:898` 附近 / `:871` 附近),类 2 循环结束后逐名核对 |
| X7 | 名字是 static 函数 | `module M exports NAME; it is static` | 同上 |
| X8 | 名字是类型或对象 | `module M exports NAME; only functions are listed` | 同上 |
| X9 | export 与 cxpack 行集合不等 | `module M export list differs from cxpack.tsv: NAME` | 同上,紧接 `cxm_listed` 调用处 |
| X10 | `use M::sym` 点名未导出符号 | 沿用 `module M does not export SYM`(不改字) | `cxm_usechk`(`:1123` / `:1105` / `:1082`) |
| X11 | 调用 M 的未导出函数 | `call to undeclared function NAME (private to M)` | `cxm_callerr`(`:1242` / `:1224` / `:1201`) |
| X12 | `export use M::sym as alias` | `export alias unsupported`(3.5 既有) | `cxm_directive`,D1b |
| X13 | `export use` 自己、成环 | `exports itself`、`module cycle`(3.5 既有) | `cxm_adduse`(`:495`),D1b |

- 既有的导出项检查不变且只对公开项生效:`under #if (conditional interface)`、`uses macro`、`prefix is reserved for the prelude`(`cxm_exportok`,`:689`)。
- 冲突规则:`use M::sym` 与 M 的 export 冲突就是 X10,不另立文案;`export NAME` 的 NAME 同时是本单元经 `use X::NAME as NAME` 引入的别名,按 X6 拒(别名不是本单元的定义)。
- X11 与现有文案共用前缀 `call to undeclared function NAME`,`tests/cx-namespace.cx:124` 的子串断言不受影响。

**(3) 可见性与内存安全**

- 不 export 的函数不进接口文本(现有 `:917` 的路径),使用方看不到原型。
- 要补的一处 [E1 已测 2026-11-14,已装产品与 cx0 两源]:今天未公开函数没有 `cxm_addsym`(`:917` 直接 continue),`cxm_checkcalls` **不能**把对它的调用归到所属模块。读码:归属查找 `cxm_owner`(`cx/cx0/plan.cx:1224`)只认 `cxm_symk == 0`(模块公开)或 `2`(非模块单元函数);可见性 `cxm_sees`(`:1203`)只认 `cxm_symk == 0`;调用点 `:1285` 要求 `cxm_owner` 命中才报错。未列入 cxpack 的 f 从未登记,`cxm_owner` 返回 0,不报错。实测夹具(目录 `$CLAUDE_JOB_DIR/tmp/e1fx`):`cxpack.tsv` 为 `a<TAB>a.cx<TAB>g`;`a.cx` 为 `#pragma unisa module a` + `int f(void){return 1;}` + `int g(void){return 2;}`;`m1.cx` 为 `#pragma unisa use a` + `int main(void){return f();}`(m2:`use a::g` 后调用 `f()+g()`)。命令 `sh ./unisacc.com tools/check-cxmod-tri.cx -- --cx0 FILE` 与 `-- --prod FILE`:m1、m2 两源均 `RC=0`,无任何文案(f 被静默放过,落到编译器隐式声明)。对照(目录 c,无 cxpack 行,f、g 皆公开,m.cx 为 `use a::g` 后调用 f):两源均 `RC=1`,`call to undeclared function f (declared in a, which m.cx does not use)`,具名且指向 a。结论:不能归属。缺口:(1) 未公开函数无 `cxm_sym` 条目,须按设计新增种类值(如 3)登记,不写原型;(2) `cxm_owner` 须认新种类以得到 owner,`cxm_sees` 与 `cxm_usechk`、`cxm_collisions` 的 `symk == 0/1/2` 匹配须明确排除它;(3) `cxm_callerr` 须据种类发 X11 文案 `(private to M)`;(4) 另一缺口:未登记也使同名跨模块冲突(`cxm_collisions`)看不到私有名,与 X11 一并定。以上缺口即 E4 至 E6 的前置,仍是设计,未实现。设计要求:未公开的类 2 函数以新的种类值登记进 `cxm_sym`(不写原型、不参与 `cxm_usechk` 的 `cxm_symk == 0` 匹配),使调用走 `cxm_callerr` 同一路径发 X11,而不是落到编译器的隐式声明。
- 与 CX-U-2 的关系:CX-U-2 管「使用方选了哪些名字」(导入侧收窄),export 管「模块给出哪些名字」(导出侧收窄);两者落在同一张 `cxm_sym` 表和同一个检查函数上,export 只是让该表少登记公开项,不新增第二条检查链。
- 不承诺的性质:export 是规划期的名字可见性,不是链接期隔离;未公开函数仍是外部链接符号,手写 `extern` 原型或 `.c` 单元(不被扫描,`src/cxmod.c:18` 注释)可以绕过。链接期隐藏(投影成 static 或改名)不在 D1 范围,不得据 D1 宣称模块私有状态已受保护。

**(4) 迁移试点:cx.stream 的 rw 面**

- 选它的依据:17 名已有显式名单(`cx/stream/cxpack.tsv` 第 4 行);无产品使用方(10.6 第 (2) 段),仓内使用方只有 `tools/check-cx-streamown.cx`、`tools/check-cx-streamapi.cx`;改 `rw.cx` 只选 1 批(R0-7)。`ext.net.resolve` 没有 cxpack 行可对拍,放第二个。
- 做法:在 `cx/stream/rw.cx` 的 module 行后加 export 行,17 名与 cxpack 行逐名相同;不改任何使用方。
- 验收命令与预期值(全部需装有 D1 的新产品;旧产品上预期 `unknown unisa pragma export`,记 UNVERIFIED needs-new-product):
  - `sh ./unisacc.com tools/layer-api.cx -- --check --pkg cx.stream`:rc=0,`LAYER_API cx.stream ok rows=34 units=3`(与 R0-1 相同,接口字节不变)。
  - `sh ./unisacc.com tools/check-cx-streamapi.cx`:rc=0,`CX_STREAMAPI ok=208 failed=0`。
  - `sh ./unisacc.com tools/affected.cx -- --files cx/stream/rw.cx --run`:选 1 批,PASS。
  - 反向一例(仓外副本):从 export 行删掉 `xs_copy`,规划 rc=1,文案 `module rw export list differs from cxpack.tsv: xs_copy`。
  - `sh ./unisacc.com tools/cx0-extract.cx -- --check --all`:两行 `EXACT`。

**(5) 次序关系**

- D0:T6(MAXU 与文案拼数字)、T7(区标与投影表)、T8(产品件改生成)必须先于 D1;X5 的上限文案依赖 T6 的读表做法,十三条新文案依赖 T8 才能只改一处。T8 未落地则 D1 的实现任务(E4 起)不开工,夹具与设计任务不受限。
- 通配棘轮 `cx-wildcard-product`:无依赖。export 名单不接受 `*`(X2),不会产生新的通配形态;棘轮规则不用改。D2(`use PFX.*`)依赖 D1b 而不是 D1a。
- layer-api:接口表读 `-cx-interface`,export 只在公开集变化时才动它。试点要求集合相等,所以 D1a 不触发 layer-api 重生;R4(接口打印覆盖面与包根门面)排在 D1b 之后,包根门面 `stream` 用的是 `export use`。
- 类别:D1a、D1b 都是 cxmod 三源类产品输入,与 D0、D2、D3 同候选、同写 plan.cx、串行。

**(6) 子任务(每项 15 分钟内;同列写域互不相交者可并发)**

| 编号 | 内容 | 写域 | 产品输入 | 类别 | 今天可做 | 验收命令与预期值 |
|---|---|---|---|---|---|---|
| E0 | 改写 `prd_cx_spec.md` 3.5 的 export 两行与表,对齐本节 (1) | `prd_cx_spec.md` | 否 | 文档 | 是 | `grep -c 'export use' prd_cx_spec.md` 不小于 2;3.5 表内 export 行的现行为仍写 `unknown unisa pragma export` |
| E1 | 只读:读完 `cxm_checkcalls` 的归属查找,定 (3) 的 UNVERIFIED;清点仓内「有 cxpack 行」与「无行」的模块数 | 仅 `$CLAUDE_JOB_DIR/tmp`,结论原位填回本节 | 否 | 只读 | 是 | 给出查找所用的 `cxm_symk` 值与文件行;`git ls-files '*cxpack.tsv'` 为 7 个文件 |
| E2 | 拒绝夹具 X1 至 X11 各一例加正例两例(有 cxpack 行、无行) | `tests/export/` 新目录 | 否 | 测试夹具 | 是 | `ls tests/export` 为 13 个子目录;每个拒绝夹具带一行预期文案文件 |
| E3 | 检查器:对每个夹具跑规划,断言 rc 与文案子串;旧产品命中 `unknown unisa pragma export` 时打印 UNVERIFIED needs-new-product,不记 PASS | `tools/check-export.cx`、批次行(`tools/register.cx`)、`prd.md` 能力速查一行 | 否 | 工具 | 是(依赖 E2) | 旧产品:rc 非 0,末行 `CHECK_EXPORT passed=0 failed=0 unverified=13`;新产品:`passed=13 failed=0 unverified=0` |
| E4 | 指令解析与名单表(X1 至 X5),上限入 `limits.tsv` | `cx/cx0/plan.cx`(`cxm_directive`)、`exec/facts/limits.tsv`、两个生成物 | 是 | cxmod 三源 | 否(等 T8) | `tools/cx0-extract.cx -- --check --all` 两行 `EXACT`;`check-cxmod-tri` 末行 `failed=0 gaps=0` |
| E5 | 导出集判定与 X6 至 X9 | `cx/cx0/plan.cx`(`cxm_project`),与 E4 串行 | 是 | cxmod 三源 | 否 | 同 E4;`tests/cxpack.cx --fixtures` 仍 `rc0:CXPACK` |
| E6 | 未公开函数登记与 X11 | `cx/cx0/plan.cx`(`cxm_project`、`cxm_callerr`),与 E5 串行 | 是 | cxmod 三源 | 否 | 同 E4;`tests/cx-namespace.cx` 的 `unimported-symbol-call-refused` 仍过 |
| E7 | `tests/cx-namespace.cx` 的 `planned-export-refused-by-name` 换成语义断言(不删) | `tests/cx-namespace.cx` | 否 | 测试 | 否(等 E4) | 该批断言总数不减;门面正例 2、拒绝 4 留给 D1b |
| E8 | 试点:`cx/stream/rw.cx` 加 export 行 | `cx/stream/rw.cx` | 是 | 库包源 | 否(等新产品装好) | 本节 (4) 的五条 |
| E9 | cxpack 第三列生成器与 `--check` | `tools/cxpack-gen.cx`、`tools/check-cxpack-gen.cx`、批次行 | 否 | 工具 | 否(等 E8) | `--check` rc=0;生成结果与现 `cx/stream/cxpack.tsv` 第 4 行逐字节相同 |
| E10 | D1b:`export use` 解析、X12、X13、门面正例 | `cx/cx0/plan.cx`,与 E6 串行 | 是 | cxmod 三源 | 否 | `tests/cx-namespace.cx` 门面正例 2、拒绝 4;`--check --all` 两行 `EXACT` |

- 并发:E0、E1、E2 互不相交,今天可同时派;E3 接 E2。E4、E5、E6、E10 同写 plan.cx,串行。E8 属库包源类,与 cxmod 三源类不同类:随 D4 的第二候选,或等 D1 候选的产品装好后作下一版首个提交,不与 E4 至 E6 同候选。
- 新工具名 `tools/check-export.cx`、`tools/cxpack-gen.cx` 开工前先跑 `git log --all --oneline -- tools/NAME` 核对未被占用(本节未核)。
- 回滚:E4 至 E6、E10 各自一个提交,撤提交即回到「export 被 `unknown unisa pragma` 拒绝」的现状;E8 撤一行即回到 cxpack 单独生效。

### 10.8 sy_res_live 与 np_exec 设计(2026-11-14 设计;未实现,不得记 PASS;0.0.63 库包源类)

输入身份:HEAD 4006fe4cd3 只读读码,源码未改,未实跑任何新代码;行号均取自该提交。`build/std-surface.tsv` 现余 `new` 两行:449(`long sy_res_live(int kind);`,三目标 `refuse:ENOSYS 本版未实现`,错误列 `-EINVAL`,note「句柄型资源存活数」)与 126(`int np_exec(const sy_spawn_t *o, mg_buf *out, mg_buf *err, int64_t cap, sy_exit_t *st);`,错误列 `-errno;-EMSGSIZE`,note「带界捕获;err 为 0 时并入 out;任一路超过 cap 记 -EMSGSIZE 并杀子进程」)。缺口行 `build/pkg-gaps.tsv` 40(B2f-RES)、41(B2f-NPEXEC),均 OPEN。

(1) sy_res_live 语义与计数点

- 决策:返回「经登记点登记、尚未注销」的句柄数,不是操作系统真值。kind 枚举放新文件:`SY_RES_FD 1`、`SY_RES_PROC 2`、`SY_RES_MAP 3`;未知 kind 返回 `-EINVAL`(与表 449 错误列一致)。计数器是模块私有 static long 数组,无系统调用,因此三目标可同码;表 449 三列由 `refuse:ENOSYS` 改 `impl` 须等各目标实跑,osx/win 先写 `unverified`(词表先例:表 124 行 win 列)。
- 冻结约束的处理:`cx/sys`、`cx/core` 只可新增文件(本文 10.3,:812)。新增 `cx/sys/reslive.cx`(模块 `reslive`),先例是 `cx/sys/sigwatch.cx`(提交 9cdbf210ab,新增文件且未进 `tests/cx-module-freeze.tsv`;该表 33 行,cx/sys 下只列既有模块如 :31 `cx/sys/proc.cx`)。新文件导出两个名字:`sy_res_live(kind)` 与登记点 `sy_res_note(int kind, int delta)`(delta 只许 +1/-1,计数将为负时返回 `-EINVAL` 且不改值,这是重复关闭的确定性信号)。`sy_res_note` 是新增表面,须在 std-surface 补一行(新行,不是改旧行)。
- 最小侵入:0.0.63 不改任何现有 `sy_*`(改现有文件即违反冻结)。不做包装层(`sy_open` 等同名包装会与现有定义冲突,换名包装等于再造一套 fd 面)。接入只发生在冻结集之外的上层:`np_exec`(本节 (2))对 PROC 与两个管道 fd 成对登记;`cx/stream` 的 reader/writer 开闭(`rw.cx`,10.6 句柄合同)可在 0.0.65 的 R2 接入。现有 `sy_*` 自身接入排到冻结解除之后(T5)。
- 因此本版计数不准的范围(据实,以下开闭全部不经过登记点):
  - fd 开:`sy_open` `cx/sys/fs.cx:57`、`sy_open_new` :84、`sy_open_read` :90、`sy_dup` `cx/sys/fd.cx:32`、`sy_dup2` :36、`sy_pipe` `cx/sys/proc.cx:171`、`sy_socketpair` `cx/sys/posix.cx:79`、`sy_dir_open` `cx/sys/posix.cx:258`、`sy_sock_open` `cx/sys/netraw.cx:168`、`sy_sock_accept` :186、`sy_tcp_listen` `cx/sys/net.cx:32`、`sy_tcp_accept` :57、`sy_tcp_dial` :202、`sy_tcp_listen_at` :501、`sy_tcp_accept_from` :511、`sy_udp_bind` :657、`sy_unix_listen` :699、`sy_unix_connect` :716、`sy_fork_session` 的两根管道 `cx/sys/proc.cx:140-141`、`sy_proc_spawn` 交回的父端 `cx/sys/proc.cx:334`。
  - fd 闭:`sy_close` `cx/sys/fs.cx:249`、`sy_dir_close` `cx/sys/posix.cx:319`、`sy_proc_close` `cx/sys/proc.cx:395`;另外 cx/sys 内除这三个名字所在行之外还有 60 处裸 `close(`(grep 计数,未逐处归类)。
  - 进程:`sy_spawn` `cx/sys/proc.cx:79`、`sy_spawn_inherit` :102、`sy_fork_session` :136、`sy_proc_spawn` :293、`sy_spawn_pipe` :426;回收 `syp_reap` :353、`sy_wait_status` :447、`sy_wait` :471。`np_run` 直接调 libc `pipe`/`fork`/`waitpid`(`ext/sysx/process.cx:54-56`、:101),同样不计。
  - 映射:`sy_map_none` `cx/sys/mem.cx:72`、`sy_unmap` :79;`sy_dlopen` `cx/sys/dlcall.cx:45` 未归入任何 kind。
  - 未逐一核:`sigwatch.cx`、`tty.cx`、`runlaunch.cx`、`netnb.cx`、`netmsg.cx`、`dl.cx`(`syd_openat` :73)内部的开闭。
- 结论口径:0.0.63 的 `sy_res_live` 只对「自己登记的调用方」成立,发布说明必须写「不是进程级 fd/子进程/映射总数」;不得用它声称无泄漏。真值路线(lnx 扫 `/proc/self/fd`)本版不做:它本身要开一个目录 fd,且 osx/win 无对应,另列缺口。

(2) np_exec 签名与合同

- 签名照表 126 不改。入参约定:`o` 只读,函数内复制一份再改;`out` 必须已 `mg_init`(`cx/mem/grow.cx:36`),只追加不清空;`err` 为 0 时置 `SY_SPAWN_ERR_TO_OUT`(`cx/sys/proc.cx:190`、:303)并入 out;`cap` 是每一路新增字节上限,`cap < 0` 返回 `-EINVAL`;`st` 必填。副本强制 `out_fd = SY_STDIO_PIPE`,`err` 非 0 时 `err_fd = SY_STDIO_PIPE`;`in_fd == SY_STDIO_PIPE` 返回 `-EINVAL`(无人喂,同 `sy_proc_run` 的拒绝理由 `proc.cx:409`);`pgroup == 0` 时改 -1 使杀进程覆盖整组(`sy_proc_kill` 按组发信号,`proc.cx:345-351`)。
- 时限:取 `o->deadline_ms`(`proc.cx:200`),0 为无时限。副本把 `deadline_ms` 清 0、由 np_exec 自己持钟(`sy_now_ms`),原因是 `sy_proc_wait` 到点直接 SIGKILL(`proc.cx:382`),交给它就做不成先 TERM 后 KILL。超时返回 0 且 `st->timed_out = 1`(与 `sy_proc_wait` :390 同口径,`sy_exit_code` 给 124,:232);超 cap 返回 `-EMSGSIZE`,该路缓冲恰好追加到 cap 为止,`st` 仍填子进程终态。
- 循环:`sy_poll`(`cx/sys/poll.cx:51`,最多 `SY_POLL_MAX` 项,EINTR 自续)等两个父端,每轮超时取 min(剩余时限, 50 ms);就绪后 `sy_read`(`cx/sys/fs.cx:147`)读入 16 KiB 栈缓冲再 `mg_append`(`grow.cx:72`);读到 0 关该端并从轮询集移除。两端都 EOF 后 `sy_proc_wait` 收尸。不设 O_NONBLOCK(poll 就绪后单次 read 不阻塞)。对比 `np_run` 是非阻塞读加 `usleep` 2 ms 忙等(`ext/sysx/process.cx:89-90`、:134-137)。
- 僵尸回收:`sy_proc_spawn` 成功之后的每一条出口都先 `sy_proc_wait`(无时限的阻塞 `syp_reap`,`proc.cx:388`)再 `sy_proc_close`(:395);spawn 失败时 L0 已关管道并回收(`proc.cx:305-309`、:324-329)。
- 已知边界:子进程退出但孙进程握着管道写端时,无时限调用会一直等 EOF(与 Rust `Command::output` 同);`setsid` 脱组的后代不在组杀范围,本接口无 residue 出参,不调 `sy_kill_tree`(`cx/sys/process.cx:149`),写进发布说明。
- osx/win:np_exec 全部经 `sy_*` 写成,自身不写平台 `#if`(架构规则);表 126 现写三目标 `impl` 且 win 写 PeekNamedPipe 轮询,与此设计不符,落地时改为 lnx `impl`、osx/win `unverified`,L0 在该目标返回的具名拒绝原样上传。`cx/sys/refusals.cx` 属冻结文件不能加行,有名拒绝登记缺口与发布说明 0.0.62 已知缺口 (25) 同类。`sy_proc_spawn`/`sy_poll` 在 osx/win 的真实行为本节未核,UNVERIFIED。
- 与 np_run 的关系:不能复用 np_run 的函数体,理由三条:无 cap;扩容失败直接 `exit(1)`(`ext/sysx/process.cx:19`);入参与结果是 `np_result` 加 Python 风格 why 文案(:27-44、:120-127)。可复用的是 L0:`sy_proc_spawn`/`sy_proc_kill`/`sy_proc_try_wait`/`sy_proc_wait`/`sy_proc_close`(`proc.cx:293-403`),np_exec 新写的只有轮询循环。反向收编(np_run 改成 np_exec 的薄封装)本版不做:np_run 现有三份(`cx/sys/process.cx:48` 属冻结且与 `ext/sysx/process.cx:48` 经 `diff -q` 同字节,`tools/cxlib/netpack.cx:789` 第三份),且其超时走 `sy_kill_tree` 给 residue(:113-115),合同不同;排 0.0.65。
- 放哪个包:决策为新文件 `ext/sysx/exec.cx`(模块 `exec`),不建 `ext/procx` 目录。依据:`ext/procx` 不存在;表里 `ext.procx` 是逻辑包名,np_run 行 124 已是「现在 ext.sysx」的先例;`build/tlib-packages.tsv:15` 的 ext.sysx 以整目录 `ext/sysx` 入包且已依赖 `cx.sys;cx.os;cx.mem`,新文件自动入包。tlib 影响:ext.sysx 版本 0.1.2 升一档并重算内容哈希、`build/layer-api/ext.sysx.tsv` 加一行、`build/iface-hash.tsv` 对应行变、批次 `tlib-ext-sysx`(`tests/batches.tsv:963`)须重跑;新建包则要多一行包表、一个 layer-api 文件与一个 tlib 批次,本版不值。`cx/sys/reslive.cx` 归哪个包(cx.sys 0.1.10 按文件清单入包,`tlib-packages.tsv:10`)须在包表文件清单加一项并升版。

(3) 内存安全

- 缓冲所有权:`out`/`err` 始终归调用者;np_exec 不 `mg_free`、不 `mg_take`。任何返回(含 `-ENOMEM`、`-EMSGSIZE`、超时)之后缓冲都处于合法状态,已追加字节保留,由调用者 `mg_free`(`grow.cx:101`)。扩容走 `mg_growto`(`grow.cx:43-57`,倍增,`-EOVERFLOW`/`-ENOMEM`,失败时原指针不动),「零写占槽、覆盖前扩容」由 mg 层承担,np_exec 不直接碰 `data`。
- cap 判定在追加之前:本次读到 k 字节,若 `新增量 + k > cap` 只追加 `cap - 新增量` 字节,置超限标记后走杀进程路径,缓冲增长上界为 cap 加一次倍增余量。
- 失败路径统一出口(单一 `fail:` 段):`mg_append` 失败、`sy_poll`/`sy_read` 返回非 EINTR 错误、超 cap、超时,全部走「TERM -> 宽限 -> KILL -> 阻塞 wait -> close」,再返回首个错误码。
- 信号:`sy_proc_kill(&c, SIGTERM)`,随后在宽限期内每 5 ms `sy_proc_try_wait`(`proc.cx:367`)并继续排空管道(防子进程写满管道而收不到退出);宽限到点 `sy_proc_kill(&c, SIGKILL)`,再 `sy_proc_wait` 阻塞收尸(KILL 后必返回)。宽限值取命名常量 200 ms,若仓内限时须读表(子代理规则「限时改读表」),实现者先查 `tools/cxlib/limits.cx` 是否已有对应键,没有就在事实表加行,不写裸字面量。`sy_proc_kill` 返回 `-ESRCH` 视为已退出。
- 不泄漏 fd:父端只存在于 `sy_child_t` 的 `out_fd`/`err_fd`(L0 已置 CLOEXEC,`proc.cx:331-333`);EOF 时逐端关并置 -1,出口再 `sy_proc_close` 关余下的(该函数对 -1 幂等,`proc.cx:397-400`)。检查用例以 `/proc/self/fd` 条目数前后相等为断言(lnx 本机),并用 `sy_res_live` 前后相等作第二断言。

(4) 实现子任务(各 15 分钟内,写域互不相交;T1、T2 可并行,T3 接两者)

| 项 | 内容 | 写域 | 产品输入类别 | 验收命令与预期值 |
| --- | --- | --- | --- | --- |
| T1 | `sy_res_live`、`sy_res_note` 与 kind 枚举;检查含:三 kind 初值 0、+1/-1 配对回 0、未知 kind `-EINVAL`、负计数拒绝且值不变、delta 非正负一拒绝 | `cx/sys/reslive.cx`(新)、`tools/check-sys-reslive.cx`(新) | 是,库包源类(`cx/` 在源身份目录集内,本文 :235) | `sh ./unisacc.com tools/check-sys-reslive.cx`:rc=0,末行 `failed=0`,断言不少于 8 项 |
| T2 | `np_exec`;检查含:正常捕获两路字节相等、err 为 0 并入 out、超 cap 返回 `-EMSGSIZE` 且该路长度恰为 cap、超时 `timed_out=1` 且总耗时小于时限加 1 秒、忽略 TERM 的子进程被 KILL 回收、不存在的程序 `-ENOENT`、`in_fd` 为 PIPE `-EINVAL`、每例前后 `/proc/self/fd` 条目数相等且无僵尸 | `ext/sysx/exec.cx`(新)、`tools/check-ext-procx-exec.cx`(新) | 库包源类;`ext/` 不在源身份目录集(本文 :235,本次未重核) | `sh ./unisacc.com tools/check-ext-procx-exec.cx`:rc=0,`failed=0`,断言不少于 12 项,单次运行 60 秒内 |
| T3 | 登记:两个检查入批次(`tools/register.cx`);std-surface 行 449、126 `new -> exists`,目标列按实跑改(lnx `impl`,osx/win `unverified`),补 `sy_res_note` 行;layer-api 重算(cx.sys、ext.sysx);iface-hash;tlib-packages 两包升版;pkg-gaps 40、41 改 CLOSED | `tests/batches.tsv`、`tests/coverage.tsv`、`build/std-surface.tsv`、`build/layer-api/cx.sys.tsv`、`build/layer-api/ext.sysx.tsv`、`build/iface-hash.tsv`、`build/tlib-packages.tsv`、`build/pkg-gaps.tsv`(共享表用 `tools/commit-own.cx --keyed`) | 表与生成物,随 T1/T2 同候选 | `grep -c '	new	' build/std-surface.tsv` 由 2 变 0;`sh ./unisacc.com tools/check-std-surface-v2.cx`、`tools/check-layer-api.cx`、`tools/check-iface-hash.cx`、`tools/check-tlib-frozen.cx` 各 rc=0;`sh ./unisacc.com tools/ratchets.cx` 输出 `ok=13 fail=0` |
| T4 | 新产品装后复验与 osx/win 证据:已装产品实编两新文件;CI 上两检查在 osx/win 的结果如实回填表列 | 只读加 std-surface 两行目标列 | 否 | `sh ./unisacc.com tools/affected.cx --files cx/sys/reslive.cx ext/sysx/exec.cx --run` rc=0;osx/win 无证据则保留 `unverified` |
| T5 | 冻结解除后:现有 `sy_*` 开闭接入 `sy_res_note`(本节 (1) 清单),np_run 三份收编为 np_exec 薄封装 | `cx/sys/fs.cx`、`fd.cx`、`proc.cx`、`mem.cx`、`posix.cx`、`net.cx`、`netraw.cx`;`ext/sysx/process.cx` | 是,需冻结解除流程,不进 0.0.63 | 0.0.65 开工前另拆 |

- 新名占用已核:`git log --all --oneline -- tools/check-sys-reslive.cx tools/check-ext-procx-exec.cx cx/sys/reslive.cx ext/sysx/exec.cx` 无输出,四个名字均未被用过。
- 未核项:`tools/check-std-surface*.cx` 是否校验包名列与文件目录一致(若校验,行 126 的 `ext.procx` 须按 np_run 行 124 的先例处理);cx.sys 新文件对 `tools/check-cx-layers.cx` 五条断言的影响;`sy_res_note` 作为新导出是否触发 `tests/coreapi.cx` 棘轮。T1/T3 开工先跑对应检查定值。
- 回滚:T1、T2 各是纯新增文件,撤提交即回到两行 `new`;T3 用 `tools/derive.cx --write`(失败自还原)或撤提交。

## 10.x examples/ 梳理(2026-11-14,只读梳理,未改 examples/;已装 0.0.62)

范围:`git ls-files examples` 共 64 个文件(apps 18、cx-lab 11、win 19、run-speed 6、https 1、根下 9)。引用计数口径:`git grep -l -F 路径 -- tests tools build docs 'prd*.md' ARCHITECTURE.md`(按完整路径计;括号内 base 为仅按文件名的粗计,含同名误中,不作证据)。`examples/apps/csih` 在本仓不存在,csih 车道无关。取证命令:`for f in examples/*.c examples/cx-lab/*.cx; do timeout 20 sh ./unisacc.com $f </dev/null; done`。

### (1)(2) 清单与实跑

| 范围(文件数) | 是什么 | 引用计数(路径) | 引用点 | 实跑 0.0.62 |
|---|---|---|---|---|
| 根 do/fact/fib/hello/host/indirect6/ptr/struct/switch .c(9) | C99 最小例:do-while、递归、指针、结构体、switch、间接调用、host 判别 | 各 1;hello 2 | `tools/check-cx-entry.cx`(全部 9 个);hello 另被 `docs/toolchain.md` | 全部 rc=0:do 输出 3、fact 120、fib 55、hello "from hello.c"、host "host: linux"、indirect6 15/16、ptr 7、struct 9、switch 6 |
| cx-lab/cxlab.cx(1) | cx 模块 `#pragma unisa module cxlab`,TSV 装载与命令中位数的共享帮手 | 3 | `tests/cxraw.tsv`、`tests/cxlibc.tsv` 及一处 | 单独跑 rc=1 "not covered: no main"(模块无 main,预期;只能经 `use cxlab` 被引) |
| cx-lab/lex_table.cx(1) | 表驱动词法器(charclass.tsv + transition.tsv -> tokens.tsv) | 3 | 同上两棘轮表等 | rc=0 "lex_table: ok tokens=88 classes=9 rules=29" |
| cx-lab/time_run.cx(1) | 对 `./unisacc.com -run` 若干命令计中位耗时,写 times.tsv 到 $CXLAB_OUT(默认 $TMPDIR/cx-lab-out) | 2 | cxraw/cxlibc 表 | rc=0 "time_run: wrote <dir>/times.tsv"(已整改:不再改写仓内文件) |
| cx-lab/runtimeiocheck.cx(1) | 运行库 IO 字节对照检查 | 4 | cxraw/cxlibc/批次相关 | rc=1 "runtimeiocheck: FAIL":`tests/runtimeiobytes.c` 不存在(cc1 fatal),native 构建失败。确定性缺陷:依赖的测试文件已缺 |
| cx-lab 其余数据(8):charclass.tsv、transition.tsv、sample.c、out/{.gitignore,runtimeio.tsv,times.tsv,tokens.tsv} | 词法表、样本、产物 | 多数 0 | sample.c 与 times.tsv 仅文件名误中 | 不适用(数据与产物;out/*.tsv 是已跟踪的生成物,与 out/ 只放产物的规则相容但不应入库) |
| apps(18):README + 17 个 .c(args bf calc colorpack ctour dijkstra exeinfo life mandel memmap procview queens sieve winlayout winlist wordfreq xgui) | 较大的 C99/POSIX 程序;procview/memmap/exeinfo 用 /proc 或 libproc;winlayout/winlist/xgui 用 macOS CoreGraphics FFI 或 X11 | 全 .c 为 0(xgui 另有 1 处文件名误中);README 2 | README 被 `docs/readme-detail.md`、`docs/sweep/md-*/lanes-and-rest.tsv` 引 | 未跑(任务限根下;apps 需平台或较大) |
| run-speed(6):empty headers headers_once include_only printf_hi .c + times.tsv | 启动耗时基准的空程序与头文件程序 | 路径 0 | 仅被 `cx-lab/time_run.cx` 内部硬编码路径(empty、printf_hi)使用;headers/headers_once/include_only 无任何使用 | 未跑 |
| https/post.c(1) | 经系统 libcurl 的 HTTPS POST(`-lcurl`,0.0.28 N1) | 1 | 一处文档/表 | 需网络,只列不跑 |
| win(19):README、顶层 10 个 .c(clockfmt envdir fileio mailbox outparam posix procsnap structs tick vmem)、gui 4(capture dll input window)、refused 4(argceil createfile7 shm wideargs) | Windows 11 上的实测与修复记录(README 自述"不是示例");refused/ 是被拒绝合同的最小复现 | 全 .c 为 0;README 2 | README 与 apps/README 被同批文档引 | 需 Windows,只列不跑 |

实跑汇总:根下 9 个 C 与 3 个可独立运行的 .cx 中,12 个里 11 个能编能跑(cxlab.cx 为模块,runtimeiocheck.cx 失败)。注意 shell 管道 `| head` 会给出 rc=141(SIGPIPE),上表 rc 已用重定向重测。

被引用计数为 0 的文件(路径口径):apps 全部 17 个 .c;run-speed 的 headers、headers_once、include_only、printf_hi(empty 与 printf_hi 被 time_run 的字符串内路径用,git grep -F 路径可能因相对写法漏计,按"弱引用"处理);win 全部 .c(18 个,含 gui 与 refused)与 README 之外的;cx-lab 的 charclass.tsv、transition.tsv、sample.c、out/*.tsv(数据,被 cx 程序按路径字符串使用,不是死文件)。

### (3) 规则核对(cx 系列 4 个 .cx 对照 prd_cx_spec.md 与 CLAUDE.md)

- 无手写 `#include`、无平台 `#if`;cxlab/lex_table/time_run 用 `#pragma unisa module/use cxlab`,合规。
- 直接 libc:cxlab.cx 用 `strchr`、`free`、`snprintf`;lex_table.cx 用 `printf/exit/strcmp/isalpha/fprintf` 等(约 50 处,`tests/cxlibc.tsv` 已把计数登记为棘轮,cxlib 53、cxraw 37);time_run.cx 用 `fopen/fprintf/printf`(18 处,同表)。依现行规则,examples 属 `tools/tests` 外的教学材料,既不是产品代码也不是工具,按产品入口教学的标准应改为 `sy_*/mm_*/tx_*`;现状由棘轮冻结计数而非清零。
- C99 示例本就是 C,不算违规。

### (4) 与 spec/docs 的重复与过期

- spec `prd_cx_spec.md` 与 `docs/cx/` 已有用 `use` 的小例子和替代 python/bun/node 对照;cx-lab 的"表驱动词法器"与"中位数计时"在 spec 里无同价物,有教学价值,保留。
- `docs/cx/api/ext.net.md` 列了 `sy_tcp_listen_at` 等 ext.net 接口,但 examples 里没有任何 cx 的 ext.net 示例;现有唯一联网示例是 https/post.c(C 加 libcurl),与 cx 方向不一致。
- win/README 自称"实测记录不是示例",且 18 个 .c 零引用:是记录而非教学,应归档候选。
- cx-lab/runtimeiocheck.cx 已因依赖文件缺失而失效,且 4 处引用(棘轮表)会让简单搬走触发棘轮;需先改表。

### (5) 建议动作表

| 对象 | 动作 | 理由 | 风险 |
|---|---|---|---|
| 根 9 个 C99 示例 | 保留;新增 1 个冒烟批次:对每个文件 `sh ./unisacc.com FILE` 比对期望输出(do 3、fact 120、fib 55、hello、host 为 host: linux 仅本机、indirect6 15/16、ptr 7、struct 9、switch 6),用 `tools/register.cx` 登记 | 全部实跑通过,现仅被 check-cx-entry 间接引用,没有断言输出 | host.c 输出随平台变,断言只在 linux 期望 linux |
| cx-lab/runtimeiocheck.cx | 修复(恢复 tests/runtimeiobytes.c 的来源,或改为内联字节)优先于归档;若确认该合同已迁到别处,再归档并同步棘轮表 | rc=1 是确定性缺陷,不得以"负载"定性;先读 runtimeiocheck.cx 报 FAIL 的源码行与上游文件的 git log | 删/移会破 cxraw/cxlibc 行数登记,须同步 |
| cx-lab 其余 3 个 .cx | 保留;新增 `examples/cx-lab/README.md`(入口文档:3 个脚本、如何 `use cxlab`、运行命令与期望输出);lex_table/time_run 的 libc 调用逐步改 `sy_*/tx_*` | 无入口文档,是教学缺口;time_run 会弄脏已跟踪的 out/times.tsv | 改 libc 调用要同步 cxlibc.tsv 计数(只降不升) |
| cx-lab 三个已跟踪产物 tsv | 已 git mv 到 archive/examples-cx-lab-out/ 并登记 ARCHIVE.tsv;示例写出位置改为 $CXLAB_OUT | 产物不应入库;跑一次就变更 | 已无活引用 |
| run-speed headers/headers_once/include_only | 先确认有无外部使用,无则列归档候选(`git mv` 到 `archive/examples-run-speed/` 并在 `archive/ARCHIVE.tsv` 加原路径行,原路径不得再被 tools tests src cx prd.md 引用) | 零引用 | 时间基准是历史量,归档后 time_run 仍用 empty 与 printf_hi,保留这两个 |
| apps 17 个 .c | 保留不归档;挑 3 至 4 个无平台依赖者(calc、queens、sieve、bf、wordfreq、dijkstra、life)登记冒烟批次 | 纯 C99/POSIX,零引用但有教学价值;procview/memmap/exeinfo 依赖 /proc 或 libproc,winlayout/winlist/xgui 依赖 macOS 或 X11 FFI,留作平台批次不入冒烟 | 未逐个实跑,先跑再登记,输出须稳定(mandel、life 视输出而定) |
| win 19 个文件 | 先不动;若归档,README 的两处文档引用(readme-detail、sweep 表)要先改,gui 与 refused 的拒绝合同要先确认已有等价批次 | 零引用的 18 个 .c 是实测记录,价值在历史证据 | 误归档会丢 Windows 回归线索;refused/ 可能是未被其它批次覆盖的拒绝合同 |
| https/post.c | 保留,标需网络;不进冒烟 | 唯一 libcurl 示例 | 无 |

### examples 作为产品入口教学材料缺什么

- 缺 cx 模块 `use` 示例的最小入口(cx-lab 有 cxlab,但无 README、也无 `sy_*` 写法);
- 缺 ext.net 示例:可用 0.0.62 新封网接口 udp/unix/tcp 的截止时间版写一个回环示例,须自带期限且只绑回环;
- 缺 `sy_*/mm_*/tx_*` 写法的 hello 级示例(现有 cx 示例均仍用 printf);
- 缺"从 C99 到 cx 的对照"小例(同一个 fib 或 wordfreq 的 .c 与 .cx 并列);
- 缺把示例和 spec 例子交叉引用的索引(examples/README 不存在)。

### 梳理发现与整改状态:cx-lab 示例写仓内 out/(已整改)

原状:cx-lab 的 out/ 目录里有三个已跟踪产物 tsv,示例运行会改写它们。整改:(1) 三个 tsv 已 git mv 到 `archive/examples-cx-lab-out/` 并追加 `archive/ARCHIVE.tsv`,无活引用;(2) time_run、lex_table、runtimeiocheck 的写出位置改为 `$CXLAB_OUT`(默认 `${TMPDIR:-/tmp}/cx-lab-out`,由 cxlab 模块的 cxlab_outdir 创建);(3) 根下 9 个 C99 示例登记冒烟批次 examples-smoke(19 条断言);(4) 新增 examples/cx-lab/README.md。runtimeiocheck 的依赖 tests/runtimeiobytes.c 已随 tests/ 归档到 `archive/testing-c43-legacy/tests/`,且它包含的 run.c 现依赖 .cx 实现的 cxcodec 符号,plain cc 链不过;现改为具名拒绝(rc 2,不是 PASS 也不是 FAIL),二进制 IO 性质在此示例中未被检查(UNVERIFIED)。

### 未验证与限制

apps、win、run-speed、https 未实跑;引用计数仅按上述口径,tests/ 里以 glob 形式引用 `examples/*.c` 的不会被 -F 计入;建议动作均未执行。

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
| M1 | unsafe 边界显式化与审计表:dock 模块即 cx 的 unsafe 区;建 `build/dock-audit.tsv`(模块、dock 种类、裸调用符号数、审计人、理由),dock 模块数与裸调用数只降不升 | Rust unsafe 区块 | 非产品输入的工具与表;立即 | `sh ./unisacc.com tools/dock-map.cx` 的模块数与表行数相等(现 89 docked,其中 module=1 的 84;依据 `build/dock-audit.tsv` 与 `tools/check-dock-audit.cx`) |
| M2 | 拥有者标注 pragma:`#pragma unisa owns sym`(返回值由调用方拥有、须交给指定释放函数),先只进接口表列与文档,由 lint 检查「标注了的返回值在同函数内被释放、转移或返回」这一条流不敏感规则 | Rust 所有权、`ref-rust.md` 第 6 节(9) | 表与 lint 为非产品输入(0.0.62 可做);pragma 进规划器属 cxmod 三源,0.0.65 之后;CXIF v1 字段属 CX-W-8 | 对 `cx/mem` 的 `mg_take` 调用点跑 lint,漏放点数入表 |
| M3 | 句柄代际号推广:`cm_*` 的世代检查模式推广到 `sy_*` 返回的 fd、进程、套接字句柄的库面包装(关闭后使用得到确定的 `-EBADF` 而不是命中被复用的号) | Python/Node 的「已关闭即确定错误」、Rust OwnedFd | 运行库头(新增文件,不改冻结的 cx/sys 既有文件);0.0.62 候选或其后 | 新增 check:关闭后读写 10 类调用全部返回 `-EBADF` |
| M4 | arena 作用域:`mem.arena`(现有 `ar_new`/`ar_alloc`/`ar_drop`)增加成对的 enter/leave 标记检查(leave 时回滚并使其间句柄世代失效),替代逐块释放 | Rust thread::scope 的「括住生命周期」 | 运行库头(cx/mem,非冻结);0.0.62 候选或其后 | `sh ./unisacc.com cx/check_memory.cx` 新增作用域外使用被拒的断言 |
| M5 | 带长度字符串视图作为库面:以 `ms_slice` 为准,`tx_*` 新增接口一律取(指针,长度)或切片,不新增以 NUL 结尾为唯一边界的接口;不加语法 | Rust &str、Node Buffer | 表规则为非产品输入(立即);新接口属运行库头 | `build/std-surface.tsv` 新增行中无长度参数的指针形参数为 0(lint) |
| M6 | 编译期可证明性质的扩面只走规划器的结构性拒绝(如「标注为独占的句柄不得出现在两个模块的静态存储里」),不做流敏感借用检查 | Rust 借用检查的可负担子集 | cxmod 三源;不早于 0.0.67,先出设计 | 设计稿每条性质配一个被拒的最小 `.cx` |

不取:垃圾回收与引用计数运行时(单文件 APE 与 no-libc 下要自带运行时,且改变 C/ASM 内存合同:零写占槽、覆盖前扩容);流敏感借用检查与生命周期标注(需要新语法或属性,宿主 C99 编译器无法忽略后仍同义,且检查器要在三源各实现一遍);把 `cx/owned.cx` 扩成通用所有权系统(CX-O-7 冻结)。

### 11.7 内存安全基线(2026-11-14 实测与设计;设计部分未实现,不得记 PASS;承 R6-L3)

定位:10.7 是 export 合同,不含内存安全;本节给 (1) `tools/cx-own.cx` 报告条数基线,(2) 运行期检查覆盖表,(3) 一条最小编译期可检性质的方案与落点。本节不声称 Rust 级所有权,不改变 11.6「任何文档不得写内存安全」的约束。

**(1) cx-own 基线**(输入身份 main HEAD dc966931c7 的工作区,已装产品实跑;命令 `sh ./unisacc.com tools/cx-own.cx -- --tracked DIR/`,五目录并行,各自单次墙钟 5.6 至 7.0 秒,均在 60 秒内;rc 1 表示有报告,rc 0 表示无)

| 目录 | rc | files | functions | analyzed | skipped | owners | DOUBLE-FREE | USE-AFTER-FREE | LEAK |
|---|---|---|---|---|---|---|---|---|---|
| cx/ | 0 | 235 | 1933 | 1925 | 8(goto) | 63 | 0 | 0 | 0 |
| ext/ | 0 | 38 | 389 | 388 | 1(goto) | 5 | 0 | 0 | 0 |
| src/ | 1 | 25 | 266 | 262 | 4(goto) | 15 | 1 | 0 | 3 |
| tools/(参考,非门) | 1 | 933 | 9236 | 9209 | 27(goto 26、parse 1) | 1297 | 1 | 6 | 377 |
| tests/(参考,非门) | 1 | 347 | 2843 | 2841 | 2(goto) | 343 | 0 | 0 | 17 |

cx/ 子目录(同命令,报告三类全为 0):cx/sys files=26 functions=297 skipped=0;cx/core 9/71/1;cx/mem 6/60/0;cx/compiler 78/619/3;cx/cx0 2/107/1;cx/runtime 87/480/3;cx/driver 10/93/0。cx/ 跟踪 `.cx` 249 个,分析 235 个,差 14 个为符号链接(`git ls-files -s` 模式 120000 实数 14,工具按头注释跳过)。

src/ 四条明细:`LEAK src/drivestream.cx:222`、`:223`、`:229`(fn=ds_attr_in,alloc=220,变量 tab;222 与 223 两行是 `mm_copy`/`mm_move` 失败的 `return 1` 未放 tab,读源码看是真漏放,229 未逐行复核);`DOUBLE-FREE src/stages.cx:370`(fn=st_span,变量 owned;369 与 370 行的条件 `pr == 3` 与 `pr == 2` 互斥,是误报,与 `prd.md` 能力速查 2026-11-03 的记载一致)。

门口径(R6-L3 的「cx/ 与 src/ 报告条数不增」):cx/ 三类合计 0;src/ 为 DOUBLE-FREE 1、USE-AFTER-FREE 0、LEAK 3。skipped 不计入报告但须同报(cx/ 8、src/ 4),跳过数上升等于覆盖下降,不得用它换报告条数下降。

基线的限制(如实):cx-own 只看函数内局部指针对 malloc/mm_alloc 一族的拥有;cx/ 全树只有 63 个局部 owner、src/ 15 个,产品的所有权多在结构体字段、`mg_buf`、`cm_*` 句柄与 arena 里,不在分析范围。「cx/ 报告 0」的含义是这 63 个局部 owner 未见三类问题,不是 cx/ 无内存缺陷。误报率与漏报率对 cx/、src/ 为 UNMEASURED(仅有 2026-11-03 全仓抽 10 条 LEAK 全真的旧抽样)。基线尚未入 build/ 下的表(R6-W9 的写域,不在本节写域内)。

**(2) 运行期检查:已覆盖与未覆盖**(只读;依据 `cx/core/memory.cx`、`cx/core/mem/{heap,bytes,slice,grow}.cx`、`cx/sys/mem.cx`;cx/sys 与 cx/core 冻结,未改)

| 性质 | `cm_*` 句柄面(cx/core/memory.cx) | `mm_*` 堆面(mem/heap.cx、bytes.cx) | `ms_*`/`mg_*` 带界面 | `sy_*`(cx/sys/mem.cx 与 fd 类) |
|---|---|---|---|---|
| use-after-release | 已覆盖:`cm_drop` 把槽的 token 清 0,token 由 `cm_issue` 单调发放不复用,旧 owner 再用得 `CM_INVALID_OWNER`(memory.cx:21 至 24、:59);`cm_move` 换新 token 使旧句柄失效(:58);lease 在 owner token 变化后得 `CM_INVALID_LEASE`(:25 至 29) | 未覆盖:`mm_free` 后指针仍可解引用,语义等同 libc free(heap.cx 头注释明写不加所有权与边界检查) | 未覆盖:`ms_slice` 是(指针,长度)值,不跟踪底层 `mg_buf` 的存活,`mg_free` 后旧切片悬垂 | 未覆盖:`sy_unmap` 后地址、关闭后的 fd 号可被复用命中(11.6 的 M3 与 10.8 的 `sy_res_live` 均未实现) |
| 越界 | 已覆盖:`CM_BOUNDS`,`cm_range` 用 `count<=length-offset` 的不溢出写法(:30);lease 访问限在 lease 的 offset/length 内(:69、:70) | 部分:`mm_copy`/`mm_move`/`mm_fill` 带目的容量 dcap,超出得 `-ERANGE`;源长度与裸指针下标、指针运算不受检 | 部分:`ms_at` 越界得 `-ERANGE`(slice.cx:63);取出 `s->p` 之后的裸访问不受检 | 未覆盖:`sy_map_none`/`sy_unmap`/`sy_protect_*` 收裸指针加长度,长度是否属于同一映射不核 |
| 双释放 | 已覆盖:第二次 `cm_drop` 得 `CM_INVALID_OWNER`,第二次 `cm_release` 得 `CM_INVALID_LEASE`;有活 lease 时 drop 得 `CM_BUSY` | 未覆盖:`mm_free(0)` 无操作,同一非空指针放两次即 libc 双释放 | 部分:`mg_free` 释放后把结构清零(grow.cx:27 合同),同一结构再放无害;结构被复制后各放一次不防 | 未覆盖 |
| 跨模块持有 | 未覆盖:句柄是可复制、可伪造的值(memory.cx:5),无模块身份;只核跨实例(`CM_CROSS_INSTANCE`),任何拿到值的模块都能用、能 drop | 未覆盖 | 未覆盖 | 未覆盖 |
| 借用冲突 | 已覆盖(仅本面):读写计数,冲突得 `CM_BUSY`,只读 lease 写得 `CM_READ_ONLY` | 无此概念 | 无此概念 | 无此概念 |

`cm_*` 面的边界:单线程;上限 16 实例、64 缓冲、128 lease、64 MiB;token 用尽得 `CM_TOKEN_EXHAUSTED`;`cm_unsafe_import` 收裸源指针,源长度不受检;该面逐字节访问,产品热路径不走它(调用者数 UNMEASURED,本轮未用 `refactor.cx sites` 清点)。结论:四条性质里只有 `cm_*` 面覆盖前三条,产品实际使用的 `mm_*`、`mg_*`、`ms_*`、`sy_*` 面对 use-after-release 与双释放基本不设防;「跨模块持有」在所有面都没有任何运行期或编译期检查。

**(3) 最小编译期可检性质 OWN-E1:转出须声明**

- 性质:非 static(模块公开)函数若把本函数内由 `mm_alloc`、`mm_zalloc`、`mm_grow`、`mm_strdup` 得到的块经 `return` 交给调用方,该函数名必须出现在本单元的 `#pragma unisa owns NAME` 里;反向,`owns NAME` 必须指向本单元定义的非 static 函数。static 函数不受检(块未出模块)。选它的理由:cx-own 已在函数内判定「所有权经 return 离开函数」这一事件(头注释 Ownership leaves the function),只是现在不输出;它是名字加声明的一致性,不需要流敏感借用检查与生命周期标注;它正是「跨模块持有」这一全空白性质的最小切口。
- 它证明什么、不证明什么:只证明「公开面上转出所有权的函数都有声明」,使调用方的释放义务可以从接口上机械列出;不证明调用方会释放、不证明无释放后使用、无双释放,也不管经 `*out = p`、结构体字段、全局存储转出的块(这几类先只计数,见下)。
- 检查位置,分两期:
  - 甲期(非产品输入,工具类,可立即做):`tools/cx-own.cx` 新增 `--escapes`,对公开(非 static)函数里每个 return 语句上转出的块输出 `ESCAPE<TAB>FILE:LINE<TAB>FUNCTION<TAB>VAR`,末行 `CX_OWN_ESCAPES<TAB>files=N<TAB>escapes=N<TAB>functions=N`(格式以已落地实现为准,2026-11-14 对齐;原设计的 `ESCAPE-RETURN`、`fn=`、`linkage=`、`CX_OWN_ESC` 写法作废;static 函数不输出;`ESCAPE-STORE`(`*out`、字段、全局)未实现,条数 UNMEASURED)。声明先放表不放 pragma:表定为新开的 `build/own-escapes.tsv`(改掉原「不新开表」的写法,理由:现成表无一以「文件加函数名」为键;列与归类见下文「甲期核对结论」),lint 核「`--escapes` 报出的函数全在表内、表内无多余行」。
  - 乙期(产品输入,cxmod 三源类):检查写在权威 `cx/cx0/plan.cx` 的规划期(与 `cxm_checkcalls` 同一遍函数体记号扫描),经 `tools/cx0-extract.cx` 投影到其余两源,不在 `cx/compiler/cxmod.cx` 手写第二份;因此前置条件是 10.5 的 D0 三源归一完成(`check-cxmod-tri.cx` gaps=0)。乙期判定收窄为流不敏感的名字规则:公开函数体内某个 `return` 的表达式是上述四个分配函数的直接调用,或是在同一函数内被赋过这类调用结果的局部名。
- 拒绝文案(沿 `cxm_start` 的 `FILE:LINE:` 前缀与现有英文风格):
  - `FILE:LINE: function NAME returns an owned block; declare it with #pragma unisa owns NAME`
  - `FILE:LINE: unisa owns NAME: no non-static function NAME in this unit`
  - `FILE:LINE: unisa owns takes a function name`
- 夹具(`tests/` 下新目录,五个最小 `.cx`):(a) 公开函数 return mm_alloc 结果且有 owns,接受;(b) 同 (a) 无 owns,拒,文案第一条;(c) owns 指向不存在或 static 的名字,拒,文案第二条;(d) static 函数 return mm_alloc 结果无 owns,接受;(e) 公开函数 return 入参指针(借用,不拥有),接受。
- 验收命令与预期值(均为设计预期,未实现未实跑,不得记 PASS):
  - 甲期:`sh ./unisacc.com tools/check-cx-own.cx` 末行仍 rc0:CHECK_CX_OWN 且新增 escapes 夹具 5 条全过;`sh ./unisacc.com tools/cx-own.cx -- --escapes --tracked cx/ src/ ext/` 末行 `CX_OWN_ESC` 的 public 未声明数入表作基线(甲期工具已落地,批次 cx-own-escapes;2026-11-14 实测公开函数经 return 转出:cx/ 13 条 4 个函数(cxi_read、mm_strdup、fp_new、cxns_audit),ext/ 1 条 1 个函数(sb_new),src/ 0 条;输出行 `ESCAPE<TAB>FILE:LINE<TAB>FUNCTION<TAB>VAR`,末行 `CX_OWN_ESCAPES`,与上文设计的 ESCAPE-RETURN/CX_OWN_ESC 格式不同,以实现为准;仅函数内局部 owner 口径;cxns_audit 的 qlen 8 条已核,全部误报,见下文「甲期核对结论」)。
  - 乙期:`sh ./unisacc.com tools/check-own-escape.cx` 末行 `CHECK_OWN_ESCAPE passed=5 failed=0`,其中 (b)(c) 两条 rc 1 且 stderr 逐字含上列文案;`sh ./unisacc.com tools/check-cxmod-tri.cx` 末行 failed=0 gaps=0;`sh ./unisacc.com tools/cx0-extract.cx -- --check --all` 两行 EXACT;全树 `cx/`、`src/`、`ext/` 用新产品实编被本规则拒绝的单元数为 0(开启规则前先由甲期把未声明数清到 0)。
- 版本落点(R6-V 版本序):甲期不动产品输入,不占版本槽,可在 0.0.65 周期内随工具提交。乙期是 cxmod 三源类产品输入:不进 0.0.65(该候选是 D0 归一加 export D1a,不与 export 同候选);0.0.66 库包源、0.0.67 cx.stream R3(驱动)、0.0.68 驱动与运行缓存二期都不是 cxmod 类,且 R6-L3 已定 0.0.63 至 0.0.68 不排内存安全实现;故乙期落在 0.0.68 之后的第一个 cxmod 三源类候选,不早于 0.0.69,单独一类不夹带。与 11.6 的 M2(owns 标注)、M6(结构性拒绝,不早于 0.0.67)是同一条线的具体化:甲期即 M2 的表与 lint,乙期即 M6 的第一条性质。
- 取消条件:甲期测得 public ESCAPE-RETURN 数为 0 或误报率过半,则乙期不做,把结论原位写回本节。
- 甲期核对结论(2026-11-14 读源码加实跑;`sh ./unisacc.com tools/cx-own.cx -- --escapes --tracked cx/ ext/ src/` 末行 `CX_OWN_ESCAPES files=298 escapes=14 functions=5`,该命令 rc 1 来自既有 double_free=1 leak=3,与转出无关):
  - cxns_audit 8 条全部判 (b) 误报。`cx/runtime/network_structure.cx` 第 40 行 `int cxns_audit(...)` 返回 int,不可能经返回值交出指针;qlen 在第 63 行 `mm_alloc`,8 条事件行 67、72、73、75、77、78、79、82 全是 `return done(qlen, CODE)`,第 15 行 `static int done(int *qlen, int code) { mm_free(qlen); return code; }` 当场释放;正常路径第 85 行 `mm_free(qlen); qlen = 0;`。块从不离开本函数,调用者无释放义务。
  - 同机理连带误报:cxi_read 3 条(`cx/driver/input.cx` 行 129、132、144)都是 `return cxi_drop(owned, fd, buf, ...)`,第 55 至 66 行的 static 帮手 `cxi_drop` 在第 57 行 `mm_free(buf)`,判 (b)。cxi_read 确实把块交给调用者,但走的是第 155 行 `*out = buf`(STORE 类),不在 return 口径内,当前工具不报。
  - 真转出 (a) 共 3 条 3 个函数:`cx/mem/heap.cx:42` mm_strdup 的 d、`cx/runtime/frameparts.cx:21` fp_new 的 s、`ext/textx/build.cx:41` sb_new 的 b(sb_new 另有 `b->inner` 字段持有的第二块,属 STORE 类)。
  - 误报机理:`tools/cx-own.cx` 第 1412 至 1414 行在 return 表达式的整个求值期间置 `g_inret = 1`;求值中 owner 作为实参传给非已知借用者(第 988 行,本单元 static 帮手也算)触发 `st_escape`,第 606 行见 `g_inret` 就记 `esc_note`。所以它记的是「return 语句里发生的任何转出」,不是「返回值就是该块」。最小复现 `$CLAUDE_JOB_DIR/tmp/probe.cx` 实跑:`return done(q, 2)` 被报;同样的调用先赋给 rc 再 `return rc` 不报;`return q` 报,输出 `escapes=2 functions=2`。
  - 修法(已实现并实测,2026-11-14):`tools/cx-own.cx` 的 K_RETURN 改为先 `ev(n, S)` 取返回值位集,只在随后的 `esc(S, rv, line)` 两侧置 `g_inret`。实测:不带 `--escapes` 对 `src/*.cx` 与 `--tracked cx/` 的输出 sha256 修前后相同(旧版取 HEAD 副本跑);带 `--escapes --tracked cx/ ext/ src/` 末行 `CX_OWN_ESCAPES files=298 escapes=3 functions=3`(与预期一致);`check-cx-own-escapes` 增 4 条断言(return done(q,2) 不报、先赋 rc 再 return rc 不报、return q 报 1 条、汇总),末行 `assertions=10 failed=0`;`check-cx-own` `assertions=40 failed=0`;`ratchets` `ok=13 fail=0`。上述 own-escapes.tsv 的 owns=no 两行(cxns_audit、cxi_read)已无对应转出,建表时不再收。
  - 取消条件裁定:按事件误报 11/14,按函数 2/5,字面上已过半;但成因是甲期工具的记点位置,不是 OWN-E1 性质本身,乙期的名字规则(return 表达式是分配调用或被赋过分配结果的局部名)对 `return done(qlen, 6)` 本就不触发。故不取消乙期;取消条件改为按修法落地后的重测数判定,届时误报仍过半再取消。
  - 表 `build/own-escapes.tsv`(未建,下列为定稿列与初始行):列 `file function events owns class note`,键为 file 加 function,一行一个公开函数。`events` 是该函数的 ESCAPE 行数(不记行号,免源码移行引起漂移);`owns` 取 yes(真转出,调用者须释放,乙期须写 `#pragma unisa owns NAME`)或 no(误报,不得写 owns);`class` 取 return 或 helper-free。现状 14 条归为 5 行:owns=yes 3 行(mm_strdup events=1、fp_new events=1、sb_new events=1,class=return);owns=no 2 行(cxns_audit events=8、cxi_read events=3,class=helper-free)。即真转出需声明 3 个函数,cx/ 2 个、ext/ 1 个、src/ 0 个。修法落地后 owns=no 两行应随之删除,表剩 3 行。
  - 乙期 plan.cx 吃什么:只吃本单元的记号流和本单元的 `#pragma unisa owns NAME`,不读 `build/own-escapes.tsv`,也不读 `--escapes` 输出(产品不依赖工具产物)。表是迁移清单与对拍基准:乙期开启前按 owns=yes 行在各自单元加 pragma,届时 lint 增一项「表内 owns=yes 的函数集合等于源码里 owns 声明的函数集合」,一一对应。乙期规则的两处已知出入:(1)mm_strdup 体内用的是 `malloc` 而非四个 `mm_*` 分配名,名字规则不会强制它声明,须在乙期分配名集里补 `malloc` 或接受它只靠表与 lint 约束;(2)return 表达式须先剥类型转换再比名字(现有 3 个真转出都是裸局部名,带强转的写法现状 0 例)。
  - 甲期 lint 方案(未实现;名字 `tools/check-own-escapes.cx` 经 `git log --all` 查无历史,与已有的夹具检查 `check-cx-own-escapes.cx`、乙期的 `check-own-escape.cx` 是三个不同文件):起一次 `cx-own.cx -- --escapes --tracked cx/ ext/ src/`(只读输出,不看其 rc),把 ESCAPE 行按 file 加 function 聚合计数后与表对拍。四类报错行:`UNDECLARED<TAB>FILE<TAB>FUNCTION`(有转出、表里无行)、`STALE<TAB>FILE<TAB>FUNCTION`(表里有行、已无转出)、`DRIFT<TAB>FILE<TAB>FUNCTION<TAB>table=N<TAB>actual=M`(events 不等)、`BADROW<TAB>LINE`(列数不为 6、owns 或 class 取值非法、键重复)。另带 `--self`,用 `/tmp` 下的夹具表与夹具源各造出四类报错一次。登记批次 own-escapes,prd 能力速查补一行。
  - 落地实测(2026-11-14):`build/own-escapes.tsv` 已建(3 行 owns=yes),`tools/check-own-escapes.cx` 已实现,批次 own-escapes 登记并 PASS;默认末行 `functions=3 owns=3 not_owned=0 undeclared=0 stale=0 drift=0 failed=0` rc 0,删 fp_new 行得 undeclared=1 rc 1,加不存在函数得 stale=1 rc 1,`--self` 为 `assertions=8 failed=0`(夹具在内存中构造,未落 TMPDIR);`derive --check` ok=9 drift=0,`ratchets` ok=13 fail=0。下一条为落地前的设计预期,保留作对照。
  - lint 验收命令与预期值(设计预期,未实现未实跑,不得记 PASS):`sh ./unisacc.com tools/check-own-escapes.cx` 末行 `CHECK_OWN_ESCAPES<TAB>functions=5<TAB>owns=3<TAB>not_owned=2<TAB>undeclared=0<TAB>stale=0<TAB>drift=0<TAB>failed=0`,rc 0(修法落地并删去两行后为 functions=3 owns=3 not_owned=0);从表里删去 fp_new 行再跑,输出含 `UNDECLARED<TAB>cx/runtime/frameparts.cx<TAB>fp_new`,末行 undeclared=1 failed=1,rc 1;表里加一行不存在的函数再跑,输出含 STALE,末行 stale=1 failed=1,rc 1;`sh ./unisacc.com tools/check-own-escapes.cx -- --self` 末行 `CHECK_OWN_ESCAPES_SELF<TAB>assertions=8<TAB>failed=0`,rc 0;单次耗时预期低于 60 秒,实际 UNMEASURED。

**仍是运行期而非编译期的性质**(乙期落地后也不变):use-after-release(仅 `cm_*` 面按 token 拒绝,`mm_*`/`mg_*`/`ms_*`/`sy_*` 面无检查);越界(仅经带界接口的访问);双释放(仅 `cm_*` 面与 `mg_free` 清零路径);借用冲突(仅 `cm_*` 计数);句柄伪造与复制(无任何检查)。完全没有的:数据竞争免疫、裸指针别名安全、生命周期证明。OWN-E1 是声明一致性检查,不是所有权系统;发布说明继续保留「运行期检查,不是编译期所有权」一句。

UNMEASURED 汇总:cx-own 对 cx/、src/ 的误报率与漏报率;public ESCAPE-RETURN 与 ESCAPE-STORE 条数;`cm_*` 面的产品调用者数;乙期规划期耗时增量;osx、win 宿主上 cx-own 的基线(本轮只在 Linux x86_64 跑)。
复核结论(2026-11-14,cx-own 复现 leak=3 double_free=1):ds_attr_in 的 LEAK 222、223、229 三条均为真漏放(错误路径):220 行 mm_alloc 的 tab 在 mm_copy 失败(222)、mm_move 失败(223)、st_attr_reset 失败(229)三处 return 1 前未释放,成功路径经 230 行 st_attr_exchange 转交,故只漏在失败时,触发需分配后复制、移动或重置返回非零(注入失败夹具,当前无);补丁 ~/.cache/unisacc/patches-0.0.65/own-leak.patch.txt(git apply --check rc 0,未应用,待 0.0.63 冻结后);另 else 分支 cxaf_strip 失败时 tab 归属未核(未验证);src/stages.cx:370 的 DOUBLE-FREE 为误报:369 与 370 行 pr==3 与 pr==2 互斥,同一次进入至多释放一次 owned。

## 12. 状态事实表化(2026-11-14,只读评估与设计;未实现,不得记 PASS)
- SFT-0 结论:缺的不是数据库,是"状态"这一类事实还没进表。代码侧已有 57 张登记表(build/table-index.tsv),状态侧只有一份 168820 字节的 prd_mempalace.md,建文件当天(2026-11-14 10:53 起约 9 小时)被提交 110 次,峰值每小时 24 次;先把"仍未验证"16 条做成一张表并只读对拍(SFT-S0),对拍为 0 差异后再让文档该节由表生成。不用 SQLite、python、node,只用 TSV 加 .cx。
- SFT-1 痛点实测(git log/numstat 与文本计数,输入身份 main HEAD 0c6847a90d;近 7 天窗口内该文件只存在 1 天):提交 110 次,占同期全仓 1447 次的 7.6%;其中只动这一个文件的 70 次(64%);增 736 行删 209 行;行数 527(取数期间他人未提交改动使其到 571);最长一行 2822 字符,超过 500 字符的行 47 条;版本号出现 290 次(0.0.62 108 次、0.0.63 81 次、0.0.65 33 次),所以改一次版本序要动上百处;不同任务号(K/H/N/W/S/Z/F 加数字)87 个;含 UNVERIFIED 或 UNMEASURED 的行 56 条;"下一动作"节条目 83 条、"仍未验证"16 条、"阻塞发布"4 条。
- SFT-2 丢行与冲突:提交说明里可证实的他人行被覆盖 1 次(62534444c1,任务表 N1 行被表头行覆盖后修复);取数时刻工作区里该文件有他人未提交改动 106 行、prd_cx.md 66 行,即随时有人在途同改;rebase 冲突次数、漏提交次数 UNMEASURED(git 历史不留痕,需 commit-own 的 foreign_kept 输出留档才能数)。
- SFT-3 现成同类表(不重复造):build/units.tsv(unit_id dim parent inputs check state owner evidence)、build/phase-times.tsv(带 waiver_task、waiver_until)、build/approvals.tsv(id kind approver date note)、build/pkg-gaps.tsv;tools/status.cx 已按 section key value age_s source 出行。任务、未验证、风险、补丁、决定、版本序六类在 table-index 中无对应表(按表名与用途列检索为 0)。
- SFT-4 表结构(都放 build/,手写事实表,首行 # 列名,键唯一,文本列不含制表符):(1) build/state-unverified.tsv:id version kind(blocker、unverified、gap) state(OPEN、CLOSED、WAIVED) subject missing action write_domain owner evidence updated;(2) build/state-tasks.tsv:id round version state(TODO、DOING、DONE、DROPPED) goal write_domain accept_cmd expect owner deadline depends evidence;(3) build/state-risks.tsv:id state trigger countermeasure owner evidence;(4) build/state-patches.tsv:id path sha16 base_rev apply_check target_version write_domain state;(5) build/state-versions.tsv:version class goal accept_cmd expect first_task state;(6) build/state-decisions.tsv:id date decision basis evidence rollback supersedes。版本号只在 (5) 出现一次定义,其余表按 version 列引用。
- SFT-5 工具落点(新名先 `git log --all --oneline -- tools/NAME` 查,2026-11-14 查 state-gen、state-q 无历史):tools/state-gen.cx(`--emit TABLE` 出 Markdown 段到标准输出;`--against FILE` 与现文对应小节逐条对拍;`--check` 文档生成段与表一致否则非零;`--write` 只重写 `<!-- state:TABLE begin/end -->` 标记之间的段)、tools/state-q.cx(`--version V --state S --domain 前缀 --owner O` 筛选;末行 `STATE_Q rows=N doing=M conflicts=C`,C 为 DOING 行之间 write_domain 前缀相交的对数)、tools/check-state-gen.cx(夹具副本,含一条缺行、一条多行、一条域相交的负控);--check 接入 tools/derive.cx 的派生物总闸,文档成为半生成物:标记段由表生成,标记外的叙述仍手写。
- SFT-6 迁移步骤:(a) SFT-S0 只读:抄出 build/state-unverified.tsv 16 行,`--emit` 加 `--against` 对拍,不改现文;(b) 0.0.63 公开后在"仍未验证"节加标记并 `--write`,`--check` 入 derive;(c) 补丁清单、风险 F 表各一版一张;(d) 任务表与 state-q 的写域冲突数最后做(83 条里夹表格与长段,抽取成本最高);(e) 每步改表用 `tools/commit-own.cx --keyed`,一行一键,合并粒度从"段"降到"行"。
- SFT-7 最小切片 SFT-S0 的验收:`sh ./unisacc.com tools/state-gen.cx -- --emit unverified --against prd_mempalace.md` 末行 `STATE_GEN rows=16 missing=0 extra=0` rc 0(rows 以开工时刻该节条目数为准);`sh ./unisacc.com tools/check-state-gen.cx` 末行 failed=0 且三条负控各报拒;`sh ./unisacc.com tools/tsv-lint.cx` 对新表无新增报错;单次运行耗时 UNMEASURED(预期远低于 60 秒,未实跑)。
- SFT-8 风险与落点:0.0.63 冻结期间不动源码与主文档,SFT-S0 只新增一张表与两个工具(非产品输入),可并行做;`--write` 不早于 0.0.63 公开,建议落在 0.0.65 窗口;风险 (1) 长条目(最长 2822 字符)塞进单元格后可读性差,对策是 subject 限一句,细节留 evidence 指向提交号或文档锚点;(2) 半生成期间有人直接改标记段,对策是 --check 入 derive 并在段首写"由表生成,勿手改";(3) 表与文档双写期不一致,对策是每类只有一步双写期且以 --against 为门;(4) 收益未实测:提交次数与丢行是否下降要到 (b) 之后用同口径再数一次,现为 UNMEASURED。
