# 0.0.107 第二轮严格预跑收官摘要与 0.0.106 对照

记录窗口: `agy-h` (仅收官对账与指标对照, 不在补跑期间动用重命令)
候选与源提交: `REV=63354b4a2cb707ecc923a89f0df6030149fa697f` (`F4`, `APE2=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63`, `8991230B`, `SID2=37ff46cd02171c8ad2d3ba04dd084c296f5290a992077f8935e8b853e67d3238`); `B-t` 表修提交 `309e2e1ab8` + `a63bc7bece` (重跑 `W0213..W0240`)
账本与证据入口: [prerun-rate-107-r2.tsv](prerun-rate-107-r2.tsv); [chain-to-public-r2.tsv](chain-to-public-r2.tsv); [stop-event-r2.md](stop-event-r2.md); [prerun-watch-v3-note.md](prerun-watch-v3-note.md); `~/.cache/unisacc/prerun/v0.0.107-63354b4a2c/supervise.log`

---

## 1. 第二轮预跑收官实测事实

1. 关键时间节点 (`+0800`):
   - 起链 `T1`: `05:08:30` (`epoch=1795554510`)
   - 链止 `T2`: `05:34:55` (`epoch=1795556095`, 链耗时 `26.4m` / `1585s`, `rc=0`, `499/499`)
   - 预跑起跑 `T_prerun2`: `05:39:02` (`epoch=1795556342`, 转段 `4.1m`, `241` 窗, `W0000..W0021` 置顶)
   - `W0213` 产品红停 (`attempt=1`): `06:40:59` (`epoch=1795560059`, `done=212/241`, `check-daily-compile-smoke FAIL rc=1`)
   - 外层自动重试 `attempt=2`: `06:40:59..06:43:26` (`epoch=1795560206`, 非人工 `--resume`, 跑过 `W0000 PASS 10607ms` 与 `W0015 rc=124 -> KG`, 再停 `W0213` 并自然退出, `done=214/241`)
   - 人工 `B-t --resume` (`resume-r1`): `06:48:28..06:59:54` (`epoch=1795560508..1795561194`, `PGID=3661284`, `W0213..W0240` 共 `28` 窗全推完, `done.W0240` 与 `prerun-daemon.out` 写毕于 `06:59:49`, `supervise.log` 合并完成于 `06:59:54`)
   - 收官 `T_done`: `06:59:54` (`epoch=1795561194`, 预跑总跨度 `80.9` 分钟 / `4852s`; 距 `T1` `111.4` 分钟, 距 `T2` `85.0` 分钟; `PGID=3661284` 已退净)
2. 终态 `PRERUN_RUN` 原行 (`supervise.log` 末行):
   `PRERUN_RUN	status=DONE	ran=28	skipped=213	out=~/.cache/unisacc/prerun/v0.0.107-63354b4a2c	window_wall_ms=687642	sum_batch_ms=2525038	outer_overhead_ms=-1837396`
   - 窗口落盘计数: `done.W0000..done.W0240` 共 `241` 个主窗完成标记全部落地, 另含 `done.W0220.z06` 串行恢复子窗共计 `242` 个 `done` 文件。
   - 关键新批与上轮坑位实测: `W0020` (`release-exec-g` `PASS 7`), `W0021` (`8/8 PASS`, 含 `c99-array-param-typedef` / `fwd-cap` `28` 断言 / `fwdstub-replay` `25` 断言), `W0023..W0090` 全部 `68` 个 `netregen` 重窗一次全 `PASS`, `W0123` (`cx-booter-ape` `PASS 46700ms assertions=10`), `W0213` (`B-t` 后 `8/8 PASS`, `check-daily-compile-smoke PASS 5073ms assertions=8`)。
   - 待串行补跑/非主停项 (`PRERUN_KEEP_GOING=5`): `W0016` (`ratchets-tool-full` `FAIL rc=1`), `W0017` (`tsv-lint-root` `UNKNOWN 55577ms`), `W0093` (`slow-split` `FAIL rc=0 55556ms`), `W0015` (`precommit-list-cap` `UNKNOWN 55488ms`), `W0231` (`retcheck`, `repository-governance FAIL rc=1 2910ms`); 历史 `PRERUN_INCOMPLETE` `2` 行中 `W0000` 已在 `attempt=2` 转 `PASS`、`W0015` 已转 `KEEP_GOING`; 另附记 `prerun-watch v2` 对 `W0220` (`PRERUN_RECOVER` 走 `W0220.z06`) 未映射回 `W0220` 计数, 故报 `done=240/241 state=INCOMPLETE` (`supervise.log` 已显式输出 `PRERUN_RUN status=DONE ran=28 skipped=213`)。

---

## 2. 第二轮预跑对 0.0.106 的量化对照

| 指标 | `0.0.106` 预跑实测 (`prerun-rate.tsv:11`) | `0.0.107` 第二轮预跑实测 (`prerun-rate-107-r2.tsv:14`) | 对比变化与原因 |
|---|---|---|---|
| 总窗口数 | `238` 窗 (`1+1+68+131+37`) | `241` 窗 (`1+1+68+133+38`) | `+3` 窗 (`107` 新增批合入) |
| 预跑总耗时 (`T_prerun -> DONE`) | `122.6` 分钟 (`00:08:15` DONE) | `80.9` 分钟 (`06:59:54` DONE) | 缩短 `41.7` 分钟 (`-34.0%`, 含 `B-t` 停等) |
| 全程平均吞吐 (`rate_cum`) | `1.94` 窗/分 | `2.98` 窗/分 | `+53.6%` (`68` 个 `netregen` 全绿 + `long` 递减排序) |
| `chain_start -> prerun_done` | `157.2` 分钟 | `111.4` 分钟 | 缩短 `45.8` 分钟 |
| `chain_stop -> prerun_done` | `129.3` 分钟 | `85.0` 分钟 | 缩短 `44.3` 分钟 |
| 停机与续跑次数 | `4` 次停机 (`W0080` 超窗等及多段续跑/切换 `--keep-going`) | `1` 次产品红停 (`W0213`) + `1` 次外层自动重试 (`attempt=2`) + `1` 次人工 `--resume` (`resume-r1`) | 消除了重窗与环境类连环 `UNKNOWN 3` |
| 收官 `keep-going` / 异常窗 | `FAIL=6` (`W0087`/`W0148`/`W0150`/`W0189`/`W0191`/`W0206`), `UNKNOWN=4` (`W0014`/`W0015`) | `KEEP_GOING=5` (`W0016`/`W0017`/`W0093`/`W0015`/`W0231`), `netregen 68/68` 与 `cx-booter-ape` 全一次 `PASS` | `UNISACC_BOOTER_CHAIN_OUT` 与 `old-product 0700` 修复完全生效 |

---

## 3. 准入第 6 条双硬线如实核对

1. `T2 -> X <= 45m` (`T2=05:34:55`, 硬线 `06:19:55`): 已于 `06:20:23` 实测越过, [chain-to-public-r2.tsv](chain-to-public-r2.tsv) 如实标为 `OVER_45M` (未满足)。
2. `T1 -> T4 <= 120m` (`T1=05:08:30`, 硬线 `07:08:30`): 已于 `07:11:56` 补跑阶段实测越过(仍在 `Step 1 X` 之前), 按准入第 `6` 条在 [chain-to-public-r2.tsv](chain-to-public-r2.tsv) 如实标为 `OVER_120M` (未满足), 后续公开发布说明须如实载明两条时间硬线未满足。

---

## 4. 预跑后串行与低负载热态补跑终态核对 (`07:00..07:11`)

1. `W0000` (`module-units-forward`, `strict-W0000.tsv`): `PASS rc=0`, `8520ms`, `load1=5.85`, `assertions=7` (`SUMMARY PASS=1 FAIL=0`)。
2. `W9916` (`ratchets-tool-full`, `strict-W9916-ratchets-tool-full.tsv`): `PASS rc=0`, `42680ms`, `load1=3.72`, `assertions=4` (`SUMMARY PASS=1 FAIL=0`)。
3. `W9993b` (`slow-split`, `strict-W9993b-slow-split.tsv`): `PASS rc=0`, `51144ms`, `load1=5.30`, `assertions=5` (`SUMMARY PASS=1 FAIL=0`; 初跑 `W9993` 曾贴限 `FAIL rc=0 55449ms`)。
4. `W9931` (`retcheck`, `strict-W9931-retcheck.tsv`, `source=5d04a80a476328d8a0b29f63df52531e3c827ace`): `PASS rc=0`, `3862ms`, `load1=2.45`, `assertions=15` (`SUMMARY PASS=1 FAIL=0`)。
5. `W9915b/c/d` (`precommit-list-cap`, 低负载热态四连测): 均呈 `UNKNOWN rc=0 transport=-1` (`note=transport-failed-raw-neg1`, `headers=tree`, `assertions=UNMEASURED`): 首跑 `W9915` `55557ms` (`load1=7.49`)、低负载同布局 `W9915b` `55530ms` (`load1=1.82`)、`W9915c` `55486ms` (`load1=1.93`)、`W9915d` `55492ms` (`load1=2.21`)。与有外部材料缓存命中的 `tsv-lint-root` 不同, `precommit-list-cap` 在 `batches.tsv:2031` 无外部材料且 `headers=tree`, `check-precommit-list-cap.cx:47` 每次新建临时仓并运行 `4` 次 `precommit.cx`, 稳定触 `run-batch.cx:264` 的 `53s` 子进程限。
6. `W9917c` (`tsv-lint-root`, `strict-W9917c-tsv-lint-root.tsv`, `source=5d04a80a476328d8a0b29f63df52531e3c827ace`): 同布局缓存命中后转 `PASS rc=0`, `11285ms`, `load1=1.94`, `assertions=2` (`SUMMARY PASS=1 FAIL=0 total_ms=12899`; 此前 `W9917b` 因新布局首次冷构造工具超时记 `UNKNOWN rc=3 57633ms`)。

小结与 `M1` 合并收口(`release/evidence/0.0.107/strict.tsv`, `07:19`): `6` 批补跑中 `5` 批实测 `PASS rc=0` (`W0000`/`W9916`/`W9917c`/`W9931`/`W9993b`), 全集 `517` 批仅余 `precommit-list-cap` `1` 批呈设计性贴限 `UNKNOWN`。`M1` 合并/归档/脱敏(`EVIDENCE_HOME_PATHS=0`)实测汇总: `PRERUN_MERGE windows=252 batches=517 PASS=516 FAIL=0 UNKNOWN=1 INVALID=0 SUPERSEDED=4`, `SUMMARY batches=517 PASS=516 FAIL=0 UNKNOWN=1 INVALID=0 total_ms=52776 layout_ms=104`, 待按 `expected-unmeasured` 登记并如实披露。
