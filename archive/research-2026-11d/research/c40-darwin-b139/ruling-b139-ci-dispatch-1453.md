# 代裁：B139 CI dispatch + acceptance 同形钉（机房主任 1453）

墙钟：2026-10-11 14:55 CST。授源：政委「根因已修 tip ed50f00c；卡点 ci.yml 仅 workflow_dispatch / 403；立刻 dispatch + acceptance 10-* 同源」。续 1446。

## 裁定
1. **CI dispatch（已执行）**：box `gh`（账号 `mgttt`，repo admin + `workflow` scope）对 `partnernetsoftware/unisacc` 执行 `gh workflow run ci.yml --ref main`。**未 403**。
2. **新 run**：`38119555397` @ `ed50f00caef0b3d9d61c24ab1d17b9c560009e09`（tip / ed50f00c）。URL：https://github.com/partnernetsoftware/unisacc/actions/runs/38119555397  
   启动时五 job：linux×2 in_progress；macos-14/15 queued；macos-15-intel in_progress。
3. **acceptance 同源**：材料 `cc-acceptance-10-bare-c-1453.md`。证据够 → **代裁同形钉已落地** `c2b140bc`：`tests/acceptance.sh` 三处 `-c` + compile `--target` 对齐 `HOST_TARGET`（禁 bump/Draft）。
4. **paste 三窗**：cc / cdx / cdx2 盯新 CI `38119555397`（见 paste-*-1453.txt）。
5. **政委一键（备用；本回合未需）**：
   ```bash
   gh workflow run ci.yml -R partnernetsoftware/unisacc --ref main
   # 或钉 SHA 所在 tip：
   gh workflow run ci.yml -R partnernetsoftware/unisacc --ref ed50f00c
   gh run list -R partnernetsoftware/unisacc --workflow=ci.yml --limit 3
   ```
   若 box 403：需 `repo`+`workflow` 的 PAT/`gh auth login`，或在 GitHub UI Actions → ci → Run workflow → branch `main`。

## 身份
- 根因 tip：`ed50f00c`（bootstrap `-t HOST_TARGET`）。
- 验证 CI：`38119555397` @ `ed50f00c`（含 bootstrap 修；**不含** acceptance 钉）。
- acceptance 同形钉 tip：`c2b140bc`（已 push main；分片路径，CI 主路径仍走 bootstrap；未另开 CI）。
- 并列旧 run：`38118763767` @ fc2c016b（diag，无本修）；`38111789528` @ 82e1a9f0（原红）。

## 硬禁（不破）
禁 bump / Draft / 公开 / Latest / seal / 放宽验收 / 以 U 代 B / 把 m4pro 或未结 CI 写成产品 PASS。

## 卡点 / 下一刀
- 盯 `38119555397` 三 mac bootstrap 出口；绿≠产品 seal，只验 B139 修。
- acceptance **已修** @ `c2b140bc`；Darwin 本地 `ACCEPT_CASE=10-*` 可再验；CI 主路径仍走 `bootstrap.sh`（本验证 run 不含该 tip）。
- 本回合 dispatch **无 403**；PAT 备用未动。
