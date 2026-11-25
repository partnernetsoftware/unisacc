# 107 起链当天值守轮次手册(02:50 到 04:25;2026-11-25 ccogweco)

依据:research/agy-h/car2-timeline-v2.md 的 106 实测分相(链 27.9 分、预跑 122.6 分/242 窗、正常六格 CI 15.7 分、
读回事故 6.1 分)与 car2-chain-launch / car2-prewarm-list / car2-16-ci-run-attempt。时限一律不放宽:
45 分钟链止到 X 硬线、120 分钟起链到公开硬线、55 秒单步窗口、2 lane 预跑。

## 0 角色

- cc:总值守,每 10 分钟一轮,只看看板三行(进度、预计完成、风险),做升级决定。
- agy-flow:链、预跑、打包、发布执行;持有进程句柄与日志。
- cdx:CI 分诊与 G4 redispatch;夹具/检查红定因。
- agyneon:追加集落车与派生表;只在需要补笔时动。
- agy-h:时间账与速率记录(chain-to-public、prerun-rate)。
- ccogweco:文档与证据页,不改运行状态。

## 1 轮次表(每 10 分钟)

| 时点 | 阶段预期(基准) | cc 看什么 | 阈值与动作 | 谁执行 |
|---|---|---|---|---|
| 02:50 | 起链 | `CHAIN RUN` 首行、`owner.tsv` 时间 | 5 分钟内无首行 -> 停,查 chain-ready/启动件 | agy-flow |
| 03:00 | 链 10/28 | 链日志最后 3 条 DONE;`exit.tsv` 未出 | 单步 >55 秒或 KILL -> 保留原行报 cc,不重试 | agy-flow 报,cc 定 |
| 03:10 | 链 20/28 | fixedpoint 相位是否开始 | 出现 compare 失败/四同破坏 -> 停链报 cc | agy-flow |
| 03:20 | 链止(基准 27.9 分) | `exit.tsv` rc=0 与 `CHAIN fixedpoint` 四同 | 未止:每多 10 分钟记一次;**链止到 X 45 分硬线**从链止起算 | agy-flow |
| 03:30 | 预跑前 10 分钟 | windows.tsv 完成数、supervise.log `PRERUN_LANES lanes=2` | 10 分钟完成 <14 窗(106 速率 2 窗/分 x 70%)-> 2 lane 复核,不加 lane | agy-flow 复核,cc 批 |
| 03:40 | 预跑 | 首个 rc=124 / tool-selfcheck 红 | 任一红:停窗、保留原行、报 cc;不登记预期失败 | cdx 定因 |
| 03:50 | 预跑过半 | 完成窗数对 106 同 phase 比例 | 落后 >20% -> 查 outer_overhead 与 PATH;不改窗数 | agy-flow |
| 04:00 | 预跑收尾 | `PRERUN_RUN status=DONE`、strict 表生成 | 仍有窗在跑 -> 报预计完成,不用旧窗 | agy-flow |
| 04:10 | 打包/X | pack/`CHAIN CMD 0a`、X 提交时刻 | 链止到 X 接近 40 分 -> 准备 45 分硬线说明;不推快 | cc |
| 04:20 | CI 或公开 | CI run 号、`CANDIDATE_QUALIFY` 六格 | CI >18 分:分诊;仅 fetch 类瞬态用 G4 `--redispatch`,不用 rerun --failed | cdx |
| 04:25 | 公开(基准) | INDEX `exec-5-publish`、读回 warm 收据 | 读回非 READY -> 不公开;首次失败按 106 回滚路径,保留两次行 | agy-flow |

## 2 三句话报告模板(cc 每轮发)

    进度:链 <n>/<预计> 或 预跑 <done>/<total> 窗,当前相位 <phase>。
    预计完成:<HH:MM> 公开 / 预跑 / CI(按当前速率外推,注明基准或推断)。
    风险:<无 | 某项接近阈值 | 已触发哪条硬线>,需要 <谁> 在 <几分钟> 内做 <动作>。

## 3 阈值出处(106 实测原行)

- 链 27.9 分:car2-timeline-v2.md 的 chain_stop 行(21:31:05 -> 21:58:56)。
- 预跑 122.6 分、242 窗、完成 00:08:15:car2-timeline-v2.md 的 PRERUN_DONE 行与 prerun-rate.tsv:11。
- 正常六格 CI 15.7 分:car2-timeline-v2.md 的 exec-3-ci 行。
- 读回冷编回滚 6.1 分:car2-timeline-v2.md 的首次 exec-5 与回滚行。
- CI 瞬态 404 与重派 21.1 分:car2-timeline-v2.md 的首轮/二轮派发行;处置用 G4 redispatch,
  见 research/cdx/car2-16-ci-run-attempt.md。

## 4 升级规则

- 任何硬线(45/120 分钟、55 秒、2 lane)接近时,cc 只报"接近"与剩余分钟,不放宽。
- 任何红:先留原行,再定因;同失败连续两次即停手报 cc,不换说法重试。
- 公开时刻以 INDEX 为准,不以窗口口头时间为准。
