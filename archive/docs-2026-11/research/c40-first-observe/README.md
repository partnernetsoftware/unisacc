# 0.0.40 首观察（范围闭合，非产品 PASS）入仓索引

- 范围：preflight(h1source) → 第一个 make；唯一 make 为 PATH 垫片 exit 97 截停（预期）。
- 闭合裁定：`ruling-first-observe-close-ingest-1159.md`；授权 `ruling-first-observe-auth-1147.md`；补证 `ruling-first-observe-gapfill-1152.md`。
- 原回执：`cc-first-observe-1147-receipt.md`；`cc-first-observe-gapfill-1152-receipt.md`。
- cdx / cdx2 短记：`first-observe-review-cdx-1147.md`、`cdx2-first-observe-watch-1147.md`、`first-observe-gapfill-review-cdx-1152.md`、`cdx2-first-observe-gapfill-watch-1152.md`、`first-observe-ingest-review-cdx-1159.md`、`cdx2-first-observe-ingest-watch-1159.md`。
- 状态样本：`state/`（副本，原件 `/tmp/cc40-first-observe-1147/`）；文件哈希清单 `HASHES.sha256`（`sha256sum -c` 校验）。
- UNKNOWN 具名（不升格）：t_end；1147 ps 原表未保存；identity-after 无 ls-remote；失效键 / seed 内存键 / exittable --h1；真 make（本机无系统 make，垫片替代）。
- 若干声明式 rc0 原证（1159 裁定接受为 UNKNOWN）：UA 与独立 H1 的 rc0 原证缺；archive 前后完整 SHA 缺；运行时身份 / argv / env / 发行版 / outer 身份缺。详见 `cdx2-first-observe-gapfill-watch-1152.md`。
- 不构成：产品 PASS、全量 queue、Draft、公开发布、seal、bump。
