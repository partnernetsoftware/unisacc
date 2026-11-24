# U33 调用点审读:sy_proc_kill / sy_kill 的"回收后再杀"(只读)

缺陷形态(cx/sys/proc.cx:423-442): sy_proc_kill 在组信号失败后回落到裸 pid;子进程已被回收时该 pid 可能被复用;syp_reap 回收后不清 pid,故任何"先 wait 成功、再 kill"的调用都是 U33 风险。

## 发布链路径:发现一处,升级 P0

- tools/chain-step.cx:486-487(发布链驱动,chain-run/candidate-chain 的每一步都经它):
      if (sy_wait_status(pid, 0, &ex, &code, &sig) < 0 || sy_now_ms() > timeout_deadline_ms || (ex ? code : 128 + sig)) {
          sy_kill(pid);
  第 486 行先 wait 回收;只要该 rev-parse 子进程超时或非零退出(最常见:git 命令失败),第 487 行仍对已回收 pid 发 `kill(-pid, SIGKILL)` 与 `kill(pid, SIGKILL)`(cx/sys/proc.cx:633-638 的 sy_kill 定义),正是"回收后再杀";发布链上必现的失败分支,按 cc 口径升级 P0。
- tools/release-*.cx、tools/chain-*.cx 其余文件、tools/prerun-drive.cx: 0 处 sy_proc_kill/sy_kill 命中(git grep)。

## 其余调用点(无"先回收再杀"顺序)

| 位置 | 顺序 | 判定 |
|---|---|---|
| tools/dvqualify.cx:431 | try_wait==0(仍活)才 kill,再 wait | 安全 |
| tools/dvreact.cx:155 | 条件触发先 kill 再 wait | 安全(未先 wait) |
| tools/cxlib/flowexec.cx:495 fl_kill_wait | kill 后 try_wait 轮询;仅在节点 RUNNING 时调用 | 安全 |
| tools/cxlib/flowfile.cx:1462 | r==0(仍活)才 kill;r<0 只标 UNDEC | 安全 |
| ext/sysx/exec.cx:85,92 | TERM 后 grace 内 try_wait;若 try_wait 返回 <0(未知,含 -ECHILD)仍走到 KILL | 窄风险(非回收成功路径) |
| ext/sysx/run.cx:185 | timed 为 -1(try_wait<0 或内存错)也调 px_kill | 窄风险(同上) |
| tools/devsvc-core.cx:1062-1067 | try_wait==1 直接 return;try_wait<0 break 后仍 sy_proc_kill(SIGKILL) | 疑点成立但形态是"回收状态未知即杀",不是"回收成功后杀";非发布链 |
| tools/precommit-flow.cx:52 | 收尾按 FL_RUNNING 节点 kill | 安全(状态位守) |

## 结论

发布链上确有一处 P0 形态(chain-step.cx:487),已按 cc 指示立即单独报;其余 14 处没有"wait 成功后再 kill"的路径,devsvc-core/exec/run 三处是"wait 返回错误后仍杀"的窄风险,建议随 U33/0.0.96 的 sy_child_t 状态位一起收口。
