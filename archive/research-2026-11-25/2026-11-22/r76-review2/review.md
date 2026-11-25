# R76 明文 HTTP/1.1 服务端 (ext/net/httpd.cx) 第二轮守门人纸面评审报告

> 评审对象: `research/2026-11-22/r76-design/design.md` (v2, 「主进程 `accept` + 每连接 `sy_fork_session` 同镜像子进程」方案)
> 核对源码基线: `cx/sys/proc.cx`、`cx/sys/fd.cx`、`cx/sys/io.cx`、`cx/sys/posix.cx`、`cx/sys/sig.cx`、`cx/sys/sigwatch.cx`、`cx/sys/poll.cx`、`cx/sys/netraw.cx`、`cx/sys/netpoll.cx`、`ext/net/{tcp,sock}.cx`、`ext/textx/chunked.cx`、`build/tlib-packages.tsv`、`build/layer-api/cx.os.tsv`、`~/repos/moltbaby/bin/lib/cc-proxy-common.inc:215`。
> 评审结论: **有条件放行 (`PASS WITH MINOR NOTES`, 允许进入 B 类库编码)**。一审 3 项阻塞级问题已在架构切换后全部消解; 新子进程模型引入的 4 个重点风险面均已形成代码级闭环, 附 4 条非阻塞编码细化建议。

---

## 1. 一审阻塞项消解核查

对照设计稿第 7 节保留的一审阻塞项原文与新方案消解路径:
(1) **回调阻塞 63 槽与 `/_health` 饿死 -> 已消解**: 已读取 `~/repos/moltbaby/bin/lib/cc-proxy-common.inc:215` 核实, 同一 Claude Code 会话通过 `ANTHROPIC_BASE_URL=http://127.0.0.1:$proxy_port` 将 `main` (最长 255s) 与 `haiku` 小请求复用同一端口。新方案由每个 `sy_fork_session` 子进程独占服务一条连接, `main` 阻塞上游 TLS 期间不影响同端口 `haiku`、4 个 profile 共用 18905、`kimi` 16 槽与 `/_health` 探活, 彻底消除头阻塞, 亦不再需要二期 `hs_poll_register` / `HS_DETACH`。
(2) **`sy_poll` `-EINTR` 中断处置 -> 已消解**: 父进程主循环仅对单一 `srv->listen_fd` 调用 `sy_sock_poll` 且步长上限 `<= 200ms`, 每次醒来或遇 `-EINTR` 均检查 `sy_sig_taken` 并循环 `sy_wait_any(SY_WAIT_NOHANG, ...)` 收割僵尸, 消除主循环意外退出或僵尸堆积隐患。
(3) **响应写同步阻塞与异步写缓冲矛盾 -> 已消解**: 子进程独占单连接执行流, `hs_res_send` 与 `hs_res_write_chunk` 直接用 `sy_sock_poll(client_fd, SY_POLLOUT, left_ms)` + `sy_send` 同步带超时逐块 flush 即可, 彻底去掉了易出错的用户态异步写环形缓冲。

---

## 2. 重点审查四项 (新模型核查结果)

### 2.1 (a) 子进程并发上限 (`max_children`) 与内存/进程表资源边界

1. **`SY_POLL_MAX = 64` 约束是否真正解耦**:
   - 核实 `cx/sys/poll.cx:23,54`: `SY_POLL_MAX = 64` 仅限制单次 `sy_poll(fds, n, ms)` 的描述符数组长度。
   - 新设计中父进程只监控 1 个 `listen_fd`, 子进程只监控自己的 1 个 `client_fd` (未来可含 1 个上游 `fd`), 均远小于 `64`; `child_pids[]` 通过 `mm_alloc_n(max_children, sizeof(int))` 按配置分配。因此 `max_children` 默认 `63` 并支持配置 `128` / `256` 在结构上完全成立。
2. **内存 (COW RSS) 边界评估**:
   - Linux `fork()` 写时复制共享 `.text` 与只读段, 闲置或流式转发子进程私有脏页仅约 `1 - 2 MiB`。
   - 风险提示 (需入编码细化): 若一个长连接子进程先处理完一次 `16 MiB` POST (`hs_req_body`), 其后在同一个连接进入 `255s` keep-alive 空闲等待下一个请求, 如果不释放或缩容该连接的缓冲内存, 该子进程将在 255s 内持续滞留 `16 MiB` 私有 RSS。当 `max_children = 256` 时理论最坏可达 `4 GiB`。
   - **建议**: 每次请求回调结束后 (`keep-alive` 复用进入下一轮前), 对超过阈值 (如 `cap > 65536`) 的内部读/解析 `mg_buf` 执行 `mg_free` 归还堆内存 (而交给调用方的 `hs_req_body` `*out` 由用户侧在回调内及时 `mm_free`)。
3. **进程表 (`RLIMIT_NPROC`) 与 fd 耗尽边界**:
   - `design.md` 第 2.4(c) 与 3.2 节已明确: 满槽前先跑非阻塞 `sy_wait_any` 腾槽; 若仍 `active_children >= max_children` 或 `sy_fork_session` 返回 `-EAGAIN`/`-ENOMEM`, 立即向 `client_fd` 写出 `503 Service Unavailable` 并 `sy_close(client_fd)`; `accept` 遇 `-EMFILE`/`-ENFILE` 时收割后 `sy_sleep_ms(100)` 退避防死循环。设计完备。

### 2.2 (b) `sy_fork_session` vs `sy_proc_spawn` 启动延迟与 stdio 管道处理细节

1. **原语选择核实 (`cx/sys/proc.cx` & `build/layer-api/cx.os.tsv`)**:
   - 已核实 `build/layer-api/cx.os.tsv:112` 与 `cx/sys/proc.cx:155-185`: `sy_fork_session` 已在 `cx.os` 公开符号表中 (`ext.net` 已声明依赖 `cx.os@0.1.0`), 内部是纯粹 `fork() + sy_setsid() + _exit(fn(ud) & 255)`, 零 `execve`、零重编译, COW `fork` 延迟约 `0.1 - 0.3 ms`, 直接携带父进程内存中的 `hs_handler` 指针与 `hs_config_t`。
   - 对比 `sy_proc_spawn` (`cx/sys/proc.cx:329-385`) 走 `fork + execve` 若重拉 `.cx` 脚本冷编译 `4.5 - 10+ s`、热 cache 亦需 `7 - 26 ms` 且无法直接传函数指针, 选型结论完全正确。
2. **stdio 管道陷阱与 `saved_stderr_fd` 闭环核实**:
   - 核实 `cx/sys/proc.cx:163-181`: `sy_fork_session` 无条件建 `po[2]`/`pe[2]` 并在子进程把 `po[1]`/`pe[1]` 分别 `dup2` 到 `1` 与 `2`, 把 `fds[0]=po[0]`/`fds[1]=pe[0]` 返回父进程。
   - 核实 `cx/sys/fd.cx:32-39` 与 `cx/sys/io.cx:96-100`: `sy_dup`、`sy_dup2`、`sy_fd_cloexec`、`sy_fd_nonblock` 均已存在于 `cx.sys` (`ext.net` 原依赖包含 `cx.sys@0.1.3`)。
   - **重点肯定设计稿中的 `saved_stderr_fd >= 3` 防护**: 若父进程调用方事先关闭了 `fd 0` 或 `fd 1`, 朴素 `sy_dup(2)` 会返回 `1`, 随后被 `sy_fork_session` 的 `dup2(po[1], 1)` 踩掉! 设计稿第 2.3 节通过循环确保 `saved_stderr_fd >= 3` 彻底堵住了这一隐蔽缺陷。
   - 管道释放时序核实: 父进程在 `sy_fork_session` 返回后立即关闭 `fds[0]`/`fds[1]`/`saved_stderr_fd`, 子进程在 `hs_child_entry` 首部立即 `dup2(/dev/null, 1)` 与 `dup2(saved_stderr_fd, 2)` 并关闭原 `saved_stderr_fd`。此时内核两条管道的读写端引用计数同时清零, 既不漏 fd 也不会因 64 KiB 管道写满阻塞, 且子进程 `stderr` 错误日志直接穿透至原宿主 `stderr`。
   - **守门人补充核查项**: 禁止在子进程路径中调用 `exit()` 或 `sy_exit()` (`cx/sys/sig.cx:334`), 必须从 `hs_child_entry` 返回交给 `sy_fork_session` 内部的 `_exit(fn(ud) & 255)` (`cx/sys/proc.cx:175`), 防止父进程 `sy_atexit` 链在每个连接结束时被子进程重复触发。

### 2.3 (c) 描述符继承与僵尸进程回收闭环

1. **描述符所有权矩阵核查**:
   - **父进程**: 仅长期持有 `listen_fd`; `client_fd`、`fds[0]`、`fds[1]`、`saved_stderr_fd` 均为单次循环临时所有权, 在 `sy_fork_session` 返回成功或失败的所有分支都必须关闭。
   - **子进程**: 入口第一行关闭 `srv->listen_fd` 并置 `-1`; 关闭 `saved_stderr_fd` 与 `/dev/null` 临时 fd; 仅保留自己的 `client_fd` 与 `stderr(2)`。子进程返回前关闭 `client_fd`。
   - **补充漏洞发现 (非阻塞建议 1)**: Linux 下 `sy_sock_poll(listen_fd, SY_POLLIN)` 返回就绪后到 `sy_sock_accept` 执行前, 若客户端发送 `RST` 取消连接, 处于阻塞模式的 `listen_fd` 可能导致 `sy_sock_accept` 卡住直到下一个连接到来。建议在 `hs_listen_cfg` 中对 `listen_fd` 调用 `sy_fd_nonblock(srv->listen_fd, 1)` (`cx/sys/io.cx:92`), 使 `sy_sock_accept` 遇瞬态 `-EAGAIN` 直接继续循环而不卡死主进程 (`client_fd` 同样在子进程显式设 `sy_fd_nonblock(client_fd, 1)`)。
   - **补充漏洞发现 (非阻塞建议 2)**: 确保 `sy_fork_session < 0` 失败路径下 (向客户端写 `503`), 同样先关闭 `saved_stderr_fd` 再写完 `503` 并关闭 `client_fd`, 不允许在错误处理分支漏关 `saved_stderr_fd` 或 `client_fd`。
2. **僵尸回收闭环核查**:
   - 核实 `cx/sys/proc.cx:530-532`: `sy_wait_any(SY_WAIT_NOHANG, &exited, &code, &sig)` 内部循环重试 `EINTR`, 无就绪子进程返回 `0`, 无子进程返回 `-ECHILD`。
   - 父进程每次 `hs_step`/`hs_serve` 步长 `<= 200ms` 且 `fork` 前先排空 `sy_wait_any`, 即使在无新连接时也会在 `<= 200ms` 内收割全部已退出子进程并更新 `active_children` 与 `child_pids[]`。
   - 崩溃记录 (`exited == 0 && sig > 0`) 与 `child_pids[]` 摘除绑定, 既留存故障可观测性又不会泄露槽位。

### 2.4 (d) 信号处理、`sy_setsid` 进程组强杀 (`sy_kill`) 与 `drain_ms` 零孤儿保证

1. **进程组孤儿清零核实**:
   - 核实 `cx/sys/proc.cx:170`: `sy_fork_session` 子进程一经创建即执行 `sy_setsid()`, 成为独立会话与独立进程组组长 (`sid == pgid == child_pid`)。
   - 核实 `cx/sys/proc.cx:598-603`: `sy_kill(pid)` 内部顺序执行 `kill(-(pid_t)pid, SIGKILL); kill((pid_t)pid, SIGKILL); return sy_wait(pid, 0);`。即便 `hs_handler` 内部派生了外部工具子进程, `-pid` 也会连同整组一并清理并同步 `waitpid` 收尸, 保证零孤儿、零僵尸。
2. **PID 回绕防误杀核查**:
   - 设计稿第 2.4(e) 节明确「绝不对已被 `sy_wait_any` 收割并从 `child_pids[]` 移除的 `pid` 再次调用 `sy_kill`」, 消除了 PID 复用误杀其它无辜进程的风险。
3. **子进程继承信号掩码/处置核查**:
   - 核实 `cx/sys/sigwatch.cx:19-25`: 父进程若调用 `sy_sig_watch` (`sy_sig_handle(sig, sgw_hit)`), 子进程会继承 `sgw_hit` 处理函数而非 `SIG_DFL`。设计稿第 2.3 节已明确在 `hs_child_entry` 入口第一时间对 `SIGTERM`/`SIGINT` 调 `sy_sig_default`, 确保父进程转发 `SIGTERM` (`sy_killpg(pid, sigterm)`) 时子进程能真实感知退出。
   - **补充细化建议 (非阻塞建议 3)**: 当父进程主动调用 `hs_close(drain_ms)` 而无外部信号时, 若某子进程正处于 `255s` keep-alive **请求间隙空闲**, 等到 `drain_ms` 到期才被 `SIGKILL` 强杀不如由父进程在 `hs_close(drain_ms)` 开始时向在途子进程组发 `SIGTERM`, 子进程在 keep-alive 空闲等待期 (通过恢复 `SIG_DFL` 或 `sy_sig_watch` 在空闲间隙响应 `SIGTERM`) 立即优雅退出 (返回 0), 从而让 `hs_close(drain_ms)` 更快排空而不必次次触发 `SIGKILL` 兜底。
   - **父进程崩溃外部边界注明**: 本方案完全立足 `cx.os`/`cx.sys` 不引入 Linux 专属 `ext.sysx` `sy_pdeathsig`。若父进程自身遭外部 `SIGKILL`, 未收信号的在途子进程仍受各自 `header_timeout_ms` (10s) / `body_read_timeout_ms` (30s) / `write_timeout_ms` (30s) / `idle_timeout_ms` (255s) 有限截止时间约束, 会在单连接完成后自然退出, 不会无限期孤立驻留。

---

## 3. 编码前置修改与验收建议 (4 条非阻塞细化)

进入 `ext/net/httpd.cx` 编码阶段时, 建议同步落实以下 4 点细化, 并按 `design.md` 第 4 节执行 B 类车道四门与 `tools/check-ext-net-httpd.cx` 全部测试 (含 `mutc` + `mut1..mut3`):
(1) `hs_listen_cfg` 中对 `srv->listen_fd` 设置 `sy_fd_nonblock(srv->listen_fd, 1)`, 防止连接在 `poll` 与 `accept` 之间被 `RST` 导致阻塞。
(2) `sy_fork_session < 0` 等失败分支上, 严格关闭 `saved_stderr_fd` 与 `client_fd`, 确保失败路径零 fd 泄漏。
(3) keep-alive 子进程每轮请求处理结束后, 若内部缓冲 `cap > 65536`, 立即 `mg_free` 释放大内存块, 避免 255s 空闲期持有 16 MiB RSS。
(4) 在 `hs_child_entry` 返回前显式 `sy_close(client_fd)` 并严禁调用 `exit`/`sy_exit`, 保持 `_exit` 语义纯净。

> 签名结论: **二审纸面评审通过 (`PASS WITH MINOR NOTES`, `UNMEASURED` 待实编实跑验证)**, 可按 `design.md` (v2) 第 6 节分片排期启动 `ext/net/tcp.cx` 与 `ext/net/httpd.cx` B 类库开发。
