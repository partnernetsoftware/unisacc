# tapebin v1：tape 的二进制封装（R14-7 落点 A）

目标是把**现有、已预处理的整个程序 tape** 编码成可检查的记录，供同一后端 `-run` 或落成六目标镜像。`target=0` 只声明指令集不含机器寄存器/字节码；它**不**撤销 C 前端已按 `_WIN32` 等宏选定的分支。跨平台语义等价仍由现有测试裁判，格式不充当证明或沙箱。当前 main 从 C 源裸 `-S` 生成时，两种 osx slice 按各自目标选择预定义及 origin；四个非 osx slice 保留 lnx/x86_64 默认。显式目标及输入自带 origin 的路由保持原合同。

## 文件与记录

所有固定字段为小端；所有偏移从文件起点算，节起点按 8 字节对齐，填充必须为零。64 字节头：`UTAPEBIN`（8 字节）、`major=1`/`minor=2`/`opset=1`/`flags`（各 u16）、`target=0`（tape ISA 中立）/`section_count`/`header_bytes=64`（各 u32）、后续目录及节内容的 SHA-256（32 字节）、`origin_target:u32`。`origin_target` 固定枚举：0=未知或手写，1=lnx/x86_64，2=lnx/arm64，3=osx/x86_64，4=osx/arm64，5=win/x86_64，6=win/arm64；从 C 前端直接写包时按实际目标设置，从无来源标记的手写 `.tape` 转包默认 0，可显式指定。未知 major、minor、opset、flags 位或 target 值必须拒绝。紧接 `section_count` 个 24 字节目录项：`kind:u16, flags:u16, offset:u64, length:u64, count:u32`；`kind` 固定为 1=`NAMES`、2=`CONSTS`、3=`RECORDS`、4=`SOURCE_SHA`、5=`TEXT_EXACT`，节 flags 仅 bit0=`required` 可置位。要求长度/计数有界、偏移不溢出、不重叠、文件无尾随字节。未知**可选**节跳过，未知必需节拒绝。

| 节 | v1 内容 |
|---|---|
| `NAMES`（必需） | ULEB128 个数；每项 ULEB128 字节长 + 原样名字字节。标签与数据符号共用索引，定义的种类由记录标明；同名定义的先后次序不丢。 |
| `CONSTS`（必需） | ULEB128 个数；每项 ULEB128 长度 + 原样字节。`.str` 引用池索引；`.bss` 只记长度，不展开零字节。池可去重，但定义记录顺序不变。实现界：C 读写器（驱动与经典）把 `.bss` 长度当非负有符号 64 位字（≤ 2^63−1），之外判无效载体；长度是否可布局由 lower 判定，不由读器判定。 |
| `RECORDS`（必需） | ULEB128 条数；依原 tape 顺序写记录种类 byte：0=`label`、1=`.str`、2=`.bss`、3=指令、4=`.global`、5=`.extern`、6=对象版本头、7=`.gdef`、8=源定位帧、9=`.end`。对象版本头携 ULEB 版本号 2，`.gdef` 携名字索引；源定位帧与尾帧见下文。链接属性与定位记录不占 opcode，不生成指令或数据；原行顺序保留。定义记录携名字索引及池索引/长度；指令携 1 字节 opcode，随后按冻结的 operand shape 写值。 |
| `SOURCE_SHA`（可选） | 32 字节原 C 输入摘要；没有原 C 输入时省略，绝不由 tape 猜造。 |
| `TEXT_EXACT`（可选） | 仅供手写 tape 在确有需要时保存注释、空白与数字拼写；执行前必须核实它重新解析所得记录与 `RECORDS` 一致，避免两个答案。规范编码与验收均不依赖此节；编译器 `-S` 所产包不携带此节。 |

opcode 0–70 按下面**固定次序**赋值（不是运行时字典迭代或 `src/back_lower.c` 的内部编号），`opset=1` 永不重排；`unisa/tape.py:SHAPE` 是唯一声明，导出 `docs/tapebin-v1.shape.tsv`，C 参考、Python、`exec/c` 从该文件生成或启动时逐项断言，门禁 `tapebin-shape` 比对三方：

```
0–10   imm mov add64 sub64 mul64 xor64 and64 or64 shl64 shr64 lshr64
11–20  .div .mod .udiv .umod slt64 sle64 ult64 ule64 eq ne
21–30  load64 store64 .ld .st .lea .zero jump jumpz call callr
31–46  callm .hostcall .librarycall .libraryaddr .hostaddr ret .frame .arg .print .write .exit .sys .sys6 .argc .argv nop
47–59  fadd64 fsub64 fmul64 fdiv64 flt64 fle64 feq64 fadd32 fsub32 fmul32 fdiv32 flt32 fle32
60–70  feq32 cvtid cvtud cvtis cvtus cvtdi cvtdu cvtsd cvtds fsqrt64 fsqrt32
```

operand shape 固定为上述 71 项快照，只能在新 opset 末尾追加。每条指令的寄存器按出现次序两个合一字节（低/高 nibble，值 0–7；奇数个时高 nibble 必须为 `0xf`）。`i` 用种类 byte（0=有符号、1=无符号）加**最短** SLEB128/ULEB128 表示 64 位值；计数和索引用最短 ULEB128（至多 10 字节）。`L`/`s` 用种类 byte（0=数值、1=`NAMES` 索引）；引用越界、数值溢出、非法寄存器、未知 opcode 或不符 shape 均拒绝。名字按原样字节存储，但含 NUL/空白/冒号而无法由当前 tape 文本语法表达的名字拒绝。行号不是执行语义；若手写 tape 以 `TEXT_EXACT` 带原文则保留物理行，除此不伪造源位置。

## 接入与兼容

`to_canonical_text` 是确定性的规范打印器：同一记录序列给出同一文本，不还原原始 `-S` 的空白与写法。`Tape.records` 按原行顺序保留定义、标签和指令；旧 `to_text` 暂留给既有种子构建路径，不作为二进制往返的依据。

验收用 `examples/` 与 `tests/c/` 的实际 tape，断言生成包不含 `TEXT_EXACT`：① 对同一输入，`encode(parse(t))` 两次逐字节相同，记录保持原行顺序；② `decode(encode(t))` 与 `parse(t)` 的记录逐项相等；③ 对所有生成的包，`encode(decode(b)) == b` 逐字节成立。这三项保证内容哈希稳定，不以文本空白为语义。文本与 tapebin 入口的 `-run` 输出及退出码相同，`-b <target>` 的六目标镜像逐字节相同。`-b <target> x.tapebin` 若 `origin_target` 非 0 且与目标不同，默认拒绝；显式 `--force-origin` 才允许跨目标试编，并在 README 限制表提示目标宏风险。C 参考与 Python 种子各有编解码器；模型产品的 `exec/c/` 只做格式校验和记录到既有 `tape.text` 的适配，把编译决策继续交给 E4/prune/lower/镜像网络。v1 不承诺省掉模型当前的文本解析，也不预先承诺包更小；记录实测尺寸和装载时间。加载前做结构校验；跳转可达性、栈平衡、权限与宿主导入安全仍属 FX-1 的独立验证器义务。

当前产品要求完整尾帧，编码与解码仅接受 minor 2；历史 minor 0/1 不带完整尾帧合同，直接拒绝，不通过补 `.end` 兼容。opset 中既有字段、编号与 shape 不变。

链接属性记录形状见 `tapebin-v1.records.tsv`。NAME 使用 `[A-Za-z_][A-Za-z_0-9.$]*`，缺失、多余操作数或非法名字拒绝。prune 对含链接属性的 tape 保守整份透传；全局/未定义符号的对象语义由后端对象模式处理。

## minor 2：源定位与完整尾帧

新编码器与解码器只接受 minor 2。minor 2 追加 kind 8（源定位）：最短 ULEB128 的翻译单元编号 N、最短 ULEB128 的文件名字节数 B、随后 B 个原始文件名字节。N 范围为 0..127，B 可为零，文件名不得含 NUL，其它字节不受名字池语法限制。N 是 TU 身份，不是文件名身份；同一 N 可反复切换逻辑文件名（例如 A→B→A→B），不同 TU 即使文件名相同也不能合并身份。规范文本为 `.unit N B:HEX\n`，其中 N/B 为无前导零的十进制，HEX 为恰好 2B 个小写十六进制字符。`.unit N 0:\n` 显式清除当前逻辑文件名，必须保留该帧，不能继承旧名或默认 TU 文件名；`00` 等非规范零写法拒绝。kind 6 仍只表示对象版本头 `.unit 2\n`，只能是对象的首条记录；不得与三字段源定位帧混淆。

kind 9 无载荷，规范文本为 `.end\n`，必须是 minor 2 的最后一条记录。新文本编码入口要求完整尾帧，缺尾、短读、尾后字节及非法 unit 边界均拒绝。源定位与尾帧不占 opcode，不生成机器码、数据或重定位；解码时保持在原记录位置。对象链接将各对象局部源 unit 映射为互不相同的最终 unit，去掉各对象尾帧后只输出一个最终 `.end\n`。

链接时，非空对象首个待输出语义行若尚无本对象的来源帧，先输出 `.unit N 0:\n`，隔断前一对象的文件名。若随后有真实来源帧，前缀复用该首帧局部 TU 的最终 N，仅清名，不提前套用后来的文件名；完全无来源帧时才分配独立匿名 TU。空白、纯注释和仅含链接属性 `.global`、`.extern`、`.gdef` 的对象不分配匿名 N；标签、指令和实际输出的数据/指令性记录属于语义行，空白与注释控制遵循 `exec/facts/tape-space.tsv`。匿名 TU 与真实帧沿用同一 `tl_source_next` 和 `tokenlocations.UNIT_MAX=128` 容量，0..127 用尽仍按原有 `tape truncated` 拒绝；版本头不额外计数，同 N 换名不额外计数。该边界由链接器补足，不要求 raw 模型伪造来源帧，也不改变唯一最终 `.end\n`。

元记录按完整词和分隔符识别；`.unit0:`、`.unitx:`、`.endfoo:` 等合法标签不属于元记录，不得因前缀相同而拒绝或丢弃。


增量 host-forward 合并同样剥离输入尾帧、输出唯一最终尾帧。定位帧附着于随后语义行，移动 stub 定义或初始化片段时恢复其定位上下文；base 与 stub 的局部 unit 在同一 0..127 空间内分配不同编号，不参与函数形状、符号或机器指令扫描。

产品解码将旧 minor 0/1、物理短读、载体尾后字节、缺最终 end 或非法源定位帧分类为 `reject: tape truncated`。完整载体的未知格式、摘要不符、来源目标不匹配以及原有指令/运行期语义拒绝仍使用原诊断；来源目标身份检查优先于结构分类。

`compiler.tapegate.tb2_tape_frame(text,n)` 是文本及 sidecar payload 共用的无分配结构门：尾部常量快拒后扫描唯一 `.end` 和严格 `.unit` 帧，返回 0（合法）、2（tape truncated）或 4（空指针/负长度）。它不检查 opcode、RI、ABI 或运行期语义。文本 Tapebin 编码也调用此门，并与该门共用 `tb2_source_frame` 的规范十进制、长度与 HEX 解析，不在驱动重新实现帧语法。

共享门的单一空白 predicate 对齐 `exec/facts/tape-space.tsv` 现有 LINE 类：leading 为 9、11、12、13、28、29、30、32、133、160，controls 为 11、12、13、28、29、30、133。只有 LF（10）建立规范帧物理首列；其它 leading 字节不能使 `.unit`/`.end` 合法。普通行及注释遇 controls 后重新扫描 leading 与完整元记录词，因此 `;comment\v.unit bad` 也拒绝；普通指令和前缀标签仍交原语义层处理。





