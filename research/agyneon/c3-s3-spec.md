# C3 S3 预备规格第五版 (`ext/sysx/run.cx` 与 `cx/sys/proc.cx` 截止有界化)

- 维护者: `agyneon` | 状态: 规格与事实表第五版 (未改代码, 明确 `kill_tree` 单一 root 收尸权)

## 1. `ext/sysx/run.cx` 修法 (`px_run` 第 119..198 行)

1. **`st` 入口初始化**: 在 `px_run` 入口 `memset(&st, 0, sizeof st);`。
2. **`sy_poll` 错误处理与原 errno 保留 (第 158 行)**:
   - 保存 `int poll_err = 0;`。`k < 0` 时仅 `k == -EINTR` 允许 `continue`, 其它负错误码记录 `poll_err = k; timed = -1; break;`。
   - 即使随后 `sy_proc_end(&c, 200, &st)` 成功收尸, 仍按 `poll_err` 原样返回负错误码 (`px_result_free(r); return poll_err;`), 绝不返回 `0` 或填充 `px_result`。
3. **常规 `read` 与 `try_wait` 负返回错误**:
   - 流读阶段 (`sy_read`) 若 `g == -EINTR` 则重试继续读 (不终止流或判 EOF); 若发生其它负错误码 (`read_err < 0`), 走有界 `sy_proc_end(&c, 200, &st)` + `sy_proc_close(&c)`, 保留原 `read_err` 返回并释放 `r`。
   - `sy_proc_try_wait` 负返回 (`wait_err < 0`, 含 `-EINTR` 等可恢复错或 `-ECHILD`) 同样走 `sy_proc_end(&c, 200, &st)` + `sy_proc_close(&c)` 收尾, 绝不吞错。若此前已进入 `LOST_LIVE`, `sy_proc_end` 返回 `-ESRCH`, `sy_proc_close` 将其转 `LOST_ABANDONED`, 最终返回原 `wait_err` (`-ECHILD`)。
4. **OOM 与超时有界收尾 + 显式移交 (第 184..190 行)**:
   - 取消无界 `while (sy_proc_try_wait(&c, &st) == 0) sy_sleep_ms(2);`。
   - 统一调 `end_rc = sy_proc_end(&c, 200, &st);` 并 `sy_proc_close(&c);`。
   - 注: `sy_child_t c` 为 `px_run` 局部句柄, `px_run` 返回后对调用方不可见, 故表内 `px_run_*` 行 `next_handle_state` 统一记为 `NONE` (内部 `sy_proc_close` 期间已收尸者保持 `REAPED`, 未收尸 `LIVE`/`LOST_LIVE` 转 `DETACHED`)。
   - `end_rc == -ECHILD` (无论 `fatal_poll`、`read_err`、`oom`、`wait_err` 或正常收尾) 一律使槽位转 `LOST_ABANDONED` (`slot_release=no`); 错误优先级: `poll_err < 0` > `read_err < 0` > `oom (-ENOMEM)` > `wait_err < 0` > `end_rc < 0`。
   - 仅当 `poll_err == 0 && read_err == 0 && !oom && wait_err == 0 && end_rc == 0`: 若 `timed > 0` 置 `st.timed_out = 1;`, `px_run` 返回 `0`, `r->timed_out = 1`, `r->code = 124`, `r->sig = st.sig`。
5. **`kill_tree` 单一 root 收尸权、`r->residue` 合同与 `atexit` 残留边界**:
   - **单一 root 收尸权**: S3 改造 `rp_kill_tree` (`ext/sysx/reaper.cx:189/228`) 在 `px_run` 路径下只向后代树发信号并等待/统计后代残留 `r->residue`, **不调用 `sy_wait_status` 收尸 `root_pid`**, 将 `root` 的唯一收尸权保留给持 `sy_child_t` 句柄的 `sy_proc_end(&c, 200, &st)`。
   - **未收 root (设计主路)**:
     - `kill_tree=1`: `px_kill` 调 `sv = rp_kill_tree(c->pid, 2000, 0, 0, rep, sizeof rep)` 有界处理含脱离进程组 (`setsid`) 的后代; 若 `sv < 0` (`-errno`) 回退 `sy_proc_kill(c, SIGKILL)`; 写 `r->residue = sv` (`0` 全清, `>0` 存活残留数, `<0` 为 `-errno`)。
     - `kill_tree=0`: 不调 `rp_kill_tree`, 直接 `sy_proc_kill(c, SIGKILL)`, 按现有合同 `r->residue = -1`。
     - 随后均走有界 `end_rc = sy_proc_end(&c, 200, &st)` + `sy_proc_close(&c)`: 若 `end_rc == 0` 且无更高优先级错误, `px_run` 返回 `0` 并保留 `r->residue`、`r->timed_out=1`、`r->code=124`; 若 `end_rc < 0` 则释放并清零 `r`, 返回 `end_rc`。
   - **已被收 root (冲突/防御路)**: 若 `root` 在 `rp_kill_tree` 期间已被外部/旧逻辑收尸 (槽已 `FREE`、句柄转 `STALE`), 随后 `sy_proc_end` 返回 `-ESRCH`, `px_run` 绝不伪造 `124` 或返回 `0`, 必须释放并清零 `r`、返回 `-ESRCH`。
   - 进程退出时 `atexit` 经 `sy_proc_own_drain(200)` 清理遗留 `ABANDONED` 槽并向 `stderr` 报告未回收残留数 `PROC_RESIDUE n=`。

## 2. `cx/sys/proc.cx` `sy_proc_wait` 截止后有界化与 S1 对齐

1. **单调绝对期限**: `long end_at = sy_now_ms() + 200;` (5ms 轮数 `<= 40` 仅作辅助上限), 循环条件 `sy_now_ms() < end_at`。
2. **`REAPED` 缓存保留 `timed_out` (即使 `st == NULL`)**: `syp_reap` 确认收尸后编码 `c->deadline_at_ms = (long)(unsigned)raw | (timed ? (1L << 32) : 0L);`, 不依赖 `st` 非空。
3. **`sy_proc_close` 对 `LOST_LIVE` 对齐**: 已于 `958e1ba05` 修复 (`c->pid = -2`, `DETACHED`), `syp_own_lost` 仅随已证实同身份回收递减。

## 3. S3 状态转移事实表 (TSV 格式)

```tsv
slot_state	handle_state	event	outcome	next_slot_state	next_handle_state	rc	exit_out	slot_release	note
LIVE	LIVE	px_run_poll	ok	LIVE	LIVE	0	N/A	no	streams-drained-within-deadline
LIVE	LIVE	px_run_poll	eintr	LIVE	LIVE	0	N/A	no	eintr-continues-poll-until-deadline
LIVE	LIVE	px_run_poll	err_fatal_end_reaped	FREE	NONE	poll_err	zeroed	yes	fatal-poll-preserves-orig-errno-local-c-gone
LIVE	LIVE	px_run_poll	err_fatal_end_failed	ABANDONED	NONE	poll_err	zeroed	no	fatal-poll-preserves-orig-errno-transfers-abandoned
LIVE	LIVE	px_run_poll	err_fatal_end_echild	LOST_ABANDONED	NONE	poll_err	zeroed	no	fatal-poll-echild-marks-lost-abandoned
LIVE	LIVE	px_run_read	eintr	LIVE	LIVE	0	N/A	no	read-eintr-retries-without-eof
LIVE	LIVE	px_run_read	err_read_end_reaped	FREE	NONE	read_err	zeroed	yes	read-err-preserves-orig-errno-local-c-gone
LIVE	LIVE	px_run_read	err_read_end_failed	ABANDONED	NONE	read_err	zeroed	no	read-err-transfers-abandoned
LIVE	LIVE	px_run_read	err_read_end_echild	LOST_ABANDONED	NONE	read_err	zeroed	no	read-err-echild-marks-lost-abandoned
LIVE	LIVE	px_run_try_wait	err_wait_recoverable_end_reaped	FREE	NONE	wait_err	zeroed	yes	wait-err-then-end-reaped-preserves-wait-err
LIVE	LIVE	px_run_try_wait	err_wait_recoverable_end_failed	ABANDONED	NONE	wait_err	zeroed	no	wait-err-then-end-failed-transfers-abandoned
LIVE	LIVE	px_run_try_wait	err_wait_recoverable_end_echild	LOST_ABANDONED	NONE	wait_err	zeroed	no	wait-err-then-end-echild-marks-lost-abandoned
LIVE	LIVE	px_run_try_wait	err_wait_echild_end_esrch	LOST_ABANDONED	NONE	-ECHILD	zeroed	no	lost-live-end-returns-esrch-close-transfers-lost-abandoned
LIVE	LIVE	px_run_kill_tree	tree_root_unreaped_zero_residue	FREE	NONE	0	real_timed_out	yes	kill_tree=1-root-unreaped-rp_kill_tree=0-end_rc=0-r.residue=0
LIVE	LIVE	px_run_kill_tree	tree_root_unreaped_pos_residue	FREE	NONE	0	real_timed_out	yes	kill_tree=1-root-unreaped-rp_kill_tree>0-end_rc=0-r.residue>0
LIVE	LIVE	px_run_kill_tree	tree_root_unreaped_rp_err_fallback	FREE	NONE	0	real_timed_out	yes	kill_tree=1-root-unreaped-rp_kill_tree<0-fallback-kill-end_rc=0-r.residue=-errno
LIVE	LIVE	px_run_kill_tree	tree_root_unreaped_end_failed	ABANDONED	NONE	end_rc	zeroed	no	kill_tree=1-root-unreaped-end_rc<0-frees-result-returns-end_rc
LIVE	LIVE	px_run_kill_tree	tree_root_unreaped_end_echild	LOST_ABANDONED	NONE	-ECHILD	zeroed	no	kill_tree=1-root-unreaped-end_rc=-ECHILD-marks-lost-abandoned
LIVE	LIVE	px_run_kill_tree	tree_root_already_reaped_stale	FREE	NONE	-ESRCH	zeroed	no	defensive-if-root-reaped-externally-end-returns-esrch-never-124
LIVE	LIVE	px_run_kill_tree	notree_root_unreaped_reaped	FREE	NONE	0	real_timed_out	yes	kill_tree=0-root-unreaped-sy_proc_kill-end_rc=0-r.residue=-1
LIVE	LIVE	px_run_kill_tree	notree_root_unreaped_end_failed	ABANDONED	NONE	end_rc	zeroed	no	kill_tree=0-root-unreaped-end_rc<0-frees-result-returns-end_rc
LIVE	LIVE	px_run_end	reaped_ok	FREE	NONE	0	real	yes	api-rc-0-r-code-real-exit-local-c-gone
LIVE	LIVE	px_run_end	reaped_timed_out	FREE	NONE	0	real_timed_out	yes	api-rc-0-r-code-124-r-timed_out-1
LIVE	LIVE	px_run_end	err_end_timeout	ABANDONED	NONE	-ETIMEDOUT	zeroed	no	end-timeout-transfers-abandoned
LIVE	LIVE	px_run_end	err_end_kill	ABANDONED	NONE	-EPERM|-errno	zeroed	no	end-kill-fail-transfers-abandoned
LIVE	LIVE	px_run_end	err_end_echild	LOST_ABANDONED	NONE	-ECHILD	zeroed	no	end-echild-marks-lost-abandoned
LIVE	LIVE	px_run_oom	end_reaped	FREE	NONE	-ENOMEM	zeroed	yes	oom-after-bounded-end-frees-result
LIVE	LIVE	px_run_oom	end_failed	ABANDONED	NONE	-ENOMEM	zeroed	no	oom-priority-over-end-err-transfers-abandoned
LIVE	LIVE	px_run_oom	end_echild	LOST_ABANDONED	NONE	-ENOMEM	zeroed	no	oom-echild-marks-lost-abandoned
LIVE	LIVE	sy_proc_wait_deadline	reaped_within_budget	FREE	REAPED	0	real_timed_out	yes	kill-then-bounded-poll-reaped-caches-timed_out-even-if-st-null
LIVE	LIVE	sy_proc_wait_deadline	budget_exhausted	LIVE	LIVE	-ETIMEDOUT	zeroed	no	monotonic-abs-deadline-no-blocking-wait-keeps-live
LIVE	LIVE	sy_proc_wait_deadline	err_kill_esrch_natural_reap	FREE	REAPED	0	real_timed_out	yes	kill-esrch-then-natural-reap-succeeds
LIVE	LIVE	sy_proc_wait_deadline	err_kill_esrch_still_running	LIVE	LIVE	-ETIMEDOUT	zeroed	no	kill-esrch-yet-still-running-exhausts-budget
LIVE	LIVE	sy_proc_wait_deadline	err_kill_esrch_echild	LOST_LIVE	LIVE	-ECHILD	zeroed	no	kill-esrch-then-echild-marks-lost-live
LIVE	LIVE	sy_proc_wait_deadline	err_kill_unreaped	LIVE	LIVE	-EPERM|-errno	zeroed	no	kill-failed-keeps-live-slot
LIVE	LIVE	sy_proc_wait_deadline	err_wait_recoverable	LIVE	LIVE	-EINTR|-errno	zeroed	no	recoverable-wait-err-keeps-live-slot
LIVE	LIVE	sy_proc_wait_deadline	err_wait_echild	LOST_LIVE	LIVE	-ECHILD	zeroed	no	echild-after-kill-marks-lost
```

## 4. 夹具与必红变异 (`tools/check-procrun.cx`)

- 补 `fatal_poll` 注入用例 (`end` 成功仍返回原负 `errno` 且不填 `px_result`)、`read` 可恢复 `EINTR` 与不可恢复 `read_err` 用例、OOM+`end` 失败/`ECHILD` 用例、`st == NULL` 下 `sy_proc_wait` 保留 `timed_out` 用例。
- 必红变异:
  1. `fatal_poll` 后返回 `0`;
  2. `px_run` 忽略 `sy_proc_end` 失败;
  3. `sy_proc_wait` 恢复阻塞 `syp_reap(c, 0, st)`;
  4. 删去 `px_run` 入口 `memset(&st, 0, sizeof st)` 时, 夹具在栈/可控路径预填非零哨兵值 (`st.timed_out=77`, `st.code=99`), 验证异常返回或未写满路径不泄漏哨兵值, 删置零必红;
  5. `rp_kill_tree` 恢复对 `root_pid` 调 `sy_wait_status` 抢收时, `px_run` 返回 `-ESRCH` 而非 `0/124`, 夹具必红。
