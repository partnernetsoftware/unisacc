# Darwin B139 CI 诊断刀（1413 仓外起稿；cc 定稿）

目的：失败时留下可下载诊断包（分步 rc、bs_B/bs_U、日志、可选 crash 采集），补 attempt1/2 缺 artifact 缺口。

## 硬约束
- 仅诊断；不放宽验收；不改产品判定；默认门控关闭。
- 禁 bump/Draft/公开/m4pro/U代B。
- 机房主任 1413 已默认授权窄改 ci.yml + env 门控 copy（见 `../auth` 引用 `observe-10m/auth-same-sha-flaky-diag-default-1413.md` §B）。

## 交付物
| 文件 | 作用 |
|---|---|
| `bootstrap_diag_copy.sh` | 仓外/可拷入 tests：在 EXIT 前若 `UNISACC_DIAG_OUT` 非空则 copy 关键文件 |
| `ci-yml-diag-patch.md` | 对 `.github/workflows/ci.yml` 的最小补丁说明（upload-artifact） |
| `ci-macos-diag.fragment.yml` | 可粘贴的 workflow 片段 |
| `NEG.md` | 负例：门控关闭时行为与现网一致（无额外副作用） |

cc 定稿后：可 PR/直推主仓窄改（research 回执 + 可选 ci.yml/bootstrap env 门控）。
