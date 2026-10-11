# 默认授权页：同 SHA flaky / 诊断类（机房主任 1413 · Idle 砍）

墙钟：2026-10-11 14:14 SGT。目的：符合下列**全部**条件时，执行者**默认已授**，不必再等逐步裁断；回执写本页路径即可。

## A. 同 SHA flaky 探针（默认授 · 每 SHA 每 run **一次**）

**已默认授权**当且仅当：
1. 目标为**已失败**的 GitHub Actions job；入口仅 `gh run rerun --failed <run_id>`（或同等「仅失败格」）。
2. `head_sha` **不变**（同 SHA）；不新开改源 push；不重跑已 success 的 linux 格。
3. 本 run 上同 SHA **尚未**用过本类默认授权（每 SHA×run **一次**；二次须另裁）。
4. 原红日志/结论**并列保全**；禁止覆盖/撤销 attempt1 原件。
5. 解读约束写进回执：一次绿 ≠ 根因闭合 ≠ 六格 PASS；仍红同形 = 复现支持非纯偶发。

**仍须另裁**：改源、bump、Draft、放宽、换 runner 镜像、新 workflow_dispatch 产品矩阵、m4pro、以 U 代 B、把探针绿写入验收。

## B. 诊断副本采证（默认授 · 窄文件域）

**已默认授权**写/跑诊断采证材料，当且仅当：
1. 目的是补 **core / PC / 分步 rc / bs_* 字节 / 签名信息** 等缺口；**不**改变产品判定、不放宽门禁、不把诊断绿写入 Draft 前置。
2. 文件域限于：
   - 仓外：`/tmp/unisacc-cdx2/workflow-efficiency/**` 与 `observe-10m/*diag*`；
   - 仓内（可进 PR/主仓窄改）：`.github/workflows/ci.yml` **仅**增加 failure/诊断路径的 copy + `upload-artifact`（或 `workflow_dispatch` 诊断 job）；以及 **env 门控**的 `tests/bootstrap.sh`（或诊断副本脚本）在 EXIT 前向 `$UNISACC_DIAG_OUT` copy——**默认关闭**，仅诊断 run / `UNISACC_DIAG_OUT` 非空时生效。
3. artifact：仅诊断包；建议保留 ≤5 天、单 job ≤50MB；不含密钥。
4. 触发：优先挂在**已在飞/已失败**的同 SHA 诊断路径；新 `workflow_dispatch` 诊断 run 本页**默认授一次**（钉同一 head_sha）；再触发须另裁。

**仍须另裁**：具名产品修域（backend/签名策略/串行代并行当已修）、关签名、扩大矩阵、任何 bump/Draft/seal/公开。

## C. Idle / 等裁砍法
- 落入 A/B 的动作：直接执行 + 写回执引用本页；**勿 Idle 等新裁定**。
- 任一条件不清或越界：停，写一行请示，不自扩。
- 本页不废首红停、不废独立复核、不把 Idle≤5min 解释成超时自动放行。

## D. 回执最小字段
`auth=1413-default`；`class=flaky|diag`；`run_id`；`head_sha`；`attempt`；`files_touched`；`artifact_yes_no`；`禁项自检=OK`。
