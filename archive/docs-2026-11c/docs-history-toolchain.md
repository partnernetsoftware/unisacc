# docs/toolchain.md 迁出的历史记录

来源文档：docs/toolchain.md。迁移日期：2026-11-13。
说明：这是历史记录，不是当前指引。逐字保留，标题后括号为原行号。

## 0.0.57 链恢复补丁合同 (toolchain.md:115-126)


- 原补丁基线为 `60acc817b86b0605016f38604ec0dbea5fac4d8d`；本次重基到 `67fb47417b7c053217a1aa1d307eb9ab9a5bef9b`，保留 A2p 动态分片、chainfacts 及全部既有断言。唯一源码冲突为 chain-step 的 use 追加，保留 chainfacts 与 sys.posix 两者。现场 MANUAL.tsv、chain-run.tsv、chain-run-16.stderr 只读，不重跑失败确认。55 秒上限不改。
- snap 不再嵌套冷编 release-build：直接 git archive 到有 root/rev/product 身份侧收据的 `.partial`，持有 flock；只清理本身份未完成归档。完整 marker 必须精确匹配40位rev；旧无owner半成品具名拒绝，须操作者隔离，不猜测归属。归档完成后可接续产品拷贝，最后 rename 发布。它不自动接管旧现场半成品。
- chain-run 在创建预算/锁/执行子进程之前拒绝 repo/out 之外的 OUT 和越界软链，不改 cx_rmtree 全局白名单。
- 基线 wave_phase 只在尚无 start 检查点时清理该作业，跳过 start 时不清理；finish 继续由 controller 校验身份，最终 canonical union 仍重新验证。不得以 checkpoint 缓存资格结论。
- 最窄原生candidate夹具按`--jobs ... tool --job-index 0 --resume --until jobs-tool-0-start`后单独`--until jobs-tool-0-finish`执行；trace实测start只一次，篡改start产物身份时finish非零，恢复身份后finish通过。60acc基线这条调用路径未发生清掉start重做，因此不额外修改candidate-chain现有清理逻辑；现场失败由仓外OUT前置拒绝避免。

- build 54993ms 是 controller `-O2 source-package.cx` 实编，ctx 51123ms 还会默认冷编同一 source-package 后执行 prepare-context。建议下一集成窗口将 ctx 调用改为已编 controller 的 prepare-context，保持 argv/root/保存bootstrap身份；这需要 controller 行为等价夹具，不在本补丁偷偷改产品构造。build 自身若仍贴界，应拆 controller 前端/对象/链接并保存输入身份收据，禁止55秒无进展盲重试。当前未测各冷编子段，不宣称已获得余量。
- 旧完整快照只有 `.release-rev`、没有新 `.chain-snap` 时安全拒绝复用，不删除、不写它；集成者可先保全目录再另建，不自动升级现场。新完整收据在发布rename之前原子写入，复用重验root/rev和launcher/build-json实际SHA；原子rename不等同断电fsync保证，半截marker一律拒绝，不当完成。
- 具体拆分建议(未实测)：`B/source-package-O2.elf prepare-context S B/Root41-bootstrap.com CC CTX` 替换ctx二次源码启动；prepare收据绑定完整rev、S绝对root、bootstrap/controller/cc SHA和argv，完成后原子写入，续跑重验后接纳。build进一步可拆`controller-front`、`controller-objects`、`controller-link`，分别绑定源码闭包与前一相SHA；这些是待实现方案，不是本次已通过的构造路径。


## 检查准备节中的 REG-PREP 重基补丁 (toolchain.md:147)

REG-PREP 重基补丁独立基为 `67fb47417b7c053217a1aa1d307eb9ab9a5bef9b`。该基线已包含 A2p 的 `cxlib.chainfacts` 动态阶段计数、事实漂移负例及 phasesgen 独立预编译；重基保留这些实现与登记/调用参数，不再迁入旧475/485或按表行数反推预期的实现。原始基线日志仅作历史记录，不代表本基线执行结果。

## K10 节中的工具名查重与交付基 (toolchain.md:160,162)

新增工具名已在完整git历史查重：`tools/cxlib/chainrunproc.cx`、`tools/check-chainrunproc.cx`此前均无历史；实际编译库存 `tests/tools-compile-sources.tsv`、行为批次与覆盖登记同步新增，20261110库存是历史快照不回写成当日盘点。真链、实际controller冷编耗时、跨宿主以及产品资格均未实跑；轻假链只证明预算/身份/恢复边界，不证明真实每相余量。

本次最终交付基为 `1e269c53bbbb7cd9e9819f808db604fd8aac17b8`，其后只叠完整替代包 `c57-g2batch-r4.patch` SHA256 `5a6a75aeab09667d0bbd72e4a7c3f3ec889a705fc09bf0ecfd5c36bfa4a7e2f3`，不重复叠旧R3/R2。K10保留生产语义与G2身份恢复；十项cx与阶段表对照已验R2树同字节。依定稿验收，本次在新父树真实 `tests/run-batch.cx` 新跑chainresume(13项,daily)、chainrunproc(7项,commit)、chain-run(29项,commit)，均输出BATCH_RUN status=PASS、rc=0、transport=0且精确marker匹配；准备仅PREPARED。修复重导表尾重复chainresume/coverage登记，保留主线CHECK_CHAINRESUME合同，不删断言、不改tier/55秒。另新跑生成--check及fresh apply检查；账本独立TSV，不入patch，不跑真链/广门。

## K9 定稿T3依赖与批次合同 (toolchain.md:164-174)

494相七列表保留deps、lane与事实并行度，六共享prepare先行；Linux对象/闭包/链接各55秒。目标最多4路，parallel1完整覆盖；独立fork owner与pidfd/starttime保护兄弟，失败停止派发并收齐在途，超时仅持久降载。prep逐调用锁及Mac stub/B锁保留，短命/proc终态竞态最小修正保留，G2 guard协议不倒退。

K9-r3重导在1e269c53+完整G2-r4+K10-r3之上，保全K10去重的chainresume批次及CHECK_CHAINRESUME合同。既有chain-run、chain-phases-gen、chainrunproc及新增candidate-chain-lanes登记均按 `tests/run-batch.cx` 真实新跑，55秒不变；候选原生映像按材料声明准备，PREPARED不是结果。该轮精确结果记独立ledger-r3。未跑真链、产品构造、广门或性能目标；不授产品资格。

K9-r3四批实际输出BATCH_RUN status=PASS、rc=0、transport=0，精确marker分别CHECK_CHAIN_RUN(46)、LANE_SPLIT(31)、CHECK_CHAIN_PHASES_GEN(10)、CHECK_CHAINRUNPROC(7)。新增lane专批依赖原生工具材料，按既有材料合同登记daily，不降档任何既有commit项；首次预检拒绝及lane输出父目录未创建的失败日志保留，后者仅补夹具创建自己的OUT后重新实跑通过，不删断言或放宽预算。

K9-r4基为origin/main `02057cd64887801e8d315610a1da90efb77db020`，已含G2-r4及K10-r3，不重叠依赖。门报新增截断调用改为检查tx_copy返回值；lane夹具不再复制snapshot布局，改消费候选只读`--chain-work ROOT REV`查询，该查询与真实执行共用chain_paths推导，不拼拆路径字面绕门。所有31项断言保留。定向cxtrunc对三文件0调用、outdep对candidate及lane两批0输出字面，基线未改；46/31/10/7四组在该树与当前已装产品真实run-batch新跑PASS。证据保留本克隆out/k9-r4，不经仓外中转；未跑真链、广门或性能目标。

K9-r5仍基`02057cd64887801e8d315610a1da90efb77db020`，完整保留K9-r4行为，只迁入AnchorBan58已验证的`tools/check-graph-check.cx`结构变异组件。该组件复用既有refactor find调用/参数事实，以generation=1参数筛选两个Win span，不再假设所有wf_win_span字面调用都属于gen1；refactor、chainargv及生产文件不随组件迁移。原route-real必须0、Win-as-Mac必须拒绝且route_mismatch=8与指定stub/units诊断均不放宽。新树真实graph-check批次29/0且BATCH_RUN PASS；全量tools/ratchets.cx实际13/0（28511ms），未改任何棘轮基线。证据在本克隆out/k9-r5，未跑真链或额外广验。
