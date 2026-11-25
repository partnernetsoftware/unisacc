# P0-X: 0.1.0 与 0.0.107 r2 预跑 rc=124 及 >45000ms 贴限窗口全名单

来源证据:
- `0.1.0`: `release/evidence/0.1.0/prerun-supervise.log`、`prerun-windows.tsv`、`strict.tsv`
- `0.0.107 r2`: `release/evidence/0.0.107/prerun-supervise.log`、`prerun-windows.tsv`、`strict.tsv`

关键核对结论:
1. 0.1.0 `W0014` (`attempt=1 rc=124 58067ms -> attempt=2 PASS sum_batch_ms=50763ms`) 对应批为 `slow-split`, 已经在 `p0-prep-window-plan.md` 既定 P0 四项拆批之内, 不是未知新批。
2. 两车对拍发现一个既有四项之外的 F 节(乙)类跨车贴限候选 `notcovered`: `0.0.107 r2` `W0091` 实测 `attempt=1 rc=124 58035ms -> attempt=2 46470ms PASS (>45000ms)`; `0.1.0` `W0094` 首跑即 `54268ms PASS` (距 `55s` 限仅余 `732ms`)。
3. `W0188 devsvc-core-jobs` (`0.1.0`) 与 `W0185 devsvc-core-jobs` (`0.0.107 r2`) 两车均首试 `rc=124 (58034/58032ms)`、次试 `16990/16139ms PASS`, 实证为冷态首试争用而非容量逼近, 按 F 节归(丙)类不拆片。

## 1. 按 F 节三类清单 (全部 rc=124 窗口对齐)

| 分类 | 批名 (`batch_id`) | 0.1.0 实测 (`window`, 尝试与耗时) | 0.0.107 r2 实测 (`window`, 尝试与耗时) | 标记与处置结论 |
|---|---|---|---|---|
| (甲) | `module-units-limits` | `W0000`: `att1 58041ms rc=124`, `att2 58040ms rc=124 -> PRERUN_STOP`; 后移 `W0244 9589ms PASS` | 位于后续正常窗 `PASS` | 两次超时(冷首窗错选); 由 T11 工具冷态选轻批关掉 |
| (甲) | `module-units-forward` | `W0000`: `3110ms` (`5297ms` 含外层) `PASS` | `W0000`: `att1 58044ms rc=124`, `att2 58047ms rc=124 -> INCOMPLETE`; 外层 `8520ms PASS` | 两次超时(107 冷首态; 0.1.0 已过); 同由 T11/预热关掉 |
| (甲) | `precommit-list-cap` | `W0022`: `att1 58037ms rc=124`, `att2 58034ms rc=124 -> STOP`; 单跑 `W9922 UNKNOWN 55587ms` | `W0015`: `58052/58048ms rc=124` 与后续 `55488..55557ms UNKNOWN` | 两次超时; 已在 P0 四项拆批 (`tests/check-precommit-list-cap.cx`) |
| (甲) | `tsv-lint-root` | `W0023.z06`: `att1 58031ms (55580ms) rc=124`, `att2 58032ms (55570ms) rc=124 -> KG UNKNOWN`; `W9923b 11733ms PASS` | `W0017`: `att1 58045ms rc=124`, `att2 55577ms UNKNOWN`; 单跑 `11285ms PASS` | 两次超时; 已在 P0 四项拆批/材料相 (`tools/check-tsv-lint.cx`) |
| (乙) | `slow-split` | `W0014`: `att1 58067ms rc=124 -> att2 50763ms PASS` (`wall=53141ms`) | `W0093`: `att1 58033ms (55538ms) rc=124 -> att2 55556ms FAIL`; 单跑 `51144ms PASS` | 重试收敛且仍贴限; 两车实锤, 已在 P0 四项拆批 (`tools/check-slow-split.cx`) |
| (乙) | `notcovered` | `W0094`: `att1 54268ms PASS` (距 `55s` 仅余 `732ms`) | `W0091`: `att1 58035ms rc=124 -> att2 46470ms PASS (>45000ms)` | 重试收敛且仍贴限(跨车新增候选); 交 `cc` 裁是否按 F(2) 插入 |
| (乙/补) | `check-split-parents` | `W0088`: `att1 58035ms rc=124 -> att2 38955ms FAIL` (缺夹具); B-t 单跑 `W9988 49056ms PASS (>45000ms)` | `W0086`: `att1 55557ms PASS` / `att2 36948ms PASS` (`strict` 取 `36948ms`) | 修复单跑实测 `49056ms` 贴限(距 `53s` 余 `3944ms`); 0.1.0 已由 `cc` 列入拆批范围 |
| (丙) | `premerge-selfcheck-main` | `W0015`: `att1 58067ms rc=124 -> att2 23006ms PASS` | 107 同系列曾录 `50251/49898ms` | 0.1.0 本轮重试收敛且宽裕(`23.0s`), 沿用 107 既定保留在 P0 再拆 |
| (丙) | `devsvc-core-jobs` | `W0188`: `att1 58034ms rc=124 -> att2 16990ms PASS` | `W0185`: `att1 58032ms rc=124 -> att2 16139ms PASS` | 重试收敛且宽裕(两车均首试超时、次试 `16-17s`); 属冷态争用, 不拆片 |
| (丙) | `lane-key-probes` | `W0001`: `att1 49414ms PASS` | `W0001`: `att1 58047ms rc=124 -> att2 20151ms PASS` | 107 重试收敛且宽裕(`20.2s`), 0.1.0 首试 `49.4s` (首 long 窗冷态敏感, 作观察) |
| (丙) | `module-units` | 正常通过 | `W0014`: `att1 58046ms rc=124 -> att2 10372ms PASS` | 重试收敛且宽裕(`10.4s`) |
| (丙) | `check-gen-all-check` | 正常通过 | `W0088`: `att1 58043ms rc=124 -> att2 35813ms PASS` | 重试收敛且宽裕(`35.8s`) |
| (丙) | `check-netregen-expected-fail` | `W0111`: 主跑重试因缺 vseq 夹具 `FAIL 25759ms`; B-t 单跑 `W9989 40738ms PASS` | 正常通过 | 40.7s 余量偏紧(`12.3s` 至 `53s`), 已并入 0.1.1 余量评估 |
| (丙/并发) | `W0023` / `W0243` (`jobs=8`) | `W0023` (`58034ms rc=124` -> `z01..z05 3.0-8.4s` + `z06`); `W0243` (`58034ms rc=124` -> `z09 2.5s`, `z10 11.5s`) | `W0220` (`58018ms rc=124` -> `z06 devsvc-git-c-operations rc=124 -> 10823ms PASS`) | 8 路并发整窗超限后串行恢复且宽裕 |

## 2. 附: 单次首跑即 PASS 但 >45000ms 的 jobs=1 长批提示 (不触发 rc=124)

供 `0.1.1` 余量观察与对比 `0.0.107`:
- `notcovered`: `0.1.0 W0094 = 54268ms` (`107 W0091 = 46470ms att2`, 见上表甲/乙分类)
- `check-register-layout-gate`: `0.1.0 W0096 = 54170ms` (`107 W0094 = 55527ms / 21754ms`)
- `cx-booter-ape`: `0.1.0 W0017 = 53031ms` (`107 W0123 = 46700ms`)
- `check-gen-all-drift`: `0.1.0 W0095 = 52966ms` (`107 W0092 = 53229ms`)
- `chain-ready-freeze`: `0.1.0 W0100 = 51871ms` (`107 W0098 = 45355ms`)
- `lane-key-probes`: `0.1.0 W0001 = 49414ms` (`107 W0001 = rc=124 -> 20151ms`)
- `real-commit`: `0.1.0 W0025 = 49060ms`
- `check-register-layout-env`: `0.1.0 W0105 = 48707ms` (`107 W0103 = 49075ms`)
- `optimization-closure-0-4`: `0.1.0 W0117 = 48175ms` (`107 W0115 = 45815ms`)
- `stream-pipe-parts`: `0.1.0 W0089 = 45356ms` (`107 W0087 = 55408ms`)
- (`check-gen-all-hang`: `107 W0089 = 51161ms`)

## 3. P0-X 裁定收口 (2026-11-25 cc 裁定)

- `notcovered` 并入 P0 贴限余量清单作为第五项(`0.1.0 W0094` 首试 `54268ms` 距 `55s` 仅余 `732ms`, `107 r2 W0091` `rc=124 -> 46470ms PASS` 属重试收敛且仍贴限; 由 `ompgpt` 追加进 `p0-prep-window-plan.md` 执行三次独占量测加拆片或冷态收据);
- `devsvc-core-jobs` (`0.1.0 W0188`, `107 W0185`) 维持(丙)类争用判断, 不拆片。
