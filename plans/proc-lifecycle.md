# 进程生命周期合同(proc-lifecycle)

所有者:ccogweco(进程生命周期唯一所有者,cc 2026-11-22 派工)。写域:`cx/sys/proc.cx` 的
`sy_proc_*` 段、`ext/sysx/reaper.cx`、`tests/manifests/proc-lifecycle.tsv`、
`tools/check-proc-lifecycle.cx`、本文件。cdx 与其他窗口只改消费者,经 devsvc 写锁按文件拿窗口。

此前 Z3(capture)、Z5(REAPED 状态位)、Z6(reaper 回收路)各补一块,互相打架,根因是
"组长何时可以回收"没有唯一答案。本文件给唯一答案。

## 一、九条合同

1. **pgid 归句柄独有**:带组 spawn 的 pgid 在 `sy_child_t` 里(`h`),对外不另存、不缓存副本。
2. **锚定规则**:组清理完成前不回收组长。僵尸组长钉住 pid 与 pgid,这是"清理先于回收"的
   物理保证。提前探测用**不消费**的 `waitid(P_PID, pid, &si, WEXITED|WNOHANG|WNOWAIT)`,
   放 `_os` 静态帮手;不读 `/proc`。Linux 先实跑验证(live 时 si_pid=0,退出后 si_pid=pid,
   两次探测都不消费,之后 waitpid 仍取到 code=7);osx 标 UNMEASURED 并写明风险
   (osx 的 waitid 存在但 WNOWAIT 语义需核;不能确认时探测返回 -1,调用方不再等截止)。
   **可观测性(部分闭合,变异未闭合)**:锚定的存在理由是"组长提前回收后组号被复用,再对
   -pgid 发信号会误伤"。夹具已可观测:`tools/check-proc-lifecycle.cx` 的 `anchor_pidns` 在
   `unshare -U -r -p -f`(user+pid,无需 root,与 cdx 的 check-reaper --reaped-root-pidns
   同法)下让哨兵复用被回收组长的同号并自建同号组,五条断言全 PASS
   (`sentinel-reuses-leader-pid`、`reaped-handle-returns-esrch`、`reaped-handle-kill-esrch`、
   `sentinel-not-signalled`、`positive-control-observes-kill`)。阳性对照证明该夹具能观察
   "哨兵被信号"(`kill(-sentinel,9)` 后 waitpid 得 WIFSIGNALED sig=9),不是盲夹具。
   **未闭合**:变异"REAPED 分支仍对组发信号"经三版验证——标记文件证明分支可达(mark=YES)、
   诊断写 h=19/PID/组号一致、kill 返回 0——但 `anchor.sentinel-not-signalled` 仍 PASS,
   `kill` 成功却未杀死哨兵的机制未明。按"同一失败连续两次即停手"停测,变异闭合登记为
   UNVERIFIED,待独立复核。锚定本身由代码结构(组长只在 sy_proc_end 末尾回收、探测不消费)
   保证。
3. **REAPED 精确含义**:禁止一切以该 pid 或以该 pgid 为目标的信号与等待;
   `sy_proc_try_wait`/`sy_proc_wait` 返回已存状态;`sy_proc_kill` 返回 `-ESRCH`。
   前提是规则 2 保证清理先于回收。**不得**为了清后代而绕开句柄直发 `kill(-pgid)`。
4. **唯一清理原语** `int sy_proc_end(sy_child_t *c, int grace_ms, sy_exit_t *st)`:
   TERM 组 -> 有界宽限 -> KILL 组 -> 有界回收组长 -> 置 REAPED。全程有截止,
   **不依赖管道是否关闭**。`sy_proc_capture`、`sy_proc_wait` 的到期分支、`px_run`、
   `fl_kill_wait` 都调它,不各写一套。
5. **rp_kill_tree 只管逃逸后代**(setsid、二次 fork 离开组者)。入参改为句柄,
   必须在回收前调用,自己不回收根。
6. **subreaper 归进程入口设置**:kill_tree 先自查;未设置时孤儿清扫返回具名拒绝,
   **不得返回 0**。`residue=0` 只在可证实时才允许。
7. **返回三态**:0 = 已证实干净;正数 = 存活数;负数 = 无法证实(-errno 或具名类)。
8. **运行期检查不是编译期所有权**:裸 pid 入口(`sy_signal`、`sy_killpg`、`sy_wait`、
   `sy_kill`)没有保护;非 Linux 的离组后代不受保护;Windows 全部 `-ENOSYS`。
   这一条同时写进 `cx/sys/proc.cx` 与 `ext/sysx/reaper.cx` 的文件头。
9. **本版不做**:pidfd;Windows 实现;发布路径三份循环的迁移(延到 0.0.95 之后)。

## 二、执行顺序(固定)

- (a) 本文件 + `tests/manifests/proc-lifecycle.tsv`(状态 x 操作矩阵)+
  `tools/check-proc-lifecycle.cx` 先落,允许红;变异自证四个:删 REAPED 置位、
  删 KILL 升级、锚定改成提前回收、删 subreaper 自查。
- (b) Z5 修订版:REAPED 恢复"全禁"(信号、等待、组),靠规则 2 的锚定保证清理在前;
  `sy_proc_wait` 超时写 `c->exit.timed_out`。
- (c) Z3 capture 改调 `sy_proc_end`,并修三处 fable 读码推断:
  子进程关 1/2 且忽略 TERM 时到期后无界挂起;非 Linux 无 /proc 时"已退出"探测恒假把正常
  退出报成超时(且 /proc 依赖必须收进 `_os`);`px_run` 在 `rp_kill_tree` 已回收根后读未
  初始化的 `st`(`ext/sysx/run.cx` 约 186-193 行)。每项先红后绿。
- (d) Z6 由 cdx 在 (c) 之后改 reaper 入参(我提供合同,不改他的文件)。

## 三、状态 x 操作矩阵(与 `tests/manifests/proc-lifecycle.tsv` 一致)

状态:LIVE(组长在跑)、ZOMBIE(组长已退出未回收)、REAPED(已回收)。
操作:`kill(TERM)`、`kill(KILL)`、`try_wait`、`wait`、`close`、`end(grace)`、`tree`。

| 状态 | kill(TERM) | kill(KILL) | try_wait | wait | close | end | tree |
|---|---|---|---|---|---|---|---|
| LIVE | 发组与 pid,0 或 -errno | 同左 | 0 运行中 / 1 已回收 | 阻塞至回收 | 只关 fd | 全流程 | 只清逃逸后代 |
| ZOMBIE | 发组,0 或 -ESRCH | 同左 | 1 并置 REAPED | 立即回收 | 只关 fd | 回收并置 REAPED | 只清逃逸后代 |
| REAPED | -ESRCH,零系统调用 | 同左 | 1,返回已存状态,不 waitpid | 0,返回已存状态 | 只关 fd | -ESRCH | 用回收前 starttime |

矩阵里每一格都是 `tools/check-proc-lifecycle.cx` 的一条断言;尚未实现的格在矩阵表里写
`GAP`,检查里写 `FAIL`(先红),实现后改 `PASS`。
