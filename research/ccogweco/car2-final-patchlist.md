# 0.0.107 最终入车与顺延补丁清单(核对 git log;2026-11-25 ccogweco)

底稿:research/agyneon/notes-107-patchlist.md;对拍 git log(车树 v0.0.107,HEAD 48f9e054ad,基线 2e8a19a342)。
结论:agyneon 页与 git log 一致;十三份 = 追加集十二份 + tools.cxlib bump 一笔(候选槽 1a1ccc5167 单列)。

## 1 最终入车十三份(git log 实际提交)

| # | 提交 | 件 | 补丁 sha / 证据 | 对公开说明一句 |
|---|---|---|---|---|
| 1 | `5db7d34071` | #1 precommit list-cap v7 | 8b9e7eec… | 提交门大输出动态扩容并 fail-closed |
| 2 | `37f0aef541` | #2 chain-step 回收后不再杀 | rebased 890b717b…(原 1271d5c8) | 发布链失败分支不再按已回收 pid 发信号 |
| 3 | `f0f07a315f` | #3 dvqualify selector reuse v3 + 0755 修订 | 787bba…(修订版) | 选择器按内容复用,并修 sy_append_file 模式 |
| 4 | `5b4f77f33e` | #4 r39c release-exec sha 缓冲 | 2e9e41e4… | 解除 106 W0150 的长路径 200 字节截断 |
| 5 | `1d3e80a4c2` | #5 producer cx-booter-ape | cdx 生产者补丁 | booter-raw-elf 有生产者,UNKNOWN 可转实跑 |
| 6 | `b32a48e979` | #6 producer tools-default-regression | 4a3aa9ca… | old-checker 有生产者,UNKNOWN 可转实跑 |
| 7 | `586db8dbcf` | #7 seal-precheck v2 | 9900d706… | 七条封版机械判据落地 |
| 8 | `04eb6d0d74` | #9 wp4 install-ape v4 | rebased cac28640…(原 2e4a07d6) | install-ape 缺 APE fail-closed 与拆片 |
| 9 | `5df7c35286` | #10 wp4 material v2 | 8958b5bf… | batch-timing/tsv-lint/ratchets-tool 材料化 |
| 10 | `62cf5bc4e6` | #11 u08 source-missing v3 | 32affee7… | 缺源具名拒绝负控 |
| 11 | `1122ace564` | #12 gen-all-drift fixture | 3fdca37e… | 漂移夹具负控,防假绿 |
| 12 | `48f9e054ad` | #14 readback prewarm gate v2 | 3e…(on-0107 微变基) | 公开前读回预热硬门,防 106 型回滚 |
| 13 | `e2f0539e2a` | tools.cxlib bump 0.4.11->0.4.12 | LAYER_API_SUMMARY pkgs=1 bad=0 | 覆盖 #3 函数体改动的接口号 |

单列:`1a1ccc5167` 候选槽 0.0.107 单行(不计入十三份)。

## 2 未入车(本次明确不带)

| 件 | 依据页 | 说明 |
|---|---|---|
| #8 v4 check-tool-material-macro | research/ompgpt/check-tool-material-macro-v4.patch(3394b077,余 36.8 秒) | 需先消 affected.cx:1203 与 devsvc-core.cx:538 字面 |
| #13 r39e runcache-units sh | research/cdx/r39e-runcache-units-sh.patch | 缺显式 shell 启动差异夹具 |
| #13 宿主遮蔽三批 | research/cdx/car2-13-status.md、ompgpt cc-shadow-scan | chain-precheck/source-bom/realprog 支持显式 CC,补丁待写 |
| #15 release body 写入回读 | research/ompgpt/release-body-fix-for-108.md | 顺延 0.0.108/0.1.x |

## 3 顺延(0.1.x)

| 件 | 依据 | 影响说明一句 |
|---|---|---|
| #16 CI run_attempt 固定 | research/cdx/car2-16-ci-run-attempt.md | 说明仍按 run id 记录;重派时保留两次 run |
| #17 tlib bump 硬阻断 | research/cdx/car2-17-tlib-bump-gate.md | 本版未把 TLIB_BUMP_NEEDED 升为拒绝 |
| prerun-watch v2 登记 | research/agy-h/car2-prerun-watch.md | 值守工具未入车,107 用私有脚本 |
| 预跑提速 S1-S4 与结构点 | research/ccogweco/prerun-speedup-0.1.x.md | 本版预跑按现调度,不承诺提速 |

## 4 与 agyneon 页的差异(均为重基/修订,不是事实冲突)

- #2 由 1271d5c8 重基为 890b717b;#9 由 2e4a07d6 重基为 cac28640;#3 用修订版 787bba…并带 0755 修复;
  #14 用 on-0107 微变基 3e…。agyneon 页均注明 rebased/fix,git log 与之一致。
- agyneon 页 #14 行记 "--part g 七绿一红";该"红"是负控(删判定后应红),不是失败。
- 候选槽 1a1ccc5167 出现在 log 中 #12 之前,顺序与 agyneon 页 (4) 记法一致。

## 5 公开说明影响汇总

- 产品改动节:十三份按上表 #1-#13 各一句;未入车的 #8/#13/#15 不写"已修",顺延 #16/#17/prerun-watch/提速
  写入已知缺口或后续版本,不冒充通过。
- 链后无追加改动:本车十三份都在 F 之前,无 post-chain-merge 节(以实际 HEAD 为准)。
