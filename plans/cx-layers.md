# CX 分层：底层对接包与上层（2026-10-30，cc）

主人口径：CX 底层要有 libc 与 POSIX 的专用对接包；上层不再直接用 malloc、memcpy 这类操作。

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
  - `apps/csih` 交给 csih 车道（unisacc-00），缺的 socket、tty、signal、poll 等接口要补进 `cx/sys`；
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
