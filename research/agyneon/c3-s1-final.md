# C3 S1 落地小结 (`cx/sys/proc.cx` L0 统一进程所有权表)

- 维护者: `agyneon` | 分支: `v0.1.3`

## 1. 已落地提交

1. `bc3070168fa4900f15f0cf8e7d82369897605e38`: `C3-S1a` — `cx/sys/proc.cx` 引入 64 槽 L0 统一进程所有权表 (`FREE/RESERVED/LIVE/ABANDONED/LOST`)、`(own_slot, own_gen)` 代次绑定、`REAPED` (`c->pid=-3`) 内联缓存、`DETACHED` (`c->pid=-2`)、`STALE` (`c->pid=-1`)、`syp_own_on_reap` 单一更新点、`syp_own_sweep_abandoned`、`atexit` 残留报告及 `tests/core-interface.tsv` / `build/` 派生物。
2. `e90e9337ef32e015efbf94bb1a0d0cf7a504469b`: `C3-S1b-1` — 落地 `tools/check-proc-own-gen.cx` (10 项代次与回绕探针) 与 `tools/proc-own-transitions.tsv` (90 行双轴转移表)。
3. `958e1ba0560e9e0c666a4d9842c54b0662c8a572`: `C3-S1c` — 修复第 67 格 (`LOST_LIVE + sy_proc_close` 句柄转 `DETACHED` 即 `c->pid=-2`), 并新增 9 个测试专用观测与注入接口 (`sy_proc_test_*`, `tests/core-interface.tsv` 标 `test-only`)。

## 2. 验证原行

- `tools/check-proc-own-gen.cx`: `CHECK_PROC_OWN_GEN failed=0` (10/10 PASS, 冷跑 `GEN_COLD_MS=10334`)。
- `verify_s1c.cx` (私有归档树): `PASS row67_detached`、`PASS setup_err_slot_free ev=3`、`PASS hs_echild_lost owner=3076011 lost=1 armed=1`、`PASS drain_no_kill_on_lost`。
- `tools/check-proc-own.cx` (工作区待提交, 21 断言): `OWN_COLD_MS=10974`, `NEW_TOOL qualify=PASS ms=26767`。

## 3. 待办与已知缺口

1. **新工具登记待提交**: `S1b-2` (`tools/check-proc-own.cx`) 与 `S1b-3` (`tools/check-proc-own-sat.cx`) 受 `tests/tools-compile-sources.tsv` 容量 (`1559/1560`) 阻塞, 保留在 `v0.1.3` 工作区等待 `batch-cap-4096` 落 `main` 并合入 `v0.1.3` 后提交。
2. **整表 90 格独立夹具覆盖**: `958e1ba05` 提交前 `cdx` 独立表夹具实测 62 PASS / 1 FAIL (第 67 格, 已于 `958e1ba05` 修复) / 27 UNOBSERVED; 已通知 `cdx` 基于 `958e1ba05` 重跑以确认 90 格全可观测且红格清零。
