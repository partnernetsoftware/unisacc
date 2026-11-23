# pidfd 方案:回收前取 pidfd 快照,已回收根的孤儿也能证实地清理(设计稿,2026-11-23)

状态:设计稿,不进 0.0.96。0.0.96 的合同是"已回收根只报告、不杀"(见 v0.0.96 分支 plans/proc-lifecycle.md 第六节)。
本方案是让"已回收后能杀"在不误杀旁人的前提下成立的唯一路径。

## 1 为什么 pid 加 starttime 不够

已回收根之后,后代只能靠 (pid, starttime) 事后识别:读 starttime 与 kill(2) 之间 pid 仍可被复用(窗口),
组号 pgid 在根被回收后可被无关进程复用(gatekeeper a1)。三轮整改都撞在这里。pidfd 是对"那一个进程"的句柄,
进程退出后 pidfd_send_signal 返回 -ESRCH,不会落到复用同一 pid 的旁人身上。

## 2 要点

(1) 回收前,对根的所有后代(父链加根为组长时的组成员)逐个 pidfd_open,取完后复核该进程的 starttime
    仍等于枚举时读到的值,不符就关掉不用(枚举与 pidfd_open 之间的复用窗口,靠复核关闭)。
(2) 根被回收后,只经 pidfd_send_signal(fd, SIGKILL) 发信号;经 waitid(P_PIDFD, fd, ..., WEXITED|WNOHANG|WNOWAIT)
    判断是否已退出(不回收)。不再经 kill(pid)、kill(-pgid)。
(3) 根自行退出的形态要求调用方先设 subreaper(sy_child_subreaper,必须在 fork 之前):孤儿因此成为自己的子进程,
    可证;没有 subreaper 时被 init 收养的孤儿快照不到,答负值(沿用 0.0.96 合同)。
(4) 快照之后新 fork 的后代只有 subreaper 兜得住:取快照越晚越好(np_run 在根退出未回收时取)。
(5) 三态答案不变:0 = 快照内全部确认退出;正数 = 仍存活数;负数 = 不可证(含 pidfd 不可用、fd 超限、-ENOSPC)。

## 3 L0 平台层封装(上层不写 __syscallN)

cx/sys 新增(落在 syscall 同层或 proc 单元,经 sys.sysnums 取号,与 sy_syscall6 同一机制):

| 函数 | 作用 | Linux 号 x86_64 / arm64 | 其余目标 |
|---|---|---|---|
| sy_pidfd_open(pid) | 取 pidfd,失败 -errno | 434 / 434 | -ENOSYS |
| sy_pidfd_send_signal(fd, sig) | 经 pidfd 发信号 | 424 / 424 | -ENOSYS |
| sy_pidfd_wait(fd, nohang, int *exited) | waitid(P_PIDFD),WNOWAIT | waitid 247 / 95,idtype 3 | -ENOSYS |

号进 exec/facts 的 sysnums 事实表(syp_pidfd_open、syp_pidfd_send_signal、syp_waitid),经生成器进 sys.sysnums,
不在 .cx 里写字面号(tools/syscall-literal.cx 会拦)。osx/win 四个目标无 pidfd,走表返回 -ENOSYS,
消费者答"未证实"(负值),不是干净。内核要求 pidfd_open 5.3、pidfd_send_signal 5.1、waitid(P_PIDFD) 5.4。

## 4 消费侧

- ext/sysx/reaper.cx:rp_snapshot 取 pidfd(每个成员一个 fd,上限 RP_SNAP_MAX,超限返回 -ENOSPC 不截断,
  已取的 fd 全部关闭);rp_snap_t 增 fd 字段;新 rp_snap_release() 关 fd(np_run 的每条返回路径都要调用,泄漏即 fd 耗尽)。
  rp_kill_tree_snap(reaped=1):逐成员 pidfd_send_signal(SIGKILL),再按 waitid 轮询确认,全部确认才答 0。
- ext/sysx/processcore.cx:np_run 在 np_exited_nowait 时取快照,沿用 0.0.96 的取点。
- 仓内先例:tools/cxlib/chainrunproc.cx 已按"每个后代持 pidfd 加 starttime"做身份(cr_add:__syscall6(434,...),
  cr_signal:__syscall6(424,...))。这是 tools 内直调,迁移时改用 L0 封装,并删掉这两处字面号。
- tools/cxlib/e2stream.cx:175 与 tools/affected.cx:676、682 的"负值后裸 kill"回退,tools/bg-candidate.cx:444-445
  的回收后 kill(-pid):见 v0.0.96 复核记录,本方案落地时一并改为只经句柄或快照。

## 5 沙箱 seccomp

ext/sysx/linuxns.cx:102、372:沙箱 denylist 含 pidfd_open 与 pidfd_getfd(被拒);pidfd_send_signal 不在表中,默认放行。
结论:沙箱内进程取不到 pidfd。两条路,须单独决定,不得默认放开:
(a) 沙箱内走"未证实"路径(负值),清理由沙箱外的监督者做;
(b) 沙箱表为清理者单独放行 pidfd_open(但放行后沙箱内进程能对同用户其他进程取 pidfd,攻击面变大)。
建议 (a)。

## 6 风险

- 枚举到 pidfd_open 之间的窗口:靠打开后复核 starttime,仍有残余但只会"漏取",不会"误杀"(取到的 pidfd 指向的进程若
  starttime 不符就丢弃)。
- fd 上限:RLIMIT_NOFILE 与 RP_SNAP_MAX;溢出返回负值。
- 子线程:pidfd 指向线程组组长,不是线程;信号发给整个进程,符合预期。
- 僵尸:pidfd 指向僵尸时 pidfd_send_signal 成功但无效;确认走 waitid WNOWAIT 的 exited 状态,不走存活探测。

## 7 验收设想(夹具,全部在 pid namespace 内:unshare -U -r -p -f --mount-proc)

(1) 根自行退出并被回收,setsid 逃逸孤儿(subreaper 已设):按 pidfd 杀掉,答 0。
(2) a1:根 pid 被无关兄弟复用并在组 R 里放旁人 Y:Y 不被停、不被杀。
(3) 成员 pid 在取 pidfd 与发信号之间被复用:旧 pidfd 发信号得 -ESRCH,新进程不受影响(用 pid 命名空间内
    ns_last_pid 强制复用)。
(4) 超过 RP_SNAP_MAX:-ENOSPC,已取 fd 全关(核对 /proc/self/fd 数量回到基线)。
(5) 沙箱内 pidfd_open 被拒:答负值,不发任何信号。
(6) 无 pidfd 目标(osx/win):表驱动 -ENOSYS,消费者报 cleanup unconfirmed。
(7) 变异:发信号前去掉 pidfd(回退 kill(pid))必须红;去掉 starttime 复核必须红;rp_snap_release 漏调用必须红(fd 数)。

## 8 不做

- 不在 0.0.96 做。不改 0.0.96 已声明的"已回收后只报告"合同,本方案落地后才可把它改成"已回收后经 pidfd 杀并确认"。
