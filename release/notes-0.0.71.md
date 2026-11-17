# unisacc 0.0.71 发布说明

状态:已公开发布并置 Latest(2026-11-17);公开读回(全新目录下载六资产、SHA256SUMS 五项、版本与最小编译运行)通过。整体裁决仍为 NOT_QUALIFIED:全套 s9 未过(见下文),不得称全批资格。

同候选原生 CI 通过:run 44072623262 attempt 1,检出 5493e53e4cb667931f4f20b05e650ab852f40229,CANDIDATE_QUALIFY verdict=QUALIFIED pass=42 fail=0 not_run=0 invalid=0。Windows 两格(win/x86_64、win/arm64)的 stdio-crossunit 与 stream-pipe 为 portable 子集通过,POSIX 部分(缺 fork、waitpid、kill、F_SETFD、environ)未运行,属平台既有范围。全套 s9 仍为 NOT_QUALIFIED。首轮 CI run 44071695047 attempt 1 失败,主调度已核原始日志定因为 Windows 流管道夹具的参考调用漏带 -O0,与 O0 管道比较时误用了默认 O1。夹具已由5493e53e4cb667931f4f20b05e650ab852f40229修复,断言与覆盖不变;同候选重新调度 run 44072623262 attempt 1,资格已核定通过(见上)。公开读回在公开后进行。P2 指定验收已完成,旧全套仍为 NOT_QUALIFIED;局部通过、草稿资产回读与完整发布资格分别记载。

## 真实身份

| 对象 | 身份 |
|---|---|
| 构造输入与冻结提交 F | 0571f88105e5da96335ae4a7add1d9e605d40d7c |
| 来源 SID | a87d1ea260de48edbddb688de4a39ad2a66e59815d04d33cc5d017cb751a75c4 |
| 候选 APE SHA256 | ca84adc0fb36748a7194f5830791f20f1998ff5e6816eab4db55b4f9d7c7c24a |
| 候选 APE 字节数 | 7228055 |
| 配对构造记录 SHA256 | 8c0d3c4f82b714a9e16b9c949ee66afdf58b1383ae605a3e723a39dc1a3bcaea |
| 候选表资产提交 X | 02d967568c727ea45f973a04f78f7217be222ced |
| CI run / attempt | 44072623262 / 1 |
| CI 工作流检出与输入 commit | 5493e53e4cb667931f4f20b05e650ab852f40229 |
| 发布标签与工作流 | v0.0.71 / candidate-native.yml |

CI 检出提交包含冻结之后的测试、工具与发布记录变更,不替代构造输入 F。发布资产配对记录仍绑定 F 与上述 SID。候选与旧构造的 APE 逐字节相同;来源记录已按 F 正式重建,没有沿用旧来源 SID。

## 改动与实跑结果

本版合入性能、编译覆盖与生成码改进。局部帧折叠常量改由事实表 LOCAL_FOLD_LINES 生成,独立生成网与九个编译产物共15对逐字节相同;check-literal 为13,未抬14基线。拒绝清点保持267条、33条已测状态,修复去重与枚举顺序;234条未测仍未测。

正规构造499/499相位完成,pack、pack-recheck、compare 与 promote --check 通过。候选 smoke 16条通过,当前现行15个棘轮全部通过,包含原13项。P2 指定175批逐批实跑全部通过,FAIL、UNMEASURED 与待执行均为0;原失败和复验收据并列保留。七条出口逐项证据在 /tmp/unisacc-session-v71-rc2/exit-criteria-evidence.md。

多单元函数指针形参检查13条通过,含6个跨单元正例。定义先于原型探针保留原顺序并把 main 放到首单元;同探针旧0.0.67拒绝,新候选输出 X 5。性质组 bound 为18/0/0,meta 为28/0/0;meta 使用各目标生成的同目标 tape 往返,六目标 SHA 断言、覆盖与时限不变。precommit-gates-reuse 夹具补齐新增源码所需编译分片后,原5条断言全部通过。夹具的目标上下文已对齐。

无缓存样例的最大单阶段最小成功预算:O0/O1 为5684701,O2 为14297501;充足预算28595002与不带预算变量的产物SHA相同,5M精确拒绝断言保留。这是预算边界实测,不是总步数或对旧版性能回退的量化。

## 验收范围与全套状态

374批直接集合中,S_main 为147批,H_sample 为57批,合计204批;其中29批按主调度裁定豁免,不算通过,其余175批全部实跑通过。另170个harness扇出批记 CARRIED_NOT_PASS:同产物字节的先前实跑证据、相同运行器,不计本轮PASS。完整清单与收据在 /tmp/unisacc-session-v71-rc2/direct-v2-status.tsv 和 p2-results/。

原全套保留 NOT_QUALIFIED:1275个已实现批次,782 PASS、204 FAIL、289 UNKNOWN,另41不适用,missing=0;覆盖1079,satisfied710、unsatisfied348、unresolved12、declared9。正式新旧对照为40修复、161回退、3新增、1100相同。153项资源症状加5项备料环境共158个ID已独立低载复验取得通过证据,逐项对照无漏项;原聚合与原收据不覆写。原原因分类为 ASSERT50、INNER_TIMEOUT30、OUTER_KILL187、NO_MATERIAL6、UNCLASSIFIED220,其中185批无收据;断言失败不归为负载假失败。

## 已知缺口与未验证范围

(1) core-compat:参考执行器新增P4解析与SBLOAD未经独立核验,旧helper仅批准空copy guard转换。不扩大批准域,保留失败;状态U20。

(2) tlib-frozen:两轮旧聚合均FAIL,HEAD同样复现,既有失败留待独立定位,不算通过;状态U24。

(3) interfaces-lnx-x86_64:既有接口oracle的寄存器次数假设与reject文案需要迁移并独立核验,保留断言和失败;状态U25。

(4) stages:五项字节差异尚未完成独立金样核验,保留原断言和失败;状态U23。

(5) 材料与平台:118材料族只有47项登记,无生产者材料与外部主机证据未补齐;旧准备中45个批次有明确no-producer拒绝收据。full-domain 外部主机完整实跑证据缺失,不算通过;同候选原生42格资格已由 CI run 44072623262 通过,不在此缺口内;状态U22。

(6) 全套入口的分片聚合修复已入冻结输入,新候选端到端验收仍未完成;状态U21。其余18项未验证事项以 build/state-unverified.tsv 为准,没有因局部通过清零。

## 发布证据与撤回

正规链与验收证据:/tmp/unisacc-session-v71-rc2。发布 pack 与 dry-run 重放:/tmp/unisacc-rel71-rehearsal。草稿六资产下载回读:/tmp/unisacc-rel71-dl,全部候选摘要已核对并与重放产物逐字节相同。草稿下载回读不是公开后的读回。临时路径是本机证据入口,不冒充公开可下载附件。

八步证据与机器裁决分别在 release/evidence.tsv、release/decision.txt。同候选 CI run 44072623262 attempt 1 全过(42/42),第7步 PASS;全套 s9 第4步与公开读回第8步为 UNKNOWN,整体裁决 NOT_QUALIFIED(unknown=2 fail=0)。撤回方案在 release/rollback.txt,回退 Latest 的目标已核为 v0.0.67。Y1/Y2与公开读回回执由后续实际提交及执行补填。
