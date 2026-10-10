# 0.0.40 等裁空转裁剪：begin 分列 / 已裁不再阻塞 / 无动作硬停（夹具首窗）

**状态（2026-10-10，机房主任授权管道补齐；与 cdx2 040-gate-and-plan-review-2014 对齐）**：机制入仓；夹具自检绿；非真实 Stop-hook 首观察。不改产品验收、不删测、不公开、不开 K5。

## 行为

| 能力 | 触发 | 行为 |
|---|---|---|
| `begun_at`（第 9 列，可选） | ledger 行 | request→decision→applied→begin 分列报墙钟；缺/UNKNOWN 不编秒；begin 早于 applied 或非 APPLIED 带 begin → rc1 |
| `--blockers FILE` | 当前范围声称阻塞 id | 任一 id 在 ledger 为 APPLIED → rc2 `STALE_BLOCK`（已裁/已消费不得继续阻当前范围）；PENDING/DECIDED 可留；未知 id 亦 STALE |
| `--prev-pending FILE` | 监视循环 | pending 集合不变且无更新 applied/begun → rc3 `IDLE_NO_PROGRESS`（无动作不虚记进展） |

8 列旧台账仍解析。负例见 `tests/rulingwaitcheck.py`（含 R9/F4 已 APPLIED 仍列阻塞、纯 pending 空转）。

## 边界

- 夹具级启用 ≠ 真实 Stop-hook / agent 等裁循环首观察；接线 Stop-hook 消费另刀。
- 不自行裁、不改裁定正文；只提供可测拒收。
- 不碰 A7、远端 hook/CI、tag、Draft、产品/K5。
