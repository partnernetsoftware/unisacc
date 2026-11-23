# 全仓 `sy_-` 返回正数/非负描述符函数被误作 `== 0` 比较缺陷普查报告

## (1) 普查总览与核心结论

- 扫描工具: `~/.claude/jobs/2798e440/tmp/sy-retval-census/sy-retval-scan.cx`
- 命中明细表: `~/.claude/jobs/2798e440/tmp/sy-retval-census/hits.tsv`
- 扫描范围: 全仓 `cx/`、`src/`、`ext/`、`tools/`、`tests/` 共 2437 个 `.cx`/`.c` 源文件
- 覆盖目标函数: 65 个返回正数(`POS_STRICT`, 24 个)、非负描述符(`FD_NONNEG`, 19 个)或计数/就绪数(`COUNT_OR_EOF_ZERO`, 22 个)的 `sy_-` 函数, 共计扫描到 2634 处调用点
- 检出与 `0` 比较或布尔非/布尔判定的命中点: 共 70 处
  - `TRUE_BUG` (真缺陷): 8 处 (`A类` 0 处、`B类` 0 处、`C类` 8 处)
  - `FALSE_POSITIVE_VAR_REASSIGNED` (控制流中变量来自另一 `0=成功` 调用): 2 处
  - `FALSE_POSITIVE_EOF_OR_TIMEOUT` (`sy_read`/`sy_poll`/`sy_wait_status(nohang=1)`/`sy_dir_next` 合法检测 EOF/超时/运行中/目录尾): 60 处

## (2) 全部 `TRUE_BUG` 真缺陷详单 (8 处, 均属 `C类(工具与测试)`, `A类` 与 `B类` 零缺陷)

| 序号 | 位置 (`file:line`) | 车道类别 | 函数名 | 误用模式 | 缺陷根因与运行时后果 | 修复方案与补丁状态 |
|---|---|---|---|---|---|---|
| (1) | `tools/handoff-check.cx:182` | C类(工具与测试) | `sy_realpath` | `!sy_fn(...)` | 将 sy_realpath 当作返回指针/布尔(写 !sy_realpath(uc, abs, sizeof abs)); 失败返回 -errno (<0) 时 !(-errno) 为 0 不报错, 继续用未初始化 abs 建损坏软链; 应改 sy_realpath(...) < 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-handoff-check-realpath.patch` |
| (2) | `tools/prerun-drive.cx:1342` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-prerun-drive-realpath.patch` |
| (3) | `tools/prerun-drive.cx:2159` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath("./unisacc.com", qp, sizeof qp) == 0 恒假(成功返回长度 >=1), 致 batch-domain-index.tsv 未物化时回退调用 tools/derived-query.cx 的整个分支成死代码; 应改 > 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-prerun-drive-realpath.patch` |
| (4) | `tools/prerun-drive.cx:2554` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-prerun-drive-realpath.patch` |
| (5) | `tools/prerun-drive.cx:2589` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-prerun-drive-realpath.patch` |
| (6) | `tools/prerun-drive.cx:2606` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 | 已出补丁 `~/.claude/jobs/2798e440/tmp/sy-retval-census/fix-prerun-drive-realpath.patch` |
| (7) | `tools/release-chain.cx:1185` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(..., pdir/cabs, ...) == 0 恒假(成功返回长度 >=1), 致候选路径 /.cache/ 判定分支永不进入(已由 R78 子窗口修复) | 已由 R78 子窗口修复 (`sy_realpath(...) > 0`) |
| (8) | `tools/release-chain.cx:1202` | C类(工具与测试) | `sy_realpath` | `sy_fn(...) == 0` | sy_realpath(..., pdir/cabs, ...) == 0 恒假(成功返回长度 >=1), 致候选路径 /.cache/ 判定分支永不进入(已由 R78 子窗口修复) | 已由 R78 子窗口修复 (`sy_realpath(...) > 0`) |

## (3) 目标 `sy_-` 函数返回值契约表与全仓调用频次 (共 65 个函数, 排除 `0=成功` 类函数)

| 分类 | 函数名 | 所属模块 | 返回值契约 | 全仓调用数 |
|---|---|---|---|---|
| `POS_STRICT` | `sy_realpath` | cx.os (sys.fs) | 成功返回规范绝对路径字节长度 >= 1, 失败返回 -errno < 0, 永不返回 0 | 525 |
| `POS_STRICT` | `sy_abspath` | cx.os (os.osx) | 成功返回规整绝对路径字节长度 >= 1, 失败返回 -errno < 0, 永不返回 0 | 10 |
| `POS_STRICT` | `sy_self_path` | cx.os (sys.fs) | 成功返回本进程可执行文件路径长度 >= 1, 失败返回 -errno < 0, 永不返回 0 | 7 |
| `POS_STRICT` | `sy_readlink` | cx.os (sys.fs) | 成功返回符号链接目标字节长度 >= 1, 失败返回 -errno < 0, 永不返回 0 | 32 |
| `POS_STRICT` | `sy_homedir` | cx.os (os.osx) | 成功返回家目录路径长度 >= 1, 未设或空返回 -ENOENT < 0, 永不返回 0 | 7 |
| `POS_STRICT` | `sy_hostname` | cx.os (os.osx) | 成功返回主机名长度 >= 1, 未设或失败返回 -ENOENT/-ERANGE < 0, 永不返回 0 | 4 |
| `POS_STRICT` | `sy_utc_fmt` | cx.os (os.utc) | 成功固定返回长度 20, 失败返回 -ERANGE/-EINVAL < 0, 永不返回 0 | 4 |
| `POS_STRICT` | `sy_local_fmt` | cx.os (sys.time) | 成功返回格式化字节长度 >= 1, 失败返回 -errno < 0 | 9 |
| `POS_STRICT` | `sy_pid` | cx.os (sys.proc) | 返回当前进程 PID >= 1, 永不返回 0 | 333 |
| `POS_STRICT` | `sy_ppid` | cx.os (sys.proc) | 成功返回父进程 PID >= 1, Windows 无实现返回 -ENOSYS < 0, 永不返回 0 | 8 |
| `POS_STRICT` | `sy_setsid` | cx.os (sys.posix) | 成功返回新会话 ID >= 1, 失败返回 -errno < 0, 永不返回 0 | 6 |
| `POS_STRICT` | `sy_getpgid` | cx.os (sys.posix) | 成功返回进程组 ID >= 1, 失败返回 -errno < 0, 永不返回 0 | 44 |
| `POS_STRICT` | `sy_tcgetpgrp` | cx.sys (sys.posix) | 成功返回终端前台进程组 ID >= 1, 失败返回 -errno < 0, 永不返回 0 | 2 |
| `POS_STRICT` | `sy_spawn` | cx.os (sys.proc) | 成功返回子进程 PID >= 1, 失败返回 -errno < 0, 永不返回 0 | 30 |
| `POS_STRICT` | `sy_spawn_inherit` | cx.os (sys.proc) | 成功返回子进程 PID >= 1, 失败返回 -errno < 0, 永不返回 0 | 7 |
| `POS_STRICT` | `sy_fork_session` | cx.os (sys.proc) | 成功返回新会话子进程 PID >= 1, 失败返回 -errno < 0, 永不返回 0 | 22 |
| `POS_STRICT` | `sy_spawn_pipe` | cx.os (sys.proc) | 成功返回子进程 PID >= 1, 失败返回 -errno < 0, 永不返回 0 | 98 |
| `POS_STRICT` | `sy_tcp_port` | ext.net (net.tcp) | 成功返回监听端口号 >= 1, 失败返回 -errno < 0 | 19 |
| `POS_STRICT` | `sy_ncpu` | cx.sys (sysx.process) | 返回 CPU 核数 >= 1, 永不返回 0 | 5 |
| `POS_STRICT` | `sy_page_size` | cx.sys (sys.sysinfo) | 返回系统页大小 >= 4096, 永不返回 0 | 2 |
| `POS_STRICT` | `sy_sa_inet4` | cx.sys (sys.netraw) | 成功固定返回地址长度 16, 失败返回 -EINVAL/-ERANGE/-ENOSYS < 0 | 17 |
| `POS_STRICT` | `sy_sa_inet6` | cx.sys (sys.netraw) | 成功固定返回地址长度 28, 失败返回 -EINVAL/-ERANGE/-ENOSYS < 0 | 4 |
| `POS_STRICT` | `sy_sa_unix` | cx.sys (sys.netraw) | 成功返回地址长度 2+len+1 >= 4, 失败返回 < 0 | 11 |
| `POS_STRICT` | `sy_sa_ip` | cx.sys (sys.netraw) | 成功返回 IP 字节数 4 或 16, 失败返回 < 0 | 7 |
| `FD_NONNEG` | `sy_open` | cx.os (sys.fs) | 成功返回文件描述符 fd >= 0, 失败返回 -errno < 0 | 218 |
| `FD_NONNEG` | `sy_open_new` | cx.os (sys.fs) | 成功返回新建文件描述符 fd >= 0, 已存在返回 -EEXIST < 0 | 79 |
| `FD_NONNEG` | `sy_open_read` | cx.os (sys.fs) | 成功返回只读文件描述符 fd >= 0, 失败返回 -errno < 0 | 95 |
| `FD_NONNEG` | `sy_openat` | cx.os (sys.posix) | 成功返回文件描述符 fd >= 0, 失败返回 -errno < 0 | 13 |
| `FD_NONNEG` | `sy_dup` | cx.sys (sysx.fd) | 成功返回新描述符 fd >= 0, 失败返回 -errno < 0 | 7 |
| `FD_NONNEG` | `sy_dup2` | cx.sys (sysx.fd) | 成功返回目标描述符 newfd >= 0, 失败返回 -errno < 0 | 18 |
| `FD_NONNEG` | `sy_mkstemp` | cx.os (os.osx) | 成功返回临时文件描述符 fd >= 0 (与 sy_mkdtemp 返回 0 不同), 失败返回 -errno < 0 | 8 |
| `FD_NONNEG` | `sy_sock_open` | cx.sys (sys.netraw) | 成功返回套接字描述符 fd >= 0, 失败返回 -errno < 0 | 26 |
| `FD_NONNEG` | `sy_sock_accept` | cx.sys (sys.netraw) | 成功返回已接受连接描述符 fd >= 0, 失败返回 -errno < 0 | 12 |
| `FD_NONNEG` | `sy_tcp_listen` | ext.net (net.tcp) | 成功返回监听套接字 fd >= 0, 失败返回 -errno < 0 | 9 |
| `FD_NONNEG` | `sy_tcp_listen_at` | ext.net (net.tcp) | 成功返回监听套接字 fd >= 0, 失败返回 -errno < 0 | 21 |
| `FD_NONNEG` | `sy_tcp_accept` | ext.net (net.sock) | 成功返回连接套接字 fd >= 0, 超时 -ETIMEDOUT < 0, 失败 -errno < 0 | 11 |
| `FD_NONNEG` | `sy_tcp_accept_from` | ext.net (net.tcp) | 成功返回连接套接字 fd >= 0, 超时 -ETIMEDOUT < 0, 失败 -errno < 0 | 9 |
| `FD_NONNEG` | `sy_tcp_connect` | ext.net (net.tcp) | 成功返回连接套接字 fd >= 0, 失败返回 -errno < 0 | 24 |
| `FD_NONNEG` | `sy_tcp_connect_to` | ext.net (net.tcp) | 成功返回连接套接字 fd >= 0, 超时 -ETIMEDOUT < 0, 失败 -errno < 0 | 9 |
| `FD_NONNEG` | `sy_tcp_dial` | ext.net (net.tcp) | 成功返回连接套接字 fd >= 0, 失败返回 -errno 或 EAI < 0 | 13 |
| `FD_NONNEG` | `sy_udp_bind` | ext.net (net.udp) | 成功返回绑定 UDP 套接字 fd >= 0, 失败返回 -errno < 0 | 7 |
| `FD_NONNEG` | `sy_unix_listen` | ext.net (net.unix) | 成功返回监听 Unix 域套接字 fd >= 0, 失败返回 -errno < 0 | 13 |
| `FD_NONNEG` | `sy_unix_connect` | ext.net (net.unix) | 成功返回连接 Unix 域套接字 fd >= 0, 失败返回 -errno < 0 | 21 |
| `COUNT_OR_EOF_ZERO` | `sy_read` | cx.sys (sys.fs) | 成功返回读取字节数 > 0, 0 表示 EOF, < 0 表示 -errno | 245 |
| `COUNT_OR_EOF_ZERO` | `sy_read_once` | cx.sys (sys.fs) | 成功返回单次读取字节数 > 0, 0 表示 EOF, < 0 表示 -errno | 14 |
| `COUNT_OR_EOF_ZERO` | `sy_pread` | cx.sys (sys.posix) | 成功返回偏移读取字节数 > 0, 0 表示 EOF, < 0 表示 -errno | 6 |
| `COUNT_OR_EOF_ZERO` | `sy_write` | cx.sys (sys.fs) | 成功返回单次写入字节数 >= 0, < 0 表示 -errno | 7 |
| `COUNT_OR_EOF_ZERO` | `sy_pwrite` | cx.sys (sys.posix) | 成功返回偏移写入字节数 >= 0, < 0 表示 -errno | 2 |
| `COUNT_OR_EOF_ZERO` | `sy_sock_send` | cx.sys (sys.netraw) | 成功返回发送字节数 >= 0, < 0 表示 -errno | 12 |
| `COUNT_OR_EOF_ZERO` | `sy_sock_recv` | cx.sys (sys.netraw) | 成功返回接收字节数 > 0, 0 表示对端关闭, < 0 表示 -errno | 16 |
| `COUNT_OR_EOF_ZERO` | `sy_send` | ext.net (net.tcp) | 成功返回发送字节数 >= 0, < 0 表示 -errno | 13 |
| `COUNT_OR_EOF_ZERO` | `sy_recv` | ext.net (net.tcp) | 成功返回接收字节数 > 0, 0 表示对端关闭, < 0 表示 -errno | 30 |
| `COUNT_OR_EOF_ZERO` | `sy_recv_until` | ext.net (net.tcp) | 成功返回接收字节数 > 0, 0 表示对端关闭, -ETIMEDOUT/-errno < 0 | 16 |
| `COUNT_OR_EOF_ZERO` | `sy_udp_send_to` | ext.net (net.udp) | 成功返回发送报文长度 >= 0, < 0 表示 -errno | 16 |
| `COUNT_OR_EOF_ZERO` | `sy_udp_recv_from` | ext.net (net.udp) | 成功返回接收报文长度 >= 0, < 0 表示 -ETIMEDOUT/-EMSGSIZE/-errno | 12 |
| `COUNT_OR_EOF_ZERO` | `sy_poll` | cx.sys (sysx.poll) | 成功返回就绪 fd 数量 > 0, 0 表示超时, < 0 表示 -errno | 56 |
| `COUNT_OR_EOF_ZERO` | `sy_sock_poll` | cx.sys (sys.netpoll) | 成功返回就绪事件掩码 > 0, 0 表示超时, < 0 表示 -errno | 24 |
| `COUNT_OR_EOF_ZERO` | `sy_readable` | cx.sys (sys.time) | 返回 1 表示可读就绪, 0 表示超时, < 0 表示 -errno | 15 |
| `COUNT_OR_EOF_ZERO` | `sy_wait_status` | cx.os (sys.proc) | 成功回收返回子进程 PID > 0, nohang=1 时 0 表示仍在运行(nohang=0 永不返 0), < 0 表示 -errno | 137 |
| `COUNT_OR_EOF_ZERO` | `sy_wait_any` | cx.os (sys.proc) | 成功回收返回子进程 PID > 0, nohang=1 时 0 表示无就绪子进程, < 0 表示 -ECHILD/-errno | 6 |
| `COUNT_OR_EOF_ZERO` | `sy_wait_rusage` | cx.os (sys.proc) | 成功回收返回子进程 PID > 0, nohang=1 时 0 表示仍在运行, < 0 表示 -errno | 6 |
| `COUNT_OR_EOF_ZERO` | `sy_proc_try_wait` | cx.os (sys.proc) | 返回 1 表示已回收, 0 表示仍在运行, < 0 表示 -errno | 18 |
| `COUNT_OR_EOF_ZERO` | `sy_dir_next` | cx.os (sys.posix) | 返回 1 表示取到目录项, 0 表示遍历结束, < 0 表示 -errno | 95 |
| `COUNT_OR_EOF_ZERO` | `sy_dir_names` | cx.os (sys.posix) | 成功返回目录项个数 >= 0 (0 表示空目录), < 0 表示 -ENOSPC/-errno | 45 |
| `COUNT_OR_EOF_ZERO` | `sy_resolve` | ext.net (net.resolve) | 成功返回解析出的地址个数 >= 1, 失败返回负数 SY_EAI_* 或 -errno < 0 | 17 |

## (4) 全部 70 处命中点逐条核验清单 (`TRUE_BUG` + `FALSE_POSITIVE`)

| 位置 (`file:line`) | 车道 | 函数名 | 契约类 | 匹配模式 | 判定结论 | 逐行核验说明 |
|---|---|---|---|---|---|---|
| `cx/runtime/runexec.cx:342` | A类(产品核心) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `cx/driver/input.cx:145` | A类(产品核心) | `sy_read_once` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `cx/sys/fs.cx:320` | A类(产品核心) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `cx/sys/fs.cx:608` | A类(产品核心) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `cx/sys/net.cx:60` | A类(产品核心) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `cx/sys/net.cx:516` | A类(产品核心) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `cx/sys/net.cx:686` | A类(产品核心) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `cx/sys/posix.cx:390` | A类(产品核心) | `sy_dir_next` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已处理 < 0 错误分支, 此处 == 0 或 !got 合法检测目录项遍历结束 |
| `ext/sysx/exec.cx:28` | B类(扩展库包) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `ext/sysx/run.cx:186` | B类(扩展库包) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `ext/net/sock.cx:114` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `ext/net/udp.cx:63` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `ext/net/resolve.cx:44` | B类(扩展库包) | `sy_resolve` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已处理 r < 0, r == 0 (0 个地址)作为防御分支返回 SY_EAI_BASE - 2 错误码 |
| `ext/net/unix.cx:60` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_VAR_REASSIGNED` | for 循环由第 54 行 if (r != -EAGAIN) break 跳出时, r 是第 53 行 sy_sock_connect 的返回值(0 表示连接成功), 而非第 57 行 sleep 用的 sy_sock_poll |
| `ext/net/tcp.cx:107` | B类(扩展库包) | `sy_sock_send` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 在发送循环中先判 r < 0 返回错误, 再对 r == 0 (0 字节进展)返回 -EIO 防死循环, 逻辑正确 |
| `ext/net/tcp.cx:169` | B类(扩展库包) | `sy_sock_send` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 在发送循环中先判 r < 0 返回错误, 再对 r == 0 (0 字节进展)返回 -EIO 防死循环, 逻辑正确 |
| `ext/net/tcp.cx:119` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `ext/net/httpd.cx:539` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `ext/net/httpd.cx:780` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `ext/net/httpd.cx:981` | B类(扩展库包) | `sy_sock_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/cxlib/flowfile.cx:371` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `var=sy_fn; var != 0` | `FALSE_POSITIVE_VAR_REASSIGNED` | else if (rc != 0) 位于 if (rc == 0 && st.kind == 'l') 的 else 分支, 此时 rc 来自第 364 行 sy_lstat; if 块内 sy_realpath 已在第 368 行按 rc < 0 判错 |
| `tools/cxlib/flowfile.cx:1463` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/cxlib/genkit.cx:201` | C类(工具与测试) | `sy_read_once` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/cxlib/releasecandidate.cx:80` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/cxlib/s9sched.cx:68` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/cxlib/flowexec.cx:496` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-cx-sys-net.cx:254` | C类(工具与测试) | `sy_recv` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-sys-net.cx:102` | C类(工具与测试) | `sy_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/check-cx-sys-net.cx:104` | C类(工具与测试) | `sy_poll` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/check-cx-sys-net.cx:121` | C类(工具与测试) | `sy_poll` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/lab-loader/check-net-check.cx:90` | C类(工具与测试) | `sy_dir_next` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已处理 < 0 错误分支, 此处 == 0 或 !got 合法检测目录项遍历结束 |
| `tools/tls13-get.cx:57` | C类(工具与测试) | `sy_recv` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-autoscan-decl.cx:46` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-procguard.cx:99` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/freeze-plan.cx:280` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/isolated-stream.cx:67` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/release-layout.cx:41` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/reg-gen.cx:52` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-sys-spawn.cx:55` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-cx-sys-spawn.cx:164` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-cx-sys-spawn.cx:179` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-cx-sys-new.cx:49` | C类(工具与测试) | `sy_write` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 单元测试断言写入 0 字节 sy_write(1, "x", 0) == 0 返回 0 |
| `tools/check-cx-streamapi.cx:291` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-streamapi.cx:277` | C类(工具与测试) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/check-cx-streamapi.cx:281` | C类(工具与测试) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/check-affected-domain.cx:127` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-unit-frontcache.cx:92` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/handoff-check.cx:182` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `!sy_fn(...)` | `TRUE_BUG` | 将 sy_realpath 当作返回指针/布尔(写 !sy_realpath(uc, abs, sizeof abs)); 失败返回 -errno (<0) 时 !(-errno) 为 0 不报错, 继续用未初始化 abs 建损坏软链; 应改 sy_realpath(...) < 0 |
| `tools/dvqualify.cx:431` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/candidate-chain.cx:1168` | C类(工具与测试) | `sy_dir_next` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已处理 < 0 错误分支, 此处 == 0 或 !got 合法检测目录项遍历结束 |
| `tools/dvreact.cx:260` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var != 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/dvreact.cx:377` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) != 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-cx-layers.cx:110` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-sys.cx:490` | C类(工具与测试) | `sy_pread` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-sys.cx:264` | C类(工具与测试) | `sy_recv` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/check-cx-sys.cx:259` | C类(工具与测试) | `sy_readable` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 合法检测轮询超时(返回 -ETIMEDOUT 或断言空闲未就绪) |
| `tools/check-l0-killtree.cx:86` | C类(工具与测试) | `sy_proc_try_wait` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-l0-rusage.cx:26` | C类(工具与测试) | `sy_wait_rusage` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/check-l0-waitset.cx:38` | C类(工具与测试) | `sy_wait_any` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/release-exec.cx:395` | C类(工具与测试) | `sy_read` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; !var` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 已单独处理 < 0 错误分支, 此处 == 0 或 !got 合法检测读到 EOF / 对端正常关闭流 |
| `tools/prerun-drive.cx:1342` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 |
| `tools/prerun-drive.cx:2159` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath("./unisacc.com", qp, sizeof qp) == 0 恒假(成功返回长度 >=1), 致 batch-domain-index.tsv 未物化时回退调用 tools/derived-query.cx 的整个分支成死代码; 应改 > 0 |
| `tools/prerun-drive.cx:2554` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 |
| `tools/prerun-drive.cx:2589` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 |
| `tools/prerun-drive.cx:2606` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(root/repo, real_*, sizeof real_*) == 0 恒假(成功返回长度 >=1), 致 run/archive/g1-dry 子命令的 --root/--repo 绝对路径归一化 100% 失效; 应改 > 0 |
| `tools/devsvc-core.cx:812` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `sy_fn(...) != 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/precommit.cx:1276` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
| `tools/release-chain.cx:1185` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(..., pdir/cabs, ...) == 0 恒假(成功返回长度 >=1), 致候选路径 /.cache/ 判定分支永不进入(已由 R78 子窗口修复) |
| `tools/release-chain.cx:1202` | C类(工具与测试) | `sy_realpath` | `POS_STRICT` | `sy_fn(...) == 0` | `TRUE_BUG` | sy_realpath(..., pdir/cabs, ...) == 0 恒假(成功返回长度 >=1), 致候选路径 /.cache/ 判定分支永不进入(已由 R78 子窗口修复) |
| `tools/tlib.cx:172` | C类(工具与测试) | `sy_wait_status` | `COUNT_OR_EOF_ZERO` | `var=sy_fn; var == 0` | `FALSE_POSITIVE_EOF_OR_TIMEOUT` | 均在 nohang=1 (WNOHANG) 非阻塞轮询下使用: == 0 合法表示子进程仍在运行, != 0 表示已退或出错 |
