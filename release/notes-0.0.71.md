# unisacc 0.0.71 发布说明草案

冻结输入为0571f88105e5da96335ae4a7add1d9e605d40d7c。正规构造499/499相位、pack与pack-recheck通过,退出0;promote检查由主调度实跑通过。候选SHA256 ca84adc0fb36748a7194f5830791f20f1998ff5e6816eab4db55b4f9d7c7c24a,7228055字节,与旧候选逐字节相同。新来源SID a87d1ea260de48edbddb688de4a39ad2a66e59815d04d33cc5d017cb751a75c4。正式重建配对build记录、核验资产并经release-layout生成本地布局,cx-root-identity实跑13条通过。来源身份与产物字节身份分别核验;同字节不代替来源记录。仍不宣称QUALIFIED。

多单元函数指针形参U17已由候选整批13项(含6个跨单元正例)实跑通过。定义先于原型探针修为首单元包含main,仍保留定义/原型顺序;同探针旧0.0.67拒绝,新候选输出X 5。入口位置未被误当类型合同,原失败收据保留。

## 改动与旧候选证据

本版包含产品速度改进、性质预算拒绝与充足预算等价探针、pins锚修复、材料夹具输出域修复及拒绝清点规范化。literal改为事实表LOCAL_FOLD_LINES,独立生成网与九个编译产物共15对逐字节相同,check-literal为13,未抬14基线。notcovered保持267条拒绝与33条已测状态,修复完整token去重和枚举顺序漂移,12条检查通过;234条未测仍未测。

以下仅绑定旧输入2044f903fd25b51d0e526a5de2f7e0dd27e054d9与旧候选SHA256 ca84adc0fb36748a7194f5830791f20f1998ff5e6816eab4db55b4f9d7c7c24a,7228055字节:正规构造499/499,pack、compare、promote检查与smoke通过。无缓存时样例O0/O1最小成功单阶段预算5684701,O2为14297501;充足预算28595002输出SHA与无预算变量相同。O2预算拒绝位于e4,这是单阶段预算边界而非总步数,未量化对旧版回退。

旧全套保留NOT_QUALIFIED:1275个已实现批次,782 PASS、204 FAIL、289 UNKNOWN,另41不适用,missing=0;覆盖1079,satisfied710、unsatisfied348、unresolved12、declared9。与前轮对照40修复、161回退、3新增、1100相同。153项资源症状加5项备料环境共158个ID已独立低载复验取得通过证据;原收据并列保留,不回写全套PASS。原因分类ASSERT50、INNER_TIMEOUT30、OUTER_KILL187、NO_MATERIAL6、UNCLASSIFIED220,其中185批无收据;50项断言失败不能归为负载假失败。

## 已知缺口与未验证范围

(1) core-compat:参考执行器cacb65b83e新增P4解析与SBLOAD未经独立核验,旧helper仅批准空copy guard转换;不扩大批准域,豁免不算通过。状态U20。

(2) tlib-frozen:两轮旧聚合均FAIL,cc核过HEAD同样失败,作为既有失败豁免,不算通过。独立定位留下一版。状态U24。

(3) interfaces-lnx-x86_64:两轮均FAIL,低载复现;setreg rdx imm0 role=arg2仅出现一次的旧断言假设被新生成边破坏,另有旧reject文案变化。保留原断言与失败,下一版迁移oracle并独立核验;豁免不算通过。状态U25。

(4) stages:五项字节差异尚未完成独立金样核验,原断言与失败保留,下一版补独立oracle;豁免不算通过。状态U23。

(5) 材料族无生产者:旧材料准备中45个批次有明确REFUSED=no-producer收据,未备材料不算功能通过;按正式材料生产者补齐后复验。状态U22。

(6) full-domain及同候选跨平台资格缺外部主机实跑证据;豁免不算通过。状态U22。

入口衔接已由423c274c42修复并进入冻结输入,新候选tests/run.cx --all分片端到端验收仍待实跑,状态U21。其余未验证事项以build/state-unverified.tsv为准,不宣称既有未验证项清零。

(7) properties-cases-meta:当前候选实跑24通过、2失败、2未运行。osx两目标tape与C构造产物摘要不同,win两目标tape构造拒绝ioctl lowering;主调度在0.0.67复现同输入,登记为既有缺口,不算通过。prune幂等及bound输入补齐.end后分别通过,未改断言或产品输入。状态U26。precommit-gates-reuse仍保留原FAIL,由主调度定因。

## 证据与撤回

新链证据:/tmp/unisacc-session-v71-rc2。旧链与对照:/tmp/unisacc-session-v71-full,旧全套:/tmp/unisacc-session-v71-tests/retest-54142681d2。直接集合清单与身份在新链目录direct-v2-plan.tsv和p1-summary.txt,P2指定175项已全部执行,当前173 PASS、2 FAIL、0 UNMEASURED、0 pending;原失败与复验收据并列保留。剩余properties-cases-meta为既有跨平台缺口,precommit-gates-reuse仍由主调度定因,不算通过;状态U26。临时路径不是公开可复现附件,正式发布应另附可取证材料。本窗口未建tag、未公开发布、未替换共享产品;集成者对外发布前另记资产撤回与旧版恢复办法。

## 直接集合验收口径

按主调度P2裁定:冻结输入0571f88105的374批直接集合中,tests/run-batch.cx扇出152批、tools/cxlib/chainargv.cx扇出79批,去重并集227批。S_main为不属于并集的147批;H_sample为并集按批次ID逐字节字典序排列后,从第1个起每4个取1个,共57批。合计204批必须在新候选上逐批验收或明确登记既定豁免;计划中29项为既定豁免,175项全部实跑,173通过、2失败;29项豁免仍不算通过。

剩余170个harness扇出批次记CARRIED_NOT_PASS,保留旧候选收据,不记新候选PASS。沿用依据是这两个harness的改动在第二轮全套前已存在,同一运行器已实际跑过相关批次;新候选产品输入相对旧ca84仅多事实表调整,15对生成物与九个编译产物对拍逐字节相同。网等价证据不冒充这170批在新候选上的执行结果。完整成员、分组、状态与豁免见/tmp/unisacc-session-v71-rc2/direct-v2-run.tsv及direct-v2-status.tsv;本段仍是验收口径,不是完成声明。
