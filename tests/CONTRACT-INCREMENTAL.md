# 0.0.43 动态 C 合同改造

范围限定 tests/ 与 release/tools/；不改变产品闭包、候选身份、gatequeue 指纹或 gatedeps 分层。每项命名闸独立验收，旧队列不续跑、不回填结果。

## ABI 推导

动态 C 入口：`sh ./unisacc.com -run tests/abimanifestcheck.cx -- --check`（生成用 `--write tests/abi.manifest`）。Python 两文件仅是已登记的逐字节对照。

初始化列表按词法屏蔽后的括号/方括号/花括号栈切分，仅顶层逗号分项；函数体配对忽略注释、字符串与字符字面量。未闭合注释/字面量、失配分隔符明确拒绝。guard 只读取外层 Windows/POSIX 两分支，嵌套 else 不抢外层分支；额外外层 elif 明确拒绝。loader 条件记录每个 elif 及此前分支，else 记录完整排除链。

自测含 f(a,(b))、复合字面量、引号/注释花括号、嵌套 else，及失配/未闭合负例；源码突变含 elif loader 新槽位和不支持外层 elif guard。清单头以 .cx 标识生成/检查入口。当前396条内容逐字节对拍。

## 后续独立项

APE正式槽位绑定；早段止于lex的封存数据及pp-literals叶子样板。封存不是跳过新候选行为闸的许可；依赖/产物/协议/拒绝覆盖变更必须失效。尚未接入指纹复用。

## APE 正式槽位

staticimage.cx 只读首4096字节内启动脚本的 case $u Linux 两槽位（tail 的偏移是1起算），检查槽位唯一、边界与互不重叠，限定长度中必须是完整gzip并校验CRC，然后要求槽位架构相符、PT_LOAD存在、无PT_INTERP/PT_DYNAMIC。不扫描全文件gzip；追加静态诱饵不能替代正式坏槽，正式槽之外gzip不参与计数。未知启动表格式fail-closed，不猜测新容器布局。

夹具含缺表/重复label/未知Linux槽、越界/重叠/长度/CRC、架构错误、坏正式槽后追加静态诱饵；保留ELF动态段与头越界负例。实际检查历史8b792a08动态候选，两架构正式槽位均被拒绝。Python仅登记的结构结论对照，闸入口保持.cx。

## 早段封存 v1（先合同，不接指纹）

首批样板的保守落点止于 lex；这不是抽象不可拆的最大边界。核心按输入字节流、模型调用/流式推导和输出字节流切合同，宿主及外部符号绑定不属于核心语义身份。样板先钉输入源码、include头、目标预定义、flags、规则表/构造器、共享执行器与资源；当前实现把适配逻辑混入共享代码的广依赖是实现债，不是不能继续拆 parse2 等阶段的理由。exec-pp-literals 是 E2 叶子，其协议是普通预处理文本与 UNIPP1 located封装；叶子的封存不宣称验证了E1/lex。

数据为 UTF-8 TSV，禁止空字段/绝对路径/..路径/软链接：

```
schema	early-stage-seal-v1
leaf	exec-pp-literals
stage	e2
early-boundary	lex
scope	measured
review	required
target	lnx/x86_64
flags	--locations
protocol	pp.text+UNIPP1
coverage	named-sample
file	input	case.c	字节数	sha256
file	rule	rules.tsv	字节数	sha256
file	builder	gen-source.c	字节数	sha256
file	executor	run-source.c	字节数	sha256
file	resource	predefines.tsv	字节数	sha256
file	oracle	compiler-identity	字节数	sha256
file	environment	environment.tsv	字节数	sha256
file	accept-output	accept.out	字节数	sha256
file	refusal-output	refusal.err	字节数	sha256
case	octal	accept	case.c	accept.out	0	-
case	overflow	refuse	bad.c	refusal.err	2	named-limit
```

每个case输入也必须列file/input，accept和refuse分别要求匹配output角色；refuse非零且具名原因，accept只许rc0。输入文件身份与字节数全核，不把UNKNOWN/BLOCKED当拒绝覆盖。至少一个接受及一个拒绝例；coverage明确为named-sample，不能推为穷举。

文件清单需由后续传递读取审计补全（含CC包装器、cc1、系统headers、默认env、模型生成器递归表）。`review=required`永久保留本批的未授复用状态；清单自洽不能证明没有漏依赖。检查器不执行叶子或重算行为，依赖同一候选runner的执行收据另列，不与旧整链PASS互换。

`tests/earlystageseal.cx --selftest` 生成以exec-pp-literals命名的显式fixture包，含017接受与溢出拒绝记录，验证修改各角色、输出/拒绝、漏类、case rc及路径/软链接必红。fixture只能在自测模式核验，正式--check拒绝scope=fixture。只有后续实测trace+完整closure+同源行为记录才能提供scope=measured材料，本批不伪造实测seal。

## 核心与观测证据分离（政委纠正）

IO抽象为字节流，函数抽象为调模型流式推导。外部符号的地址/加载、provider、宿主进程/文件系统、参考cc和环境属于适配/观测层，不能倒灌为核心阶段身份。类型/声明若影响语言输出，应作为显式流字段/资源输入；宿主实际解析和绑定仍在边界外。

v1 的file角色已经把oracle/environment与input/rule/builder/executor/resource分列。检查所有记录是否自洽是**整份证据包核验**，不表示这些记录全参与核心身份。未来核心投影只取纯流输入、规则/模型、纯执行器/构造器、语言资源与协议参数；观测投影取oracle、宿主适配器/provider、执行环境和比对结论。oracle/env变化只使相应观测证据过期，不能自动宣布核心流模型改变。当前没有把两种投影接入stamp，勿以全包hash代替核心hash。

须审计纯执行器与适配器的真实读取边界；临时把legacy run.c整体钉住只能记作未拆完的保守实现约束。验收反例：改变宿主oracle后核心投影不变、观测投影变化；改变语言资源或模型规则后核心投影及下游流证据变化。新候选装配/适配层的行为义务独立绑定候选，不能误用核心复用去免除。
