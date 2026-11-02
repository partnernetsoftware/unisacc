# 0.0.40 WF2 填尾（机房主任 22:11 a3=C、22:18 ④：未覆盖/顺延）

**结论：未覆盖 / 顺延。这是顺延，不是通过。非产品验收放行。** 不追绿，不开 a3（方案 A、B 都不跑）。不改产品、不改验收通例、不改归因、不加白名单、不改仓代码（本刀只落研究证据与 plans 指针）。

## 仓内证据（本轮落仓；字节对拷自 `/tmp/cc40-prep/wf2/`）

| 文件 | sha256 |
|---|---|
| `research/wf2/disposition-a3C.md` | `e271cad85e5db1d4be162514523895f67805435c709e15b7c12ffb4ef0bb4be2` |
| `research/wf2/receipt-attempt2.md` | `b466a8a055aaaacd0cc95b16cecf085aba08fe3825b0e125a11c9c9edec714b0` |
| `research/wf2/receipt-attempt1.md` | `5f0865cf2f507fd207d9c349549381f5fc6281dbe6233a2a6c54945e520006fa` |
| `research/wf2/plan.md` | `b17d821a6bd8856ba02c1b2c5246861149dc2d1d8804145d092d9030504381d3` |
| `research/wf2/predict-a3.md` | `f532276c339ed8f6b99604915741a25b7b3ec0eb3930cb2022469d1f855e1198` |
| `research/wf2/a2/run.log` | `e4cc6a0a51db0b7e9defe13d50f79c44a0c7b6fa810aa87afd98aa7e73d1bbca` |
| `research/wf2/a2/attemptchain.txt` | `7e18358a26465e2a28ea7be3c8628f77b15b8077f06b99cfbe292c0f6292840c` |
| `research/wf2/a2/release-queue.log` | `9a8573f1acfd68f7923a9f59ca008a461f5bbcd6c6f76cc6085538df008e60c6` |
| `research/wf2/a2/results.json` | `f9a3fe49e9cf45942def8390d014a3ae358ed6172eeb247b9fc69600507e5f93` |
| `research/wf2/a2/shared-backup-before.sha` | `73e71ffb7e2246a39e11d76fb5d98155988bdee3d36d15ed54654957db8bec4c` |
| `research/wf2/a2/shared-backup-after.sha` | `73e71ffb7e2246a39e11d76fb5d98155988bdee3d36d15ed54654957db8bec4c` |
| `research/wf2/attempt1/run.log` | `b4f03b08f290312e1673c4ad2357541d7e510355fd42d7be41078d9cffa1df85` |
| `research/wf2/attempt1/release-queue.log` | `e7ff7efb146b752a3c8dd2ceb4ae06b9b4c922496252eee71c0d29633b6bacc6` |

before/after 共享 backup 摘要文件自身 sha256 相同（cmp 一致）→ 窗前窗后共享默认 backup 内容摘要不变。

## 哈希说明（不糊弄）

- disposition 正文曾嵌 attempt2 sha `1c0b1d0e…`：那是 disposition 落笔时（~22:12）的修订；其后 receipt-attempt2 追加了 22:1x/22:3x 更正（mtime 22:22）。**当前仓内 `receipt-attempt2.md` 字节 sha 为上表 `b466a8a0…`**。不以旧嵌套 sha 冒充当前文件，也不回改原件去凑旧哈希。
- attempt1 嵌套 sha `5f0865cf…` 与当前文件一致。

## attempt2（授权单窗；填尾未覆盖）

- wt @3a2d21d5；`QUEUE_WINDOWS=1` fresh；8 套件（已剔嵌套夹具）；外层 monotonic 49.052 s；final rc=65（观察，不是验收）。
- `release-queue.log`：8 次 START 全是 `kind=full`；**tail 0、DEFER 0、142 0**。
- attemptchain --results --strict：final 8，pending/unknown/gap 0。
- 未覆盖（具名）：(1) left < span-3 时 kind=tail START；(2) tail 尝试 rc142 → DEFER → lb=2×elapsed → fullwindow → 下一窗 full 先入的恢复链。

## attempt1（原账不通过，保留）

两窗（未显式 QUEUE_WINDOWS）、四个嵌套夹具红、写了默认 backup；无 tail 准入。见 `receipt-attempt1.md` 与 `attempt1/`。

## 边界

- `predict-a3.md` 只是计划；模拟已作废，不作入窗依据。
- 仓外 `/tmp/cc40-prep/wf2/` 路径**不**作产品验收依据；本表仓内路径 + sha 才是门闩引用。
- 不量化提效；不放宽预算/重试契约。
