# prd 历史：0.0.36 A1 至 0.0.37 发布事件记录（2026-10-08 至 10-09，自 prd.md 原样移出）

0.0.38 开工前按政委/董秘令清档：以下为逐条过程回执，原文保留以备溯源；现行结论见 prd.md「0.0.37 发布结论」段与 plans/v0.0.38.md。

〔0.0.36 A1 对象互调切片，2026-10-08〕先补产品 E3 的已声明外部函数调用：unitmode 的 UM.cc.thunk 当前具名拒绝，按参考 ccx_plain 的整数/指针、至多六参数范围构造 ABI thunk，并保留不支持签名的精确拒绝；用 Linux arm64 非 PIE 对象与 agenterm 动态库真实链接/运行验收。第二片做外部函数取址 GOT：参考当前也缺 GOT，因此参考及产品 lower/enc/对象重定位共同落地，验证 PIE 链接及地址调用，不能把 -no-pie 当作完成 GOT。第三片才接 Mach-O 对象产品入口（当前 compiler.c 明确仅支持 Linux 对象）；不因 Linux 通过宣称 macOS dylib 已支持。切片只登记设计，尚未实现；不扩 K1/K2。

〔A1a 实测，2026-10-08〕产品网络已补显式原型、至多六整数/指针参的外部调用桥；私有网络驱动生成 agenterm ELF arm64 对象与参考 84056 字节完全相同，Linux 非 PIE 链接实跑与 gcc 输出一致（去 PID）。x86_64 最小对象同字节；r21 为 48 同/5 具名拒绝，整数桥另验动作契约。外部函数取址（包括先调用后取址）仍具名拒绝，等待 GOT 独立片；浮点/变参/无原型/七参仍拒。不是正式候选 .com 验收，需 cc 构建候选补正式证据。只变 parse2 八个和对象编码两条图基线，不扩 K1/K2。

〔A1b GOT 同落方案，2026-10-08〕仅对象编码器的未定义名取址改为 GOT：ELF arm64 发 adrp+ldr、重定位311/312；x86_64 lea改mov、重定位9且addend=-4。内部代码/数据地址保持旧路径，不增指令长度；外部名非零偏移继续拒绝。先与隔离参考补丁的真实对象逐字节对拍，再协调参考与δ同批落地；E3用户函数地址放行及Mach-O对象入口另行验证。

〔N1 6.7.8p13 δ 实施，2026-10-09〕自动聚合初始化在表达式求值后，以右值结构类型沿外层结构/数组槽向内查同型子对象起点；命中整体COPYSTRUCT、游标跨该结构全部槽，未命中仍标量转换/写入。仅自动存储、非指针结构表达式；全局与块静态常量判据不改。与f27a5129参考stobjat的外层优先和布局偏移同形，验收67及嵌套/结构数组/设计符。

〔N1 标准预定义同落，2026-10-09〕E2 在CLI -D之前安装三项标准宏，默认与shared-predefines共用小清单及名称/替换串领域表；不改目标宏资源格式。__STDC__=1、__STDC_VERSION__=199901L、__STDC_HOSTED__=1，对齐research/stdc-reference.patch，-D可覆盖、-U可删除，版本宏独立body。先隔离补丁参考对拍，禁止只改名表使版本值误为1。

〔N1 register 产品覆盖，2026-10-09〕register 保留为独立 token，在块声明、for 初始声明及参数类型入口消费，不在 tokenizer 全局抹掉，避免把文件域 register 当作普通定义接受。parse2 追加词由领域表统一供 Python/C 种子读取；有效声明的 tape 与参考逐字节一致。

〔云机接班单切口，2026-10-09〕cdx-unisacc 在 /home/box/repos/unisacc、main 235ca5bd、干净工作树接班，宿主原生 Linux x86_64，未携入 m4pro 的仓根产品或 out 缓存。本轮只核验 C2 g5 typedef 翻译单元隔离的产品 δ；参考 fe_units 已有 td0/ntd 复位，产品 startup-marker 的 @unit+ 当前只递增 unit_epoch/复位 ixcount，TDN 仍按 intern 名称共享。先复现并核对名字可见性和描述符保留，再决定最小修片，不动其他切口及验收措辞。

〔0.0.37 开工，2026-10-09〕cc 解冻 main，0.0.36 队列留在29728cc6独立冻结树。cdx先做C1：66结构体返回调用必须与参考tape同形；register对象取址诊断参考与δ同落，不能只用有效声明通过就把6.7.1记covered。C2由只读子代理最小化g4/g5。F4′沿用钉住v0.0.32 csih 15单元、正式同源候选冷编≤5s及csmithdiff/difftest/com无豁免；W2沿用Windows两ISA的csih DNS+真实HTTPS和参考行为对拍；E57余半随W2结算，不另换验收。预算估算另报cc，实测阻塞即重估。

〔0.0.37 C1a 验收，2026-10-09〕4ab53c46 已含产品结构体返回调用的清单/模板与错误契约：非局部返回走已有 LI.structreturnexpr 的类型/分号检查及 WCOPY，局部对象路径不变。66与259字节结构体分支返回的完整E3 tape同参考；错误/警告模式各18完整结果同，r21为48同、5具名拒绝、0 accepted-different。C3头新增后补重导k2-gen2/k2-libraryenv/pp-autoinc-gen，facts30表0 differ；重录parse2八项与pp十三项（实际变8+12），fresh-order0..8全核对且无变化；kernel stale0，op9。仓根.com直接-run seed/gen.c产出完整parse2与Python逐字节同。正式0.0.37候选.com运行66仍需cc验收后删knownfail；return(mk())等非ID入口仍具名未覆盖，不把本片说成所有结构体返回表达式均覆盖。C1b register取址及C2仍未结。

〔0.0.37 C2 g4 验收，2026-10-09〕无原型普通直接函数超过六实参走现有参数栈反转，再按恢复的sys=0选择直接调用；内建函数超限仍具名拒绝，间接调用与默认float提升控制保持参考同tape。新增b_noproto_many探针覆盖0/6/7/8参、不同位置权重及嵌套调用，系统cc与参考实跑均8 91 140 204 204 204；完整流水线E3逐字节同。parse2八变体基线重录，fresh-order0..8无变化，facts30表0 differ，r21为48同/5具名拒绝/0接受不同；仓根.com -run seed/gen.c完整parse2同Python。正式新候选跨单元运行留给cc补验。g5用zlib1.2.12历史配置尚未复现原incomplete struct：全编先被K&R声明阻挡，描述符前缀可编，不记已修。

〔C1b register 取址实施约束，2026-10-09〕声明属性独立REGISTERBANK，作用域undo新增第43格（总44）；对象与参数声明显式捕获register，结构/函数指针嵌套保存恢复，不复用类型/联结银行。unary-&按参考14例的对象/子对象规则预检；不得直接TN.raw前瞻再只恢复位置，其可达分支会写TIX与转换临时银行。独立token视图原型仅识别id、括号、点、箭头、下标，其余整行不透明跳过，24项普通/位置v1/v2及截断检查通过；尚未接入parse2，不算14例诊断验收完成。

〔C1b 元数据首片实测〕逗号续项、普通参数、局部遮蔽、函数指针对象/参数、嵌套结构、sizeof不泄漏共7例tape同参考，探针观测REGISTERBANK置1、遮蔽清0并恢复1、退域恢复0；r21仍48同/5具名拒绝/0异，facts30/0、op9，parse2八变体图重录，fresh-order0..8不变。默认图临时注入独立预读/游标恢复，含数字/字符串的4份完整tape不变；带位置的24项只读视图检查通过。以上不是unary-&14例完成，参考补丁仍隔离。

〔C1b 取址检查接线〕采用独立有限token视图，仅查询根及结构成员；合法名字在声明时已INTERN，复制图14例中新增intern顺序不变。SBFIND查询的是blob而非符号，不用它访问REGISTERBANK。根/成员数组按SHAPE rank跟踪，跨指针下标、箭头或调用放行，仍在register对象内则报C99 6.5.3.2p1。复制图6接受同tape、8精确拒绝；正式清单/errors/location与C执行器随后验。

〔C1b 诊断验收补正〕正式C执行器发现uc_kind=1会多报结构返回未覆盖帧；register取址是C约束诊断，应设uc_kind=0，完整stderr必须与参考逐字节比较，不能只检查子串。

〔0.0.37 C1b 同落验收，2026-10-09〕参考register取址检查与E3独立token视图同落，tests/diag.sh增加14例（6接受、8拒绝C99 6.5.3.2p1）。正式默认清单14例通过且接受tape与INTERN新增顺序不变；C执行器E2→E1→E3位置/错误路径14例完整tape或stderr逐字节同新参考，UNITOK2另4例完整结果同；参考diag31/0、r21为48同/5具名拒绝/0接受不同。parse2八变体graphhash重录，其他阶段记录不变；fresh-order0..8全重录核对无变化，facts30/0、kernel stale0、op9。仓根.com直接-run seed/gen.c生成完整parse2与Python逐字节同。正式0.0.37候选.com的14例运行由cc补验，未以合成tape替代；register数组隐式衰变等未纳入本片，不把整个6.7.1宣称covered。元数据额外嵌套FP不泄漏验证累计9例，前文首片7例为当时记录。

〔L1b′ 勘察约束，2026-10-09；只是设计〕不能只修.sys6的SYSFP/SYSSP：六参数SYSA格以及.sys/.write参数格也共享，信号可在保存或装载之间插入而污染外层调用。优先设计POSIX系统调用的私有栈帧（现有load64/store64/栈调整表达），同时核对真实信号入口的host ABI→tape栈边界；旧sigaction补丁直接传函数地址，既有__ccw转换只在转发桩启用时生效，不能假定直接内核回调已有包装。参考、头、lower/enc同批落，先证明嵌套重入和处理函数返回；不以仅修arm64重装FP/SP代替完整验收。详见research/l1b-signal-lowering-design.md。

〔L1b′ 第一轮原型实测，2026-10-09〕仅在/tmp复制头与导出参考C上应用旧sigaction补丁/诊断变体，main产品未改。重构sig_nested：旧参考osx/arm64 rc138、x86_64 rc139；arm64仅删除共享FP/SP恢复的诊断变体rc0，仅强制trampoline host入口rc139，两者合用rc0。私有80B sys6帧+临时host入口桥接：arm64单层rc0，x86单层rc142；更强双信号嵌套系统cc为MABCD/rc0，原型两架构只到MAB（arm64 rc139，x86 rc159）。因此不交δ、不记完成。arm64 .frame只动x7而真实SP未同步，嵌套备用栈可能覆盖tape活动帧，属待验证推断；x86另需核对sigreturn现场。证据与命令在research/l1b-signal-prototype-results.md。

〔L1b′ 第二轮原型实测，2026-10-09〕x86入口原始r8/r9与跳板uctx/token逐位相同；TO_GATE前rax=0x20000b8及三参数正确，排除桥接换址和调用号错误。cc系统clang对照定位Rosetta须UC_FLAVOR=30；临时头按此改后，私有sys6帧原型单层与双层通过。arm64仅在gate期间把真实SP移到x7下512B、保存恢复原SP，双层由MAB/139变MABCD/0。两架构双层各5/5通过。这是缩小根因的临时原型，不是产品完成：gate之外异步中断、sys/write共享格、通用入口与四目标δ同字节仍未验证；512B是诊断值，不是栈契约。详见research/l1b-signal-prototype-results.md第二轮。

〔L1b′ 生产边界设计，2026-10-09；尚未实现〕备用栈与UC_FLAVOR头补丁继续停放。arm64拟建立真实SP≤活动tape栈下界的逐指令不变量：分配先降低真实SP再提交x7，撤帧先提高x7再提高真实SP，真实SP保持16B对齐；call/ret、显式写r7、HENTRY/HLEAVE和宿主调用均须审计，不能仅在gate同步。入口优先核对既有__ccw_保留前缀契约可否表达头内信号跳板，不引入按单个函数名硬接线。sys6/sys/write的私有快照与FP恢复同片验证。此处是下一轮原型约束，不是产品验收结论。

〔L1b′ 第三轮异步反例，2026-10-09；实际跑过〕临时arm64编码器在.frame分配前降真实SP、撤帧后抬真实SP，并处理call/ret及HENTRY/HLEAVE锚点。强探针由宿主异步送ALRM/USR2，外层处理函数反复递归建帧、检查局部数组；全程保护原型3/3正常，只有gate同步原型3/3首次送信号即SIGSEGV。最初外层只在gate内触发嵌套的弱探针两者都绿，已明确不作反例。证据和诊断补丁入research；仍缺任意r7写入完整覆盖、sys/write私有化、通用入口、产品δ和四目标验收，不宣称L1b完成。

〔L1b′ r7/HOSTCALL 原型续验，2026-10-09；实际跑过〕mov写r7临时编码区分方向：向下先降真实SP再写x7，向上先写x7再抬真实SP。独立低级tape移动SP下128B再恢复，实际退出0；双层与外部异步递归反例续验绿。HOSTCALL审计见既有ARM桥用私有真栈保存x1–x7/原SP/LR；加入真实libSystem sched_yield转发的异步递归探针退出0，tape确认含该函数及.hostcall。以上不等于任意算术写r7或所有宿主签名已验；下一片仍为完整r7写入覆盖与sys/write私有快照。

〔L1b′ 私有参数与通用入口原型，2026-10-09；实际跑过〕临时POSIX .sys/.write已改为80B私有帧快照，覆盖atfd_1/atfd_1_zero/atfd_2_zero5/zero4参数形状；sys6原SP暂存由x16改x12，避免.frame计算覆盖它。两架构低级tape分别用r7作为sys6/sys/write缓冲，均ABCABCABC/0。移除信号跳板单名判断，临时头改用已有__ccw_前缀，两架构双层MABCD/0。ARM普通目的寄存器为r7的计算先产出到独立暂存，再按移动方向提交栈；sub/add/load写r7低级探针退出0，HOSTCALL+外部异步探针194次ALRM退出0。这些仍是隔离导出C原型，未生成产品δ；.exit/.print共享格、逐快照重入注入、Linux信号入口和正式四目标验收继续保留。

〔L1b′ 裁库闭包核查，2026-10-09；实际跑过〕libneed.table从头内标识符自动提依赖，不需手写sigaction边。隔离应用14c85649停放补丁后，完整table先报carried body defined twice: alarm（unistd新增POSIX/Windows两个同名static体，解析器不按条件编译去重）。仅在临时副本删去Windows重复体以检查依赖后，sigaction闭包含__ccw_unisa_sigtramp、_unisa_sigkernel、_unisa_sigaltstack、_unisa_sigrestorer与_unisa_ret，守卫名匹配。停放补丁需把alarm平台分支放同一函数体，正式头落地后导出pp facts/E2，不添加手写边或放松重复检查。

〔L1′/L2 数组拒绝最小化，2026-10-09；出货产品实跑〕宏N=32的static unsigned long mask[N]独立-run退出0；仅前置extern const char version[]，同一完整数组在[32]后被拒incomplete array declaration before another definition，退出1。因此存在可复现的未完成extern声明历史影响，不能把完整数组报错直接归因宏长度识别；sqlite现场是否同根尚未验。两例入research/l1prime-array-history，排L1b后，当前输入窗口未动。

〔C2 g5 单元边界，2026-10-09；源码核查，补丁待验〕research/g5-typedef-leak记录a.c的文件域typedef code泄漏到b.c参数同名，交换单元顺序不报错。fe_units每单元重载token而未重置ntd；typedef名字可见性应按翻译单元隔离，不能靠增大MAXTD结算。L1′参考窗口并入此项，由cc准备补丁及顺序互换/不同同名typedef/前单元签名保留的对拍；δ另核对TDN/TDB/TDD/TDE等名字绑定的单元复位。结构描述与跨单元函数签名必须保留，行映射偏移另列，不混作typedef修复验收。排在L1b首片后，尚未修改生产输入。

〔C2 g5 行数补丁审阅，2026-10-09；系统cc实跑，修后参考待验〕unsized-rows.patch用initrowsat(j,per)把每个独立字符串都计为完整外层行，但字符串可初始化指针元素或内层char数组，不能只按token判整行。反例const char *p[][2]={"a","b"}的sizeof为2*sizeof(void*)，char s[][2][4]={"a","b"}的sizeof为8；系统cc合并探针退出0。当前补丁按其扫描逻辑两者均算两行（未跑修后参考）；需携带元素类型/剩余维度，或对此精确拒绝，补对拍后再同窗落。bkscr写越界是独立后端缺陷，缩短此数组不能算边界防护完成。

〔g5 头续行不只影响诊断，2026-10-09；出货产品与系统cc实跑〕包含research/g5-header-splice-line/m.h后，在主文件第3行return __LINE__ != 3，仓根unisacc.com -run退出1，系统cc编译执行退出0（临时源/tmp/cdx-g5-splice-line/line.c）。src/front_pp.c的line_at与diag_at均以ireg_ln/ireg_nl撤销include拼接，line_at还供__LINE__/__FILE__宏使用；不能按仅诊断、不影响代码收口。参考补丁需同步line_at/diag_at/头来源映射，产品E2位置记录与宏展开同规则；验收增加主文件/头内/嵌套头的LINE与FILE及CRLF续行，普通预处理文本不变与受影响宏值的修正须分别对拍。生产输入未改，仍排L1b首片后的窗口。

〔rowcov 原生观测可行性，2026-10-09；源码勘察，尚未实现〕Python sim在transition查询之前记(q,key)，故拒绝/缺边前的最后一次观察也必须保留；t模式栈空的BOT在C为-1。C core_run与ARM/x86汇编run均需观测钩子，不能只改C备用循环。CoreModel没有状态名字，原生日志应先输出阶段/模型身份+数值state/key，以同一模型绑定的状态字典还原，再关联现有终态出处旁表；不得假定另一变体的state序号相同。UNISACC_ROW_LOG当前是构图出处旁表开关（state,key,path,line），运行期观测另用UNISACC_EDGE_LOG避免格式混淆。运行期每次模型执行只写首次命中的边并带探针身份，日志IO失败须显式失败；输出流与模型字节不改。先小模型C/两汇编与sim逐边集合对拍含拒绝/BOT，再pp一片和全pp，最后lex/parse2/lower/enc；各层保持相同模型、输入、资源和flags，不能以默认产品整链替换现有rowcov指定变体。阶段数秒是待测目标，不承诺已达到。排L1b生产首片之后，与cc的C汇总器接口设计可先并行。

〔rowcov C 日志接口反例，2026-10-09；产品-run实跑〕tests/rowcov.c把state_id=-1视为BOT跳过，但机器BOT是key=-1且state_id仍有效。最小STATES 0/S0、EDGES与ROWS S0/BOT、LOG M/0/-1/p.c：原生日志路线taken0/covered0/rc1，--seen S0/BOT/p.c路线taken1/covered1/rc0。应拒绝所有未知state_id（包括-1），把key=-1规范化BOT再关联旁表；已有--seen汇总计数对拍不覆盖该日志接口。STATE字典必须由构造时保留并绑定同模型身份，现CoreModel不含符号状态名，不能从二进制捏造恢复。运行钩子尚未实现。

〔L1b sys6 验收探针审阅，2026-10-09；源码核查〕sys6probe.c的12个__syscall6在参考前端均由sysargs6固定送r0..r5，并发出.sys6 syscall，不因常量号变成具名op，也不因局部声明顺序改变tape源寄存器；帧内buf地址经求值送入r2，不等于直接以r7作源。现12/12证据仅覆盖C参数求值/动态号写调用，尚缺具名mmap和低级tape寄存器置换/r7别名。accept.c中sh编译失败未设置bad、未清理旧产物就sha，可把旧文件当新证据；需生成临时产物、成功才替换、编译失败显式失败。private3的Windows ARM镜像变化已标反例，首片只移POSIX sys6 helper，不搬ARM编码器原型。生产代码仍未落。

〔L1b POSIX sys6 生产首片，2026-10-09；参考实跑与δ对拍进行中〕六个源寄存器在修改ABI寄存器前写入80B私有tape帧（0–40参数、48旧FP、56旧SP）；r7作源取帧分配前的快照，arm用x12、x86用r11保存旧SP，避免未来ARM.frame计算所用x16覆盖快照。动态号从槽0取、其余五参从8–40取；具名调用六参从0–40取，gate后从私有帧恢复FP并释放帧。Windows仍走原全局格路线；不在本片落信号头或编码器SP更新，.sys/.write与HOSTCALL私有化另片处理。生产参考通过验收器四次macOS运行及Windows四镜像基线；lnx/x86_64原生δ对手写tape255条指令与Python逐字段同，其他目标与正式产品验收尚待完成。验收手写tape带注释，lower文本入口目前不接受分号注释，对拍只去注释不改变指令；不把该格式拒绝当sys6错码。

〔L1b sys6 首片验收补齐，2026-10-09；实际跑过〕生产代码已由共享main提交819793f1收录，未改写该提交。六目标lower逐指令/元数据对拍、四POSIX手写tape镜像与C参考逐字节同、参考四次macOS实跑、Windows四镜像旧基线、产品-run种子构造lower整图逐字节同均通过；facts 30表0异，lower九模式基线重录（Windows/default不变），fresh-order九模式无变化。正式新.com实跑与Linux实跑尚待cc候选验收；不据本片声称完整信号安全。详见research/l1b-sys6-production-slice.md。

〔L1b 首片产品补验与第二片边界，2026-10-09；cc提供实跑回执、后续为源码勘察〕cc在同源私有候选a4943d78（8bb59293）上跑accept ok，Windows四镜像旧基线相同，POSIX十二行变化，macOS四路与Lima Linux arm64的C/tape两路通过；参考七片已同窗落7646bb81，阶段门禁进行中。本会话继续只读准备，不动src/include/kernel/exec。下一片优先.sys/.write私有参数帧：严格复用code-abi-sources.tsv中mode0/2的有序mem/imm与参数数目（plain三、zero4四、atfd_1四、atfd_1_zero三、atfd_2_zero5五、write三），仅把mem源改成帧槽读取；旧隔离bk_proto3固定装六个参数不可直接移入生产。准备说明见research/l1b-next-production-slice.md（只是设计，未实施）。Windows保持原序列；ARM逐指令SP保护、HOSTCALL和信号头继续独立结算，私有帧本身不解决ARM真实SP仍高于活动tape帧的问题。

〔C2 g5 云机产品反例，2026-10-09；实际跑过〕原有 a/b、s1/s2/sm、g1/g2 双向共六组 typed E1→E3 模拟输出与 Linux 参考逐字节相同；参数遮蔽已清 TDN，原例不足以证明产品有错。补充反例：第一单元 typedef struct {int x;} code，第二单元未声明 code 却 return (code){1}.x!=1；参考拒 unknown identifier，产品 δ 接受。拟只在 @unit+ 清 typedef 名字可见标志及其枚举类型标记，保留描述符、结构池、跨单元函数签名；用 typedef 写入的 intern ID 上界限定清理，逐名只处理有效 TDN。验收含原六组保持同字节、新反例双序拒绝以及同单元合法复合字面量。

〔C2 g5 云机验证资源边界，2026-10-09；实际跑过〕修后普通 E3 原生网络穷举 2559878 观察一致；原六组 typed E1→E3 模拟与原生网络均同参考 tape，新增复合字面量泄漏由接受变拒绝，合法全局同名对象同字节。unitlocationcheck 全过（位置序列化、10 map/7 frame 拒绝），seedfacts 1076 同/0 异。并发两 Python 变体与 C seed/gen 构造时，内核 OOM 日志杀 cdx37-seedgen（anon-rss 5932868kB），两变体超 55s；已报董秘并改串行，不放宽 60s，不计通过。C 种子构造一致性与正式候选/新增 multi isolation 全片实跑仍需补验。

〔C2 g5 云机修片冻结前验证，2026-10-09；实际跑过〕八个 parse2 变体均串行/受限构造成功，fresh 哈希及计数全部保持既有基线；仅更新对应八条 graphhash。r21 复用本轮新构造普通 E3 JSON，PP/E1 重建后原生网络实跑 48 同/5 具名拒绝/0 接受不同。errors 模式的跨单元泄漏与同单元合法复合字面量双向共四组 tape/完整诊断与参考同字节。回归借用的七份最小例复制入 tests/multi/typedef-*.c，纳入既有门禁的 tests/multi 输入闭包，避免 research 例变化不使队列缓存失效；新增 multi isolation 全片与 C 构造器一致性、正式 .com 仍待 m4pro 验收。证据整理于 research/c2-g5-unit-typedef-cloud.md，不结算整个 C2。

〔云机提交身份，2026-10-09〕云机未配 Git 作者，当前修片提交只用命令级 cdx-unisacc <cdx-unisacc@localhost>，不改全局配置、不借用人的作者身份。会话中 main 已经论文/计划文档更新前移666bb0bc；编译输入未变，本片仍只提交自身路径。

〔C2 g4 云机接单核对，2026-10-09〕董秘/cc授权先推g5再只跟g4；g5与cc的c4e5001f文档已合并推到cee7ccc7。当前main已含5a632535的g4实现：普通sys=0超过六参走CL.vdone，CL.vend按sys=0恢复直接调用；callcontrol.py已于15bbf1bf迁成manifest删除，云机/home/box与/tmp未找到暂存副本。先按当前构造链重新生成E3并复验无原型/有原型/K&R与多单元；不手改TSV、不复活已删除构造器、不以公开0.0.36旧拒绝判main缺口。cc独立克隆验收待交接。

〔C2 g4 当前源码复验，2026-10-09；实际跑过〕main d87c8917 从当前 manifest 重新构造 E3，JSON sha2187cf66、net shaf25d68ab，与g5冻结证据一致。六类九组（无原型/原型单文件、原始a+b双序、加权无原型/原型双序、float默认提升）原生E3 tape与参考逐字节同，系统cc与参考-run均退出0。cc确认K&R定义在参考本身expected {，撤销该变体；正式验收保持a+b、完整单文件、原型对照三者候选.com退出0且同参考。无需重复修5a632535已实现的控制，不手改表、不复活15bbf1bf已删除的生成器；当前仅补可复现证据并交cc独立克隆。Darwin构建限制仍使正式候选未验，不结算C2整项。

〔0.0.37 Linux 正式候选构建调查，2026-10-09〕政委明确不写死Darwin，授权查实依赖并移除可移植守卫，目标云机完成候选及发布。已pull ded65f53与后续文档至31f1ebba，不改验收。buildcompiler守卫背后是seed/blob.c、asm/blob.py以cc -arch/Apple ld参数生成Mach-O并抽取含头的PIC kernel，不是运行时OS依赖；拟先用Linux LLVM Mach-O交叉汇编/链接验证UNIKERN1、零未决导入/运行重定位、入口/slot与原生执行契约，再修改构建适配。LLVM未安装，准备安装Debian现有clang/lld/llvm19。Apple签名公证/dmg与多平台证据是独立发布边界，不能仅删Darwin行就宣布发布可行。

〔Linux Mach-O 种子最小实验，2026-10-09；实际跑过〕LLVM19交叉构造两ISA后，原blob.py抽取/结构检查均接受，ARM7672B sha9bab0499、x867696B sha56bcd071，未决导入0；与公开0.0.36 Apple ld64包内blob不同，不能冒称逐字节同。拟让C/Python两构造器共享一个host工具适配（Darwin保持原cc/ld参数，Linux用clang+ld64.lld+llvm-nm），UNIKERN1格式与所有PIC/入口/零重定位检查不变；扩seedgen首片原有blob对拍至Linux，先产品ABI原生x86网络执行后去buildcompiler守卫。cc已查Apple签名/公证/dmg及macOS门禁为真依赖、GHCR缺write:packages；这些尚不能由Linux替代，发布不予豁免。

〔Linux kernel 实跑与构建路径，2026-10-09；实际跑过〕共享machocc适配后C/Python双ISAblob逐字节同；产品ABI x86原生netcheck全过，含E3表2559878观察、资源/包/拒绝/诊断链与无Cfallback；seedgen e2首片SAME e2/ident/blob，3同0异。保留默认C构造，首台未装产品或Cgen内存不足时使用原有SEED_GEN=0/SEED_C=0路线试建（非新增回退）；build_candidate的已安装产品pair前置只在实际使用它的SEED_C=1要求，Python路线不读取该pair。正式发布仍受macOS门禁/Apple资产与GHCR权限约束，不改验收。

〔Linux 候选全构造，2026-10-09；实际跑过〕既有Python构造路线SEED_GEN=0/SEED_C=0已完成shared、六target、三pack-prep、pack-models、pack-driver，每步bound58内成功。私有产品2243464B sha8d147e0f，源码身份80f404fc；版本仍0.0.36，非封版。g4五组/C1三探针同参考；初次diag因语料缺位0项失败，补齐上游语料后原套件31通过0错（40损坏语料、13定位、14register例）；seed七工具编译通过，pack.c仍在zlib include拒绝。政委新令本云机继续闭环，拟合入cc Linux门禁修片并交独立克隆，不降低验收。

〔0.0.37 候选版本推进，2026-10-09〕cc独立克隆实际验私有Linux构造：g4五组、C1 64–66、C1b diag31/0、C3七工具与realpath通过（research/c37-linux-precheck.md）；pack.c zlib包含仍拒绝，不结算C2整项。政委要求本云机继续完成发布，现只推进同一候选构造/验收切口：版本号升0.0.37，重建同源参考/候选，构造seed对并验证comboot定点，再交Linux全量642套件。升版本不是封版或全项完成。

〔0.0.37 云机构造交接，2026-10-09；实际跑过〕版本b398a194；同源UA /tmp/cdx37-linux-ref，候选/seed对在/tmp/cdx37-linux-candidate。候选2243464B sha52173312，--version0.0.37，身份核验成功；seed7581888B sha928bda8d，hello与版本正确。g4五组、C99 64–66在新候选同参考rc0。shared并行首次58s超时，串行原预算内通过；cc已确认当时另一路comboot parse2并发OOM，原生Cgen单测也OOM（5576016KiB/18s），jemalloc试验50s超时；均不算绿。现完成Python构造后交cc独占COMBOOT_BUDGET=1现有窗口旋钮做定点及642套件，本会话停止重构造避免抢内存，不改验收。

〔0.0.37 本机封存实跑阻塞，2026-10-09〕按原seal_candidate.sh对/tmp/cdx37-linux-candidate尝试GHCR封存（bound55），registry HEAD返回403 Forbidden，未改release/candidate.json。gh api用户响应scopes为gist/read:org/repo/workflow，缺write:packages；需要补齐本机会话包写入凭据，不用未封存产物冒称正式发布。候选sha52173312和seed对已交cc串行定点/全量；cdx不并发重构造。已报董秘真实403证据。

〔0.0.37 主导互换接管，2026-10-09〕政委/董秘授权恢复并由cdx主导，cwd=/home/box/repos/unisacc-cc，HEAD b8d96260；cc已释放重活，父只读ps未见gen/matrix/候选/queue残留。未提交仅seed/gen.c的group-tail所有权释放修片，保留且只在矩阵全绿后单独提交。cc交A2三十调用全同字节、B2十三调用ASan通过，errorparse终止143及其余十七项未验；旧52173312候选/队列证据只作历史。原matrix.sh的120/600/3000秒限时不合AGENTS上限，接管改用独立输出、逐项bound50/外层≤60、rc与source/tool/output摘要齐全的串行B验证，不删矩阵项。先用clang O2 ASan单项测量（相同源、地址检查保留；既有detect_leaks=0，按cc建议quarantine64/context5），不套生产读数；若单项超时保留失败并定位，不提高预算、不冻结候选。接单回执后一机一重活，B完整通过→冻结一个提交→同源候选/seed→自举/queue，沿§24–27顺序。

〔矩阵B受限诊断，2026-10-09；cdx实际跑过〕clang19 O2 ASan+quarantine64/context5：warnunits 8.25s/rc0，与Python同字节；errorparse在bound50退出142/无输出，保留/tmp/cdx37-matrix-b/errorparse.receipt.json，未记同字节或通过。运行中约1.45GB RSS，未OOM；先单独将ASan分配栈深改context1（只诊断元数据，地址检查/quarantine64/全输入不变），以真实UAF负例确认检测仍有效后复测同组合，不改生产预算，不冻结候选。cc已只读复核acts独占/joined借用/label独立的所有权释放正确。

〔矩阵B errors预算定位与同切口修片，2026-10-09〕context1真实UAF负例检出，但errorparse仍bound50/142，第二失败回执保留，未记绿。父读group-tail发现每条边重复value_json同一intern动作序列；同一模板行内结果仅依赖oldseq、固定op/prefix/groups，且groups标签按首次命中顺序分配。拟在该行按旧序列缓存“无变更/目标label/新seq”，保持原边遍历和首次编号次序；已存在manifest_companions的mapped先例。仅触及同一group-tail热点，保留cc的独占树深释放和joined借用浅释放。改后旧A/B结果对新seed源码失效，完整三十项生产/ASan重跑；失败继续先定位，不建候选、不改预算、不降地址检查。

〔group-tail memo独立只读复核，2026-10-09〕cc核seq文本intern副作用、负缓存的行内不变性、首次标签顺序、ncache初值/新seq不重复访问、groups中label借用与spec释放顺序全部成立，无阻塞；旧A/B对新源码作废。完整B恢复quarantine64/context5原诊断设置后重跑（context1仅失败诊断实验），先errorparse最小受限验证再展开完整矩阵。

〔group-tail修片受限B最重组合已过，2026-10-09；实际跑过〕恢复quarantine64/context5，clang O2 ASan errorparse37.15s/3155040KiB、warnparse37.48s/3175204KiB，rc0且逐字节同Python，本轮seed源码sha8ba73354稳定；两个单项不是全矩阵。开始同一源完整三十项生产O2与ASan；Python参考为cc本轮A2新鲜生成，Python/TSV未改，仅seed/gen.c缓存修片。旧失败回执保留于.previous-*；所有当前项要匹配新源码hash/flags/工具才计入。

〔同源生产矩阵A完整通过，2026-10-09；cdx实际跑过〕新seed/gen.c sha8ba73354、GCC O2三十调用逐项bound50/外层55全部rc0同Python（cc本轮新鲜参考，Python/TSV无改）；source前后稳定。errors生产6.75s/2171620KiB、warnings+errors5.79s/2186436KiB，对照交接A2的40.58/33.69s。完整回执在/tmp/cdx37-matrix-a；不是候选验收。B同源最重两项37s内已过，继续其余二十八项，完整B绿前不提交冻结或建候选。

〔0.0.37 全矩阵已闭合，2026-10-09；实际跑过〕新seed源码8ba73354的A/B各三十调用全rc0/同Python，flags调用差集missing0/extra0，全部逐项bound50并记录源码/工具/参考/输出摘要；超时旧回执仍保留。证据research/c37-generator-matrix.md；cc独立只读复核所有权及memo语义。现在单独提交seed/gen.c修片与这轮证据，冻结一个源提交；之后只起一条默认C构造候选链，旧52173312及10/642不作新身份通过证据。

〔冻结候选链宿主编译定位，2026-10-09〕freeze8ea81419默认C链shared24.25s/1285236KiB、六target及pack-prep1/2/3全部rc0。pack-models主机编译seed/compilerpack.c因strict C99的glibc realpath声明不可见退出1，尚未打包；保留日志/tmp/unisacc37-8ea81419-pack-models.log。不改冻结源码，不重跑整链；通过已有SEED_GEN_CC="cc -D_XOPEN_SOURCE=700"给尚未编的host工具提供标准X/Open声明，单项先编测后仅续pack-models/driver；已完成manifest继续严格核身份/输出，预算不变。候选生产记录须标明此宿主选项。

〔冻结链第二宿主依赖缺位，2026-10-09〕X/Open编译探针rc0；续pack-models随后在host seed/pack.c:19缺zlib.h退出1，保留-xopen.log。本机补系统zlib1g-dev（先download-only受限，再no-download安装），不改冻结产品源码、不重跑模型；产品pack.c缺口与host系统头缺位分开记录，不能把host安装算产品C2通过。

〔冻结候选对与独立验收，2026-10-09；实际跑过〕8ea81419默认C构造pack-models/driver及build_seed已通过；host SEED_GEN_CC="cc -D_XOPEN_SOURCE=700"，系统zlib1g-dev在位。候选/tmp/unisacc37-8ea81419/unisacc-next.com sha52173312db634105eee1ca58d2aa72d6ca96f0e59a16ae961758fd9504f0fb64、seed sha928bda8d18efc20374e2a62b2dfb828dc847e07f9bc3694e43619e4e7d37a6dd；source e68c02ef8874870b44936cb148e1de6421ab30e23575d6ecab3cc27e99a7e93c，两者版本0.0.37。生成器只修资源行为，产物与旧候选同字节但身份不得复用旧源码验收。cc独立accept run-185235，27s/rc0，17/17 PASS，pack.c:19具名缺口不变，K&R两路皆拒且不计通过；cc确认无残留生产者并交还owner。prd未提交均为cdx构造现场记录。

〔冻结元数据与账本预检，2026-10-09；实际跑过〕freezecheck报92dbae02与8ea81419缺fix:前缀，两笔均为已授权红修（OOM/受限ASan超时），不是新功能。按freezecheck已有release/freeze-waivers.tsv机制记录红证据，不改检查器、不重写已推历史。ledgercheck --final仍报C1/C2/C3/F4′/W2/E57/X3/L2/Q2/L1′/L1b′/A1/N1十三项未结算，carry mismatch0；不以17项小验收声称整账本结算。继续同源自举，先seed再受限stage2窗口，一机一重活。

〔自举窗口实测与宿主适配评审，2026-10-09〕stage2 shared、六目标、pack-prep-1已完成；随后一窗身份检查耗6s使COMBOOT_BUDGET=5未启动pack-prep-2，rc75但无新增步骤，不计进展。仅既有步骤调度旋钮改10，单步55/外层58不变，下一窗19s完成pack-prep-2。cc只读复核queue专用cc launcher：仅seed/compilerpack.c加-D_XOPEN_SOURCE=700，其他全参数原样exec /usr/bin/cc；strict C99及Python/包字节对拍不改。这是glibc宿主声明适配，非产品修片；实际launcher与系统cc/glibc身份须进证据，后续版本正式宿主兼容项去掉临时适配，不作通用豁免。

〔0.0.37 同源自举定点，2026-10-09；实际跑过〕seed→stage2→stage3默认C构造均完成；每次outer58、make内55、生成器50，串行无并发生产者。stage2/3字节相同sha52173312，fixedpoint rc0；stage3 freshness核source e68c02ef通过，seed artifact/exported_source_sha256与seed-work/flat.c核验通过。COMBOOT_BUDGET=10只是步骤启动调度窗口，所有看门狗未放宽；5秒窗口出现一次无进展已记录。独立17/17证据与定点回执归档research/c37-frozen-linux-chain.{md,json}。按已有真实证据给C1标已完成、Q2保留既定不改结论并补已完成状态，其他未结算项不自行顺延。封存前契约仍须绿；不先起正式queue绕开§27。

〔封存前廉价契约阻塞，2026-10-09；实际跑过〕C1/Q2补既有结论后ledger --final rc1/unsettled11、carry0；freezecheck rc0、facts-export --check rc0/30表0异。subtractsafety rc1/19处dangling users，涉及research里a2-preregistration/r25-release-acceptance/formalization-roadmap旧路径，真实文件现archive/research；部分seal-a2 JSON的path/from是历史事实，不能全局替换篡改。已向cdx2报董秘及文档owner处理，不改检查器、不改他人历史回执。cc独立复核定点JSON/候选seed身份一致、树净。遵§27与cheap红先修门槛，正式封存/queue未启动；超限账裁定未到，未擅自顺延或认定完整发布通过。当前无重活生产者。

〔Linux宿主适配实测与历史引用审阅，2026-10-09〕cc已核strict C99 compilerpack.c在glibc缺realpath声明；拟于/tmp专用PATH cc启动器仅seed/compilerpack.c加-D_XOPEN_SOURCE=700，其余调用全参透传/usr/bin/cc，记录启动器/实体cc/glibc身份，并跑原seedpackcheck逐包字节与audit对拍（不改套件）。研究归档修法由cc认领：保留所有历史回执，仅改当前论文两引用与旧路径指针；JSON旧路径用相对symlink指向归档真receipt，避免消费者把元数据桩误当验收JSON。核实归档提交31244660/bff850cc；待本套件结束再通知cc动树。

〔subtract-safety 历史路径合规修，2026-10-09；实际跑过〕19处旧路径分两类：research/ujs-paper.md:11/211 为当前引用，改指 archive/research/formalization-roadmap.md；seal-a2/seal-platform/paper-notes/c37-preseal-cheap-checks 中 path/from/sources/检查器原文为历史回执，一字不改。旧路径留指针：research/a2-preregistration.md、research/formalization-roadmap.md 两份一句话MD（归档于31244660，正文在archive/），research/r25-release-acceptance.json 为相对symlink→../archive/research/r25-release-acceptance.json（读到真receipt，不另造JSON）。检查器、验收口径、归档件字节均未动。subtractsafety dangling 19→0，--selftest 两个0.0.15反例仍抓到。owner cc。

〔Linux宿主打包适配通过，2026-10-09；实际跑过〕/tmp/cdx37-host-tools/cc仅参数seed/compilerpack.c或*/seed/compilerpack.c加-D_XOPEN_SOURCE=700，其他原样exec /usr/bin/cc。启动器sha5c036d42，系统Debian GCC14.2.0、glibc2.41，--version透传逐字节相同；完整身份与源码见research/c37-host-adapter.json。原tests/seedpackcheck.sh outer55 rc0：same4(P1/P2/P3含mount、1236 routes完整compiler包及audit)、拒绝3；不改源码/套件/比较对象。仅解决宿主glibc声明，不算产品pack.c zlib缺口完成。临时adapter只给该轮进程PATH，后续正式源码兼容提案记入v0.0.38草案，未开实现窗口。

〔发布阻塞审计与CI候选绑定，2026-10-09〕连续三个目标窗口均复核ledger --final十一项未结算，carry0；已集中请求董秘逐项裁，未收到结论。其间可独立完成的默认C构造/定点/宿主包对拍/归档引用修复均已完成，当前不再重复重活。run37921949099的source/ccinterop已成功，但candidate阶段读取release/candidate.json仍指0.0.36封存SHA b4607639/source407f4f3c；冻结0.0.37实物为52173312/sourcee68c02ef，故此run候选阶段不构成新候选六格验收。工作流main推送遇closure mismatch会跳过候选步骤并仍显示绿，发布tag路径才硬失败；不能以绿色跳过或等待该run冒充新候选验收。§27与cheap门槛要求账本结算后再封存/正式queue；不擅自顺延超限项，不改验收，目标未完成，需董秘裁定后续跑同一冻结候选。

〔董秘代裁恢复与F4唯一切口，2026-10-09〕政委授评估权的董秘逐项裁已到并明确resume：冻结source e68c02ef/候选52173312保留；C2/C3按冻结独验DONE，pack.c zlib具名缺口归原C2结论、宿主zlib仅构造前提；W2/E57本版授权顺延，Windows真宿主条件与原验收不变；X3/L2/L1′/L1b′/A1/N1无整项冻结证据则+1入0.0.38候选，已验证部分具名保留。F4′不许再延：v0.0.32 csih固定15单元、新进程冷编三次中位数≤5s才DONE，编译失败或超限本版FAILED并开2h定位。cc只读核tag28c850e6/apps树8d378e6652fb与顺序、默认8单元滚动池/默认缓存；Linux静态产品曾拒host libc forwarding，失败计时不得算合格。先更新计划并push，再主测，不并行queue/生成器，不改验收。

〔F4状态标记预检注意，2026-10-09〕ledgercheck仅搜索关键词，F4“进行中：不准再顺延”被其中顺延误认结算，初次显示0项不代表性能已过。状态标记改同义“禁止再次延期”，保持≤5s/FAILED/2h验收不变，不改检查器；必须以实际F4测量结论收口。

〔F4首测FAILED与定位首因，2026-10-09；实际跑过〕v0.0.32固定15单元在冻结52173312/sourcee68c02ef新进程默认8jobs/默认单元缓存冷编，wall4.99/user8.11/sys1.17/RSS166912KiB，rc1，无产物，拒host libc forwarding needs a dynamic compiler image；失败耗时不算≤5s通过。进入董秘授权2h定位：libcurl.so.4与libdl宿主在位；src/host_dl.h明确自编Linux image无loader slot，compiler.c:725却在-o与-run两路统一拒绝，src/back_encode.c仅-run要求slot。拟先scratch派生静态driver单行收窄runit守卫、复用原compiler.pkg测写ELF/真实执行/仍拒-run；不改冻结源、不冒称派生产物正式候选，不重构造模型，cc只读审阅。

〔F4修片定位第二证据，2026-10-09；实际跑过〕scratch仅收窄runit守卫后csih15单元成功产出动态ELF并selftest rc0，但冷编11.71s，未满足5s。trace复测12.03s，fwd_ccw=1导致fast=0；91d6c1ea为callback支持强制全E3重放，覆盖此前63280efb增量快路。同一F4切口下一scratch实验仅使用已有cli/ccw资源在第一遍生成callback wrapper/mapper，再尝试既有fwd_incremental；δ/模型不改，不能取消callback语义或改变15单元。须与强制重放逐字节/行为对拍及callback反例核证后才考虑落修片。

〔cc重新主导、cdx执行F4功能修片，2026-10-09〕重活由cc统一调度。已实测Linux静态编译器的宿主loader守卫仅对-run必要；-o生成程序在运行时使用自己的loader。将compiler.c守卫收窄为runit && host_dl_slot(1)==0，保留-run拒绝及back_encode二道检查。同一单行scratch静态driver：getpagesize探针-o成功/实际运行rc0、-run仍原错；csih固定15单元-o/selftest均rc0但11.71s。该修片只解决功能阻塞，不认定F4性能完成；ccw快路仍因merge尾段形状拒绝而重放，后续同一切口处理。冻结52173312不因这次源码修片变成新候选。

〔F4快路包尾定位，2026-10-09；代码实读〕forward-result先将原tape封入USLFW1，再在全部records之后追加ccw wrapper/mapper；仅初遍启用cli/ccw仍不够，fwd_sidecar目前丢弃records后的尾部。拟scratch将首遍全程序尾部仅cnt>0且ccwc>0并入base，保留全程序static名称/回调地址；stub-only尾部不当全程序名单，merge只在base已有mapper时识别并丢弃重复生成段。无forward/ccwc0不加段，异常形状保留慢路。

〔F4本版FAILED回执提交，2026-10-09；cc主导决定〕cc决定ccw快路不落0.0.37：cdx静态scratch fast=1仍8.91s/selftest rc0，未满足原≤5s；cc宿主scratch报告6.93s，e3/lower/elf三串行段合计5.04s，仅定位参考。p1快慢运行均0而布局字节不同；p2无转发字节同且运行0；p3快慢字节同且双方rc2，既有问题另记后续，不冒称通过。冻结候选首测rc1/无产物；fc7ca424守卫源码修片尚未包含在冻结52173312中。F4状态本版FAILED，等待董秘/政委结账裁定及守卫是否进入发布物；不擅自认定账本结算、不启动候选/queue。定位数据保存在prd本段（过程JSON未入库）；cc收回重活owner。

〔过程JSON出库，2026-10-09；董秘代裁〕research下0.0.37的/tmp过程JSON（c37-f4-failed、c37-frozen-linux-chain、c37-generator-matrix、c37-linux-candidate、c2-g4-current-cloud、l1b-signal-async-results）从树删除，不改写历史；引用改指同名脱敏.md或本文摘要，.gitignore加过程产物规则。F4′摘要：冻结候选首测rc1无产物；守卫修后冷编约11.7s，ccw快路scratch约8.9s，串行e3/lower/elf约5.0s，未达≤5s。

〔cc授权同源预备链，2026-10-09〕主导cc已push8911adf4并指定cdx为唯一重活owner：固定该HEAD仅重建含fc7ca424守卫的默认C候选→seed→stage2/3，不封存/queue/Draft。已核seed/gen.c sha8ba73354与既有A/B30项同源，seed/exec/build/exec/parse2相对8ea81419零改动，复用生成器矩阵证据，不将旧产品验收复用到新候选。构造设SEED_GEN=1/SEED_C=1，原Linux宿主SEED_GEN_CC适配保留；每step55/outer58，comboot既有budget10，串行链；新source/hash待构造落地记录，产物交cc独验后等待发布物裁定。

〔同源预备链与独验完成，2026-10-09；cdx构造实跑、cc独验实跑〕从8911adf4构造默认C候选，source689a91de5cdd68c39d7ef20ba0f4cbd4a06846d4fe815ca4a7aeea61c9568d54；候选/stage2/stage3逐字节同sha740007ef4f25cb1d2a1ad0e35bf1b7dbe7ffc5d3739cf2d5d518e350b525fcc7，2243480B。经典seed sha928bda8d18efc20374e2a62b2dfb828dc847e07f9bc3694e43619e4e7d37a6dd与原版相同，导出源与侧录摘要一致；产品三份freshness均通过，候选与seed版本0.0.37。每目标/准备/打包均rc0；comboot stage2/3各八窗，以75按原身份续用、最终rc0，定点rc0。期间ce3b5d69仅文档清理，product source保持同源；构造身份记录HEAD可不同但闭包相同。cc在cf47738b树净独验：候选740007ef、seed928bda8d、同源参考0e10357e，17/17 PASS无超时；pack具名KNOWN-GAP不变，g4_kr候选/参考双拒仅作HISTORY。固定v0.0.32 csih15单元以新进程/独立TMPDIR冷编三次11.93/11.82/12.38s，rc全0、中位11.93s；产物selftest rc0，NEEDED libc.so.6（curl经宿主dl转发），-run仍拒host libc forwarding needs a dynamic compiler image。守卫功能修片通过，F4′按原≤5s口径FAILED，不以功能通过冒充性能过闸。未封存/queue/Draft，过程JSON不入库，无绝对临时路径；重活owner归cc，F4结账及发布物选择待裁。

〔0.0.37 本机全量队列结账，2026-10-09；实际跑过，董秘裁定放行〕候选740007ef（rc/v0.0.37→7a8764fc）。本机queue完成380/642：通过302，红78，未跑262（rowcov四项外层bound60先于单项limit48杀窗、空转12窗后按PID停）。红四类：①工具缺失40=csmithdiff（装Debian csmith 2.3.0并补头路径后独立state重验36通过/wrong 0，4片超时）；②宿主超时21+com-seedgen CPU 17s>10；③测试写死macOS/arm64或Mac工具链11（含exec-native-*，宿主选择修片e6910112仅语法绿、独占55冷准备超时未记绿）；④宿主cc/glibc差异5（apps-real、proc-enum、exec-driver-language-2、exec-memory-cc-1、lib-callable-catalog），均为宿主参照构建失败，未见产品字节/语义不符。放行依据：CI release-check 7a8764fc绿+cc独立17/17+stage2=stage3定点；不记为642全PASS。限时与宿主化记0.0.38 H1/H2。

〔董秘F4结账执行分工，2026-10-09〕F4′本版FAILED获准顺延5到0.0.38 F4″，原≤5s及csmithdiff/difftest/com无新红验收不改。cc主导740007ef封存/单一queue/rc绑定；cdx只改两计划账本并起草脱敏scratch发布说明，不碰candidate.json/gatedeps/rc标签，不起第二重活；无需再等政委。

〔云机nativecheck修片，2026-10-09；董秘授权〕最终分工固定cdx修nativecheck、cc建Draft/queue/0.0.38限时基线。native chain/stages/resources统一按宿主Linux/Darwin与x86_64/arm64选择原生目标及elf/macho后缀，保留同runtime网络与compiler构造字节相同等判据，预算不变。selfelf脚本默认Linux目标并已显式传目标；macself是具名Mach-O交叉对拍，不偷换成ELF。selfcheck的单独bootstrap目前限定Darwin，列为本机未执行而不算通过；本轮不扩大修片。

〔nativecheck宿主修片受限回执，2026-10-09；实际跑过〕目标与镜像后缀、bundle route统一按宿主，Linux/x86_64选lnx/x86_64/elf，Darwin/arm64保留osx/arm64/macho；同runtime网络/compiler字节相同等判据不变。sh语法与内嵌Python compile通过。首测与cc释放queue后的独占trace各bound55均rc142；独占trace正确选Linux目标，停在models.py→prepare.sh冷构造阶段，尚未进网络字节对拍，不能宣称功能rc0。按董秘接受宿主超时清单提交修片，待下版基线校准继续验证，预算不改、不重试到绿。已交还重活窗口cc跑csmithdiff；不创建第二Draft、不改产品闭包/rc/候选记录。

