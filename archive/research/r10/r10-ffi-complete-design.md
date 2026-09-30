# R10 完整 FFI 下一步设计审计

状态：**只读发现与设计；没有运行编译、测试、构建、网络、VM 或索引。** 本报告是唯一交付物。检查了 AGENTS.md、prd.md R10-5 与库实施记录，以及下面所引源文件；既有 PRD 实测记录是历史证据，本次不重新确认。

## 结论

下一项应是**完整类型图 + 具体调用点签名 + 双向共享调用帧**，不能仅把 args[6] 扩为 args[8]，也不能只增加 ffi_type_float/double。现存脚本 ABI 已能表达 FP 位串、聚合地址及全栈参数；桥和声明协议丢失了让它们可安全适配的关键事实。

建议先确定两个不可混淆的层：

1. C 类型/调用语义全部由 E3 δ 给出：完整原型、布局、转换后的实参类型、聚合值复制、函数指针类别、脚本寄存器或全栈模式、符号胜者、返回值表达。
2. 原生 ABI 是公开且可信的机械契约。宿主可以把完整**已声明**类型图转换为 libffi 类型、调用 ffi_prep_cif/ffi_prep_cif_var 并搬运字节；不得读取 C、选择原型、猜尾参、选择符号或按 shape ID 决定布局。若希望 ABI 分类本身也完全 δ 化，采用本报告的 NativePlan，通用 assembly 只装载寄存器/复制原生栈/调用/收集返回。

最小可实施主路线：共享 ValueFrame + ScriptPlan，以 libffi 完成支持的完整固定签名及导入变参；NativePlan 作为 libffi 不能完整表示的 union/bitfield/packed 等聚合的同框架后端。不要把 libffi 能接受的普通 struct 子集称“所有聚合”。若当前产品承诺包括这些已支持源码类型，则应直接把 NativePlan 纳入完成范围。

## 1. 源码现状与真实堵点

- `exec/c/libraryexports.h:20-28,56-59,101-105`：invoke 输入六个 uint64、输出一个 uint64；args[8]、ffiargs[6]；导出必须外部/已定义/supported/固定≤6。ffi_type 只覆盖整数、普通指针、void。扩 float 分支仍无法修复 invoke 的聚合及全栈约束。
- `exec/c/librarycall.h` 与四个 `librarycall_*S`：全为 entry,args6,softtop→uint64。ARM 装 x0..x5、x7 软栈续点；x86 装脚本 rax/rdi/rsi/rdx/rcx/r8 并以 sentinel 恢复宿主 rsp。当前 ARM 与 x86 同样不能把 N 个实参放到脚本全栈入口。
- `exec/parse2/libraryexports.py:59-75`、`libraryexports.md`：USLSIG1 只存 min(count,8) 个 descriptor；原 descriptor depth/base/shape/kind/width/uns 没有 aggregate 字节大小、对齐、成员布局或嵌套函数签名。不能把 width=0 的 aggregate 传给 libffi，更不能拿本地 shape 作为另一模块 ABI 身份。
- `exec/parse2/control-result.tsv:1163`：SIG.store 在 pk=8 边界停止；`functiontypes-result.tsv` 的 FPS_PARAM 与 named PDB 索引采用 sig*16+pk。只取消 pk<8 会踩相邻签名；应增加独立 library 类型参数池或整体改变索引布局，不能粗暴扩循环。
- `exec/parse2/libraryimports.py:66-99`：用完整 count 和前端原型比对，但只认 GP/数据指针/void；包装固定 `.frame 48`、store r0..r5、`.librarycall fn,argv`。变参绑定的单个原型无法描述每个调用点不同的尾参，因此不能靠一个统一原名包装完成。
- `exec/lower/libraryimports.py:15-24,44`：专用 `.librarycall` 在声明库能力下变成 `hostcall`，只有寄存器二元组；它既不携带签名也不带总参数数量。raw tape 被明说为 trusted E3 output，不能据此声称 raw 未可信 tape ABI 已完整校验。
- `exec/enc/hostbridge.py`、`unisa/hostabi.py` 与 TSV：都是固定六GP原生调用。Windows x64 后两项放栈和 shadow 的新桥仍只是六GP。可保留它作 dispatcher 的简单入口，但它不能直接调用 typed FP/aggregate native target。
- `exec/c/libunisacc.c:626-650`：私有 guarded 16MiB 软栈、嵌套新栈、逐帧 setjmp 恢复已有；新帧应复用此所有权，不重建一个单例 callback 状态。
- `exec/parse2/librarydata.py`：data 来源/可写证明已实现，但类型与访问仅 GP/普通指针，不因 us_add_symbol 可存 class3/class5 就获得 FP/聚合数据访问。

## 2. 脚本 ABI 已有能力，经典参考的限制

从 `src/front_parse.c:2024-2033,2190-2209,2221-2268,4789-4813,4862-4994` 和 E3 对应规则只读可见：

- float/double 值在脚本通用寄存器保存原始位，返回同理；不能把 uint64→double 数值转换当 FP 搬运。
- variadic 或参数数>6时**所有**参数按每项8B在栈，arg[k] 最终位于 callee FP+16+8k。不是原生 ABI 的“前六寄存器、其余栈”。
- struct value 参数为调用者快照地址，callee 再复制到自己局部；struct 返回在 `__rv_<fn>` BSS 内复制，r0 是其地址。聚合导出必须在返回宿主前立即复制 bytes；嵌套与递归时该全局缓冲是可重入性风险，不能长期借出。
- 经典 named call 只保留前8实参的参数转换事实；第9及以后可能走 default promotions。要求完整 declared ABI 时这个 bug/语义缺口也要解决，FFI桥无法修复错误的前端转换。
- 经典间接调用的非全栈路径明确拒绝六寄存器参数，因 r5 用作 callee；`src/front_parse.c:1444`。所以“固定≤6 FFI成功”不等于六参数函数指针调用成功。
- 经典最多16个按值 struct 参数有显式错误；容量限制必须写入支持边界或提高，不能据声明 count≤1024 推断源码可接受所有1024。
- `va_arg` 的实现为每项+8然后标量 load。按值 aggregate 的 va_arg 不能据 generic descriptor class5 就宣称工作；需要返回地址/正确复制的对应 δ 语义。
- `long double` 在参考中明确折叠成64位double（`src/front_parse.c:3494`，`scalar-prefix-result.tsv` 注释）。对于具有更宽 native long double 的宿主，**必须区分源 spell 与 FP格式**或拒绝 ABI 匹配，不能生成一个 class3,width8 并偷偷匹配 native long double。
- 现有源码含 union、bitfield、nested aggregate 和 array 成员；它们没有出现在当前 FFI descriptor。C ABI 类型图必须保留 union/struct 区别、array 长度、成员 offset、bit width/storage unit、alignment。数组参数 decay 后是指针；数组成员不是指针。
- `_Complex`、向量、128位整数、非默认调用约定、packed/属性：本次未找到可证明它们完整支持的 ABI 路径，不将其算现成能力；应明确拒绝未知类型，并按实际文法逐一声明边界。函数指针虽 class4 被保存，嵌套 signature 未保存，不能作为普通data pointer靠 depth/width 比较等价。

这些是源读证据；没有针对上述边界执行探针。

## 3. 共享协议（建议新版本，旧格式仍保持原子拒绝）

### TypeGraph：USLTYP2，USBIND3/USLSIG2引用

统一 LE64 编码、有记录长度和总长度；target 为明确 os/arch/data-model/endianness/native-ABI ID，不能只写 abi=0。每节点：

`id, tag, byte_size, alignment, flags, payload_len, payload`

Tag：void / signed-int / unsigned-int / bool / pointer / function-pointer / float / struct / union / array。float payload 为格式枚举 F32/F64/真实LD格式，不能以 width 唯一标识；pointer payload 可保持 opaque pointee ABI，仅 function-pointer 必须引用签名。aggregate payload 为成员计数及 `(type_id,byte_offset,bit_offset,bit_width,storage_size)`；array 为 element/count/stride。聚合成员图无按值环，指针可递归。type_id 仅协议内索引；等价由规范节点内容与布局判定。

Signature：`result_type_id, fixed_count, total_declared_count, param_type_ids[], variadic, script_mode`；固定函数必须全量 stored=count，不能再min8。source 的完整布局由 δ 发出，host 注入提供显式可信 native 布局，δ 逐节点比较并明确 mismatch；prune 只消费defined/linkage/name而需认识新 framing。

CallSite：`site_id,symbol_id,signature_id,fixed_count,actual_count,converted_type_ids[],argument_offsets[],result_offset,plan_id`。actual>=fixed，尾部类型是默认提升**之后**的类型；同名 variadic 函数的每个 site 可以不同 actual/type。source 定义胜出时不要生成 native plan。

### ValueFrame

`version,generation,signature_id,site_id,argument_count,data_bytes,result_bytes,slots_offset,data_offset,result_offset,status`

- Script slots 每项uint64：GP整数按定义扩展、FP为原始F32/F64位、pointer为uintptr_t、aggregate为frame-owned复制地址；对齐与bytes存于独立 arena。
- 参数 values offset 的含义是**原生 typed object 的 bytes**，用于 ffi 的 void** 或 NativePlan copies；不要把 void** args 本身误作脚本寄存器槽。
- 返回对象单独 bytes 缓冲：标量 memcpy 指定位宽；FP逐位；aggregate从脚本返回地址复制指定layout.size；void无缓冲。小整数返回按libffi当前ffi_arg约定分配足够大slot，不能对1B用户buf写8B。
- dynamic pointers/addresses 不参与网络模型常量内容；plan持有符号索引，context映射代际机械绑定该索引的地址。所有长度乘法/offset+size独立overflow检查并带上界。

### ScriptPlan

`mode=REGISTER|ALL_STACK,slot_count,stack_bytes,result_mode,result_size`

REGISTER 只fixed≤6，所有6槽零填；ALL_STACK 必须全部 N 槽为正序。ARM initial x7 是续点，参数在 x7+8+8k；callee push旧r6后FP+16吻合。x86 sentinel必须置于参数块上方，call push续点后同样FP+16；返回后不能假定rsp原sentinel位置，应用固定frame设计/保存位置恢复。stack_bytes不是native spill count，更不是fixed_count。传入总N用于variadic尾参数栈构造。call前检查guard/reserved容量，不能把N*8与aggregate arena漏算。

## 4. 原生 ABI 后端与变参导出边界

**libffi 后端**：用完整TypeGraph构建owned ffi_type树与parameter array，固定用ffi_prep_cif，导入variadic具体site用ffi_prep_cif_var(fixed,actual)。C在此只匹配协议tag到可信FFI类型，不读取源码、不决定语义。尺寸/对齐与声明必须核对，不能强填ffi_type.size假造布局。普通struct可表达，union/bitfields/packed不能无证据代换为uint8数组；libffi的支持不足必须进入下述计划后端或明确unsupported，后者仍未完成全部聚合目标。

**NativePlan 后端**：δ根据TypeGraph和target发一个有限搬运计划：`gp_in[ABI容量],fp_in[容量×16B],native_stack_bytes,native_stack_alignment,shadow_bytes,variadic_fp_count,indirect_copy_regions,sret_location,return_gp/fp/copy_steps`。每项copy为source region/offset,dest bank/offset,width；signed/zero-extension是显式操作码，native只解释。分别需要SysV x64、Win64、AAPCS64、Apple ARM64变参和Windows ARM64变参布局规则，不能把所有ARM统一到当前6GP。

通用asm机械装载原生GP/FP完整bank、native栈，正确设置sret与变参元数据，保存各平台nonvolatile，调用目标，收集所有返回bank。aggregate register split/HFA/indirect return不由C猜。native→script入口可由δ emitted fixed native entry stub捕获同一bank再交给shared dispatcher；这样union/packed无需libffi closure猜ABI。不得通过现场宿主C编译一个prototype wrapper来补洞。

**原生 variadic 导出不可自发现**：普通C `T f(Fixed,...);` 的函数地址无法知道调用者尾参数数量/类型；不读取源码/格式字符串/额外协议就不可能完整解码。libffi closure的cif固定也是如此。因此完整能力需要公开typed invocation API（例如`us_call_typed(ctx,name,actual_types,values,count,result)`）或`us_sym_typed(ctx,name,actual_types,count)`返回该具体调用shape的fixed native closure。后者类型应明确是特化固定入口，不能把它强转为任意variadic并冒称支持。任意native variadic pointer兼容只有API增加显式count/type约定或特定协议适配可做到，必须在PRD写清契约。这不缩小脚本→原生varargs，后者每个source callsite有类型，能够完整实现。

函数指针回调另需origin标识。script code address与native code address不能混；source取址作为native回调参数必须δ请求生成typed native入口并附generation，而不能直接发送raw script label。native callback返回script也使用共享ValueFrame。

## 5. 精确实施顺序与文件

1. **先PRD**：记录TypeGraph/CallSite/ValueFrame语义、全部ABI承诺、变参导出显式类型契约以及容量；同时用源码类型谱列必须支持和明确未支持项。
2. **完整δ事实**：`exec/parse2/gen2.py,functiontypes.py,functiontypes-result.tsv,control-result.tsv`、新`librarytypes.py/TSV`，增加独立unbounded-with-cap parameter/type pool，保留当前默认tape路径。`libraryexports.py`及md输出USLSIG2；`libraryimports.py`不再按原名生成固定6GP变参包装，而由实际CL调用点保存promoted types并生成site wrapper。`librarydata.py`消费完整FP/aggregate layout。
3. **资源协议**：`exec/c/libunisacc.h,librarybindings.h,libraryresolver.h`、`exec/modelbindings.py,modelcandidates.py`、prune签名decoder：只保存/校验framing/传递图，仍由δ做源优先与类型一致选择。添加owned plan/type arrays，失败保持旧代际。
4. **双向共享桥**：新`exec/c/libraryframe.h`，改`libraryexports.h,librarycall.h,libunisacc.c`及四ISA S，用invoke_frame替代args6；initialise/main走同一帧或保留兼容小桥。新的generic host dispatcher作为ctx资源，导入wrapper将frame ptr通过现有GP hostcall送过去。`exec/lower/libraryimports.py`、`exec/enc/hostbridge.py`仅发dispatcher调用，而不是偷渡不同native target到固定GP桥。若选择NativePlan，补新`exec/abi/`规则和各ISA机械bank bridge，不在C新增classifier。
5. **按ABI完整验**：fixed FP、混合多参数、不同聚合register/indirect返回、每site variadic、nested callbacks、data各维度必须分别正负控制；构造网络/table观察与实际host调用分账；六目标原生证据单列。现有受影响门禁和完整产物重建按父会话调度，不靠本报告宣布通过。

## 6. 生命周期与风险

- 类型树、ffi_cif、ffiargs、callsite plan、closure地址全部owned by映射generation；跨call可重用，重编译/重定位/符号变更先quiesce后全部失效。borrowed host地址与data仍由caller保持。
- 每一次invoke有独立ValueFrame/arena/return对象；nested使用独立guard软栈与ScriptFrame。退出longjmp会跳过dispatcher的正常free，arena须登记逐帧cleanup域，而不是放在native wrapper局部malloc后依赖正常返回。
- 聚合立即复制返回；原`__rv_fn`导致同context同步递归数据覆盖风险，应做有界reentrant probe再选择caller-result-buffer ABI改动，不能称已有global buffer自然安全。
- 不同contexts并发继续独立；同context跨线程并发仍不许可。native ABI plan能力不等于Windows SEH安全，现有动态guest frame的异常穿越仍没有保证。
- 序列化ABI与原生ABI不能混淆：LE64是元数据协议，object bytes依target endian/alignment。native only执行必须严格host target一致。
- 索引sig*16、min8转换、long-double别名是隐藏前端债；若未闭合，扩桥会把错误ABI包装成看似成功。

## 7. 下一次唯一有界实验

**设计，未运行**：先在私有冻结目录做一个≤55秒、每compile/call≤10秒的macOS arm64双向case：9参数混合`int,double,float,ptr,struct{double,int},int,double,int,int`，聚合返回`struct{double,int}`；再同源码用两个variadic callsite（尾参分别double/int与double/double/int）验证fixed_count/actual_count区别。选择9参数是为了同时击穿min8元数据与前8转换路径；需独立C ABI参考计算位与值、聚合复制后修改输入验证by-value、返回bytes及canary/栈恢复。宿主有一个注入真实typed target和一个typed closure调用实际script实现；不能只调用libffi参考函数或只比较wrapper文本。

实验第一道交付应为TypeGraph+ScriptPlan完整模型输出，若第9参数或layout遗漏即停止，不扩到VM。成功后再分ABI队列推进全类，不把这一case改名完整FFI。