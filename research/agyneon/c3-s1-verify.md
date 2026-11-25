# C3 S1 (L0 进程所有权表) 验证记录

- **分支/工作树**: `v0.1.3` (`~/.claude/worktrees/v0.1.3`)
- **闭合项**:
  1. 单一收尸更新点 `syp_own_on_reap` 统一接入 `syp_reap`/`sy_wait`/`sy_wait_status`/`sy_wait_any`/`sy_wait_rusage`/`sy_proc_own_drain`; 已收尸句柄置 `c->pid = -1`, `try_wait`/`wait` 返回缓存结果, `kill`/`end` 返回 `-ESRCH`。
  2. `syp_proc_spawn_os` 在 `fork()` 成功后立即 `syp_own_commit`; exec 握手失败仅在 `waitpid == pid` 时 `syp_own_on_reap` 释槽, 否则转 `ABANDONED`/`LOST`。
  3. `syp_own_sweep_abandoned` 在 `syp_own_reserve` 入口平时非阻塞清扫; `syp_own_atexit_fn` 终局统计包含 `drain + live + lost`。

## (1) `tools/check-proc-own.cx` 绿态基线 (24/24 PASS)

- 命令: `sh ./unisacc.com tools/check-proc-own.cx`
- 原行: `CHECK_PROC_OWN	failed=0` (`CORE_INTERFACE	ok=1	new=0	missing=0	signature=0	alias=0`)

## (2) 三项必红变异实跑原行

- **变异 A** (任意 wait 错误视为释槽): `CHECK_PROC_OWN	failed=6` (`FAIL I2-I5-wait-err-keeps-slot-zeros-st`, `FAIL I2-kill-eperm-keeps-slot`, `FAIL I6-close-transfers-abandoned`, `FAIL I6-double-close-idempotent`, `FAIL I2-end-timeout-keeps-slot`, `FAIL I2-end-timeout-abandoned-drained`)
- **变异 B** (去掉 fork 前占槽预留): `CHECK_PROC_OWN	failed=3` (`FAIL I1-fill-64-slots`, `FAIL I1-65th-refused-eagain`, `FAIL I1-reap-one-frees-slot`)
- **变异 C** (去掉错误路径 `*st` 置零): `CHECK_PROC_OWN	failed=2` (`FAIL I2-I5-wait-err-keeps-slot-zeros-st`, `FAIL I2-I5-echild-marks-lost-zeros-st`)
