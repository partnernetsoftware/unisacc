# UNISA SH —— 规格 v3.3

> **配套**：[`prd.tree.md`](prd.tree.md) 树+DAG，开工用 · [`prd.map.md`](prd.map.md) 记忆宫殿，记全局用 · [`research/prior-art.md`](research/prior-art.md) 先行研究，写论文前必读 · `archive/` 历史版本
>
> **结构**：五层 —— **契约 → 模型层 → 经典层 → 验收 → 知识沉淀**，同层内按依赖排序。§6 [E] 是随实现累积的实验发现，只记实测，供论文直接引用。

## 0. 导读

### 0.1 条款编号

每条规范性条款带稳定 ID，供实现、形式化证明、验收脚本三方引用。**ID 一旦分配不再改动**，废弃条款保留编号并标记。

| 前缀 | 域 | 前缀 | 域 |
|---|---|---|---|
| `T` | 命题 | `O` | Oracle 接缝 |
| `D` | 确定性 | `W` | 前端走查 |
| `F` | 超级拟合 | `TP` | Tape 与 VM |
| `P` | 证明义务 | `C` | Catalog |
| `U` | CLI | `L` | Lowering |
| `K` | kernel | `X` | 目标机执行 |
| `N` | TableNet | `I` | 镜像 |
| `S` | StageNet/combo | `A` | 验收 |
| `V` | 词表 | `B` | 预算 |
| `G` | Gold | `E` | **实验发现（§6）** |
| `TR` | 训练 | | |
| `Q` | UNS1/量化 | | |

### 0.2 术语

| 术语 | 定义 |
|---|---|
| **stage（阶段）** | 一个表形状的决策点，各由一个独立小模型作答。共 14 个：pp lex parse type scope irsel tyinfo pfconv（前端）、enc reloc regmap abi（后端），外加只在对照臂里的 `isel` 与 `combo`。清单见 §3.0 |
| **key** | 某阶段的一个输入元组，取自该阶段声明的字段词表 |
| **K_s** | 阶段 s 的 key 全域 = 各字段词表的完整笛卡尔积 |
| **gold** | 阶段 s 上的参考标签函数 `G_s : K_s → Class_s`，全函数 |
| **FULL gold** | 在整个 K_s 上评估，而非在批次或子集上 |
| **Oracle** | 网络与 gold 之间的唯一分发点，见 §4.1 |
| **tape** | 目标无关的通用指令流，§4.3 |
| **TargetProgram** | tape 经 lowering 后得到的、携带目标事实的指令流，§4.5 |
| **fold** | 把同一份 tape 铺到 6 个目标各自执行并比对，§5.1 |
| **kit** | `unisa ship` 产出的交付包 |

### 0.3 交付边界

**交付**：命令行的「编译器 + 训练器 + 推理器」。训练微型表网络 → 把 C99 子集编译到通用 tape → lower 到 6 条 ISA → 各自执行产生完全一致的 stdout → 落盘真实权重二进制与目标文件镜像。

**不交付**：Web 应用、React、演示页。

**语言**：Python 3.11+，**仅标准库**。单一包 `unisa/`。可选后续：极小的 C gemv kernel。Python CLI 通过全部验收之前不碰 Rust。

> **[U-5] 「仅标准库」只约束出货路径。** 出货的是**构造**权重 + 推理 kernel，它们必须零依赖、可复现、能塞进 `unisacc.c`。**SGD 对照组不出货**，所以它想用 numpy / torch / Metal / Rust 调底层都可以——真要跑大规模对照实验时再换，届时按需租算力（本机的 Apple GPU/NPU 也是选项）。当前所有测试、所有镜像、自举全部走构造路线，对照组只由 `tests/baseline.sh` 显式触发。

**终点**：`unisa train && unisa run examples/hello.c --fold` 打印 6/6 match，然后停。

---

# 第一部分 · 契约

## 1. 命题与契约

### 1.1 命题 [T]

Shell = **推理器 + 执行器 + 模型数据**。

- **[T-1]** 走查器 / 符号表 / 重定位算术 / ELF-MachO-PE 头 = **经典代码**（代数），**不得神经化**。
- **[T-2]** 网络只做**最后一公里选表**：离散 key → 1 个类。每一个表形状的阶段**都要**神经化。
- **[T-3]** Gold 表同时是标注、兜底与验证器。
- **[T-4]** 到处只有一个 kernel：`embed → gemv → ReLU → gemv → argmax`。换权重即换能力，kernel 不变。
- **[T-5]** 6 目标，仅 64 位：`lnx|osx|win` × `x86_64|arm64`。同一条 tape，六份镜像，stdout / exit 必须一致。

**这个实验在证明什么**：一个决策层由确定性神经策略驱动的 C99 编译器（对标 tinycc / `tcc -run`），支持 6 个目标，体积小到离谱。不是"AI 写代码"，恰恰相反——编译器的决策表是**构造/学出来的产物**：可热插拔、形式统一、可穷举验证。**主张是接口统一性，不是体积**（[R-2]；体积主张已被 E-3′ 与 Boniol et al. NFM'26 打掉）。让论点成立的交付物是 §5.3 那张表——旁边摆 `tcc`、裸表、BDD 三条基线，并诚实报告我们输在哪里。

### 1.2 确定性契约 [D]

不可字节复现的编译器不算编译器。这一节压过所有 ML 习惯。

| ID | 条款 |
|---|---|
| **D-1** | 无 dropout / 无权重噪声 / 无 label smoothing / 无数据增广 / **推理期无 RNG** |
| **D-2** | `argmax` 平局取**最小类下标** |
| **D-3** | 训练期随机性仅存在于初始化与 batch 置换，且只来自 mulberry32；置换种子 `seed ^ epoch` |
| **D-4** | `gemv` 累加顺序固定（下标升序，禁 pairwise / 禁多线程重排），跨构建位稳定 |
| **D-5** | 同源码 + 同权重 → 镜像**逐字节相同**；任何头部不得含时间戳、路径、build ID |
| **D-6** | 相同种子 → 权重**逐字节相同**，跨运行、跨机器 |
| **D-7** | 网络与 gold 对同一 key 的决定必须完全相同；不一致 = 其中之一有 bug，不是"模型方差" |

### 1.3 超级拟合 [F]

每个阶段都是**有限离散全函数**，`K_s` **就是**定义域，不存在要泛化过去的留出分布。**记忆即规格。**

| ID | 条款 |
|---|---|
| **F-1** | `NET_READY = 0.85` —— 运行时接管门槛，保证训练中途编译器可用 |
| **F-2** | `NET_HOT = 0.995` —— 训练热跳过门槛 |
| **F-3** | `SHIP_ACC = 1.000` —— 出货门槛；`unisa ship` 与验收拒绝低于此值 |
| **F-4** | `--holdout` 仅作诊断（网络是学到结构还是纯记忆？），出货运行**禁止**开启；holdout 精度作实验发现记录，门禁一律以 FULL gold 为准 |
| **F-5** | 某阶段到不了 1.000 = **gold 的 key 编码错了**。去修编码。只有编码被证明正确后才允许加宽，且必须记入 `MANIFEST.json` |

### 1.4 证明义务与接口约束 [P]

「C99 可用模型等价替代」不是一个定理，是**五层义务**，难度差几个量级。分层列明，避免把可判定的当成难题、把难题当成已解决。

#### 交给实现的硬约束

| ID | 条款 |
|---|---|
| **P-1** | **Oracle 不透明性**：走查器与 lowering 对任何阶段的依赖，**只经由 `ask` 返回的类名**。禁止读取 logits / 概率 / margin / 任何网络内部状态来改变编译输出。不得做 ensemble，不得"低置信度回退 gold" |
| **P-1a** | Oracle 的 net/gold 用量统计仅用于报告，**不得反馈进任何编译决策** |
| **P-1b** | `NET_READY` 门禁在**权重加载时一次性解析**，不是逐决策解析；给定一份权重快照，编译器是一个固定函数 |
| **P-2** | **key 全域性**：`ask` 收到的 key 必须 ∈ `K_s`。实现上**无条件断言**；「该断言不可达」是交给形式化的义务 |

#### 证明义务分层

| ID | 命题 | 性质 | 归属 |
|---|---|---|---|
| **P-3** | 逐阶段等价：`∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` | `K_s` 有限、闭合、极小（6–960 行），**穷举即完全判定过程** | 已由 `acc = 1.000` 完整证明，无剩余义务 |
| **P-4** | 量化等价：量化网络 ≡ f32 网络 | 同上，在同一有限域上穷举 `argmax` 不变性 | 由 `unisa quant` 证明。logit margin 只用来**预测**阶梯落点，不承担证明责任 |
| **P-5** | 组合等价：net 驱动的编译器 ≡ gold 驱动的编译器 | 由 P-3 经同余性直接得到，**不需要对走查器做归纳** | **前提是 P-1**。一旦读了 logits，P-5 当场失效 |
| **P-6** | 语义保持：gold 驱动的编译器 ≡ C99 语义 | CompCert 级别工作量 | **与神经化正交，本实验不声称**。它是经典编译器本来就有的负担 |
| **P-7** | 目标等价：`∀t : exec_t(lower_t(tape)) ≃ vm(tape)`（可观测行为上的模拟关系） | 每目标一个模拟关系 | 真正需要干活的部分，见 §5.1 |

#### P-8　存在性定理（构造式，已验证）

> 设 `f : K → C` 是有限积 `K = V₁×…×V_m` 上的全函数。则**存在**权重，使 kernel
> `embed → gemv → ReLU → gemv → argmax` 在 K 的**每一点**上精确计算 f。

构造：`embed` 取 one-hot，故 `x ∈ {0,1}^Σ|Vᵢ|` 恰有 m 个 1；每个 key `k` 配一个隐单元
`u_k`，令 `W1[coord(i,kᵢ), u_k] = 1`、`b1[u_k] = −(m − 0.5)`，则 ReLU 仅在 key 完全匹配时
激活（值 0.5），否则恰为 0；令 `W2[u_k, f(k)] = 1`。任意输入恰好一个隐单元激活，logit
在 `f(k)` 位取 0.5、其余为 0，argmax 必然正确。∎　上界 `h = |K|`。

**推论**：「能不能达到 100%」不是开放问题。开放的是**最小性**：

```
h_min(f)（未知）  ≤  h_训练(f)（已知实例，精确）  <  h_构造(f)（已知实例，精确）
```

尚待形式化的三条：

| ID | 命题 | 备注 |
|---|---|---|
| **P-8a** | `h_min(f)` 的组合刻画 | **小表已精确关闭**（E-26：reloc/enc/pp/scope/lex 的 `h_min` 上下界吻合；`h_min(parse) ≥ 6`）。大表、三字段表、多头表仍开放 |
| **P-8b** | **可达性分离**：存在 f 使精确解集非空，但从随机初始化出发的 SGD 在任意预算内不可达 | 已有实例（E-18 的 s2：12.8× 参数仍 0.8143）**且已有机理**（E-25 的活板门：`W1` 方向上小步只会杀死单元，改规则需 `(W1,b1)` 同时离散跳变）。论文里最硬的一块 |
| **P-8c** | 是否存在确定性算法在 `O(h_min)` 内构造 | 先验注意：**最小 DNF 是 NP-hard**，目标应为「好启发式 + 精确性作为不变量」而非最优 |

> **给形式化的提示**：P-3 / P-4 不需要任何逼近论证——不需要 PAC bound、Lipschitz 常数、鲁棒性半径。超级拟合的意义正是把 ML 泛化问题变成了**模型检验问题**。唯一穷举帮不上忙、需要对走查器做真推理的地方是 **P-2**，那也是唯一真正藏 bug 的地方。

---

## 2. CLI [U]

```
unisa train [--epochs N] [--holdout none|random15|osx/arch] [--out weights/]
unisa acc
unisa compile in.c -o out --target lnx/x86_64
unisa run in.c [--target T] [--fold] [--drive gold|spec|combo] [--fault NAME]
unisa tape in.c
unisa lower OP --target T
unisa ship [--dtype q2|q4|i8|f16|f32] [--out kit.zip]
unisa dump-weights [--dtype q2|q4|i8|f16|f32] --out weights/
unisa quant [--stage S]        # 量化阶梯报告 §3.7
unisa bench [--n N]            # 逐阶段决策吞吐 §5.3
unisa build-weights [--out weights/built.json] [--pack weights/built.uns2]
                               # 构造精确整数权重，无训练无随机 [K-5] [E-22]
unisa emit-kernel [--out kernel/]
                               # 把决策层发成 C：模型 blob + 整数 kernel [E-28]
unisa vm FILE.tape [args...]   # 直接运行一条 tape —— 基准机器 [TP-4]
unisa compile ... --from-tape  # 输入是 .tape 而非 C（只做 lowering + 汇编）
```

**出货的编译器 `unisacc`（`unisacc.com`）的契约** [S-11] [S-15 C1]——上面是 Python 驱动，这才是用户拿到的东西。每个 flag 与 `tests/cli.sh` 里的一条用例对应；行为对标 cc/tcc，凡有出入处写明：

```
unisacc [flags] FILE.c [FILE.c ...] [-- args...]
  -run            编译后直接在内存里运行，不落盘；后面的 .c 是输入，第一个非 .c 起是程序的 argv，`--` 显式分界
  -b os/arch      写该目标的可执行镜像（六个目标任选，与宿主无关）
  -S              写 tape——本编译器汇编层的 IR，与 gcc -S 同级；-c 是它的同义词（没有目标文件）
  -E              只预处理，输出文本
  -o FILE         输出位置；`-o -` 为 stdout
  -               作为输入文件名时表示 stdin
  -I DIR  -D N[=v]  -U N     与 cc 同义；-U 在全部预定义之后生效，能撤销 __linux__ 之类
  -include FILE   如同源文件第一行是 #include "FILE"；报错行号仍是用户文件的
  -nostdinc       不查内建的头文件副本（只查源文件目录与 -I）
  -l* -L* -x c    接受并忽略：库在头文件里，没有链接器，只有一种语言
  -W* -w -g -O* -f* -std=* -pipe -m*   接受并忽略：一种方言、一档优化、无独立调试信息
  -v              编译后打印每个阶段被问了多少次（仪表，不是 verbose）
  -version / --version   版本串
  --check-oracle  枚举全部问题，核对缓存与网络逐个一致 [A-49]
FILE.tape         输入是 tape 时直接进后端
```

与 cc 明确不同之处：`-c` 不产目标文件；`-O` 不做任何优化；`-g` 不产调试信息；找不到的 `#include` 是错误（C99 6.10.2p4），以前是静默跳过。

### 2.1 测试套件

`tests/all.sh` 是唯一入口，按**退出码**判定每一套（不靠匹配末行）：

| 套件 | 查什么 | 条款 |
|---|---|---|
| `acceptance.sh` | 规格自己的验收清单 | [A-*] |
| `vm.sh` | tape 解释器，手写夹具（不经前端） | [TP-4] |
| `difftest.sh` | 与系统 `cc` 对拍 —— **唯一能看见 gold 缺陷的仪器** | [P-6] [A-17] |
| `native.sh` | 发出的镜像在本机**真实执行** | [A-18] |
| `artifacts.sh` | 出货产物**可用**而非仅"格式正确"：UNS2 往返、六目标镜像被平台工具识别、两次 ship 字节相同、体积预算 | [A-24] |
| `ccrun.sh` | unisacc 编译 C，基准 VM 运行，**逐个探针与 Python 前端对答案**；`wrong` 必须为 0，一致数只能升，已知分歧记在 `ccrun.knownwrong` | [A-21] |
| `selfhost.sh` | 两种构建的 unisacc 与 Python 前端逐 token 一致 | [A-20] |
| `bootstrap.sh` | `B = C = U` 自举不动点 | [A-23] |
| `baseline.sh` | **SGD 对照组**，手动触发：E-18 / E-31 / E-37 要的那几个数 | U-5 |
| `corpus.sh` | **c-testsuite 的 220 个程序** —— 别人写的、为别的编译器写的 | [A-25] |
| `crossnative.sh` | **非本机目标**真实执行：两个 Linux 目标进本地虚机，osx/x86_64 走 Rosetta 2，win/arm64 与 win/x86_64 进 UTM 的 Windows 11 真机（虚机没开就**跳过并点名**，`STRICT=1` 时跳过即失败） | [A-27] |
| `fat.sh` | **一个文件两条 ISA**，两个 slice 都真跑（arm64 原生 + x86_64 经 Rosetta） | [A-28] |
| `closure.sh` | `unisacc -b` 写出的镜像与 Python 后端**逐字节相同**，六个目标；宿主目标的镜像真跑 | [A-34] |
| `nativeboot.sh` | N1 = N2 = N3，全程无 Python | [A-35] |
| `bigclosure.sh` | 同 closure，但对象是**编译器自己**（707 KB、1,874 个数据符号） | [A-42] |
| `layout.sh` | 数据布局**枚举**：762 条 tape × 6 目标 = 4,572 次逐字节比对 | [A-41] [P-3] |
| `datashape.sh` | 生成声明序 ≠ 地址序的病态程序，比对字节前先断言形状确实出现 | [A-41] |
| `consts_check.py` | `lower.py` 的常量链与 `unisacc_back.c` 的字面量必须相等 | [A-43] |
| `ablate.sh` | 把某阶段的答案旋成错的：必须有镜像变了或编译被拒 | [A-36] |
| `run.sh` | `unisacc -run` 在内存里编译并运行 | [A-37] |
| `cli.sh` | 空目录里把编译器当工具用：自带头文件、`-I`/`-D`/`-o`/`-E`/`--version`、shebang、argv | [A-39] |
| `diag.sh` | 报错要给用户的文件、行、列与源码行 | [A-40] |
| `ape.sh` | `unisacc.com` 一个文件、每个目标 | [A-38] |
| `multi.sh` | 多翻译单元一个程序，对 `cc a.c b.c` | [A-30] |
| `tools.sh` | 别人的库代码，多文件，自带已知答案测试 | [A-31] |
| `selfgap.sh` | 自举差距本身：unisacc 拒绝而 Python 前端接受的程序数，必须为 0 | [A-29] |
| `stages.sh` | Python 前端问过的每个阶段，C 前端也问过 | [A-33] |
| `linux.sh` | 整套跑进本地 Linux 虚机；客机架构与宿主不同则自动放大看门狗 | [A-32] |
| `release.sh` | 发布前的门：树已提交、版本串在、`STRICT=1` 全绿、没有套件被跳过 | [S-14] |

（这张表以前只列 12 套，落后了十几套。唯一权威是 `tests/all.sh` 的 `run` 行。）

| ID | 条款 |
|---|---|
| **U-1** | `unisa run file.c --fold` 是产品本体；权重缺失时首次运行自动训练 |
| **U-2** | `--drive`（默认 `built`）：`gold` 强制全部走表；`spec` = isel∘abi 两个 StageNet；`combo` = 单个 UnisaNet 供 12 头；**`built` = 构造出的整数网络**（K-5，精确性由构造保证，无 readiness 门禁）。前端表不受 spec/combo 影响，但 `built` 覆盖全部阶段 |
| **U-3** | `--fault`：`--fold` 的反向对照（§5.1），正常运行不出现 |
| **U-4** | 退出码：`0` 成功 / `1` 编译错误 / `2` fold 未达 6/6 |

---

# 第二部分 · 模型层

## 3. 模型层

### 3.0 决策点清单 —— 工作流 × 模型结构

**每个决策点一个独立的小模型**，不按编译阶段合并。合并已实测为有害（见 [E-18]）：
`parse+type+scope+irsel` 合成一段后无论加宽到多少都到不了 1.000，因为段内各表的
key 空间互不相干，共享 trunk 没有结构可共享、只有互相挤占。分立还带来**变更局部性**
——改一张 gold 只需重训那一张（约 0.3 秒）。

全部 14 个模型共用同一个 kernel（[K-1]），只有形状和权重不同。

| 阶段 | 工作流位置 | key 空间 | 行数 | 头(类数) | 单元 |
|---|---|---|---|---|---|
| `pp` | 前端 · 预处理 | `dir(9)×defined(2)` | 18 | `y(4)` | 6 |
| `lex` | 前端 · 词法 | `c(12)×peek(12)` | 144 | `y(10)` | 11 |
| `parse` | 前端 · 语法 | `nt(5)×tok(68)` | 340 | `y(36)` | 34 |
| `type` | 前端 · 类型 | `t1(15)×op(19)×t2(15)` | 4275 | `y(16)` | 61 |
| `scope` | 前端 · 作用域 | `ctx(6)×kind(5)` | 30 | `y(7)` | 9 |
| `irsel` | 前端 · IR 选择 | `family(6)×flavor(66)` | 396 | `y(66)` | 71 |
| `enc` | 后端 · 编码形式 | `op(70)×os(3)×arch(2)` | 420 | `y(5)` | 5 |
| `reloc` | 后端 · 重定位 | `kind(3)×arch(2)` | 6 | `y(3)` | 3 |
| `regmap` | 后端 · 寄存器映射 | `treg(8)×arch(2)` | 16 | `y(16)` | 10 |
| `tyinfo` | 前端 · 类型事实 | `t(16)` | 16 | `size(4),uns(2),narrow(2)` | 6 |
| `pfconv` | 前端 · printf 转换 | `conv(9)` | 9 | `y(7)` | 7 |
| `isel` | 仅 spec/combo 臂 | `op(70)×arch(2)` | 140 | `form(5),symbol(91)` | 70 |
| `abi` | 后端 · 调用约定 | `op(70)×os(3)×arch(2)` | 420 | `sysno(56),arg0(18),arg1(18),arg2(18),arg3(18),arg4(18),ar…` | 37 |
| `combo` | 后端 · 合一（替代 isel+abi） | `op(70)×os(3)×arch(2)` | 420 | `form(5),symbol(91),sysno(56),arg0(18),arg1(18),arg2(18),a…` | 83 |
| **合计** | | | **6650** | | **413** |

> 本表**由 `unisa/gold.py` 与发布的构造权重生成**（`python3 -m unisa acc` 是同一批
> 数字）。它漂过三次，所以这里只放构造路径的行数与单元数；训练 θ 属于对照臂，见
> [E-1]，不在发布路径上。`isel` 仍是一个阶段，但 **lowering 不再询问它**：它的
> `form` 由 os 感知的 `enc` 取代，`symbol` 没有任何编码器读（`tests/ablate.sh`）。

**装配方式**（`--drive`，[U-2]）：

| 模式 | 使用的模型 |
|---|---|
| `built`（默认，发布路径） | 全部 14 个的**构造整数权重** |
| `spec` | 训练臂：pp lex parse type scope irsel enc reloc **isel abi**（历史口径） |
| `combo` | 训练臂：前端各表 + **combo** 替代 isel+abi |
| `gold` | 不用模型，全部查表（见 [O-1]） |

`combo` 是 `isel`+`abi` 的**替代**而非追加：同一 key 空间 `op×os×arch` 上的头合一。
两条路径在完整 gold 上逐 key 同类（[D-7]），所以 `--drive` 不改变编译结果，只改变由谁作答。

**前端 8 个** 把源码走到通用 tape，**后端 4 个** 把 tape 铺到 6 个目标（`isel` 与 `combo` 只在对照臂里）。
形状细则见 [N]（TableNet）与 [S]（StageNet / combo）；每个 key 空间的标签函数见 [G]。

### 3.1 唯一 kernel [K]

| ID | 条款 |
|---|---|
| **K-1** | `embed → gemv → ReLU → gemv → argmax`，仅此一条路径 |
| **K-2** | 部署期**无 softmax、无 libm、热路径无 malloc** |
| **K-3** | 整数 dtype 用 `int32` 累加器，scale 每个输出只乘一次——**内层循环无浮点** |
| **K-5** | **整数构造后端（E-21 / E-22）**：`W1 ∈ {0,1}`、`b1 ∈ {0,−1,−2}` 且**无需存储**（由字面量数恢复）、`W2 ∈ {1,2,4,8,16}`。输入是恰含 m 个 1 的 one-hot ⇒ 第一层是 **m 次加法 + bias**，不是矩阵乘；隐激活恒为 0 或 1 ⇒ 第二层是**条件整数加**，连移位都不需要。全链路**零乘法、零移位、零浮点**。最小覆盖下 max\|pre\| = 2、max logit = 19 ⇒ **int8 足够** |
| **K-4** | Python 参考实现 `unisa/linalg.py`，C 部署实现 `kernel/unisa_boot.c`，两者在 FULL gold 上必须逐 key 同类 |

### 3.2 TableNet [N]

```
embed keys (d=8) → gemv W1[h0,16]+b1 → ReLU → gemv W2[16,nout]+b2 → argmax
h0 = d * (key 字段数)
train: softmax CE, Adam β1=0.9 β2=0.999 eps=1e-8
init:  E N(0,0.08); W1 N(0,sqrt(2/h0)); b=0; W2 N(0,sqrt(2/h))
rng:   mulberry32
```

| ID | 条款 |
|---|---|
| **N-1** | 默认 `d=8 h=16`；lex/pp 用 `d=6 h=12`；reloc 用 `d=6 h=8` |
| **N-2** | 种子：parse13 type17 scope19 pp23 enc29 lex31 reloc37 irsel11 |
| **N-3** | `fit` 热跳过：acc≥`NET_HOT` 且 `step%6≠0` 时跳过 |
| **N-4** | **永远评估 FULL gold，绝不用 batch acc 判 ready** |

**mulberry32**（逐位精确，每步 32 位截断）：

```
s = (s + 0x6D2B79F5) & M32 ; t = s
t = imul32(t ^ (t >>> 15), t | 1)
t = t ^ ((t + imul32(t ^ (t >>> 7), t | 61)) & M32)
u = (t ^ (t >>> 14)) & M32  ;  return u / 2^32
```

**N-5** 高斯 = 相邻两个均匀数做 Box-Muller。训练期可用 libm，**部署期不可**。

### 3.3 StageNet / combo [S]

| ID | 网络 | 规格 |
|---|---|---|
| **S-1** | isel | emb op12+arch8，h1=24 h2=16，heads form/symbol（gate 已移入 abi），seed 3 |
| **S-2** | abi | emb op12+os8+arch8，h1=32 h2=20，heads sysno/arg0-5/ret/gate/nrreg，bilinear 8d，seed 5 |
| **S-3** | combo | ~10.5kθ，seed 7，`E_op[38,16] E_os[3,8] E_arch[2,8]` |

**S-4** combo 的 `h0=52` 分解：

```
h0 = E_op(16) ++ E_os(8) ++ E_arch(8) ++ factor(12) ++ bilinear(8)      = 52
factor   = ReLU(W_op·E_op + W_os·E_os + W_arch·E_arch + b)              → 12d
bilinear = (P_op·E_op) ⊙ (P_os·E_os + P_arch·E_arch)                    → 8d
→ W1 52×48 → ReLU → W2 48×32 → ReLU → 9 heads
```

**S-5** 4 个寄存器头（arg0,arg1,arg2,ret）共享一个 `W_reg[32,|REGS|]`，各自独立 bias。

**S-6 9 个头**

| head | 来源 | 词表 |
|---|---|---|
| form | isel | FORMS |
| symbol | isel | SYMS |
| gate | isel | GATES |
| sysno | abi | SYSNOS |
| arg0 arg1 arg2 ret | abi | REGS（共享 W_reg） |
| tls | abi | TLS |

### 3.4 词表 [V]

**V-1** `OPS`（N=38），isel / abi / enc / combo 共用此 op 轴，**顺序写死，下标即 embedding 行号**：

```
SYSOPS(19) = exit read write open close mmap munmap mprotect getpid
             clock_gettime nanosleep futex socket connect bind listen
             accept clone execve
MOPS(19)   = add64 sub64 xor64 mul64 slt64 sle64 load64 store64 jump
             jumpz call ret nop cas64 fence syscall_gate tls_base
             cycle_counter stack_enter
OPS = SYSOPS ++ MOPS
```

**V-2** 输出词表：

```
FORMS(5)  = syscall svc winapi x86 arm
GATES(5)  = syscall svc0 svc80 winapi none
REGS(18)  = rdi rsi rdx r10 rcx r8 r9 rax x0 x1 x2 x3 x4 x5 x6 x7 x8 none
TLS(4)    = fsbase tpidr_el0 teb none
SYMS, SYSNOS = §4.4 派生函数实际吐出值的有序并集 + `none`
```

### 3.5 Gold [G]

**G-0** 每阶段声明 `[(field, vocab), ...]` 与标签函数。**gold 语料 = 完整笛卡尔积 `K_s`**——枚举代价极低，并强制网络学成全函数。这是 P-3 可判定的前提。

**G-1 parse** — NT(5) × TOKS(54) → PRODS(32)，**270 行**

```
默认: top=global  stmt=expr  unary=prim  postfix=done  after_name=var_def
覆盖: top/eof=end | top/typedef=typedef | top/struct=struct | top/enum=enum
      after_name/( = fn_sig
      stmt/type = stmt/struct = stmt/enum = stmt/typedef = decl | stmt/{ = block
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

**G-2 type** — TYS(15) × TOPS(19) × TYS(15) → TYS|illegal，**4275 行**

```
TYS  = void i8 i16 i32 i64 u8 u16 u32 u64 ptr arr struct fn
                                （i16 = short，E-31；u* = unsigned，E-37）
TOPS = + - * / % < == = & [] . call sizeof , un* | ^ << >>   （un* 一元解引用，忽略 t2）
默认 illegal
数值算术 (+ - * / %) 与位运算 (| ^ << >>) → 任一为 i8|i16 则 i32，否则 i64
< 与 == → i64        , → 右操作数        = → lhs        sizeof → i64
ptr|arr ± num → ptr        ptr - ptr → i64        [] → i64
ptr un* → i64        fn call → i64        i64 & i64 → ptr        struct . → i64
TY_SIZE: void=1 i8=1 i16=2 i32=4 u8=1 u16=2 u32=4 其余=8
无符号（C99 6.3.1.8 的"惯常算术转换"）：
  先做整型提升（秩 < int 的一律变 int，含 u8/u16）
  两侧都无符号 → 无符号；一侧无符号且秩不低于另一侧 → 无符号；否则有符号
  无符号结果的**宽度按 C 取**（u32 或 u64），因为无符号运算必须在自己的宽度上回绕
  有符号的那些行与加入 unsigned 之前**逐行相同**
```

`i8`/`i16` 都当作窄于 int 的类型，参与整型提升；这是 C99 6.3.1.1 的投影，不是近似。

**G-2a** `illegal` 占定义域的大半（原 960 行版本为 611/960）。v1 要求"训练时按 1/19 下采样 illegal 行"——**实测证伪，已废止**：降权后 type 恒定卡在 0.999，唯一丢失的行是 `(i64,&,i32)→illegal`，正是那个稀疏正例 `(i64,&,i64)→ptr` 的近邻，且全表最紧的 margin 全部落在 `&` 上。类别平衡保护的是泛化，而这里没有泛化可保护、只有记忆；饿死 64% 的定义域，只会让网络在稀疏正例旁边变瞎。**权重取 1.0，全部行等权训练、等权评估。** 代码中保留 `TYPE_ILLEGAL_WEIGHT` 以复现 v1 行为。

**G-3 scope** — CTX(6) × KIND(5) → ACTS(7)，**30 行**

```
ACTS = bind_global bind_param bind_local lookup type_name fn_name field
默认 lookup
top/type_kw = top/typedef_id = type_name | top/id = bind_global | top/lparen = fn_name
param/id = bind_param | local/id = bind_local
local/type_kw = local/typedef_id = type_name
sizeof/type_kw = sizeof/typedef_id = type_name
field/id = field
```

**G-4 pp** — DIRS(9) × {0,1} → take|skip|pop|macro，**18 行**

```
默认 skip
ifdef: 1=take 0=skip   ifndef: 取反   if/elif/else: 跟随 flag
endif=pop   define=undef=include=macro
```

**G-5 lex** — CHARC(11) × peek CHARC(11) → ACT(10)，**121 行**

```
CHARC = ws nl A d q sq slash star punct eof other
ACT   = skip nl ident num str charlit cmt linecmt op bad
ws→skip  nl→nl  A→ident  d→num  q→str  sq→charlit
slash|star|punct→op  eof→skip  other→bad
slash×slash = linecmt   slash×star = cmt
```

**G-6 enc** — OPS(38) × os(3) × arch(2) → FORMS，**228 行**

```
win 且 op ∉ MOPS                  → winapi
arm64 且 op ∉ MOPS                → svc
op ∈ SYSOPS 且 (x86_64, lnx|osx)  → syscall
否则                              → 按 arch（x86_64→x86，arm64→arm）
```

**G-7 reloc** — jmpkind(3) × arch(2)，**6 行**

```
x86_64 → rel32 ; arm64 且 jz → arm19 ; 否则 → arm26
```

**G-8 irsel** — family(5) × flavor(27) → recipe|bad，**135 行**（27 有效）

```
alu : add sub mul lt le gt ge eq ne neg → add64 sub64 mul64 slt64 sle64 slt64 sle64 eq ne sub64
mem : load store lea ld st zero         → load64 store64 lea ld st zero
ctrl: jump jumpz ret                    → jump jumpz ret
call: call push arg frame               → call callpush arg frame
lit : imm print write exit              → imm print write exit
默认 bad
```

`gt`/`ge` 映射到 `slt64`/`sle64`——由走查器交换操作数。这种不对称正是最后一公里的表该承载的事实。

**G-9 isel / abi / combo 的 gold 由 §4.4 的 `(op, os, arch)` 函数派生，禁止手工标注。**

### 3.6 训练 [TR]

| ID | 条款 |
|---|---|
| **TR-1** | Epoch 循环：combo + isel + abi 按 batch 16；前端网络；每 epoch 评估一次 FULL gold |
| **TR-2** | **LR 是 epoch 衰减表，不是按参数量分档**：`epoch<18 → 0.032；<50 → 0.014；<90 → 0.006；else 0.0025`。所有网络共用 |
| **TR-3** | **停机**：所有网络达 1.000，或到 `--epochs`（**默认 200**，E-14 实测 90 不够） |
| **TR-3a** | **1.000 必须是吸收态**：某网络首次达到 `SHIP_ACC` 的那个 epoch 立即**快照并冻结**，此后不再训练它。实测（E-14）99 次运行中 30 次先命中 1.000 又掉下来，含三个出货种子——`[N-3]` 热跳过会带着陈旧的 Adam 动量恢复满 LR 步，把边缘 key 踢掉。**`[F-3]` 判定的是快照，不是最后一个 epoch 的状态** |
| **TR-4** | v1 的 `combo ≥ 0.985 && 所有 table ≥ 0.85 && epoch > 30` 保留为**最低可用**状态而非停机点；停在该状态报 `UNDERFIT`，被 `unisa ship` 拒绝 |
| **TR-5** | `--holdout`：`none`；`random15` 扣 15% 行不训练但全部行仍评估；`osx/arm64` 扣掉所有 os=osx ∧ arch=arm64 的行（仅 enc/isel/abi/combo；前端表无 os/arch 轴，回落 `none`） |

### 3.7 UNS1 与量化 [Q]

**Q-1** 头部 16B：`UNS1` | dtype u8 | flags u8 | nTensors u16 | nParams u32 | acc f32
**Q-2** 张量：`name[16]` | rows u16 | cols u16 | scale f32 | payload | 补齐到 4

**Q-3** dtype：

```
dtype: 0=i8  1=f16  2=f32  3=q4  4=q2
i8 : 整张量  scale = maxabs/127        1 值 / 字节
q4 : 按行    scale = maxabs/7          取值 -7..7，    2 值 / 字节
q2 : 按行    scale = maxabs            取值 {-1,0,1}， 4 值 / 字节
```

**Q-4** 按行 scale 存为 payload 前的 `f32[rows]` 段；张量头的 `scale` 取该段最大值（忽略行 scale 的读取方仍能拿到合理上界）。
**Q-5** 亚字节 payload 按**低半字节 / 低 2 位在先**打包，行主序，每行从字节边界起。

#### 量化阶梯 —— 真正的实验

超级拟合换来极大的 logit margin，所以问题不是"q2 损失多少精度"，而是 **"q2 会不会改变任何一个决策"**。

**Q-6 唯一判据 —— argmax 不变性**：对 `K_s` 的每一个 key，量化网络必须选出与 f32 **完全相同的类下标**（平局按 D-2）。

**Q-7** `unisa quant` 沿 `f32 → f16 → i8 → q4 → q2` 逐级下探，报告每阶段能保持不变的最低 dtype，写入 `MANIFEST.json`：

```
stage    θ      f32     i8     q4     q2    min-margin   ship
parse    1642   6568B   1642B  821B   411B  ...          q?
...
TOTAL           ...
```

**Q-8 [修正]** 每阶段按**各自体积最小的不变 dtype** 出货，**不是**按"最低 dtype"。二者不等价：
按行 scale 的 `f32[rows]` 块（Q-4）在小张量上比半字节打包省下的还多，实测 `pp` 的 q4 是
452 B 而 i8 只要 436 B，`reloc` 是 360 B vs 324 B。**以实测字节为准，不以 dtype 序为准。**
混合 dtype 的 kit 是预期结果；某阶段扛不住更低 dtype 是关于这张表结构的**发现**，不是失败。
**Q-9 [修正]** 逐阶段记录最小 logit margin。**实测它并不能预测阶梯落点**（见 E-6′）：
更好的预测量是**头数**与**参数量**——真正压垮 q4 的是逐行误差在多少个独立决策上复利，
而不是最坏的那一个有多紧。margin 仅作弱排序提示，且不承担任何证明责任（P-4）。

**Q-10** kit = `weights/*.unisa` + `MANIFEST.json` + `kernel/unisa_boot.c` + 镜像。

---

# 第三部分 · 经典层

## 4. 经典层

### 4.1 Oracle —— 唯一接缝 [O]

```
oracle.ask(stage, key_tuple) -> class_name
```

| ID | 条款 |
|---|---|
| **O-1** | 网络存在**且** `net.acc >= NET_READY`（在 FULL gold 上测得）→ 网络 argmax；否则 → 查 gold 表 |
| **O-2** | 逐阶段统计 net / gold 用量，`unisa run` 末尾打印 `nets: k/14 driven` |
| **O-3** | 无条件断言 `key ∈ K_s`（P-2） |
| **O-4** | 遵守 P-1 / P-1a / P-1b：只回传类名，不泄露任何内部状态 |

这让"gold 是验证器"成为**运行时事实**：编译器在训练曲线的任何一点都正确，网络渐进接管。

### 4.2 前端走查 [W]

```
src → pp → lex → parse → type → scope → irsel → tape
```

| ID | 阶段 | key | 输出 |
|---|---|---|---|
| **W-1** | pp | dir × defined | take\|skip\|pop\|macro |
| **W-2** | lex | charclass × peekclass | act |
| **W-3** | parse | NT × TOK | production |
| **W-4** | type | t1 × op × t2 | ty |
| **W-5** | scope | ctx × kind | action |
| **W-6** | irsel | family × flavor | recipe |

**W-7** 走查器本身是经典递归下降代码（T-1），只有选表那一下问 Oracle。
**W-8** C99 子集覆盖：`#if` 家族、函数 / 指针 / 数组 / struct / typedef、if/for/while/do/switch、算术、`printf` → `.print`/`.write`。
**W-9** `printf` 在走查期按**静态格式串**脱糖（`%d %s %c %u %%`），这是快路径也是常见路径。**格式串不是字面量就根本脱不了糖**，此时放行到 `<stdio.h>` 里真正的变参 `printf`（它和 `fprintf`/`sprintf`/`snprintf` 共用同一个运行期格式化器 `_u_vfmt`）。一个坑：调用在**走查末尾**才解析，所以调用可以先于定义——对普通函数无害，但这几个函数的**调用约定不同**（参数全压 tape 栈，[W-13]），后到的定义救不回已经按另一种约定发出去的调用，所以它们的变参性写死在 `VARIADIC_LIBC` 里。`%d` 经发射的 `__itoa` 助手（纯 tape op，故可原生编码），`%s` 经 `__strlen`。
**W-10** C 字符串字面量**必须 NUL 结尾**。`write_literal` 传显式长度，故字面量块不会输出该字节；但 `%s` 走 `__strlen`，无结尾符会一路扫进相邻字面量。
**W-12** 相邻字符串字面量按 C 语义**拼接**（`"a" "b"` == `"ab"`）；扫描器逐个字面量出 token，在 token 层折叠。
**W-11** `static` 局部变量取**静态存储**（数据段、零初始化），不是栈槽。
**W-15** **libc 地板**：`include/` 带 `<ctype.h>` `<limits.h>` `<assert.h>`，以及 `exit`/`abort`。这三个头加 `exit` 几乎每个真实 C 程序都要，之前一个都没有。两条实现上的决定：① **`exit` 是唯一不能用 C 写的库函数**——它不能返回——所以它是 `__exit` 内建（`.sys` gate）的一行包装；② **`assert` 只打印表达式，不打印文件与行号**：这个预处理器没有 `__LINE__`/`__FILE__`，因为 include 是**原地展开且不插行标记**的，第一个 `#include` 之后两者都会是错的，而**错的行号比没有行号更坏**。`<limits.h>` 给的是**类型的极限**，不是本编译器的求值宽度——[G-2] 让 `int` 表达式在 64 位里算，这不改变 `INT_MAX` 是多少。
**W-14** **多翻译单元，无链接器**：`unisa compile a.c b.c` 对每个文件**各自预处理与词法**（include guard、`#define` 状态是 per file 的），再由**同一个 walker** 依次走查，最后统一解析调用 —— 因为调用本来就是在单元结束时才解析的（`called`），所以跨文件调用与跨行调用走的是同一条路。代价两条，都记在这里：① **文件作用域 `static` 必须重命名**（`name_u<k>`），否则两个文件的同名 helper 会互相覆盖；单文件编译后缀为空，于是 tape **逐字节不变** —— 这很重要，[A-23] 要拿 Python 前端的 tape 和 unisacc 自己的 tape 逐字节比。② **作用域不分文件**：前一个文件的 typedef 与 struct tag 在后一个文件里仍然可见。这是错的 C，是真实程序绊倒时第一个要修的地方。

### 4.3 Tape 与 VM [TP]

按行文本。标签 `L:`。

| ID | 条款 |
|---|---|
| **TP-1** | 寄存器 `r0–r7`，`r7` 为 SP，初值 `0x10000`；内存 64 KB 小端 |
| **TP-2** | 算术按 2^64 取模、有符号二补码 |
| **TP-3** | `.print` / `.write` / `.exit` 是仅有的三个有外部副作用的 op，lowering 后**都必须变成真实 syscall / WinAPI 调用** |
| **TP-4** | **tape 解释器是 `--fold` 的基准真值** |

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

### 4.4 Catalog [C]

**C-1** 系统调用目录（lnx-x64 / lnx-arm / osx / win）：

exit 60/93/1 ExitProcess；read 0/63/3 ReadFile；write 1/64/4 WriteFile；open 2/56/5 CreateFileW（lnx-arm 为 openat）；close 3/57/6 CloseHandle；mmap 9/222/197 VirtualAlloc；munmap 11/215/73 VirtualFree；mprotect 10/226/74 VirtualProtect；getpid 39/172/20 GetCurrentProcessId；clock_gettime 228/113/116 QPC（osx 为 gettimeofday）；nanosleep 35/101/240 Sleep；futex 202/98/515 WaitOnAddress（osx 为 ulock_wait）；socket 41/198/97 WSASocketW；connect 42/203/98 connect；bind 49/200/104 bind；listen 50/201/106 listen；accept 43/202/30 accept；clone 56/220/360 CreateThread（osx 为 bsdthread_create）；execve 59/221/59 CreateProcessW。

**C-2** **osx sysno = `0x02000000 | nr`。** 这个 class bit 承重——§5.1 用抽掉它作反向对照。

**C-3** ABI：lnx/osx x64 参数 rdi rsi rdx r10，返回 rax，gate `syscall`；lnx arm 用 x0..，gate `svc #0`；osx arm gate `svc #0x80`；win x64 参数 rcx rdx r8 r9，form/gate 均 `winapi`。

**C-4** 字节：`add64` = `48 01 f0` / `00 00 01 8b`；`ret` = `c3` / `c0 03 5f d6`；`syscall` = `0f 05` / `01 00 00 d4`。

**C-5** MOPS 寄存器映射 r0–r7 → `rax rdi rsi rdx rcx r8 r9 r10`(x86_64) / `x0..x7`(arm64)。
**C-6** 除 `tls_base` 外，MOPS 的 arg0-2/ret/tls 一律 `none`；`tls_base` 的 tls 按 os 取 `fsbase|tpidr_el0|teb`。
**C-7** win 下 `sysno = none`，WinAPI 名字落在 `symbol`。

**C-8** 本节是 isel / abi / enc / combo 的**唯一真源**；多头 gold 必须由 `(op, os, arch)` 上的函数派生（G-9），`SYMS`/`SYSNOS` 即它吐出的值。

### 4.5 Lowering [L]

**L-1** `tape → lower(isel, abi, enc, reloc) → TargetProgram`，每个目标独立。
**L-2** TargetProgram 的每条指令携带：`form, symbol, gate, sysno, arg0-2, ret, tls, 字节, reloc kind`。
**L-3** `--fault` 的注入点在此：`osx_class_bit`、`win_argregs`、`arm_gate`。

### 4.6 目标机执行 [X]

| ID | 条款 |
|---|---|
| **X-1** | 目标解释器持有**该 arch 的具名寄存器**：`rdi rsi rdx r10/rax` 对 `x0..x8` |
| **X-2** | 系统调用分发器**只**认 lowering 产出的 `(os, sysno)` 或 `(os, winapi_symbol)`，**绝不回看通用 tape op** |
| **X-3 [修订]** | 解释器是 `--fold` 的基准真值 [TP-4]，六目标比对一律在解释器上进行。**但对宿主目标，镜像是真程序**：`tests/native.sh` 在 macOS/arm64 上实际执行并比对（E-27）。原条款的"不 execve"是范围约定，已达成后解除 |

X-2 带来的后果全是设计意图：

- abi 给 lnx/x86_64 的 write 吐 sysno 4（osx 的号）→ Linux 分发器拒绝 → stdout 不匹配
- osx sysno 丢掉 `0x02000000` class bit → 不匹配
- win 路径用 `rdi` 而非 `rcx` 传 arg0 → 不匹配

**X-4** **fold 若不是目标感知的，它就不是测试。禁止把通用 tape 跑六遍再和自己比。**

### 4.7 镜像 [I]

| ID | 格式 | 结构 | 魔数 |
|---|---|---|---|
| **I-1** | ELF64 | 单 `PT_LOAD`，vaddr `0x400000` | `7f454c46` |
| **I-2** | Mach-O 64 | `mach_header_64` + `LC_SEGMENT_64(__TEXT)` + `LC_UNIXTHREAD` | `cffaedfe` |
| **I-3** | PE32+ | MZ stub + `PE\0\0` + optional header `0x20b` + 单 `.text` | `4d5a` |

**I-4** 镜像字节可复现（D-5）。

### 4.7.1 宿主平台契约 [I-5..I-19]

以下不是我们的设计选择，是**平台强制要求**。违反其中任何一条，失败方式都不是报错而是 SIGILL / SIGKILL / 进程挂死。实测代价见 E-27。

| ID | 契约 | 违反后果 |
|---|---|---|
| **I-5** | Apple Silicon **不支持静态可执行文件**。arm64 Mach-O 必须带 `LC_LOAD_DYLINKER` + `LC_LOAD_DYLIB` + `LC_MAIN`，经 dyld 引导 | dyld 拒载，`Bad executable` |
| **I-6** | 头区必须留余量（本实现 `SLACK = 256`）：`codesign` 会**追加** `LC_CODE_SIGNATURE` | 覆写 `__text` 前 16 字节 → `udf` → **SIGILL** |
| **I-7** | arm64 macOS **强制 `MH_PIE`**，镜像会滑动 ⇒ 代码必须位置无关：arm64 用 `adrp+add`，x86_64 用 rip-relative | 绝对地址全部失效 |
| **I-8** | 可写数据（scratch / globals）必须在独立的 rw `__DATA` 段，不能放进 r-x 的 `__TEXT`；且需 `__PAGEZERO` 与 `__LINKEDIT`（后者是 codesign 写签名的地方） | 写入即 SIGBUS；无 `__LINKEDIT` 则 `codesign` 报 *failed strict validation* |
| **I-9** | **系统调用号寄存器是 OS 事实**：Darwin/arm64 用 **`x16`**，Linux/arm64 用 `x8`，x86_64 用 `rax`。因此 arm64 的地址合成临时寄存器必须避开 x16（本实现用 IP1 = `x17`） | 调用号被冲掉，**进程挂死而非报错** |
| **I-10** | `call`/`ret` 必须实现 **tape 的栈语义**（在 tape SP 上压弹返回地址），不能用 `bl`/`ret` 的 lr | 递归第二层即崩 |

**I-12** **ELF 同样要分段**：text 是 `PF_R|PF_X`，data 必须是独立的 `PF_R|PF_W` PT_LOAD，且 `p_offset ≡ p_vaddr (mod 0x1000)`。这条与 I-8 是同一条物理事实的两个平台写法；Mach-O 被 macOS 当场逼出来，ELF 因为只在解释器里跑过而藏了很久（E-32）。违反后果：写 scratch 即 **SIGSEGV**，且本地解释器与 `readelf` 都看不出来。

**K-5a** **C kernel 的字面掩码按阶段定宽**：每个字段的掩码是 `ceil(|vocab|/64)` 个 u64。一个字就够用，直到 TOKS 越过 64（`~` 与其余复合赋值把它推到 67），blob 写入直接溢出。宽度是**每阶段**的而非全局的——只有 `parse` 需要两个字，全局加宽要多花 5.7 KB。

**I-16** **PE 的节 RVA 必须按 SectionAlignment 对齐**，且要分 `.text` / `.rdata` / `.data`。第一版把 text+data 塞进一个 r-x 节、节 RVA 取 0x200，Windows 直接拒载（`Access is denied`，exit 5）。**而 `MajorSubsystemVersion` 是一个开关：声明 10.0 把加载器拨到严格路径，声明 4.0 走宽松路径**。两个 arch 都如此，包括 arm64 —— “arm64 不存在于 Windows 10 之前所以必须声明 10” 是错的，它正是我们每一张镜像都被 `STATUS_INVALID_IMAGE_FORMAT` 拒掉的原因。详见 E-35。

**I-17** **在严格路径上，arm64 Windows 确实强制可重定位，而且查得很细**（拿定主自己的 arm64 exe 逐字段拆出来的，那些 exe 声明的子系统版本都 ≥ 6）：把 dir[5] 的条目换成 ABSOLUTE 填充、目录原样保留，它立刻不加载；只把 dir[10] load config 的 `SecurityCookie`（+0x58）清零，它也不加载。**但这两条并不是 PE 的普遍要求** —— 我们曾从它们推出“自己的镜像也必须带 .reloc 与 load config”，那是错的：把 `MajorSubsystemVersion` 改成 4.0，一个 **无 .reloc、无 load config** 的位置无关镜像直接跑起来。教训是方法学的：**从一个能跑的样本里拆掉某字段 → 它不跑了**，只证明该字段在**那个样本所在的模式**下是必需的，不证明它在别的模式下也必需。详见 E-35。

**I-18** **WinAPI 调用是真调用**：它按 AAPCS64 / Win64 破坏全部 volatile 寄存器，而我们八个 tape 寄存器**全都**是 volatile，**tape 栈指针 r7 也在内**。所以 win 的 gate 必须前后夹一个保存区，tape 必须有**自己的栈**（不能像别处那样把 SP 绑到进程栈），并且要把 tape 说的 POSIX 形状翻译成 kernel32 的形状（`fd → HANDLE`、`WriteFile` 的第四个出参、返回写入字节数而非 BOOL）。

**I-20** **`open` 是最不可移植的系统调用，三个 OS 三个样子**：① `O_CREAT/O_TRUNC/O_APPEND` 的**位值** Linux 与 BSD 不同（64/512/1024 vs 512/1024/8），我们曾把 Linux 的硬编进 `include/stdio.h`，于是 macOS 上 `fopen("w")` 静默失败；② **Linux/arm64 根本没有 `open`**，目录里的号是 `openat`，第一个参数是目录 fd（`AT_FDCWD = -100`），**所有参数往后移一位**，mode 落在第四个寄存器——而 abi 网只有三个参数头 [C-1]，所以这一移位是**结构性代码**，在 lowering 里；③ **Windows 根本没有 open(2)**，gate 是 `CreateFileA`，它要的是 dwDesiredAccess 与 dwCreationDisposition，而且 disposition 是**第五个**参数（得从 r8/x2 抖到影子空间）。第三条的翻译放在 **C 库里**（`#ifdef _WIN32`）而不是编码器里，因为只有它知道在为哪个平台编译；代价是这类源码像 `host.c` 一样**每个目标一份 tape**，解释器因此要记住 tape 是为哪个 OS 编的（`src_os`）。

**I-21** **gate 有返回值**，而目标机解释器曾经把它丢掉：结果是返回寄存器里留着**系统调用号**。`write` 与 `exit` 不用返回值，所以这个 bug 活了很久，直到一个程序用 `open()` 的结果去 `write`，它就把 0x2000005 当成了 fd。与 [TP-6] 同类：**解释器比真机宽容的每一处，都是一个迟早会爆的雷**。

**I-22** **arm64 的局部变量寻址有两条窄路，越界是"改符号"而不是"截断"**：缩放正偏移 `imm12`（`[fp, #off]`，按宽度缩放）够不到负偏移；非缩放形式 `LDUR/STUR` 的立即数是**有符号 9 位**，范围只有 **−256..255**。我们直接把偏移 `& 0x1FF` 塞进那个字段，于是 `[fp, #-260]` 编成了 `[fp, #+252]` —— **不是读到截断的地址，是读到另一个局部变量**，没有异常、没有诊断，只是答案不对。于是**任何帧超过 256 字节的函数在三个 arm64 目标上都是错的**，而我们所有探针的帧都太小，六目标 `--fold` 也看不见（解释器不建模寻址模式）。真实代码一天之内撞到三次（`char buf[1024]` 是 C 里最常见的局部变量之一）。办法：两条范围都不够时，把地址算进一个 scratch 寄存器再访存。见 E-45。

**I-19** **一个文件带多条 ISA**：macOS 的标准容器是 Mach-O universal（fat）——大端的 slice 描述表 + 各自按页对齐的普通镜像，内核挑 slice，`codesign -f -s -` 会把每个 slice 都签掉。这是多 ISA 主张**诚实的前半**：它是**一个 OS 之内**的多架构；cosmopolitan 那种同时是 ELF / Mach-O / PE 的文件是另一个问题，我们还没做。

**I-15** **PIE 的 Mach-O 必须告诉 dyld 怎么 rebase**：既无 `LC_DYLD_INFO` 也无 chained fixups 时，dyld 走**旧的重定位路径**，去解引用我们从未发射的 `LC_DYSYMTAB` —— 在我们第一条指令之前**就在 dyld 里面崩了**（`forEachRebase_Relocations`，EXC_BAD_ACCESS at 0x48，即空 dysymtab 上的 `locreloff`）。**Darwin 25 容忍这个缺省，Darwin 23/24 不容忍**。办法是把 `LC_DYLD_INFO_ONLY` / `LC_SYMTAB` / `LC_DYSYMTAB` 三条**全零地**发出来，dyld 于是走 opcode 路径、发现无事可做。字符串表给 8 个 NUL（字符串表不能为空）。

**I-13** **x86_64 的 `push`/`pop` 不能用**：`spinit` 把 tape SP 绑到真 `rsp`，于是 tape 栈的第一个槽正是 `push` 要写的地址，两者互相覆盖。`idiv` 当年就是这么保存 rax/rdx 的，症状是返回地址被踩、**每一个打印整数的 x86_64 程序都崩**。一切寄存器保存都必须走 **tape 栈**（这正是 I-10 的含义）。

**I-14** **x86 ALU 是两操作数**：`dst = s1 op s2` 要展开成 `mov dst,s1; op dst,s2`，当 `dst` 就是 `s2` 时，那条 `mov` 先把右操作数毁了 —— `17 - 5` 算成 `17 - 17`。同理移位的计数必须在 `cl`，而 **`rcx` 是 tape 寄存器 r4**，必须存回。arm64 是三操作数指令，解释器也不建模这些，所以两条都只能被真机看见（E-33）。

**I-11** `ld`/`st` 必须尊重宽度与偏移符号：局部变量在 `[fp − off]`，**负偏移用不了 scaled imm12**，arm64 需 `LDUR/STUR` 系列；`int` 是 4 字节，按 64 位取会读进相邻变量。

---

# 第四部分 · 验收与度量

## 5. 验收与度量

### 5.1 fold [A]

**A-1** 对 6 个目标各自独立走：

```
tape → lower → TargetProgram → 镜像 + 目标机解释执行
```

**A-2** 六份 `(stdout, exit)` 必须全同，并与 `vm(tape)` 对拍一致（P-7）。

**A-3 反向对照（必做）**：`unisa run examples/hello.c --fold --fault osx_class_bit` 必须打印 **4/6**。这证明 fold 有牙齿。若仍 6/6，说明测试是假的。

### 5.2 样例 [A-4]

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

### 5.2.1 差分测试 [A-17] —— P-6 的唯一仪器

`unisa acc` 只证明 **net ≡ gold**（P-3）。它对 **gold ≡ C99**（P-6）**完全沉默**，而 gold 已被实现打脸六次，每次 acc 都是满分：

| | 缺陷 | 被什么发现 |
|---|---|---|
| E-2 | `type` 的 illegal 行被 1/19 降权 | acc 卡 0.999 |
| E-15 | `parse` 的 stmt 行漏 struct/enum/typedef | 编译报错 |
| E-16 | `gate` 挂在 `isel` 上，而 gate 是 OS 事实 | lowering 发错 gate |
| — | `type` 缺 `ptr − num → ptr` | **差分测试**（静默错答案）|
| — | `,` 判成 illegal；TOKS 缺 goto/union/位运算符 | 编译报错 |
| — | 字符串字面量无 NUL 结尾 | **差分测试**（静默错答案）|

**[A-17]** `tests/difftest.sh`：同一份源码交给参考编译器（`cc`）与 `unisa`，比对 stdout 与退出码。探针集在 `tests/c/`，分两类：`a_*` 已知应支持、`b_*` 边界探针。**每次改动 gold 或走查器后必须跑。** 它同时把"C99 覆盖是子集"从定性描述变成可测数字。

> 参考编译器需要 `printf` 的声明而我们尚无 `#include`，所以对拍只给**参考**那一侧补 `#include <stdio.h>`，其余一字不改。

**[A-18]** `tests/native.sh`：在宿主平台**真实执行**镜像并与解释器比对（见 E-27）。
**原生执行是比解释器更强的 oracle** —— 解释器内存**清零**，会掩盖未初始化读；真机栈是垃圾，会把它们抖出来（`b_static` 即如此暴露）。

**[A-25]** `tests/corpus.sh`：外部语料 [c-testsuite](https://github.com/c-testsuite/c-testsuite)，220 个单文件 C 程序，**不是我们写的，也不是为我们写的**。三类判定：

| 类 | 含义 | 是否失败 |
|---|---|---|
| `pass` | 编译并跑出期望输出（**编成本机镜像真跑**，宿主无匹配目标时才退回解释器） | — |
| `unsupported` | 前端**拒绝**该程序 | 否，这是诚实的覆盖缺口 |
| `wrong` | 编译通过但输出不符 | **是**，这是误编译 |
| `knownfail` | 列在 `tests/corpus.knownfail` 的非 C99 扩展 | 否；但它**开始通过**时判失败，防止清单腐烂 |
| `slow` | 超过每程序时限（默认 10 秒） | 否；记账用。纯 Python 的 tape VM 上八皇后要十几分钟，这正是改走原生执行的原因 |

`tests/corpus.baseline` 是棘轮：`pass` 只能升不能降。

> 自有探针度量的是**我们想得到的东西**；外部语料度量的是**我们实际覆盖了什么**。两者的差距就是 E-30 里那六个缺陷。

### 5.3 验收清单

| ID | 断言 | 依赖条款 |
|---|---|---|
| **A-5** | `unisa run examples/hello.c --fold` → 6/6，`hello from C99\n` | X-1..4, L-1 |
| **A-6** | fact→120、switch→6、do→3、fib→55、ptr→7、struct→9，全部 6/6 | A-4 |
| **A-7** | `host.c` → 每个 OS 组内 2/2 | A-4 |
| **A-8** | `--fault osx_class_bit` → **4/6** | A-3, C-2 |
| **A-9** | `unisa compile` → 三魔数正确 | I-1..3 |
| **A-10** | 同输入编译两次 → 字节相同 | D-5 |
| **A-11** | `weights/parse.i8.unisa` 以 `554e5331` 开头 | Q-1 |
| **A-12** | `unisa acc` → **每阶段 = 1.000**，验的是**出货的构造权重**（0.05 秒，6,650 key 全枚举）；`--trained` 才看 SGD 对照组，且**不作门槛** | F-3, P-3, U-5 |
| **A-13** | `unisa quant` → 每个出货阶段在其记录 dtype 下 argmax 不变 | Q-6, P-4 |
| **A-14** | 构造两次 → `built.uns2` 字节相同。**套件不再训练**：训练是分钟级满核工作、不在出货路径上，曾把套件变成两小时的活 | D-3, D-6, U-5 |
| **A-15** | `unisa ship` → kit 四件套齐全 | Q-10 |
| **A-16** | Python kernel 与 `unisa_boot.c` 在 FULL gold 上逐 key 同类 | K-4 |
| **A-25** | `tests/corpus.sh` → `wrong = 0`，且 `pass` 不低于 `tests/corpus.baseline` | P-6 |
| **A-26** | 在**真 Linux 内核**上执行发出的 ELF（不是解释它）。默认由本机 Lima 虚机承担（[A-32]），GitHub 的 runner 只在把目录挪回来之后作为无尘室复核 | I-1, I-12, X-3 |
| **A-27** | `tests/crossnative.sh` → lnx/x86_64、lnx/arm64、osx/x86_64、win/arm64、win/x86_64 在真机上与解释器逐例一致；**虚机没开就跳过并点名**，`STRICT=1` 时跳过算失败 | I-12..14, X-3 |
| **A-28** | `unisa fat` 产出 Mach-O universal，两个 slice 都执行且与解释器逐例一致 | I-19 |
| **A-32** | `tests/linux.sh` → 整套套件在**本机虚机的真 Linux 内核**上跑过（2026-09-21 实测：`difftest 78/0`、`tools 11/11`、`corpus 209`；2026-09-23 复测 `run 11/11`、`ape 3/3`）。虚机把仓库**只读**挂载，而套件要写（`build_ref.sh` 在源码旁边生成 `unisacc.c`），所以先把树拷进客机再跑；`corpus/` 软链回挂载点，因为套件只读它。**Linux 走 Lima 而非 UTM**：UTM 里那两台 Linux 没装 QEMU guest agent，`utmctl exec/file/ip-address` 一律失败，网络 Shared 无端口转发、MAC 不进宿主 ARP 表，所以也没有现成 SSH 路；Windows 那台**装了** agent，所以 UTM 管 Windows。**CI 只做测试，不做构建**（2026-09-23 定）：交付物在本机一个环境里交叉编译出全部目标，再经 GitHub release（在途时用草稿）搬运；Actions 只是干净机器上的第二意见。仓库转公开后 runner 免费，`.github/workflows/ci.yml` 已恢复在推送时触发，但它仍然不是产物的来源。当初把 workflow 挪走是因为：runner 是计费的，macOS 那两个按 10 倍计价，而这台机器上 macOS（原生）、Linux（Lima）、Windows（UTM）**三个平台都有**，六个目标全都能真跑。**按下推送就烧一次额度**，等于把钱花在这台机器不花钱就能做的事情上 | A-26, A-27 |
| **A-33** | `tests/stages.sh` → 每个探针上，Python 前端问过的每个决策阶段，C 前端也问过。**所有别的套件量的都是答案**，而一条与表一致的手写规则答案与表完全相同 —— 只有这一条能看见「决定是不是网络做的」 | T-2, E-52 |
| **A-34** | `tests/closure.sh` → 每个探针、每个目标，`unisacc FILE -b os/arch` 写出的镜像与 Python 后端从同一条 tape 写出的**逐字节相同**；宿主目标的镜像真跑，输出与 VM 一致。一个头字段、一个位移、一个 REX 前缀错了都会在这里现形 | S-6, E-55 |
| **A-35** | `tests/nativeboot.sh` → cc 编出的 unisacc `-b` 造出 unisacc（N1），N1 造 N2，N2 造 N3：**N1 = N2 = N3 逐字节**，且 N2 交叉写出的其余五个目标与 cc 编出的 unisacc 写出的相同；UTM 的 Windows 机器开着时，win/arm64 与 win/x86_64 的 unisacc 在 Windows 上各自重建自己，逐字节相同。**全程没有 Python** | S-6, E-55 |
| **A-36** | `tests/ablate.sh` → 把某个阶段（或某个输出头）的答案**旋转成错的**，重编全部探针的六个镜像：必须有镜像变了，或者编译被拒。问过一个网络不等于听它的——[A-33] 只能证明"问过" | S-6, E-57 |
| **A-37** | `tests/run.sh` → `unisacc -run FILE.c` 编译并在内存里运行，stdout 与退出码都要与系统 cc 编出的二进制一致。不写镜像，也就没有代码签名这一步 | S-9, E-56 |
| **A-39** | `tests/cli.sh` → 在一个与本仓库无关的空目录里，把编译器当**工具**用：自带头文件、`-I`、`-D`、shebang、argv、退出码，以及"文件不存在要被诊断"。一个能用的编译器，是在它是你手上唯一一个文件时也能用的 | S-11, E-59 |
| **A-38** | `tests/ape.sh` → 构建 `unisacc.com` 并在本机运行它：检验头部（shell 必须接受第一行）、脚本里的偏移（BSD tail 的八进制陷阱）与切片本身。其余平台由 `linux.sh` 与 `crossnative.sh` 的 Windows 机器承担 | S-10, E-56 |
| **A-41** | `tests/layout.sh` + `tests/datashape.sh` → 数据布局的**枚举**验收：`lower.zero_last` 是纯函数，又有 Python 参考实现与 C 移植两套，于是枚举它的定义域——所有长度 ≤3 的数据定义序列（字母表 `{bss 1, bss 8, str 1, str 7, str 8, 全 NUL 的 str}`，长度取 1/7/8 才看得见 mod 8 规则），加上"同一符号定义两次"的变体，762 条 tape × 6 个目标 = 4,572 次逐字节比对，12 秒。`datashape` 则**生成**声明序 ≠ 地址序的 C 程序，并在比对字节之前先断言"这种形状确实出现了"——否则生成器退化后套件会在什么都没测的情况下继续绿 | A-34, E-61 |
| **A-42** | `tests/bigclosure.sh` → 对**编译器自己**做闭环：707 KB、1,874 个数据符号，六个目标的镜像与 Python 后端逐字节相同，且宿主那份镜像必须是一个能工作的编译器。90 个探针全绿而它六个目标全崩过——抽样对"只在大程序里自然出现的形状"是系统性失明的 | A-34, E-61 |
| **A-43** | `tests/consts_check.py` → `unisa/lower.py` 的常量链（`SCRATCH`、`PRINTMAX`、`WIN_HSTD`…`WIN_EXTRA`）与 `src/unisacc_back.c` 里对应的**字面量**逐个相等，共 17 个数。C 后端是手抄的，它只带结果不带推导：改一个 Python 常量会一次作废六个 C 数字，而唯一会发现的是 closure 报两张镜像在某个字节不同——那是真的，但它说不出为什么。这一条说得出 | A-34, E-61 |
| **A-44** | `tests/c99.sh` + `tests/c99/` → 每个 C99 特性一个探针，与系统 `cc` 对拍**输出**（编译通过不算通过）。分母写自标准自己的变更清单，**不写自我们的支持范围**——分母跟着分子动就什么都没量到。`corpus` 的 214/220 太宽容：那批程序又短又重叠 | A-25, S-10 #9 |
| **A-45** | `tests/bench.sh` → 编译耗时作棘轮：编译自己（707 KB）、一个小探针、以及**出货二进制与 cc -O2 构建之比**。速度从来没被量过，所以可以随便烂掉——一个每 token 调用一次、每次从头走表的查找，把 57% 的自编译时间放进了 strlen，而没有任何仪表会发现。基线按机器分文件，容差 30%。**一次耗时为零的测量等于什么都没跑**，所以有下限检查：这个套件的第一版把 "command not found" 的 15 ms 当基线记了下来 | S-10 #10, #11 |
| **A-46** | `tests/fuzz.sh` + `tests/gen_prog.py` → 从种子生成随机 C 程序，与系统 `cc` 对拍**输出**。别的套件测的都是**有人想到的东西**；活下来的缺陷在**没人组合过的组合**里。生成规则必须让每个程序行为**有定义**，否则两边有权不同、套件就只是噪声——其中一条规则是用一个 bug 换来的：**循环计数器在循环体内只读**。第一版允许循环体给它赋值，种子 2 生成的程序在 cc 下也跑了二十秒以上；一个会写出不终止程序的生成器，量的是超时不是编译器 | A-17, S-10 |
| **A-47** | `tests/hostile.sh` → 编译器遇到**意料之外的输入**时必须**退出**：截断的文件、未闭合的注释与字符串、include 循环、5000 字符的标识符、嵌套一千层的括号、二进制垃圾、嵌入的 NUL、自我展开的宏、一个目录当输入。判据分两类：(1) 不许崩、不许挂；(2) C 说无效的东西**必须被诊断**——静默接受非法 C，等于产出一个作者从没写过的程序。当天就抓到两个：未闭合注释被静默吞掉，`struct S { struct S inner; }` 被静默接受（`sizeof` 是半成品表项里碰巧的值）。两个都已修 | A-40, S-12 |
| **A-48** | `tests/docs.sh` → `prd.tree.md`、`prd.map.md`、`README.md` 里的阶段表由 `python3 -m unisa docs` 从 `unisa/gold.py` 与构造权重**生成**（标记区 `<!-- stages:begin -->`…`<!-- stages:end -->`），过期或标记丢失即失败。由来：阶段表在四个文件里各手抄一份，11→14 重构后其中两份照旧描述 11 阶段的编译器整整一天（`type 960`，实为 4,275），没有任何机制发现 | S-10 #5 |
| **A-49** | `unisacc --check-oracle` → 模型能被问到的**每一个**问题（14 个阶段、15,222 个问题）经缓存问一遍，与网络直接作答逐个比对，正序、逆序各一遍（驱逐与碰撞取决于顺序）。由来：oracle 缓存的第一版存的是问题的**哈希**、比的也是哈希；全定义域上有两个问题撞同一个哈希，命中时就静默交出另一个问题的答案。哈希计算还有有符号溢出（UB），clang 与 gcc 在 -O2 下算出不同的值，于是错答案在两个编译器之间**挪位置**——Linux CI（gcc）拒绝了一个 Mac（clang）能编的 C99 程序。把比较退化回"只比部分字段"时，这条报出 6,646 个错答案 | A-33, P-3 |
| **A-50** | `tests/gold_audit.py` → `type` 表对照系统 cc 逐键审计：10 个数值类型两两 × 14 个二元运算（1,400 键），cc 用 C11 `_Generic` 答结果类型、用编译错误答"非法"，逐键与 gold 比对。**枚举只能证明网络等于表，这一条问表是不是 C**——论文 §8 所述局限的第一个仪表。首跑即发现 200 个键错（比较运算的结果类型写成了 long，与 [G-2] 同源），已在 gold 中改正；唯一的有意偏离（`i64 & i64` 这一行编码的是取地址）记在 `tests/gold.knownfail` 并写明理由 | P-3, S-15 A1 |
| **A-51** | `tests/abi_audit.py` → `abi` 表的系统调用号对照**本机**的 `<sys/syscall.h>`：每台机器只担保自己那一格（本机 osx、Lima lnx/arm64、CI 的 Linux runner lnx/x86_64）。某平台没有对应系统调用的 op，catalog 必须写 `none`、由 lowering **拒绝**，而不是借一个号码——首跑发现 macOS 的 `nanosleep` 映射到 240，即 `SYS_listxattr`；`clock_gettime` 映射到参数完全不同的 `gettimeofday`。两个后端现在都拒绝这种 op | C-1, S-15 A2 |
| **A-31** | `tests/tools.sh` → 别人的库代码（crypto-algorithms，八个算法，每个多文件、自带已知答案测试）与 `cc` 同输出，且 `pass` 不低于 `tests/tools.baseline`。**语料清零只说明前端不拒绝，不说明跑对** | W-14, I-22 |
| **A-30** | `tests/multi.sh` → 两个翻译单元编译成一个程序，与 `cc a.c b.c` 同输出，`--fold` 6/6，且本机镜像真跑。语料构造成**共享会被看见**：两个单元各有同名不同值的 `static` | W-14 |
| **A-29** | `tests/selfgap.sh` → unisacc 接受的程序数不低于 `tests/selfgap.baseline`（自举差距只能缩小）。**A-23 的不动点不是覆盖率**：unisacc.c 只需接受它自己用到的子集，于是三十次提交里前端特性单边堆在 Python 侧而套件量不到 —— `selfhost.sh` 只比词法器，`ccrun.sh` 遇到拒绝就打印 `UNS` 走人。这条把那个数变成棘轮 | A-20, A-23 |

### 5.4 体积与速度预算 [B]

头号主张是体积，就把它量出来，摆到现有方案旁边。

| ID | 指标 | 怎么测 | 目标 |
|---|---|---|---|
| **B-1** | 总 θ | 所有网络求和 | 记录 |
| **B-2** | kit 字节 | `unisa ship` 按逐阶段最低 dtype | 权重 **≤ 32 KB**；kit ≤ 64 KB |
| **B-2a** | **对比基线必须写明** | **训练**权重比裸表大（E-3），故其体积主张只能对"手写算法代码"成立；**构造**权重比裸表小 2.19×（E-22），无此限制 | 报告时并列四列：裸表 / 训练权重 / 构造权重 / tcc |
| **B-3** | 对比 tinycc | `size $(which tcc)` 或 tcc 发布二进制 | 记录比值 |
| **B-4** | 决策吞吐 | `unisa bench`，逐阶段，冷 | Python ≥ 50k/s；`unisa_boot.c` ≥ 5M/s |
| **B-5** | 端到端编译 | `unisa run examples/fact.c` | ≤ 1 s |

**B-6** `unisa bench` 必须报**冷**数据。在确定性全函数上加 memo 缓存是正当工程手段，`run` 可开；但 `bench`/`acc`/`quant` 必须关——否则测的是 dict，不是 kernel。

### 5.5 完成度盘点 [S-*]

盘点分四层，因为**难的部分和多的部分不是同一部分**：论点层基本做完了，产品层才走了一半。每层的"100%"是一条可测的判据，不是一个百分比。

| 层 | 目标 | 完成判据 | 实测（2026-09-21） |
|---|---|---|---|
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。14 个 stage，6,650 key 全枚举，零分歧（2026-09-23 复核） |
| **S-2 目标** | 一条 tape → 六份镜像，stdout/exit 一致 | 六个目标**在真机上**逐例一致 | **已达**。`unisa fat` 是**一个 OS 内**的多架构；cosmo 式一文件多目标（MZ + shell 自选切片）**已达**，见 S-7 #9；真正同时合法为 ELF/Mach-O/PE 的**单一字节序列**仍未开工 |
| **S-3 语言** | C99 子集覆盖别人写的代码 | 外部语料 `unsupported 0`、`wrong 0` | **已达当前语料，含浮点**（2026-09-22）。`corpus 220 pass 214 wrong 0 unsupported 0 knownfail 6`；余下 6 个全是 C99 之外的扩展（GCC 语句表达式、空结构体、`push_macro`、C11 `_Generic`）与 [G-2] 的 64 位整数求值，见 E-54 |
| **S-4 产品** | 对标 tcc：能编译常见 C99 工具；真正自举 | ① 工具语料棘轮（**未建**）② unisacc 自己造出自己的可执行文件 | **最远**。见下 |

**S-5 为什么"推到 100%"没有理论风险。** [P-8] 的构造式存在性定理给了上界 `h = |K|`，所以"能不能到 100%"**不是开放问题**，开放的只有最小性；[F-5] 说到不了 1.000 是 **key 编码错了**，不准加宽网络；[D-7] 说网络与 gold 不一致是**其中之一有 bug**，不是模型方差；[D-1] 推理期无 RNG。合起来：**这个项目里任何一处"差一点"都是缺陷，没有一处可以赖给随机性**。剩下的全是确定性工程量，可枚举、可验收。

**S-6 自举闭环已达：没有 Python 的自举。**（2026-09-22，E-55）unisacc 带着自己的后端（`src/unisacc_back.c`：lowering、两个编码器、ELF/Mach-O/PE），`unisacc FILE -b os/arch` 直接写出可执行文件。判据是最严的那种：对每个探针、每个目标，它写出的镜像与 Python 后端从同一条 tape 写出的**逐字节相同**（[A-34] closure 540/540）；cc 编出的 unisacc 用 `-b` 造出 unisacc，那个原生 unisacc 再造自己，**N1 = N2 = N3 逐字节**，并且交叉写出的其余五个目标也一致（[A-35] nativeboot，osx/arm64 与 lnx/arm64 两台真机）。[A-23] 的 B=C=U 仍保留，它证明的是 tape 经 Python 后端的不动点；这里证明的是镜像经自己的后端的不动点。

**S-7 "能编译常见 C99 工具"的卡点是形态，不是语言特性。** 按"离判据多远 ÷ 成本"排：

| 序 | 事项 | 状态 |
|---|---|---|
| 1 | **自举差距上棘轮** —— 止血，否则后面每一步都在加深 | **已做**，[A-29] |
| 2 | **多翻译单元** —— 不必做目标文件格式与链接器 | **已做**，[W-14] [A-30] |
| 3 | **libc 地板** `<ctype.h>` `<limits.h>` `<assert.h>` + `exit` | **已做**，[W-15] |
| 4 | **运行期格式串的 `printf`** —— `<stdio.h>` 本来就有完整的 `_u_vfmt`，缺的只是把非字面量格式串从内建路径放行 | **已做**，[W-9] |
| 5 | **工具语料棘轮** —— 真实库代码，多文件，自带已知答案测试 | **已做**，[A-31]，三个仓 `pass 11/11`，见 E-45、E-46 |
| 6 | **自举前端追平**（由第 1 项驱动） | **已达**（2026-09-22）：probes 90/90、`ccrun` 90 一致 / 0 已知分歧 / 0 拒绝，语料接受 216/220（覆盖 Python 前端通过的全部 209），词法器 83/0，两平台全绿，见 E-47..E-53 |
| 7 | **自举闭环**：lowering/编码/镜像进 C | **已达**（2026-09-23 扩充）：镜像与 Python 后端 540/540 逐字节相同，原生 N1=N2=N3；osx/arm64、lnx/arm64、lnx/x86_64、win/arm64、win/x86_64 **五个目标**真机自举，全程无 Python。见 E-55、E-60、E-61 |
| 8 | **浮点** | **已达**（2026-09-22）：两个前端、两个 ISA、VM 与目标解释器；`%f/%e/%g` 与平台 libc 逐位一致；`<math.h>` 为 fdlibm，1 ulp 以内。见 E-54 |
| 9 | cosmo 式三格式单文件 | **已达**（2026-09-23）：`unisacc.com` 一个文件，对 Windows 是 PE、对 Unix shell 是脚本，内含四个切片；macOS/arm64、macOS/x86_64（Rosetta）、Linux/arm64、Windows/arm64（x64 仿真）都跑通。见 E-56 |
| 10 | **编译即运行**（`tcc -run` 那一类） | **已达**（2026-09-23）：`unisacc -run FILE.c` 不落盘，直接在内存里编译并运行。见 E-56 |
| 11 | **像 tinycc 一样可用**：自带头文件、`-I`、`-D`、shebang | **已达**（2026-09-23）：11 个头文件嵌在二进制里，任意目录可用；`unisaccrun` 并入 `unisacc`，产物是单文件 `unisacc.com`。见 E-59 |

**S-15 0.0.7 计划**（2026-09-24 定，2026-09-24 按摸底扩充）。

主线三条：**把"正确性证据"延伸到 gold 本身**（枚举只证明网络等于表，00200 证明表会错）；**拉动体积与速度的真杠杆**（0.0.6 实测找到的那两个）；**让编译器能直接塞进现成的 Makefile**。每项带可测判据，按依赖与性价比排序；P0 必做，P1 应做，P2 视余力。

*摸底依据（2026-09-24 实测）*：C99 标准头 24 个里有 13 个；常用 libc 缺 `qsort bsearch strtok strncat sscanf fseek ftell remove rename perror getenv atexit labs div`；遇第一个错误即停、零警告（同一文件 `cc -Wall` 给 5 条）；`-lm -l -L -U -include -MD -x -nostdinc` 与 stdin 输入均被拒；x86 text 的 36% 是 tape 栈机经 r10 的 push/pop，`call` 平均 19 字节；`.com` 约 62% 是未压缩的 PE；出货二进制编译自己比 cc -O2 构建慢 9.4×。

**A. 正确性：审计 gold 本身（P0）**

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| A1 | **type 表对照 cc 的审计** | [G-2] 那条错规则（`int + int` 是 long）在 gold 里待了整个项目，枚举 1.000、全部套件绿——因为枚举只能证明网络等于表。以系统 cc 为裁判：对 4,275 个键中的数值组合生成 `sizeof((T1)0 op (T2)0)` 与结果符号性探针，一次编译、逐键对照 | 新套件 `gold_audit`：type 表与 cc 分歧为 0，或每条分歧进 `gold.knownfail` 并写明理由（例如我们有意不区分 `long` 与 `long long`） |
| A2 | **abi 表的系统调用号对照系统头** | 六个目标的 syscall 号是手录的；在有 `<sys/syscall.h>` 的机器上逐个比对 | Linux（Lima 两台）、macOS 本机分歧 0 |
| A3 | **fuzz 扩面** —— **已达**（2026-09-24）。`tests/gen_prog.py` 分 8 类（int/unsigned/narrow/struct/pointer/float/recursion/mixed），`tests/fuzz.sh` 每类 N 个种子。7 个新类**第一次跑**就在 C 前端抓到三个 90 个手写探针从未碰到的错：形参的 `sizeof` 是 8 字节槽位（于是 `unsigned p` 在类型轴上是 u64，`(b*p)/13u` 从未截到 32 位）；带 `u` 后缀、超过 INT_MAX 的十进制常量当成 long（C99 6.4.4.1 说是 unsigned int）；`*p`（结构体指针）传值时装入前 8 字节当地址（callee 段错误）。Python 前端三处都对——这正是 selfhost/stages 只有在探针碰到时才看得见的那类缺口；`tests/c/b_fuzzfound.c` 固定它们。修后 **8×60 = 480/480** 一致；生成器自己的一个 UB（`int *` 指向 `unsigned char`）也是这轮抓出来的 | ✅ 8 类各 ≥60 种子全部一致；`N=125`（8×125=1000，种子 1000–1124）长跑 **999/1000**，唯一不一致（m_01021）是两个前端共有的规则错：窄无符号**结果**从不零扩展（`21 - (v & 1023)` 是 u32，扩到 long 时带着符号），操作数在下一步才掩码。修法是把不变量立起来——寄存器里的 u8/u16/u32 永远零扩展——两个前端各五处（二元结果、复合赋值（并且在公共类型里做：`h >>= 1` 曾把符号位移进来）、`++x`、`-x`、`~x`）；`tests/c/b_uzext.c` 固定 |

**B. 体积与速度（P0）**

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| B1 | **regmap：x86_64 的 tape 栈指针映射到 `rsp`** —— **已达**（2026-09-24）。改的是 gold 的 regmap 表（r7 → rsp），重建权重、枚举全 1.000。两个后端各自把前端的 `.frame 8; store64 [r7+0], r` / `load64 r, [r7+0]; .frame -8` 融合成 `push`/`pop`（仅当第二半不是任何标号的目标），`call`/`callr`/`ret` 用机器自己的形式；`[rsp+d]` 加 SIB 字节。Windows 的 x86_64 改在进程栈上跑 tape（WinAPI 门自己对齐并恢复 rsp），arm64 维持自有栈（x7 不是 sp）。实测：编译器自身 lnx/x86_64 text **750,137 → 475,534 B（−36.6%）**；closure 540/540、fat 90/0（Rosetta 真 x86 执行）。途中两个后端各漏改一处（Python 的 `.frame`、C 的 `.div` 溢出基址）——都是 closure 逐字节比对当场抓出来的 | ✅ −36.6%，判据 ≥20% |
| B2 | **`.com` 的 Windows 部分自解压** | PE 占 `.com` 约 62% 且原地执行。一个小 PE 存根 + **用 C 写、由 unisacc 自己编译**的 inflate，解出真正的 PE 再执行（顺带是一次对编译器的真实负载测试） | `.com` 降 ≥30%；win/arm64、win/x86_64 实机跑通 `-run` 与 `-b` |
| B3 | **B1 之后重测速度与体积，写进 bench 与 README** | 不量不算 | bench 记录新基线；README 的体积与速度数字与实测一致（生成或检查） |

**C. 工具面：能塞进 Makefile（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| C1 | **接受并正确处理**：`-lm`/`-l*`/`-L*`（libm 本就在头文件里，接受即可）、`-U`、`-include FILE`、`-x c`、`-nostdinc`、输入 `-`（stdin）、`-o -` | `cli` 套件逐个覆盖；一个真实的 `make CC=unisacc` 能构建 tools 语料里的一个库 |
| C2 | **`-MD`/`-MF`** 写依赖文件 | 生成的 `.d` 与 `cc -MD` 列出的头文件集合一致（按我们实际读到的） |
| C3 | **多错误** —— **已达**（2026-09-24）：没有 longjmp，所以出错不是回退而是把 token 指针**停在文件尾**：走查器的每个循环都在 EOF 停（hostile 套件的截断输入早就逼出了这一点），递归自己退回顶层；`unit()` 重新同步到出错构造之后的下一个 token 继续。停机期间的错误是级联、不报。`-ferror-limit=N`（clang 的拼法，默认 20，0 不限）。`diag` 加三个错三处都报（行号 3 7 10）、限流、以及**损坏语料**仪器：前 40 个 c-testsuite 程序各删掉第三个 `;`，要求有诊断、退出 1、不挂不崩 Python 前端同样：每个顶层构造 `try`，出错记录、重同步、丢掉函数内状态、继续；两个前端对同一文件报同样三处。**顺带**：损坏语料仪器第一次跑就抓到两种不走 `文件:行:列: error:` 形制的诊断（词法的“bad char at 字节偏移”、作用域表与走查器不一致的内部告警）和 `for` 头部出错后的一个死循环——660 个损坏程序（220 × 3 个删点）现在全部干净诊断 | ✅ diag 14/14；hostile 21/21；损坏语料 660/660 |
| C4 | **一组高价值警告** —— **已达**（2026-09-25）：`-Wall`（或 `-Wextra`）之下报四类，形制同 clang 含 `[-W…]` 标签：非 void 函数可能掉出末尾（无流图：每条语句退出时算“能否落出”——return/死循环/不返回的调用不能，块取末句，带 else 的 if 取两支，其余能；main 豁免）；printf 字面量格式与实参不符（无论走编译期展开还是运行时 printf，都对实参做一次**只取类型的干跑**再回退发射器，像 `expr()` 回退误起那样）；整数隐式转指针（赋值与初始化；`0`/NULL 与函数指示符豁免）；未使用的局部（写而不读也算，clang 叫 set but not used；`x = …` 作为语句开头才算只写）。头文件内的警告不报，如 cc 对系统头。新套件 `warn`：四个探针与 `cc -Wall` 的 (行, 类) 集合**双向相等**；再对 220 个语料程序要求**零个 cc 不报的警告**。校准过程 16 → 0 个假阳性，顺带修了五处类型事实：`*p` 的宽度是被指对象的（曾沿用指针的 8 字节，`unsigned *u` 的 `*u*3` 从未截窄）、`c ? p : 0` 是指针、`&&`/`\|\|` 是 int、枚举常量是 int、以及一个**真 bug**：`char x[1]` 成员按 `n > 1` 判定成了标量，传值结构体参数的 `a.x` 装的是那个字节——c-testsuite 00204 在 `-Wall` 下段错误；`tests/c/b_arr1memb.c` 固定。Python 前端不产警告（套件只判 C 前端） | ✅ warn 探针 4/4，语料假阳性 0/220 |

**D. 库（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| D1 | **头文件** —— **已达**（2026-09-24）：`<errno.h>`（`errno` 是镜像里一个普通 int，库函数在 C 规定处设置）、`<float.h>`（IEEE binary32/64 的常量，字面量与 cc 的一致）、`<iso646.h>`、`<signal.h>`（进程内的一半：`signal` 记录、`raise` 调用，SIG_DFL 以 128+n 退出；**没有外部信号会到达**，写明而不是假装）、`<time.h>`（类型与 `difftime`；`time`/`clock` 需要 catalog 在每个目标上都没有的系统调用——macOS 没有 clock_gettime，A2 审计发现过冒名的号——所以**不声明**，调用它的程序在编译期被拒而不是拿到编造的时间）。`<setjmp.h>` 评估结论：tape 没有间接跳转也读不到 arm64 的 LR，需要新的 tape 操作族并在两个后端六个目标上 lower，**不做** | ✅ 七个 c99 探针（59、5a–5f），两个前端都与 cc 一致 |
| D2 | **函数** —— **部分已达**（2026-09-24）：`qsort`（堆排序，最坏情况有界）`bsearch strtok strncat sscanf`（d i u x o c s f e g、h/l、宽度、`*`、`%n`）`perror strerror atexit labs llabs div ldiv`。`atexit` 要求 main 返回也算 `exit`：两个前端的入口存根在 `<stdlib.h>` 定义了 `exit` 时经它返回（C 前端把 `_start` 的收尾挪到所有单元走完之后才知道有没有）。**顺带**：第一次写文件作用域的函数指针数组（atexit 的表）就发现 C 前端把它当一个 8 字节标量按字节索引——`tests/c/b_fparr.c` 固定。**未做**：`fseek ftell remove rename` 需要 catalog 新增 `lseek unlink rename` 三行（三列号 + Windows 门的参数编组）并重建权重，`getenv` 需要 envp 到达程序——都留到 0.0.8，写在下方 | ✅ 每个已做函数有探针且对 cc 一致；tools 11/11 不退 |

**E. 文档与论文（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| E1 | 论文：刷新数字（corpus 216、C99 48/48、速度）；新增一节"**gold 本身会错**"——[G-2] 的来龙去脉与 A1 的审计结果，这是对 §8 所述局限的第一次实证回应 | 论文数字与 `all.sh` 当次输出一致 |
| E2 | 论文结果表改为生成区，像 [A-48] 那样由 `docs.sh` 检查 | 表过期即失败 |
| E3 | prd §2 补**出货编译器**的 CLI 契约（现在只写了 Python 驱动） | 每个 flag 一行，与 `cli` 套件一一对应 |

**F. 发布与分发（P1，含主人决定）**

| # | 事项 | 完成判据 |
|---|---|---|
| F1 | `make release` 自动起停两台虚机（现在要手动，而发布门要求零跳过） | 一条命令从起机到关机，失败也会关机 |
| F2 | **签名策略——主人决定**。Windows 直接执行 `.com` 里的 PE，未签名会触发 SmartScreen；组织的 Azure Artifact Signing 支持 APE `.com`。macOS 的切片是解到临时目录再跑的，不带下载隔离标记 | 本项只交付选项与代价；签不签由仓库主人定 |

**G. C 扩展（P2）**

| # | 事项 | 需要什么 | 完成判据 |
|---|---|---|---|
| G1 | `_Generic`（00219） | 类型系统区分指针目标的限定符（`const int *` 不可匹配 `int *`）与 `char`/`signed char`/`unsigned char` | 00219 通过，两个前端 |
| G2 | GNU 语句表达式（00213/00214） | 块作表达式求值，含块内标签与 goto | 两个都通过 |
| G3 | 初始化器（00216） | tcc 的初始化器酷刑测试 | corpus 220/220 |

**H. 优化器 `-O1`/`-O2`（P1，主人 2026-09-25 决定做，推翻下文第 11 项的“只量不做”）**

对标 cc -O2。路线不变：**表状的优化决策走构造的网络**（窥孔：tape 指令窗口 → 改写类别，新 gold 表 `peep`，两个前端都问），结构性部分（栈顶缓存、寄存器分配、帧布局）是经典代码。优化是 tape → tape 的改写，所以 Python 后端与 C 后端吃同一条优化后的 tape，closure 逐字节判据照旧成立。

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| H0 | **基线** —— 出货二进制编译自己 / cc -O2 构建编译自己 | 不量不算 | bench 记录：2026-09-25 为 1.0 s / 0.09 s ≈ 11× |
| H1 | **栈顶缓存** —— **第一步已达**（2026-09-25）：`-O`/`-O1`/`-O2` 生效（`-O0` 与缺省的 tape 与此前逐字节相同）。C 前端在所有单元走完后做 tape → tape 改写：`.frame 8; store64 [r7+0], rX; S; load64 rY, [r7+0]; .frame -8`，当 S 是只含显式操作数的直线代码（21 个操作的白名单）且不提 rY、r7 时，改为 `mov rY, rX; S`（S 空且 X=Y 时整对删去），迭代到不动点。两个后端吃同一条优化后的 tape，closure 照旧。实测：编译器镜像 **941,346 → 759,714 B（−19%）**，-O2 编出的编译器编译自己 **1.04 → 0.54 s**，写出的编译器与参考逐字节相同，-O2 下自身不动点成立；新套件 `opt`（31 s）：314 个探针/c-testsuite/c99 程序 -O0 与 -O2 运行一致、-O2 closure 在三个目标上逐字节、自编译与不动点——653/0 | 杠杆最大 | text −25%；bench 比值 ≤ 6×（现约 6×：0.54 s / 0.09 s） |
| H2 | **`peep` 表**：load-after-store、常量传播到立即数、跳到跳转、死存储；构造权重、全域枚举 1.000 | 表状决策，按路线走网络 | 枚举 1.000；两个前端同问；closure 0 差异 |
| H3 | **`-O0/-O1/-O2` 旗标生效**（今天接受但忽略） | 对接 Makefile 习惯 | 每级 closure、nativeboot N1=N2=N3、difftest 对 cc -O2 输出一致 |
| H4 | 叶函数内联、强度削减 | 余下的差距 | bench 比值 ≤ 3× |

**明确不做**：目标文件与链接器（多单元已由一个 walker 解决）；训练（对照臂）；C11/C23 中 G 组之外的特性。

**进度（2026-09-25 01:30，编译速度）**：B3 的速度一半已达。`make com` **182 s → 17 s**：Python 后端两处 O(n·m)（`size()` 每条指令重建 5k 标签字典、`lower()` 每个 pc 扫全部标签），镜像逐字节不变。出货二进制编译自己 **2.5 s → 1.0 s**、cc -O2 构建 **264 → 90 ms**：采样定位 93% 在 `vfind`/`srcfind`/`sfind` 三个线性查找（`V_NLISTS` 16 小于实际的 18 个词表，多出的永远走线性），改为哈希，六目标镜像、全部探针与 220 个 c-testsuite 程序对旧编译器逐字节相同，gen2 = gen1。验证：closure 564/0、stages/selfhost 94、native 94/0、nativeboot、bigclosure 6/0、ape 3/0。**套件慢与发热的主因不是编译器**：每个新写出的二进制首次执行被 XProtect 扫描 0.5–0.9 s（`XprotectService` ~35% CPU），编译一个探针只要 0.01 s；十三个套件逐探针写-签-执行新文件。对策（`-run` 取代落盘执行、或主人把终端加入“开发者工具”）待定，见 AGENTS.md。
**进度（2026-09-25 02:30，60 秒上限）**：单次运行不超过 60 s。`corpus` 加 `SHARD=k/n`，all.sh 与 `make corpus` 分四片跑，每片约 20 s，合计 216 pass / 4 knownfail 与基线一致（一片不能改基线，但会报它那一份里丢掉的程序）。`fuzz` 改为 par.sh 并行，cc 的参考输出按“源码 + cc 版本”的哈希缓存：热启动 13 s，160/160；冷启动一次性超过 60 s。c99 55、warn 4/0、tools 11/11 重跑为绿。
**进度（2026-09-25 02:00，两个出货 bug）**：把 closure 的宿主执行改成 `-run`（省掉一次 XProtect 扫描）时抓到 `b_libc` 在仓库外死循环——出货的 `unisacc.com` 同样。① 按需补头文件（autoinc）只试 `./include/`，仓库外什么都没补上；改为像 `#include` 一样退回到内嵌副本。② C 前端对**没有任何单元定义的函数调用**不报错，后端把缺失标签解析为偏移 0，程序跳回自己入口；Python 前端一直拒绝。现在所有单元走完后报 `unisacc: error: undefined function 'X'`（链接器形制，调用单元的文本已不在）。`cli` 加两项（仓库外无 #include 调 libc；未定义函数被拒），旧编译器上两项都失败。369 个探针/c-testsuite 程序退出码与镜像不变；multi、diag、cli 24/0、hostile、closure 564/0、stages、selfhost、nativeboot、bigclosure 全绿。规则：单次运行上限 60 s（AGENTS.md）。
**进度（2026-09-25 00:31，机器过热中断处）**：A1 A2 C1 C2 E3 B1 A3 D1 D2 C3 C4 已达；未做 B3（重测体积/速度）、B2（.com Windows 自解压）、E1 E2（论文）、F1（make release 自动起停 VM）、G1–G3（_Generic / 语句表达式 / 初始化器）、D2 剩余的 fseek/ftell/remove/rename/getenv（需 catalog 加三个 syscall 行）。
**C4 提交时的验证状态**（最后一次完整 `all.sh` 被中断两次，机器过热）：C4 之后逐个跑过且绿的——c99 55/55、fuzz 160/160、tools 11/11、corpus 216/220（4 knownfail）、diag 14/14、hostile 21/21、warn 4/4 + 语料假阳性 0、layout 4572/0、datashape 12/0；**未在最终树上重跑**的——closure/stages/selfhost/ccrun/native（探针套件）、bigclosure、nativeboot、acceptance、selfgap 汇总行。C4 顺带改了五处类型事实（`*p` 宽度、`?:`、`&&`、枚举常量、`[1]` 成员），这些正是探针套件与 bigclosure 会判的东西——**下次开工第一件事：`./tests/all.sh`（约 14 分钟，跑完才输出）**，绿了再动 B3。

**顺序**：A1（便宜、直接回应论文局限）→ C1（便宜、摩擦最大）→ B1（杠杆最大、风险最大，尽早做以便充分回归）→ A3 → D → C3/C4 → B2 → E → G。F2 等主人决定，不阻塞其余。

**S-14 发布门。** 发布曾是一串靠人记住的动作：跑套件、留意 crossnative 是不是悄悄跳过了目标、
构建 `.com`、试跑、推送、打标签。可检查的部分现在由 `tests/release.sh` 检查：树已提交（脏树里
构建出来的东西无法从它自称的标签重建）、二进制报得出自己的版本、`STRICT=1` 下全部套件绿、
**没有任何套件被跳过**。它不推送、不打标签、不上传——那些是人来决定的部分。

**S-10 0.0.6 计划**（2026-09-23 定，承 0.0.5）。按"离判据多远 ÷ 成本"排，每项都带一条可测的完成判据——
没有判据的条目不进这张表。

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| 1 | **体积：x86 rel8 短跳转** —— **已实现，判据未达，已如实改写**（2026-09-24）。两个后端同一套松弛轮次，逐字节一致（closure 540/540、bigclosure 6/6）；编译自身不变慢（227 ms）。但只省了编译器自身 x86 text 的 **1.2%**，不是计划写的 ≥5%——**估算错了**：unisacc.c 里可松弛的跳转只有 5,219 条。直方图显示 text 的真正大头是 tape 栈机的 push/pop：`.frame` 占 20.8%，紧随的 `load64/store64` 再占 16%——每次 push 是 `sub r10,8; mov [r10],rax` 共 7 字节。对应的杠杆写进 0.0.7（见下） | rel8 ✅；≥5% 改由 0.0.7 的 regmap 方案承担 |
| 2 | **体积：切片间共享** —— **已测量，判据按实测改写**（2026-09-24）。四个 Unix 切片：gzip 各自（现状）504 KB；bzip2 各自 417 KB（−17%）；**xz 放一个流 220 KB（−56%，这才是真正的共享）**——但 **macOS 不自带 xz**，bzip2 在精简 Linux 上也不保证有，换压缩器等于让一部分机器跑不了 `.com`，不做。更根本的是：`.com` 约 62% 是**未压缩的 PE**（Windows 原地执行，不能压），所以只动 Unix 切片整体最多降 6–7%，≥15% 这条从一开始就到不了。真正的办法写进 0.0.7 | 不做可移植性倒退 ✅；≥15% 改由 0.0.7 的自解压存根承担 |
| 3 | **扩表：GNU 语句表达式与 `_Generic`** —— **重新划界**（2026-09-24）。做到的：corpus 214 → **216/220**。00200 修的是 **gold 本身**：有符号整数运算的结果类型规则是 [G-2] 的旧捷径（`int + int` 是 long），按 C99 6.3.1.8 改正后重建权重、枚举重验全 1.000；00206 暴露的是**两个前端都不按位置处理宏定义**（C89 行为，`#undef` 在 C 前端根本没实现），已修。挪到 0.0.7 的三项、及各自真正需要什么：**00219 `_Generic`** 要类型系统区分**指针目标的限定符**（`const int *` 不可匹配 `int *`）与 `char`/`signed char`/`unsigned char`——横跨两个 walker 的类型改造；**00213/00214 语句表达式**要两个 walker 都能把块当表达式求值（含块内标签与 goto）；**00216** 是 tcc 的初始化器酷刑测试（花括号省略、多余花括号、混合指定初始化，282 行）。三项都在 C99 之外，各是一块独立的大活，挤进 0.0.6 会把体积两项挤掉 | corpus 216/220 ✅；余下 4 个写进 0.0.7 |
| 4 | **Python 前端的诊断追平 C 前端** | `CError` 仍然报它自己缓冲区的行号，而 C 前端已经能给出用户文件的行列与源码行（[S-12]）。两个前端的报错质量不对等 | `diag.sh` 的五个用例对 Python 前端也成立 |
| 5 | **`prd.tree.md` / `prd.map.md` 的生成器** —— **已达**（2026-09-24）：`python3 -m unisa docs`，三份文档的阶段表为生成区，[A-48] 检查 | 两份文件全靠手抄，11→14 那次重构之后它们落后了一整天而没有任何机制发现。阶段表在四个文件里各存一份 | 一条命令从 `unisa/gold.py` 生成这两份文件里的表；套件检查它们与 `unisa acc` 一致（像 [A-43] 检查常量那样） |
| 6 | **后端常量的检查面扩大** —— **已达**（2026-09-24）：[A-43] 从 17 个数扩到 30 个，覆盖三种镜像写出器的基址、页、头长与导入数；C 侧用十进制写（`5368709120` 即 `pe.IMAGEBASE`），正是肉眼最看不出漂移的地方 | [A-43] 现在核 17 个数。镜像头部、段对齐、PE 的 `SizeOfImage` 这类两边各写一遍的数字还没进去 | `consts_check.py` 覆盖三种镜像写出器里所有两侧重复的常量 |
| 7 | **`-S`（发射汇编）与 `-c` 的语义对齐** —— **已决定并落地**（2026-09-24）：`-S` 写 tape（tape 就是本编译器汇编层的 IR，与 gcc 的 `-S` 同级）；`-c` 保留为同义词，因为套件一直这么写，但 usage 行明说"没有目标文件"。不做目标文件：多单元已由一个 walker 解决（[A-30]） | 现在 `-c` 是"写 tape"，而 gcc/tcc 的 `-c` 是"写目标文件"。这个不一致会绊到把 unisacc 塞进 Makefile 的人 | 要么改名（`-c` → `--tape`）并让 `-c` 报一条明确的"没有链接器"的错，要么实现目标文件；**先写清楚选哪条** |
| 8 | **第二意见：GitHub CI 跑起来** —— **已达**（2026-09-24）：`fff9d7d` 起 Linux + macOS-14 + macOS-15 三个 job 全绿。它**确实是第二意见**：本机全绿时 CI 抓到了四个本机看不见的问题——oracle 缓存比对哈希而非问题（clang/gcc 对有符号溢出 UB 的不同处理让错答案挪位置）、复合字面量初始化的指针只存了 4 字节（栈上碰巧对）、glibc 导出真实的 `__mmap` 让参照程序崩溃、旧版 clang 拒绝 `case` 后直接跟声明。仍然**不产出任何交付物** | ✅ |
| 9 | **C99 补到 47/47** —— **已达**（2026-09-24）：`c99.knownfail` 已空 | 这是**初心那一条**："能编译并运行 C99 程序"。`corpus 214/220` 太宽容（那批程序又短又重叠）；[A-44] 按标准自己的变更清单建了 47 个特性探针，今天 **35/47 = 74%**。12 个缺口全部可定位、可枚举 | `c99.sh` 47/47，且 `difftest`、`corpus`、`closure` 不退 |
| 10 | **编译速度第二轮** —— **已达**（2026-09-23）：自编译 467 → 218 ms | 第一轮（2026-09-23）把 catalog 与词表改成一次性建索引，编译自己从 2.85 s 降到 **0.46 s**（6.2×，输出逐字节不变）。剖析里新的头部是 `lex`（168 样本）与 `infer`（45） | 编译自己 ≤0.25 s（cc -O2 构建），`bench` 棘轮不退；**输出必须逐字节不变** |
| 11 | **出货二进制的代码质量** | 同一份源码：cc -O2 编出的 unisacc 编译自己 0.46 s，**unisacc 自己编出的要 17.57 s** —— 慢 38×。这不是算法问题，是我们发的机器码没有窥孔与寄存器分配 | 先**量**，不急着做：`bench` 把这个比值记成一个数。要不要做优化器是一次范围决定，见下 |

**第 9 项的 12 个缺口**（`tests/c99/` 实测，2026-09-23）：

| 缺口 | 报错 | 性质 |
|---|---|---|
| `_Bool` / `<stdbool.h>` 的 `bool` | unknown identifier | **真语言缺口**，真实代码常用 |
| 逗号运算符 `(a++, b++, c)` | expected ')' | **真语言缺口** |
| `__func__` | unknown identifier | C99 要求的预定义标识符 |
| 柔性数组成员 `char d[]` | a constant is required here | C99 新增 |
| `##__VA_ARGS__`（空可变参数） | expected ')' | 实为 GNU 扩展，但普及到近乎必需 |
| `_Pragma` 运算符 | expected ')' | 冷门 |
| 十六进制浮点字面量 `0x1.8p1` | expected ';' | 冷门 |
| `<inttypes.h>` / `PRId64` | 头文件没带 | **只是少一个头文件** |
| `va_copy` | 缺 | C99 新增 |
| `strtod` 家族 | 缺 | 库 |
| 通用字符名标识符 `caf\u00e9` | 输出不符 | 冷门 |
| 宽字符相关 | 部分 | 冷门 |

**第 11 项是一次范围决定，不是技术债。** 做优化器意味着引入一整层"结构性代码"，而它对论点没有增量价值——论点是"表状决策可被构造的网络替代"，窥孔优化不是表状决策。建议：**只量不做**，把 38× 这个比值写进 `bench` 当作公开的已知代价。

不在 0.0.6 里、且要说清楚为什么：
- **训练**不在任何一条上——它是对照臂（[U-5]），碰它只会发热。
- **目标文件格式与链接器**不做：多翻译单元已经用"一个 walker 走多个单元"解决了（[A-30]），做 ELF 可重定位目标文件是另一个项目。
- **提交历史里的邮箱**（99 个提交带本机主机名）要不要重写，是仓库主人的决定，不是技术债。

**S-9 仪表要能证伪自己。** 一个套件如果只比对答案，它证明不了答案是**被使用**的
（[A-36] 的由来，见 E-57）；一个仪表如果自己崩溃却把崩溃算成通过，它比没有还糟——所以
`ablate.sh` 开跑前先自证，`all.sh` 用每个套件的**退出码**而不是最后一行文本定胜负。

**S-8 每一项都要落成棘轮。** 像 `corpus.baseline`、`selfgap.baseline` 那样只能升不能降，于是"完成度"不再是一个拍脑袋的百分比，而是套件里的数字。**绿色不等于覆盖**——一个套件只能证明它真的问过的问题，见 E-43。

---

---

# 第五部分 · 知识沉淀

## 6. 实验发现 [E] —— 面向论文

本章随实现推进累积。**只记实测，不记预期**；每条含可复现命令，供论文直接引用。

### 6.0 形式结果总表

先看这张表：**哪些已经证明、哪些已被自己证伪、哪些还开放**。细节见对应条目。

| 命题 | 状态 | 依据 |
|---|---|---|
| **P-8** 任意有限积上的全函数，存在精确实现它的 kernel 权重 | **已证明**（构造式，11/11 机器验证）。**但这是已知结果**，不是本文贡献 —— [R-1] | §1.4 · E-19 · Omlin & Giles'96 · Tracr'23 |
| **P-3** `net ≡ gold` 可判定 | **已证明**（全域枚举，6,650 key，0 分歧） | E-5 |
| **P-4** 量化等价可判定 | **已证明**（全域枚举，含 K-3 整数算术） | E-6′ |
| **P-5** 组合等价 `C[N] ≡ C[O]` | **已证明**（由 P-3 + P-1 同余，无需对 C 归纳） | §1.4 |
| **P-7** 目标等价 | **实证**（7 例 × 6 目标全同；三种故障注入均退化到 4/6） | E-17 |
| **P-2** key 全域性 | **未证明** —— 运行时无条件断言。**唯一需要对走查器做真推理的义务** | §1.4 · O-3 |
| **P-6** `gold ≡ C99` | **本实验不声称**。已九次由实现暴露缺陷（3 次在 gold、6 次在走查器），而 acc 全程满分。外部语料给出可测数字：220 例 pass 214 / wrong 0 / unsupported 0 | E-2 · E-15 · E-16 · **E-30** |
| **P-8a** `h_min(f)` 的组合刻画 | **不是开放问题**：即（多值）两级逻辑最小化，NP-complete 且难近似 —— [R-1] | §1.4 · Masek'79 · Brayton et al.'84 |
| **P-8b** 可达性分离（精确解存在但 SGD 不可达） | **有实例**；但这是**已建立的定理家族**，我们只贡献真实编译器表上的实例 —— [R-1] | E-18（s2：12.8× 参数仍 0.8143）· Shalev-Shwartz et al.'17 |
| **P-8c** 是否存在 `O(h_min)` 构造算法 | **一般情形已被难近似结果排除** —— [R-1] | §1.4 · Allender et al. JCSS'08 |

**一句话**（[R-4] 修订后）：「能不能 100%」1996 年就关闭了，不是我们关的；「最小解有多小」是 1979 年就被证 NP-hard 的两级逻辑最小化。**本文真正主张的是工程与方法论**：把决策点刻意限制成有限离散全函数，于是"模型 + 纠错回退"这个结构可以被整个删掉，正确性义务从统计问题塌缩成模型检验问题——并在一个自举的六目标 C99 编译器上端到端成立。

自行证伪的预测：**E-2′**（1/19 降权）· **E-3′**（神经比表省空间，另见 [R-2]：Boniol et al. NFM'26 用 BDD 做到精确无损压缩）· **E-6**（margin 预测量化落点，降级）· **E-7**（q2 可幸存，证伪）· **E-8**（混合 dtype 收益显著，不成立）。

### 6.0.1 记录规范

每条发现固定五栏：**命题 / 证据（可复现命令）/ 机理 / 对论文的意义 / 状态**。
状态取值：`已证实` · `待验证` · `已证伪` · `开放`。
一条发现被证伪时**保留编号并标记**，不删除——证伪过程本身是结果。

### 6.1 已证实

> **读法（2026-09-23 加）**：每条实验记录的是**它当时**的口径。2026-09-22 之前的
> 条目说“11 个阶段 / 4,560 key”，此后是 14 个阶段 / 6,650 key（新增 `regmap`、
> `tyinfo`、`pfconv`，`isel` 退出 lowering，`abi` 去 `tls` 加 `nrreg` 与
> `arg3..arg5`）；探针数由 83 → 89 → 90。这些数字**不更新**：改了就不再是当时的测量。
> 当前事实看 §3.0、§5.5 与 `python3 -m unisa acc`。


#### E-37　无符号语义：表涨到两倍，构造照样精确，训练掉到 0.9514

| 栏 | 内容 |
|---|---|
| **命题** | 给子集补上 C99 的无符号整数语义。这是继 `short`（E-31）之后第二次"扩表"，而且这次扩得更狠：`TYS` 从 9 涨到 13，`type` 表从 **1,539 行涨到 3,211 行**，机器操作码从 43 涨到 46（`ult64`/`ule64`/`lshr64`），`isel`/`enc`/`abi`/`irsel` 跟着一起涨 |
| **构造 vs 训练（本条的重点）** | **构造路线**：改词表 + 一条转换规则，重新构造 **3 秒**，11/11 仍全域精确，C kernel 自检 **8,792 决策 0 错**，UNS2 从 4,043 B 到 4,463 B。<br>**训练路线**：同一张表，原宽度 h=20 直接掉到 **acc 0.9514**，加宽到 **h=32** 才回到 1.000（θ 1,006 → 1,622）。<br>接上 E-31（1,216→1,539 行时 h 要 16→20），两个点连起来是同一句话：**扩表时构造的成本是确定的，训练的成本是要重新调容量的** |
| **为此补的经典代码** | 无符号比较/移位/除法三个机器操作；窄无符号的**零扩展**装载与强制转换（`.ld` 是符号扩展的，对 `unsigned char` 是错的）；**按共同类型宽度回绕**——C 说运算前要先转换操作数，不这么做 `(int)-1 != 0xffffffffu` 会算成真；以及 **C99 6.4.4.1 的字面量定型**（十六进制常量的候选表里有无符号类型，所以 `0xffffffff` 是 `unsigned int` 而 `4294967295` 是 `long`）|
| **证据** | 外部语料 00104（`~x` 与 `0xffffffff` 比较）此前被记为 knownfail，现在**通过并已从清单删除**；`tests/c/b_unsigned2.c` 与系统编译器逐字节一致 |
| **意义** | 对论文：**扩语言 = 扩表 + 重新构造**这件事第二次被量到，而且这次表翻倍。红线 [F-5]（acc 低于 0.85 = key 编码错了，不准加宽）在这里**不适用**：0.9514 远在 0.85 之上，定义域是真的翻倍了，加宽是容量决策而不是掩盖编码缺陷——这两种情形必须分清楚 |
| **状态** | **已证实**（2026-09-19）|

#### E-46　语料从一个仓扩到三个：又四个缺陷，三个还是静默的

| 栏 | 内容 |
|---|---|
| **做了什么** | `tests/tools.sh` 加入 **tiny-AES-c** 与 **tiny-regex-c**（都是 kokke 的，公有领域，整数运算），十一个条目。加进去当天：`pass 8 / 11` |
| **① `uint8_t` 是有符号的** | `include/stdint.h` 里写着 `typedef char uint8_t;`，注释还振振有词地说"无符号拼写宽度相同，反正子集里算术都是有符号的"。**那句话对 `uint8_t` 恰恰是错的**：它的全部意义就是 0..255。于是 `printf("%.2x", b)` 打出 `ffffffffffffffa2` —— 加载时做了符号扩展。tiny-AES-c 从头到尾都是 `uint8_t`，一个 typedef 就是一整块 AES 与一屏幕 f 的差别 |
| **② 整数转换的精度被忽略** | C99 7.19.6.1p5：`d i o u x X` 的**精度是最少位数**，`%.2x` 的 3 是 `"03"`。我们的 `printf` 脱糖只把精度用在 `%s` 上，于是十六进制转储少了前导零。现在整数精度按零填充实现；`%5.2x` 那种**字段宽于精度**的组合需要零的外面再套空格，脱糖发不出来，**明确报错而不是悄悄少打**  |
| **③ 括号会毁掉左值** | `(*p)++`、`(x) = 1` 是普通 C，我们直接拒绝：二元运算阶梯在**最内层**就 `load_if_lval()`，于是括号里的表达式出来就成了右值。`(*matchlength)++` 是 tiny-regex-c 的半壁江山。改法是在括号里**先试一个 unary**，若紧跟 `)` 且确实是左值就直接用，否则回滚发射器再走完整表达式 |
| **④ 结构体传值踩了参数寄存器** | 被调方把调用者的对象拷进自己的槽（[W-14]），而 `blockcopy` 是拿 r0-r2 做的 —— **那时候后面几个参数还坐在 r1/r2 里**。`f(R a, R b)` 拷 `a` 的时候把 `b` 的寄存器冲了，于是 `b` 变成了 `a` 的第二份拷贝。改成**先把所有参数落到帧里，再统一做拷贝** |
| **结果** | `tools 11   pass 11   wrong 0`。其中 regex1 是那个引擎自己的 **76/76**，tiny-aes 与 `cc` 逐字节相同 |
| **意义** | 三个仓、十一个条目、十个缺陷 —— 而**其中七个是错答案而不是崩溃**。真实代码之所以是好仪器，不在于它更复杂，而在于它**同时**压到类型系统、ABI、编码器和库这四层；我们自己写的探针每次只压一层 |
| **状态** | **已证实**（2026-09-21）|


#### E-48　自举追平第二程：struct、typedef，以及被它们逼出来的 int 宽度

| 栏 | 内容 |
|---|---|
| **结果** | 棘轮从 **probes 40 / corpus 128** 走到 **probes 47 / corpus 144**，`ccrun` **29 个全部编译成功、wrong 0、refused 0** —— 自举编译器现在能编译 ccrun 集合里的每一个探针，且结果与 Python 前端逐字一致。[A-23] 的不动点全程成立 |
| **struct 逼出了一个更根本的问题** | 结构体成员必须按**真正的 C 布局**摆（`int` 成员 4 字节、对齐 4），否则 `sizeof` 和任何指进去的指针都是错的。而 C 前端原来到处把 `int` 当 **8 字节**（数组每元素 8）。两套模型会在 `s.a` 与 `int *q` 之间直接打架：同一个 `int` 数组，一个按 4 走一个按 8 走。**所以真正要修的不是 struct，是宽度**：`declspec` 现在返回真实宽度，访存一律经 `eload`/`estore` 带宽度走。改完 bootstrap 不动点仍然成立（tape 从 30,332 行长到 34,054 行，带宽度的访存更啰嗦） |
| **typedef 的关键不在表** | C 侧原来完全没有 typedef。难点不是记名字，而是**问表之前要把 typedef 名投影成 `type`** —— 词法器跑在解析之前，第 10 行 typedef 的名字在它眼里就是普通标识符。Python 的 `tokclass()` 一直在做这件事，C 侧直接用了原始 token kind，于是 `myint x;` 落到文法的 `expr` 默认分支被当成表达式。补上同一个投影就通了。typedef 按块作用域，`tdfind` 倒着扫所以内层遮蔽外层 |
| **一并补的** | `.`/`->`（`->` 先 `loadval` 把指针的值当基址）、union（成员全在偏移 0，大小取最大）、`enum` 作为说明符（原来只在顶层认）、三元 `?:`（只求值一条臂，各自一个标签） |
| **状态** | **已证实**（2026-09-21）|


#### E-49　自举追平第三程：函数式宏，以及它下面压着的两个静默错误

| 栏 | 内容 |
|---|---|
| **函数式宏** | C 侧的预处理器原来每个宏只记**一个数值** —— 那对 `#if` 够用，对别的什么都不够：`#define STR "x"` 和 `#define MAX(a,b) ...` 是真实 C 用预处理器做的绝大部分事。现在 `#define` 把**替换列表原文**存进宏池，preprocess 与 lex 之间插一趟文本展开，做到不动点（上限 8 轮）。对象宏、带参宏、宏套宏、`#` 字符串化、`##` 粘贴、空宏、多语句宏体，全部与 `cc` 逐字一致 |
| **三个坑** | ① **函数式的判据是 `(` 紧贴名字** —— `#define A (x)` 是对象宏，体恰好以括号开头；② **数值探测要用自己的游标**：原来那段扫数字的代码把游标推过了数字，于是 `#define N 5` 存下的体是**空的**，最简单的宏反而是唯一坏掉的；③ **实参两端的空白不属于实参**：`##` 粘的是文本，`CAT(cat, ab)` 带着空格就成了 `cat ab`，粘贴静默地没发生。参数替换是**一趟扫完**，就是 E-45 在 Python 侧抓到的那条 |
| **printf 只有四种转换** | `%d %c %s %%`。长度修饰符 `%ld` 直接报错，`%x %X %o %u %i %p` 全无。补齐了；`__itoab`（进制转换）的 tape 原文是**从 Python 生成的 tape 里取的**，不是手抄 |
| **整数字面量的进制与后缀全算错** | `0xff → 7794`、`010 → 10`、`5L → 78`。四处各抄了一份 `v = v * 10 + (c - 48)`：十六进制把 `x` 当数字（`'x'-'0' = 72`），八进制当十进制，后缀 `L` 当数字（`'L'-'0' = 28`）。换成一个共用的 `numval()` |
| **C 侧也缺 phase 3** | 文本宏一上来就把它逼出来了：C 的预处理器把去注释留给了词法器，和 E-45 ① 在 Python 侧是**同一个 bug**。`b_shift.c` 开头那段块注释让整个宏展开跑偏，`SZ(...)` 根本没被认出来。补上 `decomment()` 之后，**词法器在 80 个探针上 differ 0** —— 两个前端第一次逐 token 完全一致 |
| **实参要先展开** | C99 6.10.3.1：实参在代入前**先完整宏展开**，除非它是 `#` 或 `##` 的操作数。少了这条，`XSTR(VER)` 字符串化的是 `VER` 而不是 `3`。实现成 `emitrange(from,to,depth)` 递归展开实参文本，深度设限 |
| **粘贴要断开 token** | `##` 粘出来的是**一个** token，body 里跟在它后面的是**另一个**。`#define Q(A,B) A ## B+` 用作 `Q(+,)3` 必须给 `+ +3` 而不是 `++3` —— 真正的预处理器在 token 上工作，粘不到一起；我们在文本上工作，所以要自己补一个空格 |
| **接受 ≠ 正确** | 补完这些之后 `b_shift.c` 能编译了，**但答案是错的**（`2 2 8` 对 `2 4 4`）—— C 侧的类型模型不做移位提升、不跟踪无符号。所以 `ccrun` 从一个窄集合（29 个探针）铺到**全部 80 个**，并加了 `ccrun.knownwrong`：`wrong` 必须为 0，"编译且答案一致"的个数只能升，6 个已知分歧各自写明原因。**被拒是缺口，答错是缺陷**，两者要分开记 |
| **意义** | 静默算错不是拒绝编译 —— 一个 `0xff` 写进数组长度就会悄悄给错大小。棘轮只量"接受了多少"，量不到这个；抓到它们的是**拿同一个程序在两个前端上对答案**（`ccrun`），这也是 [A-21] 存在的理由 |
| **结果** | 棘轮 **probes 47 → 51、corpus 144 → 152**；`ccrun` 铺到 80 个探针后 **45 编译且答案一致、wrong 0、knownwrong 6、refused 29**；**lexdiff 80/0**；[A-23] 不动点成立 |
| **状态** | **已证实**（2026-09-21）|


#### E-50　自举追平第四程：初始化器，以及「没写到的就是零」

| 栏 | 内容 |
|---|---|
| **结果** | `ccrun` **48 编译且答案一致**（45 → 48）、wrong 0、knownwrong 6、refused 26；棘轮 **probes 51 → 54、corpus 152 → 161**；[A-23] 不动点成立 |
| **`= {...}`** | 局部与全局同一套代码，区别只在基址怎么取名（全局的存储落在 `__init` 里）。数组、不定长数组（长度由初始化器给）、字符串（`decode` 不补 NUL，结尾那个字节要自己写）、结构体 |
| **没写到的元素是零** | C99 6.7.8p21。**先把整个对象清掉再写**就是这条规则的全部 —— tape 正好有 `.zero`。不清的话局部变量里是栈垃圾；而逐元素补零还得先知道哪些下标被跳过了，那是同一个问题绕了一圈 |
| **花括号省略** | 6.7.8p17：聚合元素**从平铺列表里取自己那一份**，`PT a[] = {1,2,3, 4,5,6}` 是**两个** PT 不是六个。实现成一张**标量槽位映射**：第 i 个标量落在「元素下标 × 元素大小 + 成员偏移 + 成员内子下标」。同一张表也修正了不定长数组的长度推断 —— 叶子数是**标量数**，元素数要除以每元素的标量数 |
| **二维数组** | `a[n][m]` 是 n×m 个元素连排，**第一个下标跨一整行**；`a[i]` 的值是一个地址（行退化成指针），第二个下标才按元素宽度走 |
| **顺带** | `sizeof a[0]` 一直是错的：下标分支没更新 `cursize`，于是 `sizeof plain / sizeof plain[0]` 算出 1。这种错只有**对答案**才看得见，棘轮看不见 |
| **状态** | **已证实**（2026-09-21）|


#### E-61　抽样全绿，编译器自己六个目标全崩

| 栏 | 内容 |
|---|---|
| **现象** | `unisacc unisacc.c -b <任意目标>` 段错误，六个目标无一幸免；而 90 个探针的 540/540 闭环、220 个外部语料、`nativeboot` 全是绿的。唯一的区别是规模：707 KB、1,874 个数据符号 |
| **根因** | 三层。(1) `bk_repack` 假设数据符号按地址升序登记——`lower.zero_last` 写的是 `sorted(set(...))`，C 移植漏掉了 sorted；unisacc 自己第 282 个符号处顺序开始回退，遍历直接读出数组。(2) 同一符号登记两次时，第一次改写地址后第二次就查不到。(3) 真正的触发者是 `-o` 那次改动把 `bkfd` 在两个半边各定义一次，前端于是发出两行 `.bss g_bkfd` |
| **更深的一层** | `Tape.string` 是**驻留**：同名第二次既不分配也不改地址。C 侧 `.bss` 却重新分配并移动符号，两边镜像差 8 字节。这条是枚举套件 [A-41] 上线当天抓到的，不是人看出来的 |
| **方法论** | 第 3.3 节对模型用的判据（枚举整个定义域而非抽样）同样适用于手写的纯函数，条件是存在独立实现作差分对照。据此补了 [A-41]、[A-42]，并给十二个套件加上"检查数必须为正"——此前不带参数的 `closure.sh` 会打印 `identical 0 differ 0` 然后以 0 退出 |
| **状态** | **已修复并已验收**（2026-09-23）：`layout 4572/0`、`datashape 12/0`、`bigclosure 6/6`，全量 25 个套件绿，wall 647s |


#### E-60　Windows 的 `-run`：三件只有真机才会告诉你的事

| 栏 | 内容 |
|---|---|
| **结果** | `-run` 在四个平台、六个目标上都成立：macOS/arm64、macOS/x86_64（Rosetta）、Linux/arm64、Windows/arm64、Windows/x86_64（真机实测）；lnx/x86_64 于 2026-09-23 在 Lima 的 x86_64 客机里补上真机执行，`native 90/0`、对 glibc 的 `difftest 88/0`。`tests/run.sh` 的 Windows 分支每次随套件跑 |
| **① 没有导入表** | 内存里的代码没有 PE 导入表，而它要调用 `WriteFile`。解法：编译器**读自己进程的导入表**——从自己的代码地址往下找到 `MZ`，走导入描述符，**按名字**取每个槽的地址（按名字，因为顺序只有在编译它的也是我们自己时才可信） |
| **② 2 GB 之外** | 第一版把代码和数据各映射一块，结果访问违例：网关用 rip 相对寻址调用导入槽，而两次 `VirtualAlloc` 可能相距超过 2 GB。改为**一整块**，导入槽放在代码段尾部 |
| **③ 指令缓存** | arm64 Windows 报非法指令：新写入的代码还在数据缓存里。改保护属性**不够**，必须显式 `FlushInstructionCache`；这一步做进了 `mprotect` 的网关（当前进程是伪句柄 -1，所以不必多导入一个函数） |
| **④ 真正的根因** | 上面三条之后仍然崩。真相是 C 后端的六参数网关**漏了 Windows 的寄存器保存/恢复**：一次 Win32 调用会破坏全部 tape 寄存器，**栈指针也在内**，返回后取指到垃圾地址。三参数的网关一直是对的，而六参数那条是新写的；没有任何探针在 Windows 上走过它，所以 closure 也比不出来——`b_mmap` 的 Windows 分支当时还在发假输出 |
| **教训** | 一个"为了让探针在所有目标上一致"而发假输出的分支，等于在那个目标上关掉了这个探针。现在 `b_mmap` 在 Windows 上真调用 `VirtualAlloc`/`VirtualProtect`/`VirtualFree`，返回值也按 POSIX 约定转换 |
| **状态** | **已证实**（2026-09-23）|

#### E-59　把编译器变成工具：自带头文件，与 Darwin 的进位标志

| 栏 | 内容 |
|---|---|
| **结果** | `unisacc -run FILE.c [args]` 并入本体（不再有单独的 unisaccrun），加上 `-I`、`-D`、shebang；11 个头文件（60,768 B）嵌进二进制。`tests/cli.sh` [A-39] 在**一个空目录**里验证这些：那里没有本仓库的任何文件，9/9 通过 |
| **为什么必须嵌** | 头文件原先按**当前工作目录**找 `include/`。一个发出去的单文件编译器根本没有那个目录，所以 `.com` 一离开仓库就报 `lex: bad char at 0`。现在顺序是：源文件旁边 → `-I` → `include/` → **自带的副本**，开发时改 `include/` 仍然立刻生效 |
| **[I-20] Darwin 的系统调用错误从不检查** | 追这个 bug 追出一条更重的：**macOS 用进位标志报告系统调用失败，返回的 errno 是正数**，而我们只判断返回值 `< 0`。于是 `open` 失败返回 2（ENOENT）被当成合法的文件描述符——自举出来的编译器于是去读 stderr。cc 编的版本看不到这个问题（它用的是 POSIX `open()`，返回 -1），所以它躲过了所有既有测试 |
| **修法** | osx 的网关在系统调用之后加两条指令：arm64 `b.cc +8` 跳过 `neg x0, x0`，x86-64 `jnc +3` 跳过 `neg rax`。于是调用方在哪个平台上看到的都是 `-errno`。两个后端同时改，closure 仍逐字节相同 |
| **教训** | 这个错误存在于**每一个 macOS 镜像的每一次失败的系统调用**里，而全部套件都是绿的——因为探针只走成功路径。"失败路径也要有探针"，`cli.sh` 里那条"文件不存在必须被诊断"就是为此 |
| **状态** | **已证实**（2026-09-23）|

#### E-58　重构的判据：产物逐字节不变

| 栏 | 内容 |
|---|---|
| **结果** | 把模型数据从生成的内核里拆出来（`unisa_core.c` 99 KB → 3 KB，自测 520 KB → 4 KB）、删掉死掉的 `tls` 事实、重写三份文档之后，`unisaccrun.com` 的 SHA-256 与重构前**完全相同**（`cecfbd46…c24646`，v0.0.1 与 v0.0.2 同一份字节） |
| **为什么值得记** | "重构不改变行为"通常只能靠测试套件间接说；而这里产物本身是确定性的，于是有了一个**直接判据**：重构之后产物必须逐字节不变。测试套件绿是必要条件，字节相同是更强的陈述——它连"输出相同但代码路径变了"都排除了 |
| **前提** | 编译产物必须是确定性的：没有时间戳、没有路径、没有随机顺序。我们的镜像本来就如此（closure 逐字节比对依赖同一性质），所以这个判据是免费的 |
| **反面** | 它只对**声称不改变行为**的改动成立。加功能时产物当然要变，那时判据回到套件与 closure |
| **状态** | **已证实**（2026-09-23，v0.0.2）|

#### E-57　问过，不等于听它的

| 栏 | 内容 |
|---|---|
| **结果** | 审计发现三个答案**被询问却从未被使用**：`isel` 的 `form`（os 感知的 `enc` 会覆盖它）、`isel` 的 `symbol`（没有任何编码器读）、`abi` 的 `tls`（没有任何前端发出 `tls_base`）。另有四张**有限表仍写死在代码里**：系统调用号寄存器、tape 寄存器→机器寄存器、类型的大小与符号性、printf 转换符→输出例程。`>` 与 `>=` 的操作数交换也由代码决定，而不是 irsel 的答案 |
| **处理** | 三个死答案：lowering 不再询问 `isel`，`tls` 头连同它的事实一起删除。四张表进网络：`nrreg` 成为 abi 的输出头，新增 `regmap`、`tyinfo`、`pfconv` 三个阶段。交换改由 irsel 答 `_rev` 配方决定。阶段数 11 → 14，键 4,560 → 6,650 |
| **新仪表 [A-36]** | 旋转某个答案，要求镜像改变或编译被拒。这是 [A-33] 之上的一层：A-33 检查"问过"，A-36 检查"答案被用上"。旋转后全部 12 个在编译路径上的阶段/头都会改变输出 |
| **方法学教训** | 仪表第一版**在它自己的钩子里崩溃**（多头是三元组，我按二元组解包），而崩溃被当成"编译被拒"计入了"答案被用上"，于是 11 个头全部虚假通过。修正后脚本先**自证**：对每个待测项直接问 oracle，必须拿到不同的答案且不抛异常，之后编译被拒才算证据 |
| **顺带暴露的错** | ① C 内核对定义域外的键**静默返回一个类**（只有 Python 端有断言）；② `infer()` 的地址计算里有乘法，而文件头声明"无乘法"；③ `acc` 不检查并列，比 `build-weights` 的检查更弱；④ 训练默认 90 轮，而第 4 段学习率要 ≥90 轮才生效，等于永远用不到 |
| **状态** | **已证实**（2026-09-23）|

#### E-56　编译即运行、自签名，以及一个文件跑遍四个目标

| 栏 | 内容 |
|---|---|
| **结果** | `unisaccrun FILE.c [args]` 编译并运行，磁盘上不落任何文件：macOS/arm64 与 Linux/arm64 各 11/11 与系统 cc 同输出（[A-37]）。`unisaccrun.com` 一个文件在 macOS/arm64、macOS/x86_64（Rosetta）、Linux/arm64、Windows/arm64（x64 仿真）上都能跑（[A-38]） |
| **不是 JIT** | 整个程序一次性编译完成，没有热点探测、没有重编译；只是产物落在这个进程映射出来的内存里，而不是磁盘上。AOT 到内存 |
| **为什么必须在内存里** | 实测：Apple silicon 上把自己镜像里的静态数组 `mprotect` 成可执行**失败**（页属于已签名的镜像），而 `mmap` 出来的匿名内存改成可执行**成功**。走「临时文件 + exec」的话，未签名的 Mach-O 会被内核直接杀掉 |
| **六参数系统调用** | `mmap` 要六个参数而网关只传三个：abi 新增 `arg3/arg4/arg5` 三个输出头，tape 新增 `.sys6`，两个前端新增 `__mmap/__mprotect/__munmap`。VM 与目标机模型把 `mmap` 实现为自己内存里的 bump 分配 |
| **自签名** | Mach-O 写出器自己生成 ad-hoc 代码签名（SHA-256 分页哈希 + CodeDirectory + `LC_CODE_SIGNATURE`），两个后端逐字节相同，`codesign -v` 通过。**镜像不再需要任何外部工具就能在 Apple silicon 上运行** |
| **一个文件** | 头部字节同时是合法的 `MZ`（Windows 读 e_lfanew 找 PE）和合法的 sh 赋值（`MZqFpD='`），脚本按 `uname` 选切片、解出来执行。Windows on ARM 靠 x64 仿真覆盖。目前 4.74 MB：四个切片各自是完整镜像，尚未做共享或压缩 |
| **顺带暴露的错** | ① x86-64 上第 4 和第 6 个系统调用参数寄存器**正是** tape 的栈指针和帧指针，六参数调用会把程序的栈抽掉；② `BKNOPS` 写死 65 而操作码表已有 66 项，最后一个查不到；③ run 模式原先用寄存器传 argc/argv，这假设了**调用方**的调用约定——我们的和 C 的在 x86-64 上不同，改为由加载方写进程序自己的数据格；④ BSD 的 `tail` 把前导零的字节数当八进制，`.com` 的偏移必须是纯十进制；⑤ 文件第一行不能含 NUL，否则 shell 拒绝执行，所以头部字节放到第二行、仍在引号内 |
| **Windows（2026-09-23 补上）** | 见 E-60：四个平台六个目标的 `-run` 全部打通 |
| **尚未覆盖** | ~~lnx/x86_64 的原生运行仍缺一台 x86 Linux 机器~~ —— 2026-09-23 已补上：Lima 的 x86_64 客机（arm64 宿主上全模拟），`native 90/0`、对 glibc 的 `difftest 88/0` |
| **状态** | **已证实**（2026-09-23）|

#### E-55　自举闭环：镜像逐字节，以及「只有真机跑它自己编的东西」才看得见的错

| 栏 | 内容 |
|---|---|
| **结果** | [A-34] closure：90 个探针 × 6 个目标 **540/540 镜像与 Python 后端逐字节相同**，宿主镜像 90/90 与 VM 一致；[A-35] nativeboot：**N1 = N2 = N3**，交叉 5/5，osx/arm64 与 lnx/arm64；**在 Windows 真机上** win/arm64 与 win/x86_64（后者经 Windows 自带的仿真）的 unisacc 也各自重建出逐字节相同的自己。四个目标、三个操作系统上没有 Python 的自举。macOS、Linux 全绿 |
| **做法** | 后端是**移植**，不是重新设计：`lower.py`、`assemble.py`、`emit_arm.py`、`emit_x86.py` 与三个镜像写出器逐函数搬进 C，`catalog` 仍是唯一真源（`ckernel.py` 把后端词表连同模型一起发成 C）。零数据从不在内存里展开：`.bss` 只是一个长度，写出器流式输出 |
| **① 指针深度** | unisacc 把 `int **q` 的 `*q` 当 4 字节 int 读、指针数组按 4 字节一格排。解释器的地址恰好放得进 32 位，**只有原生镜像**会把高半截丢掉 |
| **② 静态局部变量** | unisacc 把 `static` 局部当普通局部：每次调用重新开始。解释器的栈是新的、恰好全零，看起来「保留」了值 |
| **③ lnx/arm64 的 argc 永远是 0** | `argsave` 按 Darwin 的交接读 x0/x1；Linux 把 argc 放在 `[sp]`。**两个后端都错**（它们逐字节相同，所以错也相同），因为此前没有一个 Linux arm64 探针看 argc —— 是原生 unisacc 在 Linux 上打出自己的 usage 才暴露 |
| **④ 目标宏** | unisacc 从不预定义 `__linux__` / `_WIN32`：`<stdio.h>` 在 Linux 用了 BSD 的 `O_*` 位（`fopen(…,"w")` 失败），在 Windows 走了 `open(2)`。现在 `-b` / `-t os/arch` 决定预定义，裸 tape 为 lnx/x86_64，与 `unisa/front/pp.py` 相同；`unisa vm --os` 让解释器按同一 OS 读系统调用参数 |
| **⑤ `main(argc, argv)` 从来没拿到参数** | 两个前端的 `_start` 都直接 `call main`，r0/r1 是 `__init` 留下的东西 —— unisacc 自己用 `__argc()` 内建，所以一直没人看见。现在 `_start` 用 `.argc`/`.argv` 建出数组再调用（两个前端同一段 stub）。追下去又是三处：osx/x86_64 的 `argsave` 按 Linux `_start` 从 `[rsp]` 读 argc，而 LC_MAIN 是被 dyld **调用**的，那里是返回地址；x86 的 `argvget` 把元素读进 r11 就停了，而且 REX 字节把 X 位也置上了；Windows 根本没有 argv —— 新的 `winargs` 在入口调 `GetCommandLineA` 并原地切分（引号、制表符），两个 ISA、两个后端同一段机器码。新探针 `b_argv` |
| **⑥ Windows 上打不开文件** | unisacc 读源文件用 `__open(path, 0)`，Windows 的 gate 是 CreateFileA（访问掩码 + 处置），`0` 等于什么权限都不要。`ropen()` 按 `_WIN32` 选参数，与 `<stdio.h>` 的 fopen 同一个选择 |
| **⑦ `LC_LOAD_DYLINKER` 的 cmdsize 是 28** | 64 位镜像要求 8 的倍数；内核放行，objdump 与 otool 的解析器不放行。改为 32，从头部 SLACK 里拿 4 字节，正文位置不动 |
| **⑧ 91 MB 的镜像** | unisacc 自己的镜像 91 MB，其中 10 页非零：零数据虽不在内存里展开，却照样写进了文件，因为它的表排在大缓冲区之后。现在两个后端同一条规则：非零块在前、全零块在后（各保持地址 mod 8），文件只存到最后一个非零字节 —— ELF `p_filesz < p_memsz`，Mach-O 一个 `S_ZEROFILL` 的 `__bss` 节（从头部的 SLACK 里拿 80 字节，正文位置不动），PE 的 cookie 本来就是零，随尾部一起零填充。**91 MB → 639 KB** |
| **状态** | **已证实**（2026-09-22）|

#### E-54　浮点：一整根轴，按构造加进每一张表

| 栏 | 内容 |
|---|---|
| **结果** | `corpus` **209 → 214**（五个浮点程序全过）、difftest 86、native/fat 87；自举前端 **ccrun 87/87 一致、拒绝 0**、语料接受 207 → 216；[A-23] 不动点成立；十一个阶段在新词表上枚举 **1.000**。macOS 与 Linux 全绿 |
| **表** | type 表的 TYS 加 f32/f64，按 6.3.1.8（宽的浮点胜出，整数遇浮点即成该浮点；`%` 与位运算没有浮点行）；lex 表把 `.` 分成独立的字符类，于是 `.5` 能起一个数而 `+5` 不能；irsel 加 `fpu` 族 24 条。全部**构造**、全部**枚举验证**，没有训练。type 表需要一个 256 的权重，C 内核的权重字段从一字节加到两字节 |
| **tape 上的表示** | 浮点值就是它的 IEEE 位模式，放在通用寄存器里。**调用约定两端都是我们自己的**，所以不需要任何平台的浮点 ABI —— c-testsuite 00204 那一大套「arm64 同构浮点聚合」的考题，对一个两端都由自己编的编译器不构成问题。`unisa/fp.py` 是这些 op 的**唯一定义**，VM 与目标解释器共用 |
| **printf 逐位一致** | 一个 double 是 m·2^e，所以它的值是**有限小数**：e≥0 时是 m·2^e，e<0 时恰是 m·5^k / 10^k。于是用 10^9 进制的大数把它精确展开，每一种转换都只是在一个数字串的一个位置上「四舍六入五成双」。**格式化代码里没有一条浮点运算**，数字不会依赖它自己是被谁编译的 |
| **常量也只舍入一次** | Python 侧 float 常量从十进制直接舍入到 24 位（经 double 会舍入两次）；自举侧没有 strtod，用整数大数：M·10^e，或 M·2^s / 10^k 带余数作粘滞位，**在结果实际具有的精度上**舍入一次（非规格化数也是）。与 Python `float()` 对拍 1264 例，零差异 |
| **顺带暴露的错** | ① `1 + (int *)p` 只前进一个字节：整数在左的指针加法从未缩放，而 type 表也**没有这一行**（6.5.6p2 两种顺序都允许）—— `<math.h>` 取高字就是这样取错的；② `struct s t = f();` 被当成花括号省略；③ `uint32_t` 函数返回了 64 位；④ `%lu` 被当成 32 位有符号展开；⑤ unisacc.c 随模型长过 256 KB，**被静默截断读入**；⑥ `en()` 打不出 LONG_MIN（正是 double 的符号位）；⑦ Python 的 `dec_to_f32` 超出 FLT_MAX 时抛异常而不是给无穷 |
| **一处棘轮下调** | 自举侧在拥有浮点之前**明确拒绝** float/double（它原先把 `double` 读成 4 字节 int，编得出来、答得不对）。selfgap 的语料基线因此 211 → 207：00113 00119 00140 00178 原本是**靠被编错**才「被接受」的。浮点落地后回到 216 |
| **状态** | **已证实**（2026-09-22）|


#### E-53　自举前端追平收官，以及只有「写一个没人写过的探针」才看得见的错

| 栏 | 内容 |
|---|---|
| **结果** | `ccrun` **83 编译且全部一致**、wrong 0、**knownwrong 0**、**refused 0**（上一程 61 / 2 / 17）；[A-33] stages 83 一致；unisacc 接受语料 **211/220**，覆盖 Python 前端通过的全部 209 个。macOS 与 Linux（Lima）**两边全绿**，[A-23] 不动点成立 |
| **补的语言面** | 结构体按值传递与返回、`(*fp)(x)` 与对任意值的调用、函数指针作为值（`*fp` 就是 `fp`）、嵌套声明符（返回函数指针的函数、函数指针数组、指向数组的指针、抽象声明符）、位域、VLA（块尾与 break/continue 归还栈）、复合字面量、按花括号层级的初始化器与指定符、匿名成员、块作用域的结构体标签、三维数组、宽字符串、真正的整数常量表达式（原来没有优先级：`N + 1 * 2` 算成 `(N + 1) * 2`）、libc 头文件按需引入 |
| **① 指针运算按字节走** | C 侧 `p++` 前进 **1 字节**、`*(p++)` 读 8 字节、`p += n` 不缩放、`p - q` 不除；Python 侧 `p - arr` 答字节数（数组操作数没有被当作它退化成的指针）。**两个前端各错一半，而且一个探针都没有抓到** —— 此前所有探针的指针运算都只作用在 `char *` 上，步长恰好是 1。新探针 `b_ptrarith` 对拍系统 `cc` |
| **② Python 前端的结构体布局不是平台的** | 成员按**大小**猜对齐：8 字节的 int 结构体、`int[3]` 都被放到 8 字节边界，平台 28 字节的结构体这里是 36。布局经由 `sizeof`、偏移与每一个指向结构体的指针都**可观测**。改用成员类型自己的对齐；新探针 `b_layout` 对拍 `cc` |
| **③ 局部聚合的部分初始化没有清零** | Python 前端对局部 `{...}` 从未执行 6.7.8p21。解释器的栈是新的、恰好全零，于是 difftest 永远看不见；原生镜像读到的是 `__init` 留下的东西。同一处还有两个引用了未定义名字的分支（走到就 NameError） |
| **④ 原生编码器没有 `.zero`** | 两个 ISA 都落到 `brk`/`ud2`。没人发现，因为 Python 前端从不对局部发它，而 unisacc 的 tape 只被解释 |
| **⑤ `__init` 缓冲区 64 KB 且不检查** | c-testsuite 00205 一个初始化器写 92 KB，越界写过符号表 —— **只在 Linux 上**表现为 `unknown identifier`（gcc 与 clang 的全局布局不同）。这是 AGENTS.md 里「macOS 绿不等于绿」的又一例 |
| **⑥ 两个前端都有 2^n 的解析** | `assign`/`expr` 先解析一元式看是不是赋值，不是就**回退重解析** —— 每层括号翻一倍。00200 在 Python 侧 16.7 s → 2.0 s，在 Linux VM 里 unisacc 14 s 超过 selfgap 的时限而被记为「拒绝」。改为把已解析的操作数交给条件表达式链的最左端 |
| **测试** | 整套从十分钟以上降到约 5 分钟：`built.json` 缓存路径原来**相对 cwd**，每个在临时目录里跑的探针都重建全部 11 个网络（约 7 s 满核），这是套件大部分时间与机器发热的来源；套件与逐文件工作并行；macOS 对新签名镜像首次启动的扫描（约 0.5 s，全系统排队）是剩下的下限 |
| **意义** | ①②③④ 全是**静默**的错答案，全部是写了「此前没人写过的那个探针」才出现的 —— 与 E-43「绿色不等于覆盖」同一件事。每个都有了对拍 `cc` 的探针，从此进棘轮 |
| **状态** | **已证实**（2026-09-22）|


#### E-52　偏移：自举编译器在变成一个普通编译器

| 栏 | 内容 |
|---|---|
| **发现** | 用户问了一句「你不会忘了我们是模型推理的 cc 吧」。实测：C 前端只问 **pp、lex、parse** 三个阶段；**type、scope、irsel 从来没问过** —— 模型 blob 里带着它们，Python 前端每次都问，而 C 侧是手写的 if 链。更糟的是自举追平那几程里我**又往上加了一层**：无符号怎么传播、运算结果多宽、`sizeof` 取多少（这是 `type` 表的活），除法/比较/右移选 `.udiv`/`ult64`/`lshr64`（这是 `irsel` 表的活），全写成了代码。结构体布局、宏替换、include 拼接、变参栈布局、初始化器走查属于 [T-1] 的经典代码，没问题；**类型推断和指令选择是表形状的决策，按 [T-2] 必须是网络** |
| **为什么没有仪表抓到** | 所有套件量的都是**答案**。一条与表一致的手写规则给出的答案和表一模一样，于是 `ccrun`、`difftest`、`corpus` 全绿 —— 偏移在答案层面是**不可见的**，只有去问「是谁做的决定」才看得见 |
| **irsel** | C 侧几百个字符串字面量里写着 tape 助记符。全部换成占位符 `@族.口味`（共 **239 处**），在唯一的出口 `es()` 里**当场问 irsel** 解析 —— 每条发出去的指令都经过一次推理，key 与 Python 的 `recipe(family, flavor)` 相同。没换的恰好是 `.div/.mod/.udiv/.umod/.sys/.argc/.argv` 与 `mov`，Python 侧这几条也是直接发的。改完 74 个探针的 tape **逐字节不变**：网络选出的正是原来写死的那条 |
| **type** | 二元运算的结果类型由网络回答，key 与 Python 相同（两侧操作数投影到 TYS 轴，运算符投影到规范的 TOPS 轴）；无符号与回绕宽度读 `+` 那一行（通常算术转换）。手写的无符号/宽度/sizeof 规则**删掉**。无符号、移位、sizeof 那批探针在网络接管后**全部仍然通过** |
| **Python 侧也有一份手抄** | `unsigned_result()` 把 type 表的规则又抄了一遍（注释自己写着「与 type 表同一规则」）。**穷举 169 个类型对，与网络答案全部一致** —— 于是删掉，`uns` 直接从网络读。一份与表一致的代码副本，正是这个项目不该有的东西 |
| **scope** | 最初两侧都是**问完就丢**。现在它**说了算**（2026-09-22）：声明的名字绑到哪里 —— `bind_global` 成数据标签、`bind_local`/`bind_param` 成帧槽 —— 由它的答案**选择**符号种类；`sizeof (` 后面是类型名还是表达式由它回答（删掉 C 侧 `is_typeat` 在这里的用法）；类型词、函数名、表达式里的标识符、成员名四处是**闸门**：答案不是期望的那个就报错退出，不再静默。`struct`/`union`/`enum` 在两侧都投影成 `type_kw`，否则表永远看不到 `sizeof(struct S)` 是类型。**闸门一装上就抓到一个 key 编码错**：c-testsuite 00129 把 typedef 名 `s` 又声明成局部变量和成员名，表对 `(local, typedef_id)` 回答 `type_name` —— 表没错，是 key 错：处在声明符或成员位置的名字**按位置就是名字**（C99 6.2.1p4 遮蔽），投影成 `id`。问完就丢的时候这个错是看不见的 |
| **仪表 [A-33]** | `unisacc FILE -c -v` 报出每个阶段被问的次数；`tests/stages.sh` 逐个探针检查：**Python 前端在这个探针上问过的每个阶段，C 前端也必须问过**。62 个一致，1 个已知（`b_libc`：Python 驱动按需补 `#include`，C 侧没有），17 个仍被拒 |
| **状态** | **已证实**（2026-09-22）|


#### E-51　自举追平第五程：#include、变参、无符号，以及五个一直潜伏的错

| 栏 | 内容 |
|---|---|
| **结果** | `ccrun` **58 编译且答案一致**（48 → 58）、wrong 0、**knownwrong 6 → 2**、refused 20；棘轮 **probes 54 → 60、corpus 161 → 176**；lexdiff 在**两侧都拼进头文件之后**仍 80/0；[A-23] 不动点成立 |
| **#include** | 文件**原地拼进**指令所在的位置，扫描从拼接点重新开始 —— 于是头文件自己的指令（首先是它的 include guard）由同一个循环、同一份宏状态处理。`"x"` 先在输入文件旁边找，再在 `include/`；`<x>` 只在 `include/`；我们不提供的头直接跳过，与 Python 驱动一致 |
| **变参与超过 6 个参数** | 被调方**先把整张参数表看完**再发射第一个字节：有 `...` 或超过 6 个参数，就所有实参都走 tape 栈，`arg[k]` 在 `[FP+16+8k]`。调用方把实参按源顺序压栈后就地翻转。`va_start`/`va_arg`/`va_end` 是三个内建。于是 **`<stdio.h>` 整个能被自举编译器编译了** —— 包括它用 C 写的 `_u_vfmt` 格式化器；带字段宽度的字面量 printf 改走它，两个前端从此用**同一份格式化代码** |
| **无符号** | 声明、typedef、成员、表达式各带一个 unsigned 位；窄无符号对象**零扩展**加载；unsigned int 及更宽（或指针）参与的 `/ % < <= > >= >>` 换成 `.udiv .umod ult64 ule64 lshr64`；两边都是 4 字节时先截到 32 位再比。十六进制/八进制常量装不进 int 时是 **unsigned int**（C99 6.4.4.1），`s == 0xffffffff` 因此为真 |
| **① `#ifndef` 双重取反** | pp 表的 key 是「名字**是否已定义**」，ifndef 的取反由**表**来做（`take if not on`）。C 侧自己又先反了一次 —— **每一个 include guard 块都被跳过**。unisacc.c 自己没有 guard，所以 bootstrap 从来没发现 |
| **② 字符常量从未被求值** | `'b'` 一直走十进制循环。`a_char.c` 能过只因为它没用字符常量；`<stdio.h>` 一拼进来，每个 `putchar('c')` 都写出 NUL |
| **③ 第六个参数被自己的 scratch 冲掉** | 参数落槽用 r5 当 scratch（注释写着 "r5 is free"），而 6 个参数时 r5 **就是**第六个。tape 接受负位移 `[r6-8]`，本来就不需要 scratch |
| **④ 64 位常量装不进编译器自己的 int** | int 宽度统一成 4 之后，自举编译器**自己的** `int v` 也是 4 字节，`0xffffffffff` 被截成 -1。`numval`/`en` 改成 `long` |
| **⑤ 我们自己 `_u_vfmt` 没有整数精度** | `%.2x` 的 0 该是 `00`。这是库的缺口，Python 侧的 `sprintf` 用的也是它 |
| **意义** | 五个全是**静默**的，而且全是把两个前端**放到同一份库代码上对答案**才暴露的。`#include` 本身不难；难的是它一接上，C 侧就第一次编译了几百行**不是为它写的**代码 |
| **状态** | **已证实**（2026-09-22）|


#### E-47　自举追平的第一程：switch、标签、sizeof、cast

| 栏 | 内容 |
|---|---|
| **做了什么** | 把 C 那侧的前端往 Python 那侧推：`switch`/`case`/`default`、标签与 `goto`、`sizeof`、类型转换。棘轮 [A-29] 从 **probes 31 / corpus 111** 走到 **probes 40 / corpus 128**，`ccrun` 从 22 个编译成功到 **25，wrong 0**，[A-23] 的不动点全程成立 |
| **bootstrap 比的是什么** | 动手之前先搞清楚了一件一直含糊的事：`bootstrap.sh` 的 B、C、U **三份 tape 全是 unisacc 自己产的**（分别由 cc 构建、由 B 构建、由 unisa 构建的那个 unisacc），Python 只负责把 tape 变成可执行。**两个前端的 tape 不必逐字节相同** —— 实测一个最小程序它们本来就不同。要求是**行为一致**，由 `ccrun` 量 |
| **switch 的形状** | 派发链发在**函数体之后**：case 标签要等body走完才知道。控制值先落到帧槽里，因为比较链要重新加载它，而 body 到那时已经把寄存器全用过了 |
| **顺手抓到的 Python bug** | 读代码准备移植时发现：`switch` 把自己的出口同时当成了 `continue` 的目标。C99 6.8.6.2p1 说 `continue` 属于**最近的迭代语句**，switch 只接管 `break`。于是 `for (...) { switch (x) { case 0: continue; } rest; }` 会去跑 `rest`。两侧一起修，探针 `b_switch3.c` |
| **sizeof 需要一个新字段** | C 那侧原来只记"元素宽度"（char 1，其余 8），而 `sizeof(int)` 是 **4** —— 存储宽度和类型大小是两件事，[G-2] 管的是**求值宽度**，跟这个无关。符号表因此多了一个"整个对象的字节数"，数组是 `n × 元素大小`，指针恒 8 |
| **cast** | `(TYPE)expr`：文法只看得见 `(`，是不是类型名是走查器的事。窄化是**可观测的**（`(char)300` 是 44），tape 有带宽度的 load/store，所以绕一次栈槽就够，回来的路上顺便符号扩展 |
| **状态** | **已证实**（2026-09-21）|


#### E-45　第一次跑别人的库代码：一天之内六个缺陷，四个是静默错误

| 栏 | 内容 |
|---|---|
| **做了什么** | 接入 `tests/tools.sh` [A-31]：Brad Conte 的 crypto-algorithms（公有领域、纯 C99、无浮点），八个算法各自 `.c` + `.h` + 自带已知答案测试，**每个都是多文件**——这是真实 C 的形状，而 c-testsuite 全是单文件。参照物是系统编译器编同一批源码 |
| **结果** | 第一次跑：`pass 2 / 8`。修完之后 `pass 8 / 8`。六个缺陷里**四个是错答案而不是崩溃** |
| **① 预处理相位** | C99 phase 3 把注释换成一个空格，**发生在 phase 4 处理指令之前**。我们把注释留给了词法器（phase 7），于是 `#define SHA256_BLOCK_SIZE 32   // 32 byte digest` 把注释吃进了替换列表，**每一处用到它的地方都把该行的后半截注释掉了** —— `BYTE hash[SHA256_BLOCK_SIZE] = {...}` 丢了 `]`，解析一路跑到文件尾。五个算法同时死在这上面 |
| **② 宏参数必须同时代入** | 我们是**逐个**代入的。`#define FF(a,b,c,d,...) { a += F(b,c,d); a = b + ROT(a,s); }` 配上 `FF(d,a,b,c,...)`：`a→d` 先把所有 `a` 变成 `d`，随后 `d→c` 又把它们全变成 `c`。MD5 正是这么写的，于是**本该写 b 和 d 的那些轮写到了 c**，摘要错，无任何诊断。改成一遍扫描、所有参数同时替换 |
| **③ arm64 的 imm9 是有符号的** | 见 [I-22]。`[fp, #-260]` 被 `& 0x1FF` 编成了 `[fp, #+252]` —— **不是截断，是改符号**，读到的是另一个局部变量。**任何帧超过 256 字节的函数在三个 arm64 目标上都是错的**，而 `char buf[1024]` 是 C 里最常见的局部变量之一 |
| **④ arm64 的 imm12 同样被掩码** | `.frame` 超过 4095 字节时 SP 被挪到错误的位置。一个 Blowfish key 是 4,168 字节的局部变量，于是它的帧盖在调用者头上，SIGBUS。**同一个教训的第二次**：字段装不下必须被发现，不能被掩码 |
| **⑤ 两处更小的** | `char s[] = {"abc"}` 是 C99 6.7.8p14 的合法写法（字符数组可由字符串字面量初始化，**外面的花括号可有可无**），我们当成了长度 1 的数组；多单元下 `called` 记的是调用点看得见的名字，而该文件的 static 随后被改名，于是程序在找 `memcpy` 而拿到的定义叫 `memcpy_u0`。另外 `unisa compile -I` 这个选项**收下了但从来没往下传** |
| **为什么以前看不见** | 我们自己的探针又小又规矩：没有一个的帧超过 256 字节，没有一个宏把参数名和实参名写成同一批字母，没有一个 `#define` 后面跟注释。而 **`--fold` 的六个目标一个都抓不到 ③ 和 ④** —— 解释器不建模寻址模式，它眼里 `[fp-260]` 就是 `fp-260`。[TP-6] 的又一次兑现：**解释器比真机宽容的每一处，都是一个迟早会爆的雷** |
| **意义** | 外部语料清零（`unsupported 0`）只说明**前端不拒绝**别人的代码，不说明**跑对**。真实库代码是第一个能同时压到预处理相位、宏代入语义和两条 arm64 立即数范围的仪器 |
| **状态** | **已证实**（2026-09-21）|


#### E-43　两个被“全绿”掩盖的洞：自举差距没有仪表，多翻译单元不需要链接器

| 栏 | 内容 |
|---|---|
| **起因** | 盘点完成度时量了一个**从来没量过的数**：同一份 unisacc（`cc` 构建）在外部语料上只接受 **111/220**，在我们自己的探针上 **31/73**——而 Python 前端是 209/220。它没有 switch、没有 struct/union 定义、没有 `sizeof`、没有标签与 `goto`、没有初始化器、printf 只认几个转换 |
| **为什么没人看见** | 三个仪表各自都对，合起来正好漏掉这件事：[A-23] `bootstrap.sh` 证的是**不动点** B=C=U，那是**自我复现**，不是覆盖率——unisacc.c 只需接受它自己用到的子集；[A-20] `selfhost.sh` 比的是**词法器**；`ccrun.sh` 遇到拒绝打印 `UNS` 就 `continue`，**不计失败**。于是三十次提交里前端特性单边堆在 Python 一侧，债**单向增长**而套件**始终全绿** |
| **办法** | `tests/selfgap.sh` [A-29] 把那两个数做成棘轮（基线 probes 31 / corpus 111），`ccrun` 的汇总行补上 `refused N`。**先上仪表，再动工**——否则每修一条都不知道自己在往哪个方向走 |
| **多翻译单元** | 原以为要目标文件格式加链接器。实际上**一样都不要**：走查器本来就**把调用推迟到单元结束才解析**（`called`），所以把 N 个文件交给**同一个 walker** 依次走查，跨文件调用走的就是跨行调用那条路。真正要处理的只有一条——**文件作用域 `static` 必须改名**，否则两个文件的同名 helper 互相覆盖；**单文件的后缀为空，tape 逐字节不变**，这条是硬要求，因为 [A-23] 要把 Python 前端的 tape 和 unisacc 自己的 tape 逐字节比 |
| **代价** | 作用域不分文件：前一个文件的 typedef 与 struct tag 在后一个文件里仍然可见。这是错的 C，明写在 [W-14] 里，等真实程序绊倒时再修 |
| **棘轮当天就抓到东西** | 多翻译单元那一改把 `saw_static` 的初始化漏在了 `declspec` 里，而 `enum E *e;` 走的是**不经过 `declspec`** 的那条路——语料 00209 当场从 pass 掉到 unsupported，`corpus.baseline` 的棘轮点名报了 `lost 00209`。仪表装上去几个小时就付了一次钱 |
| **意义** | 两件事是同一条教训：**绿色不等于覆盖**。一个套件只能证明它**真的问过**的问题；一个没有数字的维度，债会安静地长上三十次提交 |
| **状态** | **已证实**（2026-09-21）|


#### E-35　六个目标全部真跑起来了；win/* 卡住的两个根因，一个在头部字段，一个在寄存器壽命

| 栏 | 内容 |
|---|---|
| **结果** | `win/arm64` 与 `win/x86_64` 在 Windows 11 arm64 上真机执行，所有例子与探针输出与解释器逐字节一致（x86_64 跑在 Windows 自己的仿真层上）。加上 E-33/E-36，**六个目标全部真机执行过** |
| **根因一：`MajorSubsystemVersion`** | 我们声明了 **10.0**。这把加载器拨到严格路径，在那条路径上 DYNAMIC_BASE 的镜像被真的要求带可用的重定位，而我们的镜像宁可不要 ASLR 也不想造重定位表。改成 **4.0** 后，一个无 `.reloc`、无 load config 的镜像立刻加载并跑出正确结果。一行常量。[I-16] |
| **根因二：tape SP 是 volatile** | arm64 通了之后 x86_64 仍 `ACCESS_VIOLATION`。二分到 `winstdh`（取三个标准句柄）：它本来排在 `spinit` 之后，而它是**真调用**，**Win64 两个 ABI 里 tape SP 所在的寄存器（x86_64 的 r10）都是 volatile** —— 刚初始化完的 tape 栈指针被调用打成垃圾。改成 `winstdh` 在 `spinit` **之前**发，当场通过。[I-18] |
| **我们曾经推错的** | 之前从“拆真 exe”得出两条结论：**dir[5] 必须有真实重定位条目**、**dir[10] 的 `SecurityCookie` 必须非零**。两条实验本身没错，推论错了：拆的那些 exe 声明的子系统版本都 ≥ 6，所以它们本来就在严格路径上。**“从能跑的样本里拆掉 X → 它不跑了”只证明 X 在那个模式下必需，不证明它普遍必需。**这次我们花了约六十轮在错误的假设上（去造 `.reloc`、造 load config、对齐 cookie），而真正的判别性实验是另一个方向的：**拿一个 `csc.exe` 生成的 PE32+（子系统 4.0、2 节、无 `.reloc`、无 load config），它跑得好好的** —— 一个存在性反例把两条“必须”同时否掉 |
| **方法学的教训** | 拆解一个能跑的样本，只能发现**该模式内**的必要条件；要找“最小可加载镜像”，应该去找**尽可能简陋的真实样本**并模仿它。两个方向的成本相差一个数量级 |
| **调试装置** | UTM 里的 Windows 11 arm64 虚机 + `utmctl file push/pull` + `exec`，一轮约 30 秒。两个坑：`utmctl` **失败也返回 0**（必须看输出判断）；**在 cmd 还占着输出文件时去 pull，qemu-ga 会泄漏句柄、那个文件名此后永远读不了**（所以先 `copy` 再写哨兵，最后只 pull 副本）。现已接入 `tests/crossnative.sh`，虚机不在就跳过 |
| **意义** | E-32/E-33/E-34 说的是“解释器看不见真机的约束”；这一条多说一层：**真机环路建起来了也不够，实验的方向决定了你能学到什么**。六个目标全部真机执行后，“一份 tape → 六份镜像”不再有任何一格是只在解释器里成立的 |
| **状态** | **已证实**（2026-09-21）|


#### E-33　六个目标里，五个从未被执行过 —— lnx/x86_64 一跑就露出三个后端缺陷

| 栏 | 内容 |
|---|---|
| **命题** | [X-3]"只解释、不 execve"让 `--fold` 6/6 看起来像一个强判据。它不是。`tests/native.sh` 只能验**本机那一个**目标（开发机 = osx/arm64），于是 **lnx/x86_64 从未在任何地方被执行过**——直到 CI 与本地 Linux 虚机把它跑起来 |
| **三个缺陷** | ① **ELF 只有一个 `PF_R\|PF_X` 的 PT_LOAD**（[I-12]）—— 第一次写 scratch 就 SIGSEGV<br>② **`idiv` 用真 `push`/`pop` 保存 rax/rdx**（[I-13]）—— 而 `spinit` 把 tape SP 绑在 `rsp` 上，两个栈重叠：返回地址被踩，**每个打印整数的程序都死**<br>③ **x86 两操作数 ALU 的别名**（[I-14]）—— `mov dst,s1` 在 `dst == s2` 时先毁掉右操作数，`17-5` 变成 `17-17`；移位还顺手把 tape r4（`rcx`）永久冲掉 |
| **为什么都藏得住** | 三条都**不在解释器的机器模型里**：`exec_target.py` 不建模页保护、不建模真实 `rsp`、不建模两操作数指令。arm64 是三操作数、且宿主恰好是 arm64，于是全部绕开。`--fold` 比对的是**同一个解释器**跑六遍 lowering 的结果——它能抓 ABI 和 syscall 号错误（三次故障注入都退化到 4/6），**抓不到编码器缺陷** |
| **证据** | 修好后：`tests/crossnative.sh` 每次跑 **lnx/x86_64 47/47、lnx/arm64 47/47（真 Linux 内核）、osx/x86_64 47/47（Rosetta 2）**；CI 在 `ubuntu-latest` 上直接执行 ELF |
| **意义** | 对论文：**"六目标等价"这个主张的强度等于最弱的那个验证环节**。此前它是"六份 lowering 在同一个解释器里输出一致"，现在两个目标有真内核背书。诚实的说法是：`osx/arm64`、`osx/x86_64`（Rosetta 2）、`lnx/x86_64`、`lnx/arm64` **四个目标每次都被真实执行**；只剩 `win/*` 仍是结构性验证——PE 还没有导入表，没有任何东西真的调到 kernel32。**（后续：E-35已把 `win/*` 也跑通，六个目标全部真机执行。）** |
| **状态** | **已证实**（2026-09-19）。固化为 [A-27]（`tests/crossnative.sh`）|

#### E-30　外部语料第一次基线：220 个别人写的程序，暴露六个自有探针看不见的缺陷

| 栏 | 内容 |
|---|---|
| **命题** | 自有探针集（`tests/c/*.c` + `examples/*.c`，44 个）度量的是**我们想得到的东西**。把 [c-testsuite](https://github.com/c-testsuite/c-testsuite) 的 220 个单文件程序接进来，才第一次得到**不是自己出题自己判卷**的覆盖率数字 |
| **证据** | `tests/corpus.sh` → `corpus 220   pass 196   wrong 0   unsupported 17   knownfail 7   slow 0`（程序被编成本机镜像**真实执行**，不是解释）。首次运行是 `pass 124   wrong 8`，其中 8 个是**真误编译**——通过了 difftest 43/43 的编译器，在别人的代码上错了八次 |
| **六个缺陷** | ① **`sizeof` 的操作数被跳过到下一个分隔符** —— `sizeof(0) < 2` 整个 `< 2` 被吞掉。`unary_skip()` 这个函数本身就是错的，删掉，改为记住 `unary()` 真正停在哪里、只回滚发射的代码<br>② **`&&` / `\|\|` 的右操作数用 `rvalue()` 解析** —— 于是 `a && b \|\| c` 变成 `a && (b \|\| c)`，两个逻辑运算符之间根本没有优先级<br>③ **宏在字符串字面量里也展开** —— `#define NULL 0` 在作用域内时，`printf("c is NULL\n")` 打印 `c is 0`<br>④ **数组形参没有退化为指针**（C99 6.7.5.3p7）—— `int x[100]` 形参按数组分配，`x[0]` 取的是栈上的垃圾<br>⑤ **声明说明符按"最后一个词"解析** —— `long int` 先 `long` 再 `int`，结果是 `int`，`sizeof(long int) == 4`<br>⑥ **标签后面不解析被标记的语句** —— `switch(x) case 1: return 1;` 把 `return` 发射到了 switch **外面**，于是它无条件执行 |
| **机理** | 这六个里有五个在"常见写法"下不可见：自有探针从不写 `sizeof(x) < 2`，从不在字符串里放宏名，从不给函数传数组形参。**探针集的覆盖率是作者想象力的函数**，外部语料不是 |
| **意义** | 这是 [P-6] 的第二级仪器：difftest 能看见 gold 表错了，外部语料能看见**走查器错了**。两者都不是 `acc`——全程 `acc = 1.000`。论文里"神经层精确"与"编译器正确"必须分开陈述，这条是证据 |
| **成本** | 抓这六个 bug 的全部代价：写一个 60 行的 shell 脚本 |
| **状态** | **已证实**（2026-09-19）。`tests/corpus.baseline` 固化为棘轮：`pass` 只能升不能降 |

#### E-31　把 `short` 加进类型表：扩语言 = 扩一张表 + 重新构造，kernel 一行不动

| 栏 | 内容 |
|---|---|
| **命题** | C99 的 `short` 此前缺失（`sizeof(short)` 给 4）。补齐它需要改的是 **`TYS` 词表**（8 → 9 个类型）和一条整型提升规则，**不需要改 kernel、不需要改推理路径、不需要改任何一个其它阶段** |
| **证据** | `TYS` 加 `"i16"` → `type` 的 FULL gold 从 1,216 行涨到 1,539 行 → `build-weights` 重新构造 → **11/11 仍然全域精确**，UNS2 从 3,991 B 到 4,004 B（+13 B）。原生 arm64 上 `short` 的取/存/截断全部与系统编译器一致 |
| **代价明细** | 构造路线：改 2 处词表 + 1 条提升规则，重新构造 3 秒，**精确性由构造保证，不需要验证运气**。训练路线：`type` 在原宽度（h=16）下掉到 **0.9968 欠拟合**，加宽到 h=20 才回到 1.000 —— key 空间涨了 27%，容量就得跟上 |
| **意义** | 这是"换权重即换能力"第一次被用来**扩语言**而不是换目标。同时它给了训练/构造对比一个新维度：**扩表时，构造路线的成本是确定的，训练路线的成本是要重新调容量的**——后者才是论文里该讲的差别，不是体积 |
| **状态** | **已证实**（2026-09-19） |

#### E-32　ELF 少了一个可写段：解释器、`readelf`、六目标 fold 全都看不出来，真 Linux 内核一上来就 SIGSEGV

| 栏 | 内容 |
|---|---|
| **命题** | 我们发射的 ELF 只有**一个 PT_LOAD，`p_flags = PF_R\|PF_X`**。scratch 单元与 print 缓冲区都在 `data` 里，程序要写自己的镜像 —— 于是在真 Linux 上第一条指令就段错误 |
| **为什么没被发现** | `exec_target.py` 的目标机模型**不建模页保护**；`tests/artifacts.sh` 只检查文件被平台工具认得；`--fold` 六目标全绿。Mach-O 早就被 macOS 逼着分出了 rw `__DATA`（I-8），**ELF 因为从没在真内核上跑过而一直藏着** |
| **证据** | GitHub Actions `ubuntu-latest` 第一次执行 `/tmp/hello` → `exit 139`。改成 text(r-x) + data(rw) 两个 PT_LOAD、`p_offset ≡ p_vaddr (mod 0x1000)` 后通过 |
| **意义** | [X-3]"我们只解释、不 execve"这条自我限制的代价在这里结清：**解释器的忠实度上限就是它建模了多少机器事实**。六分之五的目标此前只被结构性地验证过。CI 的价值不是"测试通过"，是**把镜像交给一个我们无法在开发机上模拟的内核** |
| **状态** | **已证实**（2026-09-19），记为宿主契约 [I-12] |

#### E-1　容量不是瓶颈；编译器决策表可被极小网络完全记忆

| 栏 | 内容 |
|---|---|
| **命题** | 编译管线的 10 个表形状阶段 + 1 个合体网，可由总计 21,925 参数的网络在 FULL gold 上达到 **acc = 1.000**，纯标准库、单核、18.8 秒 |
| **证据** | `python3 -m unisa train && python3 -m unisa acc` → 11/11 stages 1.0000 |
| **机理** | 输入是**有限离散 key**，embed 即 one-hot×矩阵 = 查表取行；网络本质是被压缩的真值表，不是函数逼近。问题从"能否泛化"退化为"N 参数能否精确表示 M 行"，是容量算术 |
| **意义** | 论文的第一块地基。它把"神经网络做编译"从可行性问题降格为工程问题——真正的难点不在这里 |
| **状态** | **已证实**（2026-09-18） |

实测规模（`rows` = FULL gold 行数）：

```
stage     rows  theta    h0  层形状                 min-margin   median
pp          18    274    12  12->12->[4]                 0.45      3.86
lex        121    418    12  12->12->[10]                0.83      8.85
parse      270   1288    16  16->16->[32]                2.46      9.16
type       960    801    24  24->16->[9]                 1.19     25.93
scope       30    479    16  16->16->[7]                 1.29      5.49
irsel      135    953    16  16->16->[25]                0.95      6.27
enc        228    829    24  24->16->[5]                 7.63     11.21
reloc        6    161    12  12->8->[3]                  0.70      3.26
isel        76   2413    20  20->24->16->[5+51+5]        3.97     14.77
abi        228   4256    36  36->32->20->[56+18*4+4]     1.93     41.64
combo      228  10053    52  52->48->32->[9 heads]       2.39     19.63
TOTAL            21925
```

`type` 用 **801 参数装下 960 行**——参数比行还少，是真压缩而非记名。

#### E-2　类别平衡在超拟合任务上是**有害**的 ML 习惯

| 栏 | 内容 |
|---|---|
| **命题** | v1 规定的"训练时按 1/19 下采样 illegal 行"使 `type` 恒定卡在 **0.999**；取消后立即 **1.000** |
| **证据** | 消融：`TYPE_ILLEGAL_WEIGHT = 1/19 → final 0.9990` ／ `= 1.0 → final 1.0000`（`unisa/gold.py`） |
| **机理** | 丢失的唯一一行是 `(i64,&,i32)→illegal`，正是唯一正例 `(i64,&,i64)→ptr` 的**近邻**；全表最紧的 6 个 margin 全部落在 `&` 上。该规则需要三元合取 `t1=i64 ∧ t2=i64 ∧ op=&`，至少占用一个专用隐单元。降权 64% 的定义域，等于在稀疏正例周围抽走梯度，网络就在它旁边变瞎 |
| **意义** | 可推广的反直觉结论：**类别平衡保护的是泛化**；当任务是有限域上的记忆，它只会制造盲区。论文可据此主张"超拟合任务需要一套与统计学习相反的训练守则" |
| **状态** | **已证实**，v1 规则已废止（见 G-2a） |

#### E-3　体积账：网络**比裸表大**，体积主张的对比基线必须写明

| 栏 | 内容 |
|---|---|
| **命题** | 11 张表 bit-packed 共 **2,952 B**；同样 11 个网络 i8 量化共 **24,812 B**。网络是裸表的 **8.4 倍** |
| **证据** | 裸表按 `rows × ceil(log2(classes))` 逐阶段计；权重取 `unisa dump-weights --dtype i8` 实测 |
| **机理** | 表就是表的最优编码；网络额外付出的是 embedding 与隐层的连续参数化代价 |
| **⚠ 已被 E-22 部分推翻** | 本条对**训练**权重仍然成立，但对**最小覆盖构造**权重不成立——后者比裸表**小 2.19×**（1,374 B vs 3,010 B）。体积主张对构造路线**无需**下面这条护栏 |
| **意义（仅对训练权重）** | **对论文是护栏而非坏消息。** 体积主张若写成"网络比表小"则可被一行算术证伪；正确表述是 kit（权重+单一 kernel）对比 **tcc 二进制中的手写算法代码**（200–400 KB 中绝大部分不是表）。真实收益是另外三条，与体积无关：<br>① **表示统一** —— 11 个异构决策点收敛到同一个 kernel<br>② **可热插拔** —— 换权重即换目标，代码零改动<br>③ **可穷举验证** —— 裸表亦有此性质，手写 switch 没有 |
| **状态** | **已证实**；已写入 B-2a |

```
裸表(bit-packed)   2,952 B
i8 权重           24,812 B     8.4x
f32 权重          90,516 B
```

#### E-5　可判定性重构了证明问题

| 栏 | 内容 |
|---|---|
| **命题** | 逐阶段等价 `∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` 在有限闭域上由穷举**完全判定**，无需任何逼近论证 |
| **证据** | `unisa acc` 即该判定过程本身；域大小 6–960 行，总计 2,300 key |
| **机理** | 超拟合把定义域钉成完整笛卡尔积，`K_s` 有限、闭合、可枚举 |
| **意义** | 论文可主张：**在有限决策域上，神经组件的验证不是统计问题而是模型检验问题**。PAC bound / Lipschitz 常数 / 鲁棒半径在此全部不适用且不必要。唯一穷举帮不上忙的是 key 全域性 [P-2]——它需要对经典走查器做可达性推理 |
| **状态** | **已证实**（P-3 已 discharge） |

#### E-15　走查器是 gold 的检验器；穷举证不了 gold 本身

| 栏 | 内容 |
|---|---|
| **命题** | `acc = 1.000` 只证明 **net ≡ gold**（P-3），**证不了 gold ≡ C99**（P-6）。后者只能由走查器跑真实程序来暴露 |
| **证据** | 实现 M4 时发现两处 gold 缺陷，二者在 `unisa acc` 下均为满分：<br>① **parse 的 stmt 行缺 struct/enum/typedef** —— `struct P p;` 落到默认 `expr`，`examples/struct.c` 直接语法错误。C99 中这三个 token 在语句开头永远是声明，无表达式形式。已修正 G-1<br>② **PRODS 实为 32 项**，v2 记为 34（计数笔误） |
| **附带设计事实** | `type` 表的 TOPS 轴是**规范化**的：`<` 代表全部四个关系运算，`==` 代表两个相等运算。走查器必须先投影再问 oracle（`sema.TOP_CANON`）。这属于经典代码的 key 编码职责，与 typedef 反馈同类 |
| **意义** | 验证分工必须写清：**穷举判定 net↔gold，真实程序判定 gold↔语言**。只报 acc 而不跑程序，是在用一把尺子量两样东西。论文里这是 P-3 与 P-6 边界的实证 |
| **状态** | **已证实** |

#### E-16　`gate` 挂错了 key —— 后端版的 E-15

| 栏 | 内容 |
|---|---|
| **命题** | v1/v2 把 `gate` 放在 **isel（key = op × arch）** 上，但 gate 是 **OS 事实**：x86_64 在 lnx/osx 是 `syscall`、在 win 是 `winapi`；arm64 在 lnx 是 `svc0`、在 osx 是 `svc80`。`(op, arch)` 这个 key **在表达力上就不可能**承载它 |
| **证据** | 实现 lowering 时 `--drive spec` 必然给 osx/arm64 发出 `svc0`；`unisa acc` 对此依旧满分——因为 net 忠实复现了一个本身就错的 gold |
| **修正** | `gate` 移至 **abi（op × os × arch）**。9 头总数不变：isel 2 + abi 7。`isel.form` 保留为 arch 视角，lowering 以 os 感知的 **enc.form** 为准。重训后全部仍 1.000 |
| **意义** | 与 E-15 同源但更尖锐：E-15 是表**漏了一行**，E-16 是表**选错了 key**——后者 acc 永远发现不了，因为网络会忠实地学会一个错误的函数。**key 选择是规格问题，不是训练问题**，而它只在下游用到该事实时才暴露 |
| **状态** | **已证实** |

#### E-17　目标感知的 fold 具备真实判别力（E-10 兑现）

| 栏 | 内容 |
|---|---|
| **命题** | `--fold` 不是自比：三种故障注入全部使其退化到 **4/6** |
| **证据** | `osx_class_bit → 4/6`（`no syscall 4 on osx`）· `win_argregs → 4/6`（`write to handle 0`）· `arm_gate → 4/6`（`gate svc80, this machine issues svc0`） |
| **机理** | 目标解释器持有 arch 的具名寄存器与 OS **自有的** ABI + syscall 表，只解析 lowering 产出的 `(os, sysno)` 或 `(os, winapi 名)`，绝不回看 tape op。ABI 参数寄存器由机器自己知道，不从网络读——否则 `win_argregs` 无法被检出 |
| **意义** | P-7（目标等价）的可操作形式。论文里这是"验证有牙齿"的存在性证明：**测试必须能被主动打破** |
| **状态** | **已证实** |

#### E-18　按编译阶段合并模型是有害的；每个决策点一个独立模型

| 栏 | 内容 |
|---|---|
| **命题** | 把决策点按流水线阶段合并成三个段模型，**代价与段内 key 空间的异质度成正比**；最异质的那段在任何宽度下都到不了 1.000 |
| **证据** | 分立 θ → 合段 θ / 结果：<br>`s1 源码预处理 (pp+lex)` 692 → 1,446，**1.000**<br>`s3 IR→多ISA (isel+abi+enc+reloc)` 7,679 → 15,169，**1.000 但 epoch 翻倍**（108 vs ~50）<br>`s2 代码→IR (parse+type+scope+irsel)` 3,521 → 11,513 **0.9168** ／ 27,417 **0.8616** ／ 45,209 **0.8143**，**均未达 1.000**<br>另有对照：`combo` 是 key 空间**完全相同**的最有利合并，仍贵 1.5×（6,689 → 10,053） |
| **机理** | s2 四张表的 key 空间互不相干（`NT×TOK` / `ty×op×ty` / `ctx×kind` / `family×flavor`），共享 trunk 没有结构可共享、只有互相挤占。**加宽反而更差**（0.917→0.814）：固定 epoch 预算下更宽的网络每参数梯度更稀疏，等于给互相打架的任务更大的战场 |
| **重要限定** | **该结论只在 SGD 路线下成立。** 用构造法（E-19）得到权重时，s2 合段是精确的——不同表的项落在 W1 的不相交块里，干扰从结构上不存在。构造路线下仍建议分立，理由变成**变更局部性**（改一张 gold 只需重建那一张） |
| **意义** | 给出「哪些决策该合、哪些该分」的可操作判据：**看 key 空间是否同构**。这让"每决策点一个微型网络"从工程口味升级为结构性结论 |
| **状态** | **已证实**（SGD 路线）；构造路线下不适用 |

#### E-19　精确权重可由决策表直接构造，kernel 不变

| 栏 | 内容 |
|---|---|
| **命题** | 无需训练、无需任何随机量，可从 gold 决策表**直接构造**出在全域精确的权重，且部署 kernel（K-1）一个字不改 |
| **机理** | one-hot 输入上的一个合取项**就是**一个 ReLU 单元：`ReLU( Σ_{i∈S} 1[key_i = v_i] − (|S| − 0.5) )`。构造得 `W1 ∈ {0,1}`、`b1 ∈ {0.5, −0.5, −1.5, −2.5}`、`W2 = 2^tier`——**全是小整数与 2 的幂** |
| **关键技巧** | **同层互斥**：一个 tier 只固定同一个字段子集，不同取值永不重叠 ⇒ 每层至多一个项激活 ⇒ `B=2` 即足以压住所有低层，不需要大数。配一轮**验证驱动的修补**（全域复核后把失败 key 钉在最高具体度）保证精确 |
| **证据** | 11/11 阶段全部精确，零训练零随机；3 个段模型（含 SGD 到不了的 s2）同样精确 |
| **代价** | 796,490 θ vs 训练的 21,945（36×）。但项推导是朴素实现：`parse` 推出 168 项，而 spec 自己写的规则只有 5 条默认 + 27 条覆盖 = 32 条。差距主要在覆盖优化，不是原理代价 |
| **意义** | 精确性从"被搜索发现"变为"**算法维持的不变量**"：P-3 从"由穷举发现"升级为"由构造保证、由穷举复核"。种子抽签、epoch 预算、LR 表全部退出决定链 |
| **状态** | **已证实**；紧致化见 P-8a/b/c（E-20 已归档到 `archive/prd-history.md`） |

#### E-28　编译器编译了自己的决策层 —— 自举的第一块

| 栏 | 内容 |
|---|---|
| **结果** | `unisa emit-kernel` 把决策层发成**单个自包含 C 文件**（模型数据 + 整数 kernel + 全域自检）。该文件由 **unisa 自己编译**、原生执行，复现全部 **6,414 个决策，0 错**；与系统 `cc` 编译的同一份源码结果**完全一致** |
| **为什么这是自举的第一块** | 命题说「Shell = 推理器 + 执行器 + 模型数据」。这个 C 文件**就是**推理器加模型数据，而编译它的是 unisa 本身。编译器已经能处理自己决策层所需的全部 C 构造：全局结构体数组、`char*` 索引、位运算、移位、嵌套控制流、文件 I/O |
| **形态** | 模型编码为字节 blob（`char *MODEL = "\x.."`），避开我们尚不支持的嵌套全局初始化器；kernel 是 `getb/mask64/infer`，**无乘法、无浮点、无 malloc**；`act[]`/`z[]` 是静态数组 |
| **为此补齐的 C 特性** | 相邻字符串字面量拼接 [W-12] · 全局指针的启动期初始化 [W-13] · 全局数据 8 字节对齐 · 前端改 latin-1 字节精确 |
| **三个被真机抖出来的 bug（解释器全都看不见）** | ① **PIE 下数据段里不能有绝对地址**：ASLR 让镜像滑动，数据里烘焙的链接期地址失效 ⇒ 全局指针改由 `_start` 用 PC 相对代码初始化<br>② **`spinit` 绑错了位置**：`_start` 移到中段后，`mov x7, sp` 还绑在 tape 第 0 条指令上，入口进来 tape SP 是垃圾<br>③ **全局未按 8 字节对齐**：`g_CASES` 落在 0x2abe，`str x0,[x17]` 在 arm64 上是 **SIGBUS** |
| **一个静默数据损坏** | 前端用 UTF-8 读源码，`\x80..\xff` 被编成两个字节——10,514 字节的模型 blob 变成 10,679 字节。**字符串字面量里的二进制数据被静默破坏**，只有拿它当数据用时才会暴露。全链路改 latin-1（源字节与字符一一对应） |
| **固化** | `[A-19]`：`cc` 与 `unisa` 编译同一份 C kernel，两边都必须是 `6414 decisions, 0 wrong` |
| **状态** | **已证实** |

#### E-27　原生执行达成：编译出的镜像在真机上跑起来了

| 栏 | 内容 |
|---|---|
| **结果** | `unisa compile … --target osx/arm64` 产出的 Mach-O 经 `codesign -s -` 后**直接运行**，八个样例全部与解释器逐字节一致（`tests/native.sh`：native 8 / mismatch 0）。整条链路：C 源码 → 构造出的整数网络做全部决策 → tape → lowering → 真实 AArch64 机器码 → Mach-O → dyld → 内核 |
| **⚠ 修订 X-3** | 原条款"不 execve，只解释"是**范围约定**，不是设计约束。解释器仍是 `--fold` 的基准真值 [TP-4]；但对宿主目标，镜像现在是**真程序**，`tests/native.sh` 把它固化为验收项 |
| **踩到的六个坑（都是真实平台契约，不是我们的 bug）** | ① Apple Silicon **不支持静态可执行文件**，必须 `LC_LOAD_DYLINKER` + `LC_LOAD_DYLIB` + `LC_MAIN` 经 dyld 引导<br>② `codesign` 会**追加** `LC_CODE_SIGNATURE`，头区填满就会覆写 `__text` 前 16 字节（反汇编成 `udf`，SIGILL）——必须留余量<br>③ arm64 macOS **强制 PIE**，镜像会滑动 ⇒ 绝对地址作废，改用 `adrp+add` / `rip-relative`<br>④ scratch 要写，不能放在 r-x 的 `__TEXT`，得单独的 rw `__DATA` 段<br>⑤ **Darwin/arm64 的系统调用号在 `x16`**，不是 Linux 的 `x8`；而 `adrp+add` 的临时寄存器恰好也是 x16，正好把调用号冲掉（进程挂死而非报错）<br>⑥ `bl`/`ret` 走 lr，与 tape 的栈语义不符，递归即崩——`call`/`ret` 改为在 tape 栈上压弹 |
| **新 oracle：原生执行比解释器更强** | `b_static` 在解释器下输出正确的 `3`，**原生下是垃圾 `1826745499`**——`static int n;` 被当成普通局部变量，而解释器的内存是**清零**的，侥幸掩盖了未初始化读。已修（static 局部改为静态存储）。**解释器的零初始化会掩盖真实程序里的未定义行为，真机执行会把它们抖出来** |
| **意义** | "对标 tinycc/tccrun" 从主张变成事实的前提已经具备：我们发的是真机器码，不是被解释的中间表示 |
| **状态** | **已证实** |

#### E-26　P-8a 在小表上关闭：五张表的 `h_min` 被**精确**证明

| 栏 | 内容 |
|---|---|
| **结果** | 上下界吻合，不是估计：`reloc 3→3`（**已最小**，h=2 证不可能）· `enc 5→4` · `pp 6→4` · `scope 10→4` · `lex 11→5`。五张表 **35 → 20 单元**。另：`parse` 的 h=5 证不可能 ⇒ `h_min(parse) ≥ 6` |
| **使能步骤：值重复商** | 按字段合并"标签切片相同"的取值。**双向可靠**：限制方向给 `h_red ≤ h_full`；把字面量集合提升回各组的并集**逐字复现 logits**，给 `h_full ≤ h_red`。折叠幅度：`enc 38×3×2 → 2×2×2` · `lex 11×11 → 9×3` · `pp 9×2 → 5×2` · `reloc 3×2 → 2×2` |
| **⚠ 修正一个搁置的顾虑** | 此前把"字母表商"记为"窄化 `K_s`、需先想清楚对 P-2 的影响"而搁置。**提升方向的证明说明它不是窄化，是双射重述**，`K_s` 未被改变。该顾虑取消 |
| **假设类** | 一个单元完全由其**激活集合 = 组合矩形**刻画（`W1∈{0,1}`、`b1=−(\|F\|−1)` 不增加自由度），故一个网络 = (H 个矩形, 整数 `W2`)。`W2` 在**全体有理数**上搜索——严格强于仓库的正整数约定 |
| **不可能性不依赖 LP** | 所有负面结论的 LP 调用次数为 **0**，只用两条初等必要条件：(N1) 每个 key 至少激活一个单元，否则 logits 全零、平局被拒；(N2) 激活模式相同的 key 必须标签相同。加上由此导出的计数界 |
| **穷举规模** | `scope h≤3` 覆盖 `C(945,3) = 1.40×10⁸`；`lex h≤4` 覆盖 `C(3577,4) = **6.81×10¹²**`。两套互相独立的完全搜索交叉验证，并与无剪枝暴力在 `reloc`/`enc` 上对拍 |
| **验证** | 每个给出的网络提升回**原始** key 域，经三套独立 kernel 全量枚举：自建 checker、仓库的 `construct.verify_int`、`intnet.IntNet.predict`，全部 0 错。`max\|pre\| ≤ 2`、`max\|logit\| ≤ 10`，落在 K-5 的 int8 包络内 |
| **未决** | `irsel` 预算内未完；`parse` h=6 未决；`type`（三字段）与多头的 `isel`/`abi`/`combo` 未尝试——signature 重述只适用于两字段，多头还有跨头共享单元的耦合 |
| **意义** | P-8a 从"开放"变为"**小表已精确关闭 + 大表仍开放**"。同时说明 `construct.py` 的贪心覆盖**在这五张表上就多留了 15 个单元**，最小性不是理论摆设，是可兑现的压缩 |
| **状态** | **已证实** |

#### E-13′　规模律：裸表线性增长，构造覆盖**对数**增长，从第一档就赢

| 栏 | 内容 |
|---|---|
| **命题** | 沿目标数扩张 key 空间时，裸决策表随行数**线性**增长，而精确构造所需的单元数只**对数**增长。构造不是"大了才划算"——它从最小规模就已经更省 |
| **证据** | 合成目标扩张（结构派生，仅测规模、不声称正确性），**每一档都全域精确**：<br>`6 目标 / 228 行 / 50 单元 / 表 770 B / 覆盖 607 B / 1.27×`<br>`24 / 912 / 57 / 3,192 / 734 / 4.35×`<br>`64 / 2,432 / 62 / 9,120 / 861 / 10.59×`<br>`120 / 4,560 / 68 / 17,100 / 995 / 17.19×`<br>`224 / 8,512 / 72 / 32,984 / 1,134 / **29.09×**` |
| **拟合** | 目标数涨 **37×**、行数涨 **37×**，单元数仅 50 → 72（**1.44×**）。`units ≈ 39 + 4.2·log₂(targets)` |
| **机理** | 新目标带来的是**同构的**行：字段取值变多，但决策**规则**没变多。值集合字面量把新取值并进已有规则，所以覆盖只按"规则数 + 少量区分位"增长，而真值表必须把每一行都写出来 |
| **对比 E-12** | E-12 测的是 SGD 路线：交叉点在 128 目标，且过 24 目标精度就塌到 0.4419。**构造路线每一档都精确，且从第一档就赢。** E-12 的交叉点结论对构造路线不适用 |
| **意义** | 这条关掉了 E-3 留下的最后疑虑。体积主张现在有两个独立支撑：**绝对值**上构造比裸表小 2.19×（E-22）、**规模**上差距随目标数无界增长。跨 ISA 越多，优势越大——而这正是本项目的方向 |
| **状态** | **已证实**；E-13 兑现 |

#### E-25　两条独立压缩路线收敛到同一个底；梯度方向是活板门不是斜坡

| 栏 | 内容 |
|---|---|
| **收敛** | 两条互不相干的路线殊途同归：**最小覆盖**（自顶向下综合，E-22）得 **30,103 θ**；**构造后压缩**（自底向上，从 796,490 θ 的朴素构造出发做无梯度离散搜索）得 **29,920 θ**，26.6× 压缩，**183,968 次全域检查点上精确性从未破过**。两者相差 0.6% |
| **意义** | 独立方法收敛到同一数量级，是"**这个架构类的真实下界就在 3 万附近**"的有力证据，而非某个算法的偶然。SGD 的 21,945 θ 仍小 1.36×——差距来自**分布式编码**：SGD 能把 k 条规则塞进少于 k 个隐单元，而"值集合取合取"的构造按设计做不到 |
| **逐阶段构造已胜出** | `enc 196 vs 829`（**4.2×**）· `pp 64 vs 274` · `scope 133 vs 479` · `reloc 18 vs 161` · `lex 330 vs 418`；`type` 打平（861 vs 801，**960 行装进 21 个单元**）。SGD 仍胜在 parse/irsel/isel/abi/combo——类集合大而不规则的头 |
| **最大单项技术** | **值集合字面量**（一个字段内的析取在 one-hot 上是免费的：`W1[v,j]=1` 覆盖整个集合仍只贡献 1）。单这一招把总量砍半。两条路线都独立发现了它 |
| **⚠ P-8b 的机理找到了** | 固定 `b1` 时，`W1[i,j]: 1 → 0.75` 在 **100%** 的单元上**激活集合完全不变**，只把输出从 0.5 缩到 0.25；`1 → 0.25` 则在 **100%** 的单元上**直接清空**。所以 `W1` 方向上的梯度路径**终点是单元死亡，而不是另一条规则**——要改变一条规则，必须 `(W1, b1)` **同时**发生离散跳变，也就是 `generalize`/`widen` 这两个动作（占全部压缩的 87%）。<br>**这就是"SGD 能拟合却压不动组合结构"的结构性原因：稀疏化方向是一扇活板门，不是斜坡，任何小步都到不了。** |
| **意外工具：自动"key 选错"探测器** | 精确压缩后的**单元数**本身能检出 E-16 那类缺陷：`isel.form` 在 `(op,arch)` 键上需要 **21** 个单元，而 os 感知的 form 函数在 `(op,os,arch)` 上只需 **4** 个（两者在 228 个 key 中的 38 个上不一致）。同样信号：`abi.ret` 自己把 `os` 丢掉了，而 `gate`/`sysno` 保留。**这是 acc 永远看不见的东西——网络会忠实地学会那个错误的函数** |
| **未奏效** | 低秩 `W1` 分解（精确秩从不低于盈亏点）· `W2` 稀疏化（构造本就每单元每头 ≤1 个非零）· GD 当压缩器（参数零收益，但把 max\|W2\| 从 2048 压到 0.0156，是**量化余量**而非参数余量） |
| **状态** | **已证实** |

#### E-22　最小覆盖 + 纯整数：873 B、无乘法、**比裸表还小**

| 栏 | 内容 |
|---|---|
| **命题** | 把朴素构造换成最小覆盖后，构造出的网络**同时**做到：全域精确、纯整数、零乘法、且按位打包后**比裸决策表本身还小** |
| **覆盖** | **255 项 / 30,103 θ**，11/11 全域精确（5,568 个 (key, head) 决策，三条独立路径各验一遍）。相对朴素构造 **26.5×** 更小；相对 SGD 训练的 21,925 θ 仅剩 1.37×。`parse` 压到 **31 项**——而 spec 手写规则是 5 默认 + 27 覆盖 = **32 条**，自动覆盖几乎复现了人手写的规则数 |
| **整数化（零代价）** | `b1 = −(|F|−1)` 消去半整数，其余不变。**W1 ∈ {0,1}**、**b1 ∈ {0,−1,−2}**、**W2 ∈ {1,2,4,8,16}**，隐激活恒为 0 或 1 |
| **累加器** | 全局 **max\|pre-activation\| = 2，max logit = 19** ⇒ **int8 绰绰有余**（隐层 3 bit、logit 5 bit）。界来自 `\|F\| ≤ 3` 与 rank 深度 ≤ 4，**与覆盖规模无关** |
| **kernel 进一步收紧** | 隐层 ∈ {0,1} 且 W2 是 2 的幂 ⇒ **第二层连移位都不需要，是条件整数加**；第一层 one-hot ⇒ **m 次加法 + 一个 bias**。全链路无乘法、无移位、无浮点 |
| **体积（编解码往返实测）** | 每单元存各字段的字面量集合与 (类, rank)；**`b1` 不存**（由字面量数恢复）。三套编码逐字段择优（位掩码 / 2-bit tag / 共享字面量表）：<br>**1,374 B 全部**，**873 B 默认 spec 路径**<br>对比：朴素构造 17,130 / 9,817 B；量化训练 23,640 / 12,860 B；裸表 3,010 / 1,870 B |
| **倍数** | vs 朴素构造 **12.5×** · vs 量化训练 **17.2×** · **vs 裸表 2.19×（更小）** |
| **⚠ 这条翻转了 E-3** | E-3 记录"网络比裸表大 8.4×"，据此把体积主张的基线限定为"tcc 中的手写算法代码，而非表"。**对构造权重该护栏不再需要**：最小覆盖比裸真值表**小 2.19×**，因为覆盖能利用真值表无法表达的结构（一个单元承载一整条覆盖规则、跨头共享 cube、`sysno` 的乘积结构），而 SGD 网络为"重新发现"这些结构付了连续参数化的税 |
| **状态** | **已证实**；E-3 / E-3′ / B-2a 据此修订 |

#### E-21　整数构造：零乘法、零浮点、二值隐层，且**比量化训练更小**

| 栏 | 内容 |
|---|---|
| **起因** | 观察：`代码 → IR → 多 ISA` 这条流水线在逻辑上不需要任何浮点运算。那么模型能不能也是纯整型的？ |
| **命题** | 取 `b1 = −(|S| − 1)`（而非 `−(|S| − 0.5)`）即可消去半整数。此时 **W1 ∈ {0,1}**、**b1 ∈ {−2,−1,0,1}**、**W2 = 2^tier**。又因输入是恰含 m 个 1 的 one-hot，第一层根本不是矩阵乘，而是"把选中的 m 行加起来"——**一次乘法都没有** |
| **意外性质** | `maxA = 1`：**每个激活单元的输出恰好是 1，隐层是二值的**。于是整个网络是 二值输入 → 二值权重 → **二值隐激活** → 2 的幂输出权重 → 整数 argmax，是严格意义的 BNN，但**由构造精确**，不是 straight-through 训练后祈祷精度 |
| **证据** | 11/11 阶段全域精确（total wrong = 0）；最大 logit **136** ⇒ **int16 累加器足够**；`b1` 取值仅 4 个且**无需存储**（等于 `−(非通配槽数 − 1)`） |
| **体积（按位打包实测）** | 每单元 = m 个坐标槽 + 类下标 + tier（+ 多头时的头下标），10–31 bit/单元：<br>`reloc 9 B (0.03×)` · `pp 21 B (0.05×)` · `scope 44 B (0.07×)` · `lex 60 B (0.10×)` · `isel 504 B (0.26×)` · `irsel 321 B (0.29×)` · `parse 399 B (0.34×)` · `enc 582 B (0.61×)` · `combo 7,313 B (0.68×)` · `abi 5,355 B (1.09×)` · `type 2,522 B (2.84×)`<br>**合计 17,130 B vs 量化训练 23,640 B = 0.72×**；默认 spec 路径 **9,817 B vs 12,860 B = 0.76×** |
| **修正** | E-19 与 E-20（后者已归档）用的 796,490 θ 是**稠密**计数，对一个二值且每列至多 m 个 1 的 `W1` 严重高估。**以按位打包体积为准** |
| **意义** | ① 部署 kernel 可收紧为**无浮点、无乘法、int16**；② 之前"构造贵 36×"的结论作废，构造在真实存储上**更省**；③ E-7（q2 全线失守）的对照更清楚：量化是把任意实数往格点上凑，而构造权重**本来就在格点上**，不存在量化这一步 |
| **上界性** | 项数来自朴素推导（`type` 917 项 / `abi` 1,428 项），最小覆盖优化后应显著下降。**当前数字是上界** |
| **状态** | **已证实** |

#### E-6′　量化阶梯实测：q2 全线失守，margin 不是预测量，混合 dtype 收益微小

| 栏 | 内容 |
|---|---|
| **方法** | 逐阶段跑 `f32→f16→i8→q4→q2`，判据为 Q-6 argmax 不变性，**全域枚举**（combo 2,052 个决策、abi 1,596、type 960…）。编码器对 `weights/*.f32.unisa` 与 `*.i8.unisa` **逐字节复现**仓库自己的写出结果，故格式无误；并在 K-3 的整数算术下判定阶梯落点（int32 累加、scale 每输出一次），与朴素逐元素反量化模式对比 **0 处 argmax 分歧** |
| **结果** | f16 全线不变。**8 个阶段止于 q4，3 个止于 i8**（`irsel` `abi` `combo`）。**q2 无一幸存**——`reloc` 在 q2 丢 6 个 key 中的 1 个（`jz/arm64→arm19`），`enc` 丢 228 中的 1 个（`fence/lnx/x86_64`） |
| **E-7 证伪** | 预测"多数阶段可到 q2"完全不成立；预测最先失守的 `pp`(0.33) 和 `reloc`(0.59) 反倒都到了 q4 |
| **E-6 降级** | min-margin 对落点只有**弱排序**作用：三个止于 i8 的阶段在 margin 排名中位列 1/3/6，与幸存者交错而非分离；任何 margin 阈值最好只错 2 个，而"一律猜 q4"错 3 个。更好的预测量是 **头数**（r=+0.75）与 **−log₁₀θ**（ρ=+0.65）。机理：压垮 q4 的是逐行误差在**多少个独立决策**上复利，不是最坏那一个有多紧。中位 margin 甚至是**反**相关（AUC 0.29） |
| **E-8 不成立** | 混合 dtype kit **23,692 B**，统一 i8 **24,812 B**——只省 **4.5%**。原因：kit 的 45% 是 `combo`(10,780 B)，而它恰好是**降不下 i8** 的三个之一 |
| **工程结论（收益 10×）** | `combo` 在默认 `--drive spec` 路径下**根本不参与**（U-2），把它排除出 kit 即降到 **12,912 B**——比整条量化阶梯的收益大一个数量级。**出货 kit 应按 drive 模式裁剪，而不是把所有训练产物都装进去** |
| **预算** | B-2（权重 ≤ 32 KB）**通过**，余量 9,076 B |
| **状态** | **已证实**；E-6 降级、E-7 证伪、E-8 不成立，Q-8/Q-9 已按实测修正 |

```
stage    rows   theta     f32 B   f16 B    i8 B    q4 B    q2 B   min-margin  ship
pp         18     274      1256     708     436     452x    388x      0.3302   i8*
lex       121     418      1832     996     584     572     480x      0.6023   q4
parse     270    1288      5312    2736    1448    1176     856x      2.3536   q4
type      960     801      3388    1788     988     888     692x      1.4181   q4
scope      30     479      2076    1120     640     588     468x      1.0753   q4
irsel     135     953      3972    2068    1116     912x    680x      0.7564   i8
enc       228     829      3500    1844    1016     948     748x      7.5064   q4
reloc       6     161       804     484     324     360x    320x      0.5902   i8*
isel       76    2328      9568    4916    2588    1920   1344x       4.0595   q4
abi       228    4361     17964    9244    4892    3640x   2576x      0.0973   i8
combo     228   10053     40924   20824   10780    7452x   4972x      0.5252   i8
TOTAL           21945     90596   46728   24812   18908   13524
```

`x` = 该 dtype 至少翻转一个 argmax，被 Q-6 拒绝。`i8*` = q4 虽不变但**文件更大**，按修正后的 Q-8 取 i8。

### 6.2 待验证

| ID | 命题 | 验证方式 | 依赖 |
|---|---|---|---|
| ~~E-6~~ | min-margin 可预测量化阶梯落点 | **降级**，见 **E-6′** | ✔ |
| ~~E-7~~ | 多数阶段可在 q2 下 argmax 不变 | **证伪**，见 **E-6′** | ✔ |
| ~~E-8~~ | 混合 dtype 显著小于统一 dtype | **不成立**，见 **E-6′** | ✔ |
| **E-9** | 跨 ISA 的知识是全管线中**最适合神经化**的部分（表形状最纯） | 比较 isel/abi/enc/reloc 与前端表的收敛 epoch 与 margin | 已有部分数据：enc min-margin 7.63 为全表最高 |
| ~~E-10~~ | 目标感知的 fold 具备真实判别力 | 已兑现，见 **E-17** | ✔ |
| **E-11** | 整个 kit 小于 tcc 二进制中对应的手写决策代码 | `size $(which tcc)` + 按模块归因 | M7 |
| ~~E-13~~ | 规模律 | **已兑现**，见 **E-13′**：覆盖对数增长、裸表线性增长，224 目标时差 29× | ✔ |
| **E-14** | 1.000 对 seed 稳健，不是彩票 | 11 个网络各换 8 个 seed 重训，统计到达 1.000 的比例与 epoch | 便宜，随时可做 |
| **E-44** | **分解优于合并**：把一个阶段拆成若干更小的全函数再复合，比单张大表更省、更稳、更可复用 | 见下 | 便宜；`type` 是首选标的 |

**E-44 的设计**（2026-09-21 提出）。当前每个 stage 是 `K = V₁×…×V_m → C` 的一张表，构造法给的上界是 `h = |K|`，而实测的共享隐单元已经压得很狠（`type` 3,211 行只用 **47 个单元**）。提议的方向是**反过来的那一步**：不是把两个阶段并成一个（`combo` 就是这么做的，结果 **10,053 θ vs isel+abi 的 6,669 θ**——合并更贵），而是把**一个阶段拆成若干更小的全函数再复合**。

`type` 是最该试的标的，而且**拆法不用猜，它写在 gold 的源码里**：`type_label` 本身就是「先按 op 分类 → 各自做整数提升 → 再做通常算术转换」。照此拆成

```
N_a: op → opclass                         |K| = 15
N_b: t  → promote(t)                      |K| = 9
N_c: (t1', t2') → 通常算术转换的结果       |K| = 81
N_d: (opclass, t1, t2) → 取哪一个形状      |K| = 567
```

四条都仍是**有限离散全函数**，所以 [P-3] 的穷举判定一条不少，而且**更便宜**（672 key 对 3,211）。复合不引入新的证明义务：每条精确 ⇒ 复合精确。[T-4] 不受影响——还是同一个 kernel，只是连跑四次；[P-1] 也不受影响——传下去的是**类名**，不是 logit；[P-2] 的 key 全域性在这里是**结构性成立**的，因为前一张网的输出词表**就是**后一张网的输入词表。

**但收益未必在体积上**，这一点要先说清楚：47 个单元已经是 68× 压缩，拆完未必更小。真正押注的是另外三条：

1. **同一条规则只写一处。** gold 至今被实现抓出**四次**错（E-2、E-15、E-16、E-41），**四次全在 `type`**，而其中三次是提升/转换规则。`promote` 现在散在 `type_label` 的各个分支里；拆出来它就只有一份，且被独立穷举验证。
2. **加浮点时表是加法增长而不是乘法增长。** TYS 从 9 涨到 11+ 会把 `type` 从 3,211 行推到约 4,800 行；而 `N_b`/`N_c` 只按 `|TYS|` 与 `|TYS|²` 涨，`N_a`/`N_d` 的 op 轴根本不动。**这条让 E-44 在浮点开工之前做最划算。**
3. **它是 P-8a/P-8c 的一条可操作路径**：h_min 的组合刻画在大表上仍然开放，而"照 gold 的结构分解"是一个**有现成启发式**的构造法，精确性由穷举当不变量守着——正是 P-8c 说的"好启发式 + 精确性作为不变量"，而不是求最优。

**不适用的地方**：key 本来就小或不可约的阶段（`reloc` 6 行、`pp` 18 行）拆了没有意义。另外这**不是**流式推理的问题——我们的决策本来就是逐 key、无记忆的，推理已经是可并行可缓存的极限形态；顺序状态全在走查器那一半（[T-1]），那里不归网络管。

### 6.3 已证伪

| ID | 原命题 | 证伪 |
|---|---|---|
| **E-2′** | v1：`type` 训练需按 1/19 下采样 illegal 行 | 见 E-2，该规则正是 0.999 的成因 |
| **E-3′** | 隐含假设：神经表示比裸表更省空间 | 见 E-3，实测大 8.4 倍 |

### 6.4 开放问题

1. **边界在哪里？** 已知失效条件：key 无界（任意标识符、表达式树）、稀疏单点规则挤占共享隐层。能否给出"某决策可神经化"的**可判定的充分条件**？
2. **h 的下界？** `type` 用 h=16 承载 15 个 op 的规则已近下限（min-margin 1.19）。是否存在 `h ≥ f(规则数, 合取阶数)` 的可计算下界？
3. **combo 对 spec 驱动的优劣？** 两者都到 1.000，但 combo 10,053 θ vs isel+abi 6,669 θ。合体网在什么条件下才划算？
4. **方法学能否出编译器以外的东西？** 判据不变：**有界离散 key 上的全函数**。据此筛查：<br>· **系统调用翻译层**（guest→host）——就是 `abi` 表多一根轴，且 E-13′ 的规模律说目标组合越多优势越大。**`catalog` 已经是它的八成**，把 `(op,os,arch) → sysno/参数/gate` 反过来查即可，是个几小时的验证性实验<br>· **模拟器指令译码**（`opcode × prefix × modrm × mode → micro-op`）——形状与 `isel`/`enc` 同构，x86 译码表大而易错<br>· **沙箱策略判定**（`syscall × 参数类 × 上下文 → allow/deny/audit`）——**"可穷举验证"在这里价值最高**：策略 bug 是安全漏洞，能证明"策略网络在每一输入上等于策略表"是实打实的<br>· **不合适**：容器/命名空间搭建（命令式时序、无界状态），与走查器同属经典代码那一半<br>注意优势**不是**体积（E-3 已否），是**统一基底 + 规模律 + 可穷举验证**这三条。
5. **js / wasm 这条线**（2026-09-21 记）。如果 unisacc 最终被证明**完全正确可行** —— 六个目标、语料清零、自举闭环 —— 那么同一套做法搬到 **JS / WebAssembly** 上会是更有意思的一步：wasm 的指令选择与 ABI 是**表形状**的（`opcode × 类型 × 内存模型 → 编码`），和 `isel`/`enc`/`abi` 同构，按 [E-13′] 的规模律目标越多越划算；而 JS 那半是**动态逻辑**——类型不在编译期确定，决策依赖运行期的 shape/IC 状态。那正好压到 §6.4 第 1 条那个边界：**我们的判据是「有界离散 key 上的全函数」**，而 JS 的内联缓存状态机（shape × 操作 × 保护条件 → 快路径）**看上去**也是有界离散的。如果它真是，那这套方法就不止能做静态编译器；如果不是，那它会精确地告诉我们边界在哪里。两种结果都有价值，所以值得单独开一个实验，而不是顺手做。

6. **训练守则是否可成体系？** E-2 给出一条反直觉规则。超拟合任务是否还有其他与统计学习相反的守则（LR 表、初始化、正则）？

### 6.5 复现

```bash
python3 -m unisa train              # 对照臂，14 个网络；不在发布路径上
python3 -m unisa acc                # FULL gold 穷举判定，逐头精度
python3 -m unisa dump-weights --dtype i8
```

环境：Python 3.14.7 / 单核 / 仅标准库 / macOS arm64。训练耗时随机器变动，**权重字节不变**。

---

### 6.6 先行研究与定位 [R]

完整调研见 `research/prior-art.md`（含 16 篇已下载论文与逐条负面证据）。这里只留论文写作必须遵守的结论。

#### R-1　三条**不能**再讲的话

| 曾经的说法 | 实际情况 | 必引 |
|---|---|---|
| **P-8 存在性定理是我们的贡献** | 教科书级构造。Tracr（Lindner et al., NeurIPS'23）明确把"有限定义域上的任意函数"编成矩阵查表；Omlin & Giles（JACM'96）27 年前就做了"构造而非训练 + 离散域精确性证明"。**embedding 层本身就是 one-hot × 矩阵 = 查表** | Lindner'23 · Omlin & Giles'96 · Baum'88 |
| **P-8b"精确解存在但 SGD 不可达"是论文里最硬的一块** | 是一个**成熟的定理家族**，不是开放问题 | Shalev-Shwartz et al. ICML'17 · Shamir JMLR'18 · Malach & Shalev-Shwartz'20 · Barak et al. NeurIPS'22 |
| **穷举验证是我们的方法创新** | 方法已被做过，对象正是 ACAS Xu：输入量化 + 状态枚举，对浮点误差免疫 | Jia & Rinard, SAS'21 |

另外：**P-8a / P-8c 就是（多值）两级逻辑最小化**——Masek'79 证 NP-complete，Espresso（Brayton et al.'84）是工业启发式，Allender et al. JCSS'08 给了难近似结果。把它们写成"开放问题"会被认为没读文献。正确写法是：*我们把 h_min 归约到最小 DNF 项数，并用 Espresso 给上界*。
**整数 / 二值 / 2 的幂 / 无乘法**全部是 BNN·INQ·ShiftCNN·DeepShift·LogicNets 的成熟技术。

#### R-2　体积主张必须撤回到"接口"而不是"字节"

ACAS Xu 那条线已经走完一整个循环：Julian et al.（DASC'16）用 45 个小 DNN 替掉 2 GB 查表 → 催生 Reluplex（Katz, CAV'17）与整个 NN 验证领域 → **Bak & Tran（NFM'22）证明该压缩不安全**（在网络上验过的性质对原表不成立）→ **Boniol et al.（NFM'26）用 BDD 对同一批表做了精确无损压缩**，并直接论证 NN 方案"引入逼近误差、且使形式化验证复杂化"。

我们自己的 E-3′ 已经证伪过"神经比表省空间"。结论：**任何以体积为卖点的段落都会被 NFM'26 一篇打掉**。B-2a 的四列基线（裸表 / 训练权重 / 构造权重 / tcc）保留，但**主张改为接口统一性**：11 个异构决策点收敛到**一个整数 kernel**，可度量的是 kernel 行数、接口数量、决策点数——不是字节数。

> 审稿人一定会说"每个隐单元一个 key，那网络就是那张表换了个编码"。他说得对。唯一站得住的防线是**接口熵**，不是压缩、不是学习、不是泛化。

#### R-3　我们**可能**是新的四条（调研范围内未找到先例）

1. **系统层面的组合**：一个自举的真实 C99 编译器，**全部** 10 个表形状决策点由同一 kernel 的不同权重实现，全域枚举等价，**全链路无回退**，产出 6 目标可执行文件。这是**工程/系统贡献**，不是理论贡献。
2. **"无回退"作为硬契约**（P-1 / P-1a / P-1b）。learned systems 文献里**回退普遍存在**——learned index 有 last-mile search，learned Bloom filter 有 backup filter，ACAS Xu 有安全网，Neo/Bao 有重写规则——而 algorithms-with-predictions 从理论上说明了为什么必须有。把"无回退"写成规格条款、并据此让 **P-5 由同余直接成立（不需要对走查器归纳）**，未找到先例。
3. **嵌模型的自举不动点**：自举编译器和可复现构建都是老传统，但"**编译器带着自己的神经权重逐字节自举**"未找到先例。
4. **correctness-critical vs performance-critical 的分类轴**：ML-for-compilers（MLGO、Ithemal、CompilerGym、Neo/Bao）调的全是**启发式**，错了只是慢；我们动的是**正确性**，错了就是错。这个区分显然，但未见有人把它作为立论的组织原则。

#### R-4　定位与投稿

> 我们不是在展示神经网络能做编译器的决策；**那是 1996 年就证明了的**。我们展示的是：**当一个系统的决策点被刻意限制成有限离散全函数时，学习系统一贯需要的"模型 + 纠错回退"结构可以被完全取消，正确性义务从统计问题塌缩成模型检验问题**——而这件事在一个自举的、六目标的真实 C99 编译器上端到端成立。

- **不适合** NeurIPS / ICLR：理论部分全是已知结果。
- **适合**：CC / CGO / Onward! / OOPSLA 的 experience / artifact 方向；或 NFM / CAV 的 case study（ACAS Xu 的对照在那里最锋利）。
- 最小引用集见 `research/prior-art.md` §(d.5)。

---

## 7. 附录

### 7.1 构建顺序

```
M1 数学与网络     rng → linalg → net → gold → train      ⟦A-12, A-14⟧
M2 权重落盘       uns1 + quant                            ⟦A-11, A-13⟧
M3 tape 与 VM     tape → vm                               ⟦手写 tape 跑通⟧
M4 C99 前端       pp → lex → parse → sema → ir            ⟦七样例出 tape 且 VM 正确⟧
M5 Lowering       lower → exec_target                     ⟦A-5, A-8⟧ ★主目标
M6 镜像           emit_x86/arm → image/*                  ⟦A-9, A-10⟧
M7 Ship           ship + kernel/unisa_boot.c              ⟦A-15, A-16, B-2⟧
M8 验收与度量     tests/acceptance.sh + bench             ⟦B-1..B-6⟧
```

### 7.2 三条红线

1. **fold 不许退化成「通用 tape 跑六遍自比」**（X-4）—— 那就不是测试
2. **acc 不达标不许加宽网络**（F-5）—— 是 key 编码错了
3. **走查器 / 符号表 / 文件头不许神经化**（T-1）—— 它们是代数，不是表

### 7.3 版本沿革

v1 到 v3.4 的逐版钉死条目，连同 14 阶段之前的模型总表，已归档到
[`archive/prd-history.md`](archive/prd-history.md)：那些数字记录的是当时的口径
（`OPS` 38、9 头、TYS 8→9、selfgap 31/73 等），与今天的代码不符，留在正文里只会
被当成现状读。

**此后的变化**按实验条目记在 §6：浮点 [E-54]、自举闭环 [E-55]、编译即运行与单文件
打包 [E-56]，以及表形决策的对齐（`regmap`/`tyinfo`/`pfconv` 三个新阶段、`isel`
退出 lowering、`abi` 去 `tls` 加 `nrreg`/`arg3..5`）。

### 7.4 仍可重选

§3.3 bilinear 形式 · `SYMS` 拼写 · fib/ptr/struct 常量 · §3.7 亚字节打包顺序。**除此之外全部承重。**
