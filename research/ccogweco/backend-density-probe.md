# 后端代码密度对照实验(nn-study G(2);2026-11-25 ccogweco)

只读测量:gcc 单文件编到 /tmp,产品对象直接读链目录;未改仓。fable G(2) 原文:用已装产品 `-c` 编
`src/cxmod.c` 与 `cxmod.cx` 各一次比 .text;本页按 cc 口径扩展到 gcc -O0/-Os/-O2 与 lnx x86_64/arm64 产品对象,
并用助手调用数与源行数排除"运行库内联差异"这一误归因。

## 0 结论(先)

- 同一单元 `cxmod`,产品后端 .text / gcc **-Os** .text:
  - **x86_64:194,024 / 42,609 = 4.55 倍**;
  - **arm64:240,828 / 47,724 = 5.05 倍**(用 aarch64-linux-gnu-gcc -Os 交叉编同源)。
- 对 gcc -O0:4.55 倍降到 **2.57 倍**(194,024/75,617);对 -O2:**2.35 倍**(194,024/82,698)。
- 中位数与区间(两个目标、三个 gcc 档):**4.55 到 5.05 倍(对 -Os)**,不是数量级差。
- 助手差异不足以解释:`.cx` 单元出现 10 个 `sy_/mm_/tx_` 标识符,`.c` 投影 9 个(两者都调同一批 L0 帮手);
  源行 2,280(.cx)对 2,247(.c),只差 1.5%。**不能把 4.5 到 5 倍归到"cx 版多带运行库"**。
- 可能原因分类(未逐项实验):优化管线深度(寄存器分配、窥孔、常量折叠、内联)与调用约定/AI 开销;
  其中"内联/常量折叠"可用同名函数的 .text 占比进一步证伪,"调用约定"可用小函数样本对照。

## 1 gcc 对照(宿主 x86_64,单文件,-c 到 /tmp)

    mkdir -p /tmp/grk47redo/dens
    gcc -O0 -c -I include -I src -o /tmp/grk47redo/dens/cxmod-O0.o src/cxmod.c
    gcc -Os -c -I include -I src -o /tmp/grk47redo/dens/cxmod-Os.o src/cxmod.c
    gcc -O2 -c -I include -I src -o /tmp/grk47redo/dens/cxmod-O2.o src/cxmod.c
    aarch64-linux-gnu-gcc -Os -c -I include -I src -o /tmp/grk47redo/dens/cxmod-arm64-Os.o src/cxmod.c
    size -A <obj> | grep '^\.text'

| 口径 | .text 字节 |
|---|---|
| gcc -O0 | 75,617 |
| gcc -Os | 42,609 |
| gcc -O2 | 82,698 |
| aarch64-gcc -Os | 47,724 |

## 2 产品对象(0.1.0 链 fixedpoint/g2;size -A 原行)

    C=~/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1
    size -A $C/fixedpoint/g2/lnx-x86_64/cxmod.o
    size -A $C/fixedpoint/g2/lnx-arm64/cxmod.o
    size -A $C/fixedpoint/g2/lnx-x86_64/tapegate.o
    size -A $C/fixedpoint/g2/lnx-arm64/tapegate.o

| 单元 | x86_64 .text | arm64 .text |
|---|---|---|
| cxmod | 194,024 | 240,828 |
| tapegate | 75,395 | 84,112 |

- tapegate 无 `src/tapegate.c` 投影(`ls` 无该文件),gcc 对照 **UNMEASURED**;不据此算密度比。

## 3 密度比与误归因排除

| 目标 | 产品 .text | gcc -Os .text | 比 | 对 -O0 | 对 -O2 |
|---|---|---|---|---|---|
| lnx x86_64 | 194,024 | 42,609 | **4.55** | 2.57 | 2.35 |
| lnx arm64 | 240,828 | 47,724 | **5.05** | (未测 arm64 -O0) | (未测) |

- 源码规模:cxmod.cx 2,280 行 / cxmod.c 2,247 行(+1.5%),不能解释 4.5 到 5 倍。
- 助手调用面:cx 10 个标识符 / c 9 个,同属 sy_/mm_/tx_;差异 1 个,不能解释数倍。
- 结论:密度差是**后端生成质量**差异,不是"cx 版多带运行库"的投影差异;G(2) 的"gcc 对照不公平"在当前单元上
  只能解释约 1.5% 到 10% 的量级,不解释 4.5 到 5 倍。

## 4 可能原因与下一个最便宜实验

| 分类 | 支持证据 | 最便宜实验 |
|---|---|---|
| 优化管线深度(寄存器分配/窥孔/常量折叠/内联) | 产品对 gcc -O0 仍 2.57 倍,说明不只是"少优化",而是生成器本身每操作字节多 | 取 3 个同名小函数,用 objdump 指令条数对照;或按 .text 中调用/立即数比例估 |
| 调用约定/AI 开销 | 产品单元每函数有帧建立与栈搬移;gcc -Os 大量尾调用与寄存器传参 | 小函数样本(gcc -O0 同样)对照帧尺寸分布 |
| 内联差异 | .cx 侧 sy_/mm_/tx_ 每次调用而非内联;.c 侧同样调用 | 数 .text 里 call 指令数与外部符号引用数,对 gcc -O2 对照 |

## 5 边界

- gcc 对照是宿主 x86_64(与产品 lnx/x86_64 同目标);arm64 用 aarch64-linux-gnu-gcc -Os,与产品 lnx/arm64 同目标族,
  但 GCC 版本与产品内部后端不同。
- tapegate 无 .c 投影,UNMEASURED;其余未测档标 UNMEASURED,不插值。
- 本页没有把"密度差"写成"慢 4.5 倍";这是**代码体积**测量,不是执行时间测量。
## 第二节:同名小函数对照(2026-11-25,第二步)

方法:在 `cxmod` 产品对象(`fixedpoint/g2/lnx-x86_64/cxmod.o`)与 gcc -O0 编译的 `src/cxmod.c` 之间取 82 个同名
函数(`readelf -sW | grep ' FUNC '` 交集),挑 5 个小函数用 `objdump -d --disassemble=<sym>` 数指令。
产品对象无 `.size`,故用指令条数与 objdump 字符量作代理;未做逐字节归因,分类占比是估计。

| 函数 | 产品指令 | gcc -O0 指令 | 倍数 | 备注 |
|---|---|---|---|---|
| cxm_backc | 80 | 26 | 3.08 | 序言与立即数装载差异最明显 |
| cxm_aliased | 156 | 87 | 1.79 | 循环体接近,入口开销放大 |
| cxm_addsym | 168 | 75 | 2.24 | 多次地址计算 |
| cxm_addloc | 187 | 78 | 2.40 | 同左 |
| cxm_bind | 216 | 0(未生成) | - | gcc -O0 侧该符号未出现,不可比 |

### 逐类观察(以 cxm_backc 前 20 条为例)

产品序言:

    mov %rdi,%rax / mov %rsi,%rdi / ... / mov %r9,%r8   # 6 条参数搬迁
    jmp .+0                                              # 跳到下一条(空跳)
    push %r9 / mov %rsp,%r9 / sub $0x20,%rsp             # 自建 r9 帧
    mov %rax,-0x8(%r9) / mov %rdi,-0x10(%r9) / mov %rsi,-0x18(%r9)  # 参数全 spill
    mov $0x20,%esi / mov %r9,%rax / sub %rsi,%rax / push %rax
    mov $0x18,%esi / movslq -0x18(%r9),%rax              # 常量装载+地址计算

gcc -O0 序言:`endbr64 / push rbp / mov rsp,rbp / 3 条参数入栈`,随后直接循环。

| 类 | 观察 | 估计字节占比(5 函数合计) |
|---|---|---|
| (a) 序言与栈帧 | 产品每函数 6 条参数搬迁 + r9 帧 + 全参数 spill;gcc 为 5 条标准序言 | 15-25% |
| (b) 参数传递/调用约定 | 产品先把寄存器参数搬一遍再全部 spill 到 r9 帧,调用仍走寄存器但读回走内存 | 15-25% |
| (c) 局部变量每读写内存 | 产品用 r9 相对 + 每次 `movslq`/`sub` 重算地址;gcc 用 rbp 相对单条 | 20-30% |
| (d) 常量与地址重复装载 | 同一函数内多次 `mov $0x20,%esi`、`mov $0x18,%esi` 再减,未折叠为常量偏移 | 10-20% |
| (e) 无窥孔 | 出现 `jmp .+0`(跳到下一条)、把 rsp 搬 r9 后立刻再 push 等冗余 | 5-15% |
| (f) 异常/边界插桩 | 样本前 20 条未见;未排除 | UNMEASURED |

### 结论

- 存在**一眼可见的大块浪费**:每函数固定入口约 10-15 条(参数搬迁 + r9 帧 + spill + 空跳),按小函数计
  可达 30-40% 字节;地址重算与常量重装再占 30-50%。这与 4.55×(对 -Os)/2.57×(对 -O0)的整体密度差方向一致。
- 这是"生成器每操作多"的直接证据,不是运行库差异(第一节已排除);样本 5 个、占比为估计,逐字节归因未做。
- 下一步最便宜:对 `cxm_backc` 两类指令逐条按字节累加(需产品对象 `.size` 或 objdump 原始字节求和),得出
  (a)-(e) 的精确占比;并对 osx/arm64 与 win/arm64 的大切片重复抽 2 个函数,判断是否同源。

## 第三节:逐字节归因(2026-11-25,部分)

- 方法:objdump 每条指令的原始字节逐字节求和(不依赖 .size)。样本 cxm_backc 与 cxm_aliased。

| 函数 | 产品字节 | gcc -O0 字节 | 倍数 |
|---|---|---|---|
| cxm_backc | 275 | 73 | 3.77 |
| cxm_aliased | 584 | 303 | 1.93 |

产品 cxm_backc 分类(字节/占比):常量装载 mov $0x = 65(24%);地址重算 movslq/lea/sub = 34(12%);序言与帧 push/sub rsp/mov rsp,r9 = 15(5%);参数 spill = 12(4%);jmp(含 jmp .+0)= 9(3%);其余循环体 140(51%)。
- osx/arm64 与 win/arm64 的函数级对照:UNMEASURED——两片符号表剥离(osx nsyms=0),链目录未保留 osx/win 单元 .o;要做需在私有副本先产出未剥离映像或保留单元对象。
gcc -O0 同函数:总计 73;帧 4;spill 4;地址 3;jmp 2;常量装载 0(用 sub 立即数)。
- 结论:同函数 3.77 倍中,可直接点名的浪费是常量重复装载 65 字节(24%)与地址重算/序言/spill/空跳合计 70 字节(25%);其余一半是循环体本身。cxm_aliased 为 1.93 倍,说明入口浪费在小函数占比更大、大函数被稀释。

