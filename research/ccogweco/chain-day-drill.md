# 值守表回放演练(用 0.0.106 实录;2026-11-25 ccogweco)

假设 02:50 起链,按 106 实测时间线回放 chain-day-roster.md 的 10 轮。实录来源:
~/.unisacc-release-evidence-backups/0.0.106/chain/{heartbeat.tsv,exit.tsv,chain-run.tsv}、
~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/supervise.log、research/agy-h/prerun-rate.tsv、car2-timeline-v2.md。

## 0 106 实测偏移(起点 21:31:05)

- 链止 21:58:56(+27.9m);预跑首行 PRERUN_FROZEN_SOURCE 22:05:40(+34.6m);预跑完成 00:08:15(+157.2m,预跑本体 122.6m)。
- 补跑/X 00:50:55(+199.8m);CI 派发 00:53:27;CI 过 01:30:12;首次公开 01:36:31;重公开 01:42:35。

映射到 02:50 起链:链止约 03:18,预跑启动约 03:25,预跑完约 05:27,公开约 06:13-06:32(不含事故)。
这就是演练第一结论:**按 106 原速率,02:50 起链不可能 04:25 公开**;手册里"04:25 基准"必须改成 107 自己的预跑预测。

## 1 10 轮回放(cc 的三句话 + 阈值动作)

| 轮 | 进度(按 106 速率外推) | 预计完成 | 风险与动作 |
|---|---|---|---|
| 02:50 | 链启动;heartbeat 首行 `chain start` | 链止 03:18 | 风险:低;动作:确认 `exit.tsv` 未出、owner.tsv 时间正确 |
| 03:00 | 链 10/28;heartbeat 相位 snap/build-objects 已过 | 链止 03:18 | 阈值:单相位 >55s 或 rc=124 -> 停,报原行;不重试 |
| 03:10 | 链 ~20/28;链内 checkpoint 读 heartbeat 末行与 chain-run.tsv 的 ATTEMPT_TIME | 链止 03:18 | 风险:fixedpoint 未开始则提醒;动作:看 heartbeat 相位名 |
| 03:20 | 链止(03:18 已完成) | 预跑启动 03:25 | 动作:核 `exit.tsv rc=0`、fixedpoint 四同;启动预跑并记第一行 `PRERUN_FROZEN_SOURCE` 时刻 |
| 03:30 | 预跑 0-8 分钟;参考 106 首行 40.9m 才 61 窗;前 10 分钟通常 <15 窗(含 W0000/W0001 冷重跑 rc=124 两次) | 预跑完 05:27(按 122.6m) | **风险:高**;阈值:前 10 分钟 <14 窗或出现 rc=124 -> 2 lane 复核(不加 lane);动作:报"预计公开 06:1x,超 120 线",请 cc 决定压缩或出窗 |
| 03:40 | 预跑 ~15-25 窗;106 在 40.9m 时 61 窗 | 同上 | 阈值:tool-selfcheck 红 -> 停窗留原行;W0080 型 rc=124 连续两次 -> 停手报 cc |
| 03:50 | 预跑 ~40-60 窗;106 同期(75.5m)61 窗、ETA 139-160m | 同上 | 动作:读 prerun-rate.tsv 最新行 `est_start_to_pub_min`;>120 -> 明确写"将超线" |
| 04:00 | 预跑 ~100-140 窗;106 同期 100 窗(107.3m) | 同上 | 阈值:速率 <1.0 窗/分(106 累计 1.38)-> 查 outer_overhead 与 PATH |
| 04:10 | 预跑 ~150-180 窗 | 同上 | 动作:预告链止到 X 45 分硬线从 03:18 起算,若 04:03 未到 X 即已超,需报硬线 |
| 04:20 | 预跑末段或已完;若未完 | 预跑完 05:27 | **结论:04:25 不可能公开**;动作:cc 报"按 106 速率公开 06:13-06:32,超 120 线",按 R37 记未满足或压缩(减少窗/热 kit) |

## 2 手册里模糊或缺失的判据(补丁条目)

1. 链内 checkpoint 怎么读:roster 只说"链进度",未写文件。补:每轮 `tail -1 <chain>/heartbeat.tsv`(epoch+相位)
   与 `grep -c 'ATTEMPT_TIME' <chain>/chain-run.tsv`;`exit.tsv` 只作终态。
2. 预跑开始时刻怎么确定:补 `head -1 <prerun>/supervise.log` 必为 `PRERUN_FROZEN_SOURCE` 行(106 原行),其 epoch 为预跑时钟;
   ids.txt mtime 只作辅助。
3. 预跑速率阈值要分 phase:106 实测 heavy 段 1.49 窗/分,3-lane heavy 降到 0.67;补"前 10 分钟 <14 窗即复核"与
   "累计速率 <1.0 窗/分即查负载"两条,并引用 prerun-rate.tsv 的 `rate_win_per_min`。
4. ETA 用 prerun-rate.tsv 的 `est_start_to_pub_min` 列,不用口径外推;106 首行就给 139-160 分钟,说明 04:25 基准从未成立。
5. CI 派发前最后核对点:补 `gh release view "$TAG" --json isDraft,assets` 判 isDraft=true 且五资产齐,
   再 `gh release download ... -p slice-win-arm64` 核 sha;失败不派,不用 rerun --failed。
6. 读回 warm 收据怎么找:补 `ls -t <STATE>/warm-*.tsv | head -1` 取最新,核 `WARM ... READY key=` 与候选一致;
   106 事故正是无收据直接读回。
7. 硬线起算点要写死:链止到 X 45 分钟从 `exit.tsv` epoch 起算;起链到公开 120 分钟从 `owner.tsv`/`PRERUN_FROZEN_SOURCE` 的
   链起 epoch 起算;两者每轮各报剩余分钟。

## 3 演练结论

- 值守表的时间假设与本车实测不符:04:25 基准只在预跑远快于 106 时成立;演练按 106 速率得 06:13-06:32,
  必然触发 120 分硬线告警。手册应改为"以 prerun-rate 的 ETA 为准,04:25 仅作乐观值"。
- 三句话模板可用:进度用 heartbeat/窗口计数,预计完成用 prerun-rate ETA,风险报硬线剩余分钟。
- 以上 7 条补丁项建议并入 chain-day-roster.md 的下一版;本页不改 roster,只列条目。
