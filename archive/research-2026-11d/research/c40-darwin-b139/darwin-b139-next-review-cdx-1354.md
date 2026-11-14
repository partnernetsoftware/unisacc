# 1354 Darwin B139 下一刀旁核（cdx）

采样：2026-10-11 13:56 SGT。已读终表/artifacts 扫/诊断·探针页/裁定 1354；对照 1349 evidence/hypotheses/policy 与 cdx2-1349 建议。

## 核验
- 五 job 终表：API 与 1349/1350 原证一致 — linux×2 success；14/15/intel **failure**；intel 日志 `B=139 U=0` osx/x86_64 可核（sha256 `b2111630…`）。消除「intel 未结」过时。
- artifacts=[] 与 workflow 无 upload-artifact 一致；缺 core/PC/分步 rc 判定成立 → **不应** 开具名修域请示。
- 探针范围：`rerun --failed` 覆盖三失败 mac、跳过两 linux success；固定 SHA；不 dispatch main — 与 1327/1349 勾选边界及 cdx2-1349 §2 一致。
- 硬禁未破：未见 bump/Draft/放宽/m4pro/改源指令。

## 风险/提醒
- `rerun --failed` 一次会同时重跑三格；仍只算「各一次」窗口，勿再触发第二次。
- attempt2 即使全绿 ≠ 根因闭合；artifacts 仍空则诊断副本仍必需。
- intel 无 acceptance10 红账；三项红（Ctape/compare/bootstrap）结算时勿把 intel 并吞进 arm64 acceptance10 族。

## 结论
旁核：**通过** — 证据不足修域 → 诊断+探针路径正确；代裁一次探针未越界。未改源。
