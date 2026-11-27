# 本段记分证据

基线: skills/unisacc-orch-supervise/team-scoreboard.md, 未改. 只读. 未跑冷测, 生成器, 批次.
时效列没有源码行, 一律 UNMEASURED.

| 窗口 | 提交 | 复核轮 | cdx 缺陷 | 交付 | 事后运行 |
|---|---|---|---|---|---|
| agyneon | fe0858c1d, 前置 9861a04ce, a21e9af2e; 文档 6ad7509bc, 6d405f4bd | slot12 第二轮报告 .work/report-m1x-slot12-r2.md, 实跑标 UNMEASURED | 本表未读到针对 fe0858c1d 的 cdx 阻断行 | slot12 tapepack 负控与 sy_protect_ro 设计 | 解冻验收计划 post-thaw-verify-slot12-ro.md 写 PLAN_ONLY, UNMEASURED. 任务所述 19/2 后修复: 在 w-agyneon/.work 未找到 19/2. 同窗 notes 写的是 CHECK_TAPEPACK passed=19 failed=0, 不是 19/2 |
| agy-flow | b87930f83, c186529a9, 前置 005f12f58 | cdx review-gate19-followups-r2.md 对 5e4a1ef32 仍阻断; 着陆报告写 c186529a9 仅安全子集闭合 | 生产替代开关已摘, 单源门禁仍会红; 真实列表输出兼容开口 | 门 19 安全子集与 v0.1.7 门 6 补丁, 未合 main | 任务所述 wave 3 125/4 pending: 当前 .work 未找到这句. 仓内 125/4 是 0.0.106 env-semantics 旧证据, 不是本段 wave 3. 本段运行 UNMEASURED |
| cdx | 预跑定因报告写本轮没有代码提交 | 门 19 至少两轮 (review-gate19-followups.md 与 r2) | 自身产出定因, 不是被审窗口 | 三项定因: check-ln-noderef FAIL, cx-booter-ape UNKNOWN, slow-split 超时 FAIL | 报告写修复待验证, UNMEASURED |
| omp-grk | 14f35c2a1, 72eeb90b3, 97cfa8f73, 6e4681076, 4a6cb95a1 | 无 cdx 轮 | 无 | 解冻清单, 缺口草稿, 预跑阅读, 发布后验收, notes 对照 | 文档, 无运行失败 |

基线均分不改: agy-flow 9.1, agyneon 8.9, omp-grk 7.5. 本表不新算分.
