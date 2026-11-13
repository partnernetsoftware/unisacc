<!-- 本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改 -->

# API: cx.os

本文件由 tools/surface-doc-gen.cx 自 build/std-surface.tsv 生成,勿手改。列义见该表文件头。

| name | signature | analogue | errors | lnx | osx | win | status | note | stab | since | own | test |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sy_stat_t | typedef struct { long size; long ino; long mtime; int mode; int kind; long uid; unsigned long dev; long ctime; } sy_stat_t; | Rust fs::Metadata; Node fs.Stats | none | impl | impl | impl:dev 与 ino 为 0,kind 只有 f/d | exists | size 是 long,win 上 32 位;大文件用 sy_file_size;pending E4 | pre | 0.1.0 | none | - |
| sy_fstat | int sy_fstat(int fd, sy_stat_t *out); | Node fs.fstat; QuickJS os.stat | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_stat | int sy_stat(const char *path, sy_stat_t *out); | Rust fs::metadata; Node fs.stat; QuickJS os.stat | -errno | impl | impl | impl:GetFileAttributesExA,缺该入口时 -ENOSYS | exists | - | pre | 0.1.0 | none | - |
| sy_lstat | int sy_lstat(const char *path, sy_stat_t *out); | Rust fs::symlink_metadata; Node fs.lstat; QuickJS os.lstat | -errno | impl | impl | impl:等同 sy_stat,kind 只有 f/d | exists | - | pre | 0.1.0 | none | - |
| sy_reg_size | long sy_reg_size(int fd); | Rust Metadata::len | -errno | impl | impl | unverified | exists | - | legacy | 0.1.0 | none | - |
| sy_file_size | int sy_file_size(const char *path, int64_t *out); | Rust Metadata::len; Node Stats.size | -errno | impl | impl | impl:GetFileAttributesExA 高低两半 | exists | 64 位长度,补 sy_stat_t.size 在 win 的 32 位上限 | pre | 0.1.0 | none | - |
| sy_is_reg | int sy_is_reg(const char *path); | Rust Path::is_file | -errno;0/1 | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_exists | int sy_exists(const char *path); | Rust Path::try_exists; Node fs.existsSync | -errno;0/1 | impl | impl | unverified | exists | 只把 ENOENT 当不存在 | pre | 0.1.0 | none | - |
| sy_access | int sy_access(const char *path, int mode); | Node fs.access | -errno | impl | impl | refuse:ENOSYS 前导库无 _access | exists | - | pre | 0.1.0 | none | - |
| sy_open | int sy_open(const char *path, int flags, int mode); | QuickJS os.open; Node fs.open | -errno | impl | impl | impl:无 O_CLOEXEC/O_NOFOLLOW/O_NONBLOCK | exists | - | pre | 0.1.0 | open:fd close:sy_close | - |
| sy_open_new | int sy_open_new(const char *path, int mode); | Rust OpenOptions::create_new | -errno;-EEXIST | impl | impl | unverified | exists | - | pre | 0.1.0 | open:fd close:sy_close | - |
| sy_open_read | int sy_open_read(const char *path); | Rust File::open | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | open:fd close:sy_close | - |
| sy_openat | int sy_openat(int dfd, const char *path, int flags, int mode); | Rust(cap-std) Dir::open | -errno | impl | impl | refuse:ENOSYS cap=at | exists | 现在 sys.posix | internal | 0.1.0 | open:fd close:sy_close | - |
| sy_at_fdcwd | int sy_at_fdcwd(void); | POSIX AT_FDCWD | none | impl | impl | refuse:ENOSYS cap=at | exists | 现在 sys.posix | internal | 0.1.0 | none | - |
| sy_mkdirat | int sy_mkdirat(int dfd, const char *name, int mode); | POSIX mkdirat | -errno | impl | impl | refuse:ENOSYS cap=at | exists | 现在 sys.posix | internal | 0.1.0 | none | - |
| sy_unlinkat | int sy_unlinkat(int dfd, const char *name, int dir); | POSIX unlinkat | -errno | impl | impl | refuse:ENOSYS cap=at | exists | 现在 sys.posix | internal | 0.1.0 | none | - |
| sy_linkat | int sy_linkat(int oldd, const char *oldp, int newd, const char *newp, int flags); | POSIX linkat | -errno | impl | impl | refuse:ENOSYS cap=at | exists | 现在 sys.posix | internal | 0.1.0 | none | - |
| sy_link | int sy_link(const char *from, const char *to); | Rust fs::hard_link; Node fs.link | -errno | impl | impl | refuse:ENOSYS 本版未实现 | exists | - | pre | 0.1.0 | none | - |
| sy_mkdir | int sy_mkdir(const char *path, int mode); | Rust fs::create_dir; QuickJS os.mkdir | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_mkdir_all | int sy_mkdir_all(const char *path, int mode); | Rust fs::create_dir_all; Node fs.mkdir recursive | -errno;-ENOTDIR | impl | impl | impl:两种分隔符都切 | exists | 已存在目录记 0,已存在非目录 -ENOTDIR | pre | 0.1.0 | none | - |
| sy_rmdir | int sy_rmdir(const char *path); | Rust fs::remove_dir; Node fs.rmdir | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_unlink | int sy_unlink(const char *path); | Rust fs::remove_file; QuickJS os.remove | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_remove_all | int sy_remove_all(const char *path); | Rust fs::remove_dir_all; Node fs.rm recursive | -errno;-EINVAL | impl | impl | refuse:ENOSYS 本版未实现 | exists | 不存在记 0;不跟随符号链接;拒绝空串、/ 与 . | pre | 0.1.0 | none | - |
| sy_rename | int sy_rename(const char *from, const char *to); | Rust fs::rename; QuickJS os.rename | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_copy | int sy_copy(const char *src, const char *dst, int mode); | Rust fs::copy; Node fs.copyFile | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_chmod | int sy_chmod(const char *path, int mode); | Rust fs::set_permissions; Node fs.chmod | -errno | impl | impl | unverified | exists | win 只该映射只读位(ACL 在 OUT) | pre | 0.1.0 | none | - |
| sy_symlink | int sy_symlink(const char *target, const char *link); | QuickJS os.symlink; Node fs.symlink | -errno;-EEXIST | impl | impl | refuse:ENOSYS cap=symlink | exists | - | pre | 0.1.0 | none | - |
| sy_readlink | long sy_readlink(const char *path, char *dst, long cap); | QuickJS os.readlink; Rust fs::read_link | -errno | impl | impl | refuse:ENOSYS cap=symlink | exists | - | pre | 0.1.0 | none | - |
| sy_realpath | int sy_realpath(const char *path, char *dst, int cap); | QuickJS os.realpath; Rust fs::canonicalize | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_abspath | int sy_abspath(const char *path, char *dst, int cap); | Rust path::absolute; Node path.resolve | -errno;-ERANGE | impl | impl | impl:带盘符与 UNC | exists | 只拼 cwd 并规整,不解符号链接,路径可不存在 | pre | 0.1.0 | none | - |
| sy_mkfifo | int sy_mkfifo(const char *path, int mode); | POSIX mkfifo | -errno | impl | refuse:ENOSYS 前导库无 mkfifo | refuse:ENOSYS cap=fifo | exists | - | pre | 0.1.0 | none | - |
| sy_truncate | int sy_truncate(int fd, long size); | Rust File::set_len; Node fs.ftruncate | -errno;-EINVAL | impl | impl | refuse:ENOSYS 前导库无 ftruncate | exists | pending E4 | pre | 0.1.0 | none | - |
| sy_fsync | int sy_fsync(int fd); | Rust File::sync_all; Node fs.fsync | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_fdatasync | int sy_fdatasync(int fd); | Rust File::sync_data | -errno | impl | impl:退回 fsync | refuse:ENOSYS | exists | - | pre | 0.1.0 | none | - |
| sy_dir_names | int sy_dir_names(int dfd, char *out, int cap); | QuickJS os.readdir | -errno | impl | impl:按头文件写成,源码注明未在 Mac 实跑 | refuse:ENOSYS 无 getdents | exists | 旧接口:一次倒出全部名字且不带类型;新代码用 sy_dir_open/next | legacy | 0.1.0 | none | - |
| sy_dir_t | typedef struct { int h; int kind; long name_n; char name[256]; } sy_dir_t; | Rust fs::DirEntry; Node fs.Dirent | none | impl | impl | refuse:ENOSYS 本版未实现 | exists | kind 取值同 sy_stat_t.kind;h 是运行期句柄 | pre | 0.1.0 | none | - |
| sy_dir_open | int sy_dir_open(sy_dir_t *d, const char *path); | Rust fs::read_dir; Node fs.opendir | -errno | impl:getdents64 | impl:getdirentries64 | refuse:ENOSYS 本版未实现 | exists | - | pre | 0.1.0 | open:dir close:sy_dir_close | - |
| sy_dir_next | int sy_dir_next(sy_dir_t *d); | Rust ReadDir::next; Node Dir.read | -errno;ret:1-item-0-end;-ENAMETOOLONG | impl | impl | refuse:ENOSYS 本版未实现 | exists | 跳过 . 和 ..;类型未知时补一次 lstat;顺序不保证;名字超 255 记 -ENAMETOOLONG | pre | 0.1.0 | none | - |
| sy_dir_close | int sy_dir_close(sy_dir_t *d); | Node Dir.close | -errno;-EBADF | impl | impl | refuse:ENOSYS 本版未实现 | exists | 重复关闭 -EBADF | pre | 0.1.0 | none | - |
| sy_utimes | int sy_utimes(const char *path, long atime_s, long atime_ns, long mtime_s, long mtime_ns); | QuickJS os.utimes; Node fs.utimes | -errno | impl:utimensat | impl:utimensat | refuse:ENOSYS 本版未实现 | exists | pending E4 | pre | 0.1.0 | none | - |
| sy_set_mtime | int sy_set_mtime(const char *path, long sec); | Rust File::set_modified | -errno | impl | impl | impl:SetFileTime,缺入口时 -ENOSYS | exists | 现在 sys.time | legacy | 0.1.0 | none | - |
| sy_read_file | int sy_read_file(const char *path, unsigned char **out, int64_t *n, int64_t max); | Rust fs::read; QuickJS std.loadFile; Node fs.readFile | -errno;-EFBIG;-ENOMEM | impl | impl | impl | exists | *out 由 mm_free 释放,末尾多放一个 0 不计入 n;超过 max 记 -EFBIG;分配失败 -ENOMEM | pre | 0.1.0 | none | - |
| sy_write_file | int sy_write_file(const char *path, const void *buf, int64_t n, int mode); | Rust fs::write; Node fs.writeFile | -errno | impl | impl | impl | exists | 截断重写,不保证原子 | pre | 0.1.0 | none | - |
| sy_append_file | int sy_append_file(const char *path, const void *buf, int64_t n, int mode); | Node fs.appendFile; Rust OpenOptions::append | -errno | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_write_atomic | int sy_write_atomic(const char *path, const void *buf, int64_t n, int mode); | Rust(tempfile) persist; Node 写临时再 rename | -errno;-EINVAL | impl:同目录临时 + fsync + rename | impl:同目录临时 + fsync + rename | impl:MoveFileExA REPLACE_EXISTING | exists | 失败时目标不变且临时文件已删 | pre | 0.1.0 | none | - |
| sy_mkstemp | int sy_mkstemp(const char *dir, const char *prefix, char *path, int cap); | QuickJS os.mkstemp; Rust(tempfile) NamedTempFile | -errno;-ERANGE;ret:fd | impl | impl | impl | exists | dir 为 0 时用 sy_tmpdir;O_EXCL 0600 建 | pre | 0.1.0 | none | - |
| sy_mkdtemp | int sy_mkdtemp(const char *dir, const char *prefix, char *path, int cap); | QuickJS os.mkdtemp; Node fs.mkdtemp | -errno;-ERANGE | impl | impl | impl | exists | 0700 建 | pre | 0.1.0 | none | - |
| sy_chdir | int sy_chdir(const char *path); | QuickJS os.chdir; Node process.chdir | -errno | impl | impl | impl:SetCurrentDirectoryA | exists | - | pre | 0.1.0 | none | - |
| sy_cwd | int sy_cwd(char *dst, int cap); | QuickJS os.getcwd; Rust env::current_dir | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_flock | int sy_flock(int fd, int op); | POSIX flock | -errno | impl | impl | refuse:ENOSYS cap=flock | exists | 现在 sys.posix | legacy | 0.1.0 | none | - |
| sy_lockf | int sy_lockf(int fd, int write, int wait); | Rust File::lock/try_lock | -errno | impl | impl | refuse:ENOSYS 无 fcntl 与 struct flock | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_unlockf | int sy_unlockf(int fd); | Rust File::unlock | -errno | impl | impl | refuse:ENOSYS 无 fcntl 与 struct flock | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_spawn_t | typedef struct { char **argv; char **env; const char *cwd; int in_fd, out_fd, err_fd; int flags; int pgroup; long deadline_ms; } sy_spawn_t; | Rust process::Command; Node child_process.spawn options; QuickJS os.exec options | none | impl | impl | impl | exists | 唯一的进程选项结构。env 为 0 继承;in/out/err 取 fd 或 SY_STDIO_INHERIT/-NULL/-PIPE;pgroup 0 继承、-1 新组、>0 加入;deadline_ms 0 不限;pending E5 | pre | 0.1.0 | none | - |
| sy_child_t | typedef struct { int pid; int in_fd, out_fd, err_fd; long deadline_at_ms; int h; } sy_child_t; | Rust process::Child; Node ChildProcess | none | impl | impl | impl:h 存进程 HANDLE 槽 | exists | PIPE 一侧的父端 fd 放这里,未用为 -1;pending E5 | pre | 0.1.0 | none | - |
| sy_exit_t | typedef struct { int exited; int code; int sig; int timed_out; int raw; } sy_exit_t; | Rust process::ExitStatus; Node exit code + signal | none | impl | impl | impl:sig 恒 0 | exists | - | pre | 0.1.0 | none | - |
| sy_spawn_init | void sy_spawn_init(sy_spawn_t *o); | Rust Command::new | none | impl | impl | impl | exists | 全部继承、无期限 | pre | 0.1.0 | none | - |
| sy_proc_spawn | int sy_proc_spawn(const sy_spawn_t *o, sy_child_t *c); | Rust Command::spawn; Node child_process.spawn; QuickJS os.exec block=false | -errno;-ENOENT;-EINVAL | impl:fork + execve,出错经 cloexec 管道回传 | impl:fork + execve | impl:CreateProcessA,pgroup 用 CREATE_NEW_PROCESS_GROUP | exists | 取代 sy_run/sy_spawn/sy_spawn_inherit/sy_spawn_pipe/sy_fork_session/np_run/rl_*;不经 shell;找不到程序;空 argv | pre | 0.1.0 | none | - |
| sy_proc_wait | int sy_proc_wait(sy_child_t *c, sy_exit_t *st); | Rust Child::wait; QuickJS os.waitpid | -errno;-EINVAL | impl | impl | impl:WaitForSingleObject + TerminateProcess | exists | 到期先杀(有 pgroup 杀整组)再收尸,st->timed_out=1 | pre | 0.1.0 | none | - |
| sy_proc_try_wait | int sy_proc_try_wait(sy_child_t *c, sy_exit_t *st); | Rust Child::try_wait; QuickJS os.waitpid WNOHANG | -errno;ret:1-exited-0-running | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_proc_kill | int sy_proc_kill(sy_child_t *c, int sig); | Rust Child::kill; Node subprocess.kill; QuickJS os.kill | -errno;-ESRCH | impl | impl | impl:仅 SIGKILL/SIGTERM 走 TerminateProcess,其余 -EINVAL | exists | 有 pgroup 时发给整组 | pre | 0.1.0 | none | - |
| sy_proc_close | int sy_proc_close(sy_child_t *c); | Rust drop(Child) | -errno | impl | impl | impl:关 HANDLE | exists | 关父端管道 fd;不杀不等 | pre | 0.1.0 | none | - |
| sy_proc_run | int sy_proc_run(const sy_spawn_t *o, sy_exit_t *st); | Rust Command::status; Node spawnSync; QuickJS os.exec block=true | -errno | impl | impl | impl | exists | spawn + wait + close 的合成;捕获输出在 ext.procx np_exec | pre | 0.1.0 | none | - |
| sy_exit_decode | int sy_exit_decode(int raw, sy_exit_t *st); | Rust ExitStatusExt::signal; POSIX WIFEXITED/WTERMSIG | -EINVAL | impl | impl | impl:raw 即退出码 | exists | - | pre | 0.1.0 | none | - |
| sy_exit_code | int sy_exit_code(const sy_exit_t *st); | shell $? | none | impl | impl | impl | exists | 退出码;信号记 128+sig;超时记 124 | pre | 0.1.0 | none | - |
| sy_run | int sy_run(char **argv, char *out, int cap, int *cut); | Node execFileSync | -errno | impl | impl | unverified | exists | 旧入口,由 sy_proc_spawn/np_exec 取代,实现后改为薄封装 | legacy | 0.1.0 | none | - |
| sy_spawn | int sy_spawn(char **argv); | Node spawn detached | -errno | impl | impl | unverified | exists | 旧入口,由 sy_proc_spawn 取代 | legacy | 0.1.0 | none | - |
| sy_spawn_inherit | int sy_spawn_inherit(char **argv, int keep); | Node spawn stdio inherit | -errno | impl | impl | unverified | exists | 旧入口,由 sy_proc_spawn 取代 | legacy | 0.1.0 | none | - |
| sy_spawn_pipe | int sy_spawn_pipe(char **argv, const char *cwd, int flags, int *out_fd, int *err_fd); | Node spawn stdio pipe | -errno | impl | impl | unverified | exists | 旧入口,由 sy_proc_spawn 取代 | legacy | 0.1.0 | none | - |
| sy_fork_session | int sy_fork_session(int (*fn)(void *), void *ud, int fds[2]); | POSIX fork + setsid | -errno | impl | impl | unverified | exists | 旧入口;新面不提供 fork 回调,只有 spawn(pgroup=-1) | internal | 0.1.0 | none | - |
| rl_tmpfile_exec | int rl_tmpfile_exec(const unsigned char *image, long n, char **argv); | 无(产品自举专用) | -errno | refuse:ENOSYS cap=memfd-exec | impl:fork/execv | impl:CreateProcessA 经 hostcall | exists | 运行装载专用,不属通用面;其进程部分改走 sy_proc_spawn | internal | 0.1.0 | none | - |
| rl_run_link | int rl_run_link(const char *argv0, char *dst, int cap); | 无(产品自举专用) | -errno | impl | impl | unverified | exists | 运行装载专用,不属通用面 | internal | 0.1.0 | none | - |
| sy_wait_status | int sy_wait_status(int pid, int nohang, int *exited, int *code, int *sig); | QuickJS os.waitpid | -errno | impl | impl | unverified | exists | 裸 pid 版;新代码用 sy_proc_wait | legacy | 0.1.0 | none | - |
| sy_wait | int sy_wait(int pid, int nohang); | QuickJS os.waitpid | -errno | impl | impl | unverified | exists | - | legacy | 0.1.0 | none | - |
| sy_signal | int sy_signal(int pid, int sig); | QuickJS os.kill; Node process.kill | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_kill | int sy_kill(int pid); | Rust Child::kill | -errno | impl | impl | unverified | exists | - | legacy | 0.1.0 | none | - |
| sy_pid | int sy_pid(void); | Rust process::id; QuickJS os.getpid; Node process.pid | none | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_ppid | int sy_ppid(void); | Node process.ppid; Rust os::unix::process::parent_id | -errno | impl | impl | impl:Toolhelp32 快照查父 | exists | - | pre | 0.1.0 | none | - |
| sy_setpgid | int sy_setpgid(int pid, int pgid); | POSIX setpgid | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_getpgid | long sy_getpgid(int pid); | POSIX getpgid | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_setsid | long sy_setsid(void); | POSIX setsid | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_killpg | int sy_killpg(int pgrp, int sig); | POSIX killpg | -errno | impl | impl | refuse:ENOSYS cap=pgroup | exists | 现在 sys.posix | pre | 0.1.0 | none | - |
| sy_rlimit_as | long sy_rlimit_as(void); | POSIX getrlimit RLIMIT_AS | -errno;-EINVAL | impl | impl | impl:恒 0,无地址空间上限 | exists | 现在 sys.posix;无上限记 0 | pre | 0.1.0 | none | - |
| sy_env | const char *sy_env(const char *name); | QuickJS std.getenv; Rust env::var; Node process.env | null | impl | impl | impl | exists | 0 表未设 | pre | 0.1.0 | none | - |
| sy_env_or | const char *sy_env_or(const char *name, const char *fallback); | Rust env::var().unwrap_or | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_env_vec | char **sy_env_vec(void); | QuickJS std.getenviron; Rust env::vars | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_setenv | int sy_setenv(const char *name, const char *val, int overwrite); | QuickJS std.setenv; Rust env::set_var | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_unsetenv | int sy_unsetenv(const char *name); | QuickJS std.unsetenv; Rust env::remove_var | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_tmpdir | const char *sy_tmpdir(void); | Node os.tmpdir; Rust env::temp_dir | none | impl:TMPDIR 再 /tmp | impl:TMPDIR 再 /tmp | impl:TMPDIR/TEMP/TMP,都没有时为 .,统一用 / | exists | - | pre | 0.1.0 | none | - |
| sy_homedir | int sy_homedir(char *dst, int cap); | Node os.homedir; Rust env::home_dir | -ENOENT;-ERANGE | impl:HOME | impl:HOME | impl:USERPROFILE(无 HOMEDRIVE+HOMEPATH 回退;未在目标上实跑) | exists | 只读环境变量,不查 passwd;未设 | pre | 0.1.0 | none | - |
| sy_hostname | int sy_hostname(char *dst, int cap); | Node os.hostname | -errno;-ERANGE | impl:/proc/sys/kernel/hostname,再 HOSTNAME/COMPUTERNAME 环境变量 | impl:仅环境变量,未在目标上实跑 | impl:仅环境变量,未在目标上实跑 | exists | - | pre | 0.1.0 | none | - |
| sy_uid | int sy_uid(void); | Node os.userInfo().uid | none | impl | impl | impl:恒 0 | exists | - | pre | 0.1.0 | none | - |
| sy_euid | long sy_euid(void); | POSIX geteuid; Node process.geteuid | none | impl | impl | impl:恒 0 | exists | - | pre | 0.1.0 | none | - |
| sy_self_path | int sy_self_path(char *dst, int cap); | Rust env::current_exe; Node process.execPath | -errno | impl:/proc/self/exe | impl | impl:GetModuleFileNameA,缺入口时 -ENOSYS | exists | - | pre | 0.1.0 | none | - |
| sy_ts_t | typedef struct { long sec; long nsec; } sy_ts_t; | Rust Duration; POSIX timespec | none | impl | impl | impl | exists | pending E4 | pre | 0.1.0 | none | - |
| sy_now_ms | long sy_now_ms(void); | QuickJS os.now; Node performance.now; Rust Instant::now | none | impl | impl | unverified | exists | - | legacy | 0.1.0 | none | - |
| sy_wall_s | long sy_wall_s(void); | Rust SystemTime::now; JS Date.now/1000 | none | impl | impl | unverified | exists | - | legacy | 0.1.0 | none | - |
| sy_mono | int sy_mono(sy_ts_t *t); | Rust Instant::now | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_wall | int sy_wall(sy_ts_t *t); | Rust SystemTime::now | -errno | impl | impl | unverified | exists | - | pre | 0.1.0 | none | - |
| sy_ts_ms | long sy_ts_ms(const sy_ts_t *from, const sy_ts_t *to); | Rust Instant::duration_since | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_mono_ns | int64_t sy_mono_ns(void); | Node process.hrtime.bigint; Rust Instant | -errno | impl | impl | impl:QueryPerformanceCounter | exists | 64 位,不受 win 的 32 位 long 限制 | pre | 0.1.0 | none | - |
| sy_sleep_ms | int sy_sleep_ms(long ms); | QuickJS os.sleep; Rust thread::sleep | -errno | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_deadline_t | typedef struct { int64_t at_ns; } sy_deadline_t; | Rust Instant(截止点); Go context deadline | none | impl | impl | impl | exists | 单调钟上的绝对时刻;at_ns 为 0 表示无期限 | pre | 0.1.0 | none | - |
| sy_deadline_in | sy_deadline_t sy_deadline_in(long ms); | Rust Instant::now() + Duration | none | impl | impl | impl | exists | ms 小于等于 0 得到已到期 | pre | 0.1.0 | none | - |
| sy_deadline_never | sy_deadline_t sy_deadline_never(void); | Rust Option<Instant>::None | none | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_deadline_left | long sy_deadline_left(const sy_deadline_t *d); | Rust Instant::saturating_duration_since | none;-EINVAL | impl | impl | impl | exists | 返回值可直接作 sy_poll/sy_readable 的 ms;到期记 0,无期限记 -1 | pre | 0.1.0 | none | - |
| sy_deadline_passed | int sy_deadline_passed(const sy_deadline_t *d); | Rust Instant::elapsed 比较 | 0/1 | impl | impl | impl | exists | - | pre | 0.1.0 | none | - |
| sy_deadline_min | sy_deadline_t sy_deadline_min(const sy_deadline_t *a, const sy_deadline_t *b); | Rust Ord::min | none | impl | impl | impl | exists | 嵌套期限取较早者 | pre | 0.1.0 | none | - |
| sy_sleep_until | int sy_sleep_until(const sy_deadline_t *d); | Rust(tokio) sleep_until 的同步版 | -errno;-EINVAL | impl | impl | impl | exists | 被信号打断后续睡到点;无期限记 -EINVAL | pre | 0.1.0 | none | - |
| sy_timer_next | int sy_timer_next(const sy_deadline_t *ds, int n, long *wait_ms); | QuickJS os.setTimeout 的轮询版; Node timers | -EINVAL;ret:needlen | impl | impl | impl | exists | 定时器即一组 deadline;没有事件循环(OUT),调用者把 wait_ms 交给 sy_poll;返回最早一项的下标,全无期限返回 -1 | pre | 0.1.0 | none | - |
| sy_tm_t | typedef struct { int year; int mon; int day; int hour; int min; int sec; int wday; int yday; } sy_tm_t; | C struct tm; JS Date 的 UTC 取值 | none | impl | impl | impl | exists | mon 1..12,day 1..31,只表 UTC | pre | 0.1.0 | none | - |
| sy_utc_split | int sy_utc_split(int64_t sec, sy_tm_t *out); | C gmtime_r; JS Date.getUTC* | -ERANGE | impl | impl | impl | exists | 纯算术,不调宿主;年份不在 1..9999 | pre | 0.1.0 | none | - |
| sy_utc_join | int sy_utc_join(const sy_tm_t *t, int64_t *sec); | C timegm; JS Date.UTC | -EINVAL | impl | impl | impl | exists | 字段越界 | pre | 0.1.0 | none | - |
| sy_utc_fmt | int sy_utc_fmt(char *out, int cap, int64_t sec); | JS Date.toISOString; Rust(humantime) format_rfc3339 | -ERANGE;ret:needlen | impl | impl | impl | exists | 固定 YYYY-MM-DDTHH:MM:SSZ | pre | 0.1.0 | none | - |
| sy_utc_parse | int sy_utc_parse(const char *s, long n, int64_t *sec); | JS Date.parse; Rust(humantime) parse_rfc3339 | -EINVAL | impl | impl | impl | exists | 只收 sy_utc_fmt 的形状,另收纯日期 YYYY-MM-DD | pre | 0.1.0 | none | - |
| sy_local_fmt | int sy_local_fmt(char *out, int cap, const char *fmt, long sec); | C strftime + localtime | -errno | impl | impl | unverified | exists | 本地时区;新代码优先 UTC | legacy | 0.1.0 | none | - |
