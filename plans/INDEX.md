# plans/ 索引

本索引覆盖原 `plans/` 下全部 86 个受跟踪文件:保留 28 个现行版本计划、路线图、架构规范与工具硬依赖文件,其余 58 个历史计划、草稿、一次性测量表与旧补丁已经 `git mv` 归档至 `archive/plans/`(保留完整 git 历史,无删除)。

本文件是 `plans/` 的唯一入口(2026-11-19 梳理):`plans/` 下每个受跟踪文件在下方总表都有一行;已归档文件只记指针。

## 分类速查

| 类别 | 文件 |
| --- | --- |
| 现行路线 | roadmap-0.0.83-0.0.99.md、roadmap-rev3.md、changes-rev3.md、speed-first-rev4.md、nn-migration-rev3.md、cx-lib-roadmap-2026-11-16.md、standing-goal.md |
| 现行版本计划 | v0.0.89.md(2026-11-20 顺延:核心层库面小件包与模块解析登记,原 0.0.88)、v0.0.88.md(2026-11-20 顺延:A1a 内建与符号,原 0.0.87)、v0.0.87.md(2026-11-20 重写:迭代速度专版,整版不排产品特性,证据 reviews/2026-11-20-fast-iteration.md)、v0.0.86.md(2026-11-20 骨架:改一行再跑,路线 A,见 roadmap-0.0.83-0.0.99.md 末节修订注 2026-11-20)、v0.0.85.md(2026-11-20 修订:前端诊断三期,P0 NO-GO)、v0.0.84.md(2026-11-20 已公开)、v0.0.83.md、v0.0.82.md(前几版保留) |
| 设计与规范 | devsvc.md、cx-layers.md、cx-modules.md、cx-dock-pragma.md、cx-dev.md、q-p-design.md、root-identity.md、test-loader-com.md、m7-material-spec.md、macos-signing.md、product-refactor-plan.md、q11-merge-rehearsal.md、notcovered-plan.md、dev-methodology-2026-10-28.md |
| 表(被工具读取或跟踪) | slice-units.tsv、unit-text.tsv(slice-migration-gen 输入)、slice-nn-migration.md(其产出)、v0.0.73-speed-tracker.tsv |
| 审阅记录 | nn-table-review-2026-11-18.md、reviews/(2026-11-20 起 fable 报告正文拷贝:asm-acceleration、core-layer-marking、devsvc-evolution、chain-incremental、gate-086-edit-rerun、table-construction、layer-lanes、core-ext-matrix、fast-iteration);分车道条文改写稿 claude-md-layer-lanes-draft.md(暂不生效);团队编排骨架 layer-orchestration.md(第 (12) 节主调度流程改进规则) |
| 历史但留原位(有工具或登记表字面量) | release-0.0.43.md(tools/release-board.cx 等读取)、v0.0.78.md、v0.0.79.md、v0.0.80.md、roadmap-v0.0.80-v0.0.89.md(build/version-seq.tsv 引用,state-gen 读取) |
| 子目录 | briefs/(简报规范与模板)、pending/、patches/、w5-patches/ |
| 历史归档指针 | `archive/plans/`(2026-11 首轮 58 个)、`archive/plans-pre082/`(0.0.82 前已公开版本的计划与设计,清单见其 README.tsv) |

说明:`release/notes-*.md` 与 `release/evidence/` 下的历史发布说明及证据正文保持原文不改,其中提及的旧 `plans/<file>` 路径对应下表中的 `archive/plans/<file>`。

| 文件 | 状态 | 被哪版取代 | 一行职责 |
| --- | --- | --- | --- |
| [archive/plans/bom-source-start.md](../archive/plans/bom-source-start.md) | 已归档 | v0.0.48 | BOM-SOURCE-START 候选方案设计 |
| [plans/briefs/README.md](briefs/README.md) | 现行 | - | 子代理简报与交付自检规范(cc 指定保留) |
| [plans/briefs/TEMPLATE.md](briefs/TEMPLATE.md) | 现行 | - | 子代理简报模板(cc 指定保留) |
| [archive/plans/check-coldcompile.tsv](../archive/plans/check-coldcompile.tsv) | 已归档 | v0.0.73-speed-tracker | 2026-11-11 冷编译耗时测量表 |
| [archive/plans/codegen-speedup-2026-11-16.md](../archive/plans/codegen-speedup-2026-11-16.md) | 已归档 | roadmap-v0.0.80-v0.0.89 | 0.0.71 生成码一期(X3)提速路线 |
| [archive/plans/core-interface-2026-11-02.md](../archive/plans/core-interface-2026-11-02.md) | 已归档 | coreapi 棘轮 | core 层接口清单(已固化实现) |
| [plans/cx-dev.md](cx-dev.md) | 现行 | - | cx-dev 本机快速迭代车道设计 |
| [plans/cx-dock-pragma.md](cx-dock-pragma.md) | 现行 | - | CX dock pragma 模块显式对接规范(14 活引用) |
| [archive/plans/cx-l0-fmt-2026-10-30.md](../archive/plans/cx-l0-fmt-2026-10-30.md) | 已归档 | cx-lib-roadmap-2026-11-16 | L0 格式化与数值解析补齐计划(已实现) |
| [archive/plans/cx-l0-sig-2026-10-30.md](../archive/plans/cx-l0-sig-2026-10-30.md) | 已归档 | cx-lib-roadmap-2026-11-16 | L0 信号与 symlink 补齐计划(已实现) |
| [plans/cx-layers.md](cx-layers.md) | 现行 | - | CX L0-L3 分层架构规范(31 活引用含 cxraw) |
| [plans/cx-lib-roadmap-2026-11-16.md](cx-lib-roadmap-2026-11-16.md) | 现行 | - | cx 核心库与扩展库完善路线(cc 指定保留) |
| [plans/cx-modules.md](cx-modules.md) | 现行 | - | .cx 无 #include 模块化与安全规范(19 活引用) |
| [archive/plans/cx-use-alias-reexport.md](../archive/plans/cx-use-alias-reexport.md) | 已归档 | prd_cx_spec.md | pragma unisa use 符号导入别名设计(已实现) |
| [archive/plans/defects-c43.md](../archive/plans/defects-c43.md) | 已归档 | v0.0.78+ | 0.0.43 历史已知缺陷登记 |
| [archive/plans/dep-split.md](../archive/plans/dep-split.md) | 已归档 | net-deps 门 | 共享 facts 与生成器拆分方案 |
| [plans/dev-methodology-2026-10-28.md](dev-methodology-2026-10-28.md) | 现行 | - | 双车道开发方法学(CLAUDE.md 与 doc-links 保护) |
| [archive/plans/devsvc-speedup-2026-11-16.md](../archive/plans/devsvc-speedup-2026-11-16.md) | 已归档 | plans/devsvc.md | devsvc 协同提速方案(已合入 devsvc.md) |
| [plans/devsvc.md](devsvc.md) | 现行 | - | devsvc 仓开发服务设计稿(cc 指定保留);2026-11-20 演进评审(前五项、度量基线、0.0.86 到 0.0.88 工具线节拍)见其"2026-11-20 演进评审"节 |
| [archive/plans/goal-10m-unisacc.md](../archive/plans/goal-10m-unisacc.md) | 已归档 | v0.0.80 | 历史定时器提示词(仅 1 处 release 引用) |
| [archive/plans/goal-oneoff-unisacc.md](../archive/plans/goal-oneoff-unisacc.md) | 已归档 | v0.0.80 | 2026-11-16 一次性 /goal 提示词 |
| [archive/plans/identity-inventory-2026-11-02.md](../archive/plans/identity-inventory-2026-11-02.md) | 已归档 | plans/root-identity.md | 2026-11-02 身份指纹机制盘点 |
| [archive/plans/identity-unify-2026-11-02.md](../archive/plans/identity-unify-2026-11-02.md) | 已归档 | plans/root-identity.md | 2026-11-02 身份机制收敛草案 |
| [archive/plans/interface-versioning-design.md](../archive/plans/interface-versioning-design.md) | 已归档 | v0.0.59+ | I2 接口版本化 7 阶段摘要设计 |
| [archive/plans/local-loader-2026-11-16.md](../archive/plans/local-loader-2026-11-16.md) | 已归档 | plans/product-refactor-plan.md | 0.0.69 本机快速加载器落地方案 |
| [archive/plans/local-product-0.0.71.md](../archive/plans/local-product-0.0.71.md) | 已归档 | v0.0.78+ | 0.0.71 本地混合候选测量记录 |
| [archive/plans/m4-flownet-cx-module.md](../archive/plans/m4-flownet-cx-module.md) | 已归档 | tools/cxlib/flow*.cx | m4 流网范式 cx 模块早期需求 |
| [plans/m7-material-spec.md](m7-material-spec.md) | 现行 | - | M7 材料规范(11-17 仍在维护) |
| [plans/macos-signing.md](macos-signing.md) | 现行 | - | macOS 签名与公证接入现行方案 |
| [archive/plans/materials-inventory-2026-11-02.md](../archive/plans/materials-inventory-2026-11-02.md) | 已归档 | v0.0.59+ | 2026-11-02 有材料批次只读盘点 |
| [archive/plans/nn-nf-b-prep-2026-11-05.md](../archive/plans/nn-nf-b-prep-2026-11-05.md) | 已归档 | v0.0.48+ | NN-NF-B 开工依据(SBOUT 长序列实测) |
| [plans/nn-table-review-2026-11-18.md](nn-table-review-2026-11-18.md) | 现行 | - | 2026-11-18 NN TSV 表审阅与体检摘要 |
| [plans/notcovered-plan.md](notcovered-plan.md) | 现行 | - | notcovered 编译覆盖缺口分期修复计划 |
| [archive/plans/patches/README.md](../archive/plans/patches/README.md) | 已归档 | v0.0.48+ | 旧补丁提案目录说明(与 archive/plans/README.md 重名) |
| [archive/plans/patches/driver-diagnostics.diff](../archive/plans/patches/driver-diagnostics.diff) | 已归档 | v0.0.48+ | 10-28 旧诊断补丁提案 |
| [archive/plans/patches/x1-runcache-key-v2.patch](../archive/plans/patches/x1-runcache-key-v2.patch) | 已归档 | v0.0.69+ | 11-16 X1 运行缓存键补丁 v2(0 活引用) |
| [archive/plans/patches/x1-runcache-key.patch](../archive/plans/patches/x1-runcache-key.patch) | 已归档 | v0.0.69+ | 11-16 X1 运行缓存键补丁 v1(0 活引用) |
| [archive/plans/perf-bottlenecks-2026-10-30.md](../archive/plans/perf-bottlenecks-2026-10-30.md) | 已归档 | plans/v0.0.73-speed-tracker.tsv | 2026-10-30 编译提速瓶颈地图 |
| [plans/product-refactor-plan.md](product-refactor-plan.md) | 现行 | - | unisacc 产品整体结构小型重构计划(cc 指定保留) |
| [plans/q11-merge-rehearsal.md](q11-merge-rehearsal.md) | 现行 | - | Q-11 合并预演与冻结流水线(cc 指定保留) |
| [archive/plans/rc1-gates-2026-11-06.md](../archive/plans/rc1-gates-2026-11-06.md) | 已归档 | v0.0.48+ | 0.0.44-rc1 发布门槛(tests 矩阵 20 处引用) |
| [plans/release-0.0.43.md](release-0.0.43.md) | 留:被 release-board 工具读取 | v0.0.48+ | 0.0.43 发布计划(注:tools/release-board.cx 读此路径) |
| [archive/plans/release-gates-0.0.68.md](../archive/plans/release-gates-0.0.68.md) | 已归档 | v0.0.78+ | 0.0.68 发布核对记录 |
| [archive/plans/release-notes-0.0.48.md](../archive/plans/release-notes-0.0.48.md) | 已归档 | release/notes-0.0.48.md | 0.0.48 发布说明草稿 |
| [archive/plans/release-notes-0.0.58.md](../archive/plans/release-notes-0.0.58.md) | 已归档 | release/notes-0.0.58.md | 0.0.58 发布说明草稿 |
| [archive/plans/release-notes-0.0.59.md](../archive/plans/release-notes-0.0.59.md) | 已归档 | release/notes-0.0.59.md | 0.0.59 发布说明草稿 |
| [archive/plans/release-notes-0.0.61.md](../archive/plans/release-notes-0.0.61.md) | 已归档 | release/notes-0.0.61.md | 0.0.61 发布说明草稿 |
| [archive/plans/release-notes-0.0.67.md](../archive/plans/release-notes-0.0.67.md) | 已归档 | release/notes-0.0.67.md | 0.0.67 发布说明草稿 |
| [archive/plans/release-notes-0.0.68.md](../archive/plans/release-notes-0.0.68.md) | 已归档 | release/notes-0.0.68.md | 0.0.68 发布说明草稿 |
| [archive/plans/release-speedup-plan.md](../archive/plans/release-speedup-plan.md) | 已归档 | plans/v0.0.73-speed-tracker.tsv | 0.0.68-0.0.69 发布提速计划 |
| [plans/roadmap-v0.0.80-v0.0.89.md](roadmap-v0.0.80-v0.0.89.md) | 被取代 | roadmap-rev3 | 0.0.80 到 0.0.89 路线图修订版 2(保留作历史对照,现行见 roadmap-rev3) |
| [plans/roadmap-rev3.md](roadmap-rev3.md) | 现行 | - | 路线图修订版 3(2026-11-19,fable 加我裁定):0.0.81 实际交付、速度专项 S0 到 S7、发布节奏裁决 |
| [plans/roadmap-0.0.83-0.0.99.md](roadmap-0.0.83-0.0.99.md) | 现行 | - | 0.0.83 到 0.0.99 容量模型与逐版编排,及 0.0.100 汇编级精修阶段入口准则(估计值,每 3 版重估);汇编支持基本需求见修订注 2026-11-20 |
| [plans/v0.0.82.md](v0.0.82.md) | 现行 | - | 0.0.82 详细计划(前端诊断二期、ref 判据替换、速度专项首批) |
| [plans/v0.0.83.md](v0.0.83.md) | 现行 | - | 0.0.83 Q-P 首次尝试版计划与 CI 对账前置(2026-11-20 已公开;四道表 0/10 完成,12 项计划外,Q-P 报告未做) |
| [plans/v0.0.86.md](v0.0.86.md) | 现行 | - | 0.0.86 改一行再跑计划骨架(2026-11-20):run 路单元级分别编译并缓存加 RR_TIME 带步数与内层循环实验决策门加 linuxns 白名单入口;planned 未定(开槽前判定门后写);路线依据见 roadmap-0.0.83-0.0.99.md 末节"修订注 2026-11-20(fable 深度反思)";整链预制新路线(工具线 T3 推测链真起加并行确认、发布流程项 P1 周节奏命令)见其工具线节与 roadmap 末节"修订注 2026-11-20(整链与预制)",证据入口 reviews/2026-11-20-chain-incremental.md(原会话目录 chain-incremental.md 已被取代);M2 改为先补度量再三刀分别量,储备 K1 核心层强制第一步 |
| [plans/v0.0.87.md](v0.0.87.md) | 现行 | - | 0.0.87 骨架(2026-11-20 重写):迭代速度专版,整版不排产品特性,只做结构项 S1 到 S7(发布列车去中心化、预跑并行度恢复、消费者清扫左移、仓根升级与公开脱钩、lane-inputs 与包门、度量补账、内容截止规则);证据 reviews/2026-11-20-fast-iteration.md,见 roadmap 修订注"迭代速度专版" |
| [plans/v0.0.88.md](v0.0.88.md) | 现行 | - | 0.0.88 骨架(2026-11-20 顺延):A1a 内建与符号(原 0.0.87 第三版内容,开头"原 0.0.87 内容"说明顺延);证据 reviews/2026-11-20-asm-acceleration.md 与 reviews/2026-11-20-core-layer-marking.md |
| [plans/v0.0.89.md](v0.0.89.md) | 现行 | - | 0.0.89 骨架(2026-11-20 顺延,原 0.0.88):核心层库面小件包(L0 进程一族、sy_syscall6、熵源、mtime 纳秒)与模块解析登记(testx argx fsx 三包、仓外脚本包根);证据 reviews/2026-11-20-core-ext-matrix.md;信号定时器待定 |
| [plans/v0.0.85.md](v0.0.85.md) | 现行 | - | 0.0.85 前端诊断三期计划(2026-11-20 修订):c-negative 53/9 -> 62/0 不再续期、拒绝带 文件:行:列、-Werror 升级与隐式内建误报;P0 NO-GO,固定前缀预制归档附录 B;planned=11,槽到公开目标 300 分钟 |
| [plans/v0.0.84.md](v0.0.84.md) | 现行 | - | 0.0.84 冷启动第一块计划:模型装载与前端网 E3 与 E2 固定工作量削减,产物字节不变(2026-11-20 已公开;四道表 6/9 完成,6 项计划外,F2 S2 Q2 未做;附录保留 cdx 网级推理剖面与候选项) |
| [plans/changes-rev3.md](changes-rev3.md) | 现行 | - | 修订版 3 相对旧版的差异清单与三个最危险假设 |
| [plans/nn-migration-rev3.md](nn-migration-rev3.md) | 现行 | - | 启动器与六片逻辑迁移 NN 的现状、分版本路线、决策门(与 slice-nn-migration.md 旧表互补) |
| [plans/speed-first-rev4.md](speed-first-rev4.md) | 现行 | - | 速度项前置重排(四类速度 P/B/V/D、R0 到 R3、总口径为槽到公开总墙钟、吞吐度量,fable 2026-11-19)与 0.0.82 到 0.0.86 速度道逐版清单 |
| [plans/q-p-design.md](q-p-design.md) | 现行 | - | Q-P 产品资格口径草案:整域分母的五类资格状态与发布门 |
| [plans/root-identity.md](root-identity.md) | 现行 | - | 本地根产品身份规范(11-17 维护,10 活引用) |
| [archive/plans/runtime-speedup-lessons-2026-11-16.md](../archive/plans/runtime-speedup-lessons-2026-11-16.md) | 已归档 | roadmap-v0.0.80-v0.0.89 | 运行与启动加速外部经验与落地设计 |
| [plans/slice-nn-migration.md](slice-nn-migration.md) | 现行 | - | M10a 切片 NN 化迁移清单(slice-migration-gen 硬编码产出) |
| [plans/slice-units.tsv](slice-units.tsv) | 现行 | - | M10a 切片单元事实表(slice-migration-gen 硬编码输入) |
| [archive/plans/speed-0.0.69.md](../archive/plans/speed-0.0.69.md) | 已归档 | plans/v0.0.73-speed-tracker.tsv | 0.0.69 速度一期任务书(R1/X1) |
| [archive/plans/speed-hotspots-2026-11-02.md](../archive/plans/speed-hotspots-2026-11-02.md) | 已归档 | plans/v0.0.73-speed-tracker.tsv | 2026-11-02 提速热点表 |
| [plans/standing-goal.md](standing-goal.md) | 现行 | - | cc 自驱常设目标 |
| [archive/plans/t2-pin-rationale.tsv](../archive/plans/t2-pin-rationale.tsv) | 已归档 | v0.0.59+ | 2026-11-08 T2 pin 变更依据表 |
| [plans/test-loader-com.md](test-loader-com.md) | 现行 | - | TEST-LOADER-COM 测试部件加载器设计(11-17 维护) |
| [archive/plans/unit-frontcache.md](../archive/plans/unit-frontcache.md) | 已归档 | v0.0.76+ | 单元前端(e2/e1)身份缓存键合同 |
| [plans/unit-text.tsv](unit-text.tsv) | 现行 | - | 切片单元 .text 估量表(slice-migration-gen 硬编码输入) |
| [archive/plans/v0.0.43-units.md](../archive/plans/v0.0.43-units.md) | 已归档 | v0.0.78+ | 0.0.43 单元化重构边界设计 |
| [archive/plans/v0.0.43.md](../archive/plans/v0.0.43.md) | 已归档 | v0.0.78+ | 0.0.43 版本计划(tests 矩阵 8 处引用) |
| [archive/plans/v0.0.47-corpus-feasibility.md](../archive/plans/v0.0.47-corpus-feasibility.md) | 已归档 | v0.0.78+ | 0.0.47 外部语料可行性首轮 |
| [archive/plans/v0.0.47-vm-resource-contract.md](../archive/plans/v0.0.47-vm-resource-contract.md) | 已归档 | v0.0.78+ | 0.0.47 UNIKERN1 执行器资源合同缺口 |
| [archive/plans/v0.0.48-unisacc.md](../archive/plans/v0.0.48-unisacc.md) | 已归档 | v0.0.78+ | 0.0.48 版本计划 |
| [archive/plans/v0.0.59-unisacc.md](../archive/plans/v0.0.59-unisacc.md) | 已归档;DOC-1 到 DOC-5 作废(原计划规模口径过时,见 INDEX 备注) | v0.0.78+ | 0.0.59 版本计划任务树 |
| [plans/v0.0.73-speed-tracker.tsv](v0.0.73-speed-tracker.tsv) | 现行 | - | 0.0.73+ 提速跟踪表(cc 指定保留) |
| [archive/plans/v0.0.73.md](../archive/plans/v0.0.73.md) | 已归档 | v0.0.78+ | 0.0.73 版本计划草稿(诊断一期) |
| [archive/plans/v0.0.75-r2-w3.md](../archive/plans/v0.0.75-r2-w3.md) | 已归档 | v0.0.78+ | 0.0.75 R2 一期 W3 纯链接步设计 |
| [archive/plans/v0.0.75-r2-w3a2.md](../archive/plans/v0.0.75-r2-w3a2.md) | 已归档 | v0.0.78+ | 0.0.75 R2 W3a2 对象进程槽具名化设计 |
| [archive/plans/v0.0.75-r2-w3b-check.patch](../archive/plans/v0.0.75-r2-w3b-check.patch) | 已归档 | v0.0.75+ | 0.0.75 R2 W3b 检查补丁(历史补丁) |
| [archive/plans/v0.0.75-r2-w3b.patch](../archive/plans/v0.0.75-r2-w3b.patch) | 已归档 | v0.0.75+ | 0.0.75 R2 W3b 补丁(历史补丁) |
| [archive/plans/v0.0.75-r2.md](../archive/plans/v0.0.75-r2.md) | 已归档 | v0.0.78+ | 0.0.75 R2 链接单元一期方案与基线 |
| [archive/plans/v0.0.76-modelcache-evidence.tsv](../archive/plans/v0.0.76-modelcache-evidence.tsv) | 已归档 | v0.0.76+ | 0.0.76 modelcache 实测证据表(0 引用) |
| [archive/plans/v0.0.76-modelcache.patch](../archive/plans/v0.0.76-modelcache.patch) | 已归档 | v0.0.76+ | 0.0.76 modelcache 补丁(历史补丁) |
| [archive/plans/v0.0.76-r2-w2a.patch](../archive/plans/v0.0.76-r2-w2a.patch) | 已归档 | v0.0.76+ | 0.0.76 R2 W2a 补丁(历史补丁) |
| [archive/plans/v0.0.77-p1-registry-rows.txt](../archive/plans/v0.0.77-p1-registry-rows.txt) | 已归档 | v0.0.77+ | 0.0.77 P1 登记行草稿(0 引用) |
| [archive/plans/v0.0.77-p5-registry-rows.txt](../archive/plans/v0.0.77-p5-registry-rows.txt) | 已归档 | v0.0.77+ | 0.0.77 P5 登记行草稿(0 引用) |
| [archive/plans/v0.0.77.md](../archive/plans/v0.0.77.md) | 已归档 | v0.0.78 | 0.0.77 版本计划(内存安全乙期一段) |
| [plans/v0.0.78.md](v0.0.78.md) | 历史,留原位 | 已公开 | 0.0.78 版本计划(build/version-seq.tsv 引用,不移动) |
| [plans/v0.0.79.md](v0.0.79.md) | 历史,留原位 | 已公开 | 0.0.79 版本计划(build/version-seq.tsv 引用,不移动) |
| [archive/plans-pre082/v0.0.80-f1-design.md](../archive/plans-pre082/v0.0.80-f1-design.md) | 已归档 | v0.0.80 已公开 | 0.0.80 F1 对象产出合同设计单 |
| [plans/v0.0.81.md](v0.0.81.md) | 现行 | - | 0.0.81 版本计划(前两版保留,changes-rev3 与 roadmap-rev3 引用) |
| [plans/v0.0.80.md](v0.0.80.md) | 历史,留原位 | 已公开 | 0.0.80 版本计划(build/version-seq.tsv 引用,不移动) |
| [archive/plans/win-arm64-fixedpoint.md](../archive/plans/win-arm64-fixedpoint.md) | 已归档 | v0.0.48+ | win/arm64 逐片固定点 c46-3 实证历史 |

## 作废决定

- `archive/plans/v0.0.59-unisacc.md` 中的 DOC-1 到 DOC-5 文档重组计划正式作废(原文未改,仅作历史留存)。
- 理由:跨 20 个版本未开始,涉及的旧引用数与搬运规模(prd 120 处行号引用、43 个文档折入等)已不成立,由路线图 0.0.81 用户道的轻量方案(plans/ 归档加 INDEX、doc-links 死链基线只降、README 口径对账)取代。
