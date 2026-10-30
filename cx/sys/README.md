# cx/sys — 系统对接库（L0，libc/POSIX 边界）

一组带显式错误码（-errno）的系统原语，是「底层 libc/POSIX 对接」的边界层。
上层通过 `#pragma unisa use sys.NAME` 消费，不直接 open/read/fork/getenv。

## 模块与用途

| 模块 | 文件 | 语义 |
|---|---|---|
| `sys.env` | `env.cx` | 环境变量（sy_env/sy_env_vec/sy_env_or/sy_setenv/sy_unsetenv/sy_uid/sy_cwd） |
| `sys.fs` | `fs.cx` | 文件/目录（sy_open/sy_read/sy_write_all/sy_fstat/sy_stat/sy_lstat/sy_realpath/sy_mkdir/sy_rename/sy_unlink/sy_rmdir/sy_exists 等） |
| `sys.proc` | `proc.cx` | 进程（sy_run/sy_spawn/sy_spawn_pipe/sy_pipe/sy_wait/sy_kill/sy_fork_session） |
| `sys.time` | `time.cx` | 时钟（sy_now_ms/sy_wall_s/sy_sleep_ms/sy_set_mtime） |
| `sys.net` | `net.cx` | TCP（sy_tcp_connect/listen/accept/send_all/recv） |
| `sys.posix` | `posix.cx` | 补充 POSIX（sy_flock/sy_lockf/sy_socketpair 等） |
| `sys.mem` | `mem.cx` | 映射内存（sy_map_none/sy_protect_rw/sy_protect_rx/sy_unmap） |
| `sys.dl` | `dl.cx` | 动态装载（sy_dl_addr/sy_dlclose） |
| `sys.sha` | `sha.cx` | 摘要（sy_sha） |
| `sys.guard` | `guard.cx` | setjmp/longjmp（sy_guard/sy_guard_fail） |
| `sys.ffi` | `ffi.cx` | 外来调用（sy_ffi_call） |
| `sys.poll` | `poll.cx` | 轮询（sy_poll） |
| `sys.tty` | `tty.cx` | 终端（sy_tty_raw/sy_tty_size 等） |

## 契约

- 错误码显式：所有接口返回 0 或 -errno（或 -EINVAL/-ENOMEM 等），不依赖 errno 跨模块（坑三）。
- 空指针/空路径/负长度 → -EINVAL，失败不留半开 fd。
- 这是 L0（`cx/sys/` 在 cxraw 白名单），可直接用 libc；上层经 use 消费。

## 检查

`tools/check-cx-sys.cx`（CHECK_CX_SYS PASS ok=108）、`tools/check-cx-sys-spawn.cx`、`tools/check-cx-sys-net.cx`、`tools/check-cx-sys-guard.cx`、`tools/check-cx-sys-ffi.cx`。
