# 0.1.0 第 7 条"能用清单 30 过 25"草案(ccogweco,只读整理)

依据: prd.md 第 2 节能力速查(2.1 日常/2.2 测试/2.3 发布/2.4 生成器/2.5 诊断)与第 9.4 节 E4 行(30 过 25,中期线;清单须在 107 公开前冻结);R37 第 7 条要求先定类别与每类配额再填项,已知红不得剔除,每项绑定一个现有批次。

## 1 类别与配额(先定)

| 类别 | 配额 | 说明 |
|---|---|---|
| 2.1 日常(运行、改写、查询) | 8 | 用户日常入口:启动计时、脚本计时、示例、真实输入、own-flow、网依赖、模块单元 |
| 2.2 测试(棘轮、门禁、批次) | 8 | 提交门与棘轮、诊断合同、覆盖、单元边界、限制一致性 |
| 2.3 发布(资格、会话、安装) | 6 | 发布执行、候选链、安装、端到端链、旧产品资产、tlib |
| 2.4 生成器(事实表->生成器->表) | 4 | 表生成、派生、接口清单、接口哈希 |
| 2.5 诊断(度量、盘点、普查) | 4 | 诊断普查、批次计时、TSV lint、st_gap |
合计 30。

## 2 清单(每项绑定现有批次;状态=当前是否可计入"过")

| # | 类 | 能力 | 绑定批次 | 层级 | 状态 | 备注 |
|---|---|---|---|---|---|---|
| 1 | 2.1 | 启动计时 | run-startup-bench | commit | 就绪 | IMPLEMENTED |
| 2 | 2.1 | 脚本七相计时 | script-bench | commit | 就绪 | IMPLEMENTED |
| 3 | 2.1 | 根下 C99 示例冒烟 | examples-smoke | commit | 就绪 | IMPLEMENTED |
| 4 | 2.1 | 真实输入逐项实跑 | real-input-check | commit | 就绪 | IMPLEMENTED |
| 5 | 2.1 | own-flow 释放后使用原型 | own-flow | commit | 就绪 | IMPLEMENTED |
| 6 | 2.1 | 全树 own-flow 误拒率门 | own-flow-tree | commit | 就绪 | IMPLEMENTED |
| 7 | 2.1 | 网级依赖图与第 14 号波及门 | net-deps | daily | 就绪 | IMPLEMENTED |
| 8 | 2.1 | 模块单元展开与边界 | module-units | daily | 就绪(由 107 车关闭) | 已在第二车拆片:提交 a7b2dd86c5(module-units 三片 + count-v2,13 文件;tests/module/forward.cx 等新增),v0.0.107 分支已含;冷态三片 7.46/6.87/7.12s |
| 9 | 2.2 | 十三棘轮一次跑 | ratchets-tool | daily | 缺 | KILLED 46.19s/峰 53.12s;修复见 WP4 |
| 10 | 2.2 | 提交门并发 flow 与串行对拍 | precommit-flow | commit | 就绪 | IMPLEMENTED |
| 11 | 2.2 | 诊断合同夹具 | diag-contract | commit | 就绪 | IMPLEMENTED(0.0.108 剩余件另计) |
| 12 | 2.2 | 返回值约定棘轮 | retcheck | commit | 就绪 | 本日落地 |
| 13 | 2.2 | cx 分层检查 | cx-layers | daily | 就绪 | IMPLEMENTED |
| 14 | 2.2 | funit 过程槽语料 17 份 | funit-procslot-corpus | commit | 就绪 | IMPLEMENTED |
| 15 | 2.2 | 单元数 1..129 -o 边界 | unit-boundary | daily | 就绪 | 与 CXM_MAXU 补丁对账 |
| 16 | 2.2 | 限制一致性(表与代码) | check-limits | commit | 就绪 | IMPLEMENTED |
| 17 | 2.3 | 发布执行五段 | release-exec | daily | 就绪 | IMPLEMENTED |
| 18 | 2.3 | 候选链预演 | candidate-chain | commit | 就绪 | IMPLEMENTED |
| 19 | 2.3 | 安装 APE | install-ape | daily | 缺 | KILLED 46.42s/峰 55.61s;修复见 WP4 |
| 20 | 2.3 | 发布链端到端 | release-chain | commit | 缺(保留为已知缺口) | KILLED 68(余量页);按 cc 决定不临时加拆件、不换项,登记为 0.1.x 余量缺口;U30(其 --dry 声称)由 R38 在 107 发布时用真实五步原行关闭 |
| 21 | 2.3 | 旧产品资产登记与下载核 | old-product-assets | commit | 就绪 | IMPLEMENTED |
| 22 | 2.3 | tlib 客户端合同 | tlib | commit | 就绪 | IMPLEMENTED |
| 23 | 2.4 | 登记表生成 | tables-gen | commit | 就绪 | IMPLEMENTED |
| 24 | 2.4 | 派生表总闸 | derive | commit | 就绪 | IMPLEMENTED |
| 25 | 2.4 | 包接口清单 | layer-api | commit | 缺 | 冷 53.117s 到限/热 24.7s;分片在 agyneon G35 |
| 26 | 2.4 | 接口哈希表 | iface-hash | commit | 就绪 | IMPLEMENTED |
| 27 | 2.5 | 前端诊断普查 | diag-census | commit | 就绪 | IMPLEMENTED |
| 28 | 2.5 | 批次计时度量 | batch-timing | commit | 缺 | KILLED 55.453s;修复见 WP4 |
| 29 | 2.5 | TSV lint 实库扫描 | tsv-lint | daily | 缺 | KILLED 55.83s;修复见 WP4 |
| 30 | 2.5 | st_gap 真值表生成 | st-gap-gen | commit | 就绪 | IMPLEMENTED |

## 3 现状与缺

- 当前按"批次存在且无已知超时红"计:就绪 23 项,缺 7 项(第 8、9、19、20、25、28、29)。低于 30 过 25,差 2 项。
- 7 个缺项的关闭补丁与 0.0.107 追加集归属(cc 2026-11-24 决定:只收 WP4 全部、两个生产者(booter-raw-elf/old-checker)、选择器复用、提交门 list-cap;WP1/WP3/WP2/WP6/WP7/WP8 不进 107):

| 缺项 | 关闭补丁 | 在 107 追加集? | 说明 |
|---|---|---|---|
| #8 module-units | 已在 107 车:提交 a7b2dd86c5(module-units 三片 + count-v2,13 文件,含 r35-module-units-ac48 版补丁) | 是(已并入第二车树) | 已覆盖;证据:git show --stat a7b2dd86c5、v0.0.107 树含 tests/module/forward.cx 与三份 module-units manifest |
| #9 ratchets-tool | WP4:拆 plan/run 两片 | 是(WP4 全部) | 覆盖 |
| #19 install-ape | WP4:basic 与 split2 两片 | 是(WP4 全部) | 覆盖 |
| #20 release-chain | 无拆件(按 cc 决定不临时加) | 否(保留缺口) | 登记为已知缺口:批次 KILLED 68 秒进 0.1.x 余量缺口;U30 由 R38 在 107 发布时用真实五步原行关闭;第 20 项不换、记为缺 |
| #25 layer-api | WP4:接 `{layer-api-tool}/{iface-hash-tool}` 材料并拆 real/fixture | 是(WP4 全部) | 覆盖(agyneon G35 分片为前置) |
| #28 batch-timing | WP4:抽 `{batch-timing-tool}` 材料 | 是(WP4 全部) | 覆盖 |
| #29 tsv-lint | WP4:拆夹具与实库扫描,预编 tsv-lint | 是(WP4 全部) | 覆盖 |

- 结论:#8 已由第二车 a7b2dd86c5 覆盖(就绪 23+6=29);#9、#19、#25、#28、#29 由 107 追加集的 WP4 关闭;#20 release-chain 按 cc 决定保留为已知缺口(不在 107 加拆件、不换项),其 --dry 声称 U30 由 R38 在 107 发布时用真实五步原行关闭。按此口径,封版时至少 29 项就绪,过 25 有 4 项余量;唯一缺项是 release-chain 本身。
- 清单本身还缺三样才能冻结: (1) 由 agy-h 把每项映射到它要读的 strict.tsv 批次行并核对断言与 oracle;(2) 明确"过"的机械口径(该批次在 0.1.0 strict.tsv 中 status=PASS 且 rc0,FAIL/UNKNOWN/缺失都不算);(3) fable 对类别配额与 30 项选择过一眼。
- 已知红不得剔除: 上表 7 个"缺"项全部保留,修好后再计;不把红项换成容易过的项来凑 25。
- 未选但也在能力速查里的候选(可替换,但替换须在冻结前由 fable 过): examples-cx-lab(无批次)、cx-namespace、loader-special、pp-differential、c-negative、symindex(无批次)、dup-census(无批次)、phase-times(无批次)。
