# lower 拒绝域合同草案：归属待裁

只是设计；以下源码位置已只读核对，非全部拒绝类穷举。NEG-001的既有实测见c43-middle-step0-receipt.md，本批动态C仅核验/登记其冻结证据，不重新跑模型或参考、不判断发布通过。四族按输入协议/数据记录/引用结构/目标能力整理；未找到可读取的cdx2四族仓外蓝本，不冒称已逐条对齐。

## 参考的拒绝链与当前归属

|输入类|当前拒绝拥有者及文件:行|独立lower合同归属说明|
|---|---|---|
|预处理条件字面量/表达式错误；不可提供的include|pp：unisa/front/pp.py:97/100（整数后缀/八进制）、:305/307（残留token/除零）、:563（缺include）|属于pp输入资源/规则合同；适配器提供bytes或absent，不能由宿主查找结果隐含决定|
|非法字符/游离标点|lex：unisa/front/lex.py:324/326|属于lex字节与词法规则，不要求lower接受C源后替lex兜底|
|语法错误、未声明调用、类型/初始化约束|parse及其集成的类型语义：unisa/front/parse.py:183/:285/:603/:913|先由上游语言规则拒绝，不能靠lower表重构完整C语义；拆后typed流须显式且可验证|
|族1：未知Tape opcode，NEG-001=?\n|Tape解码/验域：unisa/tape.py:335；目前在exec/lower/fullcheck.py:19的parse→lower前置调用发生|这是本次归属争议的第一条，不是已经裁定lower内部必须新增哪个guard|
|族1：操作数数量、非法寄存器、整数编码|Tape解码：unisa/tape.py:337–345（数量/r0..r7/int转换）|语法/形状规则必须显式拥有；与目标寄存器分配是两个合同|
|族2：.unit/.global/.extern/.gdef记录、版本前置|Tape解码：unisa/tape.py:300–309|字节记录/结构一致性；.gdef与.unit2关系不能靠口头“前级必正确”跳过|
|族2：.str引号/转义/容量，.bss大小文本|Tape解码：unisa/tape.py:289–294/:315–318/:237/:244/:252/:254|结构错误和资源界限归流解码；界限必须版本化，不把宿主内存不足当语言拒绝|
|族3：标签/入口/引用结构|现有parse在unisa/tape.py:325–328收集label，并未证明完整重复/悬空引用拒绝；lower消费标签在unisa/lower.py内部|待审项，不宣称已实测会红；声明解码和目标重定位各自规则须分开，不以名字推拒绝归属|
|族4：目标系统调用/ABI能力、hostaddr索引|lower：unisa/lower.py:232/:244/:266–268（无目标门/栈参）、:327/:329（宿主调用目标限制/索引）|目标合同作为显式输入；这些拒绝依赖目标事实，不能归给不见目标合同的纯Tape解码器|

这些是参考“流水线拥有谁的拒绝”的清单，不能拿上游语法错误直接当lower模型已声明拒绝域的证据。也不能因此将公开的raw-tape入口绕过前置验域后仍接受任意字节记成与参考一致。

## 只提出两种归属方案

**方案A：前置Tape验域阶段拥有族1/2及已定义的族3结构规则。** 核心输入是raw字节＋Tape语法/shape/记录规则模型，输出版本化typed-tape流或明确拒绝；lower只接该typed流＋lower模型＋目标/外部合同。公共raw-lower路径必须经过此阶段；独立调用必须携带可核验的同输入验域结果，不能仅带“trusted”布尔标记。族4仍由lower拥有。代价：新增显式边/typed格式或可核验覆盖见证，所有直接lower调用都要改；需要负例证明绕过/陈旧验证不能绿。好处：语法规则单处拥有，复用边界清楚，前级错误不污染目标lower语义。

**方案B：lower公共入口拥有raw-tape验域＋目标lower的联合合同。** 输入raw字节＋明确的Tape规则和lower模型＋目标合同；进入GENERIC前拒绝未知opcode/坏shape/记录。上游仍可拒绝自身源语言错误，但独立lower不能依赖上游隐含保证。代价：需要把现有lower模型/入口覆盖与前置参考规则对齐，规则更大且可能复制已有Tape语法知识，ABI和语法变化的重验边界需再拆清；候选身份必变。好处：独立raw入口自足，NEG可以直接按该入口验收。

两案都不允许宿主Python偷偷充当核心语法阶段，也不允许直接把?\n换成其它会红输入来消掉NEG-001。到底选择A还是B、族3完整规则与兼容范围、raw入口是否属于产品承诺，均由主人裁定；本批不代裁、不动产品。

## NEG-001：已测状态、不是修复或通过

模型接受rc0、106字节输出；参考parse→lower前置拒绝rc1、stdout空。input SHA ce773b87709a04bbcb0ead74fea94b1f20fa4a4d185fc06a24a9bc703dd99613；model stdout SHA 034ed2cdf94706615f4cad4cae78c5dd8d31494f140b9aa05f71d2c8660cd286；参考stdout SHA e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855。

定位：code-scan-result.tsv:4存任意op；code-entry-result.tsv:64走GENERIC；code-print-result.tsv:10/:6–7及code-print-byte.tsv:4原样拷贝，对比tape.py:335的SHAPE校验。疑似拒绝域/公共入口契约不齐，待主人定归属；不重分类为PASS，也不扩大到E3所有流。

本批入口tests/lowernegledger.cx只读取冻结数据并登记EXPECTED_RED/NEEDS_RULING，完成记录返回77，不是宿主跳过绿，也不进入gate/gatedeps。本登记器绑定input.bytes、output.bytes、status.txt、comparison.tsv、reference-diagnostic.txt五份判定证据；长度或整文件字节改变返回1且停止。capture.stderr仅是采证包装器的“unexpected stage verdict 0 wanted 1”日志，不提供模型/参考rc或参考诊断，属观测层，本登记器不绑定它，也不对全部目录文件宣称完整性。收据须区分：既有模型/参考实际跑过；本批C登记实际跑过；两案只是设计。

## 本批实际运行收据

`sh ./unisacc.com -run tests/lowernegledger.cx -- tests/seals/middleflow-step0/lower/red` 已实跑：rc77，输出 NEG-001 RECORDED EXPECTED_RED NEEDS_RULING。它核验的是冻结输入/输出及既有参考rc记录；没有重新执行lower或参考，没有给拒绝域所有权作裁定。C入口源码SHA256 c85811b10794c1942dbf19c05d56ab8051d461315cd07ea21ca1c688e007ddb0。原NEG模型/参考实测见第0步收据；本稿阶段归属与两种方案均只是设计。

## 绑定修订

comparison.tsv（319字节）与reference-diagnostic.txt（39字节）改为整文件SHA256核验，已移除strstr子串判断。cx_read通过include/cx.h:44–48调用_cx_slurp，后者在:39写buffer[len]=0，返回缓冲有末尾NUL；原子串搜索没有因缺终止符越界，但无法证明整文件完整性，本次不再依赖它。capture.stderr仍由外层封存manifest记录，但不影响该NEG登记的判定证据。

绑定修订实际验证：原冻结目录rc77；五份判定文件逐个追加1字节均rc1；comparison.tsv和reference-diagnostic.txt各作一次等长替换、一次插入，均rc1；capture.stderr追加1字节仍rc77，符合声明的观测层范围。测试由仓外动态C篡改驱动执行，原冻结数据未修改，模型/参考未重跑。修订后lowernegledger.cx SHA256 7b6ebfa2e7e391ffaceb5b7e46c3c90eb2f24513a375f227aba581498b6caaaf。

## 04:47 四族索引交叉（只读源码核对，未跑新套件）

本次从cdx2巡检patrol-10m-20261014-0416.md/0447.md读到四族转述：缺syscall/Win import、非法hostaddr、extent溢出、坏bindings。它是索引转述而非原蓝本全文；与本文按字节层次分的四族不是同一分类，不把二者名字强行对齐。下表各项只做源码/断言覆盖审计，未把静态规则当实际NEG通过。

|cdx2四族|对应本文归属|可读证据入口与断言|当前缺口|
|---|---|---|---|
|缺syscall/Win import|族4目标能力，lower拥有|unisa/lower.py:232/:244；exec/lower/fullcheck.py:105–114：参考必须以点名ValueError拒绝，两个执行器均rc1、空stdout且stderr以reject: not covered:开头|已有三元断言，但本次未运行；不能替代其它目标/接口的NEG|
|非法hostaddr|族4索引/目标规则，lower拥有|unisa/lower.py:325–329有目标限制/0..3规则；tests/unisatapecheck.sh:90只是索引2的合法序列化正例；libraryexitcheck.py:64–68是Windows索引0接受正例|未找到索引-1/4等完整输入→参考/模型rc、空stdout、具名stderr的一组NEG；不得把正例/源码规则计作已覆盖|
|extent溢出|族2数据记录界限与lower布局接口|exec/lower/sparsecheck.py:30–34的单项2147483648和累计2000000000+1000000000，两个执行器断言rc1、空stdout、含tape data directive诊断|已有三元方向；stderr仅子串断言，尚非完整诊断协议/逐字节绑定，本次未跑|
|坏bindings|族4显式外部合同能力|exec/lower/libraryimportscheck.py:45–47接受分支比stdout，拒分支只核rc1；:56–69含缺失/截断/超宽/签名/模块/寄存器形状等控制。libraryexitcheck.py:45/:61拒分支也只核rc1|仍缺拒绝时stdout为空和具名stderr的强断言；truncation全位置只核simulate的reject，本身不证明native完整拒绝合同|

hostaddr另外有需要留在待裁表的张力：lower.py:327参考限制宿主ABI到osx/lnx，而libraryexitcheck.py:64–68显式要求Windows .hostcall/.hostaddr0模型接受。这里只指出源码口径差异，不把任一方当自动裁定，也不在本批修改正例或产品。

### 建议拒绝记录口径（只是设计，未实施）

具名域外拒绝证据应至少满足：`model_rc==1 ∧ stdout长度==0 ∧ stderr属于该NEG ID约定的具名拒绝诊断`，并绑定输入/模型/显式资源、参考rc与stdout/stderr哈希。诊断可按家族允许既有reject前缀或位置化error格式，但必须点名原因、排除Python traceback/包装器报错；不能用“任意stderr非空”替代。精确schema/是否保留部分输出仍须主人审合同，此处不静默放宽。

超时（124/142或fuel耗尽）、信号/崩溃、启动失败、工具缺失、job/harness的rc1均不算具名拒绝。应区分内层模型rc与外层测试rc，保留OBSERVATION/UNKNOWN/BLOCKED状态，不混进拒绝样本计数。建议未来动态C合同补这些检查；本次未改Python、未叠新工具、未入gatedeps。

NEG-001 ?\n仍模型rc0/参考rc1的待裁红；加强bindings记录不能把它消掉，E3见证范围不扩大。A/B与raw-lower承诺依旧待主人裁定。

## 04:58 非法hostaddr完整NEG的最小采证形状（只是设计，未运行）

已直接只读对照仓外cdx2/observe-10m/bindings-refusal-evidence-gaps.md；该页明确区分进程拒绝与API错误，且指出libraryimports拒分支、全截断模拟分支、注册API错误不能统称同一种rc1证据。本节只提出缺口用例，不修改检查/产品、不采新模型。

优先选择 **target=osx/x86_64**，避免Windows平台支持争议、Linux绑定模式的其它拒绝分支遮住索引检查。必须使用该target的确切lower模型，记录来源/完整哈希；不能用已封存lnx模型却把账写成osx。若该模式不支持下面合法控制，应停止并列入口/模式卡点，不换目标把别类拒绝冒充索引拒绝。

|用例ID|完整原始tape字节（反斜杠n表示LF）|角色|
|---|---|---|
|HOSTADDR-CONTROL|`_start:\n.hostaddr r0, 2\nret\n`|同目标/模型/资源的合法索引控制；必须先确认参考和模型均接受及可对拍|
|HOSTADDR-NEG-LOW|`_start:\n.hostaddr r0, -1\nret\n`|低边界域外；仅相对控制改变索引|
|HOSTADDR-NEG-HIGH|`_start:\n.hostaddr r0, 4\nret\n`|高边界域外；参考规则为0≤index<4，首个最小NEG可只取这一条|

每条记录：原始input.bytes及长度/SHA、target、匹配model SHA/来源、逻辑源名、执行燃料、逐key显式资源bytes或absent、参考与模型各自rc/stdout/stderr及长度/SHA、执行器/工具观测身份。沿现有fullcheck的parse→lower参考入口，两个NEG的预期参考校验点是unisa/lower.py:329的`hostaddr index must be 0..3`；任意traceback不算该校验点证据。模型须为正常拒绝rc1、stdout长度0、stderr为该入口约定的具名拒绝（例如已有not covered: <target> lowering类别），不得接受timeout/OOM/未知资源ledger/参数解析错误作同等证据。若仅给通用lowering诊断，须用合法控制与只改索引的配对保留因果范围，不声称模型已点名index这个子原因。

参考的Python ValueError进程可能携带traceback；保留原件及具体预期校验点，不能把它误要求成native的reject前缀，也不能只看rc1。API错误另按非零返回＋具名error＋无registry副作用记录，不能硬套整个进程rc1/空stdout。查询未记录不是显式absent。

首红停：合法控制失败、参考拒绝点不符、模型接受NEG、非空stdout、诊断错类、超时/崩溃任一出现就停止并保留原件；不能改输入/资源后继续消红。HOSTADDR-CONTROL与这两条NEG均未实跑，本节不关闭原缺口。NEG-001 ?\n仍待裁红，A/B不选择，E3范围不扩大。
