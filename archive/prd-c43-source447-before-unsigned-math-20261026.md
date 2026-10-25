# Source447 evidence snapshot before unsigned table-math regeneration

The following PRD sections are copied verbatim. Their original relative links
are interpreted from the repository root. They preserve historical candidate
facts and limits; they do not qualify Source329 or a new product.

### 思维树

**目标：开发unisacc.com 0.0.43直到完成。** 尽可能用unisacc.com＋.cx持续迭代，不手写include，学习Rust的模块化、所有权及内存安全，有合理进展及时提交Git。发布仍受既有授权边界约束，不把局部smoke当开发完成。

```text
0.0.43：完成开发、重建自身迭代体系与新验收（发布须另授）
├─ 当前主刀：P53/S447的真实P3四CASE各12/0，ELF退出0/0/7/7；宿主CC/C HOST→首代→二代→三代已实际构建，三代完整产品均与根同字节，新链41/0、八拒绝71/0。当前清单109覆盖/94批已实际重解析，19完整缺口，未收口
├─ R0 已归档：2529个旧测试路径原blob/模式完整保留；构造支持移tools，新旧入口分离；原生容器/驱动重建已实跑，构造依赖迁移仍待补
├─ R1 已有首批：新根产品smoke16断言0；旧Pa7e8的cx/owned551及私有状态拒绝仅历史证据，完整覆盖仍NOT_READY
├─ R2 旧启动器部分已证：七流原模型/74资源提取不重构；40断言0含constructor真管道/产品镜像同字节/返回7；完整独立合同仍待补
├─ R3 旧Pa7e8本机已证：18网8770键/22694真实infer、28868域边界拒绝及恢复、72适配拒绝、三反空跑控制0；非完整产品/跨平台资格
├─ R4 部分已证：当前P53 RI30/15/8/17、C消费者45/capture18+7fatal、snapshot265/87fault与材料控制70+68均0；installed CLI集成/Windows/完整RI预算仍待补
├─ R5 部分已证：当前P53的11/12原生固定点58及smoke16均0；有限属性100及优化native241/run193/tape16已在当前P53重新实跑0；完整属性/优化/三阶段/外宿主待补
├─ R6 部分已证：新根库/模块ABI55断言0；宿主FFI/provider/泄漏UAF/外宿主仍待补
└─ R7 待完成：新清单全覆盖 → 同候选跨平台CI → 封存与授权发布
```

**当前产品身份：** 已装根P53e2cd1f669c92fda2c16dd93a9021cd4db97f05c57ab2d1acf9a835cbac9857（4911193B），源S447f794aeb8683492d73c520c163b45bac1d62e88dea2435b682a09772736bce，源提交589db6c39f；后续测试/文档提交不刷新二进制。11代由旧根P46构建并安装，12代由当前根独立完整重建，产品均同字节。旧builder及其记录保存在out/c43-native-kernel-11/bootstrap-before-install.com*。模型包保持1c6a503090ee8d4189d6326139fa4a18b5db14b9682f1db0949e8ee79519db3e；package-refresh双向核1241项，唯core.c精确空copy guard经corecompat批准且当前hash仍列入清单。

**当前已实跑局部证据：** 新候选smoke16、RI四批30/15/8/17；新根capture18正例+7fatal（16父检查）、consumer45+strict-missing2（10父检查）、分离NN链17（含2 PRECONDITION）、native材料70及NN材料68控制、snapshot265/87分配故障点、库ABI55、corecompat16、plan parser30，均0。原30秒resources、原55秒各独立批次及2/3/8/18秒子限时保留。相应日志out/c43-ri-*-candidate-11.log、out/c43-ri-*-installed-11.log、out/c43-ri-*-control-*-11.log、out/c43-{snapshots,library,core-compat,plan}-installed-11.log。C源适配仍是C基础实现，不冒称CX重写；synthetic RES/两个小NN不替代真实P3消费者或通用内存安全证明。

**当前自举边界：** out/c43-native-selfhost-12.log原30秒内58/0：11/12两架构各11份.s/.o、两kernel、五切片及完整APE逐字节相等；前后来源/1241材料/全部收据均核。只在单次校验内保留只读artifact字节，after另建context物理重读，不缓存PASS。旧Sb9历史源和原7f基线源未重演；unsigned receipts仅证一致性，product-context-v1未绑builder，不能冒充host-seed三阶段N22/外宿主。

**清单与未完成项：** out/c43-all-pp-source-installed-11.log原20秒实际rc3，smoke16/0；109覆盖项、94批、90已实现覆盖、19完整缺口。该聚合只执行smoke，93非smoke批/74已实现覆盖本次仍NOT_RUN，独立日志不自动转授为聚合PASS。plan parser30/0（out/c43-plan-pp-source-01.log）；当前P53有限uint32属性100/0（out/c43-properties-installed-11.log），只覆盖六word/五rotate/四loop边界及O0/O2。当前P53优化三批原55秒各实跑241/193/16均0（out/c43-optimization{,-run,-tape}-installed-11.log），六flag/两输入/14个定义C用例、in-memory run、Tape默认别名与非法flag拒绝；不转授完整优化/外宿主资格。完整RI、语料/拒绝、属性/资源/库、host-seed源重演与同候选六宿主仍待完成。旧1333队列保持停止，原失败与UNKNOWN不改PASS。

**真实P3消费者证据：** 已复用当前P53完整P3包，未重构权重/另写decoder；独立source-only prepare→默认trimming compile（COMPILED pending）→seal（PREPARED）均在原限时内完成。通用fno-trim两次20秒超时及原单步profile55秒超时保留，未放宽；拆步骤并由父直接module verify避免多层默认-run，当前flow-native=dec8638a3d79ae434a4f0f30af83570bd413bf912d96f4a2f80db671962dd990/232165B。独立DATA prepare20秒内0，只记PREPARED；四个实际P3模型CASE各55秒内12父检查/0，out/c43-ri-flow-{e3-empty,e3-absent,e3-missing,e3-to-lower}-01.log，前三fixture各8、lower9。E3实际SC2 fetch区分EMPTY存在len0、ABSENT不存在、MISSING回退到真实13B属性，实际ELF退出0/0/7；保留tapecheck的E3→lower链最终退出7。snapshot input是真E3 tokens，lower capture input是真prune后输入且SHA不同；lower attrs-fetch实际0，不假称lower消费属性。C library-guard source-adapter不是installed CLI集成，也不是完整RI/外宿主资格。新profile九CASE原55/18秒内133检查0（out/c43-ri-flow-profile-control-*-03.log），未封存/ELF字节/pending或final收据/闭包字段/旧v1/重复compile或seal逐项具名拒绝。控制器使用-O2且重执行实际父进程匿名映像，子完整SHA核验，不缓存测试结果；原O0 elf-flip02前置rc142保留，不算拒绝成功。DATA目录inventory两CASE各15/0，新增空目录/删除登记空目录均拒绝且恢复可验证，未执行NN。完整RI义务不减少。

**真实对象与链接合同：** P53/S447下tools/object-material.cx独立prepare55与verify20均0（MAT02，只PREPARED/VERIFIED），三个header-free unit-object来自实际产品。verify含前后完整source观察及末receipt复读，不称原子快照/ABA检测。tests/objects.cx九CASE各原55秒实际198/0（out/c43-objects-*-01.log）：三个ELF64LE ET_REL/机器与节表bounds、真实x64链接退出7、跨目标/缺定义/重复定义/旧unit版本拒绝，四个非Linux对象目标逐项具名拒绝；每个拒绝后实际恢复链接并退出7。ARM对象只验格式，未称Mach-O/COFF对象或系统链接器互通/外宿主。六目标镜像格式另已当前P53重跑14/0（out/c43-formats-installed-11.log），仅本机ELF执行。

**宿主来源链进展：** CTX02绑定工具293e069e71de8bdc8b27a62a70dbb8d452cdd47e9d192334840716f8c7080b1f，ident/C HOST及三代各五切片+wrap均实际成功，18步骤各原55秒。out/c43-hostseed-chain-03.log原55秒41/0：前后核完整来源/收据，逐字节比较二/三代六产物及第一/三代完整产品与根，三代均P53；29文件及models/payload不变。父准入最多64代canonical路径判环，从HOST正向重建完整stage/六产物收据，双读与source/context/HOST/models/payload复核后才首次写入；执行后再读。out/c43-hostseed-control-*-03.log八拒绝71/0，包含私有父wrap收据builder定点改零的具名拒绝，新代未创建、原材料和私有变异材料未覆盖，控制器手工-O2、原55/target20/两source8。显式构建子40+三source子6的上界58仍大于外55；实际18步成功不保证最坏64代耗时，循环/深度守卫未专测。NN/P3与ASM kernel仍是可信二进制输入；初始S测量依赖当前根，后续native helper测S；宿主CC子工具/系统库未独立哈希、收据未签名，不称完整源码N22/外宿主/历史构造证明或原子快照。旧CTX01因工具SHA改变失效，其40/60只保留为旧工具快照。CC maybe-uninitialized告警只读未发现可达未初始化读；CLI进程退出回收边界仍不外推可复用library生命周期。

**pp局部权重源码构造：** 独立CX六cube规则直接生成142B mask+sparse W2，未从旧权重数值派生；MAT01 prepare55/verify20实际0，仅PREPARED/VERIFIED。部署C infer数学体精确取ckernel.py的KERNEL_BODY，未执行Python。out/c43-pp-constructed-01.log原55秒92/0：18键/72原始logit、22字段拒绝及恢复、4适配拒绝及恢复、私有有效W2从1到2的实际score敏感性。baseline固定配方逐字段纯读校验，verify不重构weights；out/c43-pp-gold-controls-01.log原55秒23/0：私有clone先实际verify通过，再将有效W2改2并同步unsigned收据SHA，仍精确拒绝固定baseline配方；不把有效NN权重称结构非法或借receipt拒绝充实际infer敏感性。baseline weights SHA=2fd7181593523ce306cbcf0825b2d6ebd5dde3f69cf17762e8beeb6ebf25e609，142B；math-body SHA=463b02ddd20c31182cbf90458b7cf308992f903b931bbdd8d355d38edee52eb2。Python文件仍是C源码文本依赖，完整SID会读旧weights、启动产品仍受信任；只证pp新参数值的局部独立性，不称全部18网/P3从源重建、与旧weights逐字节相同或全链Python独立。

**下一动作：** 保持P53/S447产品输入冻结，分批提交已实际验证的host41/八拒绝71、objects198与pp新构造92/配方负控23；补来源链cycle/65深度的独立synthetic拒绝探针（未实跑，不假称真实代），再按依赖推进剩余网/流合同。继续补19完整缺口、全部NN/ASM源重演与同候选六宿主。C44-RUN-REUSE-01加速仍未实现：原端到端首2.009s/重复中位2.027s；独立同仪器对照已12/0，guarded noop=28015979ns、最小CX=1932617292ns（out/c43-run-watchdog-bench-01/samples.tsv），不支持固定2s watchdog退出等待假设。两组含watchdog/trace，内部startup/编译/装载/推理相位UNMEASURED，不扣对照耗时或宣称提速/缓存。未签名/Draft/Latest/公开。

原红、历史候选、试验和阶段记录已原样移至[本拍历史记录](archive/prd-c43-pre-p3-host-lineage-20261026.md)，不继续冒充当前状态。

### 记忆宫殿

| 房间 | 当前事实／决定 | 证据与行动入口 |
|---|---|---|
| 产品室：新开发产品／旧基线分列 | 根P53e2cd1f/S447f794a，4911193B；11代安装、12代由新根重建同字节。旧P46及记录保存在11代。 | 当前RI/源适配/控制/snapshot/库/固定点见上；完整19缺口与同候选平台未收口。 |
| 十八网新合同室 | 旧Pa7e8默认-run一次进程执行18网/8770键/22694个key-head整数infer，28868字段边界拒绝及等量恢复、72适配拒绝；gold标签/唯一最大logit/词表映射均核；改gold、改模型及恢复三负控0。 | `out/c43-net-all-02.log`、`out/c43-net-controls-01.log`；tools/net-material.cx源派生+只读verify，未重构权重，未借DENSE或旧PASS。只证本机经典整数核合同，不外推P3每路线或六宿主。 |
| pp权重构造室 | 新142B六cube由CX规则构造，真实C infer 18键/72logit/22字段及4适配拒绝与恢复/有效W2敏感性，92/0。 | out/c43-pp-constructed-01.log92/0、out/c43-pp-gold-controls-01.log23/0；只证局部新参数值来源独立，Python作为C文本依赖/完整SID旧weights/受信任bootstrap仍在，P3未替换。 |
| 原生汇编室 | 新CX复用产品-E处理.S并展开宏；现存C编码器由有来源材料适配默认-run执行，189断言0；新ELF链接器16断言0。03代两架构各11对象已原生构建及链接为x86 7144B／ARM 7096B UNIKERN1，均已进入新根产品。 | `tools/kernels.md`、out/c43-native-kernel-execute-e4-01.*、out/c43-native-product-case0.*；x86真实E4同字节，完整编译ELF同字节且返回7。ARM真实执行及完整产品资格仍待验；PREPARED和功能分列。 |
| 七流新合同室 | 原生stream-pack曾从旧P7裁七模型/74资源，共40断言0，含两源码链、真管道、镜像和具名拒绝；不转授新候选完整合同。 | out/c43-stream-contracts-03.log、tools/streams.md；当前根产品另已重建，完整RI/拒绝域/六目标仍待补。 |
| CX所有权室 | 旧Pa7e8上的cx/owned使用真实模块隐私、固定4×8 i64存储和不回绕代际句柄；551断言0，外部直接访问data/slot各拒绝1。复制活句柄仍可别名，不是编译期所有权或通用堆。 | `make owned`、`cx/check_owned.cx`；只验证本机运行期设施，不代替七流十八网或完整库验收。 |
| 旧套件封存室 | 旧1333队列保持停止，原F4/UNKNOWN不改PASS。当前冻结新表109覆盖/94批，19完整缺口保留。 | out/c43-all-pp-source-installed-11.log原20秒rc3，smoke16/0，93非smoke批/74已实现覆盖本次聚合NOT_RUN；不缓存PASS。 |
| ujs 子项目 | 独立规格与进度只在 [`ujs/prd.md`](ujs/prd.md)（及 `ujs/ARCHITECTURE.md`）；根宫殿不重述 ujs.com 代际／语料。跨项目只记：底座为 `unisacc.com`+`.cx`，不 tip／Draft 除非另授。 | [`ujs/prd.md`](ujs/prd.md) |
| 优化合同室 | 有限uint32属性已在当前P53重跑100/0，6word×5rot/4loop、literal gold、O0/O2各一次。 | tests/properties.cx、out/c43-properties-installed-11.log；当前P53另已重跑优化native241/run193/tape16均0，out/c43-optimization{,-run,-tape}-installed-11.log；完整属性/优化仍NOT_READY。 |
| 新自举室 | 当前P53/S447的11/12原生两代固定点58/0，原30秒；当前根与12完整产品同字节，前后全核源/1241材料/收据。 | out/c43-native-selfhost-12.log；只复用单次校验只读字节，after物理重读；旧源未重演/无签名/v1缺builder限制保留。另有固定P3宿主HOST→三代实际构建均P53、当前CTX02新链41/0和八拒绝71/0，canonical父准入已本机实际验证；循环/深度未专测。NN/ASM可信输入、初始根辅助S测量及宿主依赖限制保留，非完整源码N22或外宿主。 |
| 资源与RI室 | P53实际P3四CASE各12/0，真实E3三态与E3→lower图像退出0/0/7/7，链路保留tapecheck、分别绑定两端真实输入；lower attrsfetch0不claim消费。此前RI/源适配/控制/snapshot局部证据保留。 | out/c43-ri-flow-*-01.log；source-adapter非installedCLI或完整RI，profile九项133/0、目录两项30/0（03/01日志，手工-O2控制器、原限时），外宿主/全预算NOT_READY。 |
| 资格与发布室 | 当前P53/S447有本机具名smoke/RI/源适配/11-12固定点；完整19缺口、完整NN/ASM源重演及外宿主仍待验。 | 未签名/Draft/Latest/公开；旧CI不转授，C44-RUN-REUSE-01加速仍未实现；当前e2e首2.009s/重复中位2.027s，9/0；OS-cold/内部相位未测，不报启动提速。 |

旧M7/固定HEAD、旧CI/宿主交接、窗口计数与准备过程已移至[历史记忆快照](archive/prd-c43-pre-native-qualification-20261025.md)。本表只保留当前目标、能力、缺口和入口；原始身份、红/UNKNOWN与回退均未删除。

