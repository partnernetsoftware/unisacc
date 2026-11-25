# ccogweco 与 cdx 的 WP 拆批交叉核对(cdx 页: ~/.local/share/unisacc/research/cdx/wp2-wp4-plan.md)

| 维度 | cdx 页 | 我的 WP6/WP8 页 | 结论 |
|---|---|---|---|
| 批次认领 | WP2 九批(precommit-gates-na/toolsreg-a2/toolsreg-d/reuse-commit/reuse-key、commit-own-reg-a、precommit-toolsloc*三) + WP4 五批(layer-api、batch-timing、tsv-lint、ratchets-tool、install-ape) | WP6 五批(cx-booter-ape、cx-npfault、s8c-bind、wt-gate-clean、dev-product)+ WP8 四批(slice-inventory、flow-tool、skill-cx、signal-targets) | 无一重复认领 |
| 材料键 | precommit-tool、commit-own-tool、batch-timing-tool;WP1/WP2 共用 commit-own-tool,由一个补丁登记生产者 | all-waves-tool、symindex-tool、slice-inventory-tool,均走 NAME-tool 现成规则,不新增生产者 | 键名无冲突;cdx 的 commit-own-tool 生产者补丁与我的补丁互不依赖 |
| 写域重叠 | tests/batches.tsv、tests/coverage.tsv(经 manifest 与登记);其余限 tools/check-precommit-*.cx、check-commit-own.cx、check-layer-api.cx、check-batch-timing.cx、check-tsv-lint.cx、check-ratchets.cx、check-install-ape.cx | tests/batches.tsv、tests/coverage.tsv、tools/check-slice-inventory.cx、tests/check-wt-gate.cx、tools/check-booter-ape.cx、tools/check-npfault.cx、tools/check-s8c-bind.cx、tools/check-dev-product.cx、tools/check-flow.cx、tests/check-skill-cx.cx | 登记表同文件不同行,须串行 --keyed 落库;源码文件无一相同 |
| affected.cx 边界 | 明确预设写域不含 tools/affected.cx,材料复用只经既有 NAME-tool 规则 | WP8 slice-inventory 补丁不改 affected;wt-gate 备选 A 会改 affected(回退链加 tests/NAME.cx) | 备选 A 与 cdx 的写域预设直接冲突,且 affected 应归 WP1 所有者;若走 A 须先由 WP1/cc 统一改一次,不能由本包顺手改 |
| tier/headers 合同 | 保持 tier、headers、预算,拆片按用例标识对账 | 保持 tier、headers(product/tree)、预算 | 无冲突 |
| 在途补丁 | WP1 材料键、S1 提交语义、R28 affected-canary、agyneon layer-api 分片(G35) | 0.0.125 进程组、CXM_MAXU 256(unit-boundary 边界) | 我的 WP8 与这些无交集;WP6 cx-npfault 与 0.0.125 的 proc 语义有交集,已在页内标注 |

结论:无同批重复认领;唯一需裁决的是 wt-gate 备选 A 会动 tools/affected.cx,与 cdx 写域预设和 WP1 所有权冲突,故本包只做不改 affected 的 B 向补丁,备选 A 留待 WP1/cc 统一决定。
