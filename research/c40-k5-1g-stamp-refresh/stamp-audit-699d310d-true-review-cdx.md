# exec 戳差集独立语义真审（cdx；04:44 授权）

基准 fbb0d350f6848b398159da3813eb4e22222faf95；审阅锁 699d310d7b983123370142d90080c57c5b457c8f。依据 stamp-audit-699d310d.md（含追加更正/收窄）、exec-delta.diff 与锁定树源码。作者 cc；本审为 cdx 内容独审。未跑门、生成器或构造；未改仓、gatedeps 或审核戳。699d 之后的新 exec 变化不在本审。

## §5 第1–7项

- [x] **1 差集一致性。** 完整 diff、git name-status/summary 与两份成员映射均为四个 M：chain.sh、pp/gen-delta.sh、pp/run.sh、pipeline/prepare.sh，另一个 A：prune/gen-delta.sh。无删除，四个旧文件 mode 未变；新 helper 100755。手抄 SHA 更正全值已核。两个只读 worktree HEAD 固定且净。
- [x] **2 chain.sh 语义。** 无 flag 的 pp 构造改为真实 helper，显式私有 `$T/.seed-gen-cache`；stdout/stderr分文件，原有 `|| ... exit 1` 保留，失败不会继续 lex。T由mktemp和EXIT清理，后续tbl/net/逐输入判据未改。无静默Python回退。仓内 compilercheck 固定转调链未发现此 shell 消费者，静态范围见下，非任意外部程序全称排除。
- [x] **3 pp helper 仅注释。** 完整 diff 只有头注释，剔注释后逻辑不变；参数、互斥与 cache 语义未在本区间改变。头部“run.sh still calls gen.py”已过时，是注释卫生欠账，不影响运行语义、不据此宣称消费者现状。
- [x] **4 pp/run.sh 语义。** 两个前置 fresh 各失败立即return1；gen分支明确exit1，避免尾case覆盖。d.json经helper并保存真实rc，单次私有DIR在命令内创建，随机路径不进入fresh文本；环境标记作sh -c的$0，仅参与命令戳，真实环境由子进程继承。helper与seed/*.c/*.h进入输入，配置/头文件变化可使d重建。未改变ex/corpus判据。沿stamp.sh读了eval转调与输入/命令成戳，不能泛称完整工具身份或并发安全；既有受控同路径CTL、NEG与单冷观察限定支持本变更。固定compilercheck链不经此消费者，见下。
- [x] **5 prepare 两笔接线。** 分别审 opt --o2 与无flag prune 替换；set -eu/bound失败传播保留，产物名和后续stage次序不变。opt→prune→pp显式同一OUT私有DIR，models以键锁串行且临时工作目录隔离。受控 opt/prune/pp 三个返回后快照、真实helper rc与编译1证实单次复用；冷最终成员1另列，不替代受控证明。STAGE_MODELS_READY旁路或valid缓存命中可不走prepare。
- [x] **6 新 prune helper 单列真审。** 严格恰一个OUT，所有额外flag拒2；SEED_GEN仅0/1（unset默认1、空串拒2）。显式BIN不可执行拒2，可执行但失败直接传播；不回退。计算键时Python失败拒2，version非零拒；源码与CC字节/version/flags参与键，binary仅在sidecar SHA一致时复用；构建失败清临时文件并拒1，最终exec bound的prune。把仅诊断路径归一后，opt/pp/prune的CC/键/构建/cache代码块逐字相同（SHA d6e15144d31077181f3f3f81e8117aa3d9bf242f077538754ae45ed2eb7fa8fe），没有意外分叉。prune专属NEG opt0之后仅prune3/2、pp未启动；参数/SG0/变异证据分列。
- [x] **7 已知限定接受于本次窄真审。** 并发、完整cc1/as/ld身份、64位截短键碰撞/核键到exec实体守恒未证明；cache binary与sidecar分次安装不作跨调用并发保证。pp/run的fresh绑定配置字符串/解析路径而非所有工具字节。中断清理/损坏cache等旧未证项不补绿；schema是K5-1g独立原件，但未覆盖全部动态读集。冷closure成员未打印、cmp rc未单独落文件、包装tail rc1、提速未证都照列。仅接受审过差集的语义与此前具名限定读数，不称完整产品、全部套件或K2b准入绿。

## 间接调用的静态边界（不是只grep文件名）

1. 从exec/c/compilercheck.sh完整命令序列展开：source tests/lib.sh→ua_ready→build_ref.sh→export_ref.sh（awk递归源码include、cc构建）；watchdog tests/bound/bound.py只执行传入argv。该链不选择chain.sh或pp/run.sh。
2. compilercheck→exec/pipeline/elf.sh：TARGET只在六目标case中选择编码器/IMAGE；固定models.py入口由STAGE_MODELS_READY控制。models.prepare只在无valid缓存时用明确argv调用prepare.sh（cwd=ROOT、check=True）；prepare明确调用三个helper与gen.py/tbl/net/pack，本区间未引入动态shell选路径。故prepare及pp/prune helper是条件构造执行依赖。
3. 后续compilerpack.py的model_jobs/_construct选择固定gen.py、tbl.py、net.py，readtrace只包该Python执行；closure哈希读文件并不是执行文件。asm/cc.sh按ISA选择汇编并exec cc，不调用chain/run。unisa ape --via只派UA加文件/目标argv，compilercheck.py的动态driver列表是私有构建的driver/明确UA/run，资源与测试输入按命令显式传入；未发现仓内转调到chain/run的shell分派。
4. 因此对**仓内固定脚本及受控工具契约**确认chain/run没有这条构造执行调用，但两文件仍进入保守exec输入清单，仍应审。用户可指定UA、CC/EXEC_CC或PATH外部实体、进程可执行任意逻辑；本静态审不证明任意外部程序/环境下绝无间接调用，也不恢复“精确闭包”全称结论。

## 候选戳只读复算

本次另以只读Python按gatequeue的git可见成员、后缀/排除规则、普通文件检查、mode/内容SHA映射及json.dumps(sort_keys=True) SHA重算，命令直接退出0（工具返回exit_code=0；没有跑门或写JSON产物）。
- fbb0d350：exec 1410，2d52b9bc856caf12e939c40e29a860fb5a236e71415abfb173257b02971aea27，等记录。
- 699d310d：exec 1411，**e9d2314a3dcba3bc378075217f6c1819550e81fb32f7f44313c79bd461760c34**；include/kernel/src/unisa/weights五目录均等记录。
- cc原复算命令的rc未单独落盘仍UNKNOWN；本次只读复算不追改原账。

这是候选值记录，不是写 reviewed_trees.exec、不是刷戳。实际写键、跑门和之后新差集须另授。本审不代替机房主任处置裁定。

PASS 可进刷戳裁
