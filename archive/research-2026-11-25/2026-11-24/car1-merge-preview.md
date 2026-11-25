# 第一车(v0.0.106)合 main 预演与冻结前风险评估(2026-11-24)

采样基准: `.claude/worktrees/v0.0.106` (`HEAD=55d9badfac22dd1adb27ba16ae915cc9e3b77933`, 工作区干净), 对照 `origin/main` (`940bee00a95fbb3f2efdf886fc65c0056d3738c6`), 共同祖先 `merge-base=ce7c2b483e9afac08b873e27483a156deb735c4d`。全程只读核对, 未改工作树或共享检出。

## 1. 车独有提交表(`origin/main..HEAD`, 共 18 笔)

其中含 `9` 笔功能/登记/发布草稿提交与 `9` 笔合入提交。触及产品输入(`cx/`、`src/`、`exec/`、`include/`)共 `3` 笔, 触及冻结包与派生表(`build/tlib-packages.tsv`、`build/layer-api/`、`build/iface-hash.tsv`、`build/links.tsv`)亦集中在以下对应行:

| 序号 | 提交号 | 摘要 | 路径分类 | 是否触及冻结包/派生表 | 产品输入 SID 影响(推断) |
| --- | --- | --- | --- | --- | --- |
| 1 | `55d9badfac` | 起草 0.0.106 集成车发布说明草稿 | `release/notes-0.0.106.md` | 否 | 否(仅文档, 推断) |
| 2 | `f8c0de2157` | v0.0.108 S0 计划与 coverage 说明 | `plans/`、`tests/coverage.tsv` | 否 | 否(仅文档与覆盖表, 推断) |
| 3 | `8311639d17` | S0 表级用例 `check-forward-limit-table` | `tests/`、`build/links.tsv` | 是(`build/links.tsv`) | 否(仅测试与链接派生表, 推断) |
| 4 | `578c617de6` | v0.0.108 S0 `exec/parse2/forward-result.tsv` | `exec/parse2/` (产品输入) | 否 | 会改 SID(`exec/parse2/` 属模型图事实输入, 会改变包内 M4/M34 等型号与产品 SID, 推断) |
| 5 | `848039da19` | Merge branch 'v0.0.122' | 合并提交 | 继承 `v0.0.122` | 同被合并分支 |
| 6 | `4258ae1f83` | Merge branch 'v0.0.124' | 合并提交 | 继承 `v0.0.124` | 同被合并分支 |
| 7 | `2fa8a19d78` | 登记运行缓存修复候选版本槽 | `release/candidate.tsv` | 否(版本槽表) | 否(候选元数据, 推断) |
| 8 | `1fa331d8e6` | 登记转发桩修复候选版本槽 | `release/candidate.tsv` | 否(版本槽表) | 否(候选元数据, 推断) |
| 9 | `4db998a8c0` | Merge branch '3700373f98' | 合并提交 | 无额外变更 | 否 |
| 10 | `ca4e4a4c13` | Merge branch '3700373f98' into v0.0.122 | 合并提交 | 无额外变更 | 否 |
| 11 | `9f9ecda6d6` | 补齐未闭合引号的路径绑定断言 | `tests/check-pinscan.cx` | 否 | 否(仅测试, 推断) |
| 12 | `bc3f4fb868` | Merge branch '3689de54b2' | 合并提交 | 无额外变更 | 否 |
| 13 | `2f0accaefc` | Merge branch 'main' | 合并提交 | 无额外变更 | 否 |
| 14 | `b51814ff4d` | 合入主线并保留运行缓存回归 | 合并/保留提交 | 无额外变更 | 否 |
| 15 | `7b1c6f5925` | Merge branch 'main' | 合并提交 | 无额外变更 | 否 |
| 16 | `ec812f90ea` | Merge branch 'main' into v0.0.122 | 合并提交 | 无额外变更 | 否 |
| 17 | `c8d03bb33f` | 修复运行缓存跨目录路径漏钉(0.0.124) | `cx/runtime/`、`cx/text/`、`src/`、`tests/`、`release/`、`build/` | 是(`build/tlib-packages.tsv`、`build/layer-api/cx.runtime.tsv`、`build/layer-api/cx.text.tsv`、`build/iface-hash.tsv`、`build/links.tsv`) | 会改 SID(改 `cx/runtime/rcident.cx`、增 `cx/text/pinscan.cx`、改 `src/runcache.cx`, 属产品编译闭包核心输入, 推断) |
| 18 | `661de08935` | 隔离转发桩并验证显式分相收据(0.0.122) | `cx/compiler/`、`cx/driver/`、`exec/facts/`、`include/`、`src/`、`archive/`、`tests/`、`tools/`、`build/` | 是(`build/tlib-packages.tsv`、`build/layer-api/cx.driver.tsv`、`build/iface-hash.tsv`、`build/chain-phase-outputs.tsv`、`build/libc-decls.tsv`、`build/links.tsv`) | 会改 SID(改 `cx/compiler/fwdstub.cx`、`src/drive.cx`、`exec/facts/`、`include/`, 属产品与包阶段输出输入, 推断) |

注: 第 17、18 笔在分枝入车时已带有 `build/tlib-packages.tsv`、`build/layer-api/` 与 `build/iface-hash.tsv` 变更; 合 main 后须按最终合并树核对 `layer-api` 与 `tlib` 号(参见 `prd_mempalace.md` 第 46、77 条)。

## 2. 落后的 main 提交表(`HEAD..origin/main`, 共 31 笔)

经 `git -C .claude/worktrees/v0.0.106 log HEAD..origin/main -- cx/ src/ exec/ include/ seed/ kernel/` 核对: 触及产品输入(冻结 paths)的提交数为 `0` 笔。

| 序号 | 提交号 | 摘要 | 路径分类(tools/tests/build/plans/research/prd/产品输入) | 是否触及产品输入与 SID 推断 |
| --- | --- | --- | --- | --- |
| 1 | `940bee00a9` | 0.0.125 变异门超时杀进程组补丁与证据 | `research/` (`.md` 与 `.patch`) | 否(补丁在 `research/` 下未入产品树; 不改 SID, 推断) |
| 2 | `71c7035f9c` | R27 S2: chain-ready 23-freeze-armed + release-exec F_CHAIN_REV | `tools/`、`tests/`、`build/`(`table-index.tsv`) | 否(仅工具与派生表; 不改 SID, 推断) |
| 3 | `b1defca6b1` | check-layer-api 改用 rn_prepare | `tools/` | 否(不改 SID, 推断) |
| 4 | `51d27f88de` | 更正未验证清点口径与本周关闭方案 | `research/` | 否(不改 SID, 推断) |
| 5 | `d1a1ca1924` | 拆分 Git c 批次并缩短回执冷态 | `tools/`、`tests/`、`build/`(`links.tsv`) | 否(不改 SID, 推断) |
| 6 | `a39e23f0e3` | 清点 prd_mempalace 与 state-unverified 差异 | `research/` | 否(不改 SID, 推断) |
| 7 | `615e1c90ec` | 记忆宫殿与计划(0.0.103 不入第一车/0.0.126 根因/引用改指向) | `plans/`、`prd_mempalace.md` | 否(不改 SID, 推断) |
| 8 | `b27326d32a` | 主树 Git add 限进行中状态与冻结包私有取号 | `tools/`、`plans/` | 否(不改 SID, 推断) |
| 9 | `38ea37666a` | CXM_MAXU 256 补丁按提交门清理 | `research/` (`.md` 与 `.patch`) | 否(仅研究区补丁; 不改 SID, 推断) |
| 10 | `6948fa5438` | prerun-drive 改写 void 帮手(G24) | `tools/` | 否(不改 SID, 推断) |
| 11 | `9206724bc1` | 梳理 prd_mempalace 会话临时目录改指向清单 | `research/` | 否(不改 SID, 推断) |
| 12 | `973574b30b` | check-precommit-toolsloc 夹具补放链接 | `tools/` | 否(不改 SID, 推断) |
| 13 | `badd8c1df5` | CXM_MAXU 256 保守方案更新 | `plans/`、`research/` (`.md` 与 `.patch`) | 否(仅计划与研究区补丁; 不改 SID, 推断) |
| 14 | `706898684d` | 非 Git 进行中状态拒绝 commit-own --staged | `tools/` | 否(不改 SID, 推断) |
| 15 | `7f1e8fc26e` | 清理 15 个超 3 小时可删目录回执 | `research/` | 否(不改 SID, 推断) |
| 16 | `9d916986c1` | check-precommit-toolsloc 改用 rn_prepare | `tools/` | 否(不改 SID, 推断) |
| 17 | `992cfbdbb6` | 补 G32 清扫和并发 SIGKILL 用例 | `tools/` | 否(不改 SID, 推断) |
| 18 | `5886338dda` | 记录并清理 12 个挂死测试进程 | `research/` | 否(不改 SID, 推断) |
| 19 | `1bdd310225` | register 未跟踪工具与私有索引回滚 | `tools/`、`tests/` | 否(不改 SID, 推断) |
| 20 | `5f87ece98b` | 归档 /tmp 与 ~/.cache/unisacc 遗留清单 | `research/` | 否(不改 SID, 推断) |
| 21 | `05a42fee33` | CXM_MAXU 256 私有重放补丁与证据 | `research/` (`.md` 与 `.patch`) | 否(仅研究区补丁; 不改 SID, 推断) |
| 22 | `ad38cfd9d0` | 归档 2026-11-24 fable 设计与 gatekeeper 审查报告 | `research/` | 否(不改 SID, 推断) |
| 23 | `e2c6a48114` | 记忆宫殿与 0.0.126 计划及记分表 | `plans/`、`prd_mempalace.md`、`skills/` | 否(不改 SID, 推断) |
| 24 | `e152458614` | devsvc git unstage 具名拒绝与写窗口锁 | `tools/`、`tests/`、`build/`(`links.tsv`)、`prd.md` | 否(不改 SID, 推断) |
| 25 | `aa54ff7c94` | 0.0.125 交接笔记与变异门超时必做项 | `plans/`、`research/` | 否(不改 SID, 推断) |
| 26 | `f7e7bf76b3` | 撤销 2e90c55fd2(register --promote 重写) | `tools/` | 否(不改 SID, 推断) |
| 27 | `d75719eef4` | 0.0.108 入车清单给 v0.0.106 | `research/` | 否(不改 SID, 推断) |
| 28 | `08d45f74f4` | prerun-drive G24 拆双批与变异裁定 | `tools/`、`tests/`、`prd_mempalace.md` | 否(不改 SID, 推断) |
| 29 | `1ec29fc8ca` | 版本序与计划: 0.0.106 改为集成车 | `build/`(`version-seq.tsv`)、`plans/` | 否(不改 SID, 推断) |
| 30 | `263a36191e` | retcheck 返回值约定棘轮与两新工具登记 | `tools/`、`tests/`、`build/`(`retcheck-baseline.tsv`、`table-index.tsv`) | 否(不改 SID, 推断) |
| 31 | `2e90c55fd2` | register 实现 --promote (已被第 26 笔撤销) | `tools/` | 否(不改 SID, 推断) |

注: 按目录维度统计(单笔可跨多类): `tools/` 15 笔、`tests/` 6 笔、`build/` 5 笔、`plans/` 6 笔、`research/` 13 笔、`prd`/记分表 4 笔、产品输入(`cx/ src/ exec/ include/ seed/ kernel/`) 0 笔。

## 3. 预演合并冲突表(`git merge-tree`)

环境说明: 本机 `git version 2.34.1` 不支持 `git merge-tree --write-tree`(rc=128 `unknown rev --write-tree`), 故采用 git 2.34.1 内置只读三方预演 `git -C .claude/worktrees/v0.0.106 merge-tree ce7c2b483e HEAD origin/main` 核对全部双侧修改文件(`comm -12` 交集共 3 个文件)。

| 序号 | 双侧共改路径 | 预演合并结果 | 冲突位置与原因 | 建议合入处置 |
| --- | --- | --- | --- | --- |
| 1 | `build/links.tsv` | 冲突(`<<<<<<< .our`) | 车侧加 `tools-compile-191`, main 侧加 `tools-compile-192` 同行邻接冲突; 另有 `devsvc-git-c-*` 与 `devsvc-git-unstage` 自动合入 | 派生表冲突, 合后重跑 `derive.cx --write` 现算覆盖即可 |
| 2 | `tests/batches.tsv` | 无冲突(自动合并) | 两侧增行位于不同批号区间 | 无需手解(合后仍跑 `tsv-lint` 与 `derive` 校验) |
| 3 | `tests/coverage.tsv` | 无冲突(自动合并) | 两侧增改行位置不重 | 无需手解(合后核对 `coverage` 与 `batches` 一致性) |

## 4. 汇总与冻结前结论

(1) 落后 main 提交数: `31` 笔(车独有提交 `18` 笔)。
(2) 落后 main 提交中触及产品输入数: `0` 笔(推断不会因合 main 改变产品 SID; 车自身已带 `3` 笔产品输入提交 `578c617de6`、`c8d03bb33f`、`661de08935` 均按预期会改 SID, 推断)。
(3) 预演合并冲突文件数: `1` 个(`build/links.tsv` 单处 `tools-compile-191`/`192` 派生行冲突; `tests/batches.tsv` 与 `tests/coverage.tsv` 无冲突自动合并)。
(4) 额外提示: 合 main 后可立即获得 `71c7035f9c` 的 R27 S2(`chain-ready` 23-freeze-armed 与 `release-exec F_CHAIN_REV`)、R27 S1/prerun-drive G24 优化及 `retcheck` 门; 同时需按规则对合入树重算派生表(`build/links.tsv`、`build/table-index.tsv` 等)并核对 `build/tlib-packages.tsv`、`build/layer-api/`、`build/iface-hash.tsv`。
