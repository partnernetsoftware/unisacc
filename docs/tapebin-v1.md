# tapebin v1：tape 的二进制封装（R14-7 落点 A，设计草稿）

目标是把**现有、已预处理的整个程序 tape** 编码成可检查的记录，供同一后端 `-run` 或落成六目标镜像。`target=0` 只声明指令集不含机器寄存器/字节码；它**不**撤销 C 前端已按 `_WIN32` 等宏选定的分支。跨平台语义等价仍由现有测试裁判，格式不充当证明或沙箱。

## 文件与记录

所有固定字段为小端；所有偏移从文件起点算，节起点按 8 字节对齐，填充必须为零。64 字节头：`UTAPEBIN`（8 字节）、`major=1`/`minor=0`/`opset=1`/`flags`（各 u16）、`target=0`/`section_count`/`header_bytes=64`（各 u32）、后续目录及节内容的 SHA-256（32 字节）、保留零（u32）。未知 major、opset、flags 位、非零保留字段必须拒绝。紧接 `section_count` 个 24 字节目录项：`kind:u16, flags:u16, offset:u64, length:u64, count:u32`；`kind` 固定为 1=`NAMES`、2=`CONSTS`、3=`RECORDS`、4=`SOURCE_SHA`、5=`TEXT_EXACT`，节 flags 仅 bit0=`required` 可置位。要求长度/计数有界、偏移不溢出、不重叠、文件无尾随字节。未知**可选**节跳过，未知必需节拒绝。

| 节 | v1 内容 |
|---|---|
| `NAMES`（必需） | ULEB128 个数；每项 ULEB128 字节长 + 原样名字字节。标签与数据符号共用索引，定义的种类由记录标明；同名定义的先后次序不丢。 |
| `CONSTS`（必需） | ULEB128 个数；每项 ULEB128 长度 + 原样字节。`.str` 引用池索引；`.bss` 只记长度，不展开零字节。池可去重，但定义记录顺序不变。 |
| `RECORDS`（必需） | ULEB128 条数；依原 tape 顺序写记录种类 byte：0=`label`、1=`.str`、2=`.bss`、3=指令。定义记录携名字索引及池索引/长度；指令携 1 字节 opcode，随后按冻结的 operand shape 写值。 |
| `SOURCE_SHA`（可选） | 32 字节原 C 输入摘要；没有原 C 输入时省略，绝不由 tape 猜造。 |
| `TEXT_EXACT`（文本来源时必需） | 原 `.tape` 的原样字节（含空白、换行、数字写法），用于逐字节反解；长度及摘要由目录/头保护。执行前必须核实它重新解析所得记录与 `RECORDS` 一致，避免两个答案。直接由结构化记录产生的文件可省略；反解只保证规范打印，不承诺还原从未提供的文本。 |

opcode 0–70 按下面**固定次序**赋值（不是运行时字典迭代或 `src/back_lower.c` 的内部编号），`opset=1` 永不重排：

```
0–10   imm mov add64 sub64 mul64 xor64 and64 or64 shl64 shr64 lshr64
11–20  .div .mod .udiv .umod slt64 sle64 ult64 ule64 eq ne
21–30  load64 store64 .ld .st .lea .zero jump jumpz call callr
31–46  callm .hostcall .librarycall .libraryaddr .hostaddr ret .frame .arg .print .write .exit .sys .sys6 .argc .argv nop
47–59  fadd64 fsub64 fmul64 fdiv64 flt64 fle64 feq64 fadd32 fsub32 fmul32 fdiv32 flt32 fle32
60–70  feq32 cvtid cvtud cvtis cvtus cvtdi cvtdu cvtsd cvtds fsqrt64 fsqrt32
```

operand shape 固定为 `unisa/tape.py:SHAPE` 在 v1 封版时的 71 项快照，须由同一生成清单输出给 C、Python 与模型路线，不能让三处各自维护。每条指令的寄存器按出现次序两个合一字节（低/高 nibble，值 0–7；奇数个时高 nibble 必须为 `0xf`）。`i` 用种类 byte（0=有符号、1=无符号）加**最短** SLEB128/ULEB128 表示 64 位值；计数和索引用最短 ULEB128（至多 10 字节）。`L`/`s` 用种类 byte（0=数值、1=`NAMES` 索引）；引用越界、数值溢出、非法寄存器、未知 opcode 或不符 shape 均拒绝。名字按原样字节存储，但含 NUL/空白/冒号而无法由当前 tape 文本语法表达的名字拒绝。行号不是执行语义：有 `TEXT_EXACT` 时从原文精确保留；无原文时不伪造源位置。v1 只接受现有编译器可执行的 tape 语法；注释不进入语义记录，当前后端不支持的注释文本应拒绝，而不是悄悄吞掉。

## 接入与兼容

文本编译入口先编码再读回：`.tape → .tapebin → .tape` 对 `examples/` 与 `tests/c/` 的实际 `-S` 输出逐字节相同；`-run` 两种输入同退出状态/输出，`-b <target> x.tapebin` 与文本入口的六目标镜像逐字节同。C 参考与 Python 种子各有编解码器；模型产品的 `exec/c/` 只做格式校验和记录到既有 `tape.text` 的适配，把编译决策继续交给 E4/prune/lower/镜像网络。v1 不承诺省掉模型当前的文本解析，也不预先承诺包更小；记录实测尺寸和装载时间。加载前做结构校验；跳转可达性、栈平衡、权限与宿主导入安全仍属 FX-1 的独立验证器义务。

v1 内只可增加**可选节**和在新的 minor/opset 下追加 opcode；既有字段、编号、shape 与语义不得改写。旧读器遇到未知必需节或 opcode 明确拒绝，已接受的 v1 文件在后续 v1 读器上解释不变。任何不兼容改动升 major。
