# Darwin B139（CI bootstrap B=139）材料索引

- 范围：原红 run `38111789528` @ `82e1a9f0`；macos-14/15/intel 三红；linux×2 绿。
- **本刀（1446）**：PC 锚闭合 → 窄修 `tests/bootstrap.sh`（`-t "$HOST_TARGET"`）。裁定 `ruling-b139-pc-fix-1446.md`；取证 `cc-darwin-b139-pc-anchor-1446.md`；m4pro FINDINGS/IPS/分步 `m4pro-*`；cdx `darwin-b139-pc-fix-review-cdx-1446.md`；cdx2 `cdx2-darwin-b139-advice-1446.md`。
- 前序：`ruling-darwin-b139-next-1354.md` / `ruling-darwin-b139-cc-lead-1349.md`。
- tip diag（续盯）：run `38118763767` @ tip（采样时 in_progress；**不含** 1446 修）。
- 不构成：产品 PASS、Draft、bump、seal、放宽验收、以 m4pro 代 CI。
