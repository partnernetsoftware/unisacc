# 0.0.46 core-min：tapegate 第一试点设计

## 状态、身份与禁区

本文是设计，不是迁移收据。新旧等价实验、构造链与体积测量均为 **NOT_RUN**；父进程仅实跑既有基线：`sh ./unisacc.com tools/check-tapebin.cx`→`PASS byte-identical bytes=383`、`TAPEBIN_ROUND failed=0`，`sh ./unisacc.com tools/check-tapebin-wiring.cx -- .`→`TAPEBIN_WIRING failed=0`，不代表迁移资格。仅写此页，不改产品、测试、版本，不起链。必须等 **c46-1 候选发出且 fable 独立评审批准本设计**后才实施。route仅作热身，其结果不能替代本试点资格；cxmod/source_frame是第二批，本版不动。

已知旧盘点身份为 `5ac7784dcb`：tapegate.o 为 450432B、649 行、TABLE-DRIVEN、环境与闭包 IO 为 0，反向 use 闭包对象用户为 drive.o、drivetapebin.o、m0.o。这些是旧盘点，不是当前候选实测。本次读取 `cx/compiler/tapegate.cx:1-649`，源文件 SHA-256 为 `3c65588e40b86e5b14c98cb5fe1699dc441e7692a7400752993b3b2a649624ff`；649 行相同不证明内容与旧提交相同。当前源仍为函数实现，没有迁移。对象用户不等于源文件中三个独立调用入口。

## 真实输入输出与职责

事实：本模块并非一个完整的“tape 字节流→原因集合”函数。它是一组容器判定、LEB 解码、引用检查和规范文本输出函数；原始文件读取、SHA、全局错误与退出都在外面（`cx/compiler/tapegate.cx:3-8`）。迁移契约必须覆盖两层：

1. 模块层输入是借用的不可变字节数组及长度，以及 target/force、目录索引/期望偏移、minor、段 off/len/cnt、输出容量等显式上下文；不是只看流字节就够。
2. 集成层才把这些判定按当前顺序组合为接受/拒绝及实际原因。0 是成功，1 是原失败路径，4 是非法 API 参数且不在旧正常路径；不能把 4 归并成损坏 tape。原实现没有多原因收集 API，不能声称返回了所有错误。
3. 接受时还必须保留输出参数、游标与规范文本逐字节；只比接受率不足。拒绝时必须保留输出参数不变及原短路顺序。

### 判定与边界清单

以下每项都是迁移覆盖项，不能只迁“头检查”就宣称整个 tapegate 完成。

| 范围 | 当前规则与可观察结果 | 证据 |
|---|---|---|
| 固定字读取 | 小端累积；调用者保证可读范围 | `cx/compiler/tapegate.cx:9-19` |
| magic | 空指针→4；小于136字节或前8字节不是 UTAPEBIN→1 | `cx/compiler/tapegate.cx:20-33` |
| 固定头 | major=1，minor=0/1，格式字=1、保留=0、flags=0、目录起点=64；minor 只在成功写出 | `cx/compiler/tapegate.cx:34-48` |
| 段数 | 3或4，64+24*nsec 不越界；成功才写段数 | `cx/compiler/tapegate.cx:49-59` |
| origin | 0..6；force 为假且 target 与 origin 均非0时须相等 | `cx/compiler/tapegate.cx:60-69` |
| 对齐及目录 | 8字节对齐，负游标返回-1；index 0..3；kind=index+1；前三段flags=1，第四段=0；64位off/len不超过有符号上限，off等于expected，段落不越界；成功一次发布off/len/count | `cx/compiler/tapegate.cx:70-102` |
| 填充、第四段、终点 | 填充全0；非法范围→4；第四段必须32字节/1条，否则不观察；最终游标必须等于总字节数 | `cx/compiler/tapegate.cx:103-124` |
| 名字及区间 | 名字禁止NUL、空格、TAB、CR、LF、冒号；names池非空名字；size/room非负且size≤room | `cx/compiler/tapegate.cx:125-138` |
| 固定容量引号 | 双引号包围；LF/TAB/CR/NUL/反斜线/双引号转义，ASCII32..126直出，余者小写十六进制；预留结尾NUL；容量失败可能已改写前缀，不保证输出原子性 | `cx/compiler/tapegate.cx:139-189` |
| ULEB/SLEB | 最多10字节；拒绝截断、溢出、非最短编码；仅成功提交pos/value；SLEB符号扩展必须与原位运算一致 | `cx/compiler/tapegate.cx:190-240` |
| 池 | 解码计数与expected相同且≤262144；每项长度有符号int可表示且不越段；名字逐字节检查；段恰好消费完；两组starts/sizes指向原输入的偏移而非复制字符串 | `cx/compiler/tapegate.cx:241-245,330-388` |
| 文本缓冲 | realloc增长；正文上限33554432字节并另留NUL；十进制格式用%lld/%llu；追加失败可留临时前缀 | `cx/compiler/tapegate.cx:246-329` |
| 指令文本 | 形状/名字来自tapeshape；双空格缩进、助记词列宽7再空格、逗号空格、换行；内存操作21/22/23/24/26/31单独布局，寄存器r0..r7，有符号偏移带非负加号 | `cx/compiler/tapegate.cx:389-467` |
| 记录 | 计数等于expected且≤段长；kind 0..7但按分支限定；minor1须首条unit2且最终见unit，gdef须unit；名字/常量引用范围；bss≤有符号64位上限 | `cx/compiler/tapegate.cx:468-543,590-596` |
| 指令二进制 | opcode须有shape，arity≤8；寄存器半字节，奇数末高半字节必须15，寄存器≤7；tag仅0/1并选择SLEB/ULEB；非i类型的tag1须合法名字索引；逐条输出后检查总正文上限 | `cx/compiler/tapegate.cx:544-595` |
| text入口 | 参数非空；n≥136，minor0/1，前三段off/len/cnt非负且计数≤262144且不越输入；仅输出前三段，第四段不打印；空正文仍分配1字节NUL；成功才发布out/outn | `cx/compiler/tapegate.cx:598-649` |

### 副作用、所有权与生命周期

输入 b 及元数据借用，在完整调用期间有效；没有文件、网络、环境变量或全局tb_bad写入。不得因此称其“无副作用”：`tb2_quote`写调用者缓冲；固定头/目录/LEB写输出参数；`tb2_text` malloc/realloc/free、memcpy、snprintf，会因分配失败返回1（上述源码范围）。池索引是临时堆数组，text返回前成功失败均释放；临时文本失败释放，成功转交调用者，outn不含NUL，调用者负责free。text失败out/outn保持不变；quote容量失败可能写了前缀，两者不可混淆。LEB并未自检负pos或end与实际数组长度，内部池函数也不知道总输入n；迁移不得偷偷给这些前置条件之外的内存访问定义“等价”结果。合法API范围内做差分；非法空参数等显式返回4的边界单列，不能触发越界作为oracle。

## 等价实验协议（提案，NOT_RUN）

### 输入身份与独立oracle

实施前冻结旧源码、旧可执行文件、模型包、工具、宿主/目标切片SHA；新侧另记提交、候选、包SHA。每条输入记录原始字节SHA-256、字节长度、来源、target/force、调用层次、所有元数据和变异描述。排序按来源路径字节序、偏移递增、bit0..7，不使用未记录随机数。oracle必须是冻结的旧tapegate及其真实调用者；不得让新旧都调用迁后规则或同一生成表而循环证明。既有参考C实现可作第三方核对，但发生分歧先保留三份结果，不能以多数票改旧契约。

### 合法与系统变异矩阵

对清点后的全部现有合法语料先跑旧侧；旧侧拒绝的输入不能假标合法，保留为独立类别。覆盖minor0/1、3/4段、origin与force、所有opcode/shape、字符串全部256字节、最短与10字节LEB、空池/空正文、引用及容量边界；语料未覆盖项在仓外实验目录补输入，不创建永久测试。

对每份合法二进制长度N：完整枚举前缀截断k=0..N-1；每字节8个位单独翻转；分别删除每一目录行、每一整段payload、目录与payload成对删除（其他字节原样保存，记录是否修复布局；修复版本是另一条输入）；补充段数/flags/off/len/count、LEB末字节与非最短、寄存器填充/tag、unit位置/缺失、引用越界、尾部垃圾、非零padding。变异“预期非法”不等于实际拒绝：有效位翻转可能仍合法，必须以旧侧真实结果为准，新侧完全一致。

逐例比较：返回码、集成接受/拒绝、原因原始字节与出现顺序、成功输出参数、text正文长度与含NUL存储、LEB游标；失败检查应保持不变的哨兵和quote允许的已写前缀。原因模型使用**有序事件序列**，派生集合仅供统计，不能排序去重掩盖首因/重复/顺序差异。模块只返回1而无更细原因为事实，不能凭测试探针新编错误类别。允许的归一化仅是事先冻结的临时绝对路径替换，并同时保存原始stderr；数字、错误文字、空白、顺序、拒绝阶段、4与1绝不归一化。

### 预算、证明与失败保留

每份N字节原料的基础变异为N个截断+8N个位翻转+逐段删除，先生成清单计数与总字节数；超预算则分批而非抽样后标全覆盖。提案每逻辑组最多10000条、累计预算1小时，但必须拆为可恢复命令：每次命令及内层子进程均≤55秒，留5秒清理，单例≤30秒；每个命令结束核输入身份、退出和收据后才接续。单进程256MiB，超时/预算耗尽/OOM一律独立失败或NOT_RUN，不记拒绝等价。32MiB正文与262144池边界仍须同资源限额比较；可另开较高资源专批测峰值，但不能替代原预算等价或以提升预算掩盖新实现内存放大。总预算须在冻结语料清单后按实际N核算，由实施收据记录，不预报已覆盖数量。

分配失败不能靠不同机器上碰巧OOM来证明：正常语义差分固定充分资源；malloc/realloc失败另用仓外、可撤销的同序号故障注入比较失败返回、输出哨兵和释放轨迹，故障序号、分配大小、旧新侧实际分配轨迹均保留。新实现分配次数变化时不强求同序号代表同语义阶段，应按池索引/正文增长阶段映射并记录；无法建立对应关系就标NOT_RUN，不能把资源差异归一化掉。API指针别名、缓冲重叠等未有合法调用证据的情况不自行扩张契约。

每次首次差异立即保存旧新二进制/包身份、输入原件、变异清单、stdout/stderr、返回码、API输出快照、超时与资源记录；可最小化，但原失败输入永不覆盖。gold是旧侧原始输出字节及事件序列，保存SHA只是索引，证明用逐字节比较（例如`cmp`），不能只比较hash、退出码或最终镜像。任何候选/模型/构造链变化都重跑受影响矩阵，不缓存PASS。

## 收益预算与候选绑定（提案，NOT_RUN）

450432B是旧tapegate**整.o**的对象大小上限，不是最终链接切片可减字节；可能含重复运行库、未链接段、调试/符号和封装开销，不能预测净收益。当前候选对象大小、可回收链接段、六切片净差、网大小、运行步数、峰值内存、gen/tbl/net及总链耗时全部 **UNMEASURED**。正收益不是等价豁免。

未来测量必须在同一宿主、相同编译参数及目标上做迁前/迁后独立输出目录，并绑定各自提交/产品输入树/候选SHA/模型包SHA/工具SHA。相位：先gen逐作业状态，再tbl/net字节和耗时，再单片对象与最终切片，再六切片/包总量与固定点。可用真实基础命令模板：`sha256sum <候选> <模型包> <tapegate对象>`、`wc -c <对象> <切片> <模型包> <result.net>`、`size -A <对象> <切片>`、`/usr/bin/time -v <该相位已存在的作业命令>`；尖括号必须从该候选实际作业清单取值，不是声称已存在此字面命令。对象比较、切片比较、包增量分别报告，净部署差为切片总差加包差；构造链记录所有command.status，不能以外层退出0代替。计时包括新增网生成/装包/回归验证，不只量快路径。

## 风险与回退

短路原因、LEB溢出/符号、游标提交原子性、指令空格转义、池生命周期、资源失败与正常损坏混淆是主要风险。不得给新网保留旧函数作长期双轨fallback：批准后在隔离候选完成差分，再一次切换全部实际调用，移除被替代规则；纯分配/IO等必要宿主原语明确保留，不冒称已搬迁。失败则整批回退该迁移的源码、生成表、网、包和候选身份，恢复冻结基线，不做单行热修补后沿用旧资格。若表达能力需要新增动作/stream，先回到设计评审，不把宿主新解析器伪装成网迁移。route热身只验证构造/回退流程，本试点不扩大到路由行为，也不触及cxmod/source_frame。

## 当前调用者、原因与现有烟测（事实）

产品组合入口是 `src/drivetapebin.cx:25-89` 的 `dv_tapebin_check(src,tgt,&gotb,&gotn,&as_tape)`：非.tapebin路径直接0且不动输入；tapebin先判目标存在、magic及总输入≤33554432，再按magic→fixed→nsec→origin(force=0)→SHA→逐行目录与padding→第四段→终点→text短路。SHA在`src/drivetapebin.cx:12-22`，覆盖b[64,n)，与头[28,60)比较。目录还加rcnt≤输入长度及off/len≤INT_MAX（`:62-64`）。因此系统变异另分两层：不重签的字节破坏检验集成SHA短路；同步重算摘要的副本用于真正到达目录/池/记录拒绝分支，身份分别记录，不混同。

该入口**失败也free原始gotb但不清空调用者指针**，成功free原字节再替换为text、写长度、as_tape=1（`src/drivetapebin.cx:79-88`）。调用者失败后必须按现有终止路径处理，不能再free悬空值。模块内失败输出不变不等于这个集成入口保留原输入所有权。`src/drive.cx:291-292` 将失败压成唯一句 `invalid tapebin or origin target mismatch`；故集成原因是零或一个现有诊断，不是收集所有内部失败。内部事件轨迹只作实验定位，不是新增产品输出；比较原有可观察单句字节优先。

现有合法材料：`tests/fixtures/tapebin/return7.tapebin`（383B二进制fixture）；`tools/stream-e3-return7.tape`（592B文本）；`tests/stages/tapecheck/{in,expect,args}`（608→592B，文本阶段）；`tests/stages/{e3/expect,prune/in,lower/in,elf/in}`。文本材料先经冻结的`tb_encode`（`cx/compiler/tapebin.cx:302-358`）得到二进制并保留二者SHA，不能直接把文本喂给UTAPEBIN头判定。`archive/testing-c43-legacy/tests/tapecheck-cases.tsv`（456行）、`tapecheck-corpus-ledger.tsv`（303源身份）是历史文本域参考，须重新判旧侧，不继承旧PASS。`tests/stages/prune/bad*`及.err是文本拒绝材料而非二进制拒绝证明。现有fixture未证明minor1/4段、全部opcode与资源上界覆盖，补齐前对应项NOT_RUN。

`tools/check-tapebin.cx:43-53`已有fixture解码再编码字节比较，标记`TAPEBIN_ROUND\tfailed=N`；它不是新旧tapegate差分。`tools/check-tapebin-wiring.cx:124-127`明确规范解码文本不等于-S原文，不能使用错误oracle。`exec/facts/tape-shape.tsv`及生成物`cx/compiler/tapeshape.cx:1-16`的72项与C侧shape须绑定同一事实表，沿用`tools/tapeshape-gen.cx --check`的现有表一致性检查而不新立第二份规则。

现有API可载入上述383B fixture，按`src/drivetapebin.cx:25-89`调用`dv_tapebin_check("return7.tapebin","lnx/x86_64",&gotb,&gotn,&as_tape)`，检查0、as_tape=1、输出文本，成功free替换后的gotb；拒绝输入另开独立缓冲且不要二次free。更窄API可依`tools/check-tapebin.cx`既有fixture读取/目录构造调用`tb2_text`后重编码；不得省略目录/SHA就声称集成门通过。父已实跑上述两个既有工具作为基线；本段额外直接调用示例未单独执行，新旧逐例差分仍NOT_RUN。

## 表达能力核对与具体迁移提案

### 现有能力与不能直接替代之处（事实）

已有`tapecheck`是**文本**Tape字符语法网：成功EOF后原样输出输入，拒绝给物理行和原因；不做容器头/目录/SHA（`exec/tapecheck/README.md:8-19,25-29`）。其A64/C64U有界整数模式已存在（`:55-64`），可以复用模板，不应将“多函数”作为新增网理由。现有`st_span`整段执行入口在`src/stages.cx:249-318`，stream STAGE同走该路径（`src/drivestream.cx:396`）；无需为“流式字节”再新增一种stream。`cx/compiler/stream/*.cx`与`net/*.cx`的骨架不作为能力证据。

执行器输入帧与字节EOF观察已具备：mode0是当前字节/256，mode1栈顶，其他mode看比较结果0..256（`cx/core/execute.cx:188-222`）。A64/A64I、C64/C64U、稀疏内存LDX/STX、字节输出、段复制、受限输入子帧INPUSHXE、CORE_BYTE/XLEN均存在（`:110-168`）。**LDX不是直接读取输入字节**，它读机器稀疏内存；输入用CORE_BYTE/COPYT或受限子帧读取。寄存器能存计数和64位累积值，不需为262144个池元素创建262144个状态。INTERN会去重，不能当保留重复条目与序号的原池等价物。

现有网内容不能直接复用为二进制验证器，但现有执行机制能表达大部分规则；单整数观察并不妨碍多寄存器状态。SHA仍留外部，不能以缺rotate为新网硬障碍。输出Buf自动增长并不自然遵守quote cap，须网内逐步检查。ACCEPT/REJECT只有成功/单原因（`:175-176`），必须由窄适配层保留0/1/4和失败不发布输出语义；不能改整个stream的错误合同。

**真实硬缺口**：通用core_run分配regs与inputn+1个I属性（`:195-200`），OOM走panic；这不等价于tb2_text分配失败返回1。新网还可能显著增加32MiB输入的属性内存。批准实施前必须证明适配器能以现有资源接口实现可恢复分配、清理及所有权，或者单独评审最小执行器扩展；否则完整替换被阻断，不能接受“正常样本都一样”就切换。步数需实测；32MiB并非自动超过200M步，不能用错误不等式决定新stream，实际多动作/字节才是预算风险。

### 选择（提案，不是已实现）

复用现有整段输入/执行器/观察与动作体系，新增一个职责单一的**二进制tapegate网**（名称须按现有登记规则在评审时确定），不新增stream，不改变文本tapecheck规则。理由是其语言、输出与原因合同不同，而非函数数量。适配器只负责借用输入、显式参数/操作标签、内存和返回值发布、SHA及原调用顺序，不在宿主重写LEB/池/记录规则；全部旧公开tb2_*操作均通过同网的操作入口覆盖。入口参数编码为有版本的实验帧，保存原字节长度和区间，禁止以C结构内存布局跨目标传递。非法空指针等4在进入网前判定；网结果结构为状态码及暂存结果，只有成功才提交原API规定的参数。quote特例按原动作顺序复制允许的前缀。

状态族：INIT/按操作分派；MAGIC/FIXED/NSEC/ORIGIN；DIR_FIELD/ALIGN/PADDING/EXTRA/END；ULEB/SLEB；POOL_COUNT/POOL_LENGTH/NAME_BYTE/POOL_END；RECORD_KIND/UNIT/REF/BSS/OPCODE/REG_NIBBLE/TAG/ARG；TEXT_QUOTE/TEXT_DECIMAL/TEXT_INSN；COMMIT/REFUSE。返回子程序用栈及寄存器返回标签，计数由寄存器而非展开状态。每个状态明确EOF拒绝或成功，不依赖no-transition代替原失败码。

观察量：当前输入字节与EOF、比较结果、返回标签；寄存器存全长、绝对/段游标、minor/target/force、目录行/字段、64位off/len/count、LEB的v/shift/k/前字节、池计数与索引、unit/row/op/arity/寄存器半字节/tag、输出已用量/cap。两个池用稀疏内存按`池基址+2*index`保存start/size，保留重复条目、空常量与原序号；不同临时区地址不重叠，输入不复制到稀疏内存。

动作模板：

1. 定长LE字段用CORE_BYTE→A64按位移/或累积→ADV，先检查段剩余；8字节处理用无符号比较，不在32位INC中累积值。
2. LEB每轮检查end/k/第10字节限制，再并入低7位；结束检查最短形式；SLEB按旧条件符号扩展；失败不提交外部pos/value。
3. DIR先计算对齐与减法形式的边界，完整行通过再保存；padding逐字节0；池长度通过再写start/size与推进。
4. 记录与shape按同一72项事实生成opcode分派；寄存器拆半字节，tag选择LEB子模板，unit/gdef按原状态。不得借用文本tapecheck重新判定输出来替代二进制规则，二者接受语言不完全同义。
5. 文本用常量OUT/OUTW/SPAN2组合，数字以A64无符号除余10逆序暂存再输出，负值绝对量用无符号位模式避免INT64_MIN溢出；**不使用只处理uint32的DIVMOD10**（`cx/core/execute.cx:170`）。转义及hex直接复用既有字节映射模板；每次输出前执行与旧helper相同容量条件，保留quote两字节hex逐对容量检查和前缀失败行为。text正文≤33554432且另有NUL。
6. 临时文本/索引在拒绝后丢弃，成功只提交约定字段；新网不得把内部详细REJECT字符串泄漏替换产品唯一句。适配器正确映射timeout/panic等基础设施失败，不能映射为旧返回1来伪造等价。

gold证明同时做操作级与组合级：逐状态模板用旧公开API产生码/参数/字节gold，完整二进制走旧dv_tapebin_check产生最终文本/诊断gold；新侧同样参数、同样SHA检查顺序逐字节比较。内部轨迹是定位辅助，不当旧API原因。完整域合法语料与系统变异矩阵通过且OOM/所有权缺口关闭后才允许切换。现有门禁只能复用fixture、标记、逐作业状态与比较框架，不存在已完成的新旧tapegate等价gate。

## 实施清单（未实施）与已运行的既有基线

当前外部tb2_*调用点只在生产`src/drivetapebin.cx:50-76`、活工具`tools/check-tapebin.cx:29-43`与`tools/check-tapebin-wiring.cx:106-120`。`tb2_quote`等部分公开函数的外部测试只在`archive/reference-c/tools/check-compiler-units.cx:629-667`，不是当前活gate；不把归档断言算已测。迁移时更新上述活调用者与生成/打包引用，并决定无活调用的公开接口是整体删除还是保留为迁后适配；不得留下旧规则别名或双实现。归档是历史oracle，不按活产品调用者迁写。

构造绑定在`tools/entry-build.cx:734-776`：tapegate.o对应`src/compiler/tapegate.cx`（`:754`），drive.o对应`src/unisacc.cx`（`:734`）；闭包对象键由`:891-900`按plan顺序生成mN.o，**m0不是第三个源模块名**。旧收据`out/c44-m2/receipt.txt:50`中m0.o对应src/drive.cx，但跨候选不能固定该编号。`src/compiler`与cx/compiler的镜像/链接关系须随候选输入解析身份登记，不另写一份产品实现。对当前源的行数和SHA观察不能替代该候选的对象依赖实测。

后续文本路线`src/drive.cx:735-750`交给st_span；它消费输入所有权，成功输出交还调用者（`src/stages.cx:249-418`）。读文件失败在进入tapegate前由`src/drive.cx:275-289`及`src/entrybind.cx:136-153`诊断，含cannot read/close failed/file too large/out of memory，不能归进tapegate原因。现有`tb_encode`在`src/drive.cx:380-386`失败会保留文本输出，不要误认--tapebin选项必然产出了二进制；实验务必核对magic和产物SHA。

父已运行最小既有gate命令：

```sh
sh ./unisacc.com tools/check-tapebin.cx
```

这是既有fixture往返，实跑`PASS byte-identical bytes=383`及`TAPEBIN_ROUND failed=0`，不是迁后等价PASS。正式批次登记在`tests/batches.tsv:337`。已有产品接线烟测亦已运行：

```sh
sh ./unisacc.com tools/check-tapebin-wiring.cx -- .
```

实跑`TAPEBIN_WIRING failed=0`，登记在`tests/batches.tsv:341`；它调用当前已装产品，不是未来c46迁移候选的资格。`tools/link-malformed-input.cx:29-43`另有截断/翻转/minor损坏的具名拒绝路径可复用，但该工具还覆盖其他格式且存在已知红项，不能把整项当成tapegate独立通过证据。该malformed工具本轮未执行；没有创建永久测试。

最终切换的验收必须同时满足：所有活调用者/生成入口已迁；合法与变异全清单逐例一致；首因/原始诊断及文本逐字节一致；API失败发布与内存生命周期一致；资源失败合同已解决；新网/包/切片及构造链实际增量已量；同候选预检与fable批准有收据。任何一项未运行仍标NOT_RUN，不能用route热身或库存盘点的PASS补位。
