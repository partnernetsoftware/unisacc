# prd_mempalace 已完成版本事实原文(2026-11-23 归档)

2026-11-23 从 prd_mempalace.md 原第 271-298 行移出(保留原 275-276 的回滚办法与 Root 备份"不得清扫"在原位)。仅作历史。

### (3) 已公开版本(不可逆动作)
- 0.0.89 已公开:候选 `f77e64a25b`,冻结 `74f541c6cc`,Y2 `57de2b4ddd`,六格 CI `44935566045` QUALIFIED;严格预跑 440 PASS/0 FAIL/2 UNKNOWN(无生产者)。 入口:`release/notes-0.0.89.md`、`release/evidence/0.0.89/`
- 0.0.90 已公开并置 Latest:候选 `9109473641`,冻结 `5d1e492d7e`,阶段 A `6d7f11d22c`,Y1 `c438b6d628`,Y2 `13a48bb870`,六格 CI `44973535033`;APE sha256 前缀 `46fd3db95f97`(8991222 字节)。 入口:`release/evidence/0.0.90/`
- 0.0.90 证据:严格预跑 459 批 457 PASS/0 FAIL/2 UNKNOWN(`cx-booter-ape`、`tools-default-regression` 无生产者)。 入口:`release/evidence/v0.0.90-strict-2026-11-22.tsv`、`release/evidence/0.0.90/backup-evidence.tsv`

- U4 汇编标签裁定:仅支持 `__asm__`/`__asm`;裸 `asm` 具名拒绝但保留作普通标识符;`osx`/`win` 具名拒绝;4 处比 GCC 11.4 更严属有意差异,须留在发布说明。 依据:0.0.90 开版评审 入口:`plans/a1a-design/u4-decisions.md`

### (4) 0.0.90 已知缺口与未验证项
- 资格:0.0.90 整体裁决 NOT_QUALIFIED,全套 `s9` 的 `daily`/`release` 未跑通;`daily` 仅有 15 片编译冒烟,完整运行态未验。 入口:`release/notes-0.0.90.md`
- R75:`tools/tsv-lint.cx` 未解析引用实测 11351(0.0.89 的 11277 到 0.0.90 涨 74,来自 U4 新增的动态绑定表行,每个新行必计一条),上限由所有者 2026-11-22 从 11050 放宽到 13260(11050 加 20%,为不阻塞版本);0.0.92 只做门执行(上限必须有门拦)加 asm volatile 行列号,削表(F 绑定)延后;增长必须拦:基线固化为实测值只降不升。 入口:`build/owner-reqs.tsv` R75
- R78:变异门真实杀伤率已由 0.0.90 四补丁补 `--expect-fail` 与 `release-chain.cx` `sy_realpath>0` 补强至 `[57/82,59/82]=[69.51%,71.95%]` 高于 0.60 线(`release-chain` 20/26=0.77 PASS)。 入口:`tools/mutation-gate.cx`、`research/2026-11-22/r78-v090-named/summary.md`
- 诊断行列号:U4 差异项与 `asm volatile` 拒绝缺 `file:line:col`,排 0.0.92。 入口:`build/owner-reqs.tsv` R55
- 字面量长度:41 条真缺陷的全套补丁未落(顺序:检查器与棘轮 -> 越界读 -> 工具 -> 检查 -> 注解;`gate` 须在 `annotations` 之后)。 入口:`archive/plans-pre094/v0.0.91.md`、`research/2026-11-22/litlen-089/handoff.md`、`research/2026-11-22/litlen/real-bugs-41.tsv`
- R69 普查余项:24 个不可达 `*-gen` 待核,约 22 个二级死传递进候选清理;28 批未接线检查已登记。 入口:`build/owner-reqs.tsv` R69、`research/2026-11-22/r69-report/report.md`
- 共享写治理:`tools/check-shared-writes.cx`、预提交门与 `build/shared-writes-baseline.tsv`(27 行只减不增)均未落库(ls 核不存在);R71 的 M3/M4/M6 未收口。 入口:`build/owner-reqs.tsv` R71、R73
- `textx.isotime`:缺 JS 引擎对拍夹具(`tests/fixtures/textx-isotime/` 不存在),解析取舍(无时区返 `-EINVAL`、越界严格拒绝)仅按规范编写;未经 `affected --run` 整批。 入口:`tools/check-textx-isotime.cx`
- 死 C 源码:`tools/cx-pipeline-stage.c:5`、`tools/cx0-e2.c:9,44` 引用已迁走的 `../exec/c/`;须先查调用与批次再决定退役,不自动删。 入口:`tools/refactor.cx sites`

### (5) 后续版本与评审要点
- 0.0.91 范围:只收"身份与构建边界"(L.1 + Z5 + B3),A 类封顶 20;L.1 + Z5 + B3 不超过 17 文件时才并入 litlen 注解补丁(3 个 A 类文件)。 依据:开版拍板 入口:`archive/plans-pre094/v0.0.91.md`
- 0.0.92 至 0.0.94 主题:两处口径不一致,见 report-C;以 `build/version-seq.tsv` 为准,待核。 入口:`build/version-seq.tsv`
- R69.6 零断言守卫:第一阶段 `run-batch`/`allsession` 影子计数 `assertions=N` 已落 `22b2c76688`;第二阶段路线定为 0.0.92 十批试点(`cdx` 主刀)、0.0.93 迁移与首轮影子、0.0.94 继续影子、最早 0.0.96 满足总量门 268 加分层数据门槛后对新批强制;豁免须写可核查条件,未报存量只降不升。 依据:守门人评审 入口:`tests/run-batch.cx`、`research/2026-11-22/zero-assert/phase2-design-v2.md`
- R69.6 计数自保:`assertions=N` 须在共享断言库自增,不许手写常量;变异须覆盖"删总结字段必红""删自增必红";流式计数须对任意切分点不变;区分"只报失败"与"真零断言",存量分阶段迁。 入口:`build/owner-reqs.tsv` R69、`research/2026-11-22/r69-6-shadow/report.md`
- R78 存活变异分类:分支不可达(`UNREACHABLE_BRANCH`)、夹具无法区分(`EQUIV_OR_DEGENERATE_FIXTURE`)、弱检查(`WEAK_CHECK`)三类分报。 依据:ccogw 经验 入口:`tools/mutation-gate.cx`
- R79 释放后使用粗检:工具已落(`tools/uaf-census.cx`/`check-uaf-census.cx`,报告模式不进 `chain-ready`);历史回测在范围 5/5、全样本 5/9;漏报面(字段/跨函数/非.cx/OOB/litlen)已写明;基线只降不升。 入口:`research/2026-11-22/uaf-census-retro/report.md`、`build/owner-reqs.tsv` R79、`plans/memory-safety.md`
- R81 断言质量:阈值断言须能说出哪种可达状态会让它变红;关键门(`release-chain`、`chain-ready`、`precommit`)自测须是行为探针,不是文本存在性。 依据:R81 盘点(27 处疑似永不失败) 入口:`build/owner-reqs.tsv` R81、`research/2026-11-22/r81-census/report.md`
- 待定项治理:台账与本文件中每个待定条目须有结论,或下一动作加到期版本;缺者列清单,存量只降不升。待核扫描工具是否已落。 入口:`build/owner-reqs.tsv`
