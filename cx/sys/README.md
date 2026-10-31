# cx/sys — 系统对接库（L0，libc/POSIX 边界）

一组带显式错误码（-errno）的系统原语，是「底层 libc/POSIX 对接」的边界层。
上层通过 `#pragma unisa use sys.NAME` 消费，不直接 open/read/fork/getenv。

## 模块与用途

| 模块 | 文件 | 语义 |
|---|---|---|
| `sys.env` | `env.cx` | 环境变量（sy_env/sy_env_vec/sy_env_or/sy_setenv/sy_unsetenv/sy_uid/sy_cwd） |
| `sys.fs` | `fs.cx` | 文件/目录（sy_open/sy_read/sy_read_full/sy_read_once/sy_write_all/sy_copy/sy_access/sy_fstat/sy_stat/sy_lstat/sy_realpath/sy_mkdir/sy_mkfifo/sy_rename/sy_unlink/sy_rmdir/sy_symlink/sy_readlink/sy_truncate/sy_fsync/sy_fdatasync/sy_exists 等） |
| `sys.proc` | `proc.cx` | 进程（sy_run/sy_spawn/sy_spawn_pipe/sy_pipe/sy_wait/sy_kill/sy_fork_session） |
| `sys.time` | `time.cx` | 时钟（sy_now_ms/sy_wall_s/sy_sleep_ms/sy_set_mtime） |
| `sys.net` | `net.cx` | TCP（sy_tcp_connect/listen/accept/send_all/recv） |
| `sys.posix` | `posix.cx` | 补充 POSIX（sy_flock/sy_lockf/sy_socketpair 等） |
| `sys.mem` | `mem.cx` | 映射内存（sy_map_none/sy_protect_rw/sy_protect_rx/sy_unmap） |
| `sys.dl` | `dl.cx` | 映射地址（sy_dl_addr/sy_rtld_now），无宿主符号 |
| `sys.dlcall` | `dlcall.cx` | 宿主装载调用（sy_dlopen/sy_dlsym/sy_dlclose）；静态 33 对象产品不得拉入 |
| `sys.sha` | `sha.cx` | 摘要（sy_sha） |
| `sys.guard` | `guard.cx` | setjmp/longjmp（sy_guard/sy_guard_fail） |
| `sys.ffi` | `ffi.cx` | 外来调用（sy_ffi_call） |
| `sys.poll` | `poll.cx` | 轮询（sy_poll） |
| `sys.tty` | `tty.cx` | 终端（sy_tty_raw/sy_tty_size 等） |
| `sys.fd` | `fd.cx` | 描述符复制（sy_dup/sy_dup2；Windows 上 sy_dup2 为 -ENOSYS） |

## 非 `sys.*` 的 L0 文件

同目录下还有四个不带 `sys.` 名字的模块和一个 C 文件，它们直接对接宿主、因此属于 L0：

| 文件 | 用途 |
|---|---|
| `kernel.cx` | 执行核事实表（模块 `kernel`，`.cx` 入口经 `use sys.kernel` 消费） |
| `process.cx` | 进程捕获（`np_run`/`np_free`，源驱动器用） |
| `process_group.cx` | Linux 进程组（`setpgid` 等的对接实现） |
| `process_group_osx.cx` | Darwin 进程组提供者（x86-64 与 arm64 的 syscall 号 82） |
| `lnx-x86_64.c` | Linux `setpgid` 的 `__syscall6` 提供者；`cx/` 下唯一的分层内 C 文件 |

## 契约

- 错误码显式：所有接口返回 0 或 -errno（或 -EINVAL/-ENOMEM 等），不依赖 errno 跨模块（坑三）。
- 空指针/空路径/负长度 → -EINVAL，失败不留半开 fd。
- 平台差异收在本层：`sy_*_os` 这类 static 助手里写 `#ifdef`（如 fs.cx 的 `sy_symlink_os`/`sy_exists_os`），公开接口无平台分支，上层不看 `_WIN32`/`__linux__`（主人范式「优雅简约」：平台差异进 L0 或表）。
- 这是 L0（`cx/sys/` 在 cxraw 白名单），可直接用 libc；上层经 use 消费。

## 检查

`tools/check-cx-sys.cx`（CHECK_CX_SYS PASS ok=108）、`tools/check-cx-sys-spawn.cx`、`tools/check-cx-sys-net.cx`、`tools/check-cx-sys-guard.cx`、`tools/check-cx-sys-ffi.cx`、`tools/check-cx-sys-fd.cx`。
