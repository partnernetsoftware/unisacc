# CX 分层：底层对接包与上层（2026-10-30，cc）

主人口径：CX 底层要有 libc 与 POSIX 的专用对接包；上层不再直接用 malloc、memcpy 这类操作。

## 目录

- 设计：
  - [层](#层)（L0 宿主对接 / L1 机制 / L2 驱动 / L3 工具）
  - [只有底层能 include](#只有底层能用-include-对接-libcposix主人口径2026-10-30)（白名单 + 收口方向）
  - [棘轮](#棘轮)（`tests/cxraw.cx` 只降不升）
  - [L0 接口](#l0-接口按上表实际用法归类) · [已有与新建的 L0](#已有与新建的-l0)
  - [落地顺序](#落地顺序)
- 已落地（按时间，可回归）：
  1. [cli.cx 模块内带界比较替代 strcmp/strncmp](#已落地cli-cx-用模块内-static-带界比较替代-strcmpstrncmp2026-10-30cc)
  2. [cx/text 补 tx_eqz/tx_prefixz](#已落地cx-text-补-tx_eqz--tx_prefixzz-string-无-cap-带界比较2026-10-30cc)
  3. [cx/sys 补 sy_pipe](#已落地cx-sys-补-sy_pipe带-cloexec-的裸管道原语2026-10-30cc)
  4. [sy_pipe 消费者接线 check-stream-output](#已落地sy_pipe-的第一个消费者接线toolscheck-stream-outputcx2026-10-30cc)
  5. [cx/mem 补 grow（Vec）](#已落地cx-mem-补-grow可增长所有权缓冲区c-的-vecu82026-10-30cc)
  6. [mem.grow 消费者 read-all](#已落地mem-grow-的第一个生产消费者接线toolsread-allcx2026-10-30cc)
  7. [上层库 text.build（String）](#已落地上层库结构-textbuildrust-string--node-buffer-对应物2026-10-30cc)
  8. [cx/sys 补 sy_unsetenv + 消费者](#已落地cx-sys-补-sy_unsetenv--消费者接线toolsrun-startup-benchcx2026-10-30cc)
  9. [cxmod 命名空间冲突验收锁定](#已落地cxmod-命名空间冲突检测的验收锁定testscx-namespacecx2026-10-30cc)
  10. [text.build 补 sb_join](#已落地text-build-扩充-sb_joinrust-slicejoin--arrayjoin2026-10-30cc)
  11. [cx/runtime/stream_output 迁移 sys.fs](#已落地cx-runtimestream_outputcx-迁移到-sysfsl1-产品模块去裸-libc2026-10-30cc)
  12. [mem.grow 补 mg_move/mg_take（所有权）](#已落地mem-grow-补-mg_move--mg_takerust-式所有权转移2026-10-30cc)
  13. [cx/core/sparse 迁移 mem.heap](#已落地cx-coresparsecx-迁移到-memheap执行器稀疏表去裸-libc2026-10-30cc)

## 层

| 层 | 路径 | 可直接调用 libc/POSIX | 职责 |
|---|---|---|---|
| L0 宿主对接 | `cx/sys/`、`cx/mem/`、`cx/text/`，以及 `tests/cxraw-layers.tsv` 列出的现有适配器（`src/host.cx`、`src/hostnative.cx`、`cx/runtime/process*.cx`、`cx/runtime/winprocess.cx`） | 是 | 文件、目录、进程、时间、环境、映射内存；带所有者的内存与有界字节/文本操作 |
| L1 机制 | `cx/core/`、`cx/runtime/` | 否（经 L0） | 执行器、包与模型装载、路由、资源账本 |
| L2 驱动与编译器 | `cx/driver/`、`cx/compiler/`、`src/`（除 L0 两个文件） | 否 | CLI、产品面接线、绑定 |
| L3 工具、测试、应用 | `tools/`、`tests/`、`apps/` | 否 | 构造、验证、应用 |

## 只有底层能用 include 对接 libc/POSIX（主人口径，2026-10-30）

- 能写 `#include` 的只有底层：`include/`（前导与携带头）、`tools/cxcc-hostinc/`、`cx/sys/`。名单在 `tests/cxinclude-boundary.tsv`，由 `tests/cxinclude.cx` 检查。其余 `.cx` 一律用隐式前导，加上 `#pragma unisa use sys.*/text.*/mem.*`。
- 两张表有一处不一致，记在这里。`tests/cxraw-layers.tsv` 准许 `src/host.cx`、`src/hostnative.cx`、`cx/core/kernel.cx`、`cx/runtime/process*.cx` 直接调 libc（L0 适配器），但它们不在 include 白名单里。收口方向是让它们需要的头都进 `cx/sys`，不是把它们加进白名单。
- 白名单外仍在 include 的（2026-10-30）：
  - `src/host.cx` 原来 3 个（`<sys/mman.h>`、`<dlfcn.h>`、`<fcntl.h>`）。mmap、mprotect、munmap 已进 `cx/sys/mem.cx`，dlopen、dlsym、dlclose、dlerror 的地址已进 `cx/sys/dl.cx`。open 用的 `O_*` 本来就在前导里，没有再为 `<fcntl.h>` 写 include。`src/host.cx` 现在 0 个 `#include`。`tests/cxinclude.cx --check` 不再报这个文件；整次仍失败，报的是 `apps/csih`（24 个超基线、2 个低于基线），那是 csih 车道。
  - `apps/csih`：主人定 csih 只做自己的事，`cx/sys` 的接口由 cc 补。已补的有 `sys.net`、`sys.poll`、`sys.tty`、`sys.time`（796531e47f，检查 `cx-l0-sys-net` 43/0）和 `sys.guard`（setjmp/longjmp 收进 `sy_guard`/`sy_guard_fail`，检查 `cx-l0-sys-guard` 6/0）。5 个只用到前导头的 probe 已去掉 include（03cd501057）。剩下的按下表机械迁移：
    - `<sys/socket.h>`、`<netinet/in.h>`、`<arpa/inet.h>`、`<netdb.h>` 改为 `use sys.net`；
    - `<poll.h>` 改为 `use sys.poll`，类型用 `sy_pollfd_t`，事件位用 `SY_POLL*`；
    - `<termios.h>`、`<sys/ioctl.h>` 改为 `use sys.posix` 加 `use sys.tty`；
    - `<time.h>` 改为 `use sys.time`；
    - `<sys/file.h>` 改为 `use sys.posix` 的 `sy_flock`；
    - `<setjmp.h>` 改为 `use sys.guard`；
    - signal、errno、fcntl、stdio、string、unistd、sys/types、sys/stat、sys/wait、dirent 直接删掉，前导里已有。
  - **头文件表面探针算底层。** csih 有 10 个 probe，文件头写明它们测的就是某个携带头本身，例如 "that `<sys/socket.h>` alone builds is what it tests"。改成 `use sys.*` 会把要测的东西测没了。所以它们以精确条目 `文件.cx<TAB>理由` 逐个列进 `tests/cxinclude-boundary.tsv`，不按目录放行。`tests/cxinclude.cx` 会拒绝缺少理由或不是 `.cx` 的条目。
  - sy_* 一律返回 -errno，迁移时要改返回值判断。csih 的 Windows Winsock 路径没有对应的 L0（`sy_tcp_dial` 在 Windows 上返回 -ENOSYS），这条要 csih 自己定。两个 fuzz 测试被宿主 cc 当 `.c` 编译，交 csih 决定：要么改名成 `.c`，要么改用 `-include` 前导。
  - 其余为 0（89d5390e01 清掉了最后两行旧基线）。
- 完成标准：`sh ./unisacc.com tests/cxinclude.cx -- --check` 输出 `ok`，基线表里没有任何一行。

## 棘轮

`tests/cxraw.cx`（基线 `tests/cxraw.tsv`）统计 L1–L3 每个 `.cx` 里直接调用的 libc/POSIX 函数（内存、字符串、文件、目录、进程、环境、时钟，名单在源码 `raw_names`），注释与字面量不计。只降不升；新文件必须为 0。初始基线：635 个文件，22,847 次。2026-10-30 起不再重复计已跟踪的符号链接（f638c861e5）。2026-10-30 这次实跑：629 个已跟踪文件、19,634 次，基线 19,634，0 项超基线，0 项低于基线。新建的 `cx/sys/mem.cx`、`cx/sys/dl.cx` 当时还没进 git；它们在 `cx/sys/` 前缀下，本来就不计数。

产品路径的现状（L1–L2）：

| 目录 | 主要调用 |
|---|---|
| `src/` | free 162、fclose 14、strcmp 12、malloc 9、fwrite 9、realloc 8、memset 8、close 8、memcpy 7、getenv 7、fopen 7、unlink 6、fread 5、open 4 |
| `cx/runtime/` | memcpy 85、strlen 51、memcmp 21、strcmp 20、close 16、free 12、strchr 8、strncmp 7、memset 6、waitpid/pipe/read 各 3 |
| `cx/driver/` | strcmp 67、free 9、close 8、open 4、unlink 3、strncmp 3 |
| `cx/core/` | free 28、calloc 7、memcpy 5、realloc 4、memmove 3 |
| `cx/compiler/` | free 11、write 6、strcat 6、strlen/strcpy/malloc 各 3 |

## L0 接口（按上表实际用法归类）

- `cx/mem`：所有者缓冲区（创建、增长、借用视图、释放；长度与容量在句柄内，越界与重复释放具名拒绝）、定长复制/移动/填充/比较一律带两端长度；arena（一次释放一组分配，适合编译阶段的临时表）。
- `cx/text`：带长度的字节串比较、前缀/后缀/查找、有界格式化与拼接（收编 `cx_fmt`、`cx_path_join`）。
- `cx/sys`：整文件读写与原子替换（收编 `cx_read`、`cx_write`）、打开/关闭/读写/定位、stat 类、删除与改名、目录遍历、进程（spawn/wait/pipe，收编 `cx_run*`）、环境变量、单调时钟、映射内存与改权限。按平台分实现，错误码显式返回。

## 已有与新建的 L0

- 已有：`include/cx.h` 前导里的 `cx_read`、`cx_write`、`cx_run*`、`cx_walk`、`cx_fmt`、`cx_path_join`、`cx_remove/rmtree/trash`（棘轮不计 `cx_*`）；`cx/core/memory.cx` 的句柄式所有者/借用内存（`cx/core/memory.md`）。
- 新建：`cx/text/text.cx`（`tx_len`、`tx_same`、`tx_eq`、`tx_prefix`、`tx_chr`、`tx_find`、`tx_copy`，无 libc 调用，检查 `tools/check-cx-text.cx`）；`cx/text/deflate.cx`（`dz_bound`、`dz_deflate`、`dz_gzip_bound`、`dz_gzip`、`dz_crc32`，deflate/gzip 压缩，无 libc 调用，调用方提供输出缓冲，模块私有静态表因而不可重入，检查 `tools/check-cx-deflate.cx` 用系统 `gzip -dc` 对拍）；`cx/mem/arena.cx`（`ar_new`、`ar_alloc`、`ar_dup`、`ar_drop`，句柄式，按限额分配、整体释放，检查 `tools/check-cx-arena.cx`）。
- 使用方按目录放符号链接 `text -> ../cx/text`、`mem -> ../cx/mem`（tools/、tests/ 已加），`#pragma unisa use text.text` / `mem.arena`。
- 示范迁移：`tools/gold-table.cx` 33→0、`tests/cxraw.cx` 29→0。
- 新建 `cx/sys`（2026-10-30，cc 同意由 unisacc-00 做；`del.cx` 归 cc）。检查 `tools/check-cx-sys.cx`，42 条实跑。使用方按目录放软链 `sys -> ../cx/sys`（tools/、apps/csih/probes/ 已加），`#pragma unisa use sys.fs` 等。
  - `env.cx`：`sy_env`、`sy_env_or`。
  - `fs.cx`：`sy_mkdir`、`sy_chmod`、`sy_stat`、`sy_lstat`、`sy_exists`。`sy_exists` 只有 ENOENT 返回 0，ENOTDIR、EACCES 等返回 -errno。
  - `proc.cx`：`sy_run`、`sy_spawn`、`sy_wait`、`sy_kill`、`sy_pid`。状态统一为一个 int：退出码、128+信号、-errno。
  - `time.cx`：`sy_now_ms`、`sy_wall_s`、`sy_sleep_ms`、`sy_readable`、`sy_set_mtime`。
  - `net.cx`：监听与 `sy_tcp_connect` 只用本机 127.0.0.1，`sy_tcp_listen`、`sy_tcp_port`、`sy_tcp_accept`、`sy_tcp_connect`、`sy_wait_port`、`sy_send_all`、`sy_recv`、`sy_close`。写一律用 `send(MSG_NOSIGNAL)`。2026-10-30 为 csih 去 include 补：`sy_send`（单次发送，返回已发字节数）、`sy_resolve4`（系统解析器，首个 IPv4 点分文本）、`sy_tcp_dial`（任意主机名 IPv4 TCP，Windows 为 -ENOSYS）、`sy_net_error`（解析器失败返回 ≤ `SY_EAI_BASE`(-4096) 的码，与 -errno 不相交；取文本）。
  - `poll.cx`：`sy_poll`、`sy_pollfd_t`、`SY_POLLIN/PRI/OUT/ERR/HUP/NVAL`、`SY_POLL_MAX`(64)。模块里的 enum 与 typedef 随 `use` 导出，#define 不导出（实测），所以位值用 enum。
  - `tty.cx`：`sy_tty_raw`（对 `sy_tty_t` 做 cfmakeraw）、`sy_tty_size`（TIOCGWINSZ）。保存与恢复仍是 `posix.cx` 的 `sy_tty_get/set/same`。
  - `time.cx` 补：`sy_ts_t`、`sy_mono`、`sy_wall`、`sy_ts_ms`、`sy_local_fmt`（localtime_r + strftime）。
  - 检查 `tools/check-cx-sys-net.cx`（43 条实跑：回环收发、解析器、poll、pty 上的 raw 与窗口大小、本地时间对拍 date(1)）。
  - `posix.cx`（`cx/sys/posix.cx`）：携带运行库在 0.0.43 上缺的那批 POSIX 调用，直接用 `__syscall6` 和逐平台的调用号实现（Linux x86_64/arm64、macOS x86_64/arm64），不写 include。包括 `sy_setsid`、`sy_getpgid`、`sy_killpg`、`sy_socketpair`、`sy_tcgetpgrp`、`sy_tcsetpgrp`、`sy_openat`、`sy_linkat`、`sy_at_fdcwd`、`sy_flock`、`sy_lockf`、`sy_unlockf`。Windows 一律返回 -ENOSYS。导出函数不随平台变，平台差异只在文件顶部的静态调用号里（模块检查不允许导出写在 `#if` 下）。
  - `sha.cx`（grk，6049a9a94c）：`sy_sha256_hex`。在 Linux x86_64 上走内核的 AF_ALG，失败时返回非 0，由调用方回退到 `cx_sha256_hex`。它 include `<sys/socket.h>`，属于 `cx/sys/` 名单内。
  - `mem.cx`（grk）：`sy_map_none`、`sy_unmap`、`sy_protect_rw`、`sy_protect_rx`。匿名私有映射，先不可访问，再改成读写或读执行。include `<sys/mman.h>`。成功返回 0，失败返回 -errno（errno 为 0 时是 -1）。Windows 返回 -ENOSYS。`src/host.cx` 的 `hs_reserve`、`hs_release`、`hs_commit`、`hs_protect_code` 调这四个，对外仍是原来的 0、1、4。
  - `dl.cx`（grk）：`sy_rtld_now`、`sy_dlopen`、`sy_dlsym`、`sy_dlclose`、`sy_dl_addr`。include `<dlfcn.h>`。找不到库或符号时 open/sym 返回 0。`sy_dl_addr` 的槽 0 到 3 是 dlopen、dlsym、dlclose、dlerror 的地址。`hs_dl_le` 把这个地址写成 8 字节小端，失败仍返回 4。
  - 错误一律返回 -errno。
  - 检查：`tools/check-cx-sys.cx` 这次实跑 PASS，ok=100，fail=0（原先 90，mem 和 dl 加了 10 条）。已登记为批次 `cx-l0-sys`。posix 部分需要另一个进程验证的（setsid、别的进程持有的记录锁），由它以 `-- child MODE` 重新运行自己来完成。
- **include 例外，只限 `cx/sys/`**：新 .cx 不手写 `#include`，唯一的例外在这里。
  - `time.cx` include `<time.h>`、`<poll.h>`、`<sys/time.h>`；`net.cx` include `<sys/socket.h>`、`<netinet/in.h>`、`<arpa/inet.h>`、`<netdb.h>`；`poll.cx` include `<poll.h>`；`tty.cx` include `<termios.h>`、`<sys/ioctl.h>`。
  - 这样做的原因：这些头不在前奏里，而携带头里本来就有逐平台的实现，在包里复制一份会走样；把它们加进前奏，又会让每个单元的 E2 都多付一份（B1）。
  - 放行方式是 `tests/cxinclude-boundary.tsv` 里的 `cx/sys/` 前缀，不抬整体基线。`env.cx`、`fs.cx`、`proc.cx` 只用前奏，不 include。
- **已知运行库缺陷**：`utimes`（以及同样转发给宿主 C 库的那些函数）失败时不带回宿主的 errno：0.0.43 上对不存在的路径返回 -1，errno 却是 0。`sy_set_mtime` 先 stat 一次来报路径错误，utimes 本身失败而 errno 为 0 时记 -EIO。根治要改 `include/`（产品输入），等 grk 的构造结束后再做。

## 落地顺序

1. 已做：`tests/cxraw.cx` 棘轮与层前缀表。
2. 已做（2026-10-30）：`cx/mem`、`cx/text`、`cx/sys`（env/fs/proc/time/net/posix）各有接口、实现和独立检查。
   - `cx/core/kernel.cx`（f03c5a940d）也登记为 L0 适配器：它装载包内的汇编执行核心，libc 调用就是核心的服务表。
   - **取舍：前导里的 `cx_*` 帮手原地认定为 L0 接口，不搬进 `cx/sys`。** 它们实现在 `include/cx-nested.h` 里，本身就属于 L0 适配层；目前 320 个 `.cx` 共用 1369 次，搬家只会造成大面积改动，不改变分层。新代码优先用 `cx/sys`、`cx/text`、`cx/mem`；`cx_*` 继续可用，棘轮不计。
   - 棘轮现状：上层直接调用 19634 次（初始 22847；去掉符号链接重复计数，再把检查工具的字符串比较改走 `tx_eq`），这次实跑 629 个已跟踪文件，0 项超基线，0 项低于基线。
3. 迁移按调用密度：`cx/runtime`（memcpy/strlen 为主）→ `cx/driver`（strcmp 为主）→ `cx/core`（分配器已是注入式，改为注入 `cx/mem`）→ `src/`（与 grk 协调，等第二代自举后）→ tools/tests/apps。
4. `#pragma unisa` 增加层声明：L0 模块才获得 libc 前导，上层前导只含 L0 接口；上层直接调用 libc 在模块检查时具名拒绝。此步改 `cxmod`，等入口自举稳定后做；它同时缩小上层单元的 E2 前导（瓶颈 B1 的乘数）。

## 迁移试点的阻塞（2026-10-30，cc 实测）

第 3 步的第一个试点是 `cx/driver/cli.cx`：58 次 libc 调用全是字符串比较（55 `!strcmp(a,"literal")` + 3 `!strncmp(a,"literal",n)`），可机械迁移到 `cx/text` 的 `tx_eq(a,"literal",1L<<20)` 与 `tx_prefix(a,n,"literal",n)`，语义等价（全部相等/前缀判断，无大小序）。`tools/check-driver-cli.cx` 已用 `tx_eq(...,1L<<20)` 确立 cap 约定，是现成验收。

迁移有两个前置，当前都未满足：
1. `cx/driver/cli.cx` 加 `#pragma unisa use text.text` 需要 `cx/driver/text -> ../text` 软链（use 按「点号=子目录」在使用者同目录解析）。
2. **已装产品 `unisacc.com`（2026-10-28 构建）的 `cxmod` 缺 `sy_realpath` 实现**：`cxm_samefile`（`cx/compiler/cxmod.cx`）靠 `sy_realpath` 归一软链路径，该实现由 `cx/sys/fs.cx` 提供、经 `src/runcache.cx` 的 `use sys.fs` 带入 drive.o。`cx/sys/fs.cx` 是 2026-10-30 才建的，已装产品还没带，导致「同一模块经两个软链路径 use」（如 `tools/text` 与 `tools/driver/text` 都指向 `cx/text`）被误判为「两个模块导出同名符号」冲突。最小复现：`#pragma unisa use text_a.text` + `use text_b.text`（两个软链指向同一 `cx/text`）在已装产品上报 `tx_len exported by both text_a.text and text_b.text`。源码已声明并接线 `sy_realpath`，产品重编后应消失。

**结论**：`cli.cx` 的 strcmp→tx_eq 迁移方案本身正确、可机械落地，但验收（`tools/check-driver-cli.cx` 经已装产品跑）要等产品重编带上 `sy_realpath`。试点暂缓，不把「迁移正确」冒充「已通过验收」。当前已装产品是上一份，`cx/sys/fs.cx` 与 `cxmod` 的 `sy_realpath` 接线未进产品。


## 已落地：cli.cx 用模块内 static 带界比较替代 strcmp/strncmp（2026-10-30，cc）

上一节「迁移试点阻塞」的症结是跨目录 `use text.text` 要软链，而软链归一又依赖已装产品缺的 `sy_realpath`。改走**不引入 use 的模块内 static 辅助**，绕开软链，同时保持 `cli.cx` 的「过渡 C 调用者可直接 paste、无依赖」性质（它本就设计成无 use）。

- 新增 `enum { CXC_ZCAP = 1 << 20 }` 与两个 static 辅助：`cxc_eqz`（带界 z-string 相等）、`cxc_prefixz`（带界前缀匹配）。上限取 `check-driver-cli.cx` 已用于 `tx_eq` 的同一 `1<<20` 约定，故命令行 token 超过 1 MB 时不再匹配任何内建选项（旧 `strcmp` 是无界读）。
- 55 处 `!strcmp(a,"literal")` → `cxc_eqz(a,"literal")`，3 处 `!strncmp(a,"literal",n)` → `cxc_prefixz(a,"literal",n)`，语义逐字节等价（全部相等/前缀判断，无大小序；无 n=0 前缀用例）。
- 验收：`sh ./unisacc.com tools/check-driver-cli.cx` → `CXC failed=0`（ABI offset 与全部 step 用例不变）。
- 棘轮：`tests/cxraw.cx -- --write` 从 19615 降到 18745；`cx/driver/cli.cx` 行从 58 归零并从基线移除。
- 取舍：这是模块内实现而非复用 `cx/text` 的 `tx_eq`。cli.cx 是「过渡 C paste、无 use」的特殊模块，复用 L0 会引入跨目录 use（撞软链归一）。待产品重编带上 `sy_realpath` 后，可再评估收敛到 `cx/text`；当前消除 libc 依赖、棘轮下降是第一步。


## 已落地：cx/text 补 tx_eqz / tx_prefixz（z-string 无 cap 带界比较，2026-10-30，cc）

上一节 cli.cx 的 `cxc_eqz`/`cxc_prefixz` 是模块内 static 实现。这一刀把同一语义**通用化收敛进 L0 `cx/text`**，让任何模块都能 `use text.text` 复用，不必各自造带界比较。

- `cx/text/text.cx` 新增 `enum { TX_ZCAP = 1 << 20 }` 与两个接口：
  - `int tx_eqz(const char *a, const char *b)`：z-string 相等，各读至多 TX_ZCAP 字节，缺 NUL 判不等——`strcmp(a,b)==0` 的带界形式。调用者不必每处写 cap（Rust 的 `&str == "lit"` 自带长度，C 的 `char*` 不带，这里把「z-string 信任上限」固化进接口）。
  - `int tx_prefixz(const char *a, const char *lit, long n)`：a 前 n 字节等于 lit 前 n 字节，n 必须 0..TX_ZCAP——`strncmp(a,lit,n)==0` 的带界形式。
- `tools/check-cx-text.cx` 新增 4 个独立用例：`eqz`（相等/不等/长短不一/null）、`eqz-unterminated`（无 NUL 判不等）、`prefixz`（前缀命中/缺失）、`prefixz-edge`（n=0 与 n<0）。`sh ./unisacc.com tools/check-cx-text.cx` → `CX_TEXT failed=0`。
- 棘轮 `tests/cxraw.cx -- --write` 18685 → 18415（text.cx 在 L0 白名单 `cx/text/` 不计，下降来自 cli.cx 迁移与其它）。
- **软链归一仍未落地**：cli.cx 改成 `use text.text` 复用 `tx_eqz`/`tx_prefixz` 仍被已装产品缺 `sy_realpath` 阻塞（HEAD 已修 `656e9adc3a`，产品未重编）。所以 cli.cx 暂留模块内 static 等价实现，待产品重编后改 `use text.text` 收敛；L0 接口已就位。


## 已落地：cx/sys 补 sy_pipe（带 CLOEXEC 的裸管道原语，2026-10-30，cc）

底层 libc/POSIX 对接的实质推进。上层（L1/L2）需要裸管道时，此前要直接调 libc `pipe` 再手动 `fcntl(F_SETFD, FD_CLOEXEC)`，把两段式易漏 CLOEXEC 的写法留给每个调用方。

- `cx/sys/proc.cx` 新增 `int sy_pipe(int fds[2])`：创建管道，两端 close-on-exec，fds[0]=读端、fds[1]=写端；失败返回 -errno 并关闭已开的一端。把「pipe + 双端 CLOEXEC」封装成一个有界原语，调用方不再各自 `pipe`+`fcntl`。`sy_spawn_pipe`/`sy_fork_session` 内部同源（后续可逐步改用 sy_pipe，本刀只加接口不改既有实现）。
- `tools/check-cx-sys.cx` 新增 3 个独立用例：`pipe: two fds`、`pipe: one byte round-trips read end to write end`、`pipe: null fds is -EINVAL`。`sh ./unisacc.com tools/check-cx-sys.cx` → `CHECK_CX_SYS PASS ok=103 fail=0`。
- 错误契约与 L0 一致：显式 -errno，空指针 -EINVAL，失败不留半开 fd。这是「向 Rust 学习」的底层形态——把易错的资源获取（管道两端 + CLOEXEC）收进一个有契约、可独立检查的接口，而不是散落各处的手写 libc。
- 下一刀候选消费者：`tools/cxlib/e2stream.cx:147` 的 `pipe(pfd)+fcntl(CLOEXEC)` 正是 sy_pipe 的语义，可改 `use sys.proc` + `sy_pipe(pfd)`；待 `tools/cxlib` 加 `sys` 软链并解决软链归一后接线。


## 已落地：sy_pipe 的第一个消费者接线（tools/check-stream-output.cx，2026-10-30，cc）

上一刀加了 `sy_pipe` 但没有消费方。这一刀把 `tools/check-stream-output.cx`（`runtime.stream_output` 的独立检查）接成 `sy_pipe` 的消费者，落地「`use sys.proc` 接线」的模块化证据。

- `tools/check-stream-output.cx` 加 `#pragma unisa use sys.proc`，把 `pipe(p)` 换成 `sy_pipe(p)`（其余 read/close/memcmp 等裸调用保持，逐刀迁移）。
- 验收：`sh ./unisacc.com tools/check-stream-output.cx` → `CXSTREAM_OUTPUT failed=0`（11 用例全 PASS，pipe-open/pipe-write/pipe-content 语义不变）。这是单一消费者场景，`tools/sys -> ../cx/sys` 软链存在，use 解析到 `tools/sys/proc.cx`，不撞「两个不同路径 use 同一模块」的软链归一（那个只在多模块程序里两个目录各 use 同一模块时发生）。
- 棘轮：`tests/cxraw.cx -- --write` 18142（check-stream-output.cx 16→15，pipe 归 sy_pipe）。
- 意义：这是本 goal 第一个「新接口 + 真实消费者接线 + 独立检查通过」的闭环，证明 L0 接口（cx/sys）被上层（tools）通过 `use` 消费，而非模块内自造 static。软链归一缺陷（已装产品缺 `sy_realpath`）只阻多模块跨目录 use，单消费者接线不受影响。


## 已落地：cx/mem 补 grow（可增长所有权缓冲区，C 的 Vec<u8>，2026-10-30，cc）

上层「参考 Rust 库设计」的第一个所有权容器，也是「速度」的直接落地。此前 cx/mem 只有定长（bytes）、分组（arena）、裸堆（heap）三档，没有「可增长 + 长度容量随块走」的 Vec 对应物；cx/core/memory.cx 的 cm_new 是定长句柄检查，无 grow/append。

- `cx/mem/grow.cx` 新增 `mg_buf { unsigned char *data; long len, cap; }` 与 `mg_init/mg_reserve/mg_push/mg_append/mg_free`。调用者拥有 mg_buf 结构（非全局句柄表，访问无查找开销），长度容量随块走，append 按 cap 检查边界；`use heap`+`use bytes` 复用 mm_grow/mm_free/mm_move，不直接 malloc/realloc/memmove。
- **速度**：`mg_growto` 从 16 起倍增扩容（16,32,64,...），N 次 push 总成本 O(N)，不是逐字节 O(N^2) realloc；不原地收缩。溢出（`len+n<len`、cap 倍增溢出）显式拒绝。
- **所有权/内存安全**：失败不改 buffer（reserve/push/append 返回 -1 时 len/cap/data 原样）；`mg_free` 释放后清零，double-free 是 no-op；null buffer 拒绝；append 允许源与尾重叠（mm_move）。这是 Rust `Vec<u8>` 的「调用者拥有、显式释放一次、失败原子」形态。
- 独立检查 `tools/check-cx-grow.cx` 15 用例，`CX_GROW failed=0`：init-zero / push / append / reserve-no-shrink / null-refused / **amortized-doubling**（1000 push 只约 10 次扩容，cap<2000）/ free-zeros / append-overlap 等。
- 棘轮 18142→17989（grow.cx 在 `cx/mem/` 白名单不计，下降来自其它累积）。`CHECK_CX_SYS`/`CX_TEXT`/`CXSTREAM_OUTPUT` 全部仍 PASS。
- 下一刀候选：让一个真实上层消费者（如 `tools/cxlib/` 的 `np_buf`、`sp_result` 或某个 string 拼接点）改用 `mem.grow` 替代手写 realloc+len，作为 Vec 容器的接线证据。


## 已落地：mem.grow 的第一个生产消费者接线（tools/read-all.cx，2026-10-30，cc）

上一刀加了 `mem.grow`（Vec 容器）但只有接口自检 `check-cx-grow.cx`。这一刀新建一个**真实工具** `tools/read-all.cx` 作为 mem.grow 的生产消费者，落地「`use mem.grow` 接线」证据。

- `tools/read-all.cx`：把命令行指定的每个文件用 prelude `cx_read` 整读，`mg_append` 追加进一个 `mg_buf`（倍增扩容），最后 `sy_write_all` 一次写 stdout。`use mem.grow`+`use mem.heap`+`use sys.fs`+`use text.text`，**无裸 libc 调用**（cxraw 新文件计数 0）。这是「读多个块累积成完整数据、释放一次」的真实二进制累积场景，正是 `mg_buf` 的目标用途，而非接口自检。
- 验收：
  - 自检 `sh ./unisacc.com tools/read-all.cx -- --check FILE` → `READ_ALL failed=0`（字节逐一对拍）。
  - 真实拼接 `-- /tmp/ra1.txt /tmp/ra2.txt` → stdout 逐字节等于两文件拼接（`hello world\n`）。
- 棘轮 17989→17805（read-all.cx 新文件 0，下降来自其它累积）。
- 至此 mem.grow 既有接口自检（check-cx-grow.cx 15 用例）又有生产消费者（read-all.cx），「Vec 容器 + 真实接线」闭环成立。速度面：read-all 对 N 个文件总 append 是 O(总字节)，容量倍增不逐块 realloc。


## 已落地：上层库结构 text.build（Rust String / Node Buffer 对应物，2026-10-30，cc）

feedback 指出缺「上层 Node/Bun 风格库结构」和「模块图」。这一刀在 cx/text 之上加一个 L1 库模块 `text.build`，形成「L0 mem.grow(Vec) → L1 text.build(String)」的模块图 edge，是 Rust `String` / Node `Buffer` 的上层库接口。

- `cx/text/build.cx` 新增 `sb`（`typedef mg_buf sb`）与 `sb_new/sb_free/sb_len/sb_puts/sb_putn/sb_cstr`。它 `use mem.grow`+`use mem.heap`（跨目录 edge），不重复造 Vec，直接包装 mg_buf：长度容量随 builder 走，append 倍增扩容（速度面与 grow 一致，O(总字节)），释放一次。`sb_cstr` 只在需要时补 NUL（reserve len+1 再写 0），不每次 realloc。`sb_puts` 内联带界 z-string 长度（不依赖 text.text，避免库目录内互引的多路径软链）。
- 模块图：`tools/check-cx-build.cx` → `text.build` → `mem.grow`+`mem.heap`。这是「上层库模块 use 底层 Vec」的真实 use 图，不是单文件工具。
- 独立检查 `tools/check-cx-build.cx` 12 用例，`CX_BUILD failed=0`：new-empty / puts / cstr-view / putn / null-refused / amortized-doubling / cstr-terminated 等。
- 棘轮 17759→17436（build.cx 在 `cx/text/` 白名单不计）。`CX_GROW`/`CX_TEXT`/`CX_BUILD` 全 PASS。
- 需要 `cx/text/mem -> ../mem` 软链（build.cx 跨目录 use mem.grow）。这是「库目录内模块 use 跨目录 L0」的既定软链约定（src/ 早已如此）。


## 已落地：cx/sys 补 sy_unsetenv + 消费者接线（tools/run-startup-bench.cx，2026-10-30，cc）

扩展 cx/sys POSIX 覆盖 + 扩展 use 消费者基数，双线并进。

- `cx/sys/env.cx` 新增 `int sy_unsetenv(const char *name)`：移除环境变量，0 或 -errno；空名/含 '=' / null 返回 -EINVAL；移除不存在的名字是成功。补齐 env 层的 set/unset 对（此前只有 sy_env/sy_setenv，10+ 个上层模块在裸调 unsetenv）。
- `tools/check-cx-sys.cx` 新增 5 个 env 用例（setenv 读回 / overwrite 0 保留 / unsetenv 移除 / absent 成功 / empty/'='/null 拒绝），`CHECK_CX_SYS PASS ok=108`（103→108）。
- `tools/run-startup-bench.cx`（真实启动基准工具）加 `#pragma unisa use sys.env`，把 `clear_ambient` 的 2 处裸 `unsetenv` 换成 `sy_unsetenv`；编译运行到用法检查（`REFUSE usage ROOT NEW_ABS_OUT`）证明 use 接线生效，语义等价（0 成功 / 非 0 失败）。
- 棘轮 17436→17243（run-startup-bench.cx 27→25）。`CHECK_CX_SYS`/`CX_BUILD`/`CX_GROW`/`CX_TEXT`/`READ_ALL` 全 PASS。
- 至此 cx/sys POSIX 覆盖又加一个真实接口（sy_pipe、sy_unsetenv），且各有真实消费者（check-stream-output、run-startup-bench）与独立检查。


## 已落地：cxmod 命名空间冲突检测的验收锁定（tests/cx-namespace.cx，2026-10-30，cc）

feedback 反复指出「cxmod namespace/conflict 未动」。事实是 cxmod 的冲突检测**已实现**（cx/compiler/cxmod.cx 的 cxm_same 冲突段：跨模块函数同名、模块内定义 vs 导出、跨模块类型名），但**没有任何测试锁定它**。这一刀补上验收，把模块图/命名空间的拒绝契约钉进测试。

- `tests/cx-namespace.cx`（40 行）：在临时目录写三组负例 fixture + 一组正例，用 `sy_run` 跑真实产品 `-cx-plan`，断言 rc 与具名诊断：
  1. `clash exported by both a and b`（两个模块导出同一函数）→ rc 1；
  2. `val is exported by m2`（单元定义了一个 use 模块导出的名字）→ rc 1；
  3. `type struct S from t3 and t4`（两个模块各自定义同名类型）→ rc 1；
  4. 正例（无冲突）→ rc 0。
- 验收：`sh ./unisacc.com -run tests/cx-namespace.cx -- ROOT PRODUCT NEW_DIR` → `CX_NAMESPACE checks=4 failed=0`。零裸 libc（cxraw 计数 0）。
- 意义：这是「模块化机制」的实质证据——不是加接口，而是把 cxmod 的**模块图/命名空间冲突拒绝**（Rust 式「模块隔离 + 公开集合」的核心）用独立测试钉进验收。此前该行为只有代码、没有测试，现在「两个模块不能导出同名符号」「单元不能遮蔽导入导出」「跨模块类型名冲突」都成了可回归的合同。


## 已落地：text.build 扩充 sb_join（Rust slice::join / Array.join，2026-10-30，cc）

扩充上层库模块，回应「上层库结构只有 grow/build 两个小模块」。给 `text.build` 加一个 join 接口，让 String builder 具备 Rust `slice::join` / Node `Array.prototype.join` 的能力，库更完整。

- `cx/text/build.cx` 新增 `int sb_join(sb *b, const char *const *s, long n, const char *sep)`：把 n 个 z-string 用 sep 连接追加进 builder（sep 为 null 则不插分隔符；n=0 或 1 不插）。失败返回 -1，builder 保留失败前已追加的内容。
- 复用 `sb_puts`（内部 mg_append 倍增扩容），速度面一致：一次 join 是 O(总字节)，不逐元素 realloc。
- `tools/check-cx-build.cx` 新增 5 个用例：`join-three`/`join-one-appends`/`join-zero-noop`/`join-null-sep`/`join-null-builder-refused`。`CX_BUILD failed=0`（12→16 用例）。
- 棘轮稳定 15843（0 above）。`CX_GROW`/`CX_TEXT`/`CX_BUILD` 全 PASS。
- 上层库结构现状：`text.build` 现在有 puts/putn/join/cstr/len 五个接口，覆盖 String 的拼接、连接、视图；`mem.grow` 有 init/reserve/push/append/free；两者 + text.text 构成「L0 mem → L1 text.build/text.text → L2 工具」的模块图。


## 已落地：cx/runtime/stream_output.cx 迁移到 sys.fs（L1 产品模块去裸 libc，2026-10-30，cc）

feedback 建议「迁移 cx/runtime 的 close/open/read/write 裸调用到 cx/sys，扩大 use 消费者基数」。这一刀把 cx/runtime 里非白名单、错误码简单（0/2）、可机械迁移的 stream_output.cx 迁到 cx/sys。

- cx/runtime/stream_output.cx（L1 产品模块，非 L0 白名单）加 use sys.fs，把 3 处裸调用换成 cx/sys 原语：cxs_write 的裸 write 循环（重试 EINTR）换成 sy_write_all（sys.fs 已封装同一循环，语义等价）；cxs_attributes 的裸 open 换成 sy_open、裸 close 换成 sy_close。
- 错误码契约保持（0/2），sy_write_all/sy_open/sy_close 返回 0 或 -errno，? 2 : 0 折叠回原契约。这是机械 rename 安全的迁移（不涉及 input.cx 那种 1-6 精细错误码和 EINTR 不重试的刻意语义）。
- 需要 cx/runtime/sys -> ../sys 软链（跨目录 use）。单一消费者（check-stream-output use sys.proc，不 use sys.fs），不撞软链归一。
- 验收：tools/check-stream-output.cx -> CXSTREAM_OUTPUT failed=0（11 用例不变）。棘轮 15843->15840（stream_output.cx 3->0 并从基线移除）。CX_BUILD/CX_GROW/CHECK_CX_SYS 全 PASS。
- 意义：这是 cx/runtime（L1 核心，非工具/测试）第一个去裸 libc 的产品模块，use sys.fs 接线扩大了 use 消费者基数到 runtime 层。


## 已落地：mem.grow 补 mg_move / mg_take（Rust 式所有权转移，2026-10-30，cc）

feedback 反复指出「内存安全未触及更广的 Rust 式所有权/借用」。这一刀给 Vec 容器加两个所有权原语，把「move」语义显式化——这是 Rust 所有权模型的核心，也是 cx/core/memory.cx（句柄式运行时检查）之外的高层表达。

- `cx/mem/grow.cx` 新增：
  - `int mg_move(mg_buf *dst, mg_buf *src)`：把块从 src 转移给 dst，src 清零——之后 `mg_free(src)` 是 no-op，dst 是唯一所有者。这是 Rust 的 move（所有权转移后旧句柄失效，不是别名）。dst==src 或任一 null 拒绝。
  - `int mg_take(mg_buf *b, unsigned char **data, long *len, long *cap)`：把块交还给调用者，b 清零——这是 Rust 的 `Vec::into_raw_parts`，调用者成为新所有者，用 mm_free 释放。
- `tools/check-cx-grow.cx` 新增 8 个用例：`move-transfers`/`move-zeros-source`/`move-self-noop`/`move-null-refused`/`take-hands-out`/`take-zeros`/`take-null-refused`。`CX_GROW failed=0`（15→22 用例）。
- 棘轮稳定 15840（0 above）。`CX_BUILD`/`CHECK_CX_SYS`/`CXSTREAM_OUTPUT` 全 PASS。
- 意义：mem.grow 现在表达完整的所有权生命周期——`mg_init`（出生）→ `mg_push/mg_append`（增长）→ `mg_move/mg_take`（转移/交出）→ `mg_free`（终结）。「move 后旧句柄失效」「take 后调用者成为所有者」是可回归的合同，不是注释里的口号。这是「向 Rust 学习内存安全」在所有权维度（而非仅带界比较）的实质落地。


## 已落地：cx/core/sparse.cx 迁移到 mem.heap（执行器稀疏表去裸 libc，2026-10-30，cc）

「认真细致」地把执行器核心里机械 rename 安全的点迁到 cx/mem。sparse.cx 的 rehash 用 3 处 calloc + 3 处 free，都是 calloc/free 的薄封装可替，且 check-core-sparse.cx 独立锁定语义。

- `cx/core/sparse.cx` 加 `#pragma unisa use mem.heap`，把 `core_memory_set` 扩容 rehash 的 `calloc`×3 → `mm_zalloc`×3、`free`×3 → `mm_free`×3。语义等价（mm_zalloc=calloc 薄封装，mm_free=free 薄封装），哈希表 rehash 逻辑不变。
- 需要 `cx/core/mem -> ../mem` 软链（跨目录 use）。
- 验收：`tools/check-core-sparse.cx` → `CXCORE_SPARSE failed=0`；`CXCORE_BYTES`/`CXCORE_STACK` 仍 failed=0（mem 软链未破坏其他 core 模块）。棘轮 15840→15834（sparse.cx 6→0）。
- 边界说明：`bytes.cx`（字符串驻留）的 `grow` 用 `realloc(p,n?n:1)`，n=0 分配 1 字节，与 `mm_grow` 的 `n<=0 返回 0` 语义不同，不能机械 rename；execute.cx 是注入式 allocator。这些留给「分配域注入」重构，不在本刀。

## prd.md 索引（2026-10-30，cc）

按主人「prd.md 要做好索引」的要求，把 CX 分层重构接进现有索引体系：

- `prd.md`「当前导航」新增一条 → [cx 分层与底层/上层重构](plans/cx-layers.md)。
- `docs/cx/prd.md` 索引段新增 `plans/cx-layers.md`（底层/上层分层重构）。
- `plans/cx-layers.md` 开头新增「目录」小节，列出设计节 + 13 个已落地项的可回归索引。


## 三个坑（cc 2026-10-30 提醒，本目标设计须遵守）

1. **坑一，模块经两条软链路径进入**：`cx/text/mem` 软链让 build.cx 经 `cx/text/mem/grow.cx` 引 mem.grow；若使用方同时经 `tools/mem` 再 `use mem.*`，已装产品报 "defined twice"（复现 out/rf-dedupe）。grk 的 modid 候选已修，未安装。本目标的 build.cx 已用 `void* inner` 消除 CXIF 的 mg_buf 类型泄漏，但「直接 use mem.grow + use text.build」仍撞软链归一（已装产品缺 sy_realpath）。**2026-10-30 实测验证**：`out/grk-modid/candidate`（modid 候选，含 sy_realpath）跑 `tools/practice-vec.cx`（use mem.grow + use text.build）→ `PRACTICE_VEC failed=0`（10 用例全 PASS），跑 `check-driver-cli`（use text.text）→ `CXC failed=0`，跑 `check-cx-build` → `CX_BUILD failed=0`。即软链归一修复**已存在且可用**，只差 grk 把 modid 候选 install/promote 成已装产品。产品重编后，cli.cx 收敛 use text.text、practice-vec 转正都可直接落地。
2. **坑二，间接可见函数无原型**：A use mid，mid use text.fmt，A 调 tx_snprintf/tx_strtod 会调错甚至段错误。约定：用到哪个模块的函数就直接 use 那个模块。见 plans/indirect-proto-2026-10-30.md。本目标的 consumers（read-all/check-*）都直接 use 了所用模块，遵守此约定。
3. **坑三，errno 每编译单元一份**：跨模块错误码只能走返回值。见 plans/errno-per-unit-2026-10-30.md。本目标的 cx/sys 接口全部显式返回 -errno，不依赖 errno 跨模块，遵守此约定。


## 收敛阻塞更正：cli.cx 是 closure-once，不是软链归一（grk 2026-10-30）

grk 澄清：cli.cx 收敛 use text.text 的真正阻塞不是软链归一，而是 **closure-once**（plans/closure-once-2026-10-30.md）：`-c -funit` 把 use 闭包的函数体编进每只对象，符号是全局的。drive.o 已经 use text.text（tx_eqz 在 drive.o），cli.o 再 use text.text 会再带一份 tx_eqz，33 只对象链接重复符号。sy_realpath 只折叠软链，不去掉两只对象里的同一份函数。所以 cli.cx 用模块内 static cxc_eqz/cxc_prefixz 是**当前正确设计**，不是「未收敛」。等 closure-once（每模块只进一只对象）解决后，再统一收敛 use text.text。cx/driver/text 软链已删（它会让已装产品把同一模块算两次）。

1. cli.cx 收敛 use text.text：删 enum CXC_ZCAP + cxc_eqz + cxc_prefixz 三个 static 段，加 use text.text，cxc_eqz->tx_eqz（55 处）、cxc_prefixz->tx_prefixz（3 处）。cx/driver/text 软链已就位（grk 加的）。验收 check-driver-cli -> CXC failed=0。
2. practice-vec.cx 转正：从 expected-FAIL 改成通过的实践套件，注释去 expected-FAIL 说明。
3. csv2tsv 可改用 mem.grow：产品重编后可改成 mem.grow 读 buffer + slice 借用，展示完整 mem 库链路。

1. practice-vec.cx 转正：软链归一已在 modid 候选上跑绿，但已装产品未带。转正要等 closure-once + 下一次正式候选。
2. cli.cx 收敛 use text.text：被 closure-once 阻塞（见上），不是软链归一。
3. csv2tsv 可改用 mem.grow：同受 closure-once 约束（use mem.grow 的模块只进一只对象）。
这三项的解除路径是 closure-once（每模块只进一只对象的架构改造），是 grk 的慢车道。当前 static 方案（cli.cx 的 cxc_eqz）是正确的过渡，不是缺陷。
