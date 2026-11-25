# 变异门超时杀进程组:静态交接(ccogweco -> ompgpt)

基线: v0.0.125 分支头 7341b0b5a226afee5bcc12d4fbace632e8a45fa5(与 origin/main 合过止损点)。
补丁(旧一轮 + 本轮静态): research/ccogweco/mutation-gate-static-1-3-6.patch,对上面基线整份 git apply --check 可过;它含 tools/mutation-gate.cx 与 tools/check-mutation-gate-timeout.cx 两文件。
旧一轮证据: research/ccogweco/mutation-gate-timeout-evidence.md(G38 已判该轮夹具假绿,只作症状参考)。

## 已做的静态改(仅本地,未提交)

(1) dash 写法:本机 /bin/sh 是 dash。实测 `kill -0 -PGID` 与 `kill -TERM -PGID` 可用;`kill -0 -- -PGID` 报 "kill: Illegal number: -"。补丁里 mg_group_alive 与组 TERM/KILL 已全部去掉 `--`;watch:kill -s 0 -PGID 在 dash 下行为未采用。
(3) 身份核对:新增 mg_proc_start(pid)(读 /proc/PID/stat,comm 之后第 20 个数 starttime)、mg_pid_write/mg_pid_read(格式 "pid start")、mg_same_proc;run_all 收尾先核身份再发信号,不符只打印 MG_STALE slot=.. pgid=.. pid-reused-or-gone 且不发信号。worker 组 starttime 另写 worker-i.start 边车。
(6) 文件头注释改为与实际一致:检查由 mg_run_check 经 sy_spawn 起,各自成组,pid 记进 OUT/cur-slot.pid,收尾按该组 kill 并核实。
旧一轮仍有效的机制:mg_run_check 以 sy_spawn 起检查、写 pid、超时用 sy_kill_tree 回 124;收尾 mg_stop_group 做 TERM->等待(50x100ms)->KILL->等待->kill -0 -PGID 核实,残留 MG_RESIDUE 拒判。
编译: `sh ./unisacc.com -c -funit tools/mutation-gate.cx` rc0。冻结期未跑门。

## G38 未做(留给重做)

(2) 孙进程: sy_spawn 起的检查自成进程组(非仅组,需确认是否 setpgid/setsid);cx_runv_timeout 起的子进程同样自成组,worker 死后这些孙进程既不在 worker 组也不在检查组。计划按会话回收(扫 /proc 会话号==worker 会话)与/或按槽 cwd 回收,setsid 孙进程要用 cwd 兜底。
(4) 读管道: 现实现读满 cap-1 就停并关管道,检查继续写会 SIGPIPE rc141 被误判已杀;须读满后继续读到 EOF(丢弃)或明确丢弃但保持读端,另补旧实现有、新实现没有的读后时间上限。
(5) 夹具: deadline 30 太长(单跑 42-47s 违反余量),改 --deadline-s 8,MG_RUN 阈值改 >= deadline/2(4s),加"检查里 setsid 起孙进程"用例。
(7) 变异: 核实步骤(kill -0)要有断言守着;删核实现在不转红。
另: 本轮夹具的 group 断言在旧轮是假绿(核查实永答已空),重做时先做活组探测自证:造一个活的 sleep 组,探测必须报存活。

## 我这边遗留

v0.0.125 工作树里两文件为未提交改动;研究目录另有一份 pgid 清理留痕 research/ccogweco/pgid-cleanup-2026-11-24.md。开工前建议你在私有 archive 上应用补丁重跑,不要用我的工作树。
