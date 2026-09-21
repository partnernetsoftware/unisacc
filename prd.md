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
| **stage（阶段）** | 一个表形状的决策点，各由一个独立小模型作答。共 10 个：pp lex parse type scope irsel isel abi enc reloc（外加 `combo`，是 isel+abi 的替代装配）。清单见 §3.0 |
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
| `crossnative.sh` | **非本机目标**真实执行：两个 Linux 目标进本地虚机，osx/x86_64 走 Rosetta 2 | [A-27] |
| `fat.sh` | **一个文件两条 ISA**，两个 slice 都真跑（arm64 原生 + x86_64 经 Rosetta） | [A-28] |

| ID | 条款 |
|---|---|
| **U-1** | `unisa run file.c --fold` 是产品本体；权重缺失时首次运行自动训练 |
| **U-2** | `--drive`（默认 `spec`）：`gold` 强制全部走表；`spec` = isel∘abi 两个 StageNet；`combo` = 单个 UnisaNet 供 9 头；**`built` = 构造出的整数网络**（K-5，精确性由构造保证，无 readiness 门禁）。前端表不受 spec/combo 影响，但 `built` 覆盖全部阶段 |
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

全部 11 个模型共用同一个 kernel（[K-1]），只有形状和权重不同。

| 模型 | 工作流位置 | key 空间 | 行数 | emb 维 | 层形状 | 头(类数) | 交互项 | seed | θ |
|---|---|---|---|---|---|---|---|---|---|
| `pp` | 前端 · 预处理 | `dir(9)×defined(2)` | 18 | 6 | `12` | `y(4)` | — | 23 | 274 |
| `lex` | 前端 · 词法 | `c(11)×peek(11)` | 121 | 6 | `12` | `y(10)` | — | 31 | 418 |
| `parse` | 前端 · 语法 | `nt(5)×tok(68)` | 340 | 8 | `16` | `y(35)` | — | 13 | 1451 |
| `type` | 前端 · 类型 | `t1(13)×op(19)×t2(13)` | 3211 | 8 | `32` | `y(14)` | — | 17 | 1622 |
| `scope` | 前端 · 作用域 | `ctx(6)×kind(5)` | 30 | 8 | `16` | `y(7)` | — | 19 | 479 |
| `irsel` | 前端 · IR 选择 | `family(5)×flavor(38)` | 190 | 8 | `16` | `y(34)` | — | 11 | 1194 |
| `enc` | 后端 · 编码形式 | `op(46)×os(3)×arch(2)` | 276 | 8 | `16` | `y(5)` | — | 29 | 893 |
| `reloc` | 后端 · 重定位 | `kind(3)×arch(2)` | 6 | 6 | `8` | `y(3)` | — | 37 | 161 |
| `isel` | 后端 · 指令选择 | `op(46)×arch(2)` | 92 | 12+8 | `24→16` | `form(5),symbol(62)` | — | 3 | 2628 |
| `abi` | 后端 · 调用约定 | `op(43)×os(3)×arch(2)` | 258 | 12+8+8 | `32→20` | `sysno(56),arg0(18),arg1(18),arg2(18),ret(18),tls(4),gate(5)` | bilinear 8d | 5 | 4421 |
| `combo` | 后端 · 合一（替代 isel+abi） | `op(43)×os(3)×arch(2)` | 258 | 16+8+8 | `48→32` | `form(5),symbol(59),sysno(56),arg0(18),arg1(18),arg2(18),ret(18),tls(4),gate(5)` | factor 12d+bilinear 8d | 7 | 10397 |

> 本表由 `unisa/gold.py` 的 `STAGES` 派生，行数与 θ 为实测值（`unisa train` 末尾那张表）。
> 词表扩张会同时改行数与 θ；`type` 的 9 个类型里 `i16` 是 2026-09-19 补的（E-31）。

**装配方式**（`--drive`，[U-2]）：

| 模式 | 使用的模型 | 合计 θ |
|---|---|---|
| `spec`（默认） | pp lex parse type scope irsel enc reloc **isel abi** | **11,892** |
| `combo` | pp lex parse type scope irsel enc reloc **combo** | 13,584 |
| `gold` | 不用模型，全部查表（训练中途仍可编译，见 [O-1]） | 0 |

`combo` 是 `isel`+`abi` 的**替代**而非追加：同一 key 空间 `op×os×arch` 上的 9 个头合一。
两条路径在完整 gold 上逐 key 同类（[D-7]），所以 `--drive` 不改变编译结果，只改变由谁作答。

**前端 6 个** 把源码走到通用 tape，**后端 4 个** 把 tape 铺到 6 个目标。
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
| **S-1** | isel | emb op12+arch8，h1=24 h2=16，heads form/symbol/gate，seed 3 |
| **S-2** | abi | emb op12+os8+arch8，h1=32 h2=20，heads sysno/arg0-2/ret/tls，bilinear 8d，seed 5 |
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

**G-2 type** — TYS(13) × TOPS(19) × TYS(13) → TYS|illegal，**3211 行**

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
| **O-2** | 逐阶段统计 net / gold 用量，`unisa run` 末尾打印 `nets: k/10 driven` |
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

**C-8** 本节是 isel / abi / enc / combo 的**唯一真源**；9 头 gold 必须由 `(op, os, arch)` 上的函数派生（G-9），`SYMS`/`SYSNOS` 即它吐出的值。

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
| **A-12** | `unisa acc` → **每阶段 = 1.000**，验的是**出货的构造权重**（0.05 秒，4,560 key 全枚举）；`--trained` 才看 SGD 对照组，且**不作门槛** | F-3, P-3, U-5 |
| **A-13** | `unisa quant` → 每个出货阶段在其记录 dtype 下 argmax 不变 | Q-6, P-4 |
| **A-14** | 构造两次 → `built.uns2` 字节相同。**套件不再训练**：训练是分钟级满核工作、不在出货路径上，曾把套件变成两小时的活 | D-3, D-6, U-5 |
| **A-15** | `unisa ship` → kit 四件套齐全 | Q-10 |
| **A-16** | Python kernel 与 `unisa_boot.c` 在 FULL gold 上逐 key 同类 | K-4 |
| **A-25** | `tests/corpus.sh` → `wrong = 0`，且 `pass` 不低于 `tests/corpus.baseline` | P-6 |
| **A-26** | 在**真 Linux 内核**上执行发出的 ELF（不是解释它）。默认由本机 Lima 虚机承担（[A-32]），GitHub 的 runner 只在把目录挪回来之后作为无尘室复核 | I-1, I-12, X-3 |
| **A-27** | `tests/crossnative.sh` → lnx/x86_64、lnx/arm64、osx/x86_64 在真机上与解释器逐例一致 | I-12..14, X-3 |
| **A-28** | `unisa fat` 产出 Mach-O universal，两个 slice 都执行且与解释器逐例一致 | I-19 |
| **A-32** | `tests/linux.sh` → 整套套件在**本机虚机的真 Linux 内核**上跑过（2026-09-21 实测：`difftest 78/0`、`tools 11/11`、`corpus 209`，全绿）。虚机把仓库**只读**挂载，而套件要写（`build_ref.sh` 在源码旁边生成 `unisacc.c`），所以先把树拷进客机再跑；`corpus/` 软链回挂载点，因为套件只读它。**Linux 走 Lima 而非 UTM**：UTM 里那两台 Linux 没装 QEMU guest agent，`utmctl exec/file/ip-address` 一律失败，网络 Shared 无端口转发、MAC 不进宿主 ARP 表，所以也没有现成 SSH 路；Windows 那台**装了** agent，所以 UTM 管 Windows。GitHub 的 workflow 被**挪出 `.github/workflows/`**（放在 `workflows-disabled/`，GitHub 只读前者，所以推送、PR、手动一概点不着）：runner 是计费的，macOS 那两个按 10 倍计价，而这台机器上 macOS（原生）、Linux（Lima）、Windows（UTM）**三个平台都有**，六个目标全都能真跑。**按下推送就烧一次额度**，等于把钱花在这台机器不花钱就能做的事情上 | A-26, A-27 |
| **A-33** | `tests/stages.sh` → 每个探针上，Python 前端问过的每个决策阶段，C 前端也问过。**所有别的套件量的都是答案**，而一条与表一致的手写规则答案与表完全相同 —— 只有这一条能看见「决定是不是网络做的」 | T-2, E-52 |
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
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。11 个 stage，4,560 key 全枚举，零分歧 |
| **S-2 目标** | 一条 tape → 六份镜像，stdout/exit 一致 | 六个目标**在真机上**逐例一致 | **已达**。`unisa fat` 是**一个 OS 内**的多架构；cosmo 式同时是 ELF/Mach-O/PE 的单文件**未开工** |
| **S-3 语言** | C99 子集覆盖别人写的代码 | 外部语料 `unsupported 0`、`wrong 0` | **已达当前语料**。`corpus 220 pass 209 wrong 0 unsupported 0 knownfail 11`。缺**浮点**（整根轴）|
| **S-4 产品** | 对标 tcc：能编译常见 C99 工具；真正自举 | ① 工具语料棘轮（**未建**）② unisacc 自己造出自己的可执行文件 | **最远**。见下 |

**S-5 为什么"推到 100%"没有理论风险。** [P-8] 的构造式存在性定理给了上界 `h = |K|`，所以"能不能到 100%"**不是开放问题**，开放的只有最小性；[F-5] 说到不了 1.000 是 **key 编码错了**，不准加宽网络；[D-7] 说网络与 gold 不一致是**其中之一有 bug**，不是模型方差；[D-1] 推理期无 RNG。合起来：**这个项目里任何一处"差一点"都是缺陷，没有一处可以赖给随机性**。剩下的全是确定性工程量，可枚举、可验收。

**S-6 自举是“够自己用”的自举，不是追平 —— 现在前端已经追平。** [A-23] 的不动点 B=C=U 是**自我复现**。前端这一层已追平（2026-09-22，E-53）：unisacc 编译全部 83 个探针且答案全部一致，接受 211/220 语料，Python 前端通过的它都接受。**但它仍只出 tape**，lowering、两个编码器与三个镜像格式仍是 Python，所以准确的说法仍是：**unisacc.c 能把任何 C 程序编成 tape**，不能独立造出可执行文件。真正的 100% 自举 = 把 lowering、编码器、镜像格式也搬进 C —— 那是搬运，不是设计（`catalog` 已是唯一真源，`ckernel.py` 已会把模型与 kernel 发成 C）。这一层的仪表是 [A-29]。

**S-7 "能编译常见 C99 工具"的卡点是形态，不是语言特性。** 按"离判据多远 ÷ 成本"排：

| 序 | 事项 | 状态 |
|---|---|---|
| 1 | **自举差距上棘轮** —— 止血，否则后面每一步都在加深 | **已做**，[A-29] |
| 2 | **多翻译单元** —— 不必做目标文件格式与链接器 | **已做**，[W-14] [A-30] |
| 3 | **libc 地板** `<ctype.h>` `<limits.h>` `<assert.h>` + `exit` | **已做**，[W-15] |
| 4 | **运行期格式串的 `printf`** —— `<stdio.h>` 本来就有完整的 `_u_vfmt`，缺的只是把非字面量格式串从内建路径放行 | **已做**，[W-9] |
| 5 | **工具语料棘轮** —— 真实库代码，多文件，自带已知答案测试 | **已做**，[A-31]，三个仓 `pass 11/11`，见 E-45、E-46 |
| 6 | **自举前端追平**（由第 1 项驱动） | **已达**（2026-09-22）：probes 83/83、`ccrun` 83 一致 / 0 已知分歧 / 0 拒绝，语料接受 211/220（覆盖 Python 前端通过的全部 209），词法器 83/0，两平台全绿，见 E-47..E-53 |
| 7 | **自举闭环**：lowering/编码/镜像进 C | 未做 |
| 8 | **浮点** | 另行开题 |
| 9 | cosmo 式三格式单文件 | 野心项，不在关键路径 |

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
| **P-3** `net ≡ gold` 可判定 | **已证明**（全域枚举，4,560 key，0 分歧） | E-5 |
| **P-4** 量化等价可判定 | **已证明**（全域枚举，含 K-3 整数算术） | E-6′ |
| **P-5** 组合等价 `C[N] ≡ C[O]` | **已证明**（由 P-3 + P-1 同余，无需对 C 归纳） | §1.4 |
| **P-7** 目标等价 | **实证**（7 例 × 6 目标全同；三种故障注入均退化到 4/6） | E-17 |
| **P-2** key 全域性 | **未证明** —— 运行时无条件断言。**唯一需要对走查器做真推理的义务** | §1.4 · O-3 |
| **P-6** `gold ≡ C99` | **本实验不声称**。已九次由实现暴露缺陷（3 次在 gold、6 次在走查器），而 acc 全程满分。外部语料给出可测数字：220 例 pass 209 / wrong 0 / unsupported 0 | E-2 · E-15 · E-16 · **E-30** |
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

#### E-38　解释器比 CPU 宽容：`.write` 会冲掉 ABI 参数寄存器，而 tape VM 不建模这件事

| 栏 | 内容 |
|---|---|
| **命题** | 给 `printf` 补上宽度/标志/精度（`%5d` `%-5s` `%05d` `%.3s`）时，左对齐那条路径把长度留在 `r1` 里跨过一次 `.write`。**解释器给出正确答案，真机给出错误答案**（`[42     ]` 而不是 `[42   ]`）|
| **机理** | lowering 把 `.write` 变成真 syscall，参数按目标 ABI 放进 `x0/x1/x2`（= tape 的 `r0/r1/r2`），**这三个寄存器在 gate 之后就没了**。而 `vm.py` 把 `.write` 当成一条不碰寄存器的原子指令，于是"跨 write 保住 r1"这种代码在解释器里合法、在 CPU 上不合法 |
| **两处都修** | ① 发射器改成把长度**压栈**而不是留在寄存器里；② **让解释器不再宽容**：`.write` / `.print` / `.sys` 之后把 `r0` 设为返回值、`r1`/`r2` 写成 `0xDEAD5EA1DEAD5EA1`。任何依赖它们的代码现在在解释器里也会立刻坏掉 |
| **意义** | 与 E-33 同一类，但方向相反：那次是**解释器不建模页保护/两操作数 ALU**，这次是**解释器不建模调用约定的破坏性**。规律是一样的——**参考实现的忠实度上限就是它建模了多少机器事实**，而每一条没建模的事实都是一个"本地通过、真机失败"的坑。修法也一样：不是只修发射器，而是**把那条事实补进参考实现**，让它今后自己会抓 |
| **状态** | **已证实**（2026-09-20），固化为 [TP-6] |

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


#### E-53　自举前端追平收官，以及只有「写一个没人写过的探针」才看得见的错

| 栏 | 内容 |
|---|---|
| **结果** | `ccrun` **83 编译且全部一致**、wrong 0、**knownwrong 0**、**refused 0**（上一程 61 / 2 / 17）；[A-33] stages 83 一致；unisacc 接受语料 **211/220**，覆盖 Python 前端通过的全部 209 个。macOS 与 Linux（Lima）**两边全绿**，[A-23] 不动点成立 |
| **补的语言面** | 结构体按值传递与返回、`(*fp)(x)` 与对任意值的调用、函数指针作为值（`*fp` 就是 `fp`）、嵌套声明符（返回函数指针的函数、函数指针数组、指向数组的指针、抽象声明符）、位域、VLA（块尾与 break/continue 归还栈）、复合字面量、按花括号层级的初始化器与指定符、匿名成员、块作用域的结构体标签、三维数组、宽字符串、真正的整数常量表达式（原来没有优先级：`N + 1 * 2` 算成 `(N + 1) * 2`）、libc 头文件按需引入 |
| **① 指针运算按字节走** | C 侧 `p++` 前进 **1 字节**、`*(p++)` 读 8 字节、`p += n` 不缩放、`p - q` 不除；Python 侧 `p - arr` 答字节数（数组操作数没有被当作它退化成的指针）。**两个前端各错一半，而且一个探针都没有抓到** —— 此前所有探针的指针运算都只作用在 `char *` 上，步长恰好是 1。新探针 `b_ptrarith` 对拍系统 `cc` |
| **② Python 前端的结构体布局不是平台的** | 成员按**大小**猜对齐：8 字节的 int 结构体、`int[3]` 都被放到 8 字节边界，平台 28 字节的结构体这里是 36。布局经由 `sizeof`、偏移与每一个指向结构体的指针都**可观测**。改用成员类型自己的对齐；新探针 `b_layout` 对拍 `cc` |
| **③ 局部聚合的部分初始化没有清零** | Python 前端对局部 `{...}` 从未执行 6.7.8p21。解释器的栈是新的、恰好全零，于是 difftest 永远看不见；原生镜像读到的是 `__init` 留下的东西。同一处还有两个引用了未定义名字的分支（走到就 NameError） |
| **④ 原生编码器没有 `.zero`** | 两个 ISA 都落到 `brk`/`ud2`。没人发现，因为 Python 前端从不对局部发它，而 unisacc 的 tape 只被解释 |
| **⑤ `__init` 缓冲区 64 KB 且不检查** | c-testsuite 00205 一个初始化器写 92 KB，越界写过符号表 —— **只在 Linux 上**表现为 `unknown identifier`（gcc 与 clang 的全局布局不同）。这是 CLAUDE.md 里「macOS 绿不等于绿」的又一例 |
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


#### E-42　语料清零：宽字符串与变长数组

| 栏 | 内容 |
|---|---|
| **结果** | `corpus 220   pass 209   wrong 0   **unsupported 0**   knownfail 11`。外部语料里不再有“前端拒绝”的程序：每一个要么通过，要么在 knownfail 里写明为什么不追（浮点、GCC 扩展、C11、以及 [G-2] 那条设计偏差）|
| **宽字符串** | 源文件按 latin-1 读（一字节一字符），所以 `L"你好"` 到了词法器手里还是 UTF-8 字节串——**只有词法器知道它是一个字面量**，所以解码必须在那里做。token 的 **kind 仍是 `str`**：文法区分不了它们，不该为此给 parse 表的 token 轴加一类；区别在**值**是码点还是字节。`wchar_t` 在所有目标上都是 4 字节（Windows 自己是 2，但我们不调 -W 入口），**否则同一份 tape 就不能 lower 到六个目标** |
| **VLA** | 存储不能在固定帧里，于是在**声明处**从 tape 栈上切；两个隐藏槽存基址与字节数（`sizeof` 于 VLA 是**运行期 load**）。关键是回收：块退出要把 SP 放回去，`break`/`continue` 也要，否则**循环里的 VLA 会把栈吃光**。一遍扫描下没法在块首插指令，但不必：**第一个 VLA 动 SP 之前的 SP 就是入块时的 SP**（其余局部全在固定帧里），就地保存即可 |
| **真正的坑** | `int a[1 && 1]` **不是** VLA。我们的常量折叠器不认识 `&&`/`||`/`?:`/比较，于是把它当成了变长的；而该程序恰好 `goto` 跳过声明处——基址槽永远没被写过，直接访存出错。**“什么不是 VLA”比“什么是”更难，而且错得悄无声息** |
| **意义** | unsupported 归零后，`unsupported` 这个桶从此是一个**真正的警报**：再出现一个，就是新的覆盖缺口，而不是一堆旧欠账里又多了一笔 |
| **状态** | **已证实**（2026-09-21）|


#### E-41　gold 第四次错：移位的结果类型，以及宏参数不该被加括号

| 栏 | 内容 |
|---|---|
| **命题** | `<<`/`>>` 的结果类型被当成了“通常算术转换”。C99 6.5.7p3 说的是：两边各自做整数提升，**结果是提升后的左操作数的类型**——右边多宽都不影响。这是 gold 第四次被实现暴露出错（E-2、E-15、E-16 之后），而 `acc` 全程 1.0000 |
| **宏参数** | 同一个语料程序还抽出了一条更早的错：我们给**每一个宏参数都加了括号**，理由是“走查器没有重扫描”。真预处理器从不加括号，而且加了会**改变语义**：`(T) 1` 在 T 为类型名时变成 `((T)) 1`，那根本不是一个 cast。改成逐 token 原样代入后，69 个探针与 207 个语料程序**一个也没变**——说明那个括号从来没有救过什么，只是在等着坑人 |
| **没改的** | 00200 最终还是 knownfail，因为它紧接着测 `sizeof(expr + 0)`，而 [G-2] 让我们**把 int 算术放在 64 位里算**。那是一条写在规格里的设计决定，不是一个 bug；把它记在 knownfail 里并指回 [G-2]，比假装没看见诚实 |
| **意义** | [P-6] 的又一个计数：**`acc = 1.000` 只能说网络等于 gold，不能说 gold 等于 C99**。能看见这一层的只有参考编译器和别人的代码 |
| **状态** | **已证实**（2026-09-21）|


#### E-40　位域：没有地址的左值，和一个藏了很久的 sizeof 偏差

| 栏 | 内容 |
|---|---|
| **命题** | 位域不是“又一种成员”，它是**一种没有地址的左值**。走查器里的左值一直是一对（ACC 里的地址，`self.lval` 里的类型），位域要求它变成三元组 |
| **做法** | 把 `lval` 改成 property：**任何人设置它都会清掉位部分**，只有字段访问会在设完之后把它装回去。这把“两个字段必须同步”从 38 处约束变成了 5 处（产生左值的地方），而不是每个 `self.lval = None` 都得记得配一句 |
| **读写** | 读：把字段**左移到字顶再移回来**——它自己的最高位落在算术右移会复制的位置上，符号问题就自己回答了（无符号用 `lshr`）。写：读-改-写，并且**赋值表达式的值是回读出来的那个**，不是赋进去的那个 |
| **enum 位域** | 符号性是 implementation-defined。像 gcc 一样：**没有负枚举值就是无符号**。语料 00218 正是专门测这一条的：148 存进 8 位位域，若按有符号读回来就是 -108，switch 会静默走错分支。为此 `enum` 得按 tag 记住“有没有负值” |
| **顺手抓到的** | 结体尺寸**没有向对齐向上取整**：`struct { int a; char b; }` 算出 5 而不是 8。它一直错着，因为**没有一个探针去比过 `sizeof`**；一写位域探针就立刻暴露。这是 E-30 那条经验的另一面：**不被读取的量就不会被发现是错的** |
| **范围** | 读、写、复合赋值、前后置自增、常量与运行期初始化、无名填充与宽度 0 的对齐断点都做了；`&` 于位域按 C99 6.5.3.2p1 拒绝。六个目标 6/6，与 `cc` 逐字一致 |
| **状态** | **已证实**（2026-09-21）|


#### E-39　零散角落扫完：剩下的不是一堆 bug，是三处“用枚举形状代替文法”

| 栏 | 内容 |
|---|---|
| **命题** | 外部语料从 196 推到 **206/220，wrong 0，unsupported 4**。掉下的那一批失败**不是互相独立的**：它们抑制在三个地方，每一处都是我们把一条**递归文法**写成了**形状列表** |
| **一：declarator** | `declarator()` 原本是一串 `if`：“`(*f)(...)`”、“`(*p)[n]`”、“`int ()`”……每发现一种写法就加一条。C99 的 declarator 是**可任意嵌套的**，所以列表永远差一条。改成真递归（每层返回一个 wrapper，由外层喂给它）后，**四个语料程序同时过了**，`abstract_type()` 也不再是第二份重复实现。唯一留下的特例是“**最外层具名** declarator 不吃自己的参数表”，因为那是 [W-3] 的 `after_name` 要决定的事 |
| **二：预处理器的 token 粒度** | `##` 被实现成“把宏体从 `##` 处切开、两半拼起来”。但 `##` 接的是**两个 token**：`A ## B+` 在 B 为空时该得到 `A` 和一个独立的 `+`，切半的写法把它们粘成了 `++`。同一处还缺两件：**参数先展开**（除非它是 `#`/`##` 的操作数）——这才是 `XSTR(x) STR(x)` 惯用法成立的原因；以及 `#x` **刚造出来的字符串也要被保护**，否则 `#define VER 3` 会把 `STR(VER)` 变成 `"3"` |
| **三：初始化器的花括号省略** | 数组长度靠数顶层逗号——但 C99 6.7.8p17 允许聚合元素**从平铺列表里取走自己那份**，于是 `PT a[] = {1,2,3, 4,5,6}` 被数成了六个元素而不是两个。顺手抓到一个潜伏很久的：**尾随逗号也被数成了一个元素** |
| **四：没有链接器不等于没有库** | 一个不 include `<string.h>` 就用 `strlen` 的程序（即大部分真实 C 代码）之前直接被拒。现在前端在发现未定义函数时，把我们自己 `include/` 里定义它的头**追加到源码末尾重编一次**——调用在单元末尾才解析，所以追加就够，用户的行号也不动 |
| **意义** | 与 E-30 同一条经验的继续，但更具体：**自有探针会把“我们实现的形状”误认为“文法”**。外部语料的价值不在于多抳出几个 bug，而在于它让“这里其实有一条递归文法”这件事**变得便宜到可以被看见** |
| **状态** | **已证实**（2026-09-21）|


#### E-36　一个文件两条 ISA：Mach-O universal 已经能跑，两个 slice 都真执行

| 栏 | 内容 |
|---|---|
| **命题** | 「综合多 ISA 可执行体」的第一步不是 cosmopolitan 式的跨 OS 文件，而是**一个 OS 之内**的多架构容器。macOS 的 Mach-O universal 就是它，而且本机两个 slice 都能真跑 |
| **证据** | `unisa fat examples/fact.c -o fact` → `file` 认作 *Mach-O universal binary with 2 architectures*；`./fact` 走 arm64 原生、`arch -x86_64 ./fact` 走 Rosetta，两者都输出 120。`tests/fat.sh` 在 49 个探针上**两个 slice 各跑一遍**，0 不符（[A-28]）|
| **成本** | 40 行：大端 slice 表 + 页对齐拼接。两个 slice 本身就是已有的 `osx/x86_64` 与 `osx/arm64` 镜像，一字未改 |
| **意义** | 这条把「多 ISA」从纸面主张变成可执行文件，而且**验证方式是执行而不是读头部**。同时它划清了边界：这是**一个 OS**内的多架构，cosmopolitan 那种同时是 ELF/Mach-O/PE 的文件是另一个问题，我们还没做（各自的 win 镜像已经能跑，见 E-35）|
| **状态** | **已证实**（2026-09-19）|

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


#### E-34　同一份字节，Darwin 25 跑得动、Darwin 23/24 崩在 dyld 里

| 栏 | 内容 |
|---|---|
| **命题** | 开发机（macOS 26 / Darwin 25）上 44/44 原生通过的 Mach-O，在 GitHub 的 `macos-14`（Darwin 23）与 `macos-15`（Darwin 24）上**全部 SIGSEGV**。签名有效、`otool` 读得出、dyld 把四个段都映射上了 |
| **诊断** | 猜了三轮都不对，改成让 CI 直接给出事实：`DYLD_PRINT_SEGMENTS` + `lldb --batch -o run -o bt`。栈顶是 **dyld 自己**：`dyld3::MachOAnalyzer::forEachRebase_Relocations`，`EXC_BAD_ACCESS (address=0x48)` |
| **机理** | 我们的镜像是 `MH_PIE`，却既没有 `LC_DYLD_INFO` 也没有 chained fixups。dyld 因此回落到**旧的重定位路径**，去读 `LC_DYSYMTAB` —— 而我们从没发过这条命令，于是 `locreloff`（结构体偏移 0x48）从空指针上读。Darwin 25 的 dyld 提前短路了这条路径，23/24 的没有 |
| **修复** | 发三条**全零**的 `LC_DYLD_INFO_ONLY` / `LC_SYMTAB` / `LC_DYSYMTAB`（+8 字节 NUL 字符串表）。镜像 +152 字节，dyld 改走 opcode 路径、发现无事可做。[I-15] |
| **意义** | 两条。其一：**"在我的机器上能跑"对二进制格式尤其不可信**——加载器的容错随版本变化，而我们手写的头部正好落在容错带里。其二：这个 bug **完全在我们的代码之外**，症状是我们的进程崩溃——没有 lldb 的回溯，任何数量的猜测都到不了 `forEachRebase_Relocations` |
| **状态** | **已证实**（2026-09-19） |

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

#### E-4　确定性可完全达成，且代价为零

| 栏 | 内容 |
|---|---|
| **命题** | 两次独立训练产出**逐字节相同**的 11 个权重文件；net 与 gold 在全部 **2,300 个 key** 上零分歧 |
| **证据** | `train --out /tmp/w1; train --out /tmp/w2; diff -r` → 11 files identical（`parse.f32.unisa` sha1 `02f4a8b9…`）。Oracle 双驱动对拍 2300 keys / 0 mismatch |
| **机理** | 随机性只存在于 mulberry32 的初始化与 `seed^epoch` 的 batch 置换；gemv 累加顺序固定；推理无 RNG、argmax 平局取最小下标 |
| **意义** | 反驳"神经网络不可用于编译器因其不确定"的通行直觉。不确定性来自**工程选择**（并行归约、非确定性 kernel、采样解码），不是神经网络的固有属性 |
| **状态** | **已证实**（D-6 / D-7） |

#### E-5　可判定性重构了证明问题

| 栏 | 内容 |
|---|---|
| **命题** | 逐阶段等价 `∀k ∈ K_s : argmax(N_s(k)) = G_s(k)` 在有限闭域上由穷举**完全判定**，无需任何逼近论证 |
| **证据** | `unisa acc` 即该判定过程本身；域大小 6–960 行，总计 2,300 key |
| **机理** | 超拟合把定义域钉成完整笛卡尔积，`K_s` 有限、闭合、可枚举 |
| **意义** | 论文可主张：**在有限决策域上，神经组件的验证不是统计问题而是模型检验问题**。PAC bound / Lipschitz 常数 / 鲁棒半径在此全部不适用且不必要。唯一穷举帮不上忙的是 key 全域性 [P-2]——它需要对经典走查器做可达性推理 |
| **状态** | **已证实**（P-3 已 discharge） |

#### E-12　表乘性增长 / 网络加性增长，交叉点在 **128 目标**；但固定宽度下容量不守恒

| 栏 | 内容 |
|---|---|
| **命题（前半，成立）** | key 空间按目标数扩张时，裸表体积**乘性**增长，网络参数**加性**增长；实测交叉点 **128 个目标**（表 18,848 B vs 网络 17,354 θ），480 目标时网络已小 **3.13×** |
| **命题（后半，证伪）** | 但**同一套架构在 24 目标 / 912 行上只到 0.4419**，不到 1.000。加性 θ 增长若以精度崩塌为代价，则前半无意义 |
| **证据** | 合成目标扩张探针（`scratchpad/scale.py`，额外目标为结构派生的**合成**目标，仅用于测参数规模，不声称其正确性）：<br>`6→0.09x  24→0.29x  64→0.74x  96→0.92x  128→1.09x  192→1.62x  480→3.13x`<br>定宽训练：`6 目标 1.0000 / 12 目标 1.0000 / 24 目标 0.4419` |
| **机理（推测，待验证）** | `sysno` 头的类数随 os 数**线性**增长（≈ 19×\|os\|），且该映射本身无结构可共享——本质是纯记忆。头矩阵 `h_last × \|vocab\|` 因此持续变大，同时隐层宽度没变，导致容量先于参数量见底 |
| **意义** | E-3 的软肋（网络打不过裸表）**在大 key 空间上可翻转**，但必须补上一条真正的规模律：**达到 1.000 所需的 θ(n)**，而不是固定宽度下的 θ。这才是论文该给的曲线 |
| **状态** | **部分证实 + 部分证伪**；细化见 E-13 |

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
| **状态** | **已证实**；紧致化见 E-20 与 P-8a/b/c |

#### E-29　自举阶梯第一级：unisacc.c 的词法器，两种构建方式行为一致

| 栏 | 内容 |
|---|---|
| **结果** | `unisacc.c` = 生成的 core（模型 blob + 整数 kernel）+ 手写的编译器本体。当前本体是**词法分析器，其每一个决策都经 `infer(S_LEX, …)`**——与 Python 前端用的是同一张表、同一个 kernel。<br>**cc 构建**与 **unisa 自己构建**的两个二进制，在 28 个源文件上与 Python 词法器**逐 token 相同**（`tests/selfhost.sh`，固化为 [A-20]） |
| **规模** | `unisacc.c` 880 行；unisa 编译耗时 **0.14 s**，产出 5,328 条指令、**100% 编码**、1.1 MB 镜像 |
| **意义** | 这不是"C 写了个词法器"，而是：**编译器的 C 实现与 Python 实现共用同一份学出来的决策表**，且 C 实现可由编译器自己编译。命题里「换权重即换能力」在两套宿主实现上同时成立 |
| **阶梯** | ① 决策层自编译（E-28）✅ → ② 词法器 ✅ → ③ **预处理器** ✅ → ④ 语法分析器 → ⑤ 发射器 → ⑥ 完整自举 |
| **③ 预处理器** | `#ifdef/#ifndef/#if/#elif/#else/#endif/#define/#undef` 由 `infer(S_PP, …)` 决定，含嵌套层栈与"已取过分支"标记；跳过的行原地涂白，行号与偏移不变。**44 个源文件（含带指令的）两种构建方式与 Python 前端逐 token 相同** |
| **为此补齐的编译器能力** | 数组维度接受**常量表达式**（`macname[MAXMAC * 32]`），含 `+ - * / % << >> & ^ \|`、括号、`sizeof(T)`、enum 常量——此前只接受字面量，unisacc.c 直接编不过 |
| **⑤ 语法分析器 + 发射器** | `unisacc.c` 现在是一个真编译器：递归下降镜像 Python 走查器，每个产生式选择走 `infer(S_PARSE, …)`，产出 tape 文本。**22 个程序由 unisacc 编译、经 `unisa vm` 运行，输出与 Python 编译器逐字相同**（`tests/ccrun.sh`）。覆盖：函数与递归、全局/局部/数组/指针、`if/else` `while` `for` `do-while` `break` `continue`、`+= -= *= /=`、`++ --`、三元、逗号、位运算与移位、enum、宏常量与常量表达式、`printf` 的 `%d %c %s %%`、系统调用内置 |
| **⑦ 完整自举 ✅ —— 不动点达成** | 经典三代判据：`A = cc(unisacc.c)` → `B = A(unisacc.c)` → `C = B(unisacc.c)`，**B 与 C 逐字节相同**（sha1 `31c28069…`，23,864 行）。并且 `U = unisa(unisacc.c)` 产出的编译器编译 unisacc.c 得到**同一份字节**——即 Python 驱动与外部编译器构建出的是**同一个编译器**。固化为 [A-23]（`tests/bootstrap.sh`）|
| **⑥ 自编译 ✅** | `unisacc unisacc.c` 产出约 23,900 行 tape，**该 tape 就是一个可用的编译器**：它编译 `fact/fib/hello/a_arith/a_while/a_str` 全部正确（`tests/gen2.sh`）。<br>**第二代 = 由第一代编译出的 unisacc**，两代在测试语料上输出一致 |
| **压垮第六级的最后一个 bug** | **全局数组只分配了 8 字节** —— 我为了标记"数组"复用了 `declptr`，于是 `.bss g_src 8` 而非 `1024`，下一个全局 `nsrc` 直接压在源码缓冲区上，第 8 字节起的源码被整数覆写。症状是词法流从第 8 个字符起错乱，极难从上层观察 |
| **④ 基础设施：argc/argv** | 自举要求编译器接受文件名，所以 `argc/argv` 从加载器一路打通：tape 得到 `.argc`/`.argv`；解释器与目标机把参数串物化进内存；原生侧在入口把 loader 交付的 `x0/x1` 存进数据段（Darwin 约定），`argv[i]` 是一次 `LSL #3` 的移位加法加两次取址 |
| **为此修掉的六个缺陷（每一个都是自举暴露的，difftest 探针集想不到）** | ① **全局数组不退化为地址** —— `__read(fd, src, n)` 把源码文本的前 8 字节当指针传了过去<br>② **tape 文本格式把 `;` 当注释** —— 二进制字面量里就有 0x3b，整行被截断（`_strip_comment` 现在是引号感知的）<br>③ **`decode()` 不认 `\xNN`** —— 模型 blob 全成垃圾<br>④ **`es()` 绕过输出重定向** —— 全局初始化器的代码被劈成两半<br>⑤ **函数序言用 r1/r2 当临时寄存器** —— 而它们正是 arg1/arg2，≥3 参数的调用全错<br>⑥ **`char *` 的存储宽度被当成元素宽度 1** —— 只存了指针的一个字节 |
| **状态** | **全部七级已证实。`用 unisacc 编译 unisacc.c 得到等价` 成立。** |

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

#### E-24　UNS2：构造网络的专用格式，全部权重 2,745 B / 默认路径 1,870 B

| 栏 | 内容 |
|---|---|
| **动机** | UNS1 存稠密张量，而构造网络**不是稠密的**：`W1` 是二值且一个单元完全由"每个 key 字段一个字面量位掩码"刻画，`b1` 可由掩码恢复，`W2` 是稀疏小整数。按张量存等于存一堆零 |
| **格式** | 头 16B `UNS2`；每阶段存 `h0/H/字段长度`，`W1` 为 `H × h0` 位（单元主序）的字面量掩码，每个头存 `H` 位的存在位图 + 每个存在单元的 `(类下标, 权重)`，位宽按该头实际最大值取。**`b1` 不存**。无时间戳、无字典序依赖 ⇒ 字节可复现 [D-5] |
| **实测** | `pp 52 · lex 80 · parse 297 · type 183 · scope 61 · irsel 182 · enc 73 · reloc 44 · isel 382 · abi 500 · combo 875`<br>**全部 2,745 B**；**默认 `--drive spec` 路径 1,870 B** |
| **对比** | JSON 缓存 16,933 B（**6.2×**）· 量化训练 kit 23,640 B（**8.6×**）· 裸 bit-packed 决策表 3,010 B（**1.10× 更小**）|
| **备注** | 本实现只用"逐字段位掩码"一种编码；逐字段择优编码（掩码 / 2-bit tag / 共享字面量表）可再降到约 1,374 B，暂未实现 |
| **状态** | **已证实** |

#### E-23　构造后端已落地：编译器跑在纯整数权重上，且比训练路线快 18×

| 栏 | 内容 |
|---|---|
| **落地** | `unisa build-weights` 构造 11 个整数网络（255 单元）并验证全域精确；`--drive built` 让整条编译器跑在其上。七个样例 **全部 6/6**；三种故障注入仍全部退化 **4/6**（fold 的判别力不依赖权重来源） |
| **外延相等的最强证据** | 训练权重与构造权重编译出的镜像**逐字节相同**（lnx/x86_64、osx/arm64、win/x86_64 各验）。这是 [D-7] 在产品层面的确认：两条路线不是"精度都很高"，是**同一个函数** |
| **速度** | 逐阶段决策吞吐（冷，无 memo），构造 / 训练：`combo 76.6k / 3.3k = 23.0×` · `abi 141.8k / 7.5k = 19.0×` · `isel 329k / 20.4k = 16.2×` · `enc 1.06M / 69.9k = 15.2×` · `parse 670k / 43.6k = 15.4×` … **整体 18.3×** |
| **B-4 的意外结论** | 预算是 Python ≥ 50k dec/s。**训练路线在 `abi`(7.5k) 与 `combo`(3.3k) 上不达标**；构造路线全部阶段 76k–1.29M，**全数达标** |
| **B-5** | 端到端编译 `fib.c`：训练 0.006 s / 构造 **0.001 s**，均远低于 1 s |
| **意义** | 构造路线不是"精确但慢而大"的学术选项——它在**精确性、体积、速度**三项上同时优于训练路线，唯一的代价是构造耗时约 6 s（结果可缓存，且确定性） |
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
| **修正** | E-19/E-20 用的 796,490 θ 是**稠密**计数，对一个二值且每列至多 m 个 1 的 `W1` 严重高估。**以按位打包体积为准** |
| **意义** | ① 部署 kernel 可收紧为**无浮点、无乘法、int16**；② 之前"构造贵 36×"的结论作废，构造在真实存储上**更省**；③ E-7（q2 全线失守）的对照更清楚：量化是把任意实数往格点上凑，而构造权重**本来就在格点上**，不存在量化这一步 |
| **上界性** | 项数来自朴素推导（`type` 917 项 / `abi` 1,428 项），最小覆盖优化后应显著下降。**当前数字是上界** |
| **状态** | **已证实** |

#### E-20　构造与训练互有胜负，最小性是真正的开放问题

| 栏 | 内容 |
|---|---|
| **命题** | 精确解可由构造直接给出（P-8），11/11 全部验证通过、零训练零随机；但**训练得到的解普遍比构造小得多**，两者互为三明治的上下界 |
| **证据** | 逐阶段 θ（训练 / 构造 / 比值）：<br>`pp 274/224 0.8×` · `reloc 161/63 0.4×` · `scope 479/513 1.1×` · `lex 418/990 2.4×` · `isel 2328/18624 8.0×` · `irsel 953/7830 8.2×` · `enc 829/9506 11.5×` · `parse 1288/15456 12.0×` · `combo 10053/447219 44.5×` · `type 801/37597 46.9×` · `abi 4361/258468 59.3×`<br>合计 21,945 / 796,490 |
| **机理** | 表小则构造更省（无需 embedding 与隐层的连续参数化开销）；表大则 SGD 大幅胜出，因为梯度下降会**发现跨规则的共享结构**，而基于规则的构造逐条落项、不共享 |
| **意义** | 修正了一个容易犯的叙述错误：不能说「构造优于训练」。正确表述是**两者各自给出精确解集的一个可行点，且位于最小性谱系的两端**。真正的贡献点因此从「能否 100%」（平凡，P-8）转移到「**最小的精确解有多小、能否确定性地构造出来**」（P-8a/b/c） |
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
python3 -m unisa train              # 18.8s，11 网络全部 1.000
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

`archive/prd.v1.md` 原始规格 · `v2.1` 展开版（英文） · `v2.2` 精炼版 · **v3 分层重编号**。

**v2 钉死**（按会先咬人的顺序）：LR 是 epoch 衰减表而非参数量分档 · `h0=52` 给出五段分解 · 9 头连同词表枚举 · `OPS` 固定 38 且顺序写死 · "FULL gold" 定义为完整笛卡尔积 · parse 覆盖规则拆行（v1 原文无法无歧义切词）· type 的 `1/19 illegal` 解读为训练下采样 + 全量评估 · enc/reloc/irsel 默认分支补全 · 补齐 connect/bind/listen/accept 的 win 符号 · r0–r7 寄存器映射与 MOPS 的 `none` 约定 · 完整列出 tape 指令集 · `--fold` 明确为逐目标 lowering **且**目标感知解释 · 加入反向对照 · 补 Mach-O/PE 魔数、退出码、`--drive`、`--holdout` 语义、Oracle 分发 · 选定 fib/ptr/struct 预期输出。

**v2.1 新增**：确定性契约 [D] · 超级拟合确立为目标、`SHIP_ACC=1.000` [F] · q4/q2 dtype 与按行 scale [Q] · argmax 不变性作为量化唯一判据与逐阶段混合 dtype [Q-6..Q-9] · 体积/速度预算与 tinycc 对比 [B]。

**v3 新增**：§1.4 证明义务与接口约束 [P] —— Oracle 不透明性、key 全域性、五层义务分层；全文条款编号；四层重排；验收清单挂条款依赖 [A-5..A-16]。

**v3.1 新增**：§6 实验发现 [E] —— 面向论文的知识沉淀章节，五栏固定格式（命题/证据/机理/意义/状态），已录 E-1..E-5 已证实、E-6..E-11 待验证、E-2′/E-3′ 已证伪、四条开放问题。据实测修正两处：G-2a 的 1/19 下采样已废止（E-2），B-2a 明确体积对比基线（E-3）。并修正 v2 的计数笔误 PRODS 34→32。

**v3.2 新增**（2026-09-19）：§6.6 先行研究与定位 [R-1..R-4] —— 据 `research/prior-art.md` 撤回三条与文献重复的主张（P-8 存在性、P-8b 难度、穷举验证方法），把 P-8a/P-8c 从"开放问题"改回它们本来的名字（两级逻辑最小化），并把体积主张整体换成**接口统一性**。新增外部语料验收 [A-25] 与真内核执行 [A-26]；新增宿主契约 [I-12]（ELF 的 rw 段）；`type` 表补入 `i16`（`short`），TYS 8→9、行数 1216→1539。§3.2 的模型总表改为由 `unisa/gold.py` 派生的实测值。

**v3.4 新增**（2026-09-21）：**§5.5 完成度盘点** [S-1..S-8] —— 四层目标、各自的可测判据、实测数字与卡点排序；**libc 地板** [W-15]（`<ctype.h>` `<limits.h>` `<assert.h>` + `exit`/`abort`）；**多翻译单元** [W-14] [A-30] —— `unisa compile a.c b.c` 无链接器、无目标文件，同一个 walker 走完全部单元；文件作用域 `static` 按文件改名，单文件 tape 逐字节不变。**自举差距上棘轮** [A-29] `tests/selfgap.sh`：unisacc 自己的前端只接受 111/220 语料与 31/73 探针，而这个差距此前**没有任何仪表**（见 E-43）。

**v3.3 新增**（2026-09-21）：**六个目标全部真机执行** —— E-35 改写为已证实，两个根因（`MajorSubsystemVersion` 10.0 把加载器拨到严格路径；tape SP 在 Win64 是 volatile，`winstdh` 必须先于 `spinit`）；[I-16] / [I-17] 改正 —— 撕掉之前“`.reloc` 与 load config 对 arm64 PE 必需”的错误推论，并记下那条方法学教训；`tests/crossnative.sh` 接入 UTM 里的 Windows 11，两个 win 目标每次随套件真跑（虚机不在则跳过）。

### 7.4 仍可重选

§3.3 bilinear 形式 · `SYMS` 拼写 · fib/ptr/struct 常量 · §3.7 亚字节打包顺序。**除此之外全部承重。**
