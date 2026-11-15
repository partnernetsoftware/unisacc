<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.sys

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sy_os_name | const char *sy_os_name(void); | QuickJS os.platform; Node process.platform; Rust env::consts::OS | none | impl:lnx | impl:osx | impl:win | exists | 与目标三元组的前半同词 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_arch_name | const char *sy_arch_name(void); | Node process.arch; Rust env::consts::ARCH | none | impl | impl | impl | exists | x86_64 或 arm64 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_page_size | long sy_page_size(void); | Rust(page_size); POSIX sysconf | none | impl | impl:arm64 为 16384 | impl:GetSystemInfo | exists | - | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_ncpu | int sy_ncpu(void); | Node os.availableParallelism; Rust thread::available_parallelism | ret:ge1 | impl | unverified | unverified | exists | 现在 ext.sysx process.cx | pre | 0.1.0 | none | - |
| tn_self | const char *tn_self(void); | Rust env!(TARGET) | none | impl | impl | impl | exists | 完整目标名 os/arch;sy_os_name/sy_arch_name 由它拆出 | pre | 0.1.0 | none | - |
| sy_strerror | const char *sy_strerror(int e); | C strerror; Rust io::Error Display | none | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_errno_name | const char *sy_errno_name(int e); | Node util.getSystemErrorName; Rust io::ErrorKind | null | impl | impl | impl | exists | 收正负号两种写法,返回 ENOENT 这类名字;表由事实表生成 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_errno_of | int sy_errno_of(const char *name); | Node os.constants.errno | null | impl | impl | impl | exists | - | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_read | long sy_read(int fd, void *buf, long n); | QuickJS os.read; Rust Read::read | -errno | impl | impl | unverified | exists | 现在 sys.fs | pre | 0.1.0 | none | - |
| sy_read_once | long sy_read_once(int fd, void *buf, long n); | POSIX read 一次 | -errno | impl | impl | unverified | exists | 现在 sys.fs | pre | 0.1.0 | none | - |
| sy_read_full | int sy_read_full(int fd, void *buf, long n); | Rust Read::read_exact | -errno | impl | impl | unverified | exists | 现在 sys.fs | pre | 0.1.0 | none | - |
| sy_write | long sy_write(int fd, const void *buf, long n); | QuickJS os.write; Rust Write::write | -errno | impl | impl | impl | exists | 一次写,可能部分写入;EINTR 内部重试 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_write_all | int sy_write_all(int fd, const void *buf, long n); | Rust Write::write_all | -errno | impl | impl | unverified | exists | 现在 sys.fs | pre | 0.1.0 | none | - |
| sy_pread | long sy_pread(int fd, void *buf, long n, int64_t off); | Rust FileExt::read_at; Node fs.read position | -errno | impl | impl | refuse:ENOSYS 本版未实现 | exists | - | pre | 0.1.0 | none | - |
| sy_pwrite | long sy_pwrite(int fd, const void *buf, long n, int64_t off); | Rust FileExt::write_at; Node fs.write position | -errno | impl | impl | refuse:ENOSYS 本版未实现 | exists | - | pre | 0.1.0 | none | - |
| sy_lseek | long sy_lseek(int fd, long off, int whence); | QuickJS os.seek; Rust Seek::seek | -errno | impl | impl | unverified | exists | 现在 sys.fs;long 在 win 是 32 位,大偏移用 sy_seek64 | legacy | 0.1.0 | none | - |
| sy_seek64 | int64_t sy_seek64(int fd, int64_t off, int whence); | Rust Seek::seek(u64) | -errno | impl | impl | impl:SetFilePointerEx | exists | - | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_close | int sy_close(int fd); | QuickJS os.close | -errno | impl | impl | unverified | exists | 现在 sys.fs | pre | 0.1.0 | none | - |
| sy_dup | int sy_dup(int fd); | QuickJS os.dup | -errno | impl | impl | unverified | exists | 现在 ext.sysx fd.cx | pre | 0.1.0 | none | - |
| sy_dup2 | int sy_dup2(int oldfd, int newfd); | QuickJS os.dup2 | -errno | impl | impl | refuse:ENOSYS cap=dup2 | exists | 现在 ext.sysx fd.cx | pre | 0.1.0 | none | - |
| sy_pipe | int sy_pipe(int fds[2]); | QuickJS os.pipe; Rust io::pipe | -errno | impl | impl | unverified | exists | 现在 sys.proc | pre | 0.1.0 | none | - |
| sy_fd_nonblock | int sy_fd_nonblock(int fd, int on); | Rust set_nonblocking | -errno;-EBADF | impl | impl | refuse:ENOSYS 前导库无 fcntl | exists | - | pre | 0.1.0 | none | - |
| sy_fd_cloexec | int sy_fd_cloexec(int fd, int on); | Rust 默认 O_CLOEXEC; POSIX fcntl FD_CLOEXEC | -errno;-EBADF | impl | impl | refuse:ENOSYS 前导库无 fcntl | exists | - | pre | 0.1.0 | none | - |
| sy_isatty | int sy_isatty(int fd); | QuickJS os.isatty; Node tty.isatty; Rust IsTerminal | 0/1 | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_pollfd_t | typedef struct { int fd; short events; short revents; } sy_pollfd_t; | POSIX struct pollfd | none | impl | impl | refuse:ENOSYS 无 poll | exists | 现在 ext.sysx poll.cx | pre | 0.1.0 | none | - |
| sy_poll | int sy_poll(sy_pollfd_t *fds, int n, long ms); | POSIX poll; QuickJS os.setReadHandler 的同步版 | -errno;-EINVAL | impl | impl | refuse:ENOSYS 无 poll | exists | 现在 ext.sysx poll.cx;核心唯一的多路等待(事件循环在 OUT);n 超 SY_POLL_MAX | pre | 0.1.0 | none | - |
| sy_readable | int sy_readable(int fd, long ms); | POSIX poll 单 fd | -errno;ret:1-ready-0-timeout | impl | impl | refuse:ENOSYS 无就绪等待 | exists | 现在 sys.time | pre | 0.1.0 | none | - |
| sy_printf | int sy_printf(const char *fmt, ...); | QuickJS std.printf; Rust print! | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_eprintf | int sy_eprintf(const char *fmt, ...); | Rust eprint!; QuickJS std.err.printf | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_vprintf | int sy_vprintf(const char *fmt, va_list ap); | C vprintf | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_veprintf | int sy_veprintf(const char *fmt, va_list ap); | C vfprintf(stderr) | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_puts | int sy_puts(const char *s); | QuickJS std.puts; Rust println! | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_map_none | int sy_map_none(void **out, long n); | Rust(memmap2) MmapMut::map_anon; POSIX mmap PROT_NONE | -errno | impl | impl | refuse:ENOSYS cap=mmap-exec | exists | - | internal | 0.1.0 | none | - |
| sy_unmap | int sy_unmap(void *p, long n); | POSIX munmap | -errno | impl | impl | refuse:ENOSYS cap=mmap-exec | exists | - | internal | 0.1.0 | none | - |
| sy_protect_rw | int sy_protect_rw(void *p, long n); | POSIX mprotect RW | -errno | impl | impl | refuse:ENOSYS cap=mmap-exec | exists | - | internal | 0.1.0 | none | - |
| sy_protect_rx | int sy_protect_rx(void *p, long n); | POSIX mprotect RX | -errno | impl | impl | refuse:ENOSYS cap=mmap-exec | exists | - | internal | 0.1.0 | none | - |
| sy_sig_handle | int sy_sig_handle(int sig, void (*fn)(int)); | QuickJS os.signal; Node process.on(signal); POSIX sigaction | -errno | impl | refuse:ENOSYS cap=posix-signal | refuse:ENOSYS cap=posix-signal | exists | 可移植的做法是 sy_sig_watch/sy_sig_taken | pre | 0.1.0 | none | - |
| sy_sig_default | int sy_sig_default(int sig); | QuickJS os.signal(sig, null); SIG_DFL | -errno | impl | impl | refuse:ENOSYS cap=posix-signal | exists | - | pre | 0.1.0 | none | - |
| sy_sig_ignore | int sy_sig_ignore(int sig); | QuickJS os.signal(sig, undefined); SIG_IGN | -errno | impl | impl | refuse:ENOSYS cap=posix-signal | exists | - | pre | 0.1.0 | none | - |
| sy_raise | int sy_raise(int sig); | C raise; Node process.kill(process.pid) | -errno | impl | impl | refuse:ENOSYS cap=posix-signal | exists | - | pre | 0.1.0 | none | - |
| sy_sig_watch | int sy_sig_watch(int sig); | Rust(signal-hook) flag::register; Node process.on(signal) | -errno;-EINVAL | impl:处理函数只置位 | refuse:ENOSYS 随 sy_sig_handle,kqueue 未做 | refuse:ENOSYS 随 sy_sig_handle,SetConsoleCtrlHandler 未做 | exists | cx/sys/sigwatch.cx;置位式,不跑用户回调;经 sy_sig_handle 实现,仅 Linux;不可捕获的信号 -EINVAL | pre | 0.1.0 | none | - |
| sy_sig_taken | int sy_sig_taken(int sig); | Rust(signal-hook) AtomicBool::swap | -EINVAL;ret:needlen | impl | refuse:ENOSYS 随 sy_sig_watch | refuse:ENOSYS 随 sy_sig_watch | exists | cx/sys/sigwatch.cx;未 watch 返回 0;返回自上次调用以来的次数并清零 | pre | 0.1.0 | none | - |
| sy_sig_name | const char *sy_sig_name(int sig); | Node os.constants.signals 反查 | null | impl | impl | impl | exists | 返回 SIGINT 这类名字;表由事实表生成 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_sig_number | int sy_sig_number(const char *name); | Node os.constants.signals | null | impl | impl | impl | exists | 各平台编号不同(SIGUSR1 等),上层不得写死数字 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_atexit | int sy_atexit(void (*fn)(void)); | C atexit; Node process.on(exit) | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_exit | void sy_exit(int code); | QuickJS std.exit; Rust process::exit | noreturn | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_tty_t | typedef struct { unsigned char b[128]; } sy_tty_t; | POSIX struct termios(不透明) | none | impl | impl | refuse:ENOSYS cap=termios | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_tty_get | int sy_tty_get(int fd, sy_tty_t *t); | POSIX tcgetattr | -errno | impl | impl | refuse:ENOSYS cap=termios | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_tty_set | int sy_tty_set(int fd, const sy_tty_t *t, int flush); | POSIX tcsetattr; QuickJS os.ttySetRaw 的还原 | -errno | impl | impl | refuse:ENOSYS cap=termios | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_tty_same | int sy_tty_same(const sy_tty_t *a, const sy_tty_t *b); | 无 | 0/1 | impl | impl | impl | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_tty_raw | int sy_tty_raw(sy_tty_t *t); | QuickJS os.ttySetRaw; Node setRawMode; POSIX cfmakeraw | -errno | impl | impl | refuse:ENOSYS cap=termios | exists | 现在 ext.sysx tty.cx | pre | 0.1.0 | none | - |
| sy_tty_size | int sy_tty_size(int fd, int *rows, int *cols); | QuickJS os.ttyGetWinSize; Node stdout.rows/columns | -errno;-ENOTTY | impl:TIOCGWINSZ | impl:TIOCGWINSZ | refuse:ENOSYS 现未提供 | exists | 现在 ext.sysx tty.cx;win 改用 GetConsoleScreenBufferInfo 属版本升级 | pre | 0.1.0 | none | - |
| sy_tcgetpgrp | long sy_tcgetpgrp(int fd); | POSIX tcgetpgrp | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_tcsetpgrp | int sy_tcsetpgrp(int fd, int pgrp); | POSIX tcsetpgrp | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_err_kind | int sy_err_kind(int e); | Rust io::Error::kind; Python OSError | none | impl | impl | impl | exists | 返回 SY_K_* 类,未知码归 OTHER | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_cap | int sy_cap(int cap); | QuickJS 能力表 | 0/1 | impl | impl | impl | exists | 能力查询,未知能力为 0 | pre | 0.1.0 | none | cx-l0-sys-new |
| sy_res_live | long sy_res_live(int kind); | Python ResourceWarning 计数 | -EINVAL | impl | unverified | unverified | exists | cx/sys/reslive.cx;经 sy_res_note 登记且未注销的句柄数,不是进程级总数;kind 为 SY_RES_FD=1、SY_RES_PROC=2、SY_RES_MAP=3,其余 -EINVAL;不线程安全 | pre | 0.1.0 | none | - |
| sy_res_note | long sy_res_note(int kind, int delta); | Python ResourceWarning 计数 | -EINVAL | impl | unverified | unverified | exists | cx/sys/reslive.cx;登记点,delta 只许 +1/-1,返回新计数;未知 kind、其它 delta、减到负数返回 -EINVAL 且值不变 | pre | 0.1.0 | none | - |
