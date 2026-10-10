# 0.0.38 → 0.0.39 流水线度量（同机、同 queue.sh；cc 实测摘录，2026-10-10）

| 指标 | 0.0.38 final | 0.0.39 B.5 净开 | 来源 |
|---|---|---|---|
| 主机 | 本 Linux x86_64 云机 | 同 | — |
| 套件数 | 650 | 659 | queue.out |
| queue 墙钟 | 5593 s（单段） | 5234 s（16:10:29→17:37:43，单段） | c38-freeze-receipt.md:64；queue.start/queue.out |
| 调度窗口数 | 115 | 104 | release-queue.log `^window` |
| DEFER 次数 | 160 | 170 | release-queue.log `^DEFER` |
| INVALIDATE / restore 复用 | 0 | 0（净开） | 日志 |
| 出口 NEEDS_RULING | 0（具名裁后；原 1） | 1（subtract-safety，补验 rc0） | exittable |
| 作废整轮 | 0 | 2（examples 误删 593 作废；同产物备份自动复活，停） | c39-freeze-receipt.md |
| 发布链人裁次数 | 多次口头 | 台账化 R1–R9 + ②③，均有时间与提交 | release/ruling-requests.tsv |

读法与限定：
- 同机同工具，queue 墙钟 −359 s（−6.4%），而套件数 +9；**不是严格同条件**：源码、套件集与缓存温度不同，单次样本，不宣称因果提速。
- 0.0.39 端到端多出两次作废整轮（约 5 min + 49 s 实耗）与一次 comboot H37 漏前置；这些是本轮新暴露的流程缺口，已修或已登记 0.0.40（备份显式 opt-in、归档路径预检、rc_tag 身份、Draft PATCH 保 tag_name）。
- F4″（m4pro 同机冷编三次中位 ≤5 s）未测，顺延 0.0.40；本表不覆盖产品编译速度。
