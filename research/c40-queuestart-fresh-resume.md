# 0.0.40 误 restore 防护：fresh/resume 状态机（夹具首窗）

**状态（2026-10-10，机房主任授权管道补齐）**：机制入仓；夹具自检绿；非真实 release queue 首观察。不改产品验收、不删测、不公开、不开 K5。

## 行为

| 模式 | 触发 | 行为 |
|---|---|---|
| `QUEUE_START=fresh`（默认） | `queue.sh` 启动 | 不读 `QUEUE_BACKUP`；Q 缺失则空开；Q 已存在则续用 live 目录。若备份在，打印 `left untouched (not restored)`。写 `Q/start-receipt.txt`（mode=fresh, restored=no） |
| `QUEUE_START=resume` | 显式 | 要求 `$B/unisacc-next.com.build.json` 的 `artifact_sha256` == 候选；`$B/state` 或 `state.old` 含 `results.json`；Q 已有 `results.json` 则拒混用。拷贝后写回执（restored=yes, source=…） |
| 其他 | — | rc 2 |

## 负例（tests/queuestartcheck.sh）

fresh+同产物备份→空 Q、备份保留、无 import；resume 正路径导入+回执；错 artifact / 无 results.json / 无 build.json / live 混用 / 未知模式 → rc2。`queue.sh` 不再含旧 `restored state from` 自动路径。

## 边界

- 声明级/夹具级启用 ≠ 真实 B.5 级 queue 首观察；下次依赖净开的正式 queue 应用默认 fresh，续跑须显式 resume。
- resume 不免 gatequeue 指纹失效检查；不删历史备份账。
- 不碰 A7、远端 hook/CI、tag、Draft。
