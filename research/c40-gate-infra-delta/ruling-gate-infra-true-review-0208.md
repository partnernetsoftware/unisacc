# 代裁 ≈02:08 SGT — delta 材料已齐 → 授权真审勾选（机房主任）

## 事实
- tip 仍 **a8a114a0**；Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- K5-1c 产品锁仍 **96272cd2**。
- delta 审材料已落：`/tmp/cc40-prep/next/gate-infra-delta-review-materials.md` + `gate-infra-delta.diff`（150 行，sha 06761f9b…）。
- cdx / cdx2 / grk 三方材料核完；补正与收窄已追加；七项勾选全空；无人刷戳/选处置/改 inventory。
- 三窗 Idle，等裁三问（谁真审 / 处置 a|b|c / inventory 交政委）。

## 代裁（宿主/卫生；真审≠处置）
1. **授权真审**：由 **cdx** 对材料 §5 清单 **第 1–6 项**做真实内容审核（独立审，作者 cc 不自评）。
   - 依据：材料 + diff + 第1步回执 + tip 上三文件源码只读。
   - 产出仓外：`/tmp/cc40-prep/next/gate-infra-delta-true-review-cdx.md`（逐项勾/不勾 + 一句依据；末行写「PASS 可进处置」或「FAIL 列缺口」）。
   - 可把勾选结果追加记到材料副本末尾；**不改仓内文件**。
2. **明确不授**：
   - §5 第 7 项（重算/更新审核戳）与处置 **(a)(b)(c)** —— 真审回执后再裁。
   - 第2步私有复现；inventory 口径改写（交政委）；刷戳；改 gatedeps；改验收/删测/归因；bump / Draft；共享写者跑门。
3. **inventory**：本轮仍不改；材料里「已识别充分静态原因，并存原因未排除」口径保持。

## 分工
- **cdx**：真审持刀 → 写回执 → 停。勿刷戳、勿选 a/b/c、勿改仓。
- **cc**：Standby。勿叠写材料、勿自评勾选、勿改仓。材料已齐。
- **cdx2**：只读盯真审回执完备性（六项是否都有依据、有无越权暗示第7项/处置）+ Idle 墙钟；短记可续。
