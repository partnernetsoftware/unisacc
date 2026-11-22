# R76 明文 HTTP/1.1 服务端 (ext/net B 类库) 设计稿 (v2)

> 状态: 待二轮守门人评审 (`UNMEASURED`, 未实现, 未实编)
> 归属: `ext.net` (B 类库车道, 不占 0.0.90 A 类候选名额)
> 并发裁定: 按主调度 `1:cc` 最新裁定 (基于 `ccogw` 结构性证据: `~/repos/moltbaby/bin/lib/cc-proxy-common.inc:215` 将同一本地端口同时用于同一 Claude Code 会话的 `main` 255s 长请求与 `haiku` 小调用; 且 4 个 profile 共用 18905、`kimi` 16 槽上游池, 9 个代理中 `llm-proxy-simple/multi/kimi/cerebras` 与 `claudecode-proxy` 5 个必须并发), 一期并发模型正式定为「主进程 `accept` + 每连接 `sy_fork_session` 同镜像子进程」, 否决单线程同步回调。
> 依据: `~/repos/moltbaby/plans/proxy-http-server-requirements.md`、`~/repos/moltbaby/plans/proxy-layer-gaps.md`、`~/repos/moltbaby/bin/lib/proxy-engine.ts`、`~/repos/moltbaby/bin/lib/https.cx`、`~/repos/moltbaby/bin/lib/cc-proxy-common.inc:215`、`ext/net/{addr,resolve,sock,tcp}.cx`、`cx/sys/{netraw,netpoll,netnb,poll,time,sig,sigwatch,fs,fd,io,posix,proc}.cx`、`ext/textx/chunked.cx`、`build/tlib-packages.tsv`、`build/layer-api/cx.os.tsv`、`build/ext-dock-exceptions.tsv`、`plans/devsvc.md`。

---

## 1. 接口形状 (`ext/net/httpd.cx`)

### 1.1 模块命名与依赖边界

新增单元命名为 `ext/net/httpd.cx` (`#pragma unisa module httpd`, 调用方 `#pragma unisa use net.httpd`)。原因: `ext/net/` 已有 `addr.cx`、`resolve.cx`、`sock.cx`、`tcp.cx`、`udp.cx`、`unix.cx`, 尚无 `http.cx`; `httpd.cx` 明确标识服务端职责, 为后续可能的 HTTP 客户端 `http.cx` 留出命名空间。

合规边界:
(1) 新文件禁止 `#pragma unisa dock`、`#include`、平台 `#if`、`__syscall` 与裸 libc 调用 (`check-ext-dock.cx`、`cxinclude`、`cxplatform`、`cxraw` 必须零命中)。
(2) 仅依赖 L0 与同层包: `sys.fs`、`sys.fd`、`sys.io`、`sys.posix`、`sys.proc`、`sys.netraw`、`sys.netpoll`、`sys.netnb`、`sys.time`、`sys.sig`、`sys.sigwatch`、`mem.heap`、`mem.bytes`、`mem.grow`、`text.text`、`text.conv`、`text.fmt`、`textx.chunked`、`net.addr`、`net.tcp`、`net.sock`。
    注: 上列 `sys.fd`、`sys.io`、`sys.sig`、`sys.sigwatch`、`sys.netraw`、`sys.netpoll`、`sys.netnb` 属 `cx.sys`; `sys.fs`、`sys.posix`、`sys.proc`、`sys.time` 属 `cx.os` (`build/tlib-packages.tsv:10-11` 与 `build/layer-api/cx.os.tsv:112,126,130`), 二者均已在 `ext.net` 原声明依赖中。
(3) 需同步更新 `build/tlib-packages.tsv` 中 `ext.net` 行的 `deps`, 追加 `cx.text@0.1.0;ext.textx@0.2.4` (因 `layer-api.cx` 的 `outer_check` 要求单元 `use` 闭包不越出包声明的依赖)。

### 1.2 前置小补丁 (`ext/net/tcp.cx` 的 `SO_REUSEPORT`)

`~/repos/moltbaby/bin/lib/proxy-engine.ts` 的 `/_reload` 硬重生成要求新旧两代进程短暂绑定同一端口 (`reusePort` 默认开启)。Linux 下 `SO_REUSEPORT` 必须在 `bind()` 之前设置, 而现有 `ext/net/tcp.cx` 的 `sy_tcp_listen_at(a, backlog, flags)` 只支持 `SY_LISTEN_REUSEADDR` (1) 与 `SY_LISTEN_V6ONLY` (2), 返回已 `bind` 的 fd 后无法补设。

因此对 `ext/net/tcp.cx` (同属 `ext.net` B 类包) 做 4 行加法扩展:
- 枚举新增 `SY_LISTEN_REUSEPORT = 4`;
- 在 `sy_tcp_listen_at` 的 `sy_sock_bind` 之前, 若 `flags & SY_LISTEN_REUSEPORT`, 通过 `sy_sockopt_set(fd, SY_SO_REUSEPORT, 1)` (经 `net.sock` + `sys.netpoll` 的 `SY_SP_SO_REUSEPORT`) 启用端口复用; 失败时关闭 fd 返回 `-errno` (`-ENOPROTOOPT` 等)。

### 1.3 核心类型与配置结构

注: `cx/compiler` 目前拒绝导出接口中「函数指针形参包含结构体/typedef 指针类型」的声明 (`build/pkg-gaps.tsv` 第 36 行与 `cx/stream/pipe.cx:40-43` 已证实, 报 `not covered: incompatible semantic declaration type`)。因此回调签名 `hs_handler` 的 `req` 与 `res` 参数必须以 `void *` 传递, 并在访问器中接受 `const void *` / `void *`。

```c
enum {
    HS_OK = 0,
    HS_HANDLED = 1,          /* 回调已生成响应 */
    HS_DEFAULT_404 = 0       /* 回调返回 0, 框架发送默认 404 */
};

enum {
    HS_DEFAULT_BACKLOG = 128,
    HS_DEFAULT_MAX_CHILDREN = 63,         /* 默认并发子进程上限, 可配更高 (如 128/256), 不受 SY_POLL_MAX=64 约束 */
    HS_DEFAULT_MAX_HEADER_BYTES = 65536,  /* 64 KiB */
    HS_DEFAULT_MAX_HEADERS = 128,
    HS_DEFAULT_MAX_BODY_BYTES = 16777216, /* 16 MiB */
    HS_DEFAULT_HEADER_TIMEOUT_MS = 10000,
    HS_DEFAULT_BODY_TIMEOUT_MS = 30000,
    HS_DEFAULT_WRITE_TIMEOUT_MS = 30000,
    HS_DEFAULT_IDLE_TIMEOUT_MS = 255000   /* 255s, 覆盖长推理 SSE 空窗 */
};

typedef struct hs_config {
    const char *bind_addr;     /* NULL 或 "" 默认 "127.0.0.1" */
    int port;                  /* 0 表示由内核分配临时端口 (经 hs_port 读回) */
    int reuse_port;            /* 非 0 开启 SO_REUSEPORT (默认推荐 1) */
    int backlog;               /* <=0 使用 HS_DEFAULT_BACKLOG (128) */
    int max_children;          /* <=0 使用 HS_DEFAULT_MAX_CHILDREN (63); 可配至 128/256 等更高值 */
    long max_header_bytes;     /* <=0 使用 64 KiB */
    long max_body_bytes;       /* <=0 使用 16 MiB */
    long header_timeout_ms;    /* <=0 使用 10000 ms */
    long body_read_timeout_ms; /* <=0 使用 30000 ms */
    long write_timeout_ms;     /* <=0 使用 30000 ms */
    long idle_timeout_ms;      /* 可配至 255000 ms 及以上; <=0 使用 255000 ms */
} hs_config_t;

typedef struct hs_server hs_server;
typedef struct hs_request hs_request;
typedef struct hs_response hs_response;

/* 回调签名: 在每连接的同镜像子进程内执行. req 为 hs_request *, res 为 hs_response *.
   返回 HS_HANDLED (1) 表示已写响应; HS_DEFAULT_404 (0) 交给默认 404; 负数为内部错误并断开连接. */
typedef int (*hs_handler)(void *ud, void *req, void *res);
```

### 1.4 服务端生命周期与主从循环接口

```c
/* 初始化默认配置: 127.0.0.1, reuse_port=1, idle_timeout_ms=255000, max_children=63, max_body_bytes=16MiB */
void hs_config_init(hs_config_t *cfg);

/* 创建并监听: 0 成功, -errno 失败 (非 Linux 目标由 netraw 透传 -ENOSYS). */
int hs_listen_cfg(hs_server **out, const hs_config_t *cfg);
int hs_listen(hs_server **out, const char *bind, int port, int reuse_port, long idle_ms);

/* 查询实际绑定端口 (支持 port=0 临时端口), 监听 fd, 或当前在途子进程数. */
int hs_port(const hs_server *srv);
int hs_listen_fd(const hs_server *srv);
int hs_active_children(const hs_server *srv);

/* 模式 A (主进程 accept + 每连接 sy_fork_session 主循环):
   阻塞运行直到 hs_close / 停止信号触发且 drain_ms 排空结束.
   父进程仅以 <=200ms 步长 poll listen_fd, 每次循环先通过 sy_wait_any(SY_WAIT_NOHANG) 收割全部僵尸子进程;
   接到新连接后以 sy_fork_session 派生同镜像子进程处理该条 TCP 连接及其上的全部 keep-alive 复用请求. */
int hs_serve(hs_server *srv, hs_handler fn, void *ud);

/* 单步推进父进程循环 (单步最多等待 min(timeout_ms, 200) ms, 先收割僵尸, 若就绪则 accept+fork 一个子进程),
   便于单测或外层脚本控制迭代步数. 返回 >=0 处理数, -EINTR/-EAGAIN 或 -errno. */
int hs_step(hs_server *srv, hs_handler fn, void *ud, long timeout_ms);

/* 模式 B (单进程阻塞拉取单请求, 供纯无 fork 的轻量工具或协议单元测试直接调用):
   等待并解析下一个请求, 将 req/res 交给调用方, 处理完须调 hs_conn_finish. */
int hs_accept(hs_server *srv, long timeout_ms, void **req_out, void **res_out);
int hs_conn_finish(void *res);

/* 优雅关闭:
   (1) 立即关闭 listen_fd 停止接受新连接;
   (2) 向在途子进程组发送 SIGTERM (经 sy_killpg), 在 drain_ms (>0) 窗口内每 20ms 轮询 sy_wait_any(SY_WAIT_NOHANG) 收割;
   (3) 到期仍未退出的子进程逐个调用 sy_kill(pid) (连带其会话/进程组 SIGKILL + sy_wait), 确保零僵尸与零孤儿, 并释放全部内存与 fd. */
int hs_close(hs_server *srv, int drain_ms);
```

### 1.5 请求解析与读取请求体 (`dk_feed` 复用)

在子进程内顺序处理当前 TCP 连接的每次请求 (支持 `Connection: keep-alive` 在同一子进程内复用该 TCP 连接直至空闲超时、`Connection: close`、EOF 或出错退出), 请求视图 `hs_request` 生命周期到该次请求处理完成为止:

```c
/* 基础视图 (指针有效期至回调返回或 hs_conn_finish; bad arg 返回 "" 或 -EINVAL) */
const char *hs_req_method(const void *req);   /* "GET", "POST", ... */
const char *hs_req_path(const void *req);     /* 不含 query, 如 "/v1/messages" */
const char *hs_req_query(const void *req);    /* 含 '?' 如 "?version=1", 无则 "" */
const char *hs_req_target(const void *req);   /* 原始 request-target */
int         hs_req_version(const void *req);  /* 10 表示 HTTP/1.0, 11 表示 HTTP/1.1 */

/* 大小写无关查找请求头 (RFC 9110 第 5.1 节). 找到返回值长度 (>=0), 未找到 -ENOENT, 缓冲不足 -ERANGE. */
int         hs_req_header(const void *req, const char *name, char *out, long cap);

/* 元信息 */
long        hs_req_content_length(const void *req); /* -1 表示无 Content-Length */
int         hs_req_is_chunked(const void *req);     /* 1 表示 Transfer-Encoding 含 chunked */
int         hs_req_keep_alive(const void *req);     /* 1 表示响应后保持长连接 */

/* 读完整请求体:
   - 自动处理 Content-Length 与 Transfer-Encoding: chunked (内部使用 textx.chunked 的 dk_init/dk_feed/dk_end);
   - cap <= 0 使用服务器配置的 max_body_bytes;
   - *out 由调用方通过 mm_free 释放 (末尾多分配 1 字节写 NUL, 不计入 *outlen, 便于当作文本使用);
   - 若请求体超过 cap 或 max_body_bytes: 返回 -ENOSPC, 释放内部临时缓冲并标记连接需发送 413 并关闭;
   - 若畸形分块或连接提前断开/超时: 返回 -EINVAL / -EIO / -ETIMEDOUT. */
int         hs_req_body(const void *req, char **out, long *outlen, long cap);
```

头解析与走私防护规则:
(1) 解析请求行: 严格切分 `method`、`request-target` (拆 `path` 与 `?query`)、`HTTP/1.0` 或 `HTTP/1.1`; 其余版本返回 `505 HTTP Version Not Supported` 或 `400 Bad Request` 并关闭。
(2) 头解析直到 `\r\n\r\n` (兼容仅 `\n\n` 的简单测试端, 但不允许混合畸形行); 头部累计字节超 `max_header_bytes` 返回 `431 Request Header Fields Too Large` 并断开; 行数超 `HS_DEFAULT_MAX_HEADERS` 返回 `431` 并断开。
(3) RFC 9112 请求走私防护:
    - 同时存在 `Transfer-Encoding` 与 `Content-Length` -> 返回 `400 Bad Request` 并断开;
    - 存在多个 `Content-Length` 且值不完全相同, 或含非数字/负数/溢出 -> 返回 `400 Bad Request` 并断开;
    - HTTP/1.1 缺少 `Host` 头 -> 返回 `400 Bad Request` 并断开;
    - 解析 `Transfer-Encoding` 时忽略大小写匹配 `chunked` (对齐 `~/repos/moltbaby/bin/lib/https.cx:504-512` `hx_split` 的经验教训: 不可只匹配小写 `"chunked"`).

### 1.6 响应构造与流式分块刷新

```c
/* 设置状态码与原因短语 (默认 200 OK); 发送头之前调用 */
int hs_res_status(void *res, int status_code, const char *reason);

/* 追加响应头 (校验 name/value 不含 '\r', '\n', '\0', 否则返回 -EINVAL 防头注入).
   自动忽略调用方手写的重复 Content-Length / Transfer-Encoding 或显式校验冲突. */
int hs_res_header(void *res, const char *name, const char *value);

/* 定长整包发送: 自动补 Content-Length 与 Connection 头, 一次发完 (带 write_timeout_ms) */
int hs_res_send(void *res, const void *body, long len);

/* 开启分块流式响应 (用于 SSE text/event-stream 与透传流):
   (1) 设置 SY_SO_NODELAY=1 禁用 Nagle 算法, 防止小块 SSE 事件被内核攒批延迟;
   (2) 写入状态行 + 用户头 + "Transfer-Encoding: chunked\r\n" + Connection 头 + "\r\n";
   (3) 立即通过 sy_send 刷出响应头 (不等待第一个数据块). */
int hs_res_begin_stream(void *res);

/* 写入一个分块并立即 flush (子进程内直接调 sy_sock_poll(SY_POLLOUT, left_ms) + sy_send 同步写出, 无用户态攒批缓冲):
   - len == 0 为合法 no-op (返回 0), 绝不误发终止块 "0\r\n\r\n";
   - 编码 "<hex(len)>\r\n<payload>\r\n" 并立即发送到套接字, 直到发完或 write_timeout_ms 超时/对端断开. */
int hs_res_write_chunk(void *res, const void *data, long len);

/* 与 proxy-http-server-requirements.md 草案命名对齐的别名说明:
   可保留 hs_res_stream_write 作为薄转发或统一文档采用 hs_res_write_chunk. */

/* 结束分块流式响应: 发送终止块 "0\r\n\r\n" 并立即 flush, 标记本次响应完成. */
int hs_res_end_stream(void *res);
```

注: 子进程只服务单条客户端 TCP 连接, 因此发送逻辑无需用户态异步写环形缓冲, 直接通过 `sy_sock_poll(fd, SY_POLLOUT, left_ms)` + `sy_send` (`cx/sys/netraw.cx:200` 内部带 `MSG_NOSIGNAL`) 同步阻塞带超时发送至内核发送队列即实现逐块立刷 (`flush`), 满足 SSE TTFB 要求且无 `SIGPIPE` 风险。

---

## 2. 并发模型选项与取舍 (主进程 `accept` + 每连接 `sy_fork_session`)

### 2.1 三种并发模型对比

| 维度 | (a) 主进程 `accept` + 每连接同镜像子进程 (`sy_fork_session`) | (b) 单线程 `sys.poll` 事件循环/状态机 | (c) 每连接线程 (`pthread`) |
|---|---|---|---|
| 一期裁定 | **一期采用 (1:cc 裁定)** | **否决** (同端口 `main` 255s + `haiku` 并发会自锁) | 不可用 (`pthread` 排 `0.0.106`, `std-surface.tsv:9` 列为 `OUT`) |
| `ccogw` 真实负载适配 | 天然并发: `main` 255s 长请求、`haiku` 小调用、`/_health`、4 profile 共用 18905、`kimi` 16 槽上游池互不阻塞 | 致命缺陷: `hs_handler` 同步等上游 TLS 255s 会堵死同进程所有请求, 造成 Claude Code 会话自锁 | 不可用且 `cx` 全局状态非线程安全 |
| 并发数上限 | `max_children` 默认 `63`, 可配 `128`/`256`, 与 `SY_POLL_MAX=64` 无关 (子进程仅 poll 自己的 `client_fd` 与上游 `fd`) | 硬受 `SY_POLL_MAX=64` 限制 (`<= 63` 连接) | N/A |
| Windows 迁移 (`R67`) | 现状 `win/*` 由 `sy_fork_session`/`sy_sock_have` 透传 `-ENOSYS`; `R67` 后续可仅在 `httpd.cx` 内部替为单步/IOCP/线程后端, 外部 API 不变 | 仅当 `hs_handler` 全改为异步非阻塞才不自锁 | 排在 `0.0.106` 之后 |
| 隔离性与稳定性 | 极佳: 单请求崩溃/泄漏只在子进程内死去, 不拖垮监听进程与其他在途请求 | 单个回调崩或内存泄漏即影响全服 | 内存破坏波及全进程 |

### 2.2 `sy_proc_spawn` vs `sy_fork_session` 语义与启动延迟对比

读取 `cx/sys/proc.cx` 源码核实两个进程原语的本质差异:
(1) `sy_proc_spawn` (`cx/sys/proc.cx:329-385`): `fork + execve` 语义。若用来重拉一个 `.cx` 入口脚本, 即使走 `unisacc.com FILE.cx --` 热缓存启动仍需约 `7 - 26 ms` 加新进程初始化 (冷编译更是 `4.5 - 10+ s`), 且无法直接把父进程内存中的 `hs_handler` 回调指针与已解析配置直接交过去, 必须在磁盘或命令行重传递。
(2) `sy_fork_session(int (*fn)(void *), void *ud, int fds[2])` (`cx/sys/proc.cx:155-185`): **纯同镜像 `fork` 语义** (`pid = fork(); if (pid == 0) { sy_setsid(); ... _exit(fn(ud) & 255); }`)。特点:
    - 零 `execve`、零重编译、零重新加载模型或配置;
    - Linux 写时复制 (COW) `fork` 延迟仅约 `0.1 - 0.3 ms` (比热启动 exec 快 2 个数量级, 可支撑高频小请求如 `/_health` 与 `haiku` 路由);
    - 调用 `sy_setsid()` 使每个连接子进程成为新会话与新进程组组长 (`sid = pgid = pid`), 为后续整组强杀零孤儿奠定基础;
    - 结束执行 `_exit(fn(ud) & 255)`, 不刷新用户态缓冲也不重复执行父进程 `sy_atexit`。

### 2.3 `sy_fork_session` 的 stdio 管道细节与 `saved_stderr_fd` 无泄放技巧

读取 `cx/sys/proc.cx:161-183` 发现一个必须处理的关键细节: `sy_fork_session` 固定创建一对管道 `po[2]`/`pe[2]`, 在子进程内 `dup2(po[1], 1)` 与 `dup2(pe[1], 2)` 并关闭原描述符, 然后把读端 `fds[0] = po[0]` 和 `fds[1] = pe[0]` 返回给父进程。如果父进程既不持续排空读端也不妥善回收, 将产生两个致命问题:
- 父进程不关闭 `fds[0]`/`fds[1]` -> 每连接泄漏 2 个 `fd`;
- 父进程直接关闭 `fds[0]`/`fds[1]` 而子进程仍留着 `po[1]`/`pe[1]` -> 子进程往 stdout/stderr 打日志时触发 `-EPIPE`, 或在关闭前写满 64 KiB 管道缓冲而永久阻塞!

且本单属 B 类库车道, **不得修改 A 类核心文件 `cx/sys/proc.cx`**。解决方案 (纯在 `ext/net/httpd.cx` 内闭环完成):
(1) `fork` 前准备: 父进程在调用 `sy_fork_session` 前执行 `saved_stderr_fd = sy_dup(2)`。
    - 边界防御: 若守护化进程曾关闭 `fd 0` 或 `fd 1` 导致 `sy_dup(2)` 返回值 `< 3` (`fd 1` 或 `fd 2`), 连续 `sy_dup(2)` 直到拿到 `saved_stderr_fd >= 3` 再关闭临时占位的小编号 fd, 避免 `sy_fork_session` 内部的 `dup2(po[1], 1)`/`dup2(pe[1], 2)` 覆盖 `saved_stderr_fd`! 若 `fd 2` 本身未打开 (`sy_dup(2) < 0`), 记 `saved_stderr_fd = -1`。
    - 若 `saved_stderr_fd >= 0`, 设 `sy_fd_cloexec(saved_stderr_fd, 1)` (不影响 `fork` 继承, 但防止后续回调内 `exec` 泄漏)。
(2) `fork` 后父进程立刻回收 (`pid > 0` 分支):
    - 立即 `sy_close(fds[0]); sy_close(fds[1]);` (丢弃匿名 stdio 管道读端);
    - 立即 `sy_close(client_fd);` (父进程绝不保留已移交子进程的客户端 fd);
    - 若 `saved_stderr_fd >= 0`, 父进程立即 `sy_close(saved_stderr_fd);` (父进程自身 stderr 原来就是 `fd 2`, 无需保留副本)。
(3) 子进程入口 `hs_child_entry(void *ud)` 第一时间:
    - 关闭继承的 `srv->listen_fd` 及父进程其它内部 fd;
    - 回收匿名管道写端并恢复 stderr:
      - 打开 `/dev/null` (`dn = sy_open("/dev/null", O_WRONLY, 0)`): 若 `dn >= 0`, `if (dn != 1) { sy_dup2(dn, 1); sy_close(dn); }` (立即关闭 `po[1]`);
      - 若 `saved_stderr_fd >= 0`: `sy_dup2(saved_stderr_fd, 2); sy_close(saved_stderr_fd);` (立即关闭 `pe[1]` 且令子进程 `fprintf(stderr, ...)`/`sy_eprintf` 直达宿主原 stderr, 零 64 KiB 管道阻塞); 否则将 `fd 2` 也重定位到 `/dev/null`。
    - 将 `SIGTERM` 与 `SIGINT` 恢复为默认处理 (`sy_sig_default`), 避免继承父进程的 `sy_sig_watch` 计数回调而吞掉父进程转发的退出信号;
    - 将 `client_fd` 设为非阻塞 (`sy_fd_nonblock(client_fd, 1)`), 确保子进程自身读头/读体/空闲/写超时完全由 `sy_sock_poll` + `sy_deadline_t` 精确掌控。

### 2.4 新漏洞面逐项应对措施

(a) **描述符继承泄漏**:
    - 父进程持有: `listen_fd`、每次循环临时使用的 `saved_stderr_fd`、`client_fd`、`fds[0..1]`。`sy_fork_session` 返回 (无论成功或失败) 后父进程必关 `client_fd`、`fds[0..1]` 与 `saved_stderr_fd`; `listen_fd` 只在父进程保留至 `hs_close`。
    - 子进程持有: 入口第一时间关闭 `srv->listen_fd` (并清空其拷贝结构体中的 `listen_fd = -1`), 通过 `dup2` 顶掉 `po[1]`/`pe[1]` 并关闭 `saved_stderr_fd`, 只保留自身唯一的 `client_fd` 与原 `stderr(2)`。子进程结束前调用 `sy_close(client_fd)` 与 `mg_free`。
(b) **僵尸进程回收**:
    - `hs_server` 维护在途子进程表 (`int *child_pids`, 容量 `max_children`, 经 `mm_alloc_n` 分配) 与 `active_children` 计数。
    - 父进程在每次 `hs_step` / `hs_serve` 迭代顶部 (以及 `sy_sock_poll(listen_fd, SY_POLLIN, <=200ms)` 醒来后、新 `accept` 前), 循环调用 `sy_wait_any(SY_WAIT_NOHANG, &exited, &code, &sig)` (`cx/sys/proc.cx:530`) 直至返回 `0` 或 `-ECHILD`, 对每个收割到的 `pid` 从 `child_pids[]` 摘除并递减 `active_children`。
    - 若收割发生且曾因满槽或 `EMFILE` 暂停过新连接处理, 立即恢复正常接收。
(c) **`fd`/进程数耗尽 (`-EMFILE` / `-ENFILE` / `-EAGAIN` / `-ENOMEM`)**:
    - `sy_sock_accept` 若返回 `-EMFILE` 或 `-ENFILE`: 父进程严禁忙等空转 `accept`, 应先跑一轮 `sy_wait_any` 收割可能刚退出的子进程, 并通过 `sy_sleep_ms(100)` 暂停监听 `100ms` 避免 CPU 跑满 100%;
    - `sy_fork_session` 前先跑一次非阻塞 `sy_wait_any` 回收刚退出的空槽; 若仍 `active_children >= max_children`, 或 `sy_fork_session` 返回 `-EAGAIN`/`-ENOMEM` (触发 `RLIMIT_NPROC` 或内存不足), 父进程直接在 `client_fd` 非阻塞写出 `HTTP/1.1 503 Service Unavailable\r\nContent-Length: 19\r\nConnection: close\r\nRetry-After: 1\r\n\r\nService Unavailable` 并立即 `sy_close(client_fd)`!
(d) **子进程崩溃隔离与日志**:
    - 子进程在独立地址空间运行, 任何空指针/越界/异常退出均不影响主进程和其它在途子进程;
    - 父进程在 `sy_wait_any` 收到 `pid > 0 && exited == 0 && sig > 0` 时, 记录 `child pid=<P> crashed sig=<S>` 到 `stderr` (`sy_eprintf`) 并递增崩溃计数 `srv->crash_count` (供监控或测试断言)。
(e) **`hs_close(drain_ms)` 等待子进程退出与超时强杀**:
    - 第一步: `sy_close(srv->listen_fd)` 并置 `listen_fd = -1`, 立刻拒收新连接;
    - 第二步: 若 `drain_ms > 0` 且 `active_children > 0`, 先通知所有在途子进程 (若由信号/关闭触发可由配置决定先等待自然完成还是发 `SIGTERM`, 默认向在途 `child_pids[]` 发送 `SIGTERM` 唤醒空闲 keep-alive 子进程并让在途处理器尽快收尾; 若要求“等在途自然结束再到点强杀”, 可设在等待窗口末段或先发通知信号 — 明确约定: 正常调用 `hs_close(drain_ms)` 时, 先发送 `SIGTERM` 或允许在 `drain_ms` 内等待自然完结, 重点是在 `drain_ms` 内每 `20ms` 调用 `sy_wait_any(SY_WAIT_NOHANG, ...)` 收割所有已退出子进程并从 `child_pids[]` 摘除);
      注: 为兼顾 `keep-alive` 空闲子进程不傻等 255s 与在途长请求完成, 建议细化为: 若 `drain_ms > 0`, 先等待在途子进程自然完成 (keep-alive 子进程只应在无活跃请求时被立即通知, 或统一先发 `SIGTERM` 让闲置循环退出而活跃请求可由回调捕获或直接允许 `drain_ms` 排空后再强杀)。更准确的做法: 仅在父进程收到 `SIGTERM`/`SIGINT` 时转发 `SIGTERM` 给子进程组, 而主动调 `hs_close(drain_ms)` 时先停监听并在 `drain_ms` 窗口内每 `20ms` `sy_wait_any` 等待在途连接完成, 到期后对仍存活且未在 `child_pids[]` 摘除的 `pid` 逐个调用 `sy_kill(pid)`!
    - 第三步: `sy_kill(pid)` (`cx/sys/proc.cx:598-603`) 内部依次执行 `kill(-pid, SIGKILL); kill(pid, SIGKILL); sy_wait(pid, 0);`。因为 `sy_fork_session` 已通过 `sy_setsid()` 让子进程自成会话与进程组长 (`pgid == pid`), `-pid` 会连带杀掉该子进程及其内部 `exec`/派生的一切后代进程, 彻底保证零孤儿、零僵尸!
    - 顺序安全保证: **绝不对已被 `sy_wait_any` 收割并从 `child_pids[]` 移除的 `pid` 再次调用 `sy_kill(pid)`**, 消除 PID 重用误杀风险。
(f) **父进程信号转发**:
    - 父进程监听 `SIGTERM` 与 `SIGINT` (`sy_sig_watch(sig)`, 编号经 `sy_sig_number("SIGTERM")`/`sy_sig_number("SIGINT")` 解析, 兼容目标差异);
    - `hs_serve` 每次醒来检查 `sy_sig_taken`, 一旦收到终止信号即置停止标志, 向当前在途 `child_pids[]` 逐个调用 `sy_killpg(pid, term_sig)`, 然后进入 `hs_close(drain_ms)` 的 `20ms` 轮询收割与 `SIGKILL` 兜底流程。

### 2.5 Windows 迁移路线 (`R67`) 兼容性说明

依据 `plans/devsvc.md` (`R67`) 与 `cx/sys/proc.cx:159,505`:
(1) `ext/net/httpd.cx` 严守无 `#if`、全走 `sy_*` 调用, 6 个目标全部可编译通过;
(2) 在当前 (`0.0.89`/`0.0.90`) Windows 上, `sy_tcp_listen_at` / `sy_fork_session` / `sy_wait_any` 返回 `-ENOSYS`, `httpd.cx` 原样透传 `-ENOSYS` 具名拒绝;
(3) 当后续 `R67` (`0.0.93+` 阶段 1) 在 L0 补齐 Winsock `WSAPoll`/`select` 通信能力、或 `0.0.106` 线程就位后, 上层调用方使用的 `hs_listen` / `hs_serve` / `hs_req_*` / `hs_res_*` / `hs_close` **公开 API 保持不变**, 仅需在 `httpd.cx` 内部增加按平台能力 (`sy_cap` 或 `-ENOSYS` 探测) 选择的后端路径即可平滑迁移。

---

## 3. 三个新决定的建议值与理由

### 3.1 请求体上限 (`max_body_bytes`)

- **建议默认值**: `16 MiB` (`16 * 1024 * 1024 = 16777216` 字节), `hs_config_t.max_body_bytes` 可配 (软上限 `64 MiB`, `0` 表示使用默认 `16 MiB`; `hs_req_body(..., cap)` 取 `min(cap, max_body_bytes)`).
- **理由**:
  (1) 对照真实 LLM 上下文负载: 200K token 纯文本请求 JSON 约 `800 KB - 2 MB`, 含工具历史与多模态元数据通常在 `1 - 8 MB`, `16 MiB` 留足 2x 以上余量;
  (2) 多进程场景防内存放大: 按写时复制 (`fork`) 模型, 闲置或流式转发子进程私有内存约 `1 - 2 MiB`; 但若 `max_children = 63` 个子进程同时全量缓冲一个请求体, `16 MiB` 下峰值 RSS 约 `1 GiB`, 而 `32/64 MiB` 将翻倍至 `2 - 4 GiB` 易触及本机内存压力 (需超大图片场景时由特定代理显式配置)。
- **超限拒绝合同**:
  (1) `Content-Length > max_body_bytes`: 解析完请求头后不读任何请求体字节, 直接写 `413 Payload Too Large` (`Connection: close`) 并关闭 `client_fd` 退出子进程;
  (2) `Transfer-Encoding: chunked`: 随 `dk_feed` 解码累计载荷与帧长, 一旦超限立刻中止读取、释放 `mg_buf`, 回 `413 Payload Too Large` (`Connection: close`) 并关闭退出。

### 3.2 并发子进程上限 (`max_children`)

- **建议默认值**: `63`, **可配至 `128` / `256` 及以上** (不再受 `cx/sys/poll.cx:23` `SY_POLL_MAX = 64` 制约!)。
- **架构解耦依据**:
  - 父进程每步只轮询 1 个描述符 (`sy_sock_poll(srv->listen_fd, SY_POLLIN, step_ms)`), 不把子连接放进父进程的 `sy_poll` 数组;
  - 每个子进程内只轮询自己的 1 个 `client_fd` (以及未来可选的 1 个上游 `fd`), 远低于 `SY_POLL_MAX = 64`;
  - 因此 `max_children` 完全与 `SY_POLL_MAX = 64` 脱钩, 由 `mm_alloc_n(max_children, sizeof(int))` 动态分配 `child_pids[]` 追踪表即可支持 `128`、`256` 乃至 `512` 并发 (默认取 `63` 作为保守安全的本地代理默认上限, `ccogw` 或多 profile 网关场景可直接配 `128`/`256`)。
- **超限与耗尽处理**:
  (1) 满槽前先运行非阻塞 `sy_wait_any(SY_WAIT_NOHANG, ...)` 收割僵尸腾槽;
  (2) 确认为 `active_children >= max_children` 或 `sy_fork_session` 返回 `-EAGAIN`/`-ENOMEM`: 父进程不静默丢在队列卡死探测方, 而是在 `client_fd` 立即非阻塞写出 `503 Service Unavailable` (`Retry-After: 1`, `Connection: close`) 后立刻 `sy_close(client_fd)`;
  (3) `accept` 遇 `-EMFILE`/`-ENFILE`: 先跑 `sy_wait_any` 回收子进程释放资源, 再 `sy_sleep_ms(100)` 退避防止死循环空转。

### 3.3 慢连接 / 慢头策略

每个子进程基于 `cx/sys/time.cx` 单调时钟 (`sy_mono_ns` / `sy_deadline_t`) 对自身独占的 `client_fd` 实施四阶段超时保护:

| 阶段 | 参数名 | 建议默认值 | 触发条件与处置 | 理由 |
|---|---|---|---|---|
| 读请求头 | `header_timeout_ms` | `10000` (10s) | 子进程开始读该次请求头后, 10s 内未收齐 `\r\n\r\n` | 防止 Slowloris 慢发头长期占用子进程与进程表名额; 若已收部分字节可回 `408 Request Timeout` 后关闭退出 |
| 读请求体 | `body_read_timeout_ms` | `30000` (30s) | 每次 `sy_sock_recv` 取得新字节并推进解析后刷新截止时间; 超过 30s 无进展 | 防止慢滴请求体攻击拖死子进程; 超时回 `408 Request Timeout` 并退出 |
| 长连接空闲 | `idle_timeout_ms` | `255000` (255s, 可配 `> 255s`) | keep-alive 两个请求之间空闲等待下一请求首字节超时, 或覆盖长推理请求周期 | 满足 `proxy-http-server-requirements.md` 第三.1 节 (`claudecode-proxy`、`buddy-native-proxy` 需要 255s); 超时优雅关闭连接退出子进程 |
| 写阻塞 (慢读者) | `write_timeout_ms` | `30000` (30s) | `sy_send` 返回 `-EAGAIN` 后调 `sy_sock_poll(client_fd, SY_POLLOUT, left_ms)`, 连续 30s 无法写出 | 防止客户端挂起不读 SSE 流导致子进程永久阻塞变成游荡进程; 超时关闭 `client_fd` 并退出 |

---

## 4. 验收设计

### 4.1 B 类库车道四门 (对标 `plans/reviews/2026-11-20-layer-lanes.md` 与 `R76`)

(1) **门 1 - 六目标实编**:
    使用当前已装 Root (`0.0.89`/`0.0.90`), 对 `lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`、`win/x86_64`、`win/arm64` 六个目标执行消费端探针 (`#pragma unisa use net.httpd`) 的 `-b <target> -c` 与 `-b <target> -o` 实编, 要求 6/6 编译退出码为 0 且输出文件非空; 非 Linux 目标仅标 `COMPILED` 不伪称运行通过。
(2) **门 2 - `layer-api` 同步**:
    运行 `sh ./unisacc.com tools/layer-api.cx -- --bump ext.net` (`ext.net` 版本 `0.1.9 -> 0.2.0`, `deps` 追加 `cx.text@0.1.0;ext.textx@0.2.4`), 并运行 `sh ./unisacc.com tools/layer-api.cx -- --check` 确认 `build/layer-api/ext.net.tsv`、`build/tlib-packages.tsv` 的 `api_sha`/内容哈希、`outer_check` 闭包检查全绿。
(3) **门 3 - `affected` 选批全绿**:
    运行 `sh ./unisacc.com tools/affected.cx -- --files ext/net/httpd.cx ext/net/tcp.cx --run`, 所有被选中批次全部 `PASS`; 同时跑 `tools/check-ext-dock.cx` 与现行棘轮 (`ratchets.cx`), 确保 0 个新增 `dock`/`#include`/libc/硬编码家目录路径违例。
(4) **门 4 - 上一公开 APE 兼容实编**:
    使用上一公开 APE (`0.0.89`, sha256 `384e1152...`) 重新编译 `ext/net/httpd.cx` 与 `tools/check-ext-net-httpd.cx` 并实跑通过, 证明不依赖未公开编译器特性 (`min_product=0.0.89`)。

### 4.2 功能与边界测试矩阵 (`tools/check-ext-net-httpd.cx`)

全部测试通过真实 `127.0.0.1:0` 回环套接字与真实 `sy_fork_session` 子进程执行 (临时目录用 `tk_scratch`):

| 用例组 | 覆盖场景与预期断言 |
|---|---|
| `basic` (回环与路由视图) | `hs_listen("127.0.0.1", 0, ...)` -> `hs_port > 0`; `GET /v1/models?a=1` 断言 `method`/`path`/`query`; `hs_req_header` 大小写无关命中与 `-ENOENT`; `hs_res_send` 返回 `200 OK`; 空连即断与默认 `404`。 |
| `malformed` (畸形与走私拒绝) | 冲突 `Content-Length` + `Transfer-Encoding` -> `400`; 多个不同 `Content-Length` -> `400`; 负数/溢出 `Content-Length` -> `400`; HTTP/1.1 缺 `Host` -> `400`; `hs_res_header` 含 `\r\n` 返回 `-EINVAL`; 超 `max_header_bytes` 返回 `431`。 |
| `body-limit` (大请求体与 413) | 定长边界值 `max_body_bytes` 成功 `200`; `max_body_bytes + 1` 早拒 `413 Payload Too Large` 并断连; chunked 超限同样 `413`; `hs_req_body` 小 `cap` 返回 `-ENOSPC`。 |
| `chunked-req` (复用 `dk_feed`) | 单块/多块/大写 `CHUNKED`/大小写 hex/extension/trailer/空块/跨多次 `sy_send` 切分全部还原正确; 畸形 hex 返回 `400`。 |
| `sse-stream` (逐块 flush) | 子进程调用 `hs_res_begin_stream` -> `hs_res_write_chunk` -> 客户端 200ms 内 `sy_recv_until` 收到首块 (不等 `end_stream`, 证明禁 Nagle + 立刷); `len=0` 不发 `0\r\n\r\n`; 终块后 `dk_dechunk` 完整还原。 |
| `keepalive` (连接复用) | 单条 TCP 连接在同一个服务子进程内连续处理 4 次请求 (含定长 POST、chunked POST + 下一请求粘包、`Connection: close`), 前 3 次同一 `sy_pid` 服务且第 4 次后客户端读到 EOF (`0`)。 |
| `timeout` (慢头/慢体/255s) | 短超时配置 (150ms) 下实测半截头、半截体、keep-alive 空闲均在 500ms 内超时关闭; 默认配置校验 `idle_timeout_ms == 255000` (绝不被钳位为 10s)。 |
| `fork-conns` (并发阻塞隔离与管道/fd/僵尸/孤儿) | (1) **阻塞隔离**: 客户端 A 发 `/slow` 让子进程睡 400ms (模拟 255s 长请求), 同时客户端 B 发 `/_health`, 断言客户端 B 在 `< 50ms` 内拿到 `200` (证明 `main`+`haiku` 不互锁); (2) **`max_children` 与 `503`**: 配 `max_children=3`, 保持 3 个 `/slow` 占位, 第 4 个连接立刻收到 `503` 且配 `max_children=70` (>64) 可正常配置; (3) **stdio 管道无阻塞与 stderr 透传**: 子进程连续向 stderr 打超过 `128 KiB` 日志不发生 64KB 管道死锁且父进程 stderr 文件可收到日志; (4) **崩溃隔离**: 一请求子进程调用 `sy_raise(SIGKILL)` 异常死亡, 父进程记录崩溃计数、收割僵尸, 下一个请求仍正常 `200`; (5) **`hs_close(drain_ms)` 零僵尸零孤儿**: 验证在途短连接在 `drain_ms` 内等完退出, 顽固卡死子进程 (且自身再 fork 了一个孙进程) 在 `drain_ms` 到期后被 `sy_kill(pid)` 连同整组强杀, `sy_wait_any == -ECHILD` 且孙进程已死, 循环 50 次 fork 后父进程新建最小 fd 编号归位 (无 `fds[0..1]`/`client_fd`/`saved_stderr_fd` 泄漏)。 |
| `cross` (非 Linux `-ENOSYS` 与六目标) | 对 `win/x86_64`、`osx/x86_64` 等六目标交叉编译含 `net.httpd` 的探针; 校验 `-ENOSYS` 透传契约。 |

### 4.3 三个变异全杀设计 (`cxlib.muthelp`)

在 `tk_scratch` 树对 `ext/net/` 与 `tools/check-ext-net-httpd.cx` 跑 `mutc` (未变异必过) 与 3 个独立单点变异:
(1) **`mut1` (绕过 `max_body_bytes` 上限检查)**: 将超限拒绝判断破坏 -> `body-limit` 组 `413` 检查转红 (`rc=1`)。
(2) **`mut2` (破坏 `chunked` 大小写识别或 `dk_feed` 粘包偏移推进)**: 仅认小写或丢弃 `*consumed` 后剩余字节 -> `chunked-req` 与 `keepalive` 粘包用例转红 (`rc=1`)。
(3) **`mut3` (破坏 `saved_stderr_fd` 管道回收或 `hs_close` 超时强杀/僵尸收割)**: 删掉子进程内对 `fd 1/2` 的 `dup2` 顶替 (触发 64KB 管道写卡死) 或破坏超时 `sy_kill(pid)` -> `fork-conns` 转红 (`rc=1`)。

---

## 5. 守门人评审要点

(1) **描述符继承与管道闭环**:
    - 父进程必须严格按「`fork` 前 `saved_stderr_fd = sy_dup(2)` (`>=3`, `CLOEXEC`) -> `sy_fork_session` 返回后无论成功失败均立即关闭 `fds[0]`、`fds[1]`、`client_fd`、`saved_stderr_fd`」执行;
    - 子进程入口第一时间 `sy_close(srv->listen_fd)`、`dup2(/dev/null, 1)`、`dup2(saved_stderr_fd, 2)` 并关闭 `saved_stderr_fd` (以及临时 `/dev/null` fd), 确保 0 个管道写端残留、0 个 `listen_fd` 继承泄漏;
    - 测试需验证子进程输出 `>128 KiB` 到 stderr 不死锁且 50 次 fork 后父进程 fd 槽位归位。
(2) **僵尸收割与孤儿清理**:
    - 父进程每次 `hs_step`/`hs_serve` 循环顶部必循环调用 `sy_wait_any(SY_WAIT_NOHANG, ...)` 直至排空;
    - `child_pids[]` 中一旦由 `sy_wait_any` 摘除某个 `pid`, 绝不再对其调 `sy_kill(pid)` (防 PID 回绕误杀);
    - `hs_close(drain_ms)` 先关 `listen_fd`、再等待/收割、到期对剩余存活 `pid` 调 `sy_kill(pid)` 利用 `sy_fork_session` 内建 `sy_setsid()` 杀灭整组 (`-pid`), 保证零孤儿。
(3) **`SIGPIPE` / `EPIPE` 与子进程退出方式**:
    - `sy_send` 带 `MSG_NOSIGNAL`, 遇 `-EPIPE`/`-ECONNRESET` 视为对端断开并收尾关闭连接;
    - 子进程必须从 `hs_child_entry` 返回交给 `sy_fork_session` 内部的 `_exit(rc & 255)`, 严禁在子进程中调用 `exit()`/`sy_exit()` 导致父进程 `atexit` 被重复执行或冲刷缓冲;
    - 子进程入口第一时间对 `SIGTERM`/`SIGINT` 调用 `sy_sig_default`, 防止继承父进程 `sy_sig_watch` 的计数处理器导致无法响应关闭信号。
(4) **缓冲区边界、整型溢出与 `dk_feed` 组合**:
    - 全用 `mm_copy`、`mg_append`、`tx_to_u64`/`tx_to_i64` (禁止 `tx_atol`), `hs_res_header` 拒 `\r`/`\n`/`\0`;
    - 严禁另造 chunked 解码器, 直接在连接上下文内嵌 `dk_state` 复用 `textx.chunked` `dk_feed`。
(5) **Windows / macOS 桩合同 (`-ENOSYS`)**:
    - 零 `#if`、零 `#include`、零 `dock`、零 libc;
    - 非 Linux 目标编译 6/6 通过, 运行时 `sy_tcp_listen_at` / `sy_fork_session` / `sy_wait_any` 的 `-ENOSYS` 原样透传, 源码头带 `// ENOSYS: ...` 注释。

---

## 6. B 类库文件清单与估时 (全部 `UNMEASURED`)

### 6.1 新增与修改文件清单

| 路径 | 动作 | 类别 | 说明 |
|---|---|---|---|
| `ext/net/httpd.cx` | 新增 | B (库源码) | 主进程 `accept` + `sy_fork_session` HTTP/1.1 服务端完整实现 (`hs_listen*`, `hs_serve`, `hs_step`, `hs_accept`, `hs_req_*`, `hs_res_*`, `hs_close`) |
| `ext/net/tcp.cx` | 修改 | B (库源码) | 新增 `SY_LISTEN_REUSEPORT = 4` 与 `sy_tcp_listen_at` 内 `bind` 前设置 `SO_REUSEPORT` (4 行加法, 不新增 `dock`/libc) |
| `build/tlib-packages.tsv` | 修改 | B (包清单) | `ext.net` 行版本升 `0.2.0`, `deps` 追加 `cx.text@0.1.0;ext.textx@0.2.4` |
| `build/layer-api/ext.net.tsv` | 修改 | B (派生表) | 由 `tools/layer-api.cx --write --pkg ext.net` 重新生成 |
| `tools/check-ext-net-httpd.cx` | 新增 | C (测试/门) | 回环功能、走私/413、chunked/SSE、keep-alive、超时/255s、并发阻塞隔离 (`main`+`haiku`)、128KB 管道防卡死、崩溃隔离、进程组零孤儿、fd 归位、`mutc` + `mut1..mut3` 全杀 |
| `tests/batches.tsv` | 修改 | C (注册表) | 新增 `ext-net-httpd`、`ext-net-httpd-mutc`、`ext-net-httpd-mut1..mut3` (`--keyed`) |
| `tests/coverage.tsv` | 修改 | C (注册表) | 对应 5 个批次的覆盖 ID 登记 (`--keyed`) |
| `tests/tools-compile-sources.tsv` | 修改 | C (注册表) | 登记 `tools/check-ext-net-httpd.cx` (`register.cx --tool`) |

### 6.2 分片估时 (全部标注 `UNMEASURED`)

按项目「对折口径加倍」经验 (含 6 目标交叉编译、多进程信号/fd/管道竞态排查、`muthelp` 锚点、`layer-api`、`affected` 及 `0.0.89` APE 兼容验真):

| 分片 | 内容 | 直觉估时 | 按对折口径加倍后排期估时 | 状态 |
|---|---|---|---|---|
| 片 1: `tcp.cx` + 父进程 `accept`/`sy_fork_session`/收割/`hs_close` | `SY_LISTEN_REUSEPORT`、父循环 `sy_sock_poll`、`saved_stderr_fd` 管道回收、`child_pids[]`、`sy_wait_any`、`SIGTERM` 转发与 `sy_kill` | 65 分钟 (`UNMEASURED`) | 130 分钟 (`UNMEASURED`) | `UNMEASURED` |
| 片 2: 子进程 HTTP/1.1 引擎 + `dk_feed` + SSE 流式 + 4 类超时 | 请求行/头/走私检查、`hs_req_body` (`dk_feed`) + `413`、keep-alive 循环、`hs_res_*` (`NODELAY` + 逐块 flush)、单调时钟超时 | 70 分钟 (`UNMEASURED`) | 140 分钟 (`UNMEASURED`) | `UNMEASURED` |
| 片 3: 检查工具 `check-ext-net-httpd.cx` 与 3 变异 | 全部回环测试 + 慢请求/健康检查并发隔离 + >128KB stderr 管道不阻塞 + 进程组孤儿清零 + fd 归位 + `mutc`/`mut1..mut3` | 75 分钟 (`UNMEASURED`) | 150 分钟 (`UNMEASURED`) | `UNMEASURED` |
| 片 4: B 车道四门验收与登记提交 | 六目标实编、`layer-api --bump ext.net`、`affected --run`、`0.0.89` APE 兼容实编、棘轮检查与 `--keyed` 提交 | 30 分钟 (`UNMEASURED`) | 60 分钟 (`UNMEASURED`) | `UNMEASURED` |
| 合计 | 完整交付 R76 (`ext/net/httpd.cx` B 类库 + 全部测试与四门收据) | 240 分钟 / 4.0 小时 (`UNMEASURED`) | 480 分钟 / 8.0 小时 (`UNMEASURED`) | `UNMEASURED` |

---

## 7. 评审后修订 (保留一审阻塞项并逐项写明新模型消解方式)

以下完整保留第一轮守门人评审指出的 3 项阻塞级问题原文要点, 并逐项注明在切换为「主进程 `accept` + 每连接 `sy_fork_session` 同镜像子进程」模型后的消解方案:

(1) **原一审阻塞项 1: 回调同步阻塞导致单线程 63 槽与 `/_health` 饿死 (原需预留 `hs_poll_register` 二期异步化接口)**
    - **新模型消解**: 切换为「主进程 `accept` + 每连接 `sy_fork_session` 同镜像子进程」后, 每个已接受的客户端连接都在独立的子进程地址空间与执行流中执行 `hs_handler`。即使 `main` 推理请求正在回调内同步阻塞等待上游 TLS 长响应达 255s, 同端口随后发来的 `haiku` 小调用、`/_health` 探活或其它并发请求仍会由父进程以约 `0.1 - 0.3 ms` COW `fork` 延迟立即派生新子进程并行服务, 彻底从架构根源消解头阻塞与 `/_health` 饿死问题, 不再需要 `hs_poll_register` 或 `HS_DETACH` 异步状态机拆解。

(2) **原一审阻塞项 2: `sy_poll` 遇信号中断返回 `-EINTR` 未明确处置, 可能导致主循环异常退出或信号响应延迟**
    - **新模型消解**: 父进程不再把全部客户端放进大 `sy_poll` 数组, 而是仅对单一 `srv->listen_fd` 调用 `sy_sock_poll(srv->listen_fd, SY_POLLIN, step_ms)` 且强制单步超时 `step_ms <= 200ms`。无论 `sy_sock_poll` 正常超时返回 `0` 还是因信号返回 `-EINTR`, 循环均先检查停止标志与 `sy_sig_taken` (若触发停止则向在途 `child_pids[]` 转发 `SIGTERM` 并进入 `hs_close(drain_ms)`), 再循环调用 `sy_wait_any(SY_WAIT_NOHANG, ...)` 收割所有已退出僵尸子进程, 然后安全重入下一轮 `sy_sock_poll` (彻底消除 `-EINTR` 导致主循环退出或僵尸堆积的隐患)。子进程内 `sy_sock_poll` 遇 `-EINTR` 时亦按剩余 `sy_deadline_left` 重试。

(3) **原一审阻塞项 3: 响应写同步阻塞与异步写缓冲的矛盾 (`hs_res_write_chunk` 若同步等写可阻塞全服, 若不阻塞则需复杂异步写缓冲)**
    - **新模型消解**: 每个子进程只服务一条客户端连接, 独占执行流且不与其他连接共享任何事件循环。因此 `hs_res_send` 与 `hs_res_write_chunk` 无需构建复杂的用户态每连接异步写环形缓冲与水位状态机, 直接在子进程内调用 `sy_sock_poll(client_fd, SY_POLLOUT, left_ms)` + `sy_send` 以 `write_timeout_ms` (默认 30s) 为上限同步阻塞写出即可: 慢客户端只会阻塞或超时结束其自身对应的子进程 (`-ETIMEDOUT`), 绝不会拖累主监听进程或任何其它并发客户端连接。

(4) **`sy_fork_session` 管道闭环四项强制行为探针与两项变异 (片 3/4 与片 8 验收硬合同, 禁只做源码文本检查)**
    - **行为探针 1 (`>= 1 MiB` 写 `stdout`/`stderr` 不阻塞)**: 起真实服务端与子进程, 子进程在 handler 内往 `stdout(fd 1)` 与 `stderr(fd 2)` 各写 `>= 1 MiB` (1048576 字节, 远超 Linux 64 KiB 管道缓冲), 父进程不读管道, 断言子进程正常完成响应并退出 (不超时、不阻塞);
    - **行为探针 2 (连续 200 连接 `/proc/self/fd` 基线零泄漏与子进程无残留管道)**: 连续发起 200 个连接并收割完毕后, 断言父进程 `/proc/self/fd` 描述符数量完全回到建连前基线; 同时在子进程 handler 内检查自身 `/proc/self/fd`, 断言除 `0`、`1`、`2` 与当前 `client_fd` 外无任何残留管道或 `listen_fd`;
    - **行为探针 3 (子进程 `stderr` 穿透到守护进程 `stderr`)**: 将守护进程 `stderr` 指向临时日志文件 fd, 子进程往 `fd 2` 写特定标记串, 断言该标记串真实出现在守护进程 `stderr` 日志中 (而非丢失);
    - **行为变异 A 与 B (落在断言读取行为上并打印改了什么)**:
      - 变异 A: 删掉子进程入口第一行 `fd 1 -> /dev/null` 重定向 -> 行为探针 1 (`1 MiB` 写 `stdout`) 必须因管道写满或 `EPIPE` 失败变红;
      - 变异 B: 删掉父进程 `sy_fork_session` 返回后的 `sy_close(fds[0]); sy_close(fds[1]);` -> 行为探针 2 (200 连接后父进程 `/proc/self/fd` 计数对比基线) 必须因 fd 泄漏变红。

(5) **独立对拍字节比对归一化规则与片 3+4 慢上游 `/_pid` 自检 (2026-11-22 守门人验收骨架对齐)**
    - **字节比对归一化规则 (`1:cc` 审定, 独立验收与实现侧共同遵守的 5 条硬合同)**:
      (a) **响应头归一化**: 响应头键名比较大小写不敏感; 对比前删去 `Date` 头, 其余响应头键名统一转小写并按字典序排序后比对; 实现侧对用户通过 `hs_res_header` 显式设置的 `Content-Type` / `Connection` 做大小写不敏感去重与状态合并, 绝不输出重复或冲突的同名头;
      (b) **响应体解码比对**: 比对解码后的有效载荷字节; `Transfer-Encoding: chunked` 先经 `textx.chunked` `dk_feed` 解码后再比对字节; SSE (`text/event-stream`) 按 `\n\n` 分隔的事件序列逐事件比对;
      (c) **假上游确定性与端点数目**: 假上游响应体中的时间戳固定为常量; 请求计数查询端点仅由验收侧假上游提供, 实现侧 (`ext/net/httpd.cx`) 零依赖;
      (d) **非法与走私请求一致性**: 所有走私/非法请求 (`400`/`408`/`413`/`417`/`431`/`501`/`505`) 响应头必须带 `Connection: close`, 写出后立即关闭连接且绝不复用; 走私类请求 (`CL+TE`、双 `CL`、头折行、头名含空白、`gzip, chunked`、裸 CR/NUL 等) 必须在解析层拦截并直接回错误与关闭连接, **绝不调用 `hs_handler`, 假上游收到的字节与请求计数必须严格为 `0`**;
      (e) **验收仪器非同构与正对照约束**: 验收仪器自身在慢请求期间健康检查必须 `< 100ms` (仪器不得与被测缺陷同构阻塞), 泄漏/并发观测器必须带握连接计数上升的正对照验证.
    - **片 3+4 实现侧自检夹具 (不依赖 `ccogw` 外部脚本)**:
      `tools/check-ext-net-httpd.cx` 片 3+4 自检包含「慢请求/慢上游期间另一连接请求 `/_pid` 与 `/_health` 在 `< 50ms` (满足 `< 100ms` 门线) 内返回 `200 OK`」的独立闭环测试, 片 8+9 补齐 `SO_REUSEPORT` 双实例同端口监听、全局 `max_total_body_bytes` 动态并发体预算护栏与大小写不敏感响应头去重测试及 10 项变异全杀。
