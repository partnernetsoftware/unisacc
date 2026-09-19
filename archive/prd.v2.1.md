# UNISA SH —— 纯命令行规格 v2.1

按本规格实现 **UNISA SH**。不要做 Web 应用、不要 React、不要演示页。交付一个**命令行的「编译器 + 训练器 + 推理器」**：训练微型表网络，把 C99 子集编译到通用 tape，再 lower 到 6 条 ISA，各自执行并产生完全一致的 stdout，同时把真实的权重二进制和目标文件镜像写到磁盘。

语言：**Python 3.11+**，**仅标准库**（无 numpy / torch）。单一包 `unisa/`。后续可选：一个极小的 C gemv kernel。在 Python CLI 通过下方全部验收之前，不要动 Rust。

做到 `unisa train && unisa run examples/hello.c --fold` 打印 6/6 match，然后停。

> v2 把 v1 中所有会导致实现分叉的歧义钉死。v1 的命题、形状、种子、gold 语义一律没有削弱。§14 逐条列出改了什么。

**这个实验在证明什么。** 一个决策层由**确定性神经策略**驱动的 C99 编译器（对标 tinycc / `tcc -run`），支持 6 个目标架构，总体积小到离谱。它不是"AI 写代码"，恰恰相反：编译器的决策表是**学出来的产物**，而这种产物比它所替代的手写 switch 更小、可热插拔、形式统一。真正让论点成立的交付物，是 §13.1 那张体积/速度表——旁边摆一个 `tcc` 二进制。

---

## 1. 命题（不可协商）

Shell = **推理器 + 执行器 + 模型数据**。

- 走查器 / 重定位 / ELF-MachO-PE 头 属于**经典代码**（代数）。
- 网络只做**最后一公里的表选择**（离散 key → 1 个类）。
- Gold 表是验证器。在网络于**完整** gold 语料上达到 **acc ≥ 0.85** 之前，走查器用 gold；之后用网络。acc ≥ 0.995 时训练热跳过，每 6 步才训一次。
- 到处只有一个 kernel：**embed → gemv → ReLU → gemv → argmax**。部署期**无 softmax、无 libm**。
- 6 个目标，仅 64 位：`lnx|osx|win` × `x86_64|arm64`。同一条 tape，六份镜像，stdout / exit 必须一致。
- 换权重即换能力。kernel 不变。

不要神经化递归下降走查器、符号表、目标文件头。要神经化每一个**表形状**的阶段。

### 1.1 确定性契约

不可字节复现的编译器不算编译器。这一条压过所有 ML 习惯：

- **不用 dropout、不加权重噪声、不做 label smoothing、不做数据增广、推理期不用任何 RNG。** 没有任何随机性能活到推理阶段——推理就是 `argmax`，且**平局取最小类下标**。
- 随机性只存在于初始化和 batch 排序，且只来自**按网络 seed 初始化的 mulberry32**；每个 epoch 的 batch 置换用 `seed ^ epoch` 作种。相同种子 → 每次运行、每台机器上都得到逐字节相同的权重。
- `gemv` 的浮点累加顺序**固定**（下标升序，禁止 pairwise / 多线程重排），保证跨构建位稳定。
- 同一份源码 + 同一份权重，`unisa compile` 产出**逐字节相同的镜像**。任何头部不得含时间戳、路径、build ID。
- 网络能决定的，gold 必须能给出完全相同的决定。两者不一致就是其中之一有 bug，绝不是"模型方差"。

### 1.2 目标是超级拟合，不是"过拟合"

每个阶段都是一个**有限离散全函数**。不存在需要泛化过去的留出分布——完整笛卡尔积**就是**定义域。所以记忆即规格：

```
NET_READY = 0.85    运行时接管门槛（保证训练中途编译器依然可用）
NET_HOT   = 0.995   训练热跳过门槛
SHIP_ACC  = 1.000   出货门槛 —— `unisa ship` 与验收拒绝任何低于此值的结果
```

`--holdout` 只作为**诊断**存在（用于回答：网络是找到了结构，还是纯查表记忆？），出货运行绝不允许开启。holdout 精度作为实验发现记录，但门禁一律以完整语料为准。

若某阶段无法达到 1.000：**是 gold 的 key 编码错了。** 去修编码。只有在编码已被证明正确之后才允许加宽，且任何宽度变更都必须记入 `MANIFEST.json`。

---

## 2. CLI

```
unisa train [--epochs N] [--holdout none|random15|osx/arm64] [--out weights/]
unisa acc
unisa compile in.c -o out --target lnx/x86_64
unisa run in.c [--target T] [--fold] [--drive gold|spec|combo] [--fault NAME]
unisa tape in.c
unisa lower OP --target T
unisa ship [--dtype q2|q4|i8|f16|f32] [--out kit.zip]
unisa dump-weights [--dtype q2|q4|i8|f16|f32] --out weights/
unisa quant [--stage S]        # 量化阶梯报告（§12.1）
unisa bench [--n N]            # 每阶段决策吞吐（§13.1）
```

`unisa run file.c --fold` 就是产品本体。权重缺失时首次运行自动训练。

`--drive`（默认 `spec`）：`gold` 强制所有阶段走 gold 表；`spec` = isel∘abi 两个 StageNet；`combo` = 单个 UnisaNet 供 9 个头。前端表（pp/lex/parse/type/scope/irsel/enc/reloc）不受 `--drive` 影响。

`--fault` 是 `--fold` 的**反向对照**（§11.3），正常运行不出现。

退出码：`0` 成功；`1` 编译错误；`2` `--fold` 未达 6/6。

---

## 3. 管线（C → 镜像）

```
src
 → pp      TableNet  dir × defined → take|skip|pop|macro
 → lex     TableNet  charclass × peekclass → act
 → parse   TableNet  NT × TOK → production
 → type    TableNet  t1 × op × t2 → ty
 → scope   TableNet  ctx × kind → action
 → irsel   TableNet  family × flavor → recipe
 → isel    StageNet  op×arch → form,symbol,gate
 → abi     StageNet  op×os×arch + bilinear → sysno,arg0-2,ret,tls
 → enc     TableNet  op × os × arch → syscall|svc|winapi|x86|arm
 → reloc   TableNet  jmpkind × arch → rel32|arm26|arm19
 → exec    classic   汇编 + 包 ELF/Mach-O/PE + 解释执行
```

### 3.1 Oracle —— 唯一的分发点

所有神经阶段只经由一次调用抵达：

```
oracle.ask(stage, key_tuple) -> class_name
```

- 网络存在**且** `net.acc >= NET_READY`（在**完整** gold 语料上测得，绝不用 batch acc）→ 用网络 argmax
- 否则 → 查 gold 表

Oracle 逐阶段统计 net / gold 的使用次数，`unisa run` 结束时打印 `nets: k/10 driven`。这让"gold 是验证器"成为**运行时事实**而非训练期口号：编译器在训练曲线的任何一点都是正确的，网络是渐进接管的。

---

## 4. TableNet

```
embed keys (d=8) → gemv W1[h0,16]+b1 → ReLU → gemv W2[16,nout]+b2 → argmax
h0 = d * (key 字段数)
train: softmax CE, Adam β1=0.9 β2=0.999 eps=1e-8
init: E N(0,0.08); W1 N(0,sqrt(2/h0)); b=0; W2 N(0,sqrt(2/h))
rng: mulberry32
```

lex / pp 用 d=6 h=12；reloc 用 d=6 h=8。种子：parse13 type17 scope19 pp23 enc29 lex31 reloc37 irsel11。

`fit`：acc≥0.995 且 step%6≠0 时热跳过。永远评估**完整** gold，绝不用 batch acc 判定 ready。

`NET_READY=0.85  NET_HOT=0.995`

**mulberry32**（逐位精确，每一步都做 32 位截断）：

```
s = (s + 0x6D2B79F5) & M32 ; t = s
t = imul32(t ^ (t >>> 15), t | 1)
t = t ^ ((t + imul32(t ^ (t >>> 7), t | 61)) & M32)
u = (t ^ (t >>> 14)) & M32  ;  return u / 2^32
```

高斯 = 对两个相邻均匀数做 Box-Muller。训练期可用 libm；**部署期不可**。

---

## 5. StageNet / combo

isel：emb op12+arch8，h1=24 h2=16，heads form/symbol/gate，seed 3。
abi：emb op12+os8+arch8，h1=32 h2=20，heads sysno/arg0-2/ret/tls，bilinear 交互 8d，seed 5。

combo ~10.5kθ，seed 7：`E_op[38,16] E_os[3,8] E_arch[2,8]`；h0=52 的拼装方式为

```
h0 = E_op(16) ++ E_os(8) ++ E_arch(8) ++ factor(12) ++ bilinear(8)      = 52
factor   = ReLU(W_op·E_op + W_os·E_os + W_arch·E_arch + b)              → 12d
bilinear = (P_op·E_op) ⊙ (P_os·E_os + P_arch·E_arch)                    → 8d
```

随后 `W1 52×48 → ReLU → W2 48×32 → ReLU → 9 个头`。4 个寄存器头（arg0,arg1,arg2,ret）共享一个 `W_reg[32,|REGS|]`，各自保留独立 bias。

**LR 是 epoch 衰减表，不是按参数量分档**：`epoch<18 → 0.032；<50 → 0.014；<90 → 0.006；else 0.0025`。Batch 16。所有网络共用这张表。

### 5.1 9 个头

| head | 来源 | 词表 |
|---|---|---|
| form | isel | FORMS |
| symbol | isel | SYMS |
| gate | isel | GATES |
| sysno | abi | SYSNOS |
| arg0, arg1, arg2, ret | abi | REGS（共享 W_reg） |
| tls | abi | TLS |

```
FORMS = syscall svc winapi x86 arm
GATES = syscall svc0 svc80 winapi none
REGS  = rdi rsi rdx r10 rcx r8 r9 rax x0 x1 x2 x3 x4 x5 x6 x7 x8 none
TLS   = fsbase tpidr_el0 teb none
SYMS, SYSNOS = §8 派生函数实际吐出值的有序并集，外加 `none`
```

---

## 6. Op 词表（N = 38）

```
SYSOPS(19) = exit read write open close mmap munmap mprotect getpid
             clock_gettime nanosleep futex socket connect bind listen
             accept clone execve
MOPS(19)   = add64 sub64 xor64 mul64 slt64 sle64 load64 store64 jump
             jumpz call ret nop cas64 fence syscall_gate tls_base
             cycle_counter stack_enter
OPS = SYSOPS ++ MOPS
```

`OPS` 是 isel、abi、enc、combo 共用的 op 轴。顺序按上文固定，下标即 embedding 行号。

---

## 7. Gold（照抄）

每个阶段声明 `[(field, vocab), ...]` 与一个标签函数。**gold 语料就是各 key 词表的完整笛卡尔积**——这正是"FULL gold"的含义，枚举代价极低，且强制每个网络学成一个全函数。

**parse** NT={top,stmt,unary,postfix,after_name} × TOKS(54) → PRODS(34)。270 行。

```
默认:     top=global  stmt=expr  unary=prim  postfix=done  after_name=var_def
覆盖:     top/eof=end | top/typedef=typedef | top/struct=struct | top/enum=enum
          after_name/( = fn_sig
          stmt/type=decl | stmt/{ = block
          stmt/X = X     X ∈ {if,while,for,do,switch,case,default,return,break,continue}
          unary/- = neg | unary/! = not | unary/* = deref | unary/& = addr | unary/sizeof = sizeof
          postfix/[ = index | postfix/( = call | postfix/++ = inc | postfix/-- = inc
          postfix/. = field | postfix/-> = field
```

```
TOKS  = eof type id num str if else while for do switch case default return break
        continue sizeof struct typedef enum { } ( ) [ ] ; , = += -= *= /= ? : + - * / %
        == != < > <= >= && || ! & ++ -- . ->
PRODS = end fn global typedef struct enum decl if while for do switch case default
        return break continue block expr neg not deref addr sizeof prim index call
        inc field done fn_sig var_def
```

**type** TYS={void,i8,i32,i64,ptr,arr,struct,fn} × TOPS(15) × TYS → TYS|illegal。960 行。

```
TOPS = + - * / % < == = & [] . call sizeof , un*      （un* 为一元解引用，忽略 t2）
默认 illegal
数值算术 (+ - * / %) → 任一为 i8 则 i32，否则 i64
< 与 ==             → i64
=                   → lhs
sizeof              → i64
ptr|arr + num       → ptr
[]                  → i64
ptr - ptr           → i64
ptr un*             → i64
fn call             → i64
i64 & i64           → ptr
struct .            → i64
TY_SIZE: void=1 i8=1 i32=4 其余=8
```

`illegal` 在笛卡尔积中占绝对多数，因此**训练时把 illegal 行按 1/19 下采样**；评估永远用全部 960 行。

**scope** CTX={top,param,local,expr,sizeof,field} × KIND={type_kw,id,typedef_id,star,lparen} → ACTS。30 行。

```
ACTS = bind_global bind_param bind_local lookup type_name fn_name field
默认 lookup
top/type_kw = top/typedef_id = type_name | top/id = bind_global | top/lparen = fn_name
param/id = bind_param | local/id = bind_local
local/type_kw = local/typedef_id = type_name
sizeof/type_kw = sizeof/typedef_id = type_name
field/id = field
```

**pp** DIRS={ifdef,ifndef,if,elif,else,endif,define,include,undef} × {0,1} → take|skip|pop|macro。18 行。

```
默认 skip
ifdef: 1=take 0=skip           ifndef: 取反
if/elif/else: 跟随 flag         endif=pop
define=undef=macro             include=skip
```

**lex** CHARC(11) × peek CHARC(11) → ACT。121 行。

```
CHARC = ws nl A d q sq slash star punct eof other
ACT   = skip nl ident num str charlit cmt linecmt op bad
ws→skip  nl→nl  A→ident  d→num  q→str  sq→charlit
slash|star|punct→op  eof→skip  other→bad
slash×slash = linecmt   slash×star = cmt
```

**enc** op(38) × os(3) × arch(2) → FORMS。228 行。

```
win 且 op ∉ MOPS       → winapi
arm64 且 op ∉ MOPS     → svc
op ∈ SYSOPS 且 (x86_64, lnx|osx) → syscall
否则                   → 按 arch（x86_64 → x86，arm64 → arm）
```

**reloc** jmpkind{jmp,jz,call} × arch → rel32|arm26|arm19。6 行。

```
x86_64 → rel32 ; arm64 且 jz → arm19 ; 否则 → arm26
```

**irsel** family(5) × flavor(27) → recipe|bad。135 行，其中 27 行有效。

```
alu : add sub mul lt le gt ge eq ne neg → add64 sub64 mul64 slt64 sle64 slt64 sle64 eq ne sub64
mem : load store lea ld st zero         → load64 store64 lea ld st zero
ctrl: jump jumpz ret                    → jump jumpz ret
call: call push arg frame               → call callpush arg frame
lit : imm print write exit              → imm print write exit
默认 bad
```

（`gt`/`ge` 映射到 `slt64`/`sle64`——由走查器交换操作数。这种不对称正是最后一公里的表该承载的事实。）

---

## 8. Catalog 系统调用（lnx-x64 / lnx-arm / osx / win）

exit 60/93/1 ExitProcess；read 0/63/3 ReadFile；write 1/64/4 WriteFile；open 2/56/5 CreateFileW（lnx-arm 为 openat）；close 3/57/6 CloseHandle；mmap 9/222/197 VirtualAlloc；munmap 11/215/73 VirtualFree；mprotect 10/226/74 VirtualProtect；getpid 39/172/20 GetCurrentProcessId；clock_gettime 228/113/116 QPC（osx 为 gettimeofday）；nanosleep 35/101/240 Sleep；futex 202/98/515 WaitOnAddress（osx 为 ulock_wait）；socket 41/198/97 WSASocketW；connect 42/203/98 connect；bind 49/200/104 bind；listen 50/201/106 listen；accept 43/202/30 accept；clone 56/220/360 CreateThread（osx 为 bsdthread_create）；execve 59/221/59 CreateProcessW。

**osx sysno = `0x02000000 | nr`。** 这个 class bit 是承重的——§11.3 正是用"抽掉它"作为反向对照。

ABI：lnx/osx x64 参数 rdi rsi rdx r10，返回 rax，gate `syscall`；lnx arm 用 x0..，gate `svc #0`；osx arm gate `svc #0x80`；win x64 参数 rcx rdx r8 r9，form `winapi`，gate `winapi`。

字节：`add64` = `48 01 f0` / `00 00 01 8b`；`ret` = `c3` / `c0 03 5f d6`；`syscall` = `0f 05` / `01 00 00 d4`。

**9 头 gold 必须由一个 (op, os, arch) 上的函数派生，不许手工标注。** 该函数是 isel、abi、enc、combo 的唯一真源；§5.1 中 `SYMS`/`SYSNOS` 的内容就是它实际吐出的值。

MOPS 的寄存器映射，r0–r7 → `rax rdi rsi rdx rcx r8 r9 r10`（x86_64）/ `x0 x1 x2 x3 x4 x5 x6 x7`（arm64）。
除 `tls_base` 外，MOPS 的 arg0-2 / ret / tls 一律为 `none`；`tls_base` 的 tls 按 os 取 `fsbase|tpidr_el0|teb`。
win 下 `sysno = none`，WinAPI 名字落在 `symbol` 上。

---

## 9. Tape

按行的文本格式。标签写作 `L:`。寄存器 `r0–r7`，其中 `r7` 为 SP，初值 `0x10000`。内存 64 KB，小端。**tape 解释器是 `--fold` 的基准真值。**

```
imm    rd, K              rd = K
mov    rd, rs
add64  rd, ra, rb         sub64 mul64 xor64 同理
.div   rd, ra, rb         有符号截断除；除零/取模零 → exit 136
.mod   rd, ra, rb
slt64  rd, ra, rb         sle64 eq ne 同理 → 0|1
load64 rd, [rb+K]         store64 [rb+K], rs
.ld    rd, [rb+K], W      W ∈ {1,2,4,8}，符号扩展
.st    [rb+K], rs, W
.lea   rd, sym|K
.zero  [rb+K], N
jump   L                  jumpz ra, L
call   L                  ret
.frame N                  r7 -= N
.arg   i, rs              为下一次 call / builtin 就位第 i 个参数
.print ra                 int64 按十进制输出，不带换行，写 stdout
.write ptr, len           原始字节写 stdout（fd 1）
.exit  ra
nop
```

算术按 2^64 取模、有符号二补码。`.print` / `.write` / `.exit` 是仅有的三个有外部副作用的 op，且它们在 lowering 之后都必须变成真实的 syscall / WinAPI 调用。

---

## 10. C99 子集 / 样例

C99 走查器覆盖：`#if` 家族、函数 / 指针 / 数组 / struct / typedef、if/for/while/do/switch、算术、`printf` → `.print`/`.write`。`printf` 在走查期依据**静态格式串**脱糖（支持 `%d %s %c %%`），运行时不带格式化器。

| 样例 | stdout | 说明 |
|---|---|---|
| hello.c | `hello from C99\n` | |
| fact.c | `120\n` | 5! |
| switch.c | `6\n` | |
| do.c | `3\n` | |
| fib.c | `55\n` | fib(10) |
| ptr.c | `7\n` | |
| struct.c | `9\n` | |
| host.c | 随 OS 而变 | **允许跨 OS 不同，绝不允许跨 arch 不同** |

hello / fact / ptr / fib / switch / struct / do 全部必须 **6/6**。`host.c` 按三个 OS 分组，每组 2/2。

`main` 的返回值即进程退出码，也必须在整个 fold 中一致。

---

## 11. 镜像与 `--fold`

包成 ELF64 / Mach-O 64 / PE32+。**不要 execve，只解释执行。**

- ELF64：单个 `PT_LOAD`，vaddr `0x400000`。魔数 `7f454c46`。
- Mach-O 64：`mach_header_64` + `LC_SEGMENT_64(__TEXT)` + `LC_UNIXTHREAD`。魔数 `cffaedfe`。
- PE32+：MZ stub + `PE\0\0` + optional header 魔数 `0x20b` + 单个 `.text`。魔数 `4d5a`。

### 11.1 `--fold` 到底做什么

对 6 个目标各自独立走一遍：

```
tape → lower(isel, abi, enc, reloc) → TargetProgram → 镜像  +  目标机解释执行
```

### 11.2 解释必须是目标感知的

目标解释器持有**该 arch 的具名寄存器**（`rdi rsi rdx r10 / rax` 对 `x0..x8`），其系统调用分发器**只**认 lowering 产出的 `(os, sysno)` 或 `(os, winapi_symbol)`，绝不回看通用 tape op。由此带来的后果，全部是设计意图：

- abi 给 lnx/x86_64 的 write 吐出 sysno 4（那是 osx 的号）→ Linux 分发器拒绝 → stdout 不匹配。
- osx sysno 丢掉 `0x02000000` class bit → 不匹配。
- win 路径用 `rdi` 而非 `rcx` 传 arg0 → 不匹配。

fold 若不是目标感知的，它就不是测试。**禁止**把通用 tape 跑六遍再和自己比。

### 11.3 反向对照（必做）

`unisa run examples/hello.c --fold --fault osx_class_bit` 必须打印 **4/6**，而不是 6/6。故障注入项：`osx_class_bit`、`win_argregs`、`arm_gate`。这证明 fold 是有牙齿的，属于验收的一部分。

---

## 12. UNS1

头部 16B：`UNS1` | dtype u8 | flags u8 | nTensors u16 | nParams u32 | acc f32。
张量：`name[16]` | rows u16 | cols u16 | scale f32 | payload | 补齐到 4。

```
dtype: 0=i8  1=f16  2=f32  3=q4  4=q2
i8 : 整张量  scale = maxabs/127        1 值 / 字节
q4 : 按行    scale = maxabs/7          取值 -7..7，   2 值 / 字节
q2 : 按行    scale = maxabs            取值 {-1,0,1}，4 值 / 字节
```

按行 scale 存为 payload 前的一段 `f32[rows]`；此时张量头里的 `scale` 取该段的最大值（这样忽略行 scale 的读取方也能拿到一个合理上界）。亚字节 payload 按**低半字节 / 低 2 位在先**打包，行主序，每行从字节边界开始。

Ship 压缩包：`weights/*.unisa` + `MANIFEST.json` + `kernel/unisa_boot.c` + 镜像。i8 下约 50 KB。

`unisa_boot.c` 是部署 kernel：读 UNS1，跑 embed → gemv → ReLU → gemv → argmax。**无 softmax、无 libm、热路径无 malloc。** 整数 dtype 的 gemv 用 `int32` 累加器，scale 每个输出只乘一次——内层循环里没有浮点。

### 12.1 量化阶梯 —— 真正的实验

超级拟合（§1.2）会换来极大的 logit margin，所以问题不是"q2 会损失多少精度"，而是 **"q2 到底会不会改变任何一个决策"**。某个 dtype 能否被接受，唯一判据是：

> **argmax 不变性：** 对完整 gold 语料中的每一个 key，量化后的网络必须选出与 f32 **完全相同的类下标**。

`unisa quant` 对每个阶段沿 `f32 → f16 → i8 → q4 → q2` 逐级下探，报告该阶段能保持 argmax 不变的最低 dtype，并把结果写进 `MANIFEST.json`：

```
stage    θ      f32     i8     q4     q2    min-margin   ship
parse    1642   6568B   1642B  821B   411B  ...          q?
...
TOTAL           ...
```

每个阶段按**各自**的最低不变 dtype 出货——混合 dtype 的 kit 是预期结果，也正是重点所在。某个阶段扛不住 q2，那是关于这张表结构的**发现**，不是失败。逐阶段记录最小 logit margin，它能预测阶梯落点。

---

## 13. 训练 / 验收

Epoch 循环：combo + isel + abi 按 batch 16；前端网络；每个 epoch 评估一次完整 gold。
停机条件：**所有网络达到 1.000**（§1.2），或到达 `--epochs`（默认 90）。v1 的下限——`combo ≥ 0.985 && 所有 table ≥ 0.85 && epoch > 30`——保留为**最低可用**状态，而不是停机点；停在该状态的运行报告 `UNDERFIT`，且被 `unisa ship` 拒绝。

`--holdout`（仅诊断，绝不出货）：`none`；`random15` = 扣掉 15% 的行不参与训练，但全部行仍参与评估；`osx/arm64` = 扣掉所有 os=osx ∧ arch=arm64 的行（仅适用于 enc/isel/abi/combo；前端表没有 os/arch 轴，回落为 `none`）。

验收项：

- `unisa run examples/hello.c --fold` → 6/6，`hello from C99\n`
- fact→120、switch→6、do→3、fib→55、ptr→7、struct→9；全部 6/6
- `host.c` → 每个 OS 组内 2/2
- `unisa run examples/hello.c --fold --fault osx_class_bit` → 4/6
- `unisa compile` → ELF `7f454c46`、Mach-O `cffaedfe`、PE `4d5a`；**同一输入跑两次字节完全相同**（§1.1）
- `unisa dump-weights --dtype i8` → `weights/parse.i8.unisa` 以 `554e5331` 开头
- `unisa acc` → **每个阶段 = 1.000**
- `unisa quant` → 每个出货阶段在其记录的 dtype 下 argmax 不变
- 从干净状态跑两次 `unisa train` → **权重字节完全相同**
- `unisa ship` → kit.zip 内含 weights/ + MANIFEST.json + kernel/unisa_boot.c + 镜像

构建顺序：TableNet+gold → tape+VM → lowering → C 走查器 → 封装 → ship。

**若 acc 卡在 1.000 以下，是 gold 的 key 编码错了——不要加宽网络。**

### 13.1 体积与速度预算

头号主张是体积，那就把它量出来，并摆到现有方案旁边。

| 指标 | 怎么测 | 目标 |
|---|---|---|
| 总 θ | 所有网络求和 | 记录 |
| kit 字节数 | `unisa ship` 按逐阶段最低 dtype | 权重 **≤ 32 KB**；kit ≤ 64 KB |
| 对比 tinycc | `size $(which tcc)` 或 tcc 发布二进制 | 记录比值 |
| 决策吞吐 | `unisa bench`，逐阶段，冷（不带 memo） | Python ≥ 50k/s；`unisa_boot.c` ≥ 5M/s |
| 端到端编译耗时 | `unisa run examples/fact.c` | ≤ 1 s |

`unisa bench` 必须报告**冷**数据。在一个确定性全函数之上加 memo 缓存是正当的工程手段，`run` 可以开；但 `bench`、`acc`、`quant` 必须关掉——否则测的是 dict，不是 kernel。

---

## 14. v2 相对 v1 钉死了什么

按"会先咬人"的顺序列出已消解的歧义：

1. **LR 是 epoch 衰减表**，不是按参数量分档（§5）。v1 的 `<18 / <50 / <90` 紧挨着 "epoch 90"，只有当成衰减表才自洽。
2. **`h0 = 52` 给出了分解**（§5）——v1 只给了总数，而 16+8+8+12 凑不到 52。
3. **9 个头连同词表被完整枚举**（§5.1）；v1 只是顺带提了名字。
4. **`OPS` 固定为 38 个且顺序写死**（§6）——所有 op 轴 embedding 都依赖它。
5. **"FULL gold" 定义为 key 词表的完整笛卡尔积**（§7），这才让 acc 门禁有意义。
6. **parse 覆盖规则拆行**（§7）；v1 的覆盖行无法无歧义地切词（`postfix/++/--=inc`）。
7. **type 的 `1/19 illegal`** 解读为：训练时下采样 illegal，评估用全部 960 行（§7）。
8. **enc / reloc / irsel 的默认分支补全**，irsel 词表加入 `bad`。
9. **补上 connect/bind/listen/accept 缺失的 win 符号**（§8）。
10. **r0–r7 寄存器映射**，以及 MOPS 各 abi 头取 `none` 的约定（§8）。
11. **完整列出 tape 指令集**及语义、SP 约定、回绕规则（§9）。
12. **`--fold` 语义写清**（§11.1–11.2）：逐目标 lowering **且** 目标感知地解释执行。v1 的"interpret the exec stream"允许一种什么都测不到的偷懒读法。
13. **加入反向对照**（§11.3）—— fold 必须是可被证明打破的。
14. 补上 **Mach-O / PE 魔数**（原本只有 ELF）；明确 **退出码**、**`--drive`**、**`--holdout` 语义**、**Oracle 分发**（§3.1）。
15. **选定 fib/ptr/struct 的预期 stdout**（55/7/9）；v1 只给了 fact/switch/do。

v2.1 依据实验本身的意图（确定性神经策略、超级拟合、体积小到离谱、对标 tinycc）新增：

16. **确定性契约**（§1.1）—— 无 dropout / 无噪声 / 推理期无 RNG，argmax 平局规则固定，gemv 累加顺序固定，镜像与权重字节可复现。
17. **把超级拟合确立为目标**（§1.2）—— `SHIP_ACC=1.000`；`NET_READY=0.85` 降格为运行时接管门槛；`--holdout` 降格为诊断手段。
18. **新增 q4 / q2 dtype**（§12），按行 scale 与亚字节打包；整数 gemv 走 `int32`，内层无浮点。
19. **以 argmax 不变性作为量化的唯一判据**，配套 `unisa quant` 阶梯与单个 kit 内的**逐阶段混合 dtype**（§12.1）。
20. **体积与速度预算**（§13.1），含 tinycc 对比与冷缓存 `unisa bench`。

实现者仍可重新选择的：§5 的 bilinear 形式、`SYMS` 的具体拼写、fib/ptr/struct 的常量、§12 的亚字节打包顺序。除此之外，以上全部承重。
