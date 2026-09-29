# R10 NativePlan source facts：最小演进计划

状态：**历史源读设计**，2026-09-29。初稿没有运行探针、测试、构建、VM。随后BFS/MSZ映射已修正，当前`librarytypes.py`读取MSZ；实际源码wire回归为`tests/modelbitfieldsourcecheck.py`。下文第一节保留当时发现与红probe，不代表该缺陷仍在。匿名/屏障、宽FP等事实缺口仍未闭合；最新位域设计与实际ABI控制见`r10-bitfield-model-plan.md`及`r10-bitfield-native-controls.json`。沿既有通用 NativePlan 设计继续，本文件只处理 SOURCE FACTS，绝不把 host ABI classification 搬入C。

## 1. 最优先真实源读缺陷：BFS误作storage

精确字段语义：

| E3 bank | 写端 | 正确含义 | 既有消费 |
|---|---|---|---|
| MOF | BF.metadata | storage/container起始byte offset | member lookup、initializers、wire |
| BFO | BF.metadata | container内LSB bit offset | bf_offset |
| BFW | BF.metadata | bit width | bf_width |
| BFS | BF.metadata | `bf_signed`，0/1 | bf_signed |
| MSZ | BF.metadata | `bf_unit`，1/2/4/8 bytes | bf_unit/对象unit |
| MBS | BF.metadata | 声明/规范化基础类型码tb | child descriptor |

源证据：`exec/parse2/bitfields-result.tsv:67` 同一条明确STX BFS←bf_signed、MSZ←bf_unit；`membercontrol-result.tsv:24`读取 BFS→bf_signed、MSZ→ms→bf_unit；`initializers-result.tsv:155`也同样读取；`control-result.tsv:559`匿名成员传播两个bank而不改含义。`bitfields.py:37`按当前integer type facts绑定unit。

但 `exec/parse2/librarytypes.py:74` 的 `LTY.bitfield` 将 **BFS** 输出为 USLSIG2 `storage_bytes`。这是源读即可确定的字段映射冲突，尚未实跑确认生成wire。最小修正是此处读取 **MSZ**，其余BFS读写不动；普通member的storage也已从MSZ读取（:75）。不要新增bank或把BFS整体改成存储宽度。

### 父会话可直接跑的首个红probe

```c
struct B { unsigned int a:3; };
struct B round_b(struct B x) { return x; }
```

完整source→USLSIG2独立逐字段oracle：result与arg各一个tag1/kind5 root，member `(offset0,bit_offset0,bit_width3,storage4)`，child `(depth0,kind1,width4,unsigned1,alignment4)`；support仍0。当前源码预期storage0。

signed int :3 的storage也必须4，不是当前预期1。追加unsigned/signed long :33（unit8）、char :3（unit1）、short :9（unit2）、_Bool :1及anonymous aggregate复制路径。signedness用BFS/child事实另核，不与storage合并。

首片测试看wire与member extraction/initializer/sizeof输出，**不放宽FFI支持位或carrier认证**。`libraryexports.h:140-144`会因 unsigned storage0且bit_width>0拒绝；signed storage1有时通过decoder但布局事实错误（超过8位则拒绝），所以不能只跑宿主是否accept。旧已生成错误wire不能“推测修好”：decoder继续reject，源码重新生成正确wire即可。

## 2. 当前SOURCE事实在哪里丢失

### long double

- C：`src/front_parse.c:3489,3497`先读long，double再把kb/declflt改成double；`kindsize:3358`只返回f64。typedef保存tdflt（:3262、:3461），成员mbflt（:3685）、paramflt（:4319）及表达式curflt都只区分4/8。
- Python：`unisa/front/parse.py:47-55` `_basety`遇double直接F64；`unisa/front/lex.py:47-62`剥去L后经Python float，除了F后缀均binary64。
- E3：`scalar-prefix-result.tsv:18` `TS.longdouble`直接tb=DBL；`gen2.py:398-399`绑定DBL；`librarytypes.py:43-47`只生成float32/double64。
- wire：`modelsignature.py:72-73` FP只允4/8；`libraryexports.h:133-134`同样；`modelgraphequality.py:34-39`七词与四布局词，没有format或rank。

**不能在librarytypes序列化末端恢复已经丢掉的long-double身份**。必须从specifier捕获semantic FP rank以及目标明确存储format，沿typedef、named/anonymous function prototypes、callback graph、成员、array、变量、表达式、literal和return一路传播。

### zero-width / unnamed bitfields

- C `stbody:3603`位域类型贡献aggregate alignment；:3610-3618计算barrier位置；:3620-3623把unnamed或width0直接跳过，不进mb/stcount。它们的布局效应尚存在，但declared composition消失。
- E3 `BF.zero/validate/place`在 `bitfields-result.tsv:43-60`验证width0只能unnamed，推进soff；:61-66匿名只走BF.padding→SB.al，不进BF.metadata/SMEM。故wire没有zero-width或nonzero anonymous位域。
- wire width0目前不是barrier标志，ordinary member也用bit_width0；不能仅对tag1所有bit_width0成员判barrier。

### anonymous aggregate与packing

- C `stbody:3530-3557`把匿名struct/union成员扁平拷入外层；E3 `control-result.tsv:547-559`同样扁平copy，librarytypes以SMN/SMEM序列化。**匿名union被扁平到外层struct后，原始重叠alternative分组不再在TypeGraph中表达**。这对general union/HFA递归分类是独立缺口，不只是加zero-width旗标。
- C natural布局的member mal从size/stalign取，并在:3535、:3649裁到8；最终stalign:3726。E3 `SB.put/SB.put4/SB.al/SB.end`保存MSZ/offset/SAL，没有member effective alignment或显式alignment modifier事实。
- 本次未找到packed/_Alignas/pragma pack/attribute align的完整源处理路径；**不能写它们已有支持**。未来profile不同位域布局仍需source layout算法明确target化，schema记录事实不自动修布局。

## 3. 推荐最小且自洽 schema：USLSIG3，保留旧USLSIG2

不要往旧tag1/2 payload尾部悄悄追加（旧reader按精确length消费）；不要用parser base/shape ID表达ABI事实。推荐单一版本边界 `USLSIG3\n`，record/signature/callback ID框架保持现V2，descriptor和layout扩字段。这样一个decoder分支覆盖所有新事实，旧V1/V2字节仍原样可读。新 capability资源明确开启source-facts3输出；不懂V3的消费者确定拒绝。

### Descriptor

保持七LE64词 `depth,base,shape,kind,width,unsigned,alignment`，其中width=真实object extent，alignment=实际type alignment。增加：

```
fp_rank:u8     0=none,1=float,2=double,3=long_double
fp_format:u8   0=none,1=IEEE32,2=IEEE64,3=x87_extended80_padded16,4=IEEE128
natural_alignment:LE64
layout_flags:u8  bit0=packed-layout, bit1=explicit-type-alignment
layout_known_mask:u8  bit0=packing事实已知, bit1=explicit alignment事实已知
layout_origin:u8  0=parser投影,1=完整source事实,2=显式外部layout事实
layout_tag:u8  (保持0 primitive,1 struct,2 union,3 array,4 callable)
payload_length:LE64,payload
```

FP rank解决source通常算术转换/默认提升，format解决位表示；相同width16的x87与IEEE128绝不等价。C type equivalence与native storage equivalence不是同一个问题，ABI matcher至少比较format/真实layout；source declaration matcher还保留rank。对非FP rank/format必须0。指针opaque保持0，不宣称指针pointee FP运算已支持；将来完整pointee语义沿source type pool保留。

natural_alignment=完成成员有效alignment后、施加aggregate自身alignment override前的alignment。实际alignment仍原字段。packed/explicit旗标是声明事实，不可由host猜；只有对应known bit存在时，旗标0才表示已知未设置，而非未知。当前E2会丢弃`_Pragma(pack(...))`（macrocheck已有用例），所以parser投影不能默认写全known或origin=完整source。完整source认证需要从E2保全/拒绝相关modifier，再由E3传播；外部layout则明确标其来源并只证明提供的事实域。完全已知ordinary natural layout可写旗标0、known_mask=3。不强行保存pragma数字历史：member effective alignment和result type alignment已经保存实际效果；只有源码表达所需的pack state在source parser自己持有。

### Struct/union payload

`layout_entry_count`替代“仅可lookup的member_count”；按原始**直接声明顺序**保存：

```
entry_kind:u8      0=ordinary,1=bitfield,2=zero_width_barrier
entry_flags:u8     bit0=anonymous, bit1=explicit-member-alignment, bit2=packed-member
byte_offset, bit_offset, bit_width, storage_bytes, effective_alignment : LE64 each
child_descriptor
```

ordinary entry bit字段为0，storage=child实际extent；bitfield entry storage=MSZ真实container bytes，child保存声明基础type，bit offset/width完整，signedness用child及source extraction事实；barrier bit_width=0、bit_offset=0、storage=声明container单位，但**占用object span=0**，offset是layout cursor/barrier位置，可以等于root.width。decoder对barrier不可执行旧`child.width<=root.width-offset`占用检查，而检查其alignment/container合法及position≤root.width。

匿名aggregate是一个ordinary child，保留tag1/tag2，不能把其子field平铺到这个ABI图中。匿名位域是bitfield/barrier entry，不作为lookup field。位域也可能跨语言规则有storage overlap，需要验证有效位范围，不能把container全宽当唯一起始/终点。

array仍count/stride/element child，descriptor新增facts继承；callback定义/引用拓扑与bounded graph ID沿现有协议，nested descriptor也使用V3。对象signature图没有active member，含callback的union仍需显式typed active-member对象契约/统一安全重叠规则，schema本片不凭field大小解决它。

### 为什么这个扩展是最小 coherent

必需新增的是FP rank/format、type natural alignment、member effective alignment、entry kind与匿名direct topology。layout_flags只保证无法恢复的声明modifier不被默默当natural；不带ABI classes/HFA/GP/SIMD/寄存器分配等派生答案。**这些完全是source facts**，nativeabi δ负责后续分类。无需新NativePlan phase、source parser或宿主classifier。

## 4. source内部最小数据传播（具体接点）

保持现SMEM/member lookup路径，以免改变字段引用/initializer索引；增加每aggregate独立的**直接layout entry序列**，它是现type metadata的附表，不是新编译框架。

- 在 `BF.declare/BF.place` / C `stbody:3594`立刻建entry，anonymous/barrier也建；`BF.metadata`再把named entry关联lookup member，MSZ仍unit、BFS仍sign。union entry grouping保留。
- 在 `SB.anonlayout/SB.anoncopy` / C匿名:3530增加ordinary匿名child entry；仅lookup继续flatten。不能从flatten结果倒推曾经的anonymous union。
- `SB.put4` / C :3678记录member effective align及type/shape；`SB.al/SB.end`同时保存natural/result alignment；宽FP时去掉无条件max8 cap并采用明确target storage layout（不是host sizeof），不能只是扩大SSZ。
- FP rank/format需要decl、typedef、symbol/member、result和signature参数的facts池。现E3 `SIG.store/FN.pfpdecl1→LX.capture` (`libraryexports.py:49-57`)已经捕获后decay的depth/base/shape；在**同一个capture**保存rank/format或新的source type node引用。原型/typedef/callback/前向定义必须同样携带，不能只加export定义路径。
- `FPS_RD/FPS_RB/FPS_RSH/FPS_PARAM/FPS_PSH` (`gen2.py:41,594`)与PDB首槽及完整参数capture都须保持统一type node；`librarytypes.py:54-80`对新直接entry序列生成V3，不读source文本。
- MS canonical新增V3 validation；MG记录新增facts与layout entryKind/effectivealign，coinductive equality既不漏新字段，也不让anonymous共享拓扑改变语义。host `libraryexports.h`只decode/own/validate边界；librarycallables/NativePlan继续creation阶段机械pair/transport。

新池namespace由负责实施的父会话按当前agent最新占用断言分配，本设计不占或硬编码共享bank。source经典C/Python和E3三实现都要定义同样metadata，不能让行为参考继续long-double折叠再与它比较“字节一致”作为正确性证明。

## 5. 宽FP尚缺的行为，不能以wire schema充当实现

- literal：C `fltlit:2555-2579`只4/8、L视8；E3 `floatconst-result.tsv:14`默认52 mantissa bits/1023bias，:117把decimal exponent clamp400，:46 limb cap160，:157后mantissa积在单个64bit寄存器。须按format精度/exp表精确舍入，mantissa/结果采用多word bytes，binary128不能保留df_m一个word；160 limbs与400 clamp不够wide exponent域，须算有界容量并测试，而非只改52→112。
- Python FNum继承float，L literal先经double，超过53bit精度已经丢；wide数字必须Exact token/format converter，不能double转wide来“恢复”。
- arithmetic/compare、truth、unary、usual conversions/assignment、integer↔FP、F32/F64↔LD、constant evaluation、sqrt及NaN/signedzero都只有f32/f64现轴：`src/front_parse.c:2586-2603`、`gen2.py:514,560,721,732,762,820,847`、`unisa/fp.py`。需扩真实representation/operation semantics，不把native ABI transport当数学实现。
- storage/valueframe：现scalar raw uint64/8B本地slot/固定参数slot不能承载128bit。宽值走明确owned bytes/reference value表示并持type，不伪装scalar u64；load/store、initializers、sizeof/alignof、array stride、pointer arithmetic、return/copy/va_arg/callback都必须同一事实。暂不支持宽运算时source明确拒绝，禁止认证一个被F64折叠的long-double签名。

## 6. 可测试的最小首批（尚未执行）

1. **立即修BFS→MSZ的wire映射**。上述unsigned/signed int/long/char/short/bool probe，独立完整wire逐字段比较；现source运行与旧bitfield read/update/initializer不退。无需schema更新，也不增加ABI支持。
2. **只metadata的V3图红绿**：原始direct anonymous union grouping、unnamed非零位域、zero barrier、type/member alignment facts。普通现V2字节路径不变；V3完整图canonical/equality与截断/伪flags/control拒绝。独立oracle对struct{unsigned a:3;unsigned :0;unsigned b:3;}两named field offsets+barrier节点，不能只看sizeof。
3. **long-double rank保留和错误拒绝**：direct/typedef/array/member/named+callback signature/前向声明均看到rank3；在format64 target可使用现F64语义而保留rank；在format80/128 target尚未实现时明确unsupported/reject，不发width8冒认真实LD。格式与align必须来自显式target profile事实，不从当前host读取。
4. **wide hex literal bytes + owned storage**再到copy入返：`0x1.000000000000001p0L`等超过F64精度、边界/subnormal/NaN相关控制；system cc按target真实longdouble写独立字节oracle，padding mask不作语义位；同时测sizeof/align/struct layout/array stride。此片只是wide literal/storage，不称算术全实现。
5. 最后完整算术/转换及NativePlan双向real native，六平台分别验。普通F32/F64、fixed/variadic/callback旧路径须回归；本设计不抢父会话当前共享门禁或构建。

## 7. 给父会话的短结论

本轮最可立即执行且有源证据的修复只有：**librarytypes LTY.bitfield storage读MSZ，不读BFS**。general schema不能仅添加一个“wide”size：longdouble身份、anonymous union direct topology、barrier entry及effective alignment已经在SOURCE过程中丢失，必须在decl/layout producer保存再传播。推荐V3单版本边界，旧V2不猜/不改；source事实明确后，再由现nativeabi统一分类器生成FFI_CARRIER/BANK。

## 父会话实测更新
上述 BFS/MSZ 映射缺陷已经实际先红后绿，见 [位域回执](r10-bitfield-source-storage-evidence.json) 与 `tests/modelbitfieldsourcecheck.py`。修复仅改变 serializer 的 storage 读取，未放宽位域 FFI 支持。其余 USLSIG3/宽浮点/布局事实仍为未实施设计。

## 8. Ordered事实实施（2026-09-29，在开发）

第一步只保全模型内部按声明顺序的layout entries，保留V2原字节与support0，不从SMEM扁平化投影猜缺失条目。bank600..619已由PRD保留，记录容量有界且耗尽拒绝。真正V3消费者、model equality/NativePlan、宿主mechanical decoder和转换边仍需按同一版本边界接入，不能把内部记录完成称为公开ABI支持。实际位域carrier先验控制见`r10-bitfield-native-controls.json`，父会话复跑见`r10-bitfield-controls-rerun.json`；二者均不使用生产模型分类，不是产品认证。
