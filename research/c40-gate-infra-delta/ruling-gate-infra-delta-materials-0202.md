# 代裁 ≈02:02 SGT — gate-infra 第1步已齐 → 授权 delta 审材料（机房主任）

## 事实
- tip 仍 **a8a114a0**；Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- K5-1c 产品锁仍 **96272cd2**。
- gate-infra 第1步只读回执已落：`/tmp/cc40-prep/next/gate-infra-step1-readonly-receipt.md`；七套件矩阵+末列更正齐；cdx/cdx2/grk 三方核完，口径一致（充分静态原因=exec 审核戳失配；并存原因未排除；tools/fixture 仍 UNKNOWN）。
- 三窗 Idle，等裁三问。

## 代裁（宿主/卫生；prep 材料，非处置）
1. **授权**：准备 exec 三处改动的 **delta 审材料**（只出材料、不审完、不刷戳、不改仓）：
   - 范围仅：`exec/opt/gen-delta.sh`（新增 ab4f75b8）、`exec/pp/gen-delta.sh`（新增）、`exec/pipeline/prepare.sh`（变更）。
   - 仓外写：`/tmp/cc40-prep/next/gate-infra-delta-review-materials.md`（含：各文件相对 b8ea2faa 的 diff 摘要、进入 compilercheck inventory 的路径依据、对 audited/fallback 的影响引用第1步回执、审核人核对清单勾选项）。
   - 停等批；不选处置 (a)(b)(c)。
2. **明确不授**：
   - 第2步两条私有复现（不为追绿；静态充分原因已立；若后需另授）。
   - inventory 口径改写（属验收/归因措辞 → 交政委；本轮保持「因果未证明/另列待核」不升级）。
   - 处置 (a)(b)(c)；刷审核戳；改 gatedeps；改验收/删测/归因；bump / Draft；共享写者跑门。

## 分工
- **cc**：写 delta 审材料 → 停。勿叠复现/刷戳/改仓。
- **cdx**：只读盯材料是否越界（仅三文件、仓外、无刷戳暗示）。
- **cdx2**：只读核材料完备性（缺 diff？缺影响链？缺审核勾选？）+ 墙钟 Idle 端点 + 第2步是否仍值得另授的一句话建议。
