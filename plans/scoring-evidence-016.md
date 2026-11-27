# 0.1.6 段记分证据

基线 skills/unisacc-orch-supervise/team-scoreboard.md 未改. 只读. 未跑链. 正式链在跑, 本表不新算分.

| 窗口 | 提交 | 缺陷或复核 | 事后结果 |
|---|---|---|---|
| agy-flow item 6 | a01396be7ee660d970b0adaf03a12e8c1cac8a3d | 卫生门. 141/4 后因 dcmd 4096 缓冲在长 TMPDIR 下写不出 def-ab-repo, 经 142/3 到 145/0. 根因写在 .work/report-v016-item6-hygiene-gates.md:49-53 | 该报告写 SHARD_RESULT part=core passed=145 failed=0. 正式链本段不重跑 |
| agy-flow gate19 | 着陆报告指名 b87930f83. 后续报告 .work/report-v016-gate19-followups.md 基点 26ce8ea88, 前 15 行未再印提交号 | cdx review-gate19-followups-r2.md 只留安全子集. 6-affected 的 -z 路径三项列为 v0.1.7 | 安全子集未在本表重跑. UNMEASURED |
| agyneon item 7 | 816396585 tools: pin ident-helper | 报告 .work/report-v016-item7-ident-pin.md 写正控 PREPARED, 负控 pin-write-failed rc=2. 全量 check-chain-ready 该报告写待补跑 | 轻量验证不是正式门. UNMEASURED |
| agyneon 复核 | review-item1-closure-d29bf33d7.md 在 .work, 本轮未读裁决正文 | 文件在 | 裁决未读, 不记 PASS |
| agyneon3 item 1 | d29bf33d7 上发现缺陷, 修复提交 7485fb55d | cdx 文件 review-v016-sidecar-public-assets.md 与 r2 在盘. 本轮未重读裁决. 闭合报告写 G8b 在 d29bf33d7 抓到 step_readback 漏写三条 sidecar 的 READBACK_SHA | 闭合报告写修后 CHECK_RELEASE_EXEC PASS part=g failed=0. 远端上传 UNCOVERED |
| agyneon2 item 4 | a1f3b8d004ecbff0115450a81e9ae45ab3d77ba6. phase 4a 报告写未提交共享检出 | U1b/U3c sim --cov 在 .work/report-v016-item4-sim-cov-closure.md. phase 4a 执行报告写旧根 23/2, 新根 25/0 | 慢车道 canary UNMEASURED |
| agy-h 手册 | 50b83f92b 初稿, 957a49766 按 cdx 修订 | cdx review-v016-runbook-numbering.md 对 50b83f92b 裁决 BLOCK. 本轮 git log 只见这两笔, 第三轮未在该文件 log 里 | 手册为零运行. UNMEASURED |
| ccogweco notes | 29b2862590e3288a74521e78705020a2308197af plans/notes-0.1.6-final.patch | 按 cdx review-v016-release-docs.md 把开发计数移出产品段, 选定九资产顶层文件名 | 报告写 notes 检查 pass=5 fail=0. 九资产上传 UNMEASURED |
| cdx | 本段复核无新产品提交号 | review-v016-release-docs.md BLOCK; sidecar 两轮文件在盘; 手册 BLOCK; 门 19 第二轮 | 复核本身不是运行通过 |
| omp-grk | 2b64eecc33eb9fb95c84bb6abed95c47bd9db493 | decision 第 1、2 步改为 chain-promote.txt; rollback 去掉 isLatest, 绑 INDEX.tsv 与 0.1.5 根备份 | rollback 检查 pass=23 fail=0. 未进共享 main |

基线均分不改.
