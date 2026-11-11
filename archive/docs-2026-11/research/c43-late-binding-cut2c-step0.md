# 晚绑定 2c 第0步｜剩余入口与拆分顺序（只读草案，≤1.5页）

**证据边界：以下现状均为“读源码”，建议均为“只是设计”；未跑实现/验收、未改仓库。** 以当前工作树及已审2a/2b边界为准，不把cdx在途lower算作完成。2b只保证普通函数片段；2c结束前仍不能声称完整E3/lower或所有RI无地址。总体建议：数据 → fixed typed → variadic → callback → 运行服务与上下文；先给后续步骤立显式模式/目标/能力合同。

## 1. 剩余入口（读源码；位置为当前行号）

|类别|何时进入、今天的用途|
|---|---|
|数据符号 kind=1|`librarybindings.h:44–48,116–118`存地址/extent/writable；`libraryresolver.h:147–152,194–228`冻结进USBIND3；`libunisacc.c:608,648–649`在E3前送bindings。E3已有符号式`.libraryaddr`（`parse2/librarydata-template.tsv:2–14`），数据地址真正打印到lower在`lower/librarydata-result.tsv:97`；因此不是所有数据都在E3写地址立即数。|
|fixed typed / variadic / callback函数|`libraryresolver.h:128–144`在compile时用真实目标地址创建plan/template，wire同时带address、dispatcher、handle；`libunisacc.c:608,615–616`保存，E3 `libraryimports-result.tsv:239,252–253`解码、`:505,516`把plan/dispatch写入Tape。variadic callsite流随后在`libunisacc.c:696–697`装成可执行计划，执行`:849–870`按handle/site找计划。|
|callback及callable服务|callback图计划由`libraryresolver.h:134–136`分流；E3 `librarycallables-result.tsv:28,183`读取原始函数地址/计划并写helper。此外`libunisacc.c:627,652–653`在E3前提供callablemake/call两个服务函数地址；relocate又在`:1005–1007`提供同两地址。服务dispatcher与外部callback目标是两类所有权，不能混成一个符号。|
|exit/mmap/munmap服务函数|`libunisacc.c:1005–1009`在prune/lower前传桥地址；`lower/libraryexit-result.tsv:3–8,25`、`librarymemory-result.tsv:5–6,12,18,25`按非零地址决定服务/原始路线并打印地址。`.sys6`私有帧同样嵌入回调：`codeprivate-result.tsv:39–47`、`codeprivate-manifest.tsv:4–5`。|
|process_slots、argv、mapping上下文|`libunisacc.c:996–1009`传mapping.base/reserved、argc/argv及process_slots地址；`:1025–1026`启动含lower的run路线再交memory。`librarymodule-result.tsv:25,32`直接打印process_slots基址；`libunisacc.c:1234–1236`每次run_main更新其argc/argv（已存在image也可重用），不能冻结成一次编译的argv快照。mapping实际用于`:1028–1031`提交/写入/保护镜像。|
|四个loader槽与Windows OS入口|同一`:1005–1007`还传process/dl/0..3真实函数指针，应沿首刀UNISYM1晚绑定槽位，勿重定义`.hostaddr`；Windows额外`:987,1019–1020`枚举winimports加入实际RI，须纳入模式/目标/能力审计，不能只查17个scalar。|

## 2. 每步可独立验收（只是设计）

共同规则：合同从全部声明生成；分区只看kind/callkind/版本/显式能力，地址缺失不改归属；新旧双归属/缺归属拒绝。provider、库句柄与计划寿命留绑定层；缺provider时合同、E3及未绑定lower仍可读，绑定具名失败、不公布可调用镜像。**每步A/B只比较其模型标定片段及需求，不能人工裁字节后称整流无地址。**

1. **数据**：沿USLCON1 `kind=1,id,name,extent,writable,sigv/type`；保留现有六操作数`.libraryaddr`，lower从合同name→ID产生占位/span。可沿UNILOW1现有`role=0`地址记录，无须借用普通函数操作名。验收A/B数据地址不同而合同/E3数据引用/未绑定lower同；绑定后读写各自对象，readonly/extent/类型冲突与missing-provider具名拒绝。
2. **fixed typed**（先不含callback）：USLCON1 `callkind=1,nargs,result/args,USLSIG3`；E3输出稳定计划需求及ABI证书，原生plan只在绑定后实例化。需要dispatcher、plan-handle新角色；版本化UNILOW框架/角色表，旧reader拒绝未知角色，不能偷偷改现有role=0含义。A/B改变目标及计划实例，片段/证书同，绑定后各执行；独立provider签名不符拒绝，复制同一签名或legacy不算ABI证据。
3. **variadic**：`callkind=2`加固定参数数、promoted-tail类型、稳定callsite ID与模板/ABI证书需求（另附无地址callsite帧，不往USLCON1塞实例handle）。沿上一步版本化重定位；A/B template/dispatcher不同仍同片段，绑定后tail结果正确；缺site/provider、未覆盖promotion具名拒绝。variadic callback仍留下一步或明确拒绝。
4. **callback**：`callkind=3`、完整嵌套签名图、调用约定、同步/生命周期/可重入能力；不能把图本地ID当ABI。绑定后建立closure/alias和清理所有权，服务桥用保留ID、外部目标用符号ID；A/B两者均变而预绑定片段同，结果/错误/退出传播正确，失败与compile/free回收无悬挂调用。
5. **运行服务与上下文分迁**：先exit→mmap/munmap→callable服务，使用保留bridge ID、固定ABI及显式能力（可沿USLCON1 kind=2概念，但当前生产输入只接受kind≤1，须版本化扩展）；缺服务报missing-runtime-service，绝不能因地址为0退回raw gate。随后process_slots/argv，再mapping；上下文引用用明确context-slot角色，不能冒充外部symbol的role=0。缺上下文报missing-runtime-context，A/B映射/argv/slots不同仍未绑定lower同，重复run_main切换argv正常、失败释放预留映射。

## 3. 运行适配器、Windows与禁走捷径（只是设计）

**判断：分开。** exit/memory/callable的入口是运行服务函数，可以有声明合同+晚绑定provider；mapping、argv、argc、process_slots是实例状态，应是**lower之后运行适配器的显式输入**，有slot schema、长度/边界/所有者/有效期，不进外部符号解析器。lower只看静态模式/目标/能力与稳定槽位；实际地址/argv在绑定或memory阶段进入。共享重定位校验框架可以，但provider命名空间及生命周期须区分。最后才允许断言全lower输入无宿主地址；memory/执行输出本来就含实例地址，不能拿整镜像A/B相等作标准。

**Windows L-4**：先立顶层显式library/program模式、目标、能力集合及桥ABI版本，服务/raw路径由合同决定；不是“见地址即库模式”。`librarycontract.h:5–8,21`当前target仅四个POSIX值，须版本化扩展Windows目标/能力，不能把Windows伪装成POSIX。数据/typed步可逐类接Win能力；运行服务步补Windows内存/退出ABI（现`libunisacc.c:874–901`与POSIX不同）。只有对应Windows合同的参考/模型接受与具名NEG、真机运行证据齐，才逐项关闭L-4；ASLR片段相等、交叉编译成功均不够（`spec.md:831`）。

**冲突做法**：清零地址却保留provider派生SUPPORTED/handle作“无地址”；C扫描Tape补立即数或自行选ABI；按dlsym成功/非零地址决定分区/回退；data借`.libraryfn`或给`.hostaddr`增私用槽；把运行实例状态塞进符号声明；全类一笔迁完、缺provider时直接执行原始调用；零/自复制签名计ABI绿。这些分别破坏确定性、模型归属、模式合同、所有权或可独立验收。

## 审读（cc，2026-10-14）

认可拆分顺序（数据 → fixed typed → variadic → callback → 运行服务与上下文）与“分开”的判断：运行服务函数走声明合同＋晚绑定；mapping、argv、argc、process_slots 是实例状态，作为 lower 之后运行适配器的显式输入，不进外部符号解析。每步在 2b 落地后单独开第 0 步，按本稿的验收形状写黑盒。Windows L-4 按本稿逐项关闭，不以交叉编译成功或片段相等计绿。
