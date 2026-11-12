# 编译工具链：从单文件映像到可链接的目标文件（R16-7 设计，2026-10-01）

本文是 0.0.16–0.0.18 工具链工作的设计基线：先如实记录后端今天的形状（全部来自对 `src/back_*.c`、`unisa/image/`、`exec/enc/elfimage.py` 的阅读与对 `unisacc.com` 的实测），再给出 ELF `.o` 的规格、与系统工具的互操作矩阵、三版递进和验证方式。盘点表（已有/部分/缺/不做）在 [archive/plans/v0.0.16.md](../archive/plans/v0.0.16.md) H 节，本文只写设计。

> **现状（2026-10-02，0.0.20 已发布）**：§4 的 ELF 规格已扩到 Mach-O 与 COFF（参考侧三格式；产品 Linux ELF 由模型路由写出，产品 Mach-O/COFF 对象仍未实现，见 archive/plans/v0.0.21.md 第 8 项）；单元 tape 自 0.0.20 起以 `.unit 2` 开头、带初值定义记 `.gdef`，链接器据此拒绝重复初值与只有 extern 的对象；汇编文本与 `unisacc as`/`nm`/`objdump` 见 README；分开编译与 tape 级链接器见 §7；`.data/.bss` 分界、全局/未定义符号命名（导出与未定义数据名去 `g_`）以 src/back_image.c 的 `bk_symplan` 为准。下文 §2–§4 中“本版”“0.0.16”的措辞是当时的设计语境，保留作记录；不再成立的两点已逐条标注。

## 1. 后端今天的形状（事实，不是愿望）

| 事实 | 出处 | 对 `.o` 的含义 |
|---|---|---|
| `-c` 与 `-S` 都写出 **tape 文本**（编译器的汇编级 IR），不是目标文件或汇编 | `src/main.c` 的用法行、README 限制表、2026-10-01 对 `unisacc.com` 实测 | `-c` 的语义要翻转，但 86 处套件把 `-c` 当 `-S` 的同义词，翻转分两步（§4） |
| 映像是**固定地址**的两段静态文件：ELF 文本在 `0x400000+176`，数据紧随其后按页对齐；Mach-O、PE 同理各有基址 | `src/back_encode.c` `bk_assemble`、`src/back_image.c` `bk_elf` | `.o` 里文本从 0 起、数据从 0 起，地址全部变成重定位 |
| 代码引用数据只有两种编码：x86-64 一律 **RIP 相对**（`x_rip`，`mov/lea r64,[rip+d32]`，位移是指令最后 4 字节）；arm64 一律 **`adrp` + `add`**（`a_adrp_add`） | `src/back_encode.c` 的全部 `x_rip`/`a_adrp_add` 调用点 | x86-64 只需 `R_X86_64_PC32`；arm64 只需 `R_AARCH64_ADR_PREL_PG_HI21` + `R_AARCH64_ADD_ABS_LO12_NC` |
| 代码取**函数地址**（`.lea 标号`）也走同样两种编码；分支与调用（`jmp/call`、`b/bl`）是节内 PC 相对 | `bk_leaaddr`、`a_relf`、`x_rel` | x86-64 节内 RIP 相对差值与链接地址无关，不需要重定位；arm64 `adrp` 的页差取决于节基址的页内偏移，**文本目标也要重定位** |
| 数据区**没有地址值单元**：`tape.relocs` 只被初始化和复制，从不追加；全局指针（含字符串字面量初始化）由 `__init` 里的代码在启动时写入 | `unisa/tape.py:101`、`unisa/lower.py:207`、`unisa/prune.py:186`、`src/front_parse.c` 的 `__init` | **不需要 `.rela.data`**，没有绝对地址重定位 |
| 零数据不存储：lower 把零块排在数据末尾（`zero_last`），映像只写到最后一个非零字节，其余由加载器清零；暂存单元（`bk_scr0`…）、`argc/argv` 单元、Windows 的 tape 栈都在这段零尾里 | `src/back_lower.c` `bk_lower`/`bk_repack`、`bk_nzlen` | `.data` = 非零前缀，`.bss` = 零尾；对数据的每个引用按偏移选择节符号 |
| **调用约定不是 SysV / AAPCS64**：参数经 tape 栈压入弹出（`TO_PUSH`/`TO_POP`、`@call.frame`），返回值在 r0；x86-64 与 Linux arm64 的 tape 栈就是进程栈（r7 = sp），Windows arm64 自带栈 | `src/back_lower.c` 775–830、927–933 | 与 cc 编译的函数**不能直接互调**；需要适配序（thunk），见 §3 矩阵 |
| 入口是 `_start`：先 `__init`（全部单元的全局初始化），再把 argc/argv 写进自己的单元，再 `call main`；不依赖 crt | `src/front_parse.c:5673` | `.o` 导出全局 `_start`，`ld` 不需要 crt1；被 crt 启动（`main` 由 C 运行时调用）要等调用约定适配 |
| `extern int x;` 没有定义时被**当作定义**（静默分配）；未定义函数报错 `undefined function` | 2026-10-01 用参考编译器实测 | 跨目标文件引用数据符号需要把 extern 变成 `UND` 符号，这是前端改动，排 0.0.17 |
| 后端有**三份实现**必须逐字节一致：Python（`unisa/image/`，构造源头）、C 参考（`src/back_image.c`）、产品 δ（`exec/enc/elfimage.py` 生成 `elf.net`）；`tests/closure.sh` 对映像逐字节对拍 | ARCHITECTURE.md 后端行、`exec/pipeline/image-stages.tsv` | `.o` 写出先在 C 参考落地（本版），产品 δ 与 Python 写出跟进（0.0.17），之后 closure 把 `.o` 也纳入逐字节对拍 |
| 随带 C 库以源码形式跟着编译器，按需裁剪进程序（`-ftrim-libc` 默认） | README、R11-3 | `.o` 自带它用到的库体，**不链接系统 libc**；与 glibc 共存要等符号前缀与调用约定问题一并解决 |

## 2. 格式范围与三版递进

| 版本 | 主线 | 本文覆盖的产物 |
|---|---|---|
| 0.0.16 | 收口 + 可观测的工具链第一步 | 本文；C 参考后端写 **ELF 可重定位目标文件**（lnx/x86_64、lnx/arm64），`ld` 能把它链成可运行程序；`tests/elfobj.sh` 门禁 |
| 0.0.17 | 自带静态链接器与调试行号 | Mach-O `.o`、COFF `.o`；产品 δ 与 Python 的 `.o` 写出并入 closure；自带静态链接器（只链自家 `.o`）；`ar/nm/objdump` 子集；`-g` 行号表；extern → `UND` 符号；函数级互调的适配序设计稿 |
| 0.0.18 | 汇编器子集与 `-S` 真汇编 | 汇编器子集与内联汇编；`-S` 输出真汇编并能被 `as` 与自带汇编器接受；`-shared`/`-fPIC`/`dlopen`；完整调试信息 |

顺延规则同计划：同一项顺延两次必须降级到 0.1.x 或砍掉。

## 3. 与系统工具的互操作矩阵

| 场景 | 需要什么 | 版本 | 验收 |
|---|---|---|---|
| 系统 `ld`（GNU ld / lld）把我们的 `.o` 链成自洽程序 | ELF `.o` 的节、符号、重定位正确；`_start` 全局 | **0.0.16** | 两架构各一例：链接后运行，输出与 `-b` 映像路线逐字节相同 |
| `llvm-readelf`/`llvm-objdump`/`nm` 能读我们的 `.o` | 规范的节头、符号表、`.rela.text` | **0.0.16** | 门禁用 `llvm-readelf -S -r -s` 解析并断言 |
| 我们的 `.o` 与 cc 的 `.o` 共链，**读 cc 定义的数据符号** | extern 未定义数据 → `UND` 符号 + 对未定义符号的重定位 | 0.0.17 | cc 的 `.o` 提供 `const char banner[]`，我们打印它 |
| 我们的代码**调用** cc 编译的函数 | 调用适配序：把 tape 栈上的参数搬进 SysV/AAPCS64 寄存器、对齐栈、取回返回值；原型信息来自前端 | 0.0.17 设计 / 0.0.18 实现 | 调用 cc 的 `int add(int,int)` |
| cc 编译的代码**调用**我们的函数 | 被调适配序（反向 thunk）；导出名不加前缀 | 0.0.18 | cc 的 `main` 调我们的函数 |
| 被系统 crt 启动（`cc a.o b.o` 直接可用） | 上一行 + `main` 走 C 调用约定 + 与 libc 的符号共存 | 0.0.18 之后，按需 | — |
| 链接系统 libc 代替随带库 | 符号前缀策略、调用约定、errno/stdio 状态归属 | 远期，不承诺 | — |
| `-shared` / `-fPIC` | 全部引用已是 PC 相对；缺动态节、GOT、PLT 与导出表 | 0.0.18 | `dlopen` 自家 `.so` |
| 调用共享库里的函数（`-c` 对象 + `.so`/`.dylib`） | 参考侧 0.0.36：对未定义名取址走 GOT（arm64 `ADR_GOT_PAGE`/`LD64_GOT_LO12_NC`、Mach-O `GOT_LOAD_PAGE21/PAGEOFF12`；x86_64 `GOTPCREL`、Mach-O `GOT_LOAD`），默认 PIE 可链；产品 δ 待 cdx 首片 | 0.0.36 | agenterm 探针（archive/docs-2026-11b/research/a1-handoff.md） |

## 4. ELF 可重定位目标文件规格（0.0.16）

**驱动语义。** `-c -b lnx/x86_64` 与 `-c -b lnx/arm64` 写出 `.o`（`-o` 给名，缺省 `a.o`）。**裸 `-c`（不带 `-b`）仍写 tape**，与 `-S` 同义——86 处套件和 README 的 ccparity 条目依赖这一点；等 Mach-O 与 COFF 的 `.o` 也能写（0.0.17 末）再把裸 `-c` 翻成“宿主目标文件”，并在 README 与 `ccparity` 同一提交里改口。`-c -b osx/*`、`-c -b win/*` 在 0.0.16 报错并指向本文（0.0.17 起参考侧写 Mach-O/COFF）。产品（`unisacc.com`）在本版对 `-c -b lnx/*` 仍写 tape，盘点表如实标“参考侧有，产品 0.0.17”。

**布局**（全部小端，`ET_REL`，`e_machine` 62 / 183）：

```
ELF 头 (64)
.text        机器码，与映像完全相同的字节（除被重定位覆盖的字段）
.data        数据非零前缀（bk_nzlen），对齐 16
.rela.text   Elf64_Rela 表
.symtab      Elf64_Sym 表
.strtab      符号名
.shstrtab    节名
节头表       [0] null, [1] .text, [2] .data, [3] .bss, [4] .rela.text, [5] .symtab, [6] .strtab, [7] .shstrtab
```

`.data` 取到**最后一个非零数据块的末尾**（repack 后零块全在其后），`.bss` 是其余部分（含暂存单元与 `argc/argv` 单元）——不能取到“最后一个非零字节”，否则一个初值尾部为零的符号会跨 `.data`/`.bss` 两节，而链接器可以把两节分开放（0.0.17 修正）。Mach-O 同此；arm64 的重定位必须是外部的，所以每节有一个局部锚点符号 ltmp0..2，非零加数放在前置的 `ARM64_RELOC_ADDEND`；x86_64 的 `X86_64_RELOC_SIGNED` 把加数放在位移字段里。`.text`、`.data`、`.bss` 的 `sh_addralign` 都是 16。

**地址如何变成重定位。** 装配时对象模式把文本基址设为 0，把数据基址设为 2^32（`bk_shift = 2^32 − 256`），于是编码器看到的每个地址天然分成两类：< 2^32 是文本偏移，≥ 2^32 是数据偏移。编码器在两个且只有两个地方记录重定位：

| 编码点 | 目标 | x86-64 | arm64 |
|---|---|---|---|
| `x_rip`（位移在指令末 4 字节，P = 指令末 − 4） | 数据 | `R_X86_64_PC32`，S = `.data`/`.bss`，A = 偏移 − 4 | — |
| `x_rip` | 文本 | 不记录：节内 RIP 相对差值与链接地址无关 | — |
| `a_adrp_add` | 数据或文本 | — | `adrp` 处 `R_AARCH64_ADR_PREL_PG_HI21`，`add` 处 `R_AARCH64_ADD_ABS_LO12_NC`，S = 目标节，A = 偏移 |

数据偏移 ≥ 非零前缀长度的，S 取 `.bss`，A 减去前缀长度。分支/调用（`jmp/call/jcc`、`b/bl/cbz`）全部节内、不记录。没有 `.rela.data`（§1：数据区无地址值单元）。

**符号表。**（0.0.17 起按 `bk_symplan`：局部、全局（`.global` 与 `_start`）、未定义三段。）`[0]` 空；`[1..3]` 三个节符号（`STT_SECTION`）；随后每个代码标号与数据符号各一个 **局部** `STT_NOTYPE` 符号（名字来自 tape，供 `objdump`/`nm` 读；数据符号的值按 `.data`/`.bss` 换算）；最后 `_start` 为 **全局** `STT_FUNC`。`sh_info` 指向第一个全局符号。本版不导出其他全局符号：没有 extern 语义之前，导出只会制造与 cc `.o` 的重名冲突。

**不变量。** 同一 tape 在对象模式与映像模式下，`.text` 字节除重定位字段外逐字节相同，`.data` 逐字节相同——门禁对此断言，这把 `.o` 写出钉在已有的 closure 证据上。

## 5. 验证（`tests/elfobj.sh`，门禁 `elfobj`）

1. 对一组探针（`examples/hello.c`、`fact.c`、`fib.c`、`ptr.c`、`struct.c`、`switch.c`、`indirect6.c` 以及 `tests/c/` 中带全局指针初始化的一例），两架构各写 `.o`；
2. 用 `llvm-readelf -h -S -r -s`（本机 llvm 已装；没有时用随附的最小 Python 解析）断言：`ET_REL`、`e_machine`、八个节、`.rela.text` 条目数 > 0、每条重定位的类型只在 §4 的集合里、符号 `_start` 全局；
3. `ld.lld -m elf_x86_64|aarch64elf -o prog prog.o`（本机 lld@21），再在 Lima 里运行 arm64 程序，输出与 `-b lnx/arm64` 映像的运行输出逐字节相同；x86_64 的运行走 `tests/linux.sh` 的 x86_64 虚拟机（宿主上是模拟，看门狗放大 10 倍）；没有链接器或虚拟机时点名跳过，`STRICT=1` 让跳过成为失败；
4. 不变量检查（§4 末）：`.text` 去掉重定位字段后与映像文本相同，`.data` 相同。

## 6. 不做与已知限制

- 不链接系统 libc，不走 crt 启动（见矩阵）；
- 不生成 DWARF/行号（0.0.17 `-g`）；
- `.o` 里的局部符号名是 tape 名字，不是源码里的 C 名字加前缀规则之外的任何修饰；
- 产品 δ 在 0.0.16 不写 `.o`（0.0.17 起写 Linux ELF；Mach-O/COFF 在 0.0.18）。

## 7. 分开编译（0.0.17 R17-2/R17-9 的前提，2026-10-01 设计）

今天的 `-c` 仍是“整程序”：每次编译都带 `_start`、`__init`（全部单元的全局初始化）和按需裁剪的库体，所以两个我们的 `.o` 不能链在一起。分开编译需要四件事，按依赖排列：

1. **tape 的链接属性**（格式变更，归 tape/tapebin 维护者）：新增两条记录 `.global NAME`（本单元定义、对外可见）与 `.extern NAME`（本单元引用、定义在别处）。tapebin v1 的 shape 表、C 与 Python 两侧的读者、docs/tapebin-v1.md 同步；整程序模式下两条记录不出现，旧 tape 字节不变。
2. **前端的单元模式**（`-c -b` 时启用）：非 `static` 的函数与文件作用域对象发 `.global`；只声明未定义的函数与 `extern` 对象发 `.extern` 而不是报 “undefined function” 或静默分配；只有定义了 `main` 的单元发 `_start` 与 argv 序言；每个单元的初始化函数名为 `__init.<单元名>` 并 `.global`。库体（随带头文件里的 `static` 函数）保持局部，各单元各带一份，互不冲突。
3. **后端的未定义符号**：对 `.extern` 名字的调用与取址发重定位——ELF `R_X86_64_PLT32`/`R_AARCH64_CALL26` 与已有的 PC32/ADR_PREL_PG_HI21+ADD_ABS_LO12_NC；Mach-O `X86_64_RELOC_BRANCH`/`ARM64_RELOC_BRANCH26`；COFF `IMAGE_REL_AMD64_REL32`/`IMAGE_REL_ARM64_BRANCH26`；`.global` 名字进符号表为全局。
4. **初始化顺序**（0.0.17 已实现于参考侧：src/tapelink.c 在 tape 级合并单元，见 R17-2 收据；单元对象另带一个不装载的节 `.unisa.tape` / `__UNISA,__unisa_tape` / `.unisatp` 存本单元 tape）：自带链接器（R17-2）收集全部 `__init.*`，合成 `__init` 依链接顺序调用；用系统链接器链多个我们的 `.o` 时，需要初始化代码的单元会被拒绝并说明（只用常量初始化的单元不受影响）——不承诺系统 ld 的多对象初始化，直到有地址单元重定位（`R_X86_64_64` 等）为止。

调用约定：我们的 `.o` 之间都是 tape 约定，互调无需适配；与 cc 的函数互调仍是 0.0.18 R18-9。

## 8. 汇编文本与 `unisacc as`（0.0.18 R18-1/R18-2，实际跑过）

- **形态**：`-S -b lnx/x86_64|lnx/arm64 -o FILE.s` 把 `-c -b` 会写的那个 ELF 目标文件打印成 GNU 汇编（x86-64 用 AT&T 语法）；`unisacc as [-b lnx/ARCH] FILE.s [-o FILE.o]` 把它读回来。代码在 `src/asmtext.c`，只依赖 ELF 字节：先由后端写出目标文件，再反汇编，所以 `-S` 与 `-c` 不可能分叉。不带 `-b`、或输出不是 `.s` 文件的 `-S` 仍写 tape（内部套件大量用 `-b T -S` 取目标 tape，2026-10-01 队列预热时发现，故以 `.s` 输出为准）。
- **一个编码器两用**：反汇编出的每条指令都再交给同一个编码器汇编一遍，字节和重定位都相同才打印助记符，否则打印 `.byte`/`.inst`。所以 `-S | as == -c` 由构造保证，门禁再数回退条数（语料里是 0）。
- **子集**：后端实际发出的指令。实测 186 个 tests/c 程序：x86-64 用到 59 个助记符，arm64 用到 48 个；表里另收了几种同族形式（setcc/jcc 全集、cmpXXsd 全集、ldur/stur 各宽度、逻辑立即数）。指令之外只收 `.text/.data/.bss/.section .unisa.tape`、`.globl/.global/.local`（`.type/.size` 接受但不用）、`.byte/.ascii/.asciz/.zero/.p2align`，`.text` 里的对齐只能放在开头。
- **两处编码要写明**：x86-64 后端在 32/16/8 位存储上总带一个多余的 REX 0x40，写作 `{rex}`；有些长跳转其实短跳转也够得着（后端的松弛轮次与汇编器的最小不动点不同），写作 `{disp32}`。GNU as 与 LLVM 都接受这两个伪前缀。其余编码都与系统汇编器的默认选择一致（arm64 143 万条、x86-64 145 万条逐条对过）。
- **符号与重定位**：文件头按符号表顺序列出 `.local`/`.globl`，`as` 按出现顺序重建同一张表。数据与 bss 的引用写成 `.Ldata+N`/`.Lbss+N`，分支目标写成 `.L<十六进制偏移>`；`.L` 名字不进符号表。跨单元调用写成 `call name@PLT` / `bl name`。
- **验收（门禁 `asmtext`）**：① 语料两个目标全量往返逐字节相同，且没有指令以字节形式打印；② `-funit` 单元目标（带 `.unisa.tape`）往返，重汇编的目标链接出同一个程序；③ 系统汇编器（clang 的 Linux 三元组）接受 `-S` 文本，`.text`、重定位、`.data` 与原目标相同；④ 手写 `tests/asm/hand-*.s`，`unisacc as` 与系统汇编器在 `.text`、`.data`、重定位上一致；⑤ 子集外指令、`-S -b osx`、`as -b osx` 都按名拒绝，失败不留文件。
- **0.0.19（首次顺延）**：Mach-O 与 COFF 的汇编文本，Intel 语法，内联 `asm`。

## 0.0.57 链恢复补丁合同

- 原补丁基线为 `60acc817b86b0605016f38604ec0dbea5fac4d8d`；本次重基到 `67fb47417b7c053217a1aa1d307eb9ab9a5bef9b`，保留 A2p 动态分片、chainfacts 及全部既有断言。唯一源码冲突为 chain-step 的 use 追加，保留 chainfacts 与 sys.posix 两者。现场 MANUAL.tsv、chain-run.tsv、chain-run-16.stderr 只读，不重跑失败确认。55 秒上限不改。
- snap 不再嵌套冷编 release-build：直接 git archive 到有 root/rev/product 身份侧收据的 `.partial`，持有 flock；只清理本身份未完成归档。完整 marker 必须精确匹配40位rev；旧无owner半成品具名拒绝，须操作者隔离，不猜测归属。归档完成后可接续产品拷贝，最后 rename 发布。它不自动接管旧现场半成品。
- chain-run 在创建预算/锁/执行子进程之前拒绝 repo/out 之外的 OUT 和越界软链，不改 cx_rmtree 全局白名单。
- 基线 wave_phase 只在尚无 start 检查点时清理该作业，跳过 start 时不清理；finish 继续由 controller 校验身份，最终 canonical union 仍重新验证。不得以 checkpoint 缓存资格结论。
- 最窄原生candidate夹具按`--jobs ... tool --job-index 0 --resume --until jobs-tool-0-start`后单独`--until jobs-tool-0-finish`执行；trace实测start只一次，篡改start产物身份时finish非零，恢复身份后finish通过。60acc基线这条调用路径未发生清掉start重做，因此不额外修改candidate-chain现有清理逻辑；现场失败由仓外OUT前置拒绝避免。

- build 54993ms 是 controller `-O2 source-package.cx` 实编，ctx 51123ms 还会默认冷编同一 source-package 后执行 prepare-context。建议下一集成窗口将 ctx 调用改为已编 controller 的 prepare-context，保持 argv/root/保存bootstrap身份；这需要 controller 行为等价夹具，不在本补丁偷偷改产品构造。build 自身若仍贴界，应拆 controller 前端/对象/链接并保存输入身份收据，禁止55秒无进展盲重试。当前未测各冷编子段，不宣称已获得余量。
- 旧完整快照只有 `.release-rev`、没有新 `.chain-snap` 时安全拒绝复用，不删除、不写它；集成者可先保全目录再另建，不自动升级现场。新完整收据在发布rename之前原子写入，复用重验root/rev和launcher/build-json实际SHA；原子rename不等同断电fsync保证，半截marker一律拒绝，不当完成。
- 具体拆分建议(未实测)：`B/source-package-O2.elf prepare-context S B/Root41-bootstrap.com CC CTX` 替换ctx二次源码启动；prepare收据绑定完整rev、S绝对root、bootstrap/controller/cc SHA和argv，完成后原子写入，续跑重验后接纳。build进一步可拆`controller-front`、`controller-objects`、`controller-link`，分别绑定源码闭包与前一相SHA；这些是待实现方案，不是本次已通过的构造路径。


## 检查的原生准备与限时消费

`cx-register`、`slice-freeze-tool`、`slice-inventory` 使用显式材料 `regprep-dir=ABS_OUT`，不在 `/tmp` 隐式造夹具。先在仓根分别执行 checker 的 `--prepare ABS_OUT`，再把对应目录交给 `tests/run-batch.cx ROOT ABS_OUT_PARENT ID regprep-dir=ABS_OUT`。三个 checker 分别是 `tools/check-register.cx`、`tests/check-slice-freeze.cx`、`tools/check-slice-inventory.cx`。目录不可混用，不能并发消费同一库存目录。

准备复用既有 `cxlib.runnerprep` 内容身份原生映像；`chain-phases-gen` 沿 `cxlib.toolprep` 在断言前实编生成器。准备输出只记 `PREPARED ... no-test-result`，从不保存或读取 PASS。登记器自身在加锁前准备 `tests/run.cx` 与 `tests/run-batch.cx`，缓存位于当前私有根 `out/register-native`。55 秒批次与 affected 的54秒界均未调整。

库存准备逐个实际编译11个非空 `-funit` 对象，证书绑定产品、每个源及每个真实对象的SHA和大小。消费重新核证书、真实闭包与对象字节，再执行原排序、标签和拒绝断言；同字节排序条件仍复制真实对象，不制造空片，不缓存结论。六个既有分批均保留，依次消费，根外路径负例最后运行。

`tools/slice-inventory.cx` 的直接调用还须显式 `--out ABS_DIR --index-native ABS_BIN`。ABS_BIN由同根 `cxlib.runnerprep` 准备 `tools/symindex.cx`；每次盘点在ABS_DIR下新建索引并执行 `update --verify`，不复用索引结论。现有object-source-replay调用同步迁移，消费者原45秒deadline不变。


SFT-FIX 根因是 `pp_copy_product` 在split成员复制前就写入build记录，缺entry/resource虽真实拒绝，却留下误导完成记录。现在先复制全部成员，核源与目标成员SHA，再发布记录；开始复制时撤去旧目标记录。该语义同时影响 release-build、integrate-patch 及使用该API的隔离夹具；缺entry、缺receipt成员两条断言仍原样执行。独立phasesgen补丁的准备意图按本基线必要部分迁入，不重复叠入过期事实计数上下文。

REG-PREP 重基补丁独立基为 `67fb47417b7c053217a1aa1d307eb9ab9a5bef9b`。该基线已包含 A2p 的 `cxlib.chainfacts` 动态阶段计数、事实漂移负例及 phasesgen 独立预编译；重基保留这些实现与登记/调用参数，不再迁入旧475/485或按表行数反推预期的实现。原始基线日志仅作历史记录，不代表本基线执行结果。


## K10 有界逐相与进程所有权

`chain-run` 在创建会话及执行准备前验证整个 `build/chain-phases.tsv`，每相预算必须在1..55秒；调用实际使用该行预算，总deadline与尝试/恢复次数持久保存，换进程不重置。动态front继承所属静态相预算，不扩大55秒。

工具面局部模块 `cxlib.chainrunproc` 使用真实fork返回PID、PGID与/proc starttime，且每个身份持有Linux pidfd；不从日志或heartbeat文本获取清理目标。spawn前要求single-flight且启用subreaper，根进程经门管道等待身份固定后才exec；接管的孤儿必须是本runner的子进程。仅对经身份重核的pidfd发STOP/KILL，避免PID复用与其他窗口误杀。执行和清理共享单调时钟上界，预留最多2秒有界整树清理，核实终态零存活才允许有限恢复。pidfd不可用、身份容量超限或清理不确定均具名阻塞，不降级到裸PID/group kill。`UNISACC_CHAINRUN_FAULT=unconfirmed`只用于轻夹具：真实清理照常完成，再将终态标不确定以测试拒续；不制造PID、不跳过清理、不模拟成功。

仓外OUT仍在任何子调用前拒绝；chainfix-r的repo/out与软链约束不放宽。此工具变更不修改产品输入，不代表真链或产品资格通过。

K10静态顺序为 `snap → build-objects → build → wave-objects → wave-runner → ctx`，总488相。两个对象相先以 `-cx-plan SOURCE`取得完整闭包，再将全部路径显式作为聚合 `-O2 -c` roots（普通单根 `-c` 默认funit会丢依赖，因此不能使用）；链接相只链接已校验的聚合对象。controller/wave的对象与ELF各有独立SHA收据和真实argv表，收据原子rename发布，输出/源码/bootstrap/producer不符拒绝，不用timeout作为进展。outer mark间中断后只核收据，不重复已完成编译；对象阶段与链接阶段分别checkpoint。ctx直接运行已编controller，已有完整context仅核输入请求及原controller验证。wave在独立相预编为原生runner，逐job启动不再源码冷编source-job-wave，内层旧58秒预算亦收口55秒。

新增工具名已在完整git历史查重：`tools/cxlib/chainrunproc.cx`、`tools/check-chainrunproc.cx`此前均无历史；实际编译库存 `tests/tools-compile-sources.tsv`、行为批次与覆盖登记同步新增，20261110库存是历史快照不回写成当日盘点。真链、实际controller冷编耗时、跨宿主以及产品资格均未实跑；轻假链只证明预算/身份/恢复边界，不证明真实每相余量。

本次最终交付基为 `1e269c53bbbb7cd9e9819f808db604fd8aac17b8`，其后只叠完整替代包 `c57-g2batch-r4.patch` SHA256 `5a6a75aeab09667d0bbd72e4a7c3f3ec889a705fc09bf0ecfd5c36bfa4a7e2f3`，不重复叠旧R3/R2。K10保留生产语义与G2身份恢复；十项cx与阶段表对照已验R2树同字节。依定稿验收，本次在新父树真实 `tests/run-batch` 新跑chainresume(13项,daily)、chainrunproc(7项,commit)、chain-run(29项,commit)，均输出BATCH_RUN status=PASS、rc=0、transport=0且精确marker匹配；准备仅PREPARED。修复重导表尾重复chainresume/coverage登记，保留主线CHECK_CHAINRESUME合同，不删断言、不改tier/55秒。另新跑生成--check及fresh apply检查；账本独立TSV，不入patch，不跑真链/广门。

## K9 定稿T3依赖与批次合同

494相七列表保留deps、lane与事实并行度，六共享prepare先行；Linux对象/闭包/链接各55秒。目标最多4路，parallel1完整覆盖；独立fork owner与pidfd/starttime保护兄弟，失败停止派发并收齐在途，超时仅持久降载。prep逐调用锁及Mac stub/B锁保留，短命/proc终态竞态最小修正保留，G2 guard协议不倒退。

K9-r3重导在1e269c53+完整G2-r4+K10-r3之上，保全K10去重的chainresume批次及CHECK_CHAINRESUME合同。既有chain-run、chain-phases-gen、chainrunproc及新增candidate-chain-lanes登记均按tests/run-batch真实新跑，55秒不变；候选原生映像按材料声明准备，PREPARED不是结果。该轮精确结果记独立ledger-r3。未跑真链、产品构造、广门或性能目标；不授产品资格。

K9-r3四批实际输出BATCH_RUN status=PASS、rc=0、transport=0，精确marker分别CHECK_CHAIN_RUN(46)、LANE_SPLIT(31)、CHECK_CHAIN_PHASES_GEN(10)、CHECK_CHAINRUNPROC(7)。新增lane专批依赖原生工具材料，按既有材料合同登记daily，不降档任何既有commit项；首次预检拒绝及lane输出父目录未创建的失败日志保留，后者仅补夹具创建自己的OUT后重新实跑通过，不删断言或放宽预算。

K9-r4基为origin/main `02057cd64887801e8d315610a1da90efb77db020`，已含G2-r4及K10-r3，不重叠依赖。门报新增截断调用改为检查tx_copy返回值；lane夹具不再复制snapshot布局，改消费候选只读`--chain-work ROOT REV`查询，该查询与真实执行共用chain_paths推导，不拼拆路径字面绕门。所有31项断言保留。定向cxtrunc对三文件0调用、outdep对candidate及lane两批0输出字面，基线未改；46/31/10/7四组在该树与当前已装产品真实run-batch新跑PASS。证据保留本克隆out/k9-r4，不经仓外中转；未跑真链、广门或性能目标。

K9-r5仍基`02057cd64887801e8d315610a1da90efb77db020`，完整保留K9-r4行为，只迁入AnchorBan58已验证的`tools/check-graph-check.cx`结构变异组件。该组件复用既有refactor find调用/参数事实，以generation=1参数筛选两个Win span，不再假设所有wf_win_span字面调用都属于gen1；refactor、chainargv及生产文件不随组件迁移。原route-real必须0、Win-as-Mac必须拒绝且route_mismatch=8与指定stub/units诊断均不放宽。新树真实graph-check批次29/0且BATCH_RUN PASS；全量tools/ratchets.cx实际13/0（28511ms），未改任何棘轮基线。证据在本克隆out/k9-r5，未跑真链或额外广验。
