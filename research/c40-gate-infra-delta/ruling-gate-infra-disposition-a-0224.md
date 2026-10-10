# 代裁 ≈02:24 SGT — 真审 PASS → 处置 (a) 窄授刷 exec 审核戳（机房主任）

## 事实
- tip 已 FF：**a8a114a0 → 1157e9af**（仅 README + Paper A research；**exec 无动**）。
- Latest **v0.0.39**；无 Draft v0.0.40（陈旧 Draft v0.0.31 忽略）。
- K5-1c 产品锁仍 **96272cd2**。
- 真审回执已齐：`/tmp/cc40-prep/next/gate-infra-delta-true-review-cdx.md`（sha256 **6c0be030…14934**）；§5 第1–6项全勾；第7项未勾；末行 PASS可进处置。
- cdx2 已独核六项边界、未见越权。
- 复算（inv.py 同算法）：tip **1157e9af** 上 exec stamp = **2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27**（与 a8a114a0 全等）；记录值仍 9ae3c358… → DIFF；其余 5 目录 EQUAL。

## 代裁：选处置 **(a)**（宿主/卫生；真审已覆盖三文件 delta）
1. **授权刷戳（窄）**：只改 `tests/gatedeps.json` → `families.compilercheck.reviewed_trees.exec`
   - 旧：`9ae3c35897c7185c7b2ef2c11f6985d5281aafb424fe57b9668c2eabd3d34036`
   - 新：`2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27`
   - 绑定 tip：**1157e9af**（exec 内容与真审基准 a8a114a0 相同）。
2. **落仓外回执**：`/tmp/cc40-prep/next/gate-infra-stamp-update-receipt.md`（写明旧/新戳、成员数 1410、真审 sha、tip、未跑门）。
3. **可本地 commit**（单文件 gatedeps；信息须引用真审 6c0be030 + 本 ruling）。**可 push main**（若工作流惯例如此）或开短 PR——二选一，勿夹带其它文件。

## 明确不授
- 不宣称 gate-infra 已绿 / 精确闭包已恢复（tools/fixture/K2b 仍须各自满足；正式白名单仍空）。
- 不跑共享写者门；不私有复现第2步（除非另授）。
- 不改 inventory 归因/验收措辞（交政委）；不 bump `version.h`；不开 Draft v0.0.40；不删测；不改 queuecheck 期望。
- 不选 (b)(c)；不改其它 reviewed_trees 键。

## 分工
- **cc**：持刀执行 (a)——cwd 必须 `~/repos/unisacc-cc`；先 `python3 /tmp/cc40-prep/gi1/inv.py /tmp/cc40-prep/gi1/inv-prewrite.json` 确认 tip stamp=2d52b9bc… → 只改 exec 键 → 回执 → commit（±push）→ 停等核。
- **cdx**：只读核：gatedeps 单键 diff、新戳=inv 全等、无夹带、回执完备；短评 mux cc；停。
- **cdx2**：只读盯墙钟/越权（有无改 inventory/跑门/bump）+ 刷戳后是否仍有人宣称门绿；短记可续；停。
