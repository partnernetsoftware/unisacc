# UNISA SH —— 规格 v2.2

> 配套视图：[`prd.tree.md`](prd.tree.md)（树+DAG，开工用）· [`prd.map.md`](prd.map.md)（记忆宫殿，记全局用）· `archive/` 存历史版本。

**交付物**：一个命令行的「编译器 + 训练器 + 推理器」。训练微型表网络 → 把 C99 子集编译到通用 tape → lower 到 6 条 ISA → 各自执行产生完全一致的 stdout → 同时落盘真实权重二进制与目标文件镜像。

**不交付**：Web 应用、React、演示页。

**语言**：Python 3.11+，**仅标准库**。单一包 `unisa/`。可选后续：极小的 C gemv kernel。Python CLI 通过全部验收之前不碰 Rust。

**终点**：`unisa train && unisa run examples/hello.c --fold` 打印 6/6 match，然后停。

**这个实验在证明什么**：一个决策层由**确定性神经策略**驱动的 C99 编译器（对标 tinycc / `tcc -run`），支持 6 个目标，体积小到离谱。不是"AI 写代码"，恰恰相反——编译器的决策表是**学出来的产物**，而这种产物比它替代的手写 switch 更小、可热插拔、形式统一。让论点成立的交付物是 §13.1 那张体积/速度表，旁边摆一个 `tcc`。

---

## 1. 命题（不可协商）

Shell = **推理器 + 执行器 + 模型数据**。

- 走查器 / 重定位 / ELF-MachO-PE 头 = **经典代码**（代数）。
- 网络只做**最后一公里选表**（离散 key → 1 个类）。
- Gold 表是验证器。网络在**完整** gold 上达 acc ≥ 0.85 前用 gold，之后用网络。acc ≥ 0.995 时热跳过，每 6 步才训一次。
- 到处只有一个 kernel：**embed → gemv → ReLU → gemv → argmax**。部署期**无 softmax、无 libm**。
- 6 目标，仅 64 位：`lnx|osx|win` × `x86_64|arm64`。同一条 tape，六份镜像，stdout / exit 必须一致。
- 换权重即换能力，kernel 不变。

不神经化：递归下降走查器、符号表、目标文件头。神经化：每一个**表形状**的阶段。

### 1.1 确定性契约

不可字节复现的编译器不算编译器。这条压过所有 ML 习惯。

| 项 | 规定 |
|---|---|
| 随机性 | 无 dropout / 无权重噪声 / 无 label smoothing / 无增广 / **推理期无 RNG** |
| argmax | 平局取**最小类下标** |
| 训练随机源 | 仅初始化与 batch 置换，且只来自 mulberry32；置换种子 `seed ^ epoch` |
| gemv | 累加顺序固定（下标升序，禁 pairwise / 多线程重排），跨构建位稳定 |
| 镜像 | 同源码 + 同权重 → **逐字节相同**。头部无时间戳、路径、build ID |
| 一致性 | 网络与 gold 的决定必须完全相同；不一致 = 其中之一有 bug，不是"模型方差" |

### 1.2 目标是超级拟合

每个阶段都是**有限离散全函数**，完整笛卡尔积**就是**定义域，不存在要泛化过去的留出分布。记忆即规格。

```
NET_READY = 0.85    运行时接管门槛（保证训练中途编译器可用）
NET_HOT   = 0.995   训练热跳过门槛
SHIP_ACC  = 1.000   出货门槛，ship 与验收拒绝低于此值
```

`--holdout` 仅作诊断（网络是学到结构，还是纯记忆？），出货运行禁止开启；holdout 精度作实验发现记录，门禁一律以完整语料为准。

**某阶段到不了 1.000 = gold 的 key 编码错了。** 去修编码。只有编码被证明正确后才允许加宽，且必须记入 `MANIFEST.json`。

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
unisa bench [--n N]            # 逐阶段决策吞吐（§13.1）
```

`unisa run file.c --fold` 是产品本体；权重缺失时首次运行自动训练。

- `--drive`（默认 `spec`）：`gold` 强制全部走表；`spec` = isel∘abi 两个 StageNet；`combo` = 单个 UnisaNet 供 9 头。前端表不受影响。
- `--fault`：`--fold` 的反向对照（§11.3），正常运行不出现。
- 退出码：`0` 成功 / `1` 编译错误 / `2` fold 未达 6/6。

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

### 3.1 Oracle —— 唯一分发点

```
oracle.ask(stage, key_tuple) -> class_name
```

- 网络存在**且** `net.acc >= NET_READY`（在完整 gold 上测得，绝不用 batch acc）→ 网络 argmax
- 否则 → 查 gold 表

Oracle 逐阶段统计 net / gold 用量，`unisa run` 末尾打印 `nets: k/10 driven`。这让"gold 是验证器"成为**运行时事实**：编译器在训练曲线任何一点都正确，网络渐进接管。

---

## 4. TableNet

```
embed keys (d=8) → gemv W1[h0,16]+b1 → ReLU → gemv W2[16,nout]+b2 → argmax
h0 = d * (key 字段数)
train: softmax CE, Adam β1=0.9 β2=0.999 eps=1e-8
init:  E N(0,0.08); W1 N(0,sqrt(2/h0)); b=0; W2 N(0,sqrt(2/h))
rng:   mulberry32
```

lex/pp 用 d=6 h=12；reloc 用 d=6 h=8。
种子：parse13 type17 scope19 pp23 enc29 lex31 reloc37 irsel11。
`fit`：acc≥0.995 且 step%6≠0 热跳过；永远评估完整 gold，绝不用 batch acc 判 ready。

**mulberry32**（逐位精确，每步 32 位截断）：

```
s = (s + 0x6D2B79F5) & M32 ; t = s
t = imul32(t ^ (t >>> 15), t | 1)
t = t ^ ((t + imul32(t ^ (t >>> 7), t | 61)) & M32)
u = (t ^ (t >>> 14)) & M32  ;  return u / 2^32
```

高斯 = 相邻两个均匀数做 Box-Muller。训练期可用 libm，**部署期不可**。

---

## 5. StageNet / combo

- **isel**：emb op12+arch8，h1=24 h2=16，heads form/symbol/gate，seed 3
- **abi**：emb op12+os8+arch8，h1=32 h2=20，heads sysno/arg0-2/ret/tls，bilinear 8d，seed 5
- **combo** ~10.5kθ，seed 7：`E_op[38,16] E_os[3,8] E_arch[2,8]`

```
h0 = E_op(16) ++ E_os(8) ++ E_arch(8) ++ factor(12) ++ bilinear(8)      = 52
factor   = ReLU(W_op·E_op + W_os·E_os + W_arch·E_arch + b)              → 12d
bilinear = (P_op·E_op) ⊙ (P_os·E_os + P_arch·E_arch)                    → 8d
→ W1 52×48 → ReLU → W2 48×32 → ReLU → 9 heads
```

4 个寄存器头（arg0,arg1,arg2,ret）共享一个 `W_reg[32,|REGS|]`，各自独立 bias。

**LR 是 epoch 衰减表，不是按参数量分档**：`epoch<18 → 0.032；<50 → 0.014；<90 → 0.006；else 0.0025`。Batch 16，所有网络共用。

### 5.1 9 个头

| head | 来源 | 词表 |
|---|---|---|
| form | isel | FORMS |
| symbol | isel | SYMS |
| gate | isel | GATES |
| sysno | abi | SYSNOS |
| arg0 arg1 arg2 ret | abi | REGS（共享 W_reg） |
| tls | abi | TLS |

```
FORMS = syscall svc winapi x86 arm
GATES = syscall svc0 svc80 winapi none
REGS  = rdi rsi rdx r10 rcx r8 r9 rax x0 x1 x2 x3 x4 x5 x6 x7 x8 none
TLS   = fsbase tpidr_el0 teb none
SYMS, SYSNOS = §8 派生函数实际吐出值的有序并集 + `none`
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

isel / abi / enc / combo 共用此 op 轴。**顺序写死，下标即 embedding 行号。**

---

## 7. Gold

每阶段声明 `[(field, vocab), ...]` 与标签函数。**gold 语料 = 各 key 词表的完整笛卡尔积**——这就是 "FULL gold"，枚举代价极低，并强制网络学成全函数。

**parse** — NT(5) × TOKS(54) → PRODS(34)，270 行

```
默认: top=global  stmt=expr  unary=prim  postfix=done  after_name=var_def
覆盖: top/eof=end | top/typedef=typedef | top/struct=struct | top/enum=enum
      after_name/( = fn_sig
      stmt/type=decl | stmt/{ = block
      stmt/X = X     X ∈ {if,while,for,do,switch,case,default,return,break,continue}
      unary/- = neg | unary/! = not | unary/* = deref | unary/& = addr | unary/sizeof = sizeof
      postfix/[ = index | postfix/( = call | postfix/++ = inc | postfix/-- = inc
      postfix/. = field | postfix/-> = field

TOKS  = eof type id num str if else while for do switch case default return break
        continue sizeof struct typedef enum { } ( ) [ ] ; , = += -= *= /= ? : + - * / %
        == != < > <= >= && || ! & ++ -- . ->
PRODS = end fn global typedef struct enum decl if while for do switch case default
        return break continue block expr neg not deref addr sizeof prim index call
        inc field done fn_sig var_def
```

**type** — TYS(8) × TOPS(15) × TYS(8) → TYS|illegal，960 行

```
TOPS = + - * / % < == = & [] . call sizeof , un*      （un* 一元解引用，忽略 t2）
默认 illegal
数值算术 (+ - * / %) → 任一为 i8 则 i32，否则 i64        < 与 == → i64
= → lhs        sizeof → i64        ptr|arr + num → ptr        [] → i64
ptr - ptr → i64        ptr un* → i64        fn call → i64
i64 & i64 → ptr        struct . → i64
TY_SIZE: void=1 i8=1 i32=4 其余=8
```

`illegal` 占绝对多数，**训练时按 1/19 下采样 illegal 行；评估永远用全部 960 行。**

**scope** — CTX(6) × KIND(5) → ACTS(7)，30 行

```
ACTS = bind_global bind_param bind_local lookup type_name fn_name field
默认 lookup
top/type_kw = top/typedef_id = type_name | top/id = bind_global | top/lparen = fn_name
param/id = bind_param | local/id = bind_local
local/type_kw = local/typedef_id = type_name
sizeof/type_kw = sizeof/typedef_id = type_name
field/id = field
```

**pp** — DIRS(9) × {0,1} → take|skip|pop|macro，18 行

```
默认 skip
ifdef: 1=take 0=skip   ifndef: 取反   if/elif/else: 跟随 flag
endif=pop   define=undef=macro   include=skip
```

**lex** — CHARC(11) × peek CHARC(11) → ACT(10)，121 行

```
CHARC = ws nl A d q sq slash star punct eof other
ACT   = skip nl ident num str charlit cmt linecmt op bad
ws→skip  nl→nl  A→ident  d→num  q→str  sq→charlit
slash|star|punct→op  eof→skip  other→bad
slash×slash = linecmt   slash×star = cmt
```

**enc** — op(38) × os(3) × arch(2) → FORMS，228 行

```
win 且 op ∉ MOPS                  → winapi
arm64 且 op ∉ MOPS                → svc
op ∈ SYSOPS 且 (x86_64, lnx|osx)  → syscall
否则                              → 按 arch（x86_64→x86，arm64→arm）
```

**reloc** — jmpkind(3) × arch(2)，6 行

```
x86_64 → rel32 ; arm64 且 jz → arm19 ; 否则 → arm26
```

**irsel** — family(5) × flavor(27) → recipe|bad，135 行（27 有效）

```
alu : add sub mul lt le gt ge eq ne neg → add64 sub64 mul64 slt64 sle64 slt64 sle64 eq ne sub64
mem : load store lea ld st zero         → load64 store64 lea ld st zero
ctrl: jump jumpz ret                    → jump jumpz ret
call: call push arg frame               → call callpush arg frame
lit : imm print write exit              → imm print write exit
默认 bad
```

`gt`/`ge` 映射到 `slt64`/`sle64`——由走查器交换操作数。这种不对称正是最后一公里的表该承载的事实。

---

## 8. Catalog 系统调用（lnx-x64 / lnx-arm / osx / win）

exit 60/93/1 ExitProcess；read 0/63/3 ReadFile；write 1/64/4 WriteFile；open 2/56/5 CreateFileW（lnx-arm 为 openat）；close 3/57/6 CloseHandle；mmap 9/222/197 VirtualAlloc；munmap 11/215/73 VirtualFree；mprotect 10/226/74 VirtualProtect；getpid 39/172/20 GetCurrentProcessId；clock_gettime 228/113/116 QPC（osx 为 gettimeofday）；nanosleep 35/101/240 Sleep；futex 202/98/515 WaitOnAddress（osx 为 ulock_wait）；socket 41/198/97 WSASocketW；connect 42/203/98 connect；bind 49/200/104 bind；listen 50/201/106 listen；accept 43/202/30 accept；clone 56/220/360 CreateThread（osx 为 bsdthread_create）；execve 59/221/59 CreateProcessW。

**osx sysno = `0x02000000 | nr`。** 这个 class bit 承重——§11.3 用抽掉它作反向对照。

**ABI**：lnx/osx x64 参数 rdi rsi rdx r10，返回 rax，gate `syscall`；lnx arm 用 x0..，gate `svc #0`；osx arm gate `svc #0x80`；win x64 参数 rcx rdx r8 r9，form/gate 均 `winapi`。

**字节**：`add64` = `48 01 f0` / `00 00 01 8b`；`ret` = `c3` / `c0 03 5f d6`；`syscall` = `0f 05` / `01 00 00 d4`。

**9 头 gold 必须由一个 (op, os, arch) 上的函数派生，不许手工标注。** 该函数是 isel / abi / enc / combo 的唯一真源，`SYMS`/`SYSNOS` 即它吐出的值。

- MOPS 寄存器映射 r0–r7 → `rax rdi rsi rdx rcx r8 r9 r10`(x86_64) / `x0..x7`(arm64)
- 除 `tls_base` 外，MOPS 的 arg0-2/ret/tls 一律 `none`；`tls_base` 的 tls 按 os 取 `fsbase|tpidr_el0|teb`
- win 下 `sysno = none`，WinAPI 名字落在 `symbol`

---

## 9. Tape

按行文本。标签 `L:`。寄存器 `r0–r7`，`r7` 为 SP，初值 `0x10000`。内存 64 KB 小端。**tape 解释器是 `--fold` 的基准真值。**

```
imm    rd, K              rd = K
mov    rd, rs
add64  rd, ra, rb         sub64 mul64 xor64 同理
.div   rd, ra, rb         有符号截断除；除零/模零 → exit 136
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
.print ra                 int64 十进制写 stdout，不带换行
.write ptr, len           原始字节写 stdout（fd 1）
.exit  ra
nop
```

算术按 2^64 取模、有符号二补码。`.print` / `.write` / `.exit` 是仅有的三个有外部副作用的 op，lowering 后都必须变成真实 syscall / WinAPI 调用。

---

## 10. C99 子集 / 样例

覆盖：`#if` 家族、函数 / 指针 / 数组 / struct / typedef、if/for/while/do/switch、算术、`printf` → `.print`/`.write`。`printf` 在走查期按**静态格式串**脱糖（`%d %s %c %%`），运行时无格式化器。

| 样例 | stdout | 说明 |
|---|---|---|
| hello.c | `hello from C99\n` | |
| fact.c | `120\n` | 5! |
| switch.c | `6\n` | |
| do.c | `3\n` | |
| fib.c | `55\n` | fib(10) |
| ptr.c | `7\n` | |
| struct.c | `9\n` | |
| host.c | 随 OS 变 | **允许跨 OS 不同，绝不允许跨 arch 不同** |

前七个必须 **6/6**；`host.c` 按 OS 分三组，每组 2/2。`main` 返回值即退出码，也必须全 fold 一致。

---

## 11. 镜像与 `--fold`

包成 ELF64 / Mach-O 64 / PE32+。**不 execve，只解释执行。**

| 格式 | 结构 | 魔数 |
|---|---|---|
| ELF64 | 单 `PT_LOAD`，vaddr `0x400000` | `7f454c46` |
| Mach-O 64 | `mach_header_64` + `LC_SEGMENT_64(__TEXT)` + `LC_UNIXTHREAD` | `cffaedfe` |
| PE32+ | MZ stub + `PE\0\0` + optional header `0x20b` + 单 `.text` | `4d5a` |

### 11.1 `--fold` 做什么

对 6 个目标各自独立走：

```
tape → lower(isel, abi, enc, reloc) → TargetProgram → 镜像 + 目标机解释执行
```

### 11.2 解释必须是目标感知的

目标解释器持有**该 arch 的具名寄存器**（`rdi rsi rdx r10/rax` 对 `x0..x8`），其系统调用分发器**只**认 lowering 产出的 `(os, sysno)` 或 `(os, winapi_symbol)`，**绝不回看通用 tape op**。后果全是设计意图：

- abi 给 lnx/x86_64 的 write 吐 sysno 4（osx 的号）→ Linux 分发器拒绝 → stdout 不匹配
- osx sysno 丢掉 `0x02000000` class bit → 不匹配
- win 路径用 `rdi` 而非 `rcx` 传 arg0 → 不匹配

**fold 若不是目标感知的，它就不是测试。禁止把通用 tape 跑六遍再和自己比。**

### 11.3 反向对照（必做）

`unisa run examples/hello.c --fold --fault osx_class_bit` 必须打印 **4/6**。
故障注入项：`osx_class_bit`、`win_argregs`、`arm_gate`。这证明 fold 有牙齿，属验收项。

---

## 12. UNS1

头部 16B：`UNS1` | dtype u8 | flags u8 | nTensors u16 | nParams u32 | acc f32
张量：`name[16]` | rows u16 | cols u16 | scale f32 | payload | 补齐到 4

```
dtype: 0=i8  1=f16  2=f32  3=q4  4=q2
i8 : 整张量  scale = maxabs/127        1 值 / 字节
q4 : 按行    scale = maxabs/7          取值 -7..7，    2 值 / 字节
q2 : 按行    scale = maxabs            取值 {-1,0,1}， 4 值 / 字节
```

按行 scale 存为 payload 前的 `f32[rows]` 段；张量头的 `scale` 取该段最大值（忽略行 scale 的读取方仍能拿到合理上界）。亚字节 payload 按**低半字节 / 低 2 位在先**打包，行主序，每行从字节边界起。

Ship 包：`weights/*.unisa` + `MANIFEST.json` + `kernel/unisa_boot.c` + 镜像，i8 下约 50 KB。

`unisa_boot.c` 是部署 kernel：读 UNS1，跑 embed → gemv → ReLU → gemv → argmax。**无 softmax、无 libm、热路径无 malloc。** 整数 dtype 用 `int32` 累加器，scale 每个输出只乘一次——内层无浮点。

### 12.1 量化阶梯 —— 真正的实验

超级拟合换来极大的 logit margin，所以问题不是"q2 损失多少精度"，而是 **"q2 会不会改变任何一个决策"**。dtype 的唯一判据：

> **argmax 不变性**：对完整 gold 的每一个 key，量化网络必须选出与 f32 **完全相同的类下标**。

`unisa quant` 沿 `f32 → f16 → i8 → q4 → q2` 逐级下探，报告每阶段能保持不变的最低 dtype，写入 `MANIFEST.json`：

```
stage    θ      f32     i8     q4     q2    min-margin   ship
parse    1642   6568B   1642B  821B   411B  ...          q?
...
TOTAL           ...
```

每阶段按**各自**最低不变 dtype 出货——混合 dtype 的 kit 是预期结果，也正是重点。某阶段扛不住 q2 是关于这张表结构的**发现**，不是失败。逐阶段记录最小 logit margin，它能预测阶梯落点。

---

## 13. 训练 / 验收

Epoch 循环：combo + isel + abi 按 batch 16；前端网络；每 epoch 评估一次完整 gold。
**停机**：所有网络达 1.000，或到 `--epochs`（默认 90）。v1 的 `combo ≥ 0.985 && 所有 table ≥ 0.85 && epoch > 30` 保留为**最低可用**状态而非停机点；停在该状态报 `UNDERFIT`，被 `unisa ship` 拒绝。

`--holdout`（仅诊断）：`none`；`random15` 扣 15% 行不训练但全部行仍评估；`osx/arm64` 扣掉所有 os=osx ∧ arch=arm64 的行（仅 enc/isel/abi/combo；前端表无 os/arch 轴，回落 `none`）。

**验收**

- `unisa run examples/hello.c --fold` → 6/6，`hello from C99\n`
- fact→120、switch→6、do→3、fib→55、ptr→7、struct→9，全部 6/6
- `host.c` → 每个 OS 组内 2/2
- `unisa run examples/hello.c --fold --fault osx_class_bit` → **4/6**
- `unisa compile` → 三个魔数正确；**同输入跑两次字节相同**
- `unisa dump-weights --dtype i8` → `weights/parse.i8.unisa` 以 `554e5331` 开头
- `unisa acc` → **每阶段 = 1.000**
- `unisa quant` → 每个出货阶段在其记录 dtype 下 argmax 不变
- 干净状态跑两次 `unisa train` → **权重字节相同**
- `unisa ship` → kit.zip 含 weights/ + MANIFEST.json + kernel/unisa_boot.c + 镜像

**构建顺序**：TableNet+gold → tape+VM → lowering → C 走查器 → 封装 → ship。

**acc 卡在 1.000 以下 = gold 的 key 编码错了，不要加宽网络。**

### 13.1 体积与速度预算

头号主张是体积，就把它量出来，摆到现有方案旁边。

| 指标 | 怎么测 | 目标 |
|---|---|---|
| 总 θ | 所有网络求和 | 记录 |
| kit 字节 | `unisa ship` 按逐阶段最低 dtype | 权重 **≤ 32 KB**；kit ≤ 64 KB |
| 对比 tinycc | `size $(which tcc)` 或 tcc 发布二进制 | 记录比值 |
| 决策吞吐 | `unisa bench`，逐阶段，冷（无 memo） | Python ≥ 50k/s；`unisa_boot.c` ≥ 5M/s |
| 端到端编译 | `unisa run examples/fact.c` | ≤ 1 s |

`unisa bench` 必须报**冷**数据。在确定性全函数上加 memo 缓存是正当工程手段，`run` 可开；但 `bench`/`acc`/`quant` 必须关——否则测的是 dict，不是 kernel。

---

## 14. 版本沿革

`archive/prd.v1.md` 为原始规格。v2 消解了会导致实现分叉的歧义，v2.1 依据"确定性 + 超级拟合 + 极小体积 + 对标 tinycc"的实验意图补充约束，v2.2 为精炼版（内容等价）。

**v2 钉死**（按会先咬人的顺序）：LR 是 epoch 衰减表而非参数量分档 · `h0=52` 给出五段分解 · 9 头连同词表枚举 · `OPS` 固定 38 且顺序写死 · "FULL gold" 定义为完整笛卡尔积 · parse 覆盖规则拆行（v1 原文无法无歧义切词）· type 的 `1/19 illegal` 解读为训练下采样 + 全量评估 · enc/reloc/irsel 默认分支补全 · 补齐 connect/bind/listen/accept 的 win 符号 · r0–r7 寄存器映射与 MOPS 的 `none` 约定 · 完整列出 tape 指令集 · `--fold` 明确为逐目标 lowering **且**目标感知解释 · 加入反向对照 · 补 Mach-O/PE 魔数、退出码、`--drive`、`--holdout` 语义、Oracle 分发 · 选定 fib/ptr/struct 预期输出。

**v2.1 新增**：确定性契约（§1.1）· 超级拟合确立为目标、`SHIP_ACC=1.000`（§1.2）· q4/q2 dtype 与按行 scale（§12）· argmax 不变性作为量化唯一判据与逐阶段混合 dtype（§12.1）· 体积/速度预算与 tinycc 对比（§13.1）。

**仍可重选**：§5 bilinear 形式 · `SYMS` 拼写 · fib/ptr/struct 常量 · §12 亚字节打包顺序。除此之外全部承重。
