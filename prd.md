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
  -W* -w -g -f* -std=* -pipe -m*   接受并忽略：一种方言、无独立调试信息
  -O0 / -O / -O1 / -O2 / -O3 / -Os   tape→tape 优化档位（H1–H4；-O0 与缺省 walker tape 逐字节相同；出货构建用 -O2）
  -v              编译后打印每个阶段被问了多少次（仪表，不是 verbose）
  -version / --version   版本串
  --check-oracle  枚举全部问题，核对缓存与网络逐个一致 [A-49]
FILE.tape         输入是 tape 时直接进后端
```

与 cc 明确不同之处：`-c` 不产目标文件（与 `-S` 同义，写出 tape）；`-g` 不产调试信息；找不到的 `#include` 是错误（C99 6.10.2p4），以前是静默跳过。`-O*` **会**改写 tape（见 H1–H4），不是接受并忽略。

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

全部模型共用同一个 kernel（[K-1]），只有形状和权重不同。

决策点表**不再手抄在这里**：原先这里的 14 行手写快照（合计 6,650 行、413 单元）自称“生成”，实际已经与真表不符。bdy-ds4flash 审阅（2026-09-26）指出了这一点。生成的表见 `README.md`、`prd.tree.md` 和 `prd.map.md`，由 `unisa/docgen.py` 写出，`tests/docs.sh` 检查。按 `unisa/gold.py` 的 `ALL`，共 **18 张，8,484 行，513 单元**；C 编译器实际询问其中 16 张（isel 和 combo 不在路径上）。

> 生成的表**由 `unisa/gold.py` 与发布的构造权重生成**（`python3 -m unisa acc` 是同一批
> 数字）。手抄快照漂过四次，所以 prd 不再抄；训练 θ 属于对照臂，见
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
| **K-4** | Python 参考实现 `unisa/linalg.py`，C 部署实现 `kernel/unisa_core.c`，两者在 FULL gold 上必须逐 key 同类 |

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

**Q-10** kit = `weights/*.unisa` + `MANIFEST.json` + `kernel/unisa_core.c` + 镜像。

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
**W-9**（2026-09-26 修订：printf **一律是对 `<stdio.h>` 里 printf 的普通调用**。C 前端见 991d337，调用时自动带入该头；Python 前端见 18c8f22，由 driver 补头，脱糖代码已删。C 前端的 `do_printf` 脱糖只在单元里**没有** printf 声明时兜底，也就是 `-nostdinc`。两个前端的 tape 不要求一致：closure 比的是同一条 C tape 过两个后端，ccrun 只比运行结果。下面是修订前的原文。）`printf` 在走查期按**静态格式串**脱糖（`%d %s %c %u %%`），这是快路径也是常见路径。**格式串不是字面量就根本脱不了糖**，此时放行到 `<stdio.h>` 里真正的变参 `printf`（它和 `fprintf`/`sprintf`/`snprintf` 共用同一个运行期格式化器 `_u_vfmt`）。一个坑：调用在**走查末尾**才解析，所以调用可以先于定义——对普通函数无害，但这几个函数的**调用约定不同**（参数全压 tape 栈，[W-13]），后到的定义救不回已经按另一种约定发出去的调用，所以它们的变参性写死在 `VARIADIC_LIBC` 里。`%d` 经发射的 `__itoa` 助手（纯 tape op，故可原生编码），`%s` 经 `__strlen`。
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
| **A-12** | `unisa acc` → **每阶段 = 1.000**，验的是**出货的构造权重**（8,484 key 全枚举，18 个阶段；2026-09-26 实测）；`--trained` 才看 SGD 对照组，且**不作门槛** | F-3, P-3, U-5 |
| **A-13** | `unisa quant` → 每个出货阶段在其记录 dtype 下 argmax 不变 | Q-6, P-4 |
| **A-14** | 构造两次 → `built.uns2` 字节相同。**套件不再训练**：训练是分钟级满核工作、不在出货路径上，曾把套件变成两小时的活 | D-3, D-6, U-5 |
| **A-15** | `unisa ship` → kit 四件套齐全 | Q-10 |
| **A-16** | Python kernel 与 `unisa_core.c` 在 FULL gold 上逐 key 同类 | K-4 |
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
| **A-44** | `tests/c99.sh` + `tests/c99/` → 每个 C99 特性一个探针，与系统 `cc` 对拍**输出**（编译通过不算通过）。分母写自标准自己的变更清单，**不写自我们的支持范围**——分母跟着分子动就什么都没量到。`corpus`（基线 216 通过、4 个登记的已知失败）太宽容：那批程序又短又重叠 | A-25, S-10 #9 |
| **A-45** | `tests/bench.sh` → 编译耗时作棘轮：编译自己（707 KB）、一个小探针、以及**出货二进制与 cc -O2 构建之比**。速度从来没被量过，所以可以随便烂掉——一个每 token 调用一次、每次从头走表的查找，把 57% 的自编译时间放进了 strlen，而没有任何仪表会发现。基线按机器分文件，容差 30%。**一次耗时为零的测量等于什么都没跑**，所以有下限检查：这个套件的第一版把 "command not found" 的 15 ms 当基线记了下来 | S-10 #10, #11 |
| **A-46** | `tests/fuzz.sh` + `tests/gen_prog.py` → 从种子生成随机 C 程序，与系统 `cc` 对拍**输出**。别的套件测的都是**有人想到的东西**；活下来的缺陷在**没人组合过的组合**里。生成规则必须让每个程序行为**有定义**，否则两边有权不同、套件就只是噪声——其中一条规则是用一个 bug 换来的：**循环计数器在循环体内只读**。第一版允许循环体给它赋值，种子 2 生成的程序在 cc 下也跑了二十秒以上；一个会写出不终止程序的生成器，量的是超时不是编译器 | A-17, S-10 |
| **A-47** | `tests/hostile.sh` → 编译器遇到**意料之外的输入**时必须**退出**：截断的文件、未闭合的注释与字符串、include 循环、5000 字符的标识符、嵌套一千层的括号、二进制垃圾、嵌入的 NUL、自我展开的宏、一个目录当输入。判据分两类：(1) 不许崩、不许挂；(2) C 说无效的东西**必须被诊断**——静默接受非法 C，等于产出一个作者从没写过的程序。当天就抓到两个：未闭合注释被静默吞掉，`struct S { struct S inner; }` 被静默接受（`sizeof` 是半成品表项里碰巧的值）。两个都已修 | A-40, S-12 |
| **A-48** | `tests/docs.sh` → `prd.tree.md`、`prd.map.md`、`README.md` 里的阶段表由 `python3 -m unisa docs` 从 `unisa/gold.py` 与构造权重**生成**（标记区 `<!-- stages:begin -->`…`<!-- stages:end -->`），过期或标记丢失即失败。由来：阶段表在四个文件里各手抄一份，11→14 重构后其中两份照旧描述 11 阶段的编译器整整一天（`type 960`，实为 4,275），没有任何机制发现 | S-10 #5 |
| **A-49** | `unisacc --check-oracle` → 模型能被问到的**每一个**问题（2026-09-26 实测 20,184 个问题，0 分歧）经缓存问一遍，与网络直接作答逐个比对，正序、逆序各一遍（驱逐与碰撞取决于顺序）。由来：oracle 缓存的第一版存的是问题的**哈希**、比的也是哈希；全定义域上有两个问题撞同一个哈希，命中时就静默交出另一个问题的答案。哈希计算还有有符号溢出（UB），clang 与 gcc 在 -O2 下算出不同的值，于是错答案在两个编译器之间**挪位置**——Linux CI（gcc）拒绝了一个 Mac（clang）能编的 C99 程序。把比较退化回"只比部分字段"时，这条报出 6,646 个错答案 | A-33, P-3 |
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
| **B-4** | 决策吞吐 | `unisa bench`，逐阶段，冷 | Python ≥ 50k/s；`unisa_core.c` ≥ 5M/s |
| **B-5** | 端到端编译 | `unisa run examples/fact.c` | ≤ 1 s |

**B-6** `unisa bench` 必须报**冷**数据。在确定性全函数上加 memo 缓存是正当工程手段，`run` 可开；但 `bench`/`acc`/`quant` 必须关——否则测的是 dict，不是 kernel。

### 5.5 完成度盘点 [S-*]

盘点分四层，因为**难的部分和多的部分不是同一部分**：论点层基本做完了，产品层才走了一半。每层的"100%"是一条可测的判据，不是一个百分比。

| 层 | 目标 | 完成判据 | 实测（2026-09-21 起，2026-09-25 复核） |
|---|---|---|---|
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。18 个 stage，8,484 key 全枚举，零分歧（2026-09-25 复核） |
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
| 11 | **像 tinycc 一样可用**：自带头文件、`-I`、`-D`、shebang | **已达**（2026-09-23）：头文件嵌在二进制里（2026-09-25 为 19 个；实现私有名改为保留的 `__u_*`，用户宏不再能打坏头文件），任意目录可用；`unisaccrun` 并入 `unisacc`，产物是单文件 `unisacc.com`。见 E-59 |

**S-15 0.0.7 计划**（2026-09-24 定，2026-09-24 按摸底扩充）。

**0.0.7 切版（2026-09-25，主人定：按已落地内容切版）。** 进 0.0.7 的是下表标“已达”的各项，加上 J9 提速（自编译 0.777 → 0.646 s）、`-O2`、两处误编译修复（三维数组下标、经函数指针调用的结果类型）、libc `fwrite` 修复、头文件私有名修复。未完成的 A1、A2、B2、B3、C1、C2、F2、G1–G3、H0、H3（旗标）、H4、J2–J4、J6、J7 **移到 0.0.8**，本表保留原文作为 0.0.8 的起点。产品边界不变：`-c` 输出 tape 而不是 .o，没有链接器、`-l`/`-L`；多文件在一次调用里编译。

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
| D2 | **函数** —— **部分已达**（2026-09-24）：`qsort`（堆排序，最坏情况有界）`bsearch strtok strncat sscanf`（d i u x o c s f e g、h/l、宽度、`*`、`%n`）`perror strerror atexit labs llabs div ldiv`。`atexit` 要求 main 返回也算 `exit`：两个前端的入口存根在 `<stdlib.h>` 定义了 `exit` 时经它返回（C 前端把 `_start` 的收尾挪到所有单元走完之后才知道有没有）。**顺带**：第一次写文件作用域的函数指针数组（atexit 的表）就发现 C 前端把它当一个 8 字节标量按字节索引——`tests/c/b_fparr.c` 固定。**2026-09-25 补齐**：`fseek ftell rewind remove rename`——catalog 新增 `lseek unlink rename`（Linux/arm64 无 unlink/rename，号是 unlinkat 35 与 renameat2 276，目录 fd 由 lowering 补，renameat2 的第五参数经新的一次性通道进 x4），Windows 门为 SetFilePointer（DWORD 符号扩展）、DeleteFileA、MoveFileExA(REPLACE_EXISTING)，BOOL 换成 POSIX 的 0/-1；两个前端的 `__lseek __unlink __rename`，VM 同步。探针 `60_fseek_remove_rename`：c99 56/56；**真机**：osx/arm64、lnx/arm64（Lima）、win/arm64 与 win/x86_64（UTM）在 -O0/-O2 下都与 cc 一致；六目标 closure 逐字节；abi_audit 22/0。`getenv` 也已达：环境在 Unix 内核放的位置——argv 的 NULL 之后，`__argv(k)` 越过 argc 就读到；`-run` 把编译器自己的环境接在 runargv 的 NULL 后，VM 同样；Windows 上答 NULL（写明）。探针 `61_getenv`，c99 57/57；osx/arm64 原生与 -run、lnx/arm64（Lima）与 cc 一致。**顺带**：新套件 `kernel` 重新生成 kernel/ 下的生成文件并要求与提交的一致——改了 include/ 没重新 emit，出货编译器带的是旧 stdlib.h | ✅ 每个已做函数有探针且对 cc 一致；tools 11/11 不退 |

**E. 文档与论文（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| E1 | 论文：刷新数字；新增一节"**gold 本身会错**" —— **已达**（2026-09-25）：表 2 按当日实测（15 阶段、corpus 216、C99 57/57、闭环 564/564、编译器自身 867 KB/2,079 符号、各 -O 级 279/279、两前端优化 190/190、出货 4.0×）；新增 §6.2（优化器也走表：H1 结构 + `peep` 表，pow2 改表的实例）与 §8.1（gold 本身会错：[G-2] 与 gold_audit 1,399/1,400） | ✅ |
| E2 | 论文表改为生成区 —— **已达**（2026-09-25）：表 1（阶段、键字段、键数、单元、**全域枚举精度**）由 `unisa docs` 生成进 `<!-- stages-zh -->` 区，`docs.sh` 过期即失败。表 2 的套件结果要跑全量才能得到，不放进 60 s 的检查，按日手刷 | ✅ 表 1 过期即失败 |
| E3 | prd §2 补**出货编译器**的 CLI 契约 —— **已达**（含 `-O0/-O1/-O2` 与 H1–H4 对齐；不再写「`-O` 忽略」） | ✅ 与 `tests/cli.sh` 对表 |

**F. 发布与分发（P1，含主人决定）**

| # | 事项 | 完成判据 |
|---|---|---|
| F1 | `make release` 自动起停两台虚机 —— **已达**（2026-09-25）：`tests/vms.sh up|down`，release.sh 在 STRICT=1 全量之前 up、`trap` 在退出（含失败）时 down；**只停自己起的**，原本在跑的保持原样；Windows 以 guest agent 应答为“起来了”，每步有界。实测：Lima default 原本在跑→未动；UTM Windows 起→应答→停。顺带：release 的 `.com` 改为 -O2 构建，与 `make com` 一致，界 1800→60 s | ✅ 一条命令从起机到关机，失败也会关机 |
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
| H0 | **基线** —— 出货二进制编译自己 / cc -O2 构建编译自己 | 不量不算 | bench 记录：2026-09-25 为 1.0 s / 0.09 s ≈ 11×（-O0）；`make com` 改为 -O2 后 bench 量出货构建：**4.0×**，基线 229 → 122 ms 已记录 |
| H1 | **栈顶缓存** —— **第一步已达**（2026-09-25）：`-O`/`-O1`/`-O2` 生效（`-O0` 与缺省的 tape 与此前逐字节相同）。C 前端在所有单元走完后做 tape → tape 改写：`.frame 8; store64 [r7+0], rX; S; load64 rY, [r7+0]; .frame -8`，当 S 是只含显式操作数的直线代码（21 个操作的白名单）且不提 rY、r7 时，改为 `mov rY, rX; S`（S 空且 X=Y 时整对删去），迭代到不动点。`-O2` 另加：中间段用到了 rY 时借 r3–r5 中一个**死**寄存器承载（`mov rZ, rX; S; mov rY, rZ`）。死的判据来自 walker 的一个可逐 tape 检查的性质：r3–r5 在块内先读后写只出现在函数序言的参数保存处，所以它们在块边界都是死的，除非进入一个把它作参数的函数；call 跨过（被调者不收它为参数时）。两个后端吃同一条优化后的 tape，closure 照旧。死寄存器由**块级活跃性**判定（按标签与跳转切块，call 读 Z 当且仅当被调者入口块 Z 活跃，白名单外的操作视为读全部寄存器，求最小不动点；每轮先把每行预处理成类别与读写掩码）。`-O2` 还把局部变量读取 `imm r2, N / sub64 rD, r6, r2 / .ld rD, [rD+0], W` 在 r2 死时合成 `.ld rD, [r6-N], W`（8,216 处）。实测：编译器镜像 **974,370 → 693,666 B（−29%）**，-O2 编出的编译器编译自己 **1.04 → 0.47 s**，剩余 push/pop 约 2.2 万 → 5,466 对；-O2 编译整个编译器 0.59 s，写出的编译器与参考逐字节相同，-O2 下自身不动点成立；新套件 `opt`（31 s）：314 个探针/c-testsuite/c99 程序 -O0 与 -O2 运行一致、-O2 closure 在三个目标上逐字节、自编译与不动点——653/0 | 杠杆最大 | text −25%；bench 比值 ≤ 6×（现约 5×：0.46 s / 0.09 s） |
| H2 | **`peep` 表** —— **已达**（2026-09-25）：键 (A 类 8 × B 类 17 × 关系 10) = 1,360，动作 8 类；构造权重，**全域枚举 1.000**。结构性代码只找对子并说出关系（同槽 store→load / store→store、imm 0/1/2^n 作为第二源且该寄存器随后死、imm 后 mov 出死寄存器、跳到下一条、跳到跳转、写死寄存器），做不做、做哪种由网络答。-O2 在 H1 各轮之后跑。活跃性扩到 r0–r5（`ret` 处 r0/r1 活）。`[r7+..]` 槽不参与同槽：那是 B1 在 x86 上融成真 push/pop 的地方。**实测一次改表**：`pow2 → to_shl` 让 x86 镜像大了 24 KB（tape 的 shift 在 x86 上要经 cl），gold 改为 keep 并重建——决策在表里，量了就改表。**两个前端同问**：`unisa/opt.py` 是 C 优化器的孪生，经同一个 oracle 问 `peep`；`optpy` 套件：C 的 -O0 tape 经 Python 优化 = C 的 -O1/-O2 tape，190/190。closure、bigclosure 6/0、stages/selfhost 94、nativeboot、`--check-oracle` 16,582/0 | 表状决策，按路线走网络 | ✅ 枚举 1.000；两前端同问 190/190；closure 0 差异 |
| H3′ | **各级对 cc -O2** —— **已达**（2026-09-25）：新套件 `difftest_o`：93 个探针由 cc -O2 编出并运行作为参考（按源码+cc 版本哈希缓存），unisacc 在 -O0/-O1/-O2 下 `-run`，**279/279 一致**（冷 42 s，热 4 s） | 各级都得是同一个程序 | ✅ |
| H3 | **`-O0/-O1/-O2` 旗标生效**（今天接受但忽略） | 对接 Makefile 习惯 | 每级 closure、nativeboot N1=N2=N3、difftest 对 cc -O2 输出一致 |
| H4 | 叶函数内联、强度削减 —— **第一步**（2026-09-25）：`peep` 加两种关系、两种动作：`dest_to_mov`（A 算到 rD 只为 `mov rY, rD`，rD 随后死 → A 直接算到 rY）与 `copy_into`（`mov rY, rX` 后一条读 rY、rY 随后死 → 读 rX），键 1,632，枚举 1.000，两前端同问（optpy 192/192）。**实测**：编译器 -O2 镜像 osx/arm64 726,690 → 677,154 B（−6.8%），lnx/x86_64 629,985 → 576,737 B（−8.5%），tape 里的 `mov` 16,684 → 1,988；**自编译时间不变**（0.54 s）——乱序 CPU 上寄存器 mov 几乎免费，速度的差距在访存与调用，不在这里。回归：opt 659/0、difftest_o 282/282、closure 570/0、bigclosure 6/0、nativeboot、c99 57/57。**第二步**：局部变量写入融合（`imm r2,N / sub64 rA,r6,r2 / S / .st [rA+0],rV,W` 在 rA、r2 随后皆死时成 `S / .st [r6-N],rV,W`，读取融合的镜像），编译器 tape 里融合的写入 690 → 2,174 处，少约 2,000 行；两前端一致（optpy 192/192），回归全绿（含 corpus 216+4、datashape、fuzz） | 余下的差距 | bench 比值 ≤ 3×（现 4.0×） |

**I. 抽象与复用：把仍然手写或重复的表状逻辑收进表（P1，2026-09-25 后台勘察）**

出发点是“表状决策走网络、代码只做结构”。勘察（只读）发现四类仍是手写、且在 Python/C 或 x86/arm64 间重复的表状逻辑：

| # | 事项 | 现状（重复处） | 提案 | 风险 | 判据 |
|---|---|---|---|---|---|
| I1 | **`prec` 表**：二元运算优先级 —— **已达**（2026-09-25）：字段 op 19，头 lev 11（none, 1=`||` … 10=`* / %`），构造、枚举 1.000。实为**四份**拷贝：C 的 `bop()`/`BOP`/`BLEV` 与常量折叠的 `cprec()` 18 行比较链，Python 的 `PREC` 与 `CPREC`——全部删去，两前端按记号种类问一次并记住。**顺带修了一个 bug**：Python 常量折叠从 `&&` 那级开始，`int a[1 || 0]` 被拒（C 接受）；探针 `b_const_oror`。对 HEAD 编译器，探针/c99/corpus/编译器自身的 -O0 tape 逐字节相同；stages/selfhost 94、closure 564/0、bigclosure 6/0、nativeboot、optpy | C `bop()`/`BOP`/`BLEV`（setup_tables）；Python `PREC` 与 `CPREC`（同文件两份） | 字段 op(19)，头 lev(11)；`binop_level` 与 `binary()`/`const_expr()` 改问表，三份删去 | 低 | stages、selfhost、closure、bigclosure 不变 |
| I2 | **`opinfo` 表**：优化器的操作分类 —— **已达**（2026-09-25）：字段 op 67（tape.SHAPE 全部 + other），头 simple(2)/acls(8)/bcls(16)，构造、枚举 1.000；C 的 `ol_simple`/`pk_acls`/`pk_bcls` 与 opt.py 的 `_simple`/`_acls`/`_bcls` 改问表（每个 op 词问一次并记住），三份清单删去。对 HEAD 编译器：95 个输入 × -O1/-O2 的 tape 逐字节相同；optpy 190/190、opt 655/0 | 21 操作白名单与 ALU 列表在 C `ol_simple`/`pk_acls`/`pk_bcls`、`opt.py` `SIMPLE`/`ALU`、gold `PEEP_A/B` 三处 | 字段 op(~40+other)，头 simple(2)、acls(8)、bcls(17)、kind(8) | 低 | optpy 190/190、opt、-O2 closure |
| I3 | **irsel 补行**：除/余/无符号比较 —— **已达**（2026-09-25）：irsel 的 alu 族加 div/mod/udiv/umod；新表 `binsel`（运算符 16 × 符号 2 → alu 变体，枚举 1.000）取代 C `emit_binop` 的 23 分支链与 ir.py 的 `ALU`/`UNS` 字典及 `divmod_` 的手写选择，指针差的 `.div` 两端也改经 irsel。对 HEAD：探针/c99/corpus/编译器自身 -O0 tape 逐字节相同。回归在冻结快照上跑（新 `tests/snap.sh`）：closure 570/0、bigclosure 6/0、stages/selfhost 95、opt 659/0、optpy 192/192、difftest_o 282/282、c99 57/57、corpus 216+4 | C `emit_binop` 直写 `.udiv .umod .div .mod` 与 `@alu.ult…` 的手写 if 链；Python `parse.py` 直写 `.div` | irsel 加 binop×unsigned 的 flavor 行，两前端同问 | 低 | stages、closure |
| I4 | **abi 加头**：argshape、retconv、winimp —— **已达**（2026-09-25）：catalog 派生三个事实，abi（与 combo）各加三头，构造、枚举 1.000。`.sys` 的参数形状（Linux/arm64 的 openat/unlinkat/renameat2）由 `argshape`（plain/atfd_1/atfd_1_zero/atfd_2_zero5）决定，两个后端的按名特例删去；Windows 门拆成**主体**（每个 op 自己的参数搬运与调用）加**尾巴**（`retconv`：none/wcount/bool_inv/bool_neg/dword_sx），主调用的导入由 `winimp` 给出——C 的魔数导入下标与 Python 的字面导入名删去。`winextra` 未做：rename 的 REPLACE_EXISTING、CreateFileA 的处置参数移位只各有一处，留作结构。**内核容量**：推理内核按 s×12 为每个阶段的输出头计数编址（用移位拼，内核无乘法），abi 第 13 个头写进了下一阶段——编译器 bus error；改为 s×16（`HEADS_MAX`），生成器断言不超。这是“换表不改内核”的第一个例外：内核的**容量常数**会随表长大，逻辑不变。字节：Python 后端 291 例、C 后端六目标全部例与 HEAD 逐字节相同。**顺带**：`.com` 的启动脚本把编译器作为子进程运行，看门狗杀脚本时子进程成了孤儿（01:19 起空转 9 小时，97% CPU）；改为转发信号并以 fd 3 交出 stdin，ape 套件加两项（超时不留孤儿；stdin 到达） | openat/unlinkat/renameat2 的参数移位与第五参数通道（lower.py 与 bk_lower 各一份）；Windows 门的导入名、BOOL→0/-1、DWORD 符号扩展、CreateFileA 处置参数（四份：x86/arm × Python/C） | 门的前后序保留为结构，其余查表 | 中（Windows 只能靠 closure 与 UTM） | closure 全部 os×arch、crossnative |
| I5 | **编码规格表**：机器码模板 —— **第一批已达，其余不做**（2026-09-25）：x86 的 alu2/setcc/shiftext 与 arm64 的 alu3/invcond 成为 `catalog.ENCSPEC` 一份数据；Python 编码器读它，C 的查找由 ckernel 生成进 unisa_model.inc，四条手打 if 链删去。对 HEAD：C 后端六目标 582 例逐字节相同、Python closure 0 差异。**其余批次不做**：D/E/F/G/I 族在两端已是参数化的小函数或结构性序列，挪表收益小于改动风险；剩余重复是 Python 与 C 两个后端本身，按既定方向由 Python 退为参考实现来消化 | `enc` 只答 form；字节模板手写四份（emit_x86 约 34 例、emit_arm 约 40 例、C `bk_x86`/`bk_arm` 与 x_*/a_*）；`ALU2/SETCC/ALU3/INVCOND` 在 C 里重打成 if 链 | 一份 `unisa/encspec.py`：(op, arch) → (族, opc, cc, 固定寄存器, 宽)，族选择走网络，操作码作为生成的数据表（如 ckernel 生成 .inc）；分批：A 寄存器 ALU+C 移位 → B 比较置位 → D 访存宽度 → E/F 立即数与地址 → G 分支（碰松弛，加 bigclosure）→ I 除法 → H 浮点；J（门、itoa 等不透明块）不迁 | 中高 | 每批 closure 逐字节不变；预计 Python −150~200 行、C −200~250 行 |

**顺序**：I2、I1（便宜、低风险，先证明“表 → 权重 → 两端同问”可复用）→ I3 → I4 → I5 分批。

**J. 速度：推理与编译（P1，2026-09-25，Fable 只读评审）**

结论：自建编译器慢于 cc -O2 的 4 倍几乎全在**生成代码**（栈机临时量、局部变量的地址计算、调用开销），不在模型；-O2 的剩余耗时在**文本式的优化轮次**（每轮重切行、重建标签、重扫字符）。推理已记忆化，内核只对自建编译器的冷查询有影响。

| # | 事项 | 位置 | 预期 | 闭环风险 | 论点 |
|---|---|---|---|---|---|
| J1 | **稠密表 = 运行构造网络遍历全域生成** —— **已达**（2026-09-25）：emit-kernel 以 `intnet.predict` 遍历每个阶段全域，写出 DENSE（每键每头一个字节）；`inf` = 定义域检查 + 一次查表，记忆缓存删去；`--check-oracle` 20,184 问与 `infer()` 0 差异；192 份 tape 与 HEAD 相同。**实测速度几乎不变**（自建自编译 0.48 → 0.47 s）：原记忆缓存已够好，推理不是瓶颈——与评审判断一致；收益在于去掉冲突回退、让“表 = 网络”成为每次构建的证明 | ckernel、`inf` | 实测 ~2% | 无 | 网络的编译形态，逐项验证相等 |
| J2 | **walker 里把表达式临时量放寄存器**（右操作数为叶子时左操作数进 r3–r5），替代 push/pop | `binary`/`emit_binop` | 运行 25–35% | tape 变，Python walker 同步 | 不变 |
| J3 | **局部变量在发射时就用 `[r6-N]`** | walker 多处 | 运行 10–15% | 同上 | 不变 |
| J4 | **优化轮次改为按行记录**：kind/掩码由发射器给出，每轮只修补改写过的行 | opt_round/peep_round | -O2 编译 20–30% | opt.py 同步，文本逐字节不变 | 不变 |
| J5 | 按 cop 缓存 `bk_facts`、`pk_act` 按名字记住 —— **已达**（2026-09-25）：485 个镜像（5 目标）与 HEAD 相同；速度在噪声内（J1 之后推理本就便宜）。`@family.flavor` 预解析未做：J1 之后它只剩两次已缓存的 vfind 与一次查表 | es、bk_facts、peep_round | 实测 <2% | 无 | 不变 |
| J6 | 位切片的 `infer()`（每字段每取值一张单元位集，按位与后按优先级取胜者）作参考实现 | ckernel | check-oracle 与冷查询变快 | 无 | 同一权重的转置布局 |
| J7 | SIMD / 向量 tape 指令 | — | J1、J6 之后可忽略 | 高 | — |

**顺带修的 bug**：`--check-oracle` 的 `oracle_pass` 仍按每阶段 12 个头编址（内核已是 16），从第二个阶段起核对错位——已改为 16。另记两个前端热点：预处理在每个 `#include` 后对整个缓冲区重跑 splice/decomment（随 include 数二次方增长），`tokclass` 每次询问都查 typedef 表。

**顺序**：J1（最便宜、论点最干净）→ J5 → J4 → J3 → J2；J6 视需要；J7 不做。

**容量（J8，2026-09-25）**：编译器自身的源码已到 1,042,362 / 1,048,576 字节（MAXSRC），再过几次提交就无法编译自己。放宽 MAXSRC 1→4 MB、MAXTOK 13万→52万、MAXOUT 4→16 MB、SX_MAXOCC、MAXPOOL、MAXLIT、BL_MAX、BK_MAXI 26万→100万、BK_MAXT 52万→200万、BK_MAXN；均为静态数组，只加 bss，镜像不变，自编译内存峰值 61 MB。新套件 `scale`：自身源码、tape 文本、tape 行、tape 指令过各自上限一半即失败（提前报警）；并以 -O2 编译运行一个生成的 80 万字节、6,000 函数的程序——首跑即撞上 MAXSYM 4,096（12,000 个全局名），提到 65,536、符号哈希 SH_SIZE 提到 65,536 后 1 s 通过。继续探：3.3 MB（24,000 函数）依次撞上 MAXTOK（→ 2M）与 MAXOUT（→ 32 MB）；1.6 MB（12,000 函数）撞上后端的 BK_MAXI（100 万条 tape 指令）——再放大后端每条指令十几个 int 的数组要上百 MB 虚拟空间，不做。**设计目标**：编译器自身规模的约两倍以内（tape ≤ 100 万条指令）；`scale` 默认 6,000 函数，并对自身用量过半报警。**待探索（主人 2026-09-25 提出，未排期）**：容量现在是静态数组、只占 bss，上限固定。可做一个小实验：改为用 `__mmap` 按需申请并增长（例如 tape、token、后端指令表在满时翻倍重映射），让上限随输入伸缩、空闲时不占虚拟空间；需两前端与 -run 路径一并考虑，先量一次内存与速度再定。

**J3、J2 的重新评估**（J1、J5 之后）：两个 walker 都把局部变量的地址先算进 r0 作左值、再经 `[r0+0]` 读写；在发射时直接用 `[r6-N]`（J3）要改左值表示，两前端同改。而 -O2 的读取融合与写入融合（H4）已消去绝大多数此类访问，出货构建（-O2）上 J3 的剩余收益小，降级。J2（临时量进寄存器）同理：H1 的栈顶缓存已消去约四分之三的 push/pop。余下差距的真正来源待下一次对自建二进制的指令级剖析确定，再定 J2 的范围。

**J9. 生成代码与编译器自身的速度（2026-09-25，opus 评审之后实测）**

- 已做：arm64 的 `call`/`callr`/`ret` 改为 `bl`/`blr`/`ret x17`，使返回预测能配对。字节数不变，自建编译器的 -O2 自编译从 0.81 s 降到 0.78 s。评审按指令条数估计能快 15–30%，实测只有约 4%。
- 实测剖析（`sample` 采样，再用 Python 后端算出的标签地址归并到函数）：自建编译器 -O2 自编译的时间，**约一半在 -O2 优化器本身**（ol_*/bl_*/opt_round/peep_round），词表查找约 16%，词法约 7%，后端约 6%，推理约 2%。-O0 为 0.42 s，-O1 为 0.47 s，-O2 为 0.80 s。
- 轮次：unisacc.c 上 opt 跑 4 轮（命中 21,070、4,695、932、202），peep 跑 4 轮（17,401、1,737、14、0）。每轮都要全量重建行表、标签表和 6 个寄存器的活跃性，行数都在十万以上。
- 试过、无效、已撤回：给 ol_mask、ol_firstreg 加逐行缓存（0.79 s，没有变化）；ol_word 改为比较编号（0.94 s，反而更慢，因为原来的文本比较在第一个字节就失败返回）。
- 按段增量跑后几轮（以 ret 分段；段签名包括段内各块、以及段内每个 call 目标入口块的活跃位，精确比较）：输出与 HEAD 逐字节相同，后几轮跳过的段数为 opt 3/76/224、peep 2/197/451（共 462 段），**但总耗时不变**（0.78 s）。每轮的固定成本（建行表、标签表、6 个寄存器的活跃性、整段复制）才是大头，逐行分析不是。已撤回。
- 结论：这条路上剩下的收益要靠减少轮数本身，或让活跃性和行表跨轮增量维护。两者都会改变优化器的结构，需要单独立项；在那之前，-O2 自编译的速度维持在 0.78 s。
- **跨轮沿用逐行事实（c5c8f67，保留）。** 拆分实测（cc -O0 带符号构建，5 次采样）：优化器 1,048 个样本中，ol_prep 占 360，活跃性 145，整段复制约 113，建行表 73，其余逐行分析约 350。选 ol_prep 切入。假设：原样照抄的行，其 kind、rm、wm 只取决于行文本，可以从上一轮沿用，只有跳转和调用行的 tg 按标签重算。结果：ol_prep 0.093 降到 0.037 s；A/B 用固定输入 a39f67c:unisacc.c，两个编译器都是 cc -O2 构建后再自编译 -O2，预热后交替 7 轮，自编译中位数 0.777 降到 0.702 s（-9.7%，波动 0.769–0.782 对 0.699–0.706），tools.sh 的 11 个多文件程序 0.989 降到 0.921 s（-6.9%）。918 镜像与 a39f67c 相同；closure 570/0，nativeboot，bigclosure 6/0，opt 659/0，optpy 192/0。只测了 osx/arm64。**内存代价**：新增 6 个 OPT_MAXL 大小的 int 数组（em_pos、em_src、ol_from、pv_k、pv_rm、pv_wm），各 4 MiB，共约 24 MiB 静态虚拟地址空间；实测最大 RSS（自编译 a39f67c:unisacc.c）52,641,792 增到 56,999,936 字节（+4.2 MiB，+8.3%）。镜像字节数不变，不代表内存不变。**覆盖**：提速只归属于 osx/arm64 和上面两个负载；Linux、Windows 未测，c5c8f67 上的 acceptance、native、linux.sh、crossnative 未跑，要在下一次正式汇总或发布前、在冻结的最终树上补跑。缓存的前提：kind、rm、wm 只取决于行文本和 opinfo 表；将来若 opinfo、ABI 或消融模式在一轮之内可变，这个缓存就要随之失效。
- **重新采样后的下一项：词法器的最长匹配（c12e308，保留）。** 出货形态编译器（c5c8f67）的采样：优化器约 50%，但已分散（建行表 6.0%，复制 6.5%，活跃性 6.1%，ol_prep 4.4%）；最大的单一机制是词表查找，占 15.4%，其中约 40% 来自词法器。原因：每遇到一个运算符，都要把 TOKV 全部条目扫一遍，每条还要查 vlen 和 voff。改为按首字节预先列出候选，保持 TOKV 原序，所以选中的是同一个 token。A/B（固定输入 a39f67c:unisacc.c，cc -O2 构建后再自编译 -O2，预热后交替 7 轮）：自编译中位数 0.703 降到 0.646 s（-8.1%，波动 0.701–0.707 对 0.641–0.654），tools 的 11 个程序 0.917 降到 0.799 s（-12.9%）。918 镜像与 c5c8f67 相同；closure 570/0，stages 95，nativeboot 通过，bigclosure 6/0。lexdiff 在 examples、tests/c、tests/c99 上 150 一致、2 不一致（tests/c99 的 05_pragma_operator、53_inttypes）。c5c8f67 上也是同样这 2 个，是早已存在的差异；all.sh 不跑 lexdiff，所以一直没有暴露，待另查。
- **后续事项（cdx-unisacc 复核 c12e308 后提出，单独处理，不并入本轮）：**
  1. lexdiff 的两项差异还不能断定是词法器的错：lexdiff 同时比较预处理输出，要先分清是预处理、头文件、目标宏还是记号序列不同。
  2. `tests/lexdiff.sh` 的仪表缺口：只有 `set -u`；C 侧输出走管道，Python 侧命令替换失败后仍可能继续比较；两侧执行都没有单独的看门狗。修法：任何一步编译、预处理或词法出错都立即失败，每次执行加上不超过 60 秒的看门狗；确认后的差异转为可追踪的回归项。不要为了全绿而排除这两个文件。
  3. `pm_build` 在候选数达到 1024 时会静默漏掉候选（当前词表远小于此）。下次改动这里时，改成显式的容量失败，或按词表大小派生上限。
- 两项合计（同一口径）：自编译 0.777 降到 0.646 s，tools 负载 0.989 降到 0.799 s。只测了 osx/arm64。
- 体积（arm64，同一输入）：imm 融合与返回截断融合使自身镜像 693,666 降到 660,642 字节（-4.8%）；运行速度在噪声内。x86 没有做，要做就作为体积分支立项。

**J10. 构造离开种子（已暂停，2026-09-25）**

主人校准的目标（原话）：“应该也没有啥进化的啊，从种子层得到 unisacc 种子版本，然后种子版本能编译自己和编译其它 c99 项目，基本对标 tinycc/tccrun/cc/gcc 的核心功能应该就行了啊。”所以产品就是 `unisacc.com`：能自举（无 Python）、能编译运行 C99 程序的编译器；J10 不在产品路线上。

已达并保留为开发工具（`iterate/`，不属于产品）：`iterate/construct/` 用 C 构造 18 个阶段的 UNS2 整包，与 built.uns2 逐字节相同；`iterate/kernel/genmodel` 生成 model.inc 的 MODEL/DENSE/维度、16 个词表、BF/BH/HD 与 TYPEV，与 Python 裁判逐字节相同。未做：ENC_*、cases.inc、headers.inc、模板、来源头部；ENC 切片（c78f7d0）不合入。全部过程、判据与回执见 [`archive/prd-history.md`](archive/prd-history.md) 的“J10 全文”。

**S-16 0.0.8 计划**（2026-09-26 定；0.0.7 已于同日发布，v0.0.7，unisacc.com 1,254,432 B，sha256 00d25edb…）。

目标不变：像 tinycc / `tcc -run` / cc 那样，一个能编译自己、也能编译其他 C99 项目的编译器，达到可用于生产的级别。0.0.7 的教训（主人，2026-09-26）：`.com` 启动 1.6 s 与“不带选项输出词法转储”都不是测试发现的，而是使用时发现的——**测试套件要持续扩展，去倒逼问题**。每项带可测判据，单次运行不超过 60 s。

| 序 | 项 | 为什么 | 完成判据 |
|---|---|---|---|
| T1 | **正式的分片门禁** `tests/gate.sh` | 0.0.7 的门禁靠临时脚本，两轮约 10 分钟且有重复 | 一条命令；每片 ≤60 s；独立套件 2 路并发；整套 ≤4 分钟；`release.sh` 调用它 |
| T2 | **对出货物测试**：门禁里一组套件以 `UA=unisacc.com` 运行 | 0.0.7 的两个缺陷只在 `.com` 上可见 | cli、run、multi、diag、hostile、closure 用 `.com` 跑；另加启动时间回归（第二次运行 < 0.1 s） |
| T3 | **像用户那样用**：cc 行为对照套件 `ccparity` | 退出码、错误输出、产物命名、失败不留产物，此前无人检查 | 与 cc 对照 ≥30 例（无选项、`-o`、多个 `-o`、缺文件、空文件、无 main、编译失败、stdin、argv、退出码…），差异 0 或写明理由 |
| T4 | **平台覆盖补齐** | 0.0.7 未在 Linux x86_64 与 Windows x86_64 实跑 | Linux x86_64（Lima，模拟）每片 ≤60 s 的最小套件；Windows x86_64 实机或 UTM 跑 `.com` 与 `-b win/x86_64` 产物 |
| T5 | **UB 与实现定义的探针审计** | Linux 上两处“失败”其实是探针本身依赖未定次序与实现定义行为 | 扫描 tests/c 与 tests/c99 中同一实参列表内的多次修改、依赖 libc 实现定义行为的输出；逐个改写 |
| P1 | C1 的余项：`-c` 越界时的提示 | `-c lib.c` 现在报“undefined function main”，用户看不懂 | 提示改为说明“一次调用编译整个程序” |
| A1 | type 表对照 cc 审计（S-15 A1） | 表本身会错（[G-2]） | `gold_audit` 分歧 0 |
| A2 | syscall 号对照系统头（S-15 A2） | 手录 | Linux 两架构、macOS 分歧 0 |
| B2 | `.com` 的 Windows 部分自解压（S-15 B2） | PE 占约 62% | `.com` 降 ≥30%；两台 Windows 通过 |
| R1 | **论文 A 的推导** | 主人：理论要支撑生产级 | 论文 A 只陈述已证的（T1：网络 = 表）；[`research/delta-framework.md`](research/delta-framework.md) 是未证草稿，不进论文；推导的每一步有对应的实测或证明 |

**不进 0.0.8**：G1–G3（C11/GNU 扩展，超出 C99 边界）；J7 SIMD；J10（已暂停）；delta 框架的实现（只做理论）。F2 签名策略仍待主人决定。

**S-17 模型化迁移路线**（2026-09-26 定，主人：“直接按整个编译器规划迁移”；按 cdx-unisacc 审阅修订）。**本条覆盖 S-16 中“delta 框架的实现不进 0.0.8”一句**：迁移是 0.0.8 起的主线，S-16 的测试项作为它的安全网照做。

**目标**：每个架构的切片只是一个**几 KB** 的通用执行器（主人：先用 C 实现，再用汇编实现）；编译的动作由模型推理完成；模型在 `.com` 里只存一份。理论框架与未证之处见 [`research/delta-framework.md`](research/delta-framework.md)：已证的只有 T1（网络 = 表）。几 KB 是目标，不是定理。

**参考与证据的边界**：
- 现有手写编译器是**行为参考**，固定在一个参考提交上，不删除。
- 新旧在已测输入上的一致，只是经验性证据，**不构成**全域等价，也不构成 C99 语义正确性的证明。只有将来证明了“新旧对全部声明输入等价”，并且旧实现有相应的语义保持定理，两者组合才得到 T3；目前两者都没有。
- 旧编译器是整个参考过程，不直接就是有限的 δ_ref。每层都要说明：局部的状态、观测、动作如何定义，参考转移如何从旧过程得到。T1 只证明网络等于这张局部表，不证明“从旧过程提取表”是对的。多步动作序列要定义有限编码与解码。

**执行器**（选项 A，低层通用原语）：读字节、比较、定宽算术、栈、字典读写、输出追加、调用 δ。定宽算术必须写明宽度，以及溢出、移位、除零的规则，并且各目标一致。以下东西**不能**悄悄变成执行器的语言相关原语：宏摘要（E2）、类型与符号摘要（E3）、活跃性（E4）、模板选择（E5）、重定位布局（E6）。它们要么由 δ 调度通用原语计算（计算摘要的算法本身也在迁移范围内），要么明确列为暂未迁移的可信组件。

**模型的口径**：报告时写明实际运行的是什么——网络推理、经全域验证的 DENSE 查表、还是状态机字节码。构造成网络，不等于运行时执行了网络。

**产物账**（每个里程碑报告，压缩前与压缩后都报）：通用执行内核；OS 启动与系统调用适配；加载器与解压器；随带的 C 库；模型；模板；`.com` 总字节。编译特有的代码不能搬进模板后漏记。“几 KB”指通用执行内核，**按未压缩的机器码字节计**。目标靠结构达成——切片只做“读取 → 推理 → 执行动作”，编译的每个环节都由模型推理完成——**不靠压缩**；压缩后的字节只作参考列出。

| 里程碑 | 层 | 比较对象与判据 |
|---|---|---|
| **E0** | 执行器 + 玩具 δ（delta 草稿 §4 的 LL(1) 表达式文法） | 执行器先用 C，cc 与 unisacc 双构建；报告内核字节；再给一个 ISA 的汇编版本并报告字节 |
| **E1** | 词法 | 对 lexdiff 语料逐记号相同，含拒绝的输入与诊断；只回答词法层的规模，不外推 |
| **E2** | 预处理 | `-E` 输出、退出状态、诊断与参考提交相同 |
| **E3** | 解析与语义 | tape、退出状态、诊断相同 |
| **E4** | 优化 | -O1/-O2 tape 相同 |
| **E5** | lowering 与编码 | 六目标镜像相同；失败路径另行比较（镜像相同只覆盖成功路径） |
| **E6** | 镜像写出 | 同上 |
| **E7** | `.com` 布局 | 模型只存一份；按上面的产物账报告每一段 |

**阶段接口（2026-09-26，主人：流水线以后可能调整，接口要灵活）**：每个阶段是一对（δ，执行器），对外只有一种形状——**字节流进，字节流出**，外加一个退出状态（接受 / 拒绝 k / 未覆盖）。阶段之间不共享内存、不共享符号表；需要传递的信息必须编码进输出流（例如 E3 需要类型拼写，就由上游在记号流里带出来）。每个阶段声明自己的输入格式与输出格式（一个格式名 + 一份可执行的格式说明/检查器），流水线由一份清单（阶段名、δ 文件、输入格式、输出格式、顺序）描述，运行器按清单把阶段串起来，相邻阶段的格式必须一致才允许连接。这样增删、拆分、合并、重排阶段，只改清单和相邻两端的格式，不改执行器。现有阶段的格式：源文本 →（E2）→ 预处理后文本 →（E1）→ 带类型拼写的记号流 →（E3）→ tape 文本 →（E4–E6，待定）→ 镜像字节。

**切换规则**：指定语料与门禁通过后，才把默认路径切到新实现；旧实现保留在参考提交里，随时可对照。

**速度**：固定参考提交、相同输入、目标与优化级、相同计时边界，重复 5 次取中位数，每个里程碑照常测量并记录。**2026-09-26 主人定：速度等功能完善后再统一优化**，慢于参考 3 倍不再是停下来的门槛；E1 实测为 cc 构建执行器约慢 3.7 倍、unisacc 构建约慢 17 倍（计时边界不同，待同口径重测）。

**论文 A** 只吸收已完成、有证据的结果。

**S-17 进展与决定（2026-09-26）**

| 层 | 实际运行的形态 | 规模（实测） | 与参考的一致性 |
|---|---|---|---|
| E0 执行器 | C，通用原语 | cc -Os `__text` 4,748 B | T1 穷举通过 |
| E1 词法 | 构造式整数网络 → 全域求值的稠密表 | 335 个状态、1,215 个单元、UNS2 58 KB | 语料 243/243、lexdiff 110/110 |
| E2 预处理 | 状态机查表（Python 执行器） | 621 个状态、稀疏约 72 KB | 示例 99/107、语料 235/249，0 不一致；含函数宏、#/##、#if 求值、自动 include |
| E3 解析与 tape | 状态机查表（Python 执行器），手工逐写法铺成 | 5,230 个状态、约 130 万表项 | 随带头文件 stdio/stdlib/string/ctype/inttypes/wchar/assert 全部声明一致；语料 32/110，0 不一致 |
| 流水线 | `exec/pipeline/run.py`，E2→E1→E3 端到端 | — | fib/hello/fact 从源码到 tape 逐字节一致，不借助参考 |

**决定：E3 改为结构化生成**（主人：“不够聪明，一直在穷举，没有设计出聪明的结构”）。扁平状态机把文法、属性（类型/宽度/转换）、模板三种知识揉在一起，规模是乘积。改为三张声明数据 + 一个通用 LL 驱动：文法（产生式）、属性表（直接用 gold 的 type/tyinfo/pfconv/binsel/irsel）、tape 模板（每条产生式一段带槽位的模板，实测一次），规模变为三者之和。设计见 [`research/e3-structured.md`](research/e3-structured.md)，原型在 `exec/parse2/`；旧 E3 保留为已验证的参照，新结构在同一批输入上做到 0 不一致且更小之后才替换。

**向 tinycc 学的三点**（主人：“好的地方一定要学”）：
1. **流式衔接**：阶段按需拉取字节，不先物化整份文本（接口不变）。
2. **值描述符**：栈符号带有限的值描述（常量 / 局部槽 / 全局 / 左值地址 / 已在 r0 × 类型），模板按“运算符 × 操作数描述”选；先生成与参考相同的冗长 tape，稳定后再换成延迟落地（那时判据改为行为一致，属 E4）。
3. **回填**：执行器新增通用原语 ORES/OFILL（预留字节、之后回填）。实测参考自己就是这样做的——`.frame` 的数字总是右对齐占 7 个字符——所以不再需要把函数扫两遍。

**迁移逼出的产品缺陷（均已修复、配了与 cc 对照的探针）**：预处理嵌套宏调用丢实参、缺 hide set、`#if` 不认十六进制与括号；`!p` 的结果被当成指针；调用结果的有无符号与宽度取自最后一个实参而非返回类型；字面量到文件尾未闭合时越界读 1 字节；随带 malloc 只有 64 KB 且 free 不回收。

**已知未决**：头文件里定义的 printf——参考即使在 stdio.h 定义了 printf 仍走内建降级（每个实参占一个帧槽），多头文件时正文顺序与插入顺序相反；E3 目前明确拒绝。E2、E3 的表还不是网络（只有 E1 是）。C 执行器还没补上 E2/E3 的原语。

**结构化 E3 原型（`exec/parse2/`，2026-09-26 晚）**：模板表（实测一次的 tape 片段）+ 由 prec.tsv 生成的运算符层级 + 编译成过程的文法 + 回填帧大小。现覆盖 int 函数与参数、局部变量与块作用域、全部运算符（含 && ||）、赋值与复合赋值、++ --、if/while/for、调用、printf 内建展开：**843 个状态，探针 10 个、语料 15 个与参考逐字节一致，0 不一致**。诚实对比：旧 E3 在相近覆盖时约 1,009 个状态、语料 18 个——状态数只小一些，目前的收益主要是“知识变成数据”（模板行、由表生成的层级）和去掉两遍扫描。真正要验证的是下一步：类型（指针、char/long、数组、struct）走属性表后，规模是否按加法增长而不是乘法；若否，这个结构就没有达到目的，要如实记录。新增的实测：参考的 `.frame` 字段固定 7 个字符宽（它自己回填）；块结束后栈槽被复用，帧取最深点；常量池里 `"` 写作 `\"`、`\` 写作 `\\`，其余不可打印字节写作 `\xHH`。

**加法增长的第一个证据（2026-09-26 晚）**：给结构化原型加上类型（char/short/long/void、指针层数、& * 下标、强制转换、指针加减按被指类型缩放、按类型截断的返回与调用结果）只增加了 281 个状态（843 → 1,124），因为读写宽度由一个按“值描述”分派的共享过程决定，而不是每种写法按每种类型各铺一遍。对照：旧 E3 在加完同类功能时约 2,128 个状态。探针 17 个、语料 17 个一致，0 不一致。新实测：语句只写 `p[i];` 时参考只算地址不读取。

**结构化原型进度（2026-09-27）**：加上全局变量与数组（+163）、typedef/static/原型/名字随函数撤销与自动 include 检查（+76）后为 1,363 个状态，探针 23、语料 22 个一致，0 不一致。新实测：函数帧最终向上取整到 8；全局变量在声明处输出 `.bss g_名 大小`，int 初值统一在 `__init` 里写入；原型不产生任何输出，所以函数头与参数保存要推迟到看见 `{` 才写。下一步：struct、unsigned（随带头文件里剩下的主要卡点）。

**结构化原型进度（2026-09-27 续）**：struct、unsigned char/short/long、long 常量、系统调用内建、__argc/__argv、do/break/continue、?:、字符串字面量、变参定义与 va_*、double 存储依次接入，2,147 个状态；探针与语料合计 70 个一致，0 不一致；随带 string.h 19/19、stdio.h 44/48。唯一一处明显的非加法增长：unsigned 让每个运算符的有/无符号分支在两条层级链里各复制一次（+296），待把“选指令”收成一个共享分派。新实测：原型不占返回标签；va_arg 取 unsigned char/short 不掩码。

**结构化原型进度（2026-09-27 再续）**：函数指针、~、void return、exit 的返回路径、double（常量、四则、比较、cvtid/cvtdi）接入；随后把每个运算符的尾部逻辑收成一个被两条层级链共享的过程，**同样的输出从 2,549 降到 2,279 个状态**——这就是“结构化”的收益在起作用。当前 2,279 个状态、84 个一致、0 不一致；随带头文件 string/ctype/inttypes/time 全过，stdio 44/48、stdlib 21/40。新实测：双精度二元运算的固定形式（右值 cvtid 后压栈、左值从 [r7+8] 取回并 cvtid、mov r1,r0、.frame -16、f 指令，> 与 >= 用操作数颠倒的 flt64/fle64）；函数指针的局部被调方用 `load64 r0, [r6-N]`；本文件定义了 exit 时 __main_ret 先 `call exit`。

**工作方法**（实测教训）：需要读懂 gen.py 的改动由主会话直接做（一轮验证约 8 秒）；子代理只做测量、独立脚本和其他目录的事，用 sonnet、10 分钟以内；共享 /tmp 的产物一律按内容哈希打戳（`exec/stamp.sh`）。

**远期设想（主人，2026-09-26，明确“完全不应该急”）**：推理替代程序与函数会带来少量性能损耗；将来可对性能极敏感的少数环节，用 FFI 一类的外部调用做针对性封装。理论上的位置：被外部调用的原生代码就是 delta 草稿 §3 选项 B 的“可信组件”，必须逐个列出、计入产物账，并说明它替代了哪一段 δ 以及两者的等价证据；不能悄悄进入执行器。在功能完成、速度统一优化之前不做。

### 5.6 路线建议与完成度评估（2026-09-27，研究员估计，非规范）

来源：两位独立研究员（opus 出路线，sonnet 出评估）读了脱敏简报后各写一份报告；简报里的数字取自 cdx 的日志，我没有独立复核，研究员也没有读仓库。这里只记要点，不构成验收，也不改变 S-17 的里程碑定义。

**完成度**（下限、中心值、上限）：

| 口径 | 完成度 |
|---|---|
| a 六个目标功能跑通 | 55%、**68%**、78% |
| b 可以把默认路线切到模型路线（E7 前提满足） | 28%、**38%**、55% |
| c 原始目标全部达成（几 KB 执行器、表只存一份、编译动作全由表完成） | 32%、**45%**、60% |

九个维度（权重：完成度）：阶段覆盖 15%：85%；目标覆盖 15%：75%；未覆盖构造 20%：70%；命令行与错误信息对等 8%：65%；体积 8%：20%；速度 7%：10%；离线生成不依赖 Python 7%：10%；验证深度 8%：45%；E7 切换 12%：3%。最可能高估的是“未覆盖”和阶段覆盖，因为 130 个输入与 101 个套件都是自选样本；最能改变估计的新证据是拿一批没参与开发的外部真实 C 程序跑一次覆盖统计。按现在的节奏，各涨 10 个百分点：a 约 3–7 天，b 约 2–4 周，c 约 3–6 周；外部程序的未覆盖比例若远高于 25%，时间翻倍。

**路线建议**（4–6 周，三个阶段）：
- **阶段 A（第 1–2 周，收口）**：清零已知的“不同”（无符号按位取反的截断，按值传结构体的局部副本；该数字来自 0d3358b 之后的一次探索，是否已修复未核实）；补完 Windows x86_64 并原生三轮自举；WINARGS 要么迁移，要么写进规格登记为执行器外壳；为 32 个未覆盖输入和约 42 类构造建覆盖账本；冻结门禁数字。
- **阶段 B（第 3–4 周，验证与前置条件）**：随机生成程序对拍（目标 10 万个，“接受且不同”为 0）；每张表做拒绝完备性检查；拿系统编译器当第二真值，交叉对拍参考本身；离线生成可复现；体积和速度只定底线，不做全面优化。
- **阶段 C（第 5–6 周，E7）**：六目标合成单文件、表只存一份；先跑后台对拍的影子期再切默认；留回退开关；三个平台原生跑完整门禁。
- 最该停：以提交次数衡量进度，改为每天固定一次门禁加探索快照，看不同数、未覆盖数、目标数。可推迟：全面速度优化、去 Python 化、FX-1 到 FX-4（FX-4 在阶段 B 只做设计文档，FX-3 的 wasm 目标先排除，避免稀释收尾）。
- 最大的被低估风险：对拍的标准答案就是手写参考本身，参考里的错会被表原样复制，门禁永远是绿的。对策是系统编译器做第二真值加随机程序，并把参考路线冻结为只修 bug。

**待主人决定**（研究员的推荐）：
1. 未覆盖时切换后自动回落到参考路线？推荐是。冲突：这会把手写编译器留在产品里，与“编译动作全由表完成、只存一份”的原始目标矛盾，是方向问题。
2. 切换底线：体积不超过手写路线 3 倍、自编译不超过 5 倍？推荐是。现状：模型路线开发容器约 5.29 MB，手写路线约 1.35 MB，约 3.9 倍，按这个底线现在过不了。
3. WINARGS 是否可登记为执行器外壳常量？推荐可以。
4. 切换是否要求去掉 Python？推荐不要求，只要求生成可复现。
5. 参考路线至少保留两个版本周期？推荐是。

### 5.7 "C99 子集"措辞核查与两处未记录缺口（2026-09-27，cc-unisacc 回报；cdx 核对清单）

**背景**：README 称 "A C99-subset compiler"。核查了它为什么不叫 "C99-compatible"，兼容度多少、缺什么、缺的东西有没有排进计划。

**已有的两个量化**（现成套件，非新测）：
- `tests/c99.sh`：按标准自身变更清单写的 57 个探针，实测 **57/57（100%）**。
- `tests/corpus.sh`（c-testsuite，外部语料 220 个程序）：本轮有界分片实测 216 pass、0 wrong、0 unsupported、4 knownfail；`tests/corpus.knownfail` 的四项是两个 GCC 语句表达式、一个空结构体、一个 C11 `_Generic`。此前回报的 214/6 是旧数字。57 个特性探针和 220 个语料程序都不是 C99 完整性证明。

**"subset" 而不是 "compatible" 的三层原因**：
1. 构建系统不兼容，不是语言问题：`-c` 不产目标文件，没有链接器、没有 `-l`/`-L`（README 已写明）。
2. 明确排除了不属于 C99 的东西：C11 `_Generic`、GCC 扩展（语句表达式、空结构体、inline asm，README 已写明）。
3. 真正的语言/库缺口，下面两条由 cc-unisacc 回报；cdx 核对了头文件清单，未独立复跑三字符组探针：
   - **三字符组（trigraphs）未实现**：`??<`/`??>` 之类直接语法错误。
   - **标准头文件缺 6 个**：C99 要求 24 个标准头，现在有 18 个（`assert ctype errno float inttypes iso646 limits math signal stdarg stdbool stddef stdint stdio stdlib string time wchar`），缺 `complex.h`、`fenv.h`、`locale.h`、`setjmp.h`、`tgmath.h`、`wctype.h`（覆盖 75%）。头文件数量只描述文件清单，不表示已有头文件的接口与语义全部合规；不能用后续标准中的可选特性宏来豁免 C99 的要求。

**`<setjmp.h>` 不算未记录缺口**：D1（本节前文，已有条款）写明是**评估过、主动决定不做**：tape 没有间接跳转，也读不到 arm64 的 LR，需要新的 tape 操作族并在两个后端六个目标上 lower；这是权衡过的架构决定，不在"未来安排"里，是永久性的。

**结论**：将缺失头文件与三字符组登记为覆盖限制；不因本次登记自动排期。`setjmp.h` 保留此前的明确非目标说明。

### 5.8 跨目标验证：新增 examples/apps 四个程序（2026-09-27，cc-unisacc 实跑回报，cdx 未独立复跑）

procview.c、winlayout.c、memmap.c、exeinfo.c（3359cc5）此前只验证过 osx/arm64（`-run`、`-O2`、host cc 三者一致）。本轮补验，用 `tests/vms.sh up`/`down` 自己开关虚拟机（只关自己开的那些，没碰已经在跑的 default Lima），不经子代理：

- **lnx/x86_64**（Lima）：把 `unisacc.com` 与四个 `.c` 拷进虚拟机，`-run` 跑，四个都与本机 `cc -std=c99` 参考逐字节相同。
- **win/arm64**（UTM）：同样拷进虚拟机跑；`unisacc.com` 只带一份 x86_64 的 PE 切片，在 Windows arm64 上靠系统自带的模拟层执行，四个都逐字节相同。这证明该 x86_64 切片在 Windows ARM64 的模拟环境运行；不证明 win/arm64 原生产物或 Windows x86_64 实机通过。
- 本条新增证据仅为 Linux x86_64 与 Windows ARM64 主机上的 x86_64 模拟执行。其他目标沿用各自有来源的历史记录，不并入本轮通过数。回报未给被测 `.com` 的完整哈希，因此不将它作为当前冻结候选的发布门禁。


### 5.9 真实系统调用缺口——对标 tinycc/cc 的一个真实短板，需要修正（2026-09-27，主人定性）

**纠正一（方法论）**：不同编译器产出的二进制本来就不会完全一样，这正是测试套件重要的原因——TDD 把"效果"（可观测行为）限定为一致，不要求内部实现或内存布局一致。cc-unisacc 之前以"两个编译器产出的自身 /proc/self/maps 逐字节对不上"为理由，判定 memmap.c 不该自动读自身内存映射，这个判断用错了标尺：**逐字节对拍 cc 只是本项目众多验证手段之一**，不是唯一合法手段。对于"自身内存映射"这类天然依赖具体二进制布局（动态链接与否、缓存路径等）的输出，正确做法是换一种测试仪器（结构自检、与独立真实来源交叉核对），而不是因为"cc 比不了"就放弃这个功能。**结论未改**（这次仍未启用 memmap.c 的自动 `/proc/self/maps`——原因是还没设计出替代的验证仪器，不是因为它不可行），但理由记录纠正为：缺一种新测试仪器，不是缺一个可行的功能。

**纠正二（定性，主人 2026-09-27）：这不是"可选功能"，是对标 tinycc/cc 的一个真实短板，需要安排修正，不是记录后搁置。** unisacc 自己从零写的 C 库没有实现这些系统调用/绑定，tinycc 和系统 cc 链接完整系统 libc，天生就有；这是设计与实现的缺口，不是架构选择：
- **目录列举（`getdents`/`getdirentries64`）**：完全没有。procview 在 Linux 上只能靠对 pid 逐个探测（已实现，见 76bf9a0），不能真正列目录。**已核实的系统调用号**：Linux `getdents64`：x86_64 = 217，arm64 = 61；macOS `getdirentries64`：两个架构统一为 344（`0x2000158`，遵循 abi.tsv 里 osx 系统调用号的固定 `0x2000000` 前缀模式）。Windows 没有等价的原始系统调用，走 `FindFirstFileW`/`FindNextFileW` 这条 WinAPI 路（与现有 abi.tsv 里 `gate` 字段的 `winapi` 分支一致，不是 `syscall`/`svc0`/`svc80`）。
- **`fork`/`exec`/`popen`**：完全没有暴露给用户代码；但 S-17 迁移原型的 ABI 目录（`weights/gold/abi.tsv`）**已经配好 `clone`/`execve` 的系统调用号**（Linux：`clone` x86_64=56、arm64=220，`execve` x86_64=59、arm64=221；macOS：两者的 osx 行都是 `0x2000168`/`0x200003b`），只是没有接到现有前端的内建名字识别（`src/front_parse.c` 里 `__open`/`__read` 那一类）或头文件封装上——**这是四个缺口里工作量最小的一个，数据已经齐了**。Windows 同样没有 fork 语义，只有 `CreateProcessW`。
- **`sysctl`**（macOS 不用 `/proc` 拿进程列表就得靠它）：完全没有配号，是真正的新工作，需要先确定用哪个 `KERN_PROC_ALL` 变体和它的 BSD 系统调用号。
- **窗口系统访问**（X11/Wayland socket、Win32 API、CoreGraphics）：完全没有绑定，比系统调用封装更难（涉及协议或框架链接），四个缺口里工作量最大，其余三个应该先做。

**已证明的目标（主人建议：先用系统 cc 证明可行，再定目标）**：`examples/apps/tools/wingeom.c` 用系统 `cc` 链接 CoreGraphics（`CGWindowListCopyWindowInfo`），在 macOS 上拿到真实的、当前屏幕上所有窗口的坐标、大小与标题，通过管道喂给 `winlayout.c`（用 `unisacc.com -run` 跑），整条链路真实数据端到端验证过。**它不是、也不会是 unisacc 自己编译的产物**——链接框架超出这个编译器的能力边界——它的作用是**给"unisacc 未来若要做窗口感知"提供一个已知正确的参考实现**，其余平台（X11、Win32）同理可以先用系统工具/系统 cc 做出参考，再谈要不要把对应系统调用接进 unisacc。

**下一步（主人：安排修正，不是"按需再评估"）**：
1. **`fork`/`execve` 接上前端内建名字**，Linux 与 macOS 先做（数据已备好，见上）；Windows 需要 `CreateProcessW` 这条不同的路，可以后做。
2. **`getdents64`/`getdirentries64` 接上**，用于真正的目录列举（不是 procview 现在的逐 pid 探测）；Windows 走 `FindFirstFileW`/`FindNextFileW`。
3. 设计"结构自检"类测试仪器（不依赖与 cc 逐字节对拍），用于验证自身内存映射一类天然依赖具体二进制的输出，作为 memmap.c 自动读取 `/proc/self/maps` 的前置条件。
4. `sysctl`、窗口系统绑定排在后面，工作量更大。
5. 一个后台代理（opus，独立 git worktree，不碰 cdx-unisacc 正在用的工作树）已按此清单第 1、2 项开工，见 §6 或后续记录其结果；这两项不必等 cdx-unisacc 手上的大任务腾出手再做。

## 6. 实验发现 [E] —— 面向论文

本章随实现推进累积。**只记实测，不记预期**；每条含可复现命令，供论文直接引用。

### 6.0 形式结果总表

先看这张表：**哪些已经证明、哪些已被自己证伪、哪些还开放**。细节见对应条目。

| 命题 | 状态 | 依据 |
|---|---|---|
| **P-8** 任意有限积上的全函数，存在精确实现它的 kernel 权重 | **已证明**（构造式，11/11 机器验证）。**但这是已知结果**，不是本文贡献 —— [R-1] | §1.4 · E-19 · Omlin & Giles'96 · Tracr'23 |
| **P-3** `net ≡ gold` 可判定 | **已证明**（全域枚举，8,484 key，0 分歧；2026-09-26） | E-5 |
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
5. **js / wasm 这条线**（2026-09-21 记；**2026-09-25 温故**）。实验已开成 **Paper B / `ujs/`**：构造脊上 IC 等有限表仍按同一判据；出货另有 **M3 手写子集自举**（`compiler_core`，stage2≡）——**不是**把 Web 编译器等同于 IntNet。开放点收窄为：UJS 表阶段全量 P-2、UJS-1_ship→全表、P1 构造迁回 unisacc。正文见 `research/ujs-paper.md`，规格 `ujs/prd.md`。

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

## 未来猜想与探索区 [FX]

这里只记猜想，不排期、不验收。一条猜想要进入实施，须先改写成带编号的条款，放进前面的章节。前缀用 `FX`，因为 `X` 已是 §4.6“目标机执行”的条款前缀（X-1…X-3）。

### FX-1 可移植 tape 包（主人 2026-09-27 提出）

**动机**：应用内嵌 `libunisacc`（或提供 `unisaccrun()`），分发的是一个跨架构的中间文件，而不是 `.c`；这个文件本身就是一个包，将来也用作包升级的单位。类比 `.class` 与 wasm。

**已有基础**：
- 同一条 tape 在 6 个目标上的可观测行为必须一致（P-7，由 fold 与 closure 测试强制）。
- `--from-tape` 只走后端，从 tape 直接得到本机代码。
- `-run` 在内存里映射代码执行，不落盘，也不触发 macOS 首启扫描。
- `libunisacc` 实质上就是把“后端 + `-run`”暴露成 API。

**缺口**：
1. **目标中立**：tape 在预处理之后生成，`_WIN32` 这类目标宏的分支已经定死。两种办法：用一个“中立目标”生成（约定不写目标分支），或者一个包里放 6 份 tape、加载时挑选。
2. **格式冻结**：现在没有文件头、版本号和 op 集版本。需要冻结一个版本化的 op 子集，文件头包含魔数、版本、目标和内容哈希。
3. **加载前校验**：加载 tape 等于执行任意代码。需要校验器：跳转目标合法、栈与 `.frame` 平衡、副作用只经由 `.print`/`.write`/`.exit` 及声明过的调用。之后才谈签名。
4. **保密预期**：tape 接近汇编，标签里保留函数名。去掉符号能挡住随手翻看，挡不住认真的逆向，大致与 `.class` 或 wasm 同级。
5. **包与升级**：tape 是整个程序连同内嵌的 libc，天然是单文件包，整文件替换加哈希即可升级。增量补丁和按模块加载需要链接层，而多个翻译单元目前在编译期就合并了（W-14）。

6. **二进制编码 `.tapebin`**：`.tape` 是汇编型文本，分发单位应当是它的二进制编码。编码方式：1 字节 opcode；两个寄存器号合成 1 字节（r0–r7 各占 4 位）；立即数用变长整数；标签与符号进符号表，指令里只存索引；字符串和数据进常量池（与 wasm 或 `.class` 同类）。文件头（魔数、格式版本、op 集版本、目标、内容哈希）成为格式的一部分。好处是体积小、加载时不用解析文本，第 3 点的校验也只需检查编码合法、索引在界内。这一层只是编码，不改变第 1 点的跨架构前提，也不改变第 4 点的保密预期。验收：`tape → tapebin → tape` 逐字节往返一致；fold 测试直接跑 `.tapebin`，6 个目标结果不变。

**前置条件**：S-17 完成，E7 切换到 `.com` 之后。

### FX-2 libunisacc 与 crate：给别的应用当脚本引擎（主人 2026-09-27 提出）

**动机**：让其他应用把 unisacc 当作嵌入式脚本引擎，执行动态 C、`.tape`，将来还有 `.tapebin`（FX-1）。它比解释型脚本引擎快，因为输出的是本机代码；一个引擎覆盖六个目标；自带头文件，不依赖系统工具链。

**现状（2026-09-27 核对）**：没有库形态。`-run` 已并入本体（E-59），产物只有单文件 `unisacc.com`；`src/main.c` 是进程入口，编译器自带 `_start` 和 syscall，不依赖 libc。

**两级路线**：
1. **子进程封装（现在就能做）**：crate 捆绑 `unisacc.com`（或只带目标平台对应的切片），API 形如 `run_c(src, args, timeout)`、`run_tape(tape, args, timeout)`，内部调用 `unisacc -run` 或 `--from-tape`，拿回 stdout 和退出码。子进程天然隔离，崩溃、死循环、越权 syscall 都不会伤到宿主，超时直接杀掉（注意 alarm 只约束它 exec 的那个进程）。
2. **进程内库 `libunisacc.a`（需要改造）**，缺口有五个：
   - 符号冲突：本体自带一份 libc 实现（printf、malloc 等，x86 上约 27.7 KB 代码），与宿主的 libc 或 Rust std 链接时会重名，需要加前缀、不导出；
   - 全局状态：全是静态大数组，bss 约 610 MB 虚拟空间，不可重入、非线程安全，需要收进一个堆上的上下文结构；
   - 错误处理：出错路径直接 `__exit`，会把宿主一起结束，需要改为返回错误码；
   - 输出形态：API 返回内存里的机器码或镜像，由宿主决定是映射执行还是交给子进程；macOS 进程内执行要处理 hardened runtime 下的 `MAP_JIT` 权限；
   - 宿主交互：生成的代码只会直接发 syscall，没有调用宿主函数的机制。脚本引擎需要一套宿主函数导入约定（tape 层声明、lowering 层绑定），并配上 FX-1 的加载前校验，否则就只是“运行一个外部程序”。

**安全前提**：在进程内执行动态代码等于把宿主的全部权限交给脚本。没有校验器（FX-1 第 3 点）或沙箱之前，只推荐子进程这一级。

**前置条件**：第 1 级随时可做，而且不改编译器；第 2 级排在 S-17 与 E7 之后，并与 FX-1 的格式冻结一起设计。

### FX-3 tape → wasm：第 7 输出目标（主人 2026-09-27 提出）

**定位**：保留 tape 作为内部 IR，**新增**一条从 tape 到 wasm 的输出后端，不是把 tape 换成 wasm。它服务的是没有我们运行时的外部环境（浏览器、wasmtime、WASI）；六个原生目标和 S-17 都不受影响。与 FX-1 的 `.tapebin` 互补：`.tapebin` 是无损序列化，由我们自己的运行时加载；wasm 是翻译，控制流结构上有损，交给别人的运行时。

**为什么不整体替换**（2026-09-27 评估）：
- tape 是寄存器形态（r0–r7），降级几乎一对一，所以能查表、能做成 delta；wasm 是栈机，六个目标的下沉层都要改为做寄存器分配。
- tape 用任意标签跳转；wasm 只有结构化控制流，需要 relooper 或 stackifier。
- r7 软件栈可以取地址；wasm 需要在线性内存里另设影子栈。
- 标准 wasm 是 32 位地址，要守住 LP64 契约就得用 memory64。

整体替换粗估要重写现有 C 源码的 50–65%（front_parse 的生成部分、opt 的 H1–H4、back_lower，以及 Python 孪生实现、以 tape op 为键的表网络、exec/ 的 E3/E4/lower），对六个原生目标没有收益，本体代码估计还会增大 10–25%。

**第 7 目标需要做的事**：
- 控制流结构化（只在这个后端里做）；
- 影子栈与 memory64（或者明确声明为 32 位子集）；
- `.print`/`.write`/`.exit` 映射为 WASI 导入；
- 等价验收：wasm 的运行结果与 `vm(tape)` 的可观测行为一致，并入 fold 测试。

规模估计是一个新后端，约 3,000–5,000 行 C 加上 Python 孪生实现。

**边界**：js/wasm 的研究线归 Paper B / `ujs/`（csr，见第 5 条“js / wasm 这条线”）。FX-3 只是 unisacc 后端的一个输出目标，不碰 `ujs/`，也不以 ujs 的编译器为前提；立项时要与 csr 对齐，避免重叠。

**前置条件**：S-17 与 E7 之后；与 FX-1、FX-2 共用文件头和校验器的设计。

### FX-4 规格优先：规则化表规格、分层覆盖、δ 最小化（主人 2026-09-27 提出问题）

**问题**：构造路线的方向是对的，但灵活性不够。这里说的不是统计泛化，而是工程上的：改动的局部性、可组合、可复用、新增构造与目标的成本。

**诊断**（两位独立研究员的结论一致，2026-09-27）：瓶颈是**规格缺位，只剩外延**。表与 δ 是从手写参考实现转写出来的结果，规则本身（内涵）丢了，所以任何改动都只能整张重铺、全量穷举。具体是：
- 解析 δ 约 5,230 个状态、约 130 万个条目，是文法、属性、模板预先相乘的结果，相当于一次没有保留源码的部分求值（Futamura 第一投影）；
- 表之间没有组合运算（没有覆盖、积、串联）；
- 验证是全域的，没有“这次改动影响哪些 key”的依赖信息。

字节流接口**不是**瓶颈：它保证阶段能按变换器组合，应当保留并加 schema，不要换成共享内存。“多表合并有害”的实测结论也说明：共享应发生在构造期，不该发生在推理期。

**总原则**：规则是源头，表是可重新生成的产物；参考实现降为逐字节对拍的对照。这不违反确定性与穷举验证，也符合“旧编译器留作行为参考”的口径。

**候选思路**（都在构造期，不改执行器；实验设计已有，未运行）：
1. **规则化表规格**：每张表写成有序守卫规则，首条命中；生成器枚举 key 域得到表，同时输出“每个 key 由哪条规则产出”的覆盖图。验收：对全域 rules(k) 等于参考表。
2. **分层覆盖（默认加例外）**：表等于基表加有序补丁，补丁是（key 模式到类别）；生成期合并成一张扁平全函数表，运行时不变。
3. **δ 最小化**：把解析 δ 看成带输出的变换器，先做输出规范化，再做 Hopcroft 划分精化，得到行为逐字节相同的商机器，并保留反向映射。一次测量就能量出 5,230 个状态里有多少是冗余，决定下一步的收益上限。
4. **因子化（Ashenhurst–Curtis）**：找划分 {A,B} 使 f = h(g(A), B)，g 的像域小。
5. **文法机、属性机、模板变换器的同步积**：新增构造只加产生式与一条模板，规模相加而不是相乘。E3 的结构化设计（research/e3-structured.md）已经走了这一步。
6. **类型化的阶段接口**：仍是字节流，但每个格式有 schema，清单连接时检查，并从 schema 生成两端。
7. **增量验证**：由覆盖图得到“规则到 key 区域”的依赖，只重验受影响区域，其余靠哈希证书；定期全量验证兜底。
8. **受限等价饱和**：只用于优化阶段，只在生成期、限定窗口和步数，抽取结果必须与参考逐字节一致。

**推荐先试**：1 与 2（只改生成层，秒级实验，且是其余思路的地基），加上 3（一次测量）。

**粗测（2026-09-27，对 `weights/gold/*.tsv` 已导出的真值表，只读、纯 Python；方法粗糙，仅供定方向）**：

| 表 | 行数 | 结果 |
|---|---|---|
| type | 4,275 | 可分解为 h(g(t1,op), t2)，g 只有 39 类；存储约 870 项，压缩约 5 倍 |
| peep | 1,632 | 91.5% 是默认值，约 118 条规则可表达，压缩约 14 倍 |
| enc | 438 | lnx 与 osx 在两个架构上各自完全相同，只有 win 不同；覆盖层成立，win 只需补丁 |
| abi、combo | 438 | 六个目标两两之间有 30% 到 100% 的项不同，是真数据；跨目标覆盖帮不上。**新增目标靠覆盖只对 enc 成立** |

限定：这些表合计只有约 8,900 行，本来就不大；真正的体积在解析 δ，其三源结构化（E3）正在进行，所以思路 5、3 有一半已在路上，缺口是其余阶段和覆盖层。

**与硬约束冲突的方向**（不做）：
- 在推理层组合、共享或“软”补全：把逐字节精确变成近似，还破坏“改一张表只重建一张”；
- 运行期的等价饱和或特化：结果依赖搜索预算，执行器也会远超几 KB；
- 用共享内存的 IR 取代字节流：丢掉阶段隔离与“未覆盖即拒绝”的边界。

**前置条件与归属**：不阻塞 S-17。δ 最小化的测量需要读取 exec/ 的生成物，建议由 cdx 顺手做一次；思路 1、2 可在 E3 结构化收尾之后，先拿 peep 与 enc 两张表试点。


### FX-5 分层 libc：直接系统调用 / 自研纯计算 / 声明并运行时导入系统真身（主人 2026-09-27 提出，两位研究员核对）

**动机**：unisacc 自己从零写 C 库（不用系统 libc），今天已经给 execve、getdents64 这两个纯系统调用接上了真实实现（隔离分支，未合并）。主人提出更大的问题：要不要学 tinycc，做一套"只声明、运行时接目标机器真正的系统 libc"的机制，一次性拿到 fork/exec/opendir/sysctl/pthread/dlopen/locale 这一整片目前完全没有的能力？同时指出两个具体难点：用户程序自己提供同名实现时怎么处理；tinycc 这种做法本身"兼容性不太好"，要弄清楚为什么。

**建议的三层，只新增一层**：
- **L0 直接系统调用**（现状不变，已验证稳定）：open/read/write/close/execve/getdents64 这类很薄的内核包装，参数只有整数和指针，不依赖 libc 内部状态。
- **L1 自研纯计算**：string.h、printf、malloc（基于 mmap）等——这是自研库的核心价值（确定性、自包含），不外包。
- **L2（新增）声明 + 运行时导入系统真身**：只用于 L0/L1 做不到、也不该自己做的部分：`dlopen`/`pthread`/`locale`/DNS 解析等。Windows 现在已经有一个范围很窄的同类机制（一张"符号名→导入源"的固定表，`weights/gold/abi.tsv` 的 `winimp` 列），可以推广成跨平台的 `libspec` 表（字段：name、layer、各平台的 import_symbol、version、family、owner_allocator、layout_id）。

**判定某个函数该进哪一层，看两条**：
1. 调用者要不要**直接读**它返回的结构体内部字段？只经函数访问、从不自己拆开看的（不透明句柄，如目录句柄本身）适合进 L2；调用者要直接读偏移的（透明布局，如目录项里的文件名字段、`struct stat`）必须继续留在 L0/L1 自己填，否则声明的布局和目标机器真实库不一致时会**静默读错**，不报错。
2. 是否依赖进程级隐式状态（TLS、线程列表、动态加载器的内部表、locale 数据库）？依赖的只能进 L2，自己实现一套会和系统真正接入的库互相打架。

**用户自己提供同名实现时怎么办**：单遍编译没有传统链接阶段，但可以在"整份源码读完、生成代码之前"设一个统一的符号终结点，按顺序解析：用户定义 > 编译器自研实现 > 声明并导入系统真身 > 报错未定义。自研实现之间必须通过同一套符号槎互相调用（不能内联死），这样用户重写 `malloc` 时，自研的 `calloc`/`strdup` 会自动跟着换成用户版本。另留一个逃生口（类似 `__real_opendir`）给"既想覆盖又想调用系统原版"的场景。一致性检查：覆盖了一个资源族的一员（如 `malloc`）却不管其它成员（`free`/`realloc`），要报错或警告，不能静默放过——这是 tinycc 类工具常见的崩溃根源。

**tinycc"兼容性不好"的六个具体根子**（不是空泛的"ABI 不稳定"）：
1. 透明结构体跨版本/跨架构布局漂移（`struct stat` 在 x86_64 与 arm64 上字节数都不同）；
2. glibc 符号版本（同一个函数名绑定到不同版本，行为可能不同）；
3. 很多"函数"其实是宏或内联，真正的符号名完全不同（`errno` 在三个平台是三个不同的东西）；
4. 一旦引入系统线程，自研的全局状态（自己的 `errno`、`malloc` 锁）不再线程安全，且不在系统 TLS 里；
5. 两套 `stdio`/两套分配器混用会导致输出错序或跨分配器释放崩溃；
6. Windows 的 CRT 本身就有多个互不兼容的版本（msvcrt/ucrtbase/api-ms-win-crt-*）。

每一条都有对应的缓解方法（按平台固化透明布局、显式绑定符号版本基线、给每个符号记录"直接符号/调用 getter/数据重定位"三种访问方式、一旦用 L2 就整族切到系统的程序启动流程、stdio 与 exit 整族切换不混用、Windows 只导入 kernel32/ntdll 与固定的 ucrtbase 集合）。

**最小验证实验**（≤60 秒）：12 个探针程序分四组——透明布局（`readdir`/`stat`/`localtime_r`）、不透明句柄（`opendir`/`getaddrinfo`）、隐式状态（`pthread`/`dlopen`）、用户覆盖（自定义 `opendir`/`malloc` 配 `__real_` 逃生口），各自编译 L0 版和 L2 版，在 macOS、glibc Linux、musl 三种环境跑，比对与系统 cc 的逐字节差异、崩溃、超时。

**结论：值得做，但范围要窄**——L2 只当"L0/L1 做不到的出口"，不是拿来替换现有的自研库。确定性和单文件自包含仍是主线卖点，L1 必须保留；透明布局类（`readdir`/`stat`）继续留在 L0，除非实验证明 L2 更好。

**第一小步（未排期）**：把 Windows 现有的固定导入表推广成跨平台 `libspec` 结构；先在 Linux 上只接一个完全不涉及透明结构体的功能族（建议 `dlopen`/`dlsym`/`dlclose`），落地"符号终结点解析 + `__real_` 逃生口"，用上面的 C、D 组探针验证。做 `pthread` 之前必须先解决"一旦用了 L2 就要走系统的程序启动流程"这个前置问题。

**前置条件与归属**：不阻塞 S-17 与当前正在做的门禁验收；不影响已经验证过的 L0（execve、getdents64，隔离分支）。留给 cdx 判断何时排期。



### 5.10 只读巡查发现：bit-field 存储单元宽度错误（2026-09-27，cc-unisacc 用 examples/apps/colorpack.c 实测发现，未修复）

**背景**：主人要求在 cdx 没有漂移的巡查间隙，充实测试套件与 examples/、用 unisacc.com 实测找问题。新增 `examples/apps/colorpack.c`（bit-field 打包像素格式：RGB565/555、RGBA4444），只经具名字段读写，不做内存重解释，规避了 C99 6.7.2.1p10 允许的"位分配顺序因实现而异"这条合法差异。

**发现**：所有字段值在 `-run` 与 `-O2`、host cc 与 unisacc.com 之间逐一比对**完全一致**（说明 bit-field 的读写语义是对的）；但 `sizeof` 不一致。最小复现（`/tmp/bfsize.c`）：

```c
struct a { unsigned short r:5, g:6, b:5; };   /* cc: 2   unisacc: 4 */
struct b { unsigned char x:3, y:3; };          /* cc: 1   unisacc: 2 */
struct c { unsigned r:5, g:6, b:5; };          /* cc: 4   unisacc: 4，一致 */
```

**规律**：只要 bit-field 声明的基础类型比 `int` 窄（`unsigned short`、`unsigned char`），unisacc 分配的存储单元就比正确值大一档（2→4、1→2，正好是"下一个整数尺寸"），像是取存储单元宽度时跳到了下一级，而不是停在声明的基础类型宽度上；基础类型本来就是 `int`/`unsigned` 宽度时（4 字节）不受影响。

**已核实的旁证**：审查 `src/front_parse.c` 里 `own[256]`（结构体成员计数上限）已有边界检查（`3512` 行），不是本次巡查要修的对象；`decode()` 的两个 4096 缓冲区调用点（`3854`、`3951` 行）已经在 f004934 里统一走显式容量检查，不再是 B2 那类风险。这次的 bit-field 问题是**新发现，与 f004934/12be67c 那批修复无关**。

**影响面**：任何用窄类型声明 bit-field 的结构体，`sizeof`、数组步长、结构体内后续成员的偏移都会算错，是真实的正确性缺陷，不是实现定义范围内的合理差异。

**留给 cdx**：本条只记录、不修复，不动 src/。建议定位到 bit-field 存储单元宽度选择的那段代码（结构体成员布局，`stbody`/`bitpos`/`bw` 附近，`src/front_parse.c` 3467 行前后），核对是否是"取下一个尺寸类"而不是"取声明类型自身尺寸"的选择逻辑写错了下标。

## 7. 附录

### 7.1 构建顺序

```
M1 数学与网络     rng → linalg → net → gold → train      ⟦A-12, A-14⟧
M2 权重落盘       uns1 + quant                            ⟦A-11, A-13⟧
M3 tape 与 VM     tape → vm                               ⟦手写 tape 跑通⟧
M4 C99 前端       pp → lex → parse → sema → ir            ⟦七样例出 tape 且 VM 正确⟧
M5 Lowering       lower → exec_target                     ⟦A-5, A-8⟧ ★主目标
M6 镜像           emit_x86/arm → image/*                  ⟦A-9, A-10⟧
M7 Ship           ship + kernel/unisa_core.c              ⟦A-15, A-16, B-2⟧
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

- 2026-09-26 (S-17, gen2 54cbaee): multi-dimensional arrays measured and done. `b[i][j][k]`: each subscript that is not the last multiplies by (element size × the remaining dimensions) and adds, with no load. A DIM table per variable holds the dimensions. Also: printf with any conversion other than %d/%% is a real variadic call. mkdump now runs autoinc on the dump path. The old E3 has 1 DIFF (p67) on the corrected input. Structured E3: 2,371 states, 96 equal, 0 differ.
- 2026-09-26 (S-17, gen2 up to 3404ebc): new measured rules, all done in the structured E3.
  - **sizeof** is a constant `imm` and its operand emits nothing.
  - **Pointer difference** is sub64 then `.div` by the element size, giving a long. **n + p** scales the pushed n in place.
  - **Local initialisers** emit: the value; `imm r2, slot; sub64 r1, r6, r2`; then a store at the variable's width. **Comma declarators** add their stars on the base depth.
  - **A statement `*p;` alone** computes the address only.
  - **++, -- and op=** work on every width and on pointers (the step is the element size). Postfix loads raw. Prefix and op= load masked and mask an unsigned narrow result again.
  - **unsigned int** loads and casts are masked to 32 bits. Operators mask both operands, use the unsigned form, and mask the result. Hex constants above INT_MAX are unsigned int.
  - **Tooling:** a name defined twice is now a build error (it had silently merged DSCALE with DIMSAVE).
  - **Status:** 2,828 states; 127 equal, 0 differ. The old E3 matches 110. What remains is where the old one covered and the new does not yet: p43/p58/p59 (the pairing of long and unsigned int), p47 (parameter), p53 and p7, p65 (double), p76/p81/p83/p84 (width).
- 2026-09-26 (S-17): **E3 switched to the structured delta** (97fd265, exec/pipeline/stages.tsv).
  - **Switch gate:** `E3KEEP=exec/parse2/keep-old-e3.txt` passes with exit 0. All 110 files the old E3 matched are kept. In total 141 files are equal and 0 differ.
  - **Sizes, old vs new:** states 5,233 → 3,210; entries 1,345,252 → 825,517; json 29.9 → 17.3 MB.
  - **Product fix (9052993):** a constant now resets curuns, curflt and curstruct. Before, a unit's first expression saw curstruct 0 ("a struct"), so `1 + 10u` lost its unsignedness.
  - **compare.py (cdx review):** tool failures, an empty file list and an empty keep list now all fail. reject-both is an observation only.
  - **Status:** still a structured state machine / lookup-table prototype. E3 is not yet a net.
- 2026-09-26 (S-17, cdx review of 97fd265): **exact byte count.** Both deltas are serialized the same way (`json.dumps(separators=(",", ":"))`) and built from gen.py at acb6f58 and gen2.py at a8ae6fb.

  | | raw bytes | gzip -9 |
  |---|---|---|
  | old | 29,938,744 | 3,147,912 |
  | new | 17,262,831 | 1,903,000 |

  The "23.9 → 13.3 MB" in the first version of the 97fd265 message was wrong. The message was amended before it was pushed.
  - **Speed:** not yet compared per file on the same file set.
  - **Dependency:** only the old *generator entry* is retired. gen2.py still imports exec/parse/gen.py as its token reader, table assembler and constants, so that file stays live code.
  - **Product regression:** added tests/c/b_constkind.c, for the constant-kind fix 9052993. It checks `-1 < 1u` as a unit's first expression and again right after a struct operand, plus a hex constant above INT_MAX.
    - cc prints `0 0 0 1`.
    - The fixed compiler prints the same at -O0, -O1 and -O2.
    - The compiler before the fix (9052993^) prints `1 1 0 1`.
- 2026-09-26 (S-17 / reviews by cdx and bdy-ds4flash), measured:
  - **E3 now reads the type table.** OPX's integer tail asks type.tsv the way the product's binary() does (549e88d): `ck = type(t1 + t2)` gives the masks and the spelling, `res = type(t1 op t2)` gives the result. The fixed set exec/parse2/keep-e3.txt (164 files) stays all equal. States went 3,212 → 3,014, but gen2.py grew by 37 lines: the rules moved into the table, the code did not shrink.
  - **printf is always a real call** in both front ends: the C one in 991d337, the Python one in 18c8f22.
  - **main falls off its `}` with 0** in both front ends (C99 5.1.2.2.3). This was a bug exposed by the real printf: corpus 00206/11/12 exited 12.
  - **%p prints 0x** followed by the hex digits.
  - **The referee ledger is now data**: research/referee.tsv, ratcheted to gold ALL by tests/docs.sh.
  - **New external referees:**
    - `tests/prec_audit.py`: 291 of 324 operator pairs agree, 33 are undetermined, and the level domain 1..10 is asserted.
    - `tests/tyinfo_audit.py`: 27 facts agree.

    External referees now cover 4 of 18 stages (type partly, abi, prec, tyinfo).
  - **ablate** is in release.sh as 8 shards. prec is in ablate's list; peep and opinfo are registered as not covered on the Python path.
  - **pfconv:** since printf became a real call, pfconv is asked only by the fallback for a unit with no printf declared.
- 2026-09-26 (S-17), **the C executor**, measured (4b37a77):
  - **What it is.** `exec/c/run.c` is the generic executor in C; it copies `exec/pp/sim.py` action for action (55 actions). `exec/c/tbl.py` renumbers a JSON delta into an integer table, and nothing in either file is specific to a language.
  - **E3 check.** `exec/c/check.sh` compares its verdict with sim.py's over all 225 files. All 225 match: 164 identical tapes and 61 identical reject reasons.
  - **E2 check.** 32 files (examples/ and tests/c/a_*) give byte-identical output, with headers read through the file dictionary.
  - **Speed**, one file (tests/c/a_for.c), wall time including the table load:

    | stage | C | Python |
    |---|---|---|
    | E3 | 0.07 s | 0.44 s |
    | E2 | 0.01 s | 0.06 s |
  - **Remaining steps toward the slice:**
    - E1 on the same executor (its nets have their own simulator today);
    - a binary table format;
    - the table as nets.
  - **Product fix.** `-nostdinc` no longer adds `<stdio.h>`. A printf there takes the compile-time lowering (`do_printf`); `tests/cli.sh` now runs this path.
- 2026-09-26 (S-17), **one executor, three tables, source to tape** (exec/c/chain.sh). One generic C executor, `exec/c/run.c`, runs all three stages:
  - E2, the preprocessor delta. It now follows the product's rule that any `printf(` pulls in stdio.h.
  - E1, the typed lexer.
  - E3, the parser.

  **Result:** on examples/ and tests/c (113 files), 54 compile from source to a tape identical to the reference's, 59 are not covered by some stage, and 0 differ. Checked alone, each stage agrees with its Python executor: E3 on 225 files, E2 and E1 on 32 each.

  **Still missing:**
  - chain.sh in the gate;
  - a binary table format;
  - E4 and later (lowering and encoding), which have no delta yet;
  - the tables as nets.
- 2026-09-26 (S-17), **table sizes after row defaults** (acc894f). Each byte-keyed or r-keyed row now lists only the keys that differ from its most common (next, seq) pair; that pair answers every key not listed. The tables are still text, not yet a binary format.

  | table | size |
  |---|---|
  | E3 | 9,772,470 → 368,976 B |
  | E1 | 155,080 B |
  | E2 | 72,226 B |
  | executor binary (cc -O2) | 54,120 B |

  So the whole front end, from source to tape, is one 54 KB executor plus about 0.6 MB of text tables.
  - **Speed:** one E3 file takes 0.02 s warm.
  - **Checks:** check.sh gives 225/225 the same verdict as the Python executor. The chain of three stages keeps its 54 files equal and is in the gate (exec-chain, gate 30/30).
  - **Still missing:** a binary format, the stages after E3 (lowering and encoding), and the tables as nets.
- 2026-09-26 (S-17), **E4 met on the tested inputs** (522f746, 1d4a953, 9a705d4). `exec/opt/gen.py` turns the tape optimiser into a delta that runs on the generic executor. The reference's -O0 tape run through it equals its -O1 and -O2 tapes, byte for byte:
  - on examples/ and tests/c (113 files at each level);
  - on unisacc.c itself: a 3.9 MB tape, 11 s on the C executor.

  **The delta:** the -O1 table has 191 states; the -O2 table has 904 states.

  **What -O2 contains**, each computed by the delta itself, not by the executor:
  - the per-line facts of `ol_prep`;
  - blocks, labels and targets;
  - the liveness solver;
  - the push/pop rule with the carry through r3..r5, and `ol_local`;
  - `stfuse`;
  - the peep relations. The peep table (1,632 keys) and opinfo's `simple`/`acls`/`bcls` columns are loaded into memory at START.

  **Gate:** exec-e4 (28 s) and exec-e4self (12 s); the gate is 33/33.

  **Still missing:**
  - E1–E3 coverage: E3 matches 164 of 225 files, and the source-to-tape chain 54 of 113;
  - E5 (lowering and encoding, six targets), E6 (image writing) and E7 (.com layout);
  - the tables as nets.
- 2026-09-26, **working rule** (the owner caught this): after every batch of product source changes, rebuild unisacc.com (`make com`) and run `tests/gate.sh --com`. Committing the sources alone is not enough; the .com had not been rebuilt for most of a day. It was rebuilt at 04a127b's tree (1,342,224 B), and gate --com passed 41/41. The exec/ migration is still a parallel path that unisacc.com does not use. S-17's switch rule, which moves the default path to the new implementation only after its gate passes, has not been applied yet.
- 2026-09-26 (S-17), **fixed product miscompile: `*"literal"`**. At product reference 4b37a77, `int f(void) { return *"z"; }` returned the literal's address truncated to int: `.lea` without the required load. Product fix 48a5c4b resets the string literal's complete type state before setting its `char *` descriptor. `tests/c/b_strderef.c` covers preceding struct and pointer-to-pointer expressions and ordinary string pointers. Independent takeover verification: system cc, rebuilt `.com` at -O0/-O1/-O2 and Python `--drive built` all print `122 67` / `284 Q pq`, exit 0. Python was already correct. E3's temporary rejection was removed; `exec/parse2/probes/s35d.c` now matches the fixed reference on both executors and joins the keep list after all old 191 entries passed in four bounded shards (192 kept, not a fresh full-corpus count). The broader product probe remains outside E3 coverage (`expected ;`), and no extra syntax was added.
- 2026-09-26, **takeover build verification**: product sources at 48a5c4b, `unisacc.c` regenerated by `make com`; artifact `unisacc.com` is 1,342,320 B, SHA256 `d8ac8275157951044dcd22de0677984e81473387eff969af751fdabd583f4dd3`. Independent `tests/gate.sh --com`: 41 suites, 0 failed, 92 s aggregate wall time, each suite bounded at 60 s (longest 45 s). Host macOS arm64; Rosetta fat execution 114/0. Linux/Windows native runs were not repeated. Existing known cases remain: corpus 4 known failures, ccparity 1 known case. E3 chain retained 68/68; E4 and E5 gates passed. Local working artifact rebuilt, no release or push; exec remains outside the product execution path. Detailed local logs: `/tmp/unisacc-takeover-48a5c4b/` (temporary, not archival).

- 2026-09-26, **continued refactoring after takeover**: E5 x86 integer division/remainder (`.div/.mod/.udiv/.umod`) share a delta encoding path and the existing ALU/MEM helpers; no new executor primitive. `exec/enc/check.sh`: 32 fixtures pass on both executors, including hand-worked signed-division bytes and scratch-register rejection. One-off comparison: all 1,372 combinations of four ops and the seven non-stack tape registers, 59,682 bytes, identical on C and Python executors. gen.py 533 lines (net +50 from this batch's baseline); 416 states, JSON 2,116,894 B, text table 20,033 B. This adds hand encoding rules; it does not reduce rule-source code or turn tables into nets. Product inputs are unchanged, so the previously validated `.com` is unchanged. Full S-17 remains active: E1–E3 completeness and failure contracts, remaining E5/ARM/lowering, E6 images, network execution, compact per-ISA executor and single-model E7 product adoption all still require implementation and verification.

### Takeover continuation: FP encoding and gate reliability (2026-09-26)

- E5: all 24 FP operations encoded by delta, with hand rules in `exec/enc/fp.py`;
  no executor primitives added and no network claim. Both executor byte checks
  plus independent host-C execution: `exec/enc/check.sh` 35 ok, 0 bad; FP execution
  550 cases, 0 bad. One-off exhaustive register comparison: 5,292 combinations.
- `tests/fat.sh`: build/sign/incomplete runs now fail instead of silently skipping;
  every subprocess has a 60 s bound, output and exit code must match the reference.
  hello/fib normal run: 2 pass; a PATH substitute failing only fib's build leaves
  hello passing but makes the suite fail (1 pass, 1 mismatch, rc 1).
- This is not complete test coverage: E3 remains partial; E5 is not lowering or
  full six-target image generation. These changes have not rerun Linux/Windows.
  No product source changed, so the existing .com artifact is unchanged.

### E5 non-WinAPI gate continuation (2026-09-26)

- Migrated syscall and Darwin carry correction as delta transitions, no new
  executor primitive. Known non-WinAPI metadata accepted; winapi and malformed
  carry rejected. `exec/enc/check.sh`: 38 ok, 0 bad, including the 550 independent
  FP execution cases retained from the preceding batch. Syscall bytes compared,
  syscall execution not claimed.
- Real hello/fib lowering audit now identifies address-dependent forms as the
  next encoder boundary: .lea, setmem, setreg mem, argsave, argvget. Full lowering,
  address layout and image generation remain to be migrated; no product switch.

### Complete lowering payload transport (2026-09-26)

`exec/enc/tins.py` now has an explicit full form for target, data bytes,
DATA_BASE-relative symbols, src_os, data_len, BSS and relocation offsets.
The instruction-only form remains compatible. `roundtrip.py` compares every
TargetProgram attribute and typed instruction/meta field, not just code.
Three real programs across six targets are now fixed gate checks; E5 gate
43 ok, 0 bad. This is transport, not six-platform execution or header support
in the delta encoder. Relaxed layout/address encoding remains the next step.

### Address encoding reaches complete real code sections (2026-09-26)

- Delta now computes Linux/macOS x86 layout from final relaxed text size and
  emits .lea, setreg mem/addr, setmem, argsave and argvget at final positions.
  Format constants are read declarations; layout/RIP algorithms are hand rules
  in `address.py`. No new executor primitive and no network claim.
- `realcheck.py`: whole hello/fib lowering (including stdio, no filtering),
  10,009/10,072 instructions each; exact reference bytes on Linux and macOS x86.
  macOS x86 execution of delta code wrapped by the existing Python image writer
  gives expected outputs and rc 0. E5 gate: 48 ok, 0 bad.
- Header payload is now consumed for target and symbols; data-related headers
  are retained but image writing is not migrated. Lowering, remaining targets,
  image emission and product adoption remain unfinished.

### E6 first image route: Linux x86_64 ELF (2026-09-26)

- `gen.py --elf` compiles the ELF image writer into the delta, sharing the
  address/text encoder. Generic executor unchanged. Data decoding, pointer
  relocation, zero-tail trimming, entry point and two load segments are emitted
  by ordinary actions; no Python image writer in this execution path.
- Whole hello/fib ELF output equals reference (45,366 / 45,358 bytes). Two
  synthetic relocated-pointer cases match on C and Python executors, plus
  independent entry/segment/address/file-length assertions. Out-of-data
  relocation rejected on both. `exec-elf` added as a bounded gate batch.
- Input lowering is still Python; Linux native execution not run in this batch
  (the x86 Lima VM is stopped). Other formats/architectures and frontend
  completion remain required; current .com unchanged.

### E6 Linux execution evidence (2026-09-26, f506dc6)

The actual delta-produced hello.elf and fib.elf were copied unchanged into
`minicon-lnx-x86_64` (Lima/QEMU on the ARM host). Each ran under a 10 s guest
watchdog: hello stdout `hello from C99\n`, fib stdout `55\n`, both rc 0 and
empty stderr. This is Linux x86_64 emulation, not a native x86_64 machine.
The VM was initially stopped, started for this check and stopped afterwards
(shutdown confirmed). The separately running default ARM VM was untouched.

Lowering remains the next algorithmic dependency: raw tape data directives
must be decoded and reordered by zero_last before scratch cells are assigned;
register mapping, entry setup, syscall shapes and adjacent push/pop fusion
must then be executed by the delta. Serialising an already lowered program
does not complete that dependency. Its interface must eventually consume E4's
actual tape output, rather than moving the Python lowering behind an adapter.

### Lowering starts at raw E4 tape data (2026-09-26)

`exec/lower/gen.py` now generates a 153-state data-pass delta. It parses .str
(including byte escapes) and .bss, aligns data, performs zero-last reordering,
rewrites symbol addresses and allocates Linux scratch cells. Non-data code
and label meaning remain unchanged. It outputs a data header plus tape code,
not yet target instructions. No executor primitive was added.

The fixed `exec-lowdata` gate includes five edge fixtures on C and Python
executors (escapes, empty data, all-zero data, aliases and terminal alignment),
plus actual hello -S input and fib output produced by the E4 delta. Bytes and
symbols match the Python referee; code/labels survive. Python parsing and
zero_last only judge results, not supply intermediate answers to the delta.
This is a hand algorithm compiled into a state table, not a new network.
Instruction lowering and remaining architectures are still pending.

### Raw source through six deltas to Linux ELF (2026-09-26)

The 1,063-state full lowering delta now consumes raw tape and handles Linux
x86_64 register mapping, entry setup, argument/syscall rules and adjacent
push/pop fusion. It reads regmap/enc/abi/reloc facts; control algorithms remain
hand-written in the generator. `.print` and other targets remain unsupported.
The full typed TargetProgram comparison passes a 41-instruction fixture on
both executors, hello (9,791 instructions) and optimized fib (5,530) on C.
Data-only checks remain green after introducing the full-mode hook.

`exec/pipeline/elf.sh` generates six tables, then runs source through E2/E1/E3/
E4/lowering/ELF on one generic C executor. No Python stage processes source
once tables have been generated. `exec-srcelf` checks hello and fib: optimized
tapes match the product reference, and complete ELF files match the Python
image referee (28,982 and 28,970 bytes). `exec-lower` checks lowering separately.
These two examples establish an end-to-end route, not C99 completeness, six-
target support, network implementation or replacement of the product path.

The two new source-to-ELF outputs were executed on local Lima Linux x86_64
(emulated on the arm64 host): stdout `hello from C99\n` and `55\n`, exit 0,
empty stderr. An initial manual hello expectation was wrong; after checking
the source the rerun passed. The VM started for this check was stopped;
the already-running default VM was left alone. This is runtime evidence for
these two outputs, not a Linux suite pass.

### End-to-end fixed set expands to 68 (2026-09-26)

All 68 existing chain keep inputs now complete the six-stage source-to-ELF
route. Each optimized tape equals the product reference and each full ELF
image equals the reference lowering/assembler/image result, with encoded
instruction count checked. The set is retained independently in
`exec/pipeline/keep-elf.txt`; `exec-srcelf` now rebuilds tables and checks all
68, failing on a missing input, rejecting stage, nonzero reference or byte
mismatch. The complete check passed under one 60-second watchdog. Linux
execution evidence remains the two examples above, not all 68 programs.

### Self-source blocker: literal tokens and escape decoding (2026-09-26)

E2 and E1 accept unisacc.c. E3 formerly stopped at MODEL's multiline adjacent
string token: its reader stopped at the first newline. 6bf71f7 makes the token
span quote-aware and shares a byte decoder between pooling and character
array initialization. Ordinary adjacent literals, hex escapes and one-to-three
digit octal escapes are covered by s37/s38; the old 192 E3 keep inputs stayed
equal and the two new probes agree on both executors (194 retained). Wide
prefixes remain unsupported. This is added hand parser logic in a delta,
not network construction or removal of the original parser.

The new probes found a product defect, fixed independently in 8a87421:
C decode assumed exactly two hex digits and could read through a closing
quote; it also reinterpreted octal results 'r'/'x' as escape instructions.
Python limited hex to two digits and lacked four standard simple escapes.
The fixed paths decode each escape once and stop at the literal boundary.
`tests/c/b_stresc.c` agrees with system cc in the reference, Python and rebuilt
.com at all three optimization levels. Kernel provenance was regenerated.

Frozen-tree gate --com, JOBS=2: 45 suites passed, none failed, 183 seconds total,
each at most 60 seconds; exec-srcelf (68 programs) 46s, fat 115/0, nativeboot,
bigclosure 6/6, difftest_o 342/0, kernel 23/0. This run was macOS arm64 plus
Rosetta, not a fresh Linux/Windows product gate. The rebuilt .com is 1,343,904 B,
SHA256 736a6c86ff71b1b34e29de969ba6501572f622cb37a7a0455073b0d617378f3c,
from the product inputs in 8a87421. Nothing was pushed or released.

Self-source E3 now passes the large MODEL declaration and next stops on an
array bound constant expression (64 * 1024). Full frontend self-compilation,
other targets, constructed-network execution and product-path replacement
remain incomplete; this is not a self-bootstrap result for the new route.

### Self-source: constant bounds and postfix values (2026-09-26)

E3 now evaluates integer bound/enum expressions using the shared prec rows
and generic integer actions. `constexpr.py` adds 49 lines of hand parser
rules; it is not a newly constructed network. Current limits: signed 64-bit
arithmetic without full C conversion/overflow handling; sizeof and casts
are not yet covered; a zero divisor is refused even in an unselected arm.
These are prototype coverage limits, not C language restrictions.
Global numeric expressions reuse EXPR once, saving the resulting initializer
tape and source end position for later emission, preserving source-order
label allocation. String and function-call values now reuse POSTIX, clearing
stale array rank before subscript processing. No executor action was added.

All old 194 retained inputs stayed equal; s39 (bounds/enum), s40 (global
expressions), s41 (literal subscripts), s42 (call-result subscripts) and the
product b_strderef regression pass on both executors and join the fixed set:
199 equal, 0 lost/differ/tool-fail. gen2.py 1629 -> 1633 lines, plus the new
49-line evaluator; rules were added/reused, not removed. Delta: 3647 states,
938024 entries, JSON 19854065 B, text table 410815 B. Full current self-source
E2/E1 succeeds; E3 stops at optimizer `t < 0 ? 1 : bl_live[base+t]`, whose
integer arms differ in width. This remains an incomplete frontend self-run.

The walk found a product type-projection defect, fixed separately in fb5e167:
binary i64 & i64 asked the table's address-of sentinel row, so `(5L & 3L)+1`
scaled by eight and yielded 9. Binary & now asks the | arithmetic row, leaving
unary address-of unchanged; E3 uses the same contextual projection. System
cc, the rebuilt native reference at -O0/-O1/-O2 and Python built all give
`2 0 2 8 1` for b_longand. That regression is still outside E3 coverage because
of its sizeof-expression form, so it is not added to E3's retained set.
Full rebuilt-product gate results are recorded after the frozen run below.

Frozen b06170a/fb5e167 product run: gate --com 45/45, JOBS=2, 191s total,
with each suite bounded by 60s. This is macOS arm64/Rosetta evidence only.
Rebuilt unisacc.com: 1343904 B, SHA256
58df0c97b398a600b344b967be3a2b3033422f057a5b753d5d78d2385ac1d7b5.
The .com b_longand check at -O0/-O1/-O2 also agrees with host cc.
The next self-source blocker exposed another reference defect: for
`sizeof(k ? 1 : c)`, `sizeof(k ? c : 1)`, `sizeof(k ? c : c)` with char c,
host cc gives 4/4/4, native gives 1/4/1, Python gives 4/1/1. Mixed signed and
unsigned arms also inherit only one arm's type. This must be corrected
against the common-type rule before it is used as a migration reference.

### Conditional integer types follow the shared table (2026-09-26)

The product's two frontends now ask type(t1 + t2) for two integer arms of
?:, retain that result's width and signedness, and mask a narrow unsigned
selected value at the merge. Neither the first nor the second arm alone
supplies the type. b_condkind agrees with host cc: `4 4 4 0 0`, in native
-O0/-O1/-O2 and Python built; the pre-fix outputs are recorded above.
E3 reuses its existing CKT and RESD procedures for the same decision,
replacing the earlier hand check limited to signed int/long. The old 199
retained inputs pass; s43 agrees on both executors, bringing the fixed set
to 200. Delta 3659 states, JSON 19929833 B, text table 411066 B.
Current self-source E2/E1 succeeds; E3 next stops at main.c's block-scope
`static char dname[520]`. No self-source success is claimed yet.

Frozen 47ef8c3 / product 1f3ba5b validation: gate --com 45/45, JOBS=2,
187s total, each suite <=60s. macOS arm64/Rosetta only; no new Linux/Windows
product-suite claim. Rebuilt .com 1345056 B, SHA256
44e0fe5ca0194ef567a92181b7e9984619ebf2e2a7963fecb5b46f33865bf25d.
The .com conditional regression also matches cc at -O0/-O1/-O2.

### S-17 current self-source milestone (2026-09-26, bb4b4b8)

The generic C executor now runs E2 -> E1 -> E3 on the complete current
unisacc.c. Its 3,918,249-byte tape equals the native reference exactly;
exec/parse2/selfcheck.sh enforces this in the gate. This is a frontend
self-source result, not bootstrap of the replacement compiler or net execution.
E3's fixed set is 207 equal (old 200 retained, plus s44-s49 and b_static).
The seven additions also agree through the C executor. The added rules cover
block static storage and scalar initialisation, saved array dimensions and
enum shadowing, int-return function-pointer casts, repeated tentative global
definitions, and the product's declared syscall list. gen2.py is 1,678 lines
(+42 from the preceding 1,636), plus statics.py 45 lines; rules and source grew.
3918 states, JSON 21,449,911 B; this is still a generated transition table.

Product 7d50875 rejects automatic objects referenced from static initialisers;
sizeof non-VLA objects remains legal and discards both output buffers. Python's
constant sizeof now also accepts expression operands. staticinit checks nine
compile-only rejections, including addresses and array decay; b_staticsize
checks the legal path. Calls and ordinary global-value reads in C static
initialisers remain known nonconformances; this is not a full constant-expression
validator. E3 isolates automatic-storage initialisers with a named not-covered
rejection. Static aggregate initialisers and other function-pointer cast return
types remain outside this slice's coverage.

Frozen validation: gate --com 49/49, JOBS=2, 186s total, every suite <=60s;
exec-chain 68/68, source-to-ELF's fixed 68 retained, fat 118/118. macOS arm64
and Rosetta only. Product .com: 1,345,808 B, SHA256
60b396834a71d96e93a942c1c33d75e8b4e955709b6dbaffad069f51ca150908.
No push or release. The shipped compile path is still the handwritten compiler.

The preceding self-source tape also passed E4 -O2 equality. Full lowering hit
alarm 60: its data pass expands 609,909,154 bytes of declared zero storage,
scans and copies them byte by byte, and emits hex. Next is sparse zero-region
layout with unchanged symbol addresses and image bytes, not a higher watchdog.
The full six-target route, constructed-network runtime, compact executor and
E7 product adoption remain outstanding.

### S-17 sparse self-source ELF milestone (2026-09-26, 1775315)

The zero-storage blocker above is resolved. Lowering keeps virtual storage
separate from literal bytes; ELF records the omitted zero tail in p_memsz.
No executor action was added. The complete six-delta route now processes
unisacc.c (product source 7d50875) into a 671,404-byte Linux x86-64 ELF,
byte-identical to the reference. Generator/runtime source: 1775315.
SHA256: d8f7586e1d0250064c248cfbb1bfe8ad8dc8021abf6f9f6e86da5d50b58d1979.

Actual provenance: exec/pipeline/elf.sh produced
/tmp/unisacc-self-route/final-pipeline/unisacc.elf. That image was copied as
n1 into /tmp/unisacc-delta-self-7ba6f76 in Lima minicon-lnx-x86_64. There,
`n1 -O2 -b lnx/x86_64 unisacc.c -o n2` and the same command using n2 to
produce n3 gave N1=N2=N3 with the hash above. Each command was bounded at
50 s in the guest and 60 s on the host. All six examples/apps programs ran
via n1 -run, exited 0 and matched host cc stdout. This VM is emulated on
arm64, not native x86 hardware; it was stopped afterwards.

Frozen gate --com: 51/51, JOBS=2, 191 s total; exec-selfelf took 16 s.
Standalone exec/pipeline/selfcheck.sh (table generation, six stages, reference
tape and ELF comparisons): 15.68 s wall, 14.56 s user, 0.34 s system.
The 60 s watchdog is unchanged. Supplemental alias/duplicate and sparse
relocation boundary checks passed on both executors after the full gate;
the sparse-input substitution now asserts exactly one matching data line.

This output is the existing C compiler compiled through transition tables.
The tables are not constructed networks, and the shipped .com has not switched
to the executor. Other targets, broader coverage, network execution, compact
executor and E7 adoption remain open. No push or release.

### S-17 ARM64 encoder started (2026-09-26)

The remaining-target work now includes exec/enc/arm.py: integer constants,
register ALU/shifts/comparisons, multiply, nop and tape callr/ret. ENCSPEC
supplies ALU3 and condition values; packing and tape ABI sequences remain
hand-written generator rules. Same generic executor, no new action. 220 states,
10,059 B text table. This is not a network or a product-path change.

Targeted armcheck passed: both executors match 60 reference instructions / 384
bytes; eleven out-of-domain cases reject. On macOS arm64, 540 operations agree
with system-assembler functions, ten full-width immediate values execute as
expected, and the tape call/return bytes match independent expectations.
Added exec-arm to the gate; the previous 51-suite run predates this entry, so
no new full-gate total is claimed. Memory, labels, metadata, FP, image formats
and integration with ARM lowering remain to be implemented.

ARM64 memory follow-up: load64/store64 and .ld/.st support widths 1/2/4/8,
scaled imm12, signed imm9 and MOVIMM/ADD-or-SUB long-address fallback. Opcode
constants come from emit_arm's LDS/STS/LDU/STU declarations; selection and
packing are still hand generator rules. Fallback rejects a clobbered x16 base
or store source; x16 as load destination is allowed. callr now also rejects
x7, which its software push changes. regmap maps tape r7 to x7 (SP), not an
ordinary value register; x17 has no regmap row.

Frozen gate --com: 52/52, JOBS=2, 191 s, max suite 40 s, macOS arm64/Rosetta.
ARM memory initially checked 92 instructions; afterwards a test-only addition
covers -2^63 offsets for all widths. Targeted rerun: 100 instructions / 768 B
match on both executors, four memory-domain rejects, 64 native load/store
cases match C memcpy and signed values. The earlier 550 native arithmetic and
constant checks still pass. 313 states, 13,213 B table; no new executor action,
no product source change, no .com rebuild or publication needed for this slice.

ARM64 branch follow-up: section-local jump/jumpz/call and shared/end labels now
run on the same executor. Two passes measure actual lengths, discard provisional
bytes, then resolve offsets. Calls use the BL position after three setup words.
RELFIELD supplies bit width/shift; layout/range logic remains hand-written in
the generator. Duplicate/undefined labels reject. 367 states, 16,297 B table.
Targeted armcheck passes all earlier checks plus ten branch fixtures on both
runtimes, six hand-worked byte expectations, seven rejects, eight synthetic
helper boundary contexts for imm19/imm26, and native loop/direct-call results
5/42. No new full-gate run is claimed after the previous 52/52. Metadata,
remaining ops, ARM lowering and images are still outstanding; .com unchanged.

ARM64 integer lowering forms: .div/.udiv/.mod/.umod, sext, addi/subi/lsli and
.frame now encode through the same executor. Remainder rejects x17 source
clobbers; frame changes software SP x7. These are hand-written encoding rules,
not table-derived semantics. Immediate field checks use full width before
narrow selectors; only imm accepts unsigned positive 64-bit bit patterns.
Targeted armcheck retains all prior checks and adds 28 instructions / 152 B
on both runtimes, ten domain/scratch rejections and 24 native functions checked
against defined C arithmetic/frame results. 524 states / 22,355 B. No new full
gate claim; product sources and .com unchanged. Metadata, setreg/address forms,
FP, syscall setup and image/ARM-lowering integration remain outstanding.

ARM64 FP follow-up: all 24 FP operations emit on the generic integer executor,
with v16/v17 used by generated machine code. Opcode declarations are read from
FARITH/FCMP_INV/FP_OPS; packing/conversion sequences remain hand rules. 72
operand/alias fixtures / 960 bytes agree on both runtimes; the shared native
C referee passes 550 cases on macOS arm64. x86's 48 fixtures, real hello/fib
encoding/execution, and its 550 numerical cases were rerun successfully after
the harness gained an explicit arm64 mode. 744 states / 37,347 B table. No full
gate rerun or .com change is claimed. Reference equality and finite numerical
cases are not full FP semantic proof.

Cross-target known UB behaviour: ARM SDIV/UDIV return zero for divide by zero,
and SDIV returns MIN for MIN/-1; x86 IDIV traps. C-defined-input tests exclude
these cases, so a difference on them is not evidence of a delta regression.

ARM64 TIns interface: setreg imm/reg, host-sp spinit and POSIX gate forms now
encode. Metadata keys follow tins.META; duplicates, unknown/empty fields and
incompatible reloc/gate/carry values reject. Tags retain argument types.
Windows annotations on POSIX gate remain informational. Five whole fixtures
match both executors/reference, three worked byte strings and seventeen rejects
pass; all prior ARM checks remain green. 822 states / 41,672 B. Inspection of
real hello lowering found remaining address/setup forms (.lea, setmem, setreg
mem, argsave, argvget); no whole real ARM program claim yet. Product unchanged,
no new full-gate run claimed. Next is actual payload/address layout integration.

### S-17 ARM64 real code encoding (2026-09-27)

armlayout now derives text/data addresses from measured text size and format
constants, resolves data/code symbols and emits ADRP/ADD and POSIX setup forms.
Whole hello/fib lowering outputs encode identically: lnx/arm64 54,352/54,696 B;
osx/arm64 54,476/54,820 B. macOS images execute with expected output/exit 0.
No instructions were filtered. Python remains the lowering producer and image
wrapper; ARM delta lowering/image output and product adoption are still open.
Payload headers are retained, not certified as valid image data by this encoder.

Address fixtures cover both layouts, page crossings and data-over-code name
precedence (same as reference); actual ADRP/ADD bytes are decoded to assert
addresses in fixtures and all four real encodings. Thirteen malformed header,
unknown symbol and scratch alias inputs reject on both executors. 981 states /
48,726 B table. Gate --com: 52/52, JOBS=2, 196 s, ARM suite 13 s and Linux
self-source suite 16 s. Test-only strengthening after the gate passed the full
ARM suite again. Product source/.com unchanged; no push or release.

### S-17 Linux ARM64 ELF writer (2026-09-27)

ARM --elf reuses elfimage.py; e_machine and label representation are explicit
parameters, with no architecture-specific image logic in the executor. Shared
ELF byte output is self-contained and relocation-index digits are bounded
before overflow. ARM requires a data declaration and Linux target. ELF mode
1,079 states / 55,788 B; no new runtime primitive.

hello/fib complete ELF images match reference (57,654/57,646 B) and execute in
existing Lima default aarch64: expected stdout, no stderr, exit 0, guest timeout
10 / host 60 s. Hashes: hello 1f227471a8d2cb471fcc2eb8b7a2e90319266b982663be152138d3575937e2b2;
fib d4efd5598e6d77f9c342189b350fad35287ad63d5a1af8171c86ac7a71e9181f.
No VM was started; the existing VM was left running. Python still does lowering.
ARM text checks, both image suites, large sparse extent and x86 self-source
671,404-byte ELF equality all reran green. exec-armelf added to gate, but this
batch does not claim a new full-gate count. Product .com unchanged. Next: ARM
lowering on the executor, then remaining formats/targets and network adoption.


### S-17 ARM lowering and source route (2026-09-27)

Shared data/register/ABI lowering now selects Linux ARM64. armfuse.py encodes
sext and immediate peepholes, including the 32-instruction dead-after scan,
as ordinary actions. These are migrated hand rules, not removal of algorithms
or a constructed network. No executor primitive added. ARM lowering table:
1,482 states, 134,562 B compressed text.

Both executors match setup/sext fixture (96 instructions) and immediate fixture
(131), including 4095/4096, signed constants, power-of-two multiply through
2^63, labels, aliases and 31/32 scan boundary. C executor matches hello/fib
full typed lowering and current self-source: 103,254 target instructions plus
labels/data/metadata. x86 self-source route rerun: 671,404 B equal.

TARGET=lnx/arm64 elf.sh now runs all six deltas from source. Optimized hello
28,982 B sha256 54008849528a13e1d1168e7c3e706d55d9cdac08e8f9f8f56f261257b6d50e34;
fib 28,970 B sha256 ab4f92611388643a4a21805ae089d7f0f26798458ab3de52a5fb8c4bdbe738fd.
Both match ua_ref -O2 ARM ELF and run in existing Lima default (aarch64), exact
stdout, rc 0, empty stderr; guest 10 s, host 60 s. VM was already running.
Self-source reaches encoder but rejects; .zero is still unmigrated. This is
not an ARM self-bootstrap or E7 adoption. exec-armlower added to gate; no new
full-gate result claimed. Product source/.com unchanged, no push/release.


### S-17 Linux ARM64 self-source closure (2026-09-27)

ARM .zero now stores XZR using existing memory transitions, 8/4/2/1 pieces.
Both-executor byte checks plus seven native zero-fill cases passed (memory
suite total 136 instructions / 1,128 B, 71 native cases). Negative lengths,
fallback scratch aliases and signed offset wrap reject before output.

First full self-source image differed by two data bytes: E2 still declared
__x86_64__. E2 now takes the explicit Linux target, preserving the default x86
path. elf.sh passes TARGET through E2, lowering and encoder. Complete ARM ELF
now equals the reference: 716,458 B, SHA256
5e95f0dc9efbcbfacf3552d5e1b505a5fe9010b4cf57ecec8f828002c8bc07ef.
Input product source remains 7d50875. Host outputs /tmp/unisacc-arm-self-target;
Lima default guest /tmp/unisacc-arm-self-target/n1,n2,n3. n1 is the six-delta
output; n1 -O2 -b lnx/arm64 unisacc.c -> n2; n2 similarly -> n3; all three
cmp and SHA256 identical. Guest compiles timeout30, host each command<=60.
VM was already running and remains running. Tables are still lookup tables,
generated by Python; this is not E7's executor-based product switch.

ARM selfcheck is now a gate item with target-specific -S and ELF reference.
Both ARM (716,458 B) and x86 (671,404 B) selfchecks reran green. ARM ELF table
1,112 states / 56,816 B; text table 1,014 / 50,134 B. Full ARM suite green.
No product-source or .com change; no push/release. Full-gate status separately.


Post-70f7c9a full local gate --com: 55/55, 115 s, slowest suite 43 s;
ARM and x86 source-to-ELF selfchecks each 16 s. Actual concurrency was the
wrapper default 4: JOBS=2 outside term.sh was not forwarded. Do not report 2.
Use explicit `tests/term.sh env JOBS=2 ...` until wrapper propagation is fixed.
After this gate, only selfcheck's target-macro fixture was strengthened: the
six Linux names must each expand to 1 and the other architecture/Apple/Mach/
Windows names must be absent. Generated E2 and product -E both checked; ARM
and x86 selfchecks reran green with this fixture. No new full-gate run claimed.


### S-17 Darwin lowering preparation (2026-09-27)

`gen.py --full --osx [--arm64]` reuses the POSIX lowering transitions. ABI
facts select Darwin syscall numbers and svc80/syscall; gates carry=true and
argsave takes loader registers (false), with @src_os osx. Tests compare all
TargetProgram fields, not only code bytes. Darwin ARM fixture93/immediate131
on both executors, hello/fib12395/5824 instructions; Darwin x86 fixture41 on
both, hello/fib9791/5530. Both Linux suites reran green. Mach-O image writing
is not yet a delta; no complete Darwin source-route claim.

Test wrapper repair 81294d2 forwards explicit JOBS, TARGET and E2/E3/E4/chain
settings through Terminal, quoting apostrophes/newlines and preserving empty
versus unset. Actual Terminal probe checked those values and exit7 propagation.
No compiler/product-source change, no .com rebuild or push.


### S-17 Mach-O prerequisite: SHA-256 transitions (2026-09-27)

The existing Mach-O writer requires ad-hoc page signatures. sha256delta.py
compiles SHA-256 padding, schedule, rounds and digest emission into ordinary
A64/LDX/STX/input/output actions; no runtime hash/image primitive added. The
64 round constants and eight initial words are read as declarations from
src/back_image.c, with counts/indices/ranges checked. SHA256 takes a blob,
appends 32 bytes, restores input and returns. This is an explicitly migrated
algorithm, not a constructed network and not yet a complete Mach-O writer.

shacheck: 17 inputs, both C and Python executors, each hashes twice in one run.
Three fixed known digests (empty, abc, long standard text) plus deterministic
binary inputs checked against hashlib: 55/56,63/64/65,119/120,127/128 and
4095/4096/4097/8192 boundaries. All passed. Standalone harness has 30 states,
5,786 B table. exec-sha registered in gate; only targeted suite run this batch.
Next integrate header/layout and CodeDirectory/SuperBlob, then full Mach-O
byte comparison and native execution. Product .com unchanged; no push.


### S-17 shared Mach-O writer (2026-09-27)

arm.py/gen.py --macho select one machodelta.py for both architectures. Common
payload decoding, sparse extent, relocation and trimming stay in elfimage.py;
Mach-O emits its load commands, aligned segments, LC_MAIN and ad-hoc signature.
CodeDirectory/SuperBlob and per-4KB SHA hashes use ordinary actions. No native
signer or hash/image-specific runtime primitive; the Python writer is only a
test oracle. Static format constants/templates come from unisa.image.macho.

Read-only review found DATA at 93e6 could overlap SHA W/K at 95/96e6. DATA,
W and K now occupy distinct 1<<40 regions (byte extent <2^31). A 2,000,100-byte
stored data fixture with relocation at 2,000,000 matches the complete reference
image (2,047,650 B) and independently validates all 497 page hashes. Small
empty/16/16385-byte data fixtures compare both executors. A Linux-target
payload in Mach-O mode is rejected by both runtimes, with no output.

Complete hello/fib images: ARM 82,722 B each, x86 66,210 B each, match reference.
Both run on macOS arm64 host (x86 via Rosetta), expected stdout/rc0/no stderr.
Every signature page is separately checked with hashlib. ARM states1332, x86
states1189. Both ELF suites reran green after shared changes. Registered
exec-macharm/exec-machx86; only targeted suites run this batch, not full gate.
Input producer is still Python lowering for these tests; source-to-Mach-O,
self-bootstrap and E7 product adoption are not claimed. Product .com unchanged.


### S-17 both macOS source routes/self-bootstrap (2026-09-27)

elf.sh now selects all four POSIX OS/arch combinations, preserving Linux's
.elf path and writing .macho for Darwin. E2 supplies __APPLE__, __MACH__,
__unix__, the architecture macro, __LP64__ and __UNISA__; selfcheck explicitly
checks all six values and absence of other target macros using both E2 and
product -E. It compares the target-specific -O2 tape and full image.

osx/arm64: 743,202 B, SHA256
d2bf5cc5f485f3b7ad16f01dbd9b459d4a130d84206c9dea1e8fa1f00425cc55.
osx/x86_64: 693,666 B, SHA256
f971def83f4cb8e6cb085c84c62ce28b8c36637b3a5eb8188bd7b41474f6409f.
Both six-delta images equal the reference; N1 compiles unisacc.c to N2, N2 to
N3 with -O2 -b TARGET, all bytes identical. ARM native, x86 via Rosetta on
macOS arm64. Artifacts /tmp/unisacc-osx-{arm,x86}-source. Product source is
still 7d50875. Added native bootstrap to supported-host selfcheck, otherwise
explicitly reports not executed. Individual commands remain <=60 s.

Mach-O additionally rejects a header/text size mismatch, padding overrun and
file/signature extent exceeding 32-bit fields. Both Darwin selfchecks passed
after these guards; Linux x86 source selfcheck still 671,404 B equal. Registered
exec-macself and exec-macxself; no new full-gate result claimed for this batch.
Windows/PE and executor/model product adoption remain outstanding; tables are
not neural networks. No product-source/.com changes, no push/release.

### Product follow-up: target predefinitions and `-U` (2026-09-27)

Moved native `predef()`'s `-U` loop after all target predefinitions. Previously
Darwin, Windows, architecture, `__LP64__` and `__UNISA__` names were defined
after the removal loop. `tests/cli.sh` now checks every such name across all
six targets, including command exit status: 64 passed, 0 wrong. This does not
claim full ordered `-D`/`-U` compatibility or add `-U` to the delta preprocessor.
Regenerated `unisacc.c`, rebuilt `unisacc.com`: 1,345,792 B,
SHA256 `d1583b832d7bb4d0020a8448af3c008dd597cfcc4bcf1ba3d063d0a9051ffe82`.
Frozen-tree local `gate --com`: 60 suites, 0 failed, 138 s total, each suite
bounded at 60 s; macOS host evidence, not a new Linux/Windows native run.
Log: `/tmp/unisacc-u-gate.log`. No release or push.

### Windows typed lowering (2026-09-27)

Windows data/extra-stack layout and full typed lowering now share the existing
transition generator with POSIX. `wincheck.sh` covers x86_64 and arm64, all
catalog WINAPI calls, mmap's argument truncation, setup/save/restore and
hello/fib; compares complete typed fields and layout, not selected bytes.
Both executor fixtures and C-executor real tapes passed. Existing four POSIX
lowering comparisons passed unchanged. Added `exec-winlower` to gate; the last
full gate remains the preceding 60/60 run, not a claim for the new 61-suite list.
No new executor primitive. This is lookup-table migration, not network runtime;
Windows encoding/PE/native execution and E7 product adoption remain unfinished.

### Windows ARM64 text setup (2026-09-27)

Raw encoder now computes PE text/import/data addresses and emits addressed
spinit, winsave/winrest and winstdh, including indirect GetStdHandle calls.
`armcheck.sh` passed within its 60 s bound: three OS address layouts and page
crossings on both executors; prior native arithmetic, memory, FP and Darwin
program execution retained. Table: 1,073 states, 54,766 B text. Encoding rules
remain explicit in generator; declarations supply imports and standard-handle
numbers. No new runtime primitive. PE files/native Windows and the remaining
WinAPI sequences are not yet covered. Log: /tmp/unisacc-win-armsetup-final.log.

### Windows ARM64 gate text (2026-09-27)

ARM delta now encodes reference-supported WinAPI bodies and return conversions;
winargs retains the explicitly declared machine-code parser template. Tested
fixture 132 instructions / 2,772 B on both executors; Windows hello/fib complete
text 56,304 / 56,648 B equals reference. Seven bad contracts reject, including
winrest with result other than x0 (review22). Existing ARM native arithmetic,
FP/memory and Darwin execution suite passed. Logs /tmp/unisacc-arm-winapi.log
and /tmp/unisacc-arm-winapi-regression.log. Added exec-armwin gate entry;
no new full-gate result claimed. PE writer and native Windows execution remain.

### ARM64 PE writer and first native Windows evidence (2026-09-27)

Complete ARM PE emitted by the delta, not a Python image wrapper. Whole-file
reference comparison plus independent structure checks passed for sparse/BSS,
relocation duplicates/order/page boundaries and 2 MB data; small fixtures on
both executors. ELF/Mach-O regressions passed. Initial duplicate relocation
sort corruption was caught and fixed; fixture retained.
Windows 11 ARM64 UTM actual runs, separate process timeout 30 s: hello/fib
exit 0, stdout equals host cc. hello 59,392 B SHA256
`3de62f3522f36f6831d70a99c8be451231bc3246323fe36f9b17786ab5fc1153`;
fib 59,904 B SHA256
`a59d240caa6f4ab8d861224b6ea0761b210782c2ad0c8b1f65a7f76604774ffa`.
Artifacts /tmp/unisacc-delta-pe; logs /tmp/unisacc-pe-final.log and
/tmp/unisacc-pe-native.log. VM was stopped after use. Reference lowering is
still the input producer in this test, not the full source pipeline. Windows
x86 encoding/PE route, self-source, E7 and network runtime remain unfinished.

### Windows ARM64 full source route and bootstrap (2026-09-27)

E2 now selects the five Windows predefines; source pipeline accepts win/arm64
and writes .exe. Source is unisacc.c at f360ba0, SHA256
`11ef59801d8c18f637e67b2a5b0e988ad31baec1859fabdd06dc879935574307`.
Six-delta output, reference image, Windows-native N2 and N3 are identical:
741,888 B, SHA256
`a831d9b1000f7661e6db4346cd9f17a1bd0c0c568a12c40b78a4208430ffa08f`.
Actual Windows 11 ARM64 execution through UTM, independent exit receipts,
30 s per guest compile; started VM stopped after use. Artifacts in
/tmp/unisacc-win-arm-source and log /tmp/unisacc-pe-bootstrap.log.
The self-source input has zero data relocations (@relocs -); quadratic sorting
is a general scaling limitation, not a measured self-source timeout.

Code-frozen local gate --com: 64 suites, 0 failed, 147 s total; exec-winself
17 s, exec-armwin 3 s, exec-pearm 4 s, every suite bounded at 60 s. Log
/tmp/unisacc-win-source-gate.log. The peer's prd-only 1435ed3 was retained during
this run; product/code inputs were unchanged. Windows native bootstrap was a
separate run, not implied by the macOS gate. No push/release. This closes the
fifth source-to-image target, not Windows x86_64, full frontend coverage,
network inference or product adoption. WINARGS_BODY remains a named template.

### Windows x86 setup after relaxation (2026-09-27)

The encoder retains per-instruction setup operands, measures fixed sizes, and
regenerates winsave/winrest/winstdh/winargs after branch relaxation and PE
address layout. A 359-byte fixture with a shortened preceding jump equals the
reference on C and Python executors; wrong return register, target and arity
are rejected. WINARGS_BODY is a declared template, not a migrated algorithm.
No executor primitive added. WinAPI gate bodies and full x86 PE/source closure
remain pending. Existing x86 fixture regression passes; product unchanged.

### Windows x86 full source route and bootstrap (2026-09-27)

Eleven WinAPI bodies and ABI-selected return conversions now run as deferred
x86 encoding transitions. The final RIP bytes follow relaxation, with fixed
length rechecked. No executor primitive was added. The reference's short
zero-extending immediate form is preserved (the first gate fixture caught
an overlong MOVABS in the new implementation, corrected before acceptance).
132 real lowered instructions / 2,381 B agree on both executors. Shared PE
fixtures include unsorted/duplicate DIR64 entries, a page crossing, and
2,000,100 B data with a relocation at 2,000,000; the machine field is checked.
Real hello/fib PE files are 46,080 / 46,592 B, identical to the reference.

Six-stage source pipeline and selfcheck pass for win/x86_64 (target macros,
optimized tape and complete PE). Source is f360ba0's unisacc.c, SHA256
`11ef59801d8c18f637e67b2a5b0e988ad31baec1859fabdd06dc879935574307`.
N1/N2/N3 are all 694,272 B, SHA256
`19bfcf3d809ac42e6120a4cbc033b9002de3d556c1634c6f59ad23647b312fc8`.
Actual execution: Windows 11 ARM64 UTM, **x86_64 emulation**, not x86 hardware;
each guest compile has a 30 s timeout, command has a 60 s outer watchdog.
hello/fib each exit 0 with host-cc expected output. VM started here was stopped
and is not left running. Artifacts: /tmp/unisacc-win-x86-source,
/tmp/unisacc-winx86-pe; logs: /tmp/unisacc-winx86-bootstrap.log and
/tmp/unisacc-winx86-exec.log. WINARGS_BODY remains a template. All six measured
self-source routes exist; frontend completeness, networks, compact executor
and E7 product adoption remain open. Product sources/.com unchanged.

Frozen-tree gate --com: 67 suites, 0 failed, 264 s aggregate with JOBS=2;
each suite <=60 s. exec-x86win 2 s, exec-pex86 2 s, exec-winx86self 16 s;
ARM PE/self-source regression also green. Log /tmp/unisacc-winx86-gate.log.
Windows x86 PE transition table: 1,401 states; JSON 7,548,924 B, text table
94,862 B. x86win.py 125 lines (setup and gate rules); no code-shrink claim.
No push/release. The reviewer is paused at the owner's request and did not
review this batch; the test and Windows execution evidence above is mine.

### E3 sizeof expressions and reference type correction (2026-09-27)

General scalar sizeof operands reuse UNARY/expression parsing, then discard
emitted instructions using a stack-saved output mark. Static-initializer
frame checks are suppressed only during that unevaluated walk and restored.
The existing named-array/dimension path is retained; general non-scalar
sizeof expressions remain explicitly not covered. No executor primitive or
new hand type ladder: ELSZ derives the size from the existing descriptor.

Independent cc execution of the new probe found a product defect that tape
comparison alone had hidden: sizeof(sizeof x) was 4 in C, and sizeof returned
a signed type in Python. Both frontends now return the compiler ABI's unsigned
64-bit size_t; C resets the result kind instead of inheriting operand state.
Regression tests/c/b_sizeoftype.c and exec/parse2/probes/s50.c: host cc,
rebuilt .com at O0/O1/O2 and Python agree, exit 0. Nested sizeof is 8, unsigned
comparisons agree, ++ and a function call inside sizeof have no side effect.

Before growing lists, old keep-e3 207 all pass; full exploration: 258 files,
221 equal, 37 not-covered, 0 DIFF/tool-fail/LOST. Six additions are fixed:
s50, b_sizeoftype, b_notint, b_shift, b_condkind, b_longand. Source-to-tape:
old 68 plus these six = 74/74 equal, no rejection or loss. Five new cases
(excluding b_longand) ran through all six deltas to macOS ARM64 images:
whole image equals reference, native output/exit equals host cc. Logs:
/tmp/unisacc-e3-sizeof-fixed.log and /tmp/unisacc-sizeof-chain.log.
Rebuilt .com: 1,345,824 B, SHA256
`dfb14d6bb8f0df5a774feec3fd8a78fb9dc6b255e473026172244c782da58f5c`.
Earlier route/bootstrap hashes remain historical evidence for their recorded
source commits; no claim that those old hashes describe this new product.

Product correction committed as c4ba1c4; the same frozen source tree
passed gate --com: 67 suites, 0 failed, 264 s aggregate, JOBS=2, each suite
<=60 s. exec-chain 74/74, exec-srcelf all 74 complete images equal, all six
self-source routes pass; macOS native/Rosetta self-bootstrap remains in those
checks. No new Linux/Windows native run is claimed for the sizeof batch.
Log /tmp/unisacc-sizeof-gate.log. No push/release.


### S-17：实际阈值网络执行首片（2026-09-27）

- `exec/c/net.py` 按有限行的相邻差分构造整数阈值网络；`run.c` 实际计算隐藏阈值与整数输出，不展开成查表。状态选择网络 bank；下一状态、动作序列号是两个线性整数输出。证明与边界见 `exec/c/NETWORK.md`，不借用旧 18 阶段的验证替代本次证明。
- `NETWORK=1 exec/pipeline/elf.sh ...` 为可选开发路线，默认表路线仍保留。六段的动作和字符串声明不变；Python 仍构造模型，生成器的手写编译规则未消失。
- osx/arm64 六段全域：E2 159,188 / E1 86,625 / E3 1,013,168 / E4 232,976 / lowering 366,362 / image 399,644，共 2,257,963 个观测（含缺项），网络与表完全相同。E3 的同一检查在 UBSan 下通过。
- 六份 `.net` 文本含动作与字符串的字节数依次为 59,840 / 42,873 / 453,241 / 105,323 / 137,869 / 111,969；这是完整声明文件口径，不是仅网络权重或执行内核大小。
- s50 的六段输出逐字节等于表路线；完整 `unisacc.c` 经六段网络生成 osx/arm64 镜像 743,202 B，与参考相同，实跑 N1=N2=N3。生成的仍是现有 C 编译器，不是 E7 发布切换。
- `netcheck.py`：三种观测、非状态栈符号、缺项、接受路径以及改坏 bias 必须失败。`gate --com` 新增 net/netself：69/69，JOBS=2 总 277 s，每套件 ≤60 s；netself 19 s。产品 `.com` 本轮未改变。其他五目标本轮通过的是原表路线，网络路线尚不借此声称已验收。
- 仍未完成：前端全部覆盖、失败诊断等价、几 KB 内核与单份模型 `.com`；T2/T3 仍未证。cc-unisacc 保持暂停，本片无其独立复核。


### S-17：六目标网络路线与默认切换（2026-09-27）

- 在 `004d28e` 上显式 `NETWORK=1`，其余五目标 selfcheck 全通过：lnx/x86_64 671,404 B（17.6 s）；lnx/arm64 716,458 B（17.3 s）；osx/x86_64 693,666 B（21.2 s）；win/arm64 741,888 B（18.4 s）；win/x86_64 694,272 B（17.5 s）。每项重新构造六段网络并全域核对，完整 tape/镜像等于参考；macOS x86_64 经 Rosetta 实跑 N1=N2=N3。Linux/Windows 本轮只验证生成字节，未重启 VM 实跑。
- `elf.sh` 与 `chain.sh` 默认网络推理，`NETWORK=0` 为显式查表对照；回执标明 net/tbl，Terminal 环境传递 NETWORK。门禁的六个 selfcheck 因默认切换而测网络，另保留 osx/arm64 查表 selfcheck。
- 切换后：默认网络 source-to-tape 74/74、source-to-ELF 固定 74 项全等；显式 NETWORK=0 的同一 74 项 tape 全等。产品代码及 `.com` 未改，最近完整 gate --com 仍是上一片 69/69；本片没有把定向复跑写成新一轮全门禁。
- 下一处实际依赖：`unisacc.com -O2 exec/c/run.c` 目前在三个 typedef 形式的 for 初始化处报未知标识符。执行器尚不能称为自托管或产品路径；继续沿真实构建错误处理。


### S-17：执行器由 unisacc 构建（2026-09-27）

- 真实构建发现并修复两处产品问题：for 初始化只看内置 type token，现改用已有 `is_typeat`，Python 同步允许 enum/struct/union 类型；词法器把内部类名 `str`/`num` 当成字面量，现将前五个 TOKV 类名都视为普通标识符。E1 生成器同步修正，未把参考误编译继续抄入网络。回归 `b_fortype.c`、`b_toknames.c`：cc、`.com` 的 O0/O1/O2、Python 全部一致；旧产物对两例以 1 失败。隔离的 `char **str` 复现原先还会把变量读成字符串地址。
- `run.c` 移除 fscanf/ferror/atoll 依赖，用显式整数读取器与分块 IO。动作语义不变；64 位边界以实际 OFILL 输出核对，越界拒绝；输出 fwrite/fclose 失败以 IO 错误 2 退出。native POSIX read/open 保留负 errno，原有 read-dir / ENOTDIR 等五个负例在自建执行器上通过。
- `nativecheck.sh`：先由 unisacc 生成编译器，再用它构建执行器；六段全域核对通过；hello/fib/b_toknames 的 18 份阶段输出等于 cc 构建执行器；两种执行器在真实 1,024 B 文件限制下都报告写失败。入门禁 `exec-native`，11 s。
- 完整负载：自建执行器对先前冻结的 3.9 MB self tape 跑 E4，26.81 s，2,231,188 B 等于 cc 执行器。另在当前源码上 `.com` → native compiler → native executor → 六段网络 → osx/arm64 编译器，完整 selfcheck 在 alarm 60 内通过；743,202 B 等于参考，N1=N2=N3。这里没有把旧负载的计时当作新源码或跨平台性能。
- 规模：cc -O2 runtime __text 16,620 B / 文件 56,520 B（动态 libSystem 不计）；unisacc -O2 __text 85,220 B / 文件 115,746 B（含随带库）。均为加载、IO、校验、执行合计，不是单独内核，也未达几 KB 目标。
- `.com` 已随产品修复重建：1,345,792 B，SHA-256 `236aee387020e2f5d04f8d4a18bff05cb44121eb11f0ae553332d2480c8e1c13`；源 `unisacc.c` SHA-256 `caa8a5ba4b8685403505f299fd8ce2a90d50af999339b136f48164fb70aeea2b`。本地 gate --com 70/70，JOBS=2 总 287 s，每套件 ≤60 s；fat 121/0，两架构均执行；六目标网络 self-source 全部相同。未推送、未发布。
- 仍有实质缺口：b_fortype 在 E3 为 `not covered: identifier is not a local`（产品修复不代表 prototype 已覆盖）；b_toknames 已端到端 tape 相同。native Windows IO 错误分类未闭合；Linux/Windows 本轮未实跑自建 runtime；发布 `.com` 仍为手写 C 编译器路径。cc-unisacc 继续暂停。


### S-17：for 声明与作用域（2026-09-27）

- cc-unisacc 保持暂停，由 cdx-unisacc 继续。E3 的 for 初始化接入已有 TSPEC/S.decl，识别内置类型、typedef、struct/union 与已声明 enum tag；标签在初始化之后分配，避免条件表达式先分配标签时错位。循环用现有 BIND/UNWIND 保存和恢复局部作用域，没有新增执行器原语。enum tag 使用独立命名空间，不混用 typedef 或值名。
- 独立执行暴露产品缺陷：外层 i=7，`for (int i=0;i<2;i++) {}` 后返回 i，旧 `.com` 返回 2。C 前端补循环作用域的符号、帧偏移、类型及 VLA 栈恢复；Python 已有循环作用域，不改其算法。回归 b_forscope 覆盖同名遮蔽、嵌套、continue/break、条件初始化及表达式初始化；cc、C O0/O1/O2、Python 输出 `7 2 9`，rc 0。
- 固定清单扩大之前，原 E3 213 项全部 equal；b_fortype 与 b_forscope 再加入，现 215 项。两例通过六段实际网络生成 macOS ARM64 镜像，完整字节等于参考，实际运行等于 cc。chain/ELF 清单 74→76；门禁 source-to-tape 76/76。未声称前端全覆盖：exec/c/run.c 仍停在 `sizeof non-scalar expression`。
- 规模：gen2.py +18/-10（净 +8 行），状态 3928→3942，JSON 21,506,653→21,584,161 B。属于行为补齐，不是代码缩减；既有类型/声明机制被复用，未把语言规则放进执行器。
- 已重建 `.com`：1,347,184 B，SHA256 `0475e2c62a3835c6a5104479dda948a4c11456be5cb87af8d658fcc9e0af9fa9`；unisacc.c SHA256 `ce491daefe821f8eacda919fc94acfbd697b05e0873d246ced89115efd894157`。
- 冻结树 gate --com 70/70，JOBS=2，总 285 s，每套件 ≤60 s；fat 122/0（两架构实跑），六目标网络 self-source 与参考相同，macOS 自举沿用门禁实跑。Linux/Windows 本轮仅字节核对，未启动 VM。日志 /tmp/unisacc-for-gate.log；旧 E3 清单证据 /tmp/unisacc-for-keep.log。未推送、未发布；完整重构与 E7 产品切换仍未完成。


### S-17：执行器源码的尺寸与成员声明（2026-09-27）

- E3 sizeof 识别 typedef 与 enum 类型；`sizeof *name`/多重解引用复用命名对象的 DIM/ELSZ 计算，保留数组剩余维度，不把数组退化为 8 字节指针。复杂操作数仍走既有表达式解析，未覆盖者继续明确拒绝。结构体逗号声明复用 DSTARS 与同一成员布局，支持每个声明符独立的指针深度与数组长度。无新执行器原语。
- 独立 cc 对照发现产品错误：sizeof 解引用结构体指针原为 8、实际应为 16；二维数组解引用原为 4、应为一行的 12。C 前端解引用现在记录聚合尺寸与剩余维度，指针结果明确为 8。b_sizeofderef 在 cc、C O0/O1/O2、Python 的输出相同：`8 1 8 16 8 16 4 / 24 12 4 / 96 48 16 4`。Python 原本正确，未改。s51 核对同一声明的 char 指针/char/指针、int、数组成员，布局与 cc 相同。
- 新直接分支一度抢先 LOOKUP，令旧有 `sizeof *f()` 从 equal 退为 not-covered。补测发现后，改为先分派复杂操作数，再查直接对象；s52 固定此回归，cc、.com、Python 与六段网络均为 `4 4 0`、rc 0（函数未被调用）。没有把旧覆盖损失当作可以接受的扩展代价。
- 扩清单前旧 215 E3 项全部通过；最终 218/218 equal。chain 从 76 增至 79，最终网络路径 79/79，无拒绝/丢项。三例均通过六网络生成 macOS ARM64 镜像，原始字节等于参考，实跑等于 cc。E3 网络全域 1,023,230 观测相等，3967 banks / 7445 hidden units / 含动作字符串的文本 455,303 B。
- gen2.py +21/-8（净 +13），状态 3942→3967，JSON 21,584,161→21,725,618 B：覆盖进展而非代码缩减。执行器源码已越过 sizeof 和逗号成员声明，当前真实阻挡是 `o->n++`，未称整个 runtime 源码已经由网络编译。
- 产品已重建：.com 1,347,504 B，SHA256 `cd19cca573d7a9fc44eccb4ffb4d88d72617c2f096e452cec2cec9aa64aa04b7`；unisacc.c SHA256 `c20e182aa65723f4315673c461d2929d8ada0d2081e90e7bcd0825a137c9f912`。冻结产品/前端初版 gate --com 70/70、287 s、JOBS=2，各套件 ≤60 s；fat 123/0，六目标网络镜像等于参考（Linux/Windows 本轮不宣称 VM 实跑）。日志 /tmp/unisacc-szd-gate.log。
- gate 之后只有上述 E3 分派回退及 s52/固定清单改变，产品未变；最终再跑 E3 218、chain 79、s52 六段完整镜像，以及 osx/arm64 网络 selfcheck（743,202 B、N1=N2=N3），均通过。没有把前一轮 70 项门禁冒称为最后两行解析调整后的全量复跑。日志 /tmp/unisacc-szd-final-{keep,chain,self}.log。未推送、未发布；cc-unisacc 保持暂停。


### S-17：网络路线重建通用执行器（2026-09-27）

- `exec/c/run.c` 现在由六段实际阈值网络生成 osx/arm64 可执行文件，字节等于原编译器构建结果；该执行器再加载同一组网络，重编自身源码，六个中间结果与最终镜像逐字节相同。`nativecheck.sh` 固定这条检查，并保留六段全域核对、原有 18 份输出比较、三种执行器的真实写失败。Python 仍构造模型，未参与六段的源码处理；`.com` 尚未切换路线，前端完整覆盖、几 KB 内核和单份模型打包仍未完成。
- 实际源码驱动的 E3 修改：命名/成员/下标左值共享后缀增减与赋值，结构体复制统一到同一例程；指针成员初始化复用既有 8 字节存储；地址/成员遍历复用 MEMB；case 复用 CE；全局初始化根据首次声明记录的名字处理，不再被数组界限中的 enum 名覆盖。没有增加执行器原语。s53/s54/s55/s57 分别固定左值、聚合复制、指针初始化、含 enum 界限的全局声明与 64 位常量。
- 独立 cc 比较发现产品宽 case 标签被 int 截断：旧产物输出 `1 2 3 5 4`，应为 `1 2 3 4 5`。catom/cexpr 与 case 存储保持 long；case 按控制表达式的提升类型转换，32 位有符号/无符号及嵌套状态分别处理。C/Python/E3 同步；b_casewide 在 cc、最终 .com O0/O1/O2、Python 输出 `1 2 3 4 5 / 6 7 8 9 10`。这是已测整数路径，不声称完整 C 常量表达式语义已证明。
- 扩清单前旧 218 E3 全通过；最终 224/224 equal（含真实 runtime）。chain/ELF 79→85，全部一致。新探针六网络 macOS 镜像实际运行与 cc 相同。gen2.py +59/-49（净 +10 行），状态 3967→3991，JSON 21,725,618→21,869,510 B；复用减少重复路径，但本批源码没有净缩减。
- 最终冻结代码 gate --com：70/70、294 s aggregate、JOBS=2，每套件 ≤60 s；exec-native 19 s，fat 124/0 两架构实跑，六目标完整源码镜像相同。Linux/Windows 本轮仅核对生成字节，未启动 VM；没有新的跨平台运行主张。日志 /tmp/unisacc-runtime-model-gate.log、/tmp/unisacc-runtime-model-keep.log。
- `.com` 1,348,688 B，SHA256 `c018f8275910808d8e7e750b437faa298ab815a93ef1699080fbb923b6ef7967`；unisacc.c SHA256 `aab9b9378dd5d81a0df3066cf514e81742d737be0fe609886a5880812f464edc`。未推送、未发布；cc-unisacc 保持暂停，重构目标继续。


### S-17：产品语料中的三处接受差异（2026-09-27）

- 在 `0d3358b` 的 70 项门禁后，额外检查 examples、examples/apps、tests/c 共 130 个输入：95 equal、32 not-covered、3 DIFF。固定清单通过不能替代这一探索结果。差异为 queens 的 unsigned 按位取反缺少截断，以及 b_fuzzfound/b_structarg 的按值结构体参数没有局部副本；未把接受差异改名为 not-covered。
- unsigned int 的 `~` 复用 NARU。参数全部 spill 后，才为结构体逐个分配局部副本；调用端同样建立临时副本，避免后一次结构体返回覆盖前一实参。赋值、参数、实参临时值、返回共用 COPYSTRUCT，删除原来返回只支持 8 字节倍数的重复循环。没有执行器原语或产品源码改动。
- s58 固定 3 字节结构体、多结构体实参、连续结构体返回和超过六参数的栈约定：cc、现有 .com 与网络产物输出 `924 44 3 4 5`，调用后原对象仍是 `3 4 5`。另外三例的网络完整镜像等于参考，实际输出等于 cc。旧 224 项先通过再扩至 228；source-to-ELF 固定清单 85→89，89 项全部通过。nativecheck 再次通过网络执行器自身重建及真实写失败。
- 同一 130 输入重新检查：99 equal、31 not-covered、0 DIFF（通用复制顺带使 b_struct3 一致）。保留未覆盖清单，不称全部 C99 产品测试已迁移。日志 /tmp/unisacc-structargs-{keep,frontier,srcelf,native,network}.log。
- gen2.py +26/-13（净 +13 行），4007 状态，JSON 21,971,103 B。仅定向回归，没有把前一提交的 70/70 称为此提交的完整门禁；产品 .com、源码哈希沿用上一记录。下一重点仍是这些真实语料的拒绝缺口与最终产品接入，不以 equal 数量代替收尾。


### S-17：共享聚合初始化与静态数组声明（2026-09-27）

- 删除 E3 全局数字列表的独立 walker；全局、自动、静态聚合初始化共用 INITLIST，只由 INITADDR 区分地址来源。全局/静态初始化在声明处生成并缓存，随后回放到 __init，保留字符串编号和声明顺序。静态路径复用同一 walker，未增加执行器原语。无括号 `sizeof lines[0]` 复用已有维度求积，修正先消费名字后留下下标的缺口。
- 新探针 s59 同时覆盖三种存储期的二维短行、零填充、指针和结构体成员；calc 与 b_ptrdepth 也由共享路径通过。旧 228 E3 项先保持，再加入三例，231/231 equal；chain/ELF 固定清单 89→92。三个完整网络 macOS ARM64 镜像逐字节等于参考，实跑等于 cc；b_ptrdepth 给宿主 cc 显式包含 stdio.h（其源码依赖产品自动包含），没有把宿主缺声明的首次编译失败写成编译器差异。
- 探针发现产品静态局部数组只解析一维，合法的二维声明被拒。C 前端抽出 dimtail，自动、静态、全局声明共用第二/第三维处理，并给静态聚合初始化传入行尺寸。Python 已支持，不改其实现。产品 b_staticarray 覆盖二维/三维、逗号声明、sizeof、跨调用持久性，cc、Python、最终 .com O0/O1/O2 均输出 `24 48 6 2 0 9 1 / 24 48 6 4 0 9 3`。不据此称网络 frontend 已支持三维嵌套初始化。
- gen2.py 净 -33 行，statics.py 净 +4 行，合计 -29；状态 4007→3969，JSON 21,971,103→21,768,127 B。减少的是重复初始化逻辑；未把规则搬到执行器或另一个生成脚本。任意嵌套聚合、designator、完整 C99 初始化语义仍未覆盖。
- 冻结代码 gate --com 70/70，JOBS=2，合计 298 s，每套件 ≤60 s；fat 125/0 两架构实跑，chain 92/92，source-to-ELF 固定清单通过，六目标网络自源码镜像相同，nativecheck 18 s。Linux/Windows 本轮未启动 VM，仅验证生成字节。日志 `/tmp/unisacc-init-gate.log`、`/tmp/unisacc-init-final-keep.log`、`/tmp/unisacc-init-network.log`。
- 重建 .com：1,348,432 B，SHA256 `d1d29056f7d8fb32e2173d8c6fd71ac8872580ab575d0834fc8251a903b22e83`；unisacc.c SHA256 `1442c8ee298439f6adf3ea934ea012dd1292f21a26790d5abe9c30dafd7ed3b9`。未推送、未发布。产品入口仍未切换为网络运行时，重构未完成；cc-unisacc 保持暂停。


### S-17：递归聚合上下文与成员数组（2026-09-27）

- 全局、自动、静态初始化继续共用一个 INITLIST；用嵌套上下文记录当前聚合的标量槽范围，右括号跳过省略成员，designator 在当前层解析。成员偏移/槽数来自已解析布局，宽度复用 ELSZ/STOREV；默认 union 只遍历第一成员。原二维专用循环被替换，新增 initializers.py 仅组织生成器过程，没有执行器原语。省略维度按聚合槽计数；全局标量表达式也共用 EXPR/STOREV，删除独立地址/字符串指针初始化路径。PWIDTH 复用 ELSZ，修复 double 指针步长。
- s60 固定三种存储期的嵌套结构体数组、三维数组、designator、零填充、跨调用持久性、默认 union 与 double 元素。b_init6/b_init7 由同一机制通过。期间发现并修复逗号声明类型状态未保存、&function 被新公共表达式路径拒绝的回归；旧 231 项先全部通过，再扩至 234/234。chain/ELF 92→95，全通过。实际产品语料 132 项：104 equal、28 not-covered、0 DIFF，不称前端完整覆盖。
- 独立 cc 对照发现 C 产品把结构体数组成员当成结构体值，取下标步长错误。新增显式 mbarr（包括长度 1），随匿名/复制成员传播；成员数组衰变时设置指针深度与元素宽度。Python 原本正确。b_structarrmember 的 cc、最终 .com O0/O1/O2、Python 结果为 `3 4 20 9 10 30 16 16`；该产品回归在 E3 仍因 sizeof 复杂成员表达式未覆盖，不虚列入 equal。s60 覆盖不含该 sizeof 的成员数组路径。
- 三个新增 E3 输入的六网络 osx/arm64 完整镜像等于参考，实际输出等于 cc；b_init6/b_init7 为宿主显式包含 stdio.h。gen2.py 净 -60 行，initializers.py +81，合计 +21；状态 3969→4033，JSON 21,768,127→22,187,240 B。这是统一规则路径与补齐已测形状，不是源码净缩减；任意初始化语义、非首 union designator 等不在完成主张内。
- 最终冻结 gate --com 70/70，JOBS=2，297 s aggregate，每套件 ≤60 s；fat 126/0，两架构实际执行；六目标网络 self-source 等于参考，nativecheck 18 s。Linux/Windows 本轮未启动 VM，仅验证生成字节。日志 /tmp/unisacc-aggr-{finalkeep,frontier,network,gate}.log。
- .com 重建为 1,348,784 B，SHA256 `66d466a7d2a48796b62a30a71167d7befd4a08b6e301317a73806b11ce28041a`；unisacc.c SHA256 `577e9261925d3c3c2becbd3247b2c12ab6f85e650e7dd5180ed14507dc835d6e`。未推送、未发布。cc-unisacc 保持暂停；完整前端、失败契约、紧凑内核与单份模型 .com 切换仍需继续。


### S-17：单进程网络字节流（2026-09-27）

- `run.c` 抽出 execute，新增开发入口 `--chain INPUT SRCPATH INCLUDE_DIR MODEL...`。列表显式、有序、长度不限于六段；驱动不认识 C 阶段或目标架构。每次只把上一段接受的字节交给下一段，与原文件边界相同，不传递输出属性。每段释放模型与临时状态，只有接受的输出存活；stdout 在全部接受后才写出。旧单模型与全域核对接口保留，没有新增模型动作。
- 普通运行的寄存器、索引内存、栈、reader frame、blob、intern、文件缓存逐段重置；SWAP 释放已替换的输入。netcheck 对同一带状态模型连续运行 20 次，检查空输出、拒绝/步数超限后不打开后续不存在的模型、无半成品 stdout。cc 与 unisacc 构建均通过；nativecheck 的三种构建（cc、unisacc、六网络生成）对 hello/b_toknames/runtime 源码跑内存链，均与原六进程镜像相同，含网络执行器重建自身。ASan+UBSan 在 hello/b_printf3/runtime 三个真实六段输入上通过，无报告；未把它称为全平台内存证明。
- 同批删除 E3 对已定义 printf 的专用格式预扫描，直接 CALL；只有未定义 fallback 才要求字面量。b_printf3 动态格式通过且 macOS 网络产物实跑等于 cc。旧 234 项先保持，再扩至 235；chain/ELF 95→96。gen2.py 净 -11 行，4029 状态，JSON 22,169,415 B。不是新增 printf 规则，也未把规则放进执行器。
- macOS ARM64 六网络文本合计 917,921 B；cc -O2 runtime __text 16,688 B / 文件 56,496 B（动态 libSystem 不计），网络/unisacc 构建 runtime __text 87,808 B / 文件 115,746 B（含库）。这是加载/IO/校验/执行合计，不是独立几 KB 内核。run.c 净 +43 行，用于重入、清理与字节流驱动。模型仍是显式六文件，尚未合成 E7 单份 payload，也没有对外切换 .com CLI。
- 冻结 gate --com 70/70，JOBS=2，312 s aggregate；各套件 ≤60 s，最大 40 s，新增 nativecheck 39 s。fat 126/0，六目标 self-source 字节对齐。Linux/Windows 未启动 VM，单进程本轮实际运行仅 macOS ARM64。日志 /tmp/unisacc-stream-{native,network,gate}.log 与 /tmp/unisacc-call-keep.log。产品源码/.com 未改变，沿用 dec6553 的产物哈希；未推送、未发布。下一步仍须完成产品驱动/单份模型接入、紧凑内核与前端/错误路径缺口，目标未完成。


### S-17：共享模型包与声明路线（2026-09-27）

- 新增构造期 pack.py：读显式 TSV 清单（路线、阶段、输入格式、输出格式、网络路径），按完整网络字节去重。runtime `--bundle PACKAGE ROUTE INPUT [SRCPATH] [INCLUDE_DIR]` 只读一个包，先核对目录、邻接格式、重复阶段、索引与范围，再复用既有 loadbytes/execute；不含 C 阶段或架构决策，不启动 Python。包格式 P1 及边界写入 exec/c/PACKAGE.md。发布 CLI 未改变。
- image-stages.tsv 声明六段顺序和格式；elf.sh 用其顺序，构造与全域核对后写 route.tsv/models.pkg。原逐段文件路线仍作为比较对象。旧 pipeline/stages.tsv/run.py 仍是早期三段参考工具，没有冒称所有历史工具已共用唯一清单；独立格式检查与产品参数接入仍需收敛。
- 六目标 36 个条目合为 21 份唯一网络：原分散网络合计 5,595,439 B，单包 2,555,823 B，gzip-9 参考值 359,340 B（运行器不解压）；SHA256 `5b69ca54701e191f7b7f59aee9537532f514f8d889a3337539d49823618ae379`。合包的 hello 六目标镜像均等于各自散文件路线。另在门禁中，各目标完整 unisacc.c 从各自包生成的镜像均与逐段及产品参考相同；没有把 hello 合包测试写成六目标合包的完整自源码实跑。
- cc/unisacc 的 netcheck 通过共享体、路线选择、格式失配、重复阶段、越界索引、截断包；nativecheck 在 cc、unisacc、网络构建执行器上从包重建 runtime，输出相同。ASan+UBSan 对包内 hello 和 runtime 全六段通过、无报告；E3 对新增包加载器源码仍 equal。单目标 macOS ARM64 包 918,211 B。
- 规模：cc -O2 runtime __text 18,636 B，文件 57,648 B，gzip-9 15,780 B；网络/unisacc runtime __text 92,452 B，文件 115,746 B，gzip-9 21,090 B。动态 libSystem 不计入 cc 文件，unisacc 包含随带库；这些都是整个工具而非独立内核。运行器净 +66 行用于目录与加载，未增加模型动作。几 KB 内核和 .com 内嵌单份模型仍未达成，包也未包含产品内嵌头文件资源。
- 冻结 gate --com 70/70，JOBS=2，360 s aggregate；各套件 ≤60 s，nativecheck 53 s（余量仅 7 s），六目标包自源码各 29–33 s。fat 126/0、chain 96/96。macOS 实际执行，Linux/Windows 本轮未启动 VM，仅生成字节对齐。日志 /tmp/unisacc-package-{native,network,gate}.log；六目标合包产物 /tmp/unisacc-package-six/all.pkg。产品源码/.com 未改，沿用 dec6553 的哈希。未推送、未发布；cc-unisacc 继续暂停。


### S-17：包内命名字节资源（2026-09-27）

- 包格式增加兼容 v1 的 v2 资源目录：非空字节名、任意内容（可为空），长度在索引前检查。构造期 `--mount PREFIX_HEX DIRECTORY` 显式读取目录；相同键/相同内容合并，冲突拒绝。runtime 通过现有 SBFIND 先读精确字节键资源，再走原文件系统适配；缓存仍逐阶段清理，资源本身保持不可变。不新增模型动作、不包含语言相关解码规则。
- elf.sh 将 19 份 include/*.h 以 E2 既有的 NUL-hdr/ 名称挂入包。它们是 103,636 B 的源文本资源，不是网络权重；单目标 osx/arm64 包 1,022,275 B，gzip-9 参考 157,046 B（运行时不解压），SHA256 `a91b49311e348a91b3e4fd91b1a4280e4c98e0077e13ee7162fd776d636d44c2`。这不是已嵌入 .com 的发布产物。
- nativecheck：隔离目录只有 runtime.c 和 models.pkg，没有 include/，也不给 include-directory 参数；cc、unisacc、网络构建的三种 runtime 都生成相同自身镜像。去掉资源的同源包在该目录以 2 拒绝、无 stdout，避免隐式读回仓库的假通过。ASan+UBSan 对同一隔离资源路线生成自身通过，无报告。netcheck 覆盖含 NUL/高字节的内容、空资源、重复读取缓存、两段间重置、冲突挂载、缺目录及截断/越界资源。
- 运行器净 +17 行 C；cc -O2 整体 __text 19,140 B，文件 57,904 B，gzip-9 16,240 B；unisacc runtime __text 94,448 B，文件 115,746 B，gzip-9 21,516 B。包含加载/IO/校验/执行及各自库口径，不以这些数字冒称独立几 KB 内核。产品程序源码与用户自备头文件仍是文件输入。
- 冻结 gate --com 70/70，JOBS=2，364 s aggregate；各套件 ≤60 s，nativecheck 54 s、余量 6 s，六目标包自源码字节一致；fat 126/0，两架构实际执行。Linux/Windows 本轮未启动 VM。日志 /tmp/unisacc-resource-{native,gate}.log。E3 对修改后的 runtime 源码单独 equal；固定清单仍 235/96。产品源码/.com 未改，仍沿用 dec6553 的哈希；未推送、未发布。单份 payload 嵌入、产品驱动/CLI、紧凑内核与剩余前端/错误契约继续推进，cc-unisacc 保持暂停。

### S-17：单份模型包嵌入开发 .com（2026-09-27）

- APE 打包增加可选 `--payload`，包在所有切片之后只存一次；16 B 尾部由 `UNIPKG1\n` 与 LE64 长度组成。runtime 先验证范围，再复用已有包/模型加载器。Unix 启动脚本把原容器路径放入 UNISA_CONTAINER，避免缓存的架构切片丢失包来源；`--embedded ROUTE INPUT` 为开发入口。无环境变量时尝试 argv[0]，Windows PE 路径尚未实跑，不称已通过。
- 六目标合包含 21 个共享模型体、36 个阶段目录项和 19 个头文件资源，2,659,887 B，SHA256 `f3edc4e9ff9faf172f4f39105e01678623f7aaf5c23a2adb37b663704ce0e23a`。独立开发容器 `/tmp/model-runtime.com` 为 2,845,967 B，SHA256 `d342972ce88e278d77ba1f4db4a88fc4d67db16f920a644efc0ed4479fb345cb`。使用当前 runtime 源码与此前已验收六路线模型构建；不覆盖产品 unisacc.com。
- 隔离目录中只有开发容器和 hello.c：六目标输出均与外置同包路线逐字节相同，osx/arm64 镜像实际打印 hello from C99；其余为字节检查，本轮未开 Linux/Windows VM。通用 embeddedcheck 检查两容器共用同一执行器切片但携带不同模型、路径含空格、删除外置包后仍输出各自结果。netcheck 在 cc/unisacc 构建上拒绝尾部截断、错误 magic、零长度与超界长度，无 stdout。E3 对 runtime 源码保持 equal。
- 默认无 payload 的打包结果 SHA256 仍为 `66d466a7d2a48796b62a30a71167d7befd4a08b6e301317a73806b11ce28041a`，与现有产品逐字节相同。冻结 gate --com 71/71，JOBS=2，362 s aggregate；nativecheck 55 s、新增 embeddedcheck 3 s，单套件均 ≤60 s。日志 `/tmp/unisacc-embedded-gate.log`。
- Python 用于离线模型构造与 APE 包装，开发容器执行阶段不调用它。运行器仍包含加载/IO/校验等，独立几 KB 内核尚未完成；产品 CLI/默认路线尚未切换，前端与失败契约仍有缺口。单份模型嵌入已落地不等于完整重构完成。未推送、未发布；cc-unisacc 保持暂停。

### S-17：普通编译命令接入网络包（2026-09-27）

- 新 `exec/c/compiler.c` 复用 run.c 的字节流执行器，独立入口只做参数/文件/路线选择；没有源码解析、tape 生成、优化、lowering 或编码代码，也不调用参考编译器。run.c 的工具入口与 --check-net 在库构建中排除；runroute 共用于开发执行器与新驱动，readstream 支持 stdin。产品 unisacc.com 未切换。
- `compiler-routes.tsv` 声明 pp、tape/O0/O1/O2、image/O0/O1/O2；compilerpack.py 将已有 image 清单展开为路线目录，O0 跳过优化器，O1 使用显式构造的 O1 网络，O2 复用原网络。六目标包 42 条路线、174 阶段项、22 个去重模型体、19 资源；编译的动作仍由网络执行，没有复制各目标的模型体来实现 CLI。
- 已接 `-E`、`-S/-c`、`-b/-t`、`-O/-O0/-O1/-O2`、`-o`、一个 `-I`、单文件及 stdin。默认目标/输出模式与现产品对应。成功接受后才打开输出文件；短写循环推进、写入停止与关闭失败均拒绝。`-run`、多单元、宏/强制 include、依赖输出、告警/仪器等尚未迁移，显式拒绝，不默默退回旧实现。完整 CLI/失败诊断一致性仍未达成。
- 新 compilercheck 每次构造模型；O1 的 49,538 个观测全域等价检查通过。cc/unisacc 驱动 18 组模式×优化级输出与当前产品逐字节相同，另核对 stdin、无效输入不截断既有文件、未迁移选项、未知路线、打不开输出与真实 RLIMIT 短写。六网络构建新驱动自身，镜像与 unisacc 构建相同；该网络构建驱动再编译 hello 输出相同。隔离目录仅有嵌入式开发容器及 hello.c，普通 hello.c -O2 生成 a.out 并实际打印 hello from C99。E3 对 compiler.c 与 run.c 均 equal。
- 产物：六目标包 2,675,518 B（gzip-9 388,434 B），SHA256 `4c04fbddbcdbcfc4e051d89f676ad5bec92f41588e433e4daa3057046c27a98d`；独立 `/tmp/compiler-model.com` 2,861,502 B（整个文件 gzip-9 499,295 B），SHA256 `6e46ec4168d6da8169be3b5b446015f37b85326ac85b0954e23d2c475869f3f5`。运行时包不解压，这里 gzip 仅为账目参考。cc 驱动整体 __text 20,176 B/文件 57,808 B，unisacc 驱动整体 __text 95,368 B/文件 115,746 B（含随带库），仍不是独立几 KB 内核。新驱动 105 行，包展开 43 行；功能尚不等于旧 main，不把行数差说成整个产品缩减。
- 冻结 gate --com 72/72，JOBS=2，370 s aggregate，最长 54 s；新 exec-driver 23 s。日志 `/tmp/unisacc-driver-gate.log`。当前产品源码与 .com 均未改；本轮实际执行 macOS，Linux/Windows 未开 VM。后续继续其余 CLI、前端/失败契约、紧凑内核与最终默认切换。未推送、未发布；cc-unisacc 继续暂停。

### S-17 continuation: command-line macro inputs interpreted by E2 (2026-09-27)

- `compiler.c` now passes raw `-D`, `-U`, `-include`, and one `-I` value as four NUL-named, immutable process resources. The generic runtime only serves exact byte keys through existing SBFIND; no new action or macro parser is in C. E2 performs name/body parsing, forced-source prefixing, define/predefine/undefine ordering, and source-relative / -I / include/ / carried-header search. Runtime action semantics and network inference are unchanged. Macro expansion of an empty body now emits the reference's single separating space.
- The integration exposed two product defects, fixed in `1e1a6b3`: string literals had pointer size under sizeof (the embedded-NUL resource name was truncated to seven bytes); `'U'` and `'u'` incorrectly acquired unsigned integer type from their contents. C/Python now preserve literal array extent and decay it for pointer arithmetic, conditional and comma results. Python also accepts the parenthesised comma operand used by the regression. New b_strsizeof and b_charkind agree with host cc in C O0/O1/O2, Python, and the rebuilt product .com O0/O1/O2. The character rule is unchanged in Python, which was already correct.
- E3 recognises complete literal operands of sizeof, including redundant parentheses and decoded escapes, without changing string-pool numbering; other operands return to the existing expression path. Old keep-e3 235/235 passed before adding s61 and b_charkind: 237/237 equal, 107,047,479 Python action steps, no refusal/difference/tool failure. Driver source itself is equal. This does not claim all pointer/wide sizeof forms are now covered by E3.
- Fresh compilercheck: cc/unisacc native drivers and Python E2 action oracle agree on 13 macro/include cases (empty/default/expression/negative body, replacement, -U before/after -D, target predefines, forced-header ordering, quoted local header priority, -I override of carried stdio). A -D-controlled complete image also equals the product. Existing 18 mode/level comparisons, stdin, failures before opening output, real write failure, network-built driver, and isolated embedded container still pass. Exact failure diagnostic rendering and the remaining CLI are not claimed complete.
- Frozen gate --com: **72/72**, JOBS=2, 370 s aggregate; longest nativecheck 54 s, driver 23 s, each suite <=60 s. Log `/tmp/unisacc-cli-gate.log`; E3 fixed-list log `/tmp/cli-e3-keep.log`. Real host execution macOS arm64/Rosetta; Linux/Windows VMs not started this round. Cross-target byte equality is not a VM run.
- Product .com rebuilt: 1,349,584 B, SHA256 `e46a8baaf0c7c4e49e2b2cf9eba0b6509e4150acd56a2afff777665783e03dd0`; generated unisacc.c SHA256 `a0bdb5014bb4f481fb11e76ee9b02295da6299d0fd4d8e0af2bb979f87322afd`. The product includes the literal fixes, but its default compiler route has not switched to the new network driver. -run, multiple units, remaining options/diagnostics, frontend gaps, and the compact executor remain. Local commits only, no push or release; cc-unisacc stays paused.

### S-17 continuation: model-generated native-memory execution (2026-09-27)

- The development driver now connects POSIX `-run`. Lowering and encoding networks receive raw argc/argv and mapped-base resources; they compute argument cells, addresses, relocations, code/data and entry. The C adapter only allocates adjacent memory, validates the UNIMEM1 byte image, copies it, makes text RX and enters it. No temporary executable, reference compilation, new executor action or runtime Python is used. A size-plan pass and a bound pass reuse the same encoder network; ordinary ELF/Mach-O/PE routes remain available.
- `memorycheck.sh`: for hello and function-pointer input, code/data/entry match the retained bk_run backend at its actual mapped addresses, independently through the Python action oracle and C network runtime. cc/unisacc drivers match 24 native runs (six programs, O0/O2), plus explicit argv/environment/O1/exit-7 checks. The unisacc main argv copy omits the environment; the adapter therefore reads the existing process-vector intrinsic. Both builds reject malformed memory lengths/entry/extent. Both executors reject half-specified mapped bases and argument headers without process context.
- Fresh driver integration remains green: 18 mode/level comparisons, CLI resource cases, network-built driver, write failures and isolated embedded container. The embedded development container also runs hello in memory without creating an executable file. No product source or shipped .com changed in this slice.
- Frozen `gate --com`: **73/73**, JOBS=2, 387 s aggregate, each suite <=60 s; new exec-memory 31 s, exec-driver 25 s. Log `/tmp/unisacc-memory-gate.log`. New memory execution was measured on macOS arm64; existing gate also exercises Rosetta image paths, which is not evidence of x86 memory execution. Linux/Windows VMs were not started.
- Windows memory imports remain explicitly unsupported. Multiple units, remaining CLI and failure compatibility, frontend coverage, isolated compact executor and final product switch remain unfinished. This is a development-driver slice, not completion of S-17. Local commit only; cc-unisacc remains paused, no push/release.

### S-17 continuation: Windows memory imports and Rosetta execution (2026-09-27)

- The development driver now runs model-generated Windows code in memory. A generic OS adapter enumerates its loaded PE's named imports into immutable process resources; the encoder network chooses the declared imports, lays out IAT slots, binds code/data addresses and emits UNIMEM1. Allocation/protection remain OS operations. No compiler import-name list, instruction encoding or new executor action was added to C. Ordinary image routes remain unchanged.
- Windows optional header lookup follows the existing product's try-next behaviour: its native open wrapper does not provide POSIX errno discrimination. Explicit missing input still fails; this is not a claim of POSIX error fidelity. The adapter uses unisacc's 64-bit long, not Windows host-cc LLP64.
- Fresh simulated binding checks on both Windows architectures compare lower/encoder network output with the action oracle and independent assembly at declared addresses, including import slots, argv cells, entry and extent. Generic PE import enumeration gives 14 exact names/values in cc and unisacc builds and rejects an out-of-image name. These simulated checks are separately labelled.
- Actual Windows 11 arm64 VM: a network-built driver, byte-identical to the reference-built driver, passed eight memory runs per architecture (hello/function pointers at O0/O2, argv/O1/exit 7, file IO, malloc, explicit missing input). arm64 was native; x86_64 used Windows emulation, not an x86 physical host. Driver SHA256: arm64 `1201c5610c24a4dd036270835f6a805648e93993539ff09daa3569bbef95d678`; x86_64 `7b8fbf28a0a36968055d83fbe6674cab8e4f025b1103eb44e2c48e7e6bacfa5b`. Evidence under `/tmp/unisacc-memory-win-arm/` and `/tmp/unisacc-memory-win-x86/`; VM stopped afterwards. Windows embedded APE runtime was not tested by these standalone PE runs.
- Rosetta x86_64 now runs the same native-memory checks as macOS arm64, including binding to the retained backend, 24 executions, argv/environment and loader bounds. Frozen gate --com **76/76**, JOBS=2, 415 s aggregate; each suite <=60 s. Memory arm/x86 suites 31/34 s, simulated Windows arm/x86 4/3 s. Log `/tmp/unisacc-winmemory-full-gate.log`. No product source or shipped .com changed.
- Linux native execution of this new memory route remains unmeasured. Multiple units, remaining CLI/diagnostic compatibility, frontend coverage, compact isolated executor and final default switch remain unfinished. Local commit only; no push/release; cc-unisacc remains paused.

### S-17 continuation: Linux ARM64 memory execution (2026-09-27)

- Fresh six-network Linux ARM64 route built `exec/c/compiler.c`; its ELF is byte-identical to the reference-built driver, SHA256 `b7618ad8fb3265186062b13c90a489e129920cff3e38f89ecde61675245d24bc`. Package 1,045,263 B, built from the current route models and the unchanged O1 network. No runtime Python or retained compiler path is used by the driver.
- Actual native ARM64 Lima run: nine programs (hello, fib, struct, argv, printf, static, function pointers, file IO, malloc) at O0/O1/O2 give **27/27** output/stderr/status matches against guest system cc, plus explicit argv/environment/exit-7 and missing-input rejection. The test Python script only runs the independent compiler and compares results. Per-process bound 15 s, outer run 60 s, exit 0. Evidence `/tmp/unisacc-memory-linux-arm/native-result.log`; reusable opt-in harness `exec/c/posixmemoryrun.py`.
- Dedicated minicon-lnx-aarch64 initially lacked cc: that attempt failed before comparisons and is not counted. It was stopped. Tests instead ran in the already-running default ARM64 Lima VM with system gcc; that VM was left running. Windows VM remains stopped. Linux x86_64 memory execution is still unmeasured.
- This adds only a test harness and evidence/documentation after the frozen 76/76 gate, not compiler changes; the full gate was not redundantly rerun. Product .com remains unchanged. Local commit only; no push/release; remaining S-17 work continues.

### S-17 continuation: multiple translation units through model inference (2026-09-27)

- The development driver now accepts multiple C files for tape, image and native-memory output. It runs each file through fresh E2/E1 state, preserving that file's path/macros/header guards, then passes LE32-length-framed typed streams to a shared network. C only frames bytes and dispatches routes; declaration scanning, file-static renaming, unit markers and shared-program state are handled by models. Runtime Python and reference fallback are absent. Multiple -E outputs and exact diagnostic compatibility remain unfinished.
- New units.py is an 89-line handwritten constructor compiled into a threshold network, not a gold-derived rule or code reduction: 417 states, 1,400 hidden units, 31,558 B; all 107,330 observations equal its table, with identical actions/strings. It validates up to 64 units and uses the reference's later-unit __uN names. Inline aggregate static specifiers remain explicitly unsupported. The package reuses one network body across routes.
- Integration exposed a product defect fixed in acb7e84: per-file token ordinals gave two functions' block statics the same ls8 storage. Independent system cc returned 19; the old product returned 100. Labels are now unit*MAXTOK+token, with original token identity retained for symbol lookup and unit-zero labels unchanged. E3 uses the same source-declared MAXTOK at construction. Python already separated units. tests/staticunits.sh checks both orders against cc in product C O0/O1/O2, Python and rebuilt .com, including static scalar, character array, aggregate and string-pointer initializers.
- E3 keeps one program symbol/string/label space, tracks the declaring unit and resets only per-file token ordinals. The static-character-array case exposed a missing prototype path; global, automatic and static string initialization now share one emitter. E3 states fell from 4,072 to 4,058 in that refactor. The fixed list remains 237/237 equal (107,047,638 action steps), with no refusal, difference or tool failure; no acceptance list was weakened.
- New exec-multi checks 24 tape comparisons (two existing pairs, both orders, all three optimization levels, cc/unisacc drivers), native runs, independent macro/guard/static-function-pointer behavior (cc exit 27), block-static isolation (exit 19), and a missing later input without truncating output. Framing acceptance and seven malformed frames are checked on the action oracle and native inference. Host package 1,092,647 B. The new driver remains a development route, not the product default.
- First frozen run was **77/79**: exec-multi exposed the missing static-array path, and com-staticunits used direct exec on an APE file (ENOEXEC). Both were fixed without dropping tests. Final frozen gate --com: **79/79**, JOBS=2, 428 s aggregate, each suite <=60 s; exec-multi 28 s, driver 25 s, nativecheck 54 s, fat 128/0 with both slices executed. Logs /tmp/unisacc-multi-final-gate.log, /tmp/unisacc-multi-keep-final.log and /tmp/unisacc-multi-check3.log. macOS arm64/Rosetta were exercised; no fresh Linux/Windows VM run this round.
- Rebuilt product unisacc.com: 1,349,776 B, SHA256 `1ff98759433f58acc6e74e37b19e17d2c0a322934a54d958a0a69870207e190d`; generated unisacc.c SHA256 `b0c86ced0b40ee73def60b5cfffd517ad5c6b2509c632c27e9ba1d5cd2a57c8e`. It includes the storage fix, not the new default route. Remaining CLI/failure compatibility, frontend gaps, isolated compact executor and final switch remain active. Local commits only; no push/release; cc-unisacc stays paused.

- Post-gate lifecycle cleanup: the multi-input framing buffer's position array is freed before route inference (single-input NULL remains safe). This removes retained scratch storage without changing bytes or model semantics. Fresh multicheck and compilercheck both pass, including cc/unisacc drivers and the network-built driver; logs /tmp/unisacc-multi-buffer-{check,driver}.log. This one-line follow-up is covered by those targeted runs, not by the preceding frozen 79-suite run. Product source and .com are unchanged.

### S-17 continuation: independently compiled generic C kernel (2026-09-27)

- core.c/core.h now contain inference, all unchanged actions, arithmetic, input/control stacks, byte/attribute buffers, sparse memory, blobs, interning, resource-cache identities and reset/free. run.c retains decoded model/package loading, route dispatch, step-limit configuration, exact resource/file adaptation and diagnostic printing. Two fixed host symbols fetch opaque byte keys and report fatal failures; no compiler-specific primitive or model rule was added to C. Model/action bytes are unchanged.
- The first interface used function-pointer members in a host structure and exposed an E3 struct-member-call refusal. A single-active-execution kernel needs no per-instance callback table: the final interface uses ordinary fixed linkage. The independent object and default included-C build share the exact same implementation. This is an interface simplification, not a claim that E3 now supports that rejected syntax.
- Direct `cc -Os` object measurement: **arm64 __text 6,468 B, x86_64 __text 7,496 B**, uncompressed, including every generic helper. Constants/strings/static storage/unwind and imports are separately recorded in exec/c/CORE.md; gzip-9 of object text is 4,004/4,232 B only as a reference. libc memory primitives and host adapter implementations are external dependencies, not silently counted as zero whole-product cost. These are C compiler measurements, not unisacc-generated core sizes or handwritten assembly. The few-KB C baseline is now measurable; assembly and final slicing remain unfinished.
- Whole run tool on macOS arm64: cc -Os __text 16,184 B/file 55,288 B/gzip-9 14,187 B (dynamic libSystem excluded); unisacc -O2 __text 98,288 B/file 132,258 B/gzip-9 22,613 B (carried library included). Different build modes, no performance comparison claimed. The former 603-line run.c becomes 418 lines +254 core.c +32 core.h =704; this is isolation and explicit ownership, not source reduction.
- corecheck links the object separately and repeats inference/stream reset/failure/resource/container tests; its undefined-symbol audit allows only generic libc/stack-protector support and the two host entries. The opcode/arity crosscheck now reads core.h. Nativecheck again passes cc, unisacc and network-built runtime, six full-domain checks, 18 outputs, in-process chains, self-reconstruction and real short writes. Isolated self-build inputs explicitly include core.c/core.h; nothing is silently fetched from the repo.
- Frozen gate --com **80/80**, JOBS=2, 430 s aggregate, each suite <=60 s; nativecheck 51 s, fat 128/0 on both slices, chain 96/96. Log /tmp/unisacc-core-full-gate.log. Afterward the separate-linkage network checks also passed ASan+UBSan with halt-on-error, no report (/tmp/unisacc-core-sanitizer.log); leak detection was disabled because deliberate fatal-loader cases terminate without cleanup, so no leak-freedom claim. This specifically checks the new borrowed/owned resource and result boundary.
- No product source, .com or model payload changed; .com hash stays `1ff98759433f58acc6e74e37b19e17d2c0a322934a54d958a0a69870207e190d`. macOS execution this round; no new Linux/Windows VM claim. CLI/diagnostic compatibility, frontend gaps, assembly core and final default switch remain. Local commit only, no push/release; cc-unisacc stays paused.

### S-17 continuation: handwritten inference on both ISAs (2026-09-27)

- Added handwritten core_transition assembly for AArch64 and x86-64 System V. Both evaluate the actual threshold network with signed 64-bit accumulators, including negative weights, and retain the table-control path for comparison. No compiler rule or new action was added. CoreModel offsets are build-asserted against the real C struct; no external function is called from either routine. The selected development build omits the C transition body rather than calling it behind the assembly entry.
- Each ISA passes **537,620** checks against the retained C function plus independently specified missing/domain/output/64-bit-overflow expectations. The ordinary network/stream/reset/error/resource/container checks also pass with the assembly symbol linked. Four fresh six-stage images (hello, fib, b_strderef, run.c) per ISA equal the product reference; three programs per ISA run with output/status equal to system cc. The generated run.c image is the C runtime, not an all-assembly self-rebuild. Mac arm64 native and x86-64 Rosetta actually ran; Linux ELF spelling and Windows bindings are not claimed validated. Windows selection is explicitly rejected.
- Text: assembly 332/334 B (arm64/x86-64), remaining cc -Os C core 6,104/7,148 B, total 6,436/7,482 B. Each assembly object additionally has 58 B error strings; gzip-9 text 278 B each is only a reference. The original C-only baseline is 6,468/7,496 B. No significant optimization claim: actions and storage remain C, and whole-driver/libc/model costs still apply.
- Logs /tmp/unisacc-asm-arm.log and /tmp/unisacc-asm-x86.log. First host-cc comparison failed because hello omits stdio; the comparison now explicitly supplies stdio.h for host cc (product auto-includes it), and both complete scripts pass. The C-only external-linkage baseline was rechecked in /tmp/unisacc-asm-c-baseline.log. New exec-asm / exec-asmx86 jobs join the next gate. This slice used targeted checks; the previous **80/80** remains the preceding commit's full gate, not a freshly claimed 82-suite result.
- core.c only gains conditional implementation selection; default runtime, product source, shipped .com and model payload are unchanged. Full assembly action/storage migration, remaining frontend/CLI/diagnostics and final default switch remain unfinished. Local commit only; no push/release; cc-unisacc remains paused.

### S-17 continuation: handwritten fixed-width arithmetic (2026-09-27)

- The explicit assembly build now replaces both alu32 and alu64 as well as transition inference on arm64 and x86-64 System V. The original C helper bodies remain the default/reference and are excluded in this selected build; the C object has unresolved core_alu32/core_alu64/core_transition symbols, satisfied by assembly. No compiler rule or new machine primitive was introduced.
- Contract kept: low-32-bit operands with sign-extended results; modulo add/sub/mul; 5/6-bit-masked shift counts; 32-bit div/rem by zero return 0; 64-bit signed/unsigned division by zero returns 0 and sets the error flag; other 64-bit ops clear it. MIN/-1 returns MIN or zero, with explicit guards against x86 IDIV traps. Unknown operations tail-call the existing non-returning panic hook. These are execution-machine semantics, not C-language UB promises.
- Each ISA passes **108,919** value/flag comparisons against C compiled directly from core.c, including independent expected edges, plus 16 invalid-operation C/ASM invocations. The existing 537,620 inference comparisons remain green. Fresh network checks and four six-stage images per ISA equal the reference; three native program runs per ISA equal host cc. Actual execution macOS arm64 and Rosetta x86-64 only; no new Linux/Windows execution claim. Logs /tmp/unisacc-arith-{arm,x86}-route.log; C-only baseline rechecked in /tmp/unisacc-arith-c-baseline.log.
- Object __text: arithmetic 436/450 B (arm64/x86-64), transition 332/334 B, remaining cc -Os C 5,360/6,655 B; sums **6,128/7,439 B**. Arithmetic error strings 24 B each; gzip-9 arithmetic text 283/330 B for reference only. Generic storage and action dispatch are still C; this is not a full assembly core. Full runtime/OS/libc/model costs remain separately counted.
- Default runtime/product source/.com/model payload unchanged. Existing asm gate jobs now include arithmetic; no new gate category. This was a targeted recheck, not a fresh full gate (last full result remains the earlier 80/80). Next work remains storage/dispatch assembly plus frontend/CLI/diagnostic compatibility and final default switching. Local commit only, no push/release; cc-unisacc stays paused.


### S-17 continuation: assembly byte-buffer append (2026-09-27)

- The explicit development build now selects assembly append on arm64 and x86-64 System V, alongside inference/arithmetic. Bytes and their 64-bit attributes grow together via libc realloc; state layout is compile-time checked. Other storage and action dispatch remain C. Added the same pre-doubling signed-capacity overflow guard to C and assembly; default C behavior for valid sizes is unchanged.
- Each ISA passes 70,000 moving-allocation append checks, truncation/reset reuse, and six explicitly simulated C/ASM allocation/capacity failure checks. Existing 537,620 inference and 108,919 arithmetic comparisons, network checks, four six-stage images and three native comparisons remain green. C-only external linkage also passed. Logs /tmp/unisacc-buffer-{arm,x86}-route.log and /tmp/unisacc-buffer-c-baseline.log. Actual macOS arm64/Rosetta only.
- Buffer __text 208/170 B, strings 39 B each; remaining C 5,232/6,562 B. Mixed uncompressed text sums are **6,208/7,516 B**, slightly larger than the previous 6,128/7,439 B, so no size improvement claimed. Current C-only baseline including the new guard: 6,508/7,530 B. Host/libc/model costs remain outside these object sums.
- This was targeted verification, not a fresh full gate; preceding full gate remains 80/80. Existing ASM gate jobs include the new checks. Product sources, .com and model payload unchanged; local commit only, no push/release. cc-unisacc remains paused; full assembly core and product-route completion continue.


### S-17 continuation: assembly sparse indexed memory (2026-09-27)

- Migrated complete generic map lookup/insertion/growth/rehash to arm64 and x86-64 System V assembly. CoreMemory makes the former five globals one explicitly laid-out state; all fields are build-asserted. The hash, half-load growth, collision probing, overwrite and absent-zero behavior are unchanged. Stage cleanup/reset stays in C; calloc/free are explicit library dependencies. No compiler rule or action was added.
- Each ISA passed 140,000 keys through growth, zero overwrites, 64 engineered end-slot collisions, signed/extreme keys, absent reads and cleanup/reuse; backing arrays also equal the actual retained C implementation. Eight simulated fault invocations per ISA check three allocation failures and the 64-bit extent guard. The selected C object has unresolved core_memory_get/set, resolved by assembly.
- Fresh four-image six-network routes and three native program comparisons passed on Mac arm64 and Rosetta x86-64; inference/arithmetic/buffer checks remain green. First route failed because carried stdint has no SIZE_MAX; the guard now uses the explicit 64-bit storage bound and the rerun passed. C-only external linkage and ASan+UBSan network/resource/reset/failure tests passed; leak detection disabled for intentional fatal exits. Logs /tmp/unisacc-map-arm-route2.log, /tmp/unisacc-map-x86-route.log, /tmp/unisacc-map-c-baseline.log, /tmp/unisacc-map-sanitizer.log.
- Sparse memory __text 500/473 B plus 39 B strings each; remaining C 4,636/6,016 B. All assembly plus remaining C text sums **6,112/7,443 B**. C-only current baseline **6,480/7,548 B**. This is not yet an all-assembly kernel; blobs/interning, other state and action dispatch still remain in C. No new platform execution claim, product source/model/.com unchanged, no push/release; cc-unisacc remains paused.
- Frozen-tree `JOBS=2 tests/gate.sh --com`: **82/82**, failed 0, 430 s aggregate; each suite <=60 s. exec-native 52 s includes network-built runtime self-reconstruction, six full-domain checks and real write failures; ASM arm64/x86 jobs 8/10 s; fat 128/0 with both slices executed. Log /tmp/unisacc-map-full-gate.log. This supersedes the older 80-suite full-run record for this candidate. No code was edited during the run.


### S-17 continuation: binary string interning and prefix member updates (2026-09-27)

- d9b0332 extends E3 prefix ++/-- through the existing member/subscript address walk, then shared type/step/load/store logic. It was exposed by the new core's `(I)++t->n`; the initial complete route rejected it with expected-semicolon. No language-specific executor primitive was added and the source was not rewritten to evade the missing syntax. Constructor +9 net lines, states unchanged at 4,058; no code-reduction claim.
- Before growing keep-e3, all previous **237/237** passed in four disjoint <=60 s batches, 107,170,884 actions total. New prefix_members probe equals the reference and is now the 238th kept file. Its nested/narrow/unsigned/pointer members and side-effecting subscript produce independent exit **40**, actually checked on the five-image ARM64 and x86-64 routes against system cc. The two ASM route jobs now retain four old images plus this one. Evidence /tmp/unisacc-intern-debug/keep-{0,1,2,3}.log and /tmp/unisacc-intern-{arm,x86}-final.log.
- Handwritten intern/hash assembly on both ISAs preserves the existing nonstandard hash seed, binary lengths, copied ownership, insertion IDs, collision probing, growth-before-lookup and pointer-preserving rehash. CoreIntern's entry/state offsets are compile-asserted; the selected C object imports core_string_intern rather than a C fallback. Blob/resource/frame storage and dispatch remain C; allocation/copy/compare are explicit libc dependencies.
- Per ISA: 20,000 distinct strings, five fixed hash vectors, empty/NUL/prefix distinctions, 16 engineered wrapping collisions, growth on duplicate, reverse lookup, mutated input-buffer ownership and reset pass. Eight explicitly simulated C/ASM fault runs cover table/byte allocation, rehash allocation and 64-bit extent overflow. Fresh network/stream/error/package tests, five complete images and four actual native/Rosetta executions also pass.
- C-only external linkage and ASan+UBSan network/resource/reset/failure checks passed (leak detection disabled for fatal-exit cases). nativecheck passes six full-domain checks, 18 outputs, network-built runtime self-reconstruction, isolated packages and three real short writes. Logs /tmp/unisacc-intern-c-baseline.log, /tmp/unisacc-intern-sanitizer.log and /tmp/unisacc-intern-native.log.
- Intern/hash __text **508/479 B** (arm64/x86-64), strings 39 B each. Remaining C **4,144/5,533 B**; assembly plus remaining C text **6,128/7,439 B**. Current C-only baseline 6,528/7,641 B. This slice used targeted checks; last complete **82/82** gate belongs to de81ff3, not this new source/model state. Model schema/actions unchanged, E3's constructed model changes with its new path. Product sources and .com unchanged; no push/release; cc-unisacc remains paused. Full core and final product-route completion remain active.


### Assembly core: blob copies/resource ownership and required E3 addressing fix

- cc-unisacc remains paused. Product source and .com unchanged; no push/release.
- Explicit CoreBlobs/CoreResources state is shared by C and both assembly
  helpers. Blob IDs, cached absence, copied borrowed buffers and exactly-once
  freeing of host-owned buffers are preserved. Cleanup still belongs to C.
- Both macOS arm64 and Rosetta x86-64 pass 300 moving-allocation blocks,
  256 resource keys, cache/reset checks and ten simulated failure invocations.
  Resource count INT32_MAX guard is not exercised. Network tests, six full
  images per ISA and five native behavior comparisons per ISA pass.
- The new C source exposed E3's early return on &s.pointer[index]. The model
  now loads the pointer before resolving the final element address. Old 238
  fixed files passed in four bounded batches before adding the independently
  checked member_index_address probe (exit 25), making 239. No executor
  language-specific primitive was introduced.
- Blob/resource __text is 548/513 B (arm64/x86-64), strings 70 B each;
  remaining C 3,848/5,153 B; mixed text sums 6,380/7,572 B. C-only baseline
  6,664/7,707 B. This is migration, not size reduction. Host/libc/model costs
  remain separate. Native C network self-reconstruction and ASan/UBSan
  network checks pass. Logs: /tmp/unisacc-bytes-{arm-final,x86-final,native,core,sanitize}.log
  and /tmp/unisacc-bytes-keep-{0,1,2,3}.log.
- Only targeted checks were run for this slice; the last full 82/82 gate
  remains de81ff3. Complete assembly dispatch, platform bindings and the final
  product switch remain unfinished.


### Assembly core: signed decimal and reserved output fields

- Both ISA helpers now own signed decimal conversion and OFILL byte writes.
  They retain no-terminator output, INT64_MIN, right alignment, untouched
  attributes and field-overflow precedence. Bounds are checked by subtraction
  after the offset is validated, avoiding signed overflow in at + width.
- Each actual macOS arm64/Rosetta run passes 10,013 numbers and 250,325 fields
  against independent snprintf output, plus ten C/ASM bounds failures. Existing
  network checks and six images/five program executions stay equal. Native C
  network self-reconstruction and ASan/UBSan pass. Logs:
  /tmp/unisacc-format-{arm,x86,native,sanitize}.log.
- Format text 272/225 B, strings 26 B per ISA; remaining C 3,504/4,612 B;
  combined text 6,308/7,256 B. C-only 6,664/7,715 B. Allocation/host/model
  bytes remain outside these object sums. No speed claim. Dispatch, frames,
  initialization and cleanup are still C, so full assembly migration remains
  incomplete. Product unchanged, no push. Targeted checks only; last full gate
  still de81ff3 82/82.


### Assembly core: control symbols and input-frame stacks

- The two stacks now have explicit ABI-checked state and real assembly push/pop
  implementations. Symbols retain 32-bit storage; frame records retain borrowed
  byte/attribute pointers and 64-bit cursor/end. The bottom input frame is never
  popped. Signed capacity overflow is rejected before doubling; no new model
  limit. Initialization, action dispatch and final cleanup are still C.
- Per actual macOS arm64/Rosetta ISA: 20,000 symbols and frames checked through
  moving allocation, all values, reverse pops and reuse; fourteen simulated
  allocation/overflow/empty-pop failures; existing six images/five native
  comparisons pass. Network-built C self-reconstruction and ASan/UBSan pass.
  Logs /tmp/unisacc-stack-{arm,x86,native,sanitize}.log.
- Stack text 288/275 B, strings 84 B each. Remaining C 3,240/4,215 B;
  full assembly-helper-plus-C text 6,332/7,134 B. C-only baseline 6,532/7,758 B.
  These object sums exclude linked host/libc/model data, and do not mean the
  full assembly kernel or product-route switch is complete. Product unchanged;
  no push. Last complete gate remains de81ff3 82/82; this slice uses targeted
  checks and actual self-reconstruction.


### Assembly core: explicit machine state and all ARM action handlers

- All mutable action state is now one invocation-local CoreMachine (280 B,
  layout asserted), replacing C globals. C reference network checks and actual
  C network self-reconstruction passed before enabling the assembly dispatcher.
- arm64 now selects hand-written dispatch plus all 56 action implementations,
  calling only the migrated assembly primitives and generic host/libc entries.
  Its build rejects a mixed C primitive fallback. x86-64 still selects the C
  action engine explicitly; its existing assembly helpers continue to pass.
- The action test compares 1,050 full states across all 56 actions and two
  bad-action failures; includes both output selections, absent attributes,
  clipped/reversed spans, signed/unsigned values, cache reuse and repeated SWAP.
  A missing address-add caused by an assembly semicolon comment was caught and
  fixed. SWAP allocator failures are not injected by this suite.
- Both ISA jobs retain six image/five native comparisons. ASan/UBSan and C
  network self-reconstruction pass. Logs /tmp/unisacc-action-{context,
  context-native,arm-route,x86-route,sanitize}.log. Targeted checks only.
- ARM action __text 1,968 B, strings 41 B. Remaining C 980 B (outer inference
  loop, initialization, cleanup), full mixed text 6,040 B. x86 remaining C
  3,855 B, mixed text 6,774 B. C-only baseline 6,396/7,106 B, static C state
  zero. Host/libc/model bytes still excluded from object sums. No product
  switch or all-assembly self-rebuild claimed. No push/release.


### Frozen gate and x86-64 action dispatch

- Frozen 8743d4f: JOBS=2 gate --com **82/82**, 428 s total, all per-suite
  bounds <=60 s. exec-native 56 s, exec-asm 10 s, exec-asmx86 12 s; fat
  executed both slices, 128/0. Log /tmp/unisacc-action-full-gate.log. No edits
  overlapped that run. ARM action state comparisons additionally pass ASan/
  UBSan; this macOS sanitizer does not support leak detection, which was off.
- After that gate, x86-64 assembly dispatch was integrated. It passes the
  same 1,050 full-state comparisons over all 56 actions, bad-action rejection,
  all primitive checks, network checks and six-image/five-native route under
  Rosetta. Log /tmp/unisacc-action-x86-final.log. This is targeted evidence
  for the later x86 change, not a new full-gate result.
- x86 action text 2,116 B, diagnostic strings 41 B. Remaining C 1,135 B;
  total mixed text 6,170 B (ARM remains 980 B C / 6,040 B mixed). Both selected
  action engines now execute all actions in assembly; outer loop, initialization
  and cleanup remain C. Host/libc/model costs are still separate. Product
  .com unchanged, no push/release, cc-unisacc remains paused.


### Complete assembly execution lifecycle

- Both ISA adapters now use assembly for the run loop, initialization and
  ownership transfer/cleanup as well as all actions/primitives; core.c is no
  longer linked by asm/cc.sh. Shared action arities remain checked against OPS.
- Lifecycle differential checks: 62 exits with an allocation ledger, plus four
  simulated initial-allocation failures. Both macOS ISA jobs retain six image
  comparisons, five native runs and the generated C runtime comparison. Native
  C network self-rebuild and C ASan/UBSan pass. Logs:
  /tmp/unisacc-lifecycle-{arm,x86,native,sanitize}.log. No edits overlapped tests.
- Uncompressed assembly __text totals 5,976/6,195 B (arm64/x86-64), including
  lifecycle 916/1,160 B. Current C-only -Os baseline 6,436/7,146 B. Host, libc,
  data/strings, loader and model costs remain outside these sums.
- Targeted verification only; the latest full 82/82 gate remains 8743d4f.
  Product .com unchanged. Actual product ABI/OS bindings and the default
  switch remain unfinished; no assembly self-rebuild is claimed. Local work
  continues; cc-unisacc remains paused, no push/release.


### Assembly kernel through the compiler CLI

- Existing compiler, multi-unit and memory suites now require a third driver,
  linked against the full assembly kernel (no core.c). Native macOS checks:
  27 mode/level matches, 36 multi-unit tape comparisons, isolated package/file
  compilation and memory execution, macro/include handling, failure-before-
  output and real short writes. Memory suite: 36 runs per ISA across all three
  drivers, plus argv/env/O1/exit-status checks, on arm64 and Rosetta x86-64.
- Logs /tmp/unisacc-asm-{driver,multi,memory-arm,memory-x86}.log; all four suites
  exited 0, each bounded by 60 s. These are new required paths in existing
  gates, not optional tests. The package remains unchanged (host route:
  1,092,744 B arm64, 1,072,876 B x86-64). No fresh full-gate claim.
- The host-linked assembly driver is not the shipped .com. Product-internal
  ABI/carried-library binding remains necessary; x7 is the ARM tape stack and
  the first x86 tape argument is rax rather than System V rdi. Product sources
  and .com unchanged. Local commit only, cc-unisacc paused, no push/release.


### Product-ABI assembly binding and network driver self-rebuild

- Added a small tape-ABI/assembly bridge on both ISAs and nine generic carried-
  library/host services. ARM separates hardware stack use from the live x7
  tape stack; x86 aligns rsp and preserves the tape r9 frame. The entry takes
  a tagged argument record, avoiding the reference's six-register indirect-
  call limit. No language-specific action was added, no C core fallback.
- Offline macOS assembly/static link yields a self-contained, position-
  independent blob with no unresolved imports or rebasing/binding. One service
  pointer is filled before RX protection. Complete blob: **7,664 B each ISA**,
  including envelope, header/padding, strings/constants, core and bridge.
  UNISA_KERNEL is still an explicit development input, not embedded yet.
- Fresh native arm64/Rosetta x86 binding checks both pass: network/domain/
  resource/error tests; four complete images; nine CLI modes; nine memory runs;
  two multi-unit runs; missing/corrupt blob rejection; **N1=N2=N3** for the
  assembly-bound compiler driver, rebuilt through its networks using the same
  explicit fixed assembly blob. Logs /tmp/unisacc-binding-self-{arm2,x862}.log.
- Self-rebuild exposed E3's int-only function-pointer cast guard. Added signed
  long casts and made indirect results the reference's full machine word.
  New long_fptr probe equals the reference tape and executes with exit 0 under
  host cc and product. Old fixed 239 all pass in eight bounded Python-oracle
  shards before adding the probe (keep now 240). Generator +1 net line; this
  matches the measured word-result convention, not complete function types.
- Binding suites are required macOS gate entries. Linux/Windows binding,
  single-package integration and default switching remain unfinished. Product
  source/.com unchanged, cc-unisacc paused, local commits only; no push/release.

### Assembly core carried with the compiler models

- Frozen 8c3ab88 full `JOBS=2 tests/gate.sh --com`: **84/84**, all exit 0,
  467 s aggregate; each suite bounded at 60 s. Evidence:
  `/tmp/unisacc-binding-full-gate.log`. This covers that commit, not later edits.
- New development `exec/c/buildcompiler.sh OUTPUT_DIR` builds one container
  using `asmcompiler.c`, six model routes, headers and both fixed ISA core
  resources. Kernel lookup uses the package first; an external path is retained
  only for standalone tests. No C core fallback. No product/CLI source parsing
  was moved into the assembly bridge.
- Local targeted checks: carried ARM binding N1=N2=N3; isolated container with
  loose model/kernel files removed, all six hello images equal the reference,
  native/memory execution on macOS arm64 and Rosetta x86-64. Package has 23
  unique models, two cores and one container payload. Logs:
  `/tmp/unisacc-carried-bindarm.log`, `/tmp/unisacc-carried-container.log`.
- Actual Windows ARM execution caught the bridge's POSIX shared-stack assumption.
  Windows now keeps AAPCS/WinAPI on the hardware stack and tape callbacks on x7;
  x28 carries that callback stack and is reserved (the run loop's sequence count
  moved into its frame). The first fix's register conflict failed macOS and was
  corrected before acceptance. Full ARM helper/action/lifecycle checks and
  carried binding N1=N2=N3 pass again. ARM blob now 7,704 B, x86 remains 7,664 B.
- Windows then rejected the embedded .com before entry: PE offset 781 was
  unaligned. APE construction now aligns the PE signature at an 8-byte boundary
  (784 here), guarded in the constructor and embedded/container checks. The
  actual full container subsequently passed five x86-64-emulated Windows runs
  (hello/pointer at O0/O2 and argv at O1, including exit 7). ARM native passed
  those five with a carried-core package, then file/malloc/missing-input runs
  with one native image carrying its own package. This VM was stopped afterward.
  Logs `/tmp/unisacc-carried-winarm3.log`, `-wincom2.log`, `-winarmio.log`.
- Actual Lima Linux arm64: four programs at O0/O1/O2 in memory and as native
  images, six-target cross-output comparison, driver N1=N2=N3; no external
  kernel. `containerlinux.sh BUILD_DIR` preserves the opt-in reproduction.
  Linux driver bare-image SHA-256:
  `0eba58a2e360df6bd10a328351cd2e80910a0a931e41f4eda5f65c79abb155d0`.
- Development container: **3,064,442 B**, SHA-256
  `e31284381653f87e208ec963fc4459e1a3fd2724d08746e9b3fee2da370315d8`.
  Package 2,892,922 B = 2,749,364 B network/action/string models + 103,636 B
  carried headers + 15,368 B two kernel blobs + 24,554 B directory/framing.
  Other executable/launcher/compressed-slice bytes 171,504; footer 16 B. These
  are different byte categories, not an assertion that the whole compiler is
  a few KB. Final isolated macOS checks pass in `-container-final.log`.
- Still a development route: default switch, complete CLI/source parity,
  final same-input speed comparison and remaining platform scope are not
  complete. No Linux x86-64 or Windows x86-64 hardware claim from this batch.
  The reference .com will be rebuilt for the APE fix without changing its
  handwritten compiler path. No push/release; cc-unisacc remains paused.

- Acceptance at **95d4f17**: full `JOBS=2 tests/gate.sh --com` **85/85**,
  exit 0, 465 s aggregate; all suites individually <=60 s. New container suite
  18 s, ARM/x86 binding suites 15/18 s. Log `/tmp/unisacc-carried-full-gate.log`.
  Final aligned container also passed the Linux reproduction, with the same
  e3128438... hash as the actual Windows container; log `-linux-aligned.log`.
  Rebuilt reference product remains 1,349,776 B, SHA-256
  `ee67c33f323ddb4bafa4d1bc1925fe7a04aa009c906ecb86e78610b0fe5fa055`.
  Follow-up builder chmod and documentation updates do not change image bytes;
  their targeted check is recorded separately. No default-route switch.

- Post-gate targeted follow-up: builder now marks its container executable;
  `containercheck.sh` asserts that and passes (`/tmp/unisacc-carried-executable.log`);
  docs check passes. `tests/cli.sh`'s six legacy 120 s child limits were reduced
  to 60 s. Reference CLI remains **64/64**. Running the same suite directly on
  the assembly/model container gives **53 pass / 11 fail**, not product parity:
  math-header coverage, ignored build flags, -l/-L/-x, -nostdinc's two cases,
  dependency target/header output, undefined-function wording, --version,
  -dump-tokens, missing-file wording. Logs `/tmp/unisacc-carried-cli-{reference,next}.log`.
  This is the concrete next acceptance list; the 85-suite green result does
  not mean this new container passed the old product's complete CLI suite.


### Model CLI follow-up: sqrt builtins (local acceptance)

- The carried math-header rejection was not evidence of a forward-declaration
  defect: E3 lacked `__builtin_sqrt` and `__builtin_sqrtf`. Their parser-delta
  paths now select the product's signed/unsigned/double/float conversions and
  square-root operation. FPU spellings are read from irsel; f32 access width
  comes from tyinfo. No executor operation or host source parser was added.
- Before extending keep-e3, all old **240/240** accepting C-executor tapes were
  byte-identical to the stamped reference. The new `builtin_sqrt.c` also agrees
  on the Python and C executors; keep is now 241. It covers nested builtins,
  float loads, signed integers, u64, and both result precisions. Host cc and the
  rebuilt assembly/model container at O0/O1/O2, in-memory and native execution,
  all return the independently expected 3 on macOS arm64.
- Logs: `/tmp/unisacc-sqrt-keep.log`, `/tmp/unisacc-sqrt-cli.log`; scratch
  reproduction scripts `/tmp/unisacc-sqrt-{keep,run}.py`. The candidate is
  `/tmp/unisacc-sqrt-candidate/unisacc-next.com`, not the shipped product.
- CLI remains **53/64**: full math.h now reaches `isnan`'s double `!=`, which
  E3 still refuses. This batch is a local capability repair, not closure of
  the complete header test. Other ten CLI failures are unchanged. No full
  gate or cross-platform rerun is claimed for this batch. cc-unisacc remains
  paused; no push or release.


### Model math-header closure and exact decimal constants (local batch)

- E3 now handles double `!=` as inverted floating equality, including NaN.
  Decimal constants use a delta implementation of exact multiword arithmetic,
  normalisation and nearest/even rounding, for binary32/64, exponents and
  subnormals. No float-parser executor primitive or host-source parsing was
  added. The old small-decimal FCONV generator is no longer called by E3.
  Hexadecimal floating constants remain outside this slice. A 160-limb bound
  is checked before appending; oversized significands reject rather than wrap.
- Casts, sqrt builtins and typed floating arguments share conversion procedures;
  instruction spellings come from irsel. Scalar struct/union member size now
  uses ELSZ rather than treating encoded float types as byte sizes. These two
  latent errors were exposed by whole-tape comparison, after the simple CLI
  example already ran correctly.
- New `floatconstcheck.sh` compares 34 host-cc bit expectations with both table
  and network execution, including half-way cases, f32 double-rounding-sensitive
  values, overflow, subnormals and a long significand cancelling its exponent.
  Five invalid/capacity inputs reject on both routes. It is in the gate.
  Decimal/NaN and full math-header probes are also exercised by the real ASM
  network driver suite, against independently executed host-cc programs.
- Old keep-e3 241/241 stayed byte-identical before adding the two new probes
  (243 now). Both new inputs match the stamped reference on Python and C action
  executors. Actual carried-container runs at O0/O1/O2, memory and native, return
  the expected zero on both; `/tmp/unisacc-float-{keep2,oracles,run}.log`.
  CLI improved to **54/64**, including the complete carried-header case;
  `/tmp/unisacc-float-cli.log`. The other ten failures remain open.
- The long-decimal oracle found a product bug too: `10^1000 * 10^-1000` was
  discarded solely because its decimal exponent was below -800. Host cc exits
  0, the pre-fix reference 1. Product and delta now prove underflow from both
  significand bit length and exponent. `tests/c/b_longdecimal.c` covers double
  and float; generated unisacc.c and the reference .com were rebuilt.
- This batch adds a conversion algorithm to generated model logic; it is not
  a claim of fewer source lines or complete C99 floating support. No default
  route switch, push or release. Full frozen-tree acceptance follows below.
- Frozen-tree local gate: 86 suites, 85 passed and exec-driver failed; the
  failure was the new test's relative models.pkg lookup from the wrong cwd,
  not a semantic mismatch. After the run finished, the harness was corrected
  to run those calls in its isolated package directory. The independently
  rerun exec-driver passed. This records the original red run, not an invented
  86/86 rerun (`/tmp/unisacc-float-full-gate.log`,
  `/tmp/unisacc-float-driver-fixed.log`).
- Final development container: 3,076,544 B, SHA256
  `22c0e8bbca6d9ed3bb2e6d4e06597c3509c2169614a2b023819589ce4d69c377`.
  Both macOS arm64 and Rosetta x86_64 actually executed the decimal, math-header
  and long-decimal probes at O0/O1/O2, in memory and as native images, all as
  expected (`/tmp/unisacc-float-finalrun.log`). Its final CLI result is 54 pass,
  10 fail (`/tmp/unisacc-float-cli-final.log`); no CLI parity is claimed.
- Rebuilt reference product .com: 1,349,952 B, SHA256
  `3f914d98624f82122d5841611bcd4de384746761b7db837a725f0c829a14cd07`.
  This remains the default product route. No Linux or Windows execution of
  this batch's final artifacts was performed; cross-output checks do not
  establish that. cc-unisacc remains paused. No push or release.

### Model driver: source IO diagnostics

- Source reads now report `unisacc: error: cannot open PATH`, exit 1, while
  runtime package/resource failures retain their separate diagnostics. Both
  the first input and a later translation unit are checked; neither truncates
  an existing output. No source parsing moved into the driver.
- Fresh compilercheck passes (host cc, unisacc, ASM driver and network-built
  driver integration); log `/tmp/unisacc-source-io.log`. Rebuilt development
  container executed both missing-source cases on macOS arm64 and Rosetta
  x86_64, with exact stderr/rc and preserved output. Size 3,076,656 B, SHA256
  `83d5322d30fba24ffd2249e35a7d653f396e1119570e3023d6ad0daea0af9b14`.
- Targeted verification only; no new full-gate or full-CLI result claimed.
  Default reference .com unchanged. No push/release; cc-unisacc stays paused.

### Model driver: shared version query

- `src/version.h` is the single product version declaration. build_ref embeds
  it into the standalone unisacc.c; the model driver includes it directly.
  ua_ready hashes it too, so a version-only edit cannot reuse a stale reference.
- `--version` and `-version` return before package/source loading. Fresh
  compilercheck passes, checking cc/unisacc/ASM builds from outside the repo
  without a package. Rebuilt container also executes both queries on macOS
  arm64 and Rosetta x86_64: exact `unisacc 0.0.7`, exit 0, no stderr.
- Development container: 3,076,896 B, SHA256
  `e9214bae33045acb0c5dbc6438de4f7b3383885f3af9aca1184428b65b592f03`.
  Reference .com rebuilt and unchanged byte-for-byte (SHA256
  `3f914d98624f82122d5841611bcd4de384746761b7db837a725f0c829a14cd07`).
  docs: 3 generated tables, stale 0, referee ledger ok. Logs are
  `/tmp/unisacc-version-{check,build,com,docs}.log`. Targeted checks only;
  no new full gate, platform-wide validation, default switch, push or release.

### Model CLI: compatibility arguments and -nostdinc preprocessing

- Driver consumes attached/separate -l/-L/-x arguments, matching the product's
  no-linker, C-only compatibility behaviour. Missing arguments fail explicitly.
  -g and -std= are accepted with the same no-debug/no-dialect-switch behaviour;
  warning options are not silently treated as implemented.
- -nostdinc is a raw resource. E2, not host C, disables built-in include search
  and automatic headers while preserving source-relative and explicit -I
  lookup. CLI resources were expanded by one slot; process/memory/import
  slots moved together. No executor operation was added.
- Fresh compilercheck passes the implemented contracts, including Python
  action-oracle vs network preprocessing. Its printed KNOWN line explicitly
  records that undeclared printf's runtime fallback remains unfinished.
  The complete CLI suite remains red: **58 pass / 6 fail**. Remaining: warning
  flags, -nostdinc printf conversion/lowering, dependency target/header output,
  undefined-function diagnostic, token dump. Logs:
  `/tmp/unisacc-nostd-finalcheck.log`, `/tmp/unisacc-compat-final-cli.log`.
- An attempted fallback expansion was withdrawn after tape comparison exposed
  literal-pool order (argument strings must precede format fragments). Another
  concrete blocker is DO.print's explicit rejection in exec/lower/code.py:
  faithful migration requires the itoa TIns encoder on both ISAs. Scratch
  all-conversion probe and tapes remain under `/tmp/unisacc-nostd-*`; none of
  that incorrect parser trial remains in the tree. This is not printf closure.
- Rebuilt development container: 3,080,406 B, SHA256
  `714b4b41a45cd10f71b8489f7c761c09eb9b43827f4aeebd8ccbbadbf40a7725`.
  Both macOS arm64 and Rosetta x86_64 executed attached/separate compatibility
  options plus -nostdinc with a user header; built-in stdio rejection checked
  on both. No full gate or Linux/Windows execution claimed for this batch.
  Default product unchanged; no push/release; cc-unisacc remains paused.

### Model printf continuation: .print / itoa through both encoders

- Lowering now handles .print with the reference's scratch spill, itoa, and
  write ABI sequence. Print-buffer/address and length arguments are resolved
  by the delta. Both ISA encoders emit the signed decimal count/write loops;
  x86 RIP operands wait for final relaxation, ARM ADRP uses the final layout.
  No formatting or encoder primitive was added to the generic executor.
- Integer conversion selection/control is explicit generated code and emitted
  machine-code templates, not a newly discovered gold-table fact. This adds
  source and model bytes; no code-size reduction is claimed.
- ARM encoding: three OS layouts, page-crossing fixtures and both executors
  agree with reference bytes. x86: 53 fixtures pass, including a branch before
  itoa, and rejects for a register address, missing operand, lone minus and a
  too-wide decimal address. The new x86 address parser bounds each digit before
  wraparound; existing operand paths retain their previous semantics.
- Typed lowering suites for all six targets pass, with .print added to their
  shared fixture; both executors compare the complete typed fields. Fresh
  compilercheck passes decimal fallback tape equality and actual output for
  0, 1, -1, 10, INT64_MAX and INT64_MIN. Logs:
  `/tmp/unisacc-{arm-itoa,x86-itoa-final,print-driver,print-lower-x86,print-lower-arm,print-lower-win}.log`.
- Rebuilt development container: 3,193,943 B, SHA256
  `8693dad1ce748ed72346b3f18b81cf3dbd50a4a69a3e89dd57c37cb13ce329b7`.
  Actual macOS arm64 and Rosetta x86_64 execution at O0/O1/O2, memory and native
  output, prints `0/1/-1/10/9223372036854775807/-9223372036854775808` correctly.
  No new Linux/Windows native execution or full-gate result is claimed.
- Full undeclared printf fallback remains unfinished: pfconv conversion
  dispatch and argument-string-before-format-fragment pool order still need
  migration. The old 58/64 CLI result is not relabelled green by this slice.
  Default product unchanged, no push/release, cc-unisacc stays paused.

### Model printf fallback: conversion dispatch and nested arguments

- E3 now reads pfconv for d/i/u/x/X/o/p/c/s and emits the reference fallback's
  explicit tape helpers. Declared printf remains an ordinary function call.
  This preserves the fallback's zero return and ignored width/precision;
  it is not a claim of full libc printf semantics (notably fallback %p).
- Format strings are decoded before scanning, including escaped percent and
  adjacent literals. Argument literals are pooled before format fragments;
  nested calls record their actual frame slots instead of assuming contiguous
  slots. No executor primitive was added. Source grows; no reduction claimed.
- Final generated E3 preserves the fixed 243 accepting tapes byte for byte.
  Fresh compilercheck passes: all nine conversion letters, nested calls,
  literal-pool order, escaped/adjacent formats, exact tape and actual output
  on cc/unisacc/assembly drivers. Permanent probe:
  exec/parse2/probes/printf_fallback.c. Logs:
  /tmp/unisacc-pf-keep-final.log and /tmp/unisacc-pf-driver.log.
- Fresh development container: 3,211,436 B, SHA256
  49cfc5183488fe36a95fda2466e4fa50cc3ccdb223fd290eb36564cfa778c4f2.
  Actual macOS arm64 and Rosetta x86_64, O0/O1/O2, memory and native output:
  all pass the permanent probe. /tmp/unisacc-pf-native.log.
- Full candidate CLI suite: 59/64, exit 1. Remaining: warning options,
  dependency target/header output, undefined-function diagnostic, token dump.
  /tmp/unisacc-pf-cli.log. No full-gate or Linux/Windows execution claimed.
  Default product unchanged; no push/release; FX conjectures remain unscheduled.

### Model compiler token-dump route

- -dump-tokens selects a package route containing E2 and the existing plain
  E1 network. Runtime C does not decode tokens. The route uses Linux/x86_64
  predefines, matching the reference token instrument's fixed target; normal
  compilation keeps typed E1 output and its selected target. Multiple-source
  token dumps remain explicitly rejected.
- Fresh compilercheck: cc, unisacc and assembly drivers match full reference
  output for declarations, macro expansion, string/hex literals and predefines;
  existing driver/printf tests also pass. /tmp/unisacc-token-driver.log.
- Rebuilt container: 3,254,497 B, SHA256
  14eec9e7764e1c59ed80ef4ab4aa15ba77308545a54e043ad209e755844f7cb6.
  Embedded route executed on macOS arm64 and Rosetta x86_64 and matched the
  reference; /tmp/unisacc-token-native.log. No Linux/Windows run claimed.
- Candidate CLI: 60/64, exit 1. Remaining: warning options, dependency target
  and header output, undefined-function diagnostic. /tmp/unisacc-token-cli.log.
  No full gate claimed. Default product unchanged; local commit, no push.

### Model compiler dependency-file output

- -MD/-MMD/-MF now write make-style input/header prerequisites after successful
  compilation. The generic host adapter optionally records successful disk
  resource reads; the model still decides which include is active and which
  path to request. Carried/process resources and failed reads are excluded.
  No new executor action or C-side include parser was introduced.
- Duplicate paths are canonicalised (including reads shared across units),
  unlike the reference's repeated include entries; the prerequisite set is
  preserved. Formatting/default names follow the reference. Like the existing
  product, no additional make escaping for whitespace in filenames is claimed.
- Fresh compilercheck passes on cc/unisacc/assembly drivers: nested headers,
  inactive missing header, repeated include, multiple inputs, carried stdio,
  explicit/default dependency paths, missing -MF argument and dependency IO
  failure preserving the existing compiled output. All prior driver checks
  pass as well. /tmp/unisacc-deps-driver-final.log. The first trial used a
  Linux-default -S route absent from the single-host test package; the test
  now explicitly selects its packaged target before -S.
- Rebuilt development container: 3,260,145 B, SHA256
  5752a0e8f840175db0219647efa0eb13d2546a87a39c036861154d27d02baaba.
  Actual macOS arm64 and Rosetta x86_64 dependency output matches reference;
  /tmp/unisacc-deps-native.log. Candidate CLI is 62/64, exit 1: warning options
  and undefined-function diagnostic remain. /tmp/unisacc-deps-cli.log.
- Default compiler unchanged; no full-gate or Linux/Windows native result
  claimed for this batch. No push/release. Reconstruction remains incomplete.

### Model compiler deferred undefined-function diagnostics

- E3 no longer rejects an ordinary call merely because the definition has not
  been read yet. Unknown calls initially use the reference's scalar result
  descriptor. After all input units and helper generation, unresolved.py scans
  the actual tape twice: collect labels, then check calls in emission order.
  This includes nested-call ordering and reports each missing name once.
  Prototypes alone do not satisfy the check. This migrates the reference's
  behaviour, including its acceptance of an undeclared call defined later;
  it is not a new C99 conformance claim for implicit declarations.
- Diagnostics and singular/plural error counts are model-produced bytes on
  the existing error stream. Empty rejection reasons no longer cause the host
  to prepend a spurious 'reject:' line. No language-specific runtime primitive
  or C-side tape scan was added. The byte scans add generated rules/work;
  no source-size or speed improvement claimed.
- Fixed 243 accepting tapes remain byte-identical. E3 self-source produces
  3,929,446 bytes equal to the reference under the 60 s watchdog. Fresh
  compilercheck passes on cc/unisacc/assembly drivers: unknown names,
  declaration without definition, duplicate calls, nested calls, later
  same-unit and later-unit definitions, and fail-before-output protection.
  Logs: /tmp/unisacc-ud-{keep,self,driver}.log.
- Rebuilt container: 3,262,731 B, SHA256
  6f79bf96c2b6e91a4d80c48dbb2897ed0545c69a161f185b83bf4bc507e0863b.
  macOS arm64 and Rosetta x86_64 execute the later-definition program with
  expected exit 7 and produce exact nested missing-function diagnostics.
  /tmp/unisacc-ud-native.log. CLI 63/64, exit 1; -Wall remains unmigrated.
  /tmp/unisacc-ud-cli.log. No full gate or new Linux/Windows execution claimed.
- Default product unchanged. Local commit only, no push/release; full
  reconstruction and the final product switch are still incomplete.

### Warning migration prerequisite: successful diagnostic streams

- Audited -Wall: the reference implements unused-variable, return-type,
  printf-format and integer-to-pointer diagnostics; -Wextra shares the switch,
  while -Werror prevents output/run when warnings exist. These cannot be
  replaced by silently accepting a flag. Model CLI still rejects them.
- Fixed a generic host-adapter defect: execute() discarded the model error
  byte stream on successful acceptance. It now forwards a nonempty diagnostic
  stream independently of acceptance. Python's action oracle also preserves
  successful diagnostics via an optional collector (existing return shape
  retained) and its CLI; empty rejection reasons do not prepend a fake line.
  No executor action or compiler-specific primitive added.
- netcheck adds exact status/stdout/stderr evidence for successful diagnostics,
  two-stage order, and a rejecting later stage (no partial stdout or later
  model execution). Same actions pass in the Python oracle. The fixture
  serializer now uses the production '-' spelling for an empty string.
- Passed netcheck with host cc, unisacc, arm64 assembly and Rosetta x86_64
  assembly runtimes. Fresh compilercheck remains green. Logs:
  /tmp/unisacc-warning-channel{,-ua,-arm,-x86}.log and
  /tmp/unisacc-warning-driver.log.
- Rebuilt development container: 3,262,731 B, SHA256
  1effcec46db3a186a378e569e32a1aa6f4fe10149fb68b681b5c162d9b27e7c3.
  Exact missing-function diagnostics and later-definition execution pass on
  both macOS ISAs (/tmp/unisacc-warning-native.log). This is a transport fix,
  not completion of warning analysis; previous CLI 63/64 is not raised.
- Remaining warning work includes original-file positions, header suppression,
  continuation/include adjustments and the four analysis rules. E2 currently
  retains splice/include line records internally but does not export them to
  E1/E3; token streams do not yet carry source positions. Do not infer source
  locations by searching token text. Default product unchanged; no push,
  release, full-gate claim or new Linux/Windows run in this batch.

### Warning source-location prerequisite: E2 diagnostic envelope

- Optional offline `exec/pp/gen.py --locations` now constructs an E2 network
  that emits pp.locations: the exact preprocessed text plus forced/automatic
  prefix counts, splice offsets, and ordered header insertion regions/names.
  The binary envelope is specified in exec/pipeline/formats.md. All framing
  is generated model actions; no host parser or new executor primitive.
- Header names follow the reference diagnostic map's first-62-byte limit.
  A forced-include test in the long macOS scratch path exposed this; the
  diagnostic-only model now reproduces it explicitly. Actual file lookup
  still uses the complete path.
- locationcheck.py builds a scratch-only instrumented current reference at
  the real expandsrc/lex boundary. Nine complete records+text are byte-equal:
  plain, macro, LF/CRLF splice, multiline comment, nested/repeated includes,
  forced include and automatic stdio. Five small cases additionally agree
  with the Python action oracle. cc, unisacc and both macOS ISA assembly
  runtimes pass all nine. Logs /tmp/unisacc-location-{check,ua,arm,x86}.log.
- Default E2 states and action sequences compare exactly with the pre-change
  generator for all six targets (/tmp/unisacc-location-default.log). No
  default container rebuild is needed for this unused optional mode; the
  existing container and default product are unchanged. exec-pploc joins the
  bounded local gate; no full-gate or Linux/Windows native claim in this batch.
- This exports the actual position-recovery inputs only. E1/E3 do not consume
  the envelope yet, the compiler route does not select it, and warning rules
  are not migrated. CLI remains 63/64, not a completed -Wall implementation.
  Next is model consumption/token positions and warning analysis, retaining
  header suppression and the reference's macro-expanded-column convention.
  Local commit only; no push/release. Reconstruction remains active.

### E1 token positions — optional, not yet a warning route

- `exec/lex/gen.py --positions` emits typed tokens with fixed-size binary
  prefixes holding the saved token-start offset in the preprocessed buffer;
  EOF carries the buffer length. Generic model actions perform the output;
  no token/position logic was added to the C or assembly executor.
- `exec/lex/positioncheck.py` builds an instrumented reference in scratch,
  captures the actual lexer input and tpos values, and compares nine cases:
  empty, ordinary, macro, splice, multiline comment, string/char prefixes,
  numeric/operator lookahead, skipped attributes, and offsets above 255.
  All pass on cc and unisacc C builds and both macOS assembly runtimes
  (x86_64 through Rosetta), plus the Python action oracle. Logs:
  `/tmp/unisacc-lex-position{,-ua,-arm,-x86}.log`.
- Removing the six-byte prefix before each token yields exactly ordinary
  typed output. Default and --typed states/action sequences are unchanged
  from 84b18d5. exec-lexpos joins the bounded gate; no full-gate or other
  platform claim in this batch. No product/container rebuild or switch.
- Remaining: connect pp.locations and token offsets to E3, then implement
  warning decisions and rendering. This does not make -Wall available;
  CLI remains last measured 63/64. FX-3 remains an unscheduled conjecture.

### Located E2/E1/E3 route — optional diagnostic input plumbing

- E1 --locations now consumes pp.locations, validates its framing, preserves
  the complete map and emits positioned typed tokens in tokens.locations.
  E3 --locations reads that envelope and retains source text, splice records,
  forced/automatic prefix counts and include-region/name records. Each NEXT
  records source_pos and TOKEN_POS keyed by the original token-buffer position;
  parser rewinds and bounded string/initializer views still use that buffer.
  All operations are model actions; no language-specific host primitive.
- Six E2/E1 joined cases match direct positioned lexing and the Python action
  oracle; eleven bad preprocessing frames reject without output. Eight joined
  E2/E1/E3 cases produce unchanged tape: small, macro, splice, explicit header,
  adjacent strings/array initializer, automatic printf header, fib, strderef.
  A test-only model continuation reads every retained map field back out and
  reproduces the E2 envelope exactly. Ten malformed token frames reject with
  no partial tape. E3 join tests pass on cc, unisacc and both macOS assembly
  runtimes (x86_64 through Rosetta): /tmp/unisacc-e3-location-{chain,ua,arm,x86}.log.
- E1 default/typed/positions serialized models are unchanged. E3 default is
  byte-identical to ac3009d with the same PYTHONHASHSEED=0 (24,349,909 B); an
  initial comparison without a fixed seed differed because generator set
  iteration is seed-dependent. No runtime semantics change is inferred from
  that uncontrolled serialization comparison.
- exec-lexloc and exec-parseloc join the bounded gate. This batch did not run
  the whole gate or Linux/Windows native tests. No product/container rebuild
  or switch: compiler routes do not select this mode. Remaining is diagnostic
  rendering and warning rules, plus multi-unit location framing and CLI
  -Wall/-Wextra/-Werror integration. Last measured CLI remains 63/64.
  Local commit only; no push/release. Full reconstruction remains active.


### Diagnostic renderer and first warning rule — development route

- Optional E3 --warnings (implies --locations) implements reference
  -Wreturn-type decisions and summary, plus source/caret rendering and header
  warning suppression. Compiler CLI still does not select it: full -Wall,
  other warning kinds, multi-unit integration and -Werror remain unfinished.
- Renderer: 11 cases x 2 modes compare with actual reference diagnostic
  functions. Return warnings: 32 cases, 14 with warnings, compare tape and
  stderr; quiet mode also matches tape with empty stderr. Both checks passed
  with cc, unisacc and macOS arm64/x86_64 assembly runtimes (Rosetta for x86).
- These tests exposed an existing E3 floating-condition gap: it omitted
  reference ftruthy conversion. Model FTRUTH/FNOT now cover if/loops/ternary,
  logical operands and unary !, preserving negative-zero and NaN behavior.
  Product/reference code was already correct. float_truth.c runs successfully
  with host cc and the reference; quiet/warning model tapes match reference.
- Before adding float_truth.c, all old 243 kept tapes passed unchanged.
  The new probe is separately verified and raises keep-e3 to 244. Self-source
  tape remains byte-identical (3,929,446 B). compilercheck.sh passed all
  27 mode/level matches and its existing CLI/resource/IO/decimal checks.
- exec-diag and exec-returnwarn join the bounded gate. No whole-gate result,
  Linux/Windows native claim, default .com switch or release in this batch.
  Default E3 model changes for the floating-condition fix; no serialized-model
  identity claim. Runtime remains generic model actions.

- Rebuilt development container (not shipped default): 3,265,064 B,
  SHA-256 b8c29bed5211b39071ee4fc58ae0de255efed0c1199cc2cc7594a225c1a12f63.
  Its float_truth probe compiled and ran with exit 0 as native osx/arm64,
  osx/x86_64 (Rosetta), and memory -run. The subprocess harness must launch
  the APE through /bin/sh on macOS; a direct subprocess exec first raised
  ENOEXEC before compilation, then the corrected harness passed.


### Integer-to-pointer warning rule — optional E3 diagnostic route

- --warnings adds the reference intptr_check rule for assignments (names,
  members, indexed/dereferenced lvalues) and local scalar initializers. It
  preserves the lone numeric-zero exemption and call-result tracking,
  including the difference between direct and function-pointer calls.
  It does not extend checks to returns, compound assignments or global/static
  initializers where the reference does not run intptr_check.
- A chained assignment exposed an older E3 descriptor difference: stores
  returned target facts where the reference normally retains RHS facts.
  AS.result now retains RHS facts except for a floating target's setkind.
  Store width still uses the target. This is reference compatibility, not a
  claim that all reference conversion semantics are proved C-correct.
- 29 cases compare full tape and diagnostic bytes, 16 with actual warnings;
  the quiet route also matches. Passed on cc, unisacc, macOS arm64 assembly
  and x86_64 assembly (Rosetta). Includes zero/parenthesized zero/enum/casts,
  nested assignment, direct/indirect calls, member/index/star stores, header
  suppression and explicit no-check contexts. Existing 32 return-warning
  cases also pass (14 warn). Original 244 kept tapes and self-source tape
  (3,929,446 B) remain byte-identical. No keep-list expansion in this batch.
- exec-intwarn enters the bounded gate. Full -Wall remains unavailable:
  unused-variable and format warnings, multi-unit location integration,
  -Werror and CLI selection remain. No full-gate or new native-platform claim.
  Product source and shipped .com unchanged; local work, no push/release.


### Located reader repair: static-local token ordinals

- Source inspection found that the located NEXT entry bypassed IX.have,
  which assigns stable source-token ordinals for static local labels. A new
  two-function static-local fixture failed before the fix with "identifier
  is not a local" (rc 1). This was a development location-mode defect, not
  a shipped product regression.
- The located reader now preserves IX.have/IX.new after its position prefix;
  it passes the ordinal table explicitly. The isolated diagnostic-renderer
  test continuation has no grammar ordinal table and retains its own entry.
- Nine location-mode tape comparisons and ten malformed-frame checks pass
  on cc, unisacc and both macOS assembly runtimes. The new static case is
  also in the return-warning check: 33 cases, 14 with warnings, exact tape
  and diagnostics on cc. This prevents a green location suite that only
  exercises automatic/global storage.
- Current compilercheck.sh passes 27 mode/level comparisons plus its existing
  CLI/resources/IO/decimal/native and memory-run checks; package 1,250,262 B.
  No full-gate result, default product switch or publication is claimed.
  Remaining warning/CLI work and the full reconstruction goal remain active.


### Unused-variable warnings — optional E3 mode

- The model now tracks binding/use records alongside the existing scope undo
  stack. It restores shadowed bindings, reports in source declaration order,
  excludes parameters and static/global storage as the reference does, and
  uses primary()'s token-context rule for reads versus standalone writes.
  Token facts come from the existing whole-token INDEX walk; declaration
  locations feed the shared diagnostic renderer. No executor primitive added.
- 38 cases (21 warning-producing) match complete reference tape and stderr
  on cc, unisacc, macOS arm64 assembly and x86_64 assembly via Rosetta.
  Cases include shadow/restore (automatic, static and enum), arrays, members,
  indirect calls, sizeof, nested blocks, for scopes, initializer reads,
  multiple functions, name truncation, macros, splices and header suppression.
  An initial splice fixture contained literal backslash-n instead of a
  physical continued line; corrected before the successful runs.
- Existing 33 return-warning and 29 integer-conversion cases also pass on cc.
  Default (non-warning) E3 serialization is unchanged from ec628eb with fixed
  PYTHONHASHSEED=0: 24,570,115 B, directly compared. Thus no default product
  rebuild or new default-route regression claim in this optional-only batch.
- New rule module: 52 source lines plus parser hooks and tests; this migrates
  the rule into model actions, not a code-size reduction. exec-unusedwarn is
  added to the bounded gate. No full-gate/platform/release claim. Format
  warnings, multi-unit warning framing, -Werror and CLI integration remain;
  reconstruction is still active and the shipped .com is unchanged.


### Format warnings and scalar-float default argument promotion

- Optional E3 --warnings now includes the reference printf preflight and
  format checks, using generic model actions and the shared source/caret
  renderer. It preserves literal decoding, star arguments, call exemptions,
  nested diagnostics and the reference label-allocation side effects. Quiet
  and warning tapes are each compared against the matching reference mode.
  No language-specific executor primitive was added.
- 36 cases, 28 with format warnings, pass with complete tape/stderr comparison
  on cc, unisacc, macOS arm64 assembly and x86_64 assembly/Rosetta. The initial
  unsplit unisacc check hit the 60 s alarm (rc 142), not a pass; two disjoint
  18-case shards pass with 15 and 13 warning cases. The bounded gate gains
  these two shards. Existing return/int-conversion/unused checks pass on cc
  (33/29/38 cases). No full-gate result is claimed.
- A quiet-route difference exposed missing default scalar-float promotion
  when no formal kind is recorded. The parser now uses the existing TO.d
  conversion there. Original 244 kept tapes passed before list expansion;
  the new vararg_float probe brings the keep list to 245. Self-source tape
  is still 3,929,446 B and byte-identical; compilercheck passes 27 mode/level
  comparisons and its existing CLI/IO/native/memory checks.
- Fresh development container: 3,265,512 B, SHA-256
  97e8a6a33dc5e9898d1e6489b6dfb05769e30ef25217e2d3a335ab2322ef79fb.
  The new probe prints exactly `1.5 2.5` (rc 0) in memory and as native
  osx/arm64 and osx/x86_64 programs. Shipped unisacc.com is unchanged.
- Four warning rules are implemented in optional single-unit mode; -Wall
  CLI selection, multi-unit locations and -Werror remain. No default product
  switch, new Linux/Windows execution claim, push or release. Reconstruction
  remains active; FX conjectures do not add implementation scope.


### Single-unit warning CLI and Werror acceptance barrier

- The development package now carries target-specific located preprocessors
  plus shared located lexer/warning-parser networks. Its driver routes
  single-source -Wall/-Wextra/-Werror to them. Warning decisions stay in the
  parser model; -Werror is an explicit byte resource, and the model rejects
  after its summary when the warning count is nonzero. Empty rejection keeps
  the reference stderr without adding a runtime reason. No executor action
  added, and no C-side diagnostic-text classification.
- c/warningcheck.sh passes 60 complete rc/stdout/stderr comparisons across
  host-C, unisacc-C and assembly-backed drivers, under alarm 60. It covers
  all three flags, three optimization levels, preprocessing/token dumping,
  and rejection before output/dependency mutation or execution. A clean
  -Werror program still runs. Multi-unit warning mode remains an explicit
  refusal until its source maps are integrated; no full-parity claim.
- This test exposed an existing token-dump mismatch: its preprocessor was
  performing implicit header selection. That route now constructs E2 with
  E2_AUTOINC=0, matching the reference lexer instrument; explicit includes
  remain supported. Compilation routes retain implicit header selection.
- compilercheck.sh remains green (27 mode/level comparisons and its other
  contracts). A rebuilt development container passes tests/cli.sh 64/64
  from an isolated directory, plus six full warning-mode tape/run result
  comparisons. Container: 4,830,280 B; SHA-256
  a4f47e3f329ba8a5a135e460dd8c694a6b7cca24ff986052e02b5e707ac189f6.
  Warning models increased the container from 3,265,512 B; no size reduction
  claimed. Offline generation still uses Python; runtime does not.
- These are local macOS arm64 results, not a full gate or cross-platform
  release result. The default shipped .com is unchanged. No push/release.
  Next: per-unit located framing and warning diagnostics for multiple inputs;
  the complete reconstruction goal remains active.

### Multi-unit warning maps and per-file diagnostic state

- The located unit-framing model emits UNITOK2: filenames and independent
  UNIPP1 source maps followed by unit-indexed positioned tokens. E3 reloads
  the appropriate context on every read/rewind. Static-name isolation stays
  in the existing scanner; C only frames bytes and selects routes.
- The physical-token scanner now retains qualifiers and shares the parser's
  multiline adjacent-string reader. Warning neighbor facts include the unit
  epoch. Warning names and printf recognition use original preprocessed
  spelling, avoiding internal __uN suffixes in diagnostics or missed checks.
- c/multiwarningcheck.sh passes 120 complete rc/tape/stderr comparisons across
  host-C, unisacc-C and assembly-backed drivers: both file orders, four warning
  kinds, headers, splices, static shadowing, adjacent strings, empty/clean units,
  optimization, run and Werror barriers. Single-unit entry points remain.
- unitlocationcheck.py passes on cc, unisacc and both macOS assembly ISAs:
  independently serialized bytes, UTF-8 filenames, reference diagnostics,
  ten malformed map containers and seven malformed input frames. No executor
  primitive added. This is migration evidence, not a proof over all inputs.
- Testing found a product defect: fe_load retained prior units' splice,
  include-name/region and automatic-include maps. It now resets nspl, nireg,
  nfnpool and nautoinc per file, while program warning counts remain shared.
  tests/diagunits checks 18 fixed file/line/column expectations at O0/O1/O2 in
  both orders; host cc independently confirms the warned source line. The
  old shipped artifact fails this check (missing a warning); the rebuilt
  reference passes. Another pre-fix observation reported line 5 for line 4.
- Frozen-tree gate evidence: 101 distinct suites, all rc 0 and each at most
  60 s. The serial aggregate's existing 900 s outer watchdog expired (142)
  after 71 saved successful receipts; a scratch copy of the same gate skipped
  exactly those names and ran the remaining 30 successfully in 208 s. This
  is complete coverage across two invocations, not a successful first aggregate.
- Rebuilt reference unisacc.com: 1,350,016 B, SHA-256
  bbff6f182d8b90060b0e9527432869d180cd58046e48ab972f197aac6e1c9165.
  Its product-facing gate cases passed, including diagunits and CLI 64/64.
- Separate development assembly/network container: 5,294,871 B, SHA-256
  788155c72a685ec924efb16de3c07a5565f98fe4f75f8c51b8d6622b0c5d2cbf.
  Actual container CLI passes 64/64 from an isolated directory, plus 10 full
  result comparisons for multi-unit warning/tape/run modes in both orders,
  including Werror output/dependency preservation. Size increased; no reduction
  claimed. Offline construction still uses Python.
- These are macOS arm64/Rosetta results and cross-generation checks; no Linux
  or Windows VM was run for this batch. No default model-route switch, push
  or release. The overall reconstruction goal remains active.


### Located parser errors and bounded recovery (development route)

- Normal and warning compilation routes now retain per-unit source maps.
  E3 --errors maps unknown identifiers, expression starts and expected
  punctuation to reference diagnostics, unwinds scopes and input views, and
  resumes at a balanced top-level boundary. Any error prevents tape publication.
  Other prototype limitations retain their explicit not-covered reason, gain
  a location, and stop; this is not reference-error equivalence.
- The driver carries -ferror-limit= as an opaque resource; the model reads it
  (default 20, zero unlimited). No diagnostic classification or recovery was
  added to the C executor. Existing warning rules remain model actions.
- errorcheck: 17 complete rc/stdout/stderr comparisons plus one located
  prototype limitation on host C, unisacc C and both macOS assembly cores.
  Every loaded network is enumerated against its table. Multi-unit checks:
  120 warning comparisons plus 18 error/recovery/limit comparisons, both
  file orders, across three drivers. Ordinary multi-unit checks also pass:
  36 tape comparisons, native runs, scope isolation and failure preservation.
- The combined driver suite exceeded its outer 60 s budget (rc 142); it is
  split into core/resources with the same assertions. Both bounded parts
  pass. diag.sh's two legacy 120 s child alarms are now 60 s. These targeted
  results do not claim a new complete gate or new-platform validation.
- Actual development unisacc-next.com: 5,735,300 B, SHA-256
  417a110d6593a5cbb1091fd472858a195c51af744a30e4c66728964807756bb1.
  diag 14/14 (including 40 damaged inputs), CLI 64/64. ccparity at this slice
  is 50 ok, 3 wrong, 1 known: C99 macro examples ex3/ex4/ex7 still fail;
  the known -c object-file incompatibility remains. Macro parity is next.
- Product sources/default .com are unchanged. No default model-route switch,
  push or release; offline construction still uses Python. FX-1..FX-4 remain
  unscheduled conjectures, not prerequisites for this work.


### Active compatibility work: macro invocation and replacement lists

The remaining ccparity macro failures reach explicit E2 limitations: zero-argument
and variadic calls, replacement-list boundary crossing, and hash/paste handling.
Continue the existing model path by reproducing the three examples, then add
only the missing preprocessing semantics with focused reference comparisons.
Runtime decisions must remain generic actions in constructed models. Preserve
existing byte output, hide-set behaviour and located preprocessing; do not
replace the model route with a C macro expander or count a rejection as parity.


### Macro invocation/rescan compatibility accepted (development route)

- E2 now parses zero-parameter and final variadic parameters, preserves the
  remaining commas as variadic argument text, and collects a macro call across
  replacement-frame boundaries without crossing argument-expansion barriers.
  It handles object-like # and the standard # ## # form, strips internal
  separators from operands, and removes a hide mark only on a pasted boundary
  token. Other tokens in the same argument retain their hide marks. This last
  distinction caught and fixed a repeated expansion in a multi-token operand.
- macrocheck.py covers 20 inputs: the three C99 examples from ccparity plus
  focused zero/variadic/cross-frame/stringize/paste/hide cases. Both ordinary
  and located preprocessing produce exact reference bytes (40 comparisons);
  system cc independently agrees on preprocessing tokens. Python simulation
  and actual constructed-network execution agree. All pass with host C,
  unisacc C and both macOS assembly cores; network=table enumeration is run
  for both formats. The suite is in gate as exec-macros.
- Fixed chain list: 96/96 equal, 0 rejected/not-covered/bad/lost. Existing
  location suite: 9 reference envelopes and 5 Python oracle cases pass.
  Full current-source osx/arm64 route: separate stages and shared package
  give the same 743,202 B image as the reference; native N1=N2=N3.
- Actual development container: 5,761,242 B, SHA-256
  36d173b7f138b430eaaf1b574cad8e602cb473291404172e5e176435f2a83826.
  ccparity is now 53 ok, 0 wrong, 1 existing known (-c is not an object file);
  diag remains 14/14 and CLI 64/64. These runs use the container itself.
- Size account: gen.py 1,254 -> 1,307 lines; ordinary E2 642 -> 671 states,
  164,787 -> 172,240 entries. Its Linux/x86-64 network is 64,375 B; the complete
  package is 5,577,930 B. No new executor action; no source-size reduction
  claim. The container grew 25,942 B over the diagnostic slice.
- Every invoked suite/build was bounded at 60 s. This is targeted validation,
  not a new complete release gate or Linux/Windows native run. General
  punctuator/literal paste forms, _Pragma and other declared E2 limitations
  remain; passing these examples does not prove all preprocessing semantics.
  Default product remains the reference; no push/release or switch this batch.


### Broader candidate audit after e291398

Actual model container 36d173b7... passes ccparity but is not ready to replace
all product paths. C99 reports 39/57, 18 refusals: empty variadics, __func__,
_Pragma, hexadecimal floating literals, bool/_Bool, restrict/inline, VLAs and
VLA parameters, flexible/static array parameters, compound literals, va_copy,
math/atexit/div/labs and signal declarations. run.sh passes 11 with one failure:
b_float stops at mixed float/double arithmetic (double-operand limitation).
These remain implementation work, not waived baseline entries.

The C99 harness also loses the original status through `if ! command; then
rc=$?`, and ignores a host/non-host status mismatch on the success branch.
Fix status capture and bound the host build/run before relying on expanded
coverage. Verify with a compiler wrapper that returns correct output then
exits 2; it must fail. No product compiler change is needed for that fix.


C99 harness status fix verified: host compilation and execution are now bounded;
compiler/run status is captured before branching, compared on both paths, and
signal/timeout-shaped exits fail instead of becoming unsupported cases. A
scratch wrapper emitted the reference output, suppressed stderr and exited 2:
the old script falsely passed 57/57 with rc 0, the corrected script reports
57 wrong and exits 1. Normal reference remains 57/57; the actual model
container remains 39/57 with the same 18 refusals and rc 1. The baseline was
not lowered and no known-failure waiver was added. Next: finish the remaining
preprocessor gaps in that list, then front-end type/declarator/expression gaps.


### Next E2 compatibility slice

Migrate the reference's comma-before-final-variadic-argument rule into the
existing macro rewrite delta. Empty raw arguments remove the comma; nonempty
arguments retain it without token pasting. This is GNU-compatible reference
behaviour, not a claim about mandatory C99 semantics. No executor action or
product-source change. Pin both empty and nonempty cases before proceeding
to the remaining _Pragma and front-end gaps.

Comma/variadic slice verified: 25 probes in both ordinary and located formats
(50 full outputs), on host C and both macOS assembly executors. All match the
reference and Python simulator; 24 cases additionally match host preprocessing
tokens. Explicitly empty final arguments have a separate hand token expectation
because host compilers may retain their comma. Network/table enumeration passes.
No executor primitive added; gen.py grows 1,307 -> 1,322 lines.

Actual development candidate is 5,770,202 B, SHA-256
985d90cc87bc837305bc99059d5a5ce0c01b0d89f7a78b6254cce286f650d038;
package 5,586,890 B. C99 is 40/57, 0 wrong, 17 refused (suite correctly exits
1); ccparity remains 53 ok, 0 wrong, 1 existing known difference. The empty
variadics probe is now supported. Logs: /tmp/unisacc-comma-{check,arm,x86,
build,c99,ccparity}.log. Every run bounded at 60 s. This is targeted validation,
not a release gate or default-product switch. _Pragma and the other 16 refusals
remain work; no baseline reduction, push or release.


### E2 _Pragma operator migration

The reference consumes a balanced _Pragma call without honouring its contents
or expanding its arguments. Implement that scan with ordinary delta actions,
including quoted parentheses, replacement-frame boundaries and source newline
accounting. Bare names remain text. Compare exact reference output separately
from host pragma semantics: this is not an implementation of pack or diagnostic
pragmas. Unterminated calls remain an explicit prototype limitation.

_Pragma slice verified: 43 macro probes in both output formats (86 full
outputs), plus two malformed-call refusals per format. Host C and both macOS
assembly runtimes agree with the simulator and reference; constructed network
= table enumeration also passes. Twenty-four probe token sequences additionally
match host cc; nineteen use explicit reference-contract token expectations
(comma extension and ignored pragma behaviour). A self-referential _Pragma
name exposed lost suppression after frame lookahead; the name's active/paint
state is now captured before popping, and the formerly looping case is fixed
on the permanent list. No new executor action. gen.py: 1,322 -> 1,376 lines.

Existing location checks pass (9 reference envelopes, 5 simulator cases),
fixed chain 96/96 with no rejects/not-covered/bad/lost. Actual model container:
5,796,265 B, package 5,612,953 B, SHA-256
8c341754b0354e6f07a02b1055d995d3421c72d9fb2f936dfe8eb57e4cca9ac9.
ccparity remains 53 ok / 0 wrong / 1 known. C99 now 41/57, 0 wrong, 16 refused;
the suite still correctly exits 1. The reference's ignored _Pragma is covered;
pack semantics are not newly implemented or claimed. Logs are
/tmp/unisacc-pragma-{check,arm,x86,location,chain,build,c99,ccparity}.log.
All runs bounded at 60 s. No default-product switch, push or release. Next are
__func__ and the remaining type/declarator/expression/library compatibility gaps.


### E3 predefined function name

Implement __func__ inside function bodies as a pooled name, preserving the
reference's source-order literal numbering and its 120-byte name bound. The
reference currently writes an explicit NUL in this pool entry and the usual
pool terminator, so byte comparison must retain both. Use normal delta string
operations, no executor intrinsic. Outside-function use stays explicitly
unsupported; it is not a C99 use of the predefined identifier.

A required __func__ subscript probe exposed a reference defect: its primary
expression only sets curptr/curelem, retaining curpd and other kind state, so
__func__[0] emits no character load. Model tape has the load and therefore
differs. Fix the product descriptor through setkind before accepting parity;
retain a character-access regression in the existing C99 feature probe. The
Python front end has no __func__ implementation to update. Rebuild the product
.com and run its bounded gate after this product-source change.

The committed-reference binary was rebuilt privately for a before/after test.
With first() returning __func__[0], it printed an address-valued integer
(648511783 in that run) instead of 102. Fixed reference, rebuilt product .com
and host cc print `main 109 109 102`. The first model candidate still rejected
*__func__ through its specialised dereference-name path; wire that path to the
same pooled-name value before reporting model acceptance. The earlier direct
printf argument happened to work and is not the defect's regression proof.
The existing C99 probe now includes this independent function. Old fixed chain
96 passed before adding the two new function-name files (98/98 total).

The first frozen gate was cancelled (rc 137), not counted as validation, after
the model dereference gap became known. Re-run the complete bounded gate after
the fix. An earlier scratch command incorrectly sent Python stdin through
term.sh and timed out; the subsequent test uses a real script file.


### Owner correction: aggregate timeout must remain bounded

The second function-name gate was incorrectly launched with TERM_SH_ALARM=0.
Per-suite alarms did not bound the overall background run. At the owner's
correction the gate and its 10 descendants were killed; no gate success is
claimed. Set terminal/gate defaults to 60 seconds and reject zero or values
over 60. Remaining full validation must be split into bounded batches. The
function-name/product changes remain uncommitted pending that validation.

Timeout correction follow-up: merely setting alarm(60) on the wrapper still
leaves descendants alive. Add a small process-group watchdog that preserves
exit status and kills the whole owned group on timeout/interruption. Gate runs
will require explicit suite selection; --list enumerates the same authoritative
job declarations without executing them. Validate selection before running.
No unbounded aggregate gate will be relaunched.

The new timeout probe passed: an outer 1-second watchdog terminated a nested
30-second watchdog and its sleeping child, returned 142 in 1.29 seconds;
normal exit 2 remained 2. Gate rejects absent/unknown selections before builds.
Next enforce the same outer limit for direct TERM_SH=0 gate calls, and bound
the Terminal handoff wait as well as the command. Full gate is now a sequence
of explicit selected batches; an interrupted batch supplies no passing receipt.

### Test efficiency: rolling queue and shared preparation

Owner explicitly requests parallel queue scheduling and measured efficiency.
Current frozen checks: 102/105 pass; exec-native, exec-multi and exec-memx86
hit the 60-second limit (not passes). All other queued checks finished.
Inspection found elf.sh reconstructs the same six models in every private
suite directory. Separate immutable, content-checked preparation from source
execution; key it by generator/table/header/runtime inputs, target, network
mode and compiler identity, serialize identical builds, and publish atomically.
Cache hits still verify artifact hashes; cold preparation retains full-domain
network checks. This changes preparation reuse, not suite assertions.
Make rolling scheduling reusable with a bounded scheduling window and retained
per-suite results, so work continues without hand-written batch barriers.
The three timeouts must be rerun after the preparation improvement; no waiver.

The first rolling-window rerun retained partial logs: native reaches the stream
chain but not resource-packaged self checks; multi reaches only the first pair.
Both still exceed 53 s under two-way load, so caching alone is insufficient.
Split native by stages / chain / resources; split multi and POSIX memory checks
by compiler backend (cc / ua / asm), retaining the union of assertions. Build
only the backend used in a shard. The queue keeps late-window timeouts pending
for an early full-window attempt; a full-window timeout remains a failure.

Rolling queue validation: 12/12 replacement shards pass; the longest is
46.34 s (Rosetta/UA memory). Native stages/chain/resources took
20.86/30.32/17.36 s. Cold model preparation was 4.36 s, warm verified reuse
0.08 s. A simultaneous same-key cold request built once (4.20 s); the second
waited and reused it (4.09 s), all 21 artifacts identical. Queue/cache controls
pass for rolling refill, resume, changed-input refusal, nonzero exit despite
PASS text, timeout, artifact corruption, missing manifest entry and header
invalidation. Terminal output is now streamed (measured 2.05 s between lines
emitted two seconds apart), not buffered until completion.
The backend split initially repeated backend-independent framing/loader
checks in every shard. Keep those in the cc shard only (and all mode), retaining
their original two-executor / two-build checks, and skip their extra preparation
in ua/asm shards. Every selected backend still runs its entire behavior matrix.

After corruption, a cold rebuild restored all executable tables, networks and
packages byte-for-byte. Two JSON files differed only in dictionary key order
(parsed objects equal). Pin PYTHONHASHSEED=0 in cold preparation so cached
intermediate JSON is also reproducible; this is not a model-behavior change.
The post-deduplication queue passed 10/10, with windows 51.41/48.16/30.70 s.

The final current-key corruption experiment rebuilt the private cache and
restored all 21 artifacts byte-for-byte, now including JSON (4.17/4.25 s cold).
Two concurrent same-key callers build only once; a subsequent warm check was
0.08 s. These are preparation timings, not a claimed whole-suite speedup.
Queue windows retain all completed results, validate the frozen input snapshot,
and use exit 75 for pending work; no pending task is reported green. The legacy
release caller now invokes one bounded queue window and treats pending as not
ready. Its older all.sh release orchestration is not run or certified here.

cc-unisacc supplied two new reported product defects (not yet independently
executed in this session): unsized static-local and file-scope function-pointer
arrays. Source reading finds fpdecl defaults [] to one slot and the local
function-pointer-array path precedes the static-storage branch. Reproduce with
host cc and bounded runs, then fix the declaration/storage rules, not a special
case for 3 or 8 elements. They remain open; test-green is not a C99 proof.
The requested section 5.6 roadmap is retained as explicitly unverified advice,
not a replacement for S-17 acceptance criteria or a completion percentage claim.

Final targeted queue on the finished infrastructure: 6/6 pass in one 35.39 s
window (docs, kernel, C99 57/57, complete self-source through Linux x86_64 and
arm64 network routes, queue/cache controls). The two full-source images equal
the reference at 671422 / 720572 B. This is targeted revalidation after harness
changes, not a claim that one fresh invocation reran the entire new 114-item
list. Earlier bounded runs plus replacement shards cover the old gate's tests.
No push, release, default-route switch, or new cross-platform execution claim.

Function-name slice closed locally: __func__ and its direct dereference use the
same captured function-name pool entry path; the fixed chain is 98/98. Product
C99 is 57/57 including `main 109 109 102`; the actual model container is 42/57,
0 wrong, 15 explicit refusals (not a green C99 claim). Product .com SHA-256:
61d01e7a7017df9d9c31ece1a874a403fe998163c910283a8da342f5226be27e,
1,350,032 B. The remaining declaration/library gaps and reported function-
pointer-array defects remain open. Test infrastructure is committed separately
as 1cad167; no pending unrelated changes are swept into the product slice.


Function-pointer-array correction in progress (2026-09-27): independently
reproduced both reported probes on the current product .com. Host cc exits 0
(argc 1); product local probe prints 652742492, global eight-pointer probe
terminates with SIGSEGV. fpdecl treats an unsized [] as one pointer, and the
local array branch bypasses static storage. Fix the inferred extent and route
static arrays through the existing static-storage initializer. Check explicit
and inferred extents, automatic/global/static storage, and persistence across
calls. Python's declarator/local_decl already represents an unsized array and
infers its initializer count; verify before deciding it needs a change.

Function-pointer-array fix, targeted evidence: product -O0/-O1/-O2 in both
-run and native forms, plus unchanged Python front end, equal host cc on
b_staticfnptr_local (1 71 61 30 20 24 24) and b_fnptr_table8 (56 1 64 72).
These cover inferred/explicit extents, automatic/global/static storage,
zero-filled trailing slots, frame preservation and static persistence.
E3 uses the shared initializer counter and static storage walker; global
function-pointer initializers now enter the existing initializer path.
Old fixed E3 245 + new 2 equal using the C table executor; both new cases
also equal on the Python simulator. Source -> E2/E1/E3 threshold networks:
old 98 + new 2 = 100 equal, 0 lost/bad/refused. Only after those runs were
keep-e3/keep-chain raised to 247/100. E3 = 4492 states, 8777 hidden units,
506991 B network, exhaustive network/table check on 1158680 observations.
Full fresh gate --com queue follows on this tree; targeted success does not
replace it. No release or default-route switch.

Gate observation: closure-c2 found a probe-environment mismatch, not an image
or array-value mismatch. Its `$UA source -run` invocation passes the trailing
-run as a program argument (argc=2), while the tape VM sees argc=1. New local
probe now captures argc at main entry and checks it remains unchanged after
array initialization, rather than requiring a fixed argument count. All six
images already matched; the static-array values were identical. Continue the
frozen run, then recheck the corrected probe through both executors/closure;
do not erase the original failed receipt or call it a passing run.

Function-pointer-array slice verified (frozen candidate based on 31e033d plus
this patch): `/tmp/unisacc-fp-frozen-gate` completed all 114 gate --com items,
112 pass and two original failures (closure-c2, difftest_o), both solely the
new probe's argc=1 assumption. Preserve that failed run. After changing ONLY
the probe to compare argc with its entry value, `/tmp/unisacc-fp-finalcheck`
passed closure-c2 (186 identical images; 31 host runs), difftest_o (390 agree,
0 wrong) and the 100-file network chain. No product/model source hash changed
between these runs. This is full-list evidence plus focused correction, not
one pristine 114/114 run. Longest suite 49.861 s; rolling queue uses two slots,
all windows bounded below 60 s, no timeout failures. Earlier root-tree window
was invalidated on a concurrent example edit; its receipts were not reused.

Both final probes also match host cc with product and actual model container
at -O0/-O1/-O2, in -run and native compiled execution. Unchanged Python front
end agrees. Product .com: 1350992 B, SHA-256
bbb4bd59b4b0e9b5c2f92b5fbd28571ad3408be39b3e08a4e7d4c64edd9424ca.
Development container `/tmp/unisacc-fp-modelcandidate/unisacc-next.com`:
5819785 B, SHA-256
fae217ff8fefbe923912535e17cc639dba3197ac9413ec7960e81d7d3e704bdd.
Fresh model C99: 42/57, 0 wrong, 15 refusals, exit 1; remaining compatibility
work is not waived. Product C99 57/57 is not proof of language completeness.
No default-path switch, push, release, or new VM execution claim.

External example updates 76bf9a0/31e033d/2287f58 are cc-unisacc's independent
work. Optional user-facing process/directory/syscall bindings are reported
as absent, not scheduled in this repair. Their new VM claims were not rerun
by cdx; no such claim is inferred from the image-comparison gates above.

Next compatibility slice: declaration-only `restrict` and `inline` follow the
reference's qualifier handling. Share the token-reader skip policy with the
unit-framing pass, which must preserve physical tokens and static-name spans.
Do not fold static/extern into this policy: they affect storage/linkage. Verify
the C99 probes and fixed E3 list; this does not claim full C99 coverage.

Qualifier slice verified: old E3 247 plus s56_qualifiers all equal on the C
table executor; the new probe is also equal through all three threshold
networks. Old network chain 100/100 remains green. Located multi-unit test
now includes a static inline function with a restrict parameter and passes
independent framing bytes plus reference tape/diagnostics. Both gate jobs
ran in a two-slot bounded queue: 3.26 s and 9.84 s, wall 9.88 s. After those
checks the E3/chain keep sets become 248/101. No executor action added.
Actual rebuilt development container at /tmp/unisacc-qual-candidate:
C99 44/57, 0 wrong, 13 refusals (previously 42/57); restrict and inline
are now accepted with correct output. The suite correctly exits 1 against
the unchanged 57 baseline. No default switch or completeness claim.

Next floating-literal slice: hexadecimal significands reuse the existing
limb ratio normalizer and nearest-even packer. Parse base-16 digits and a
mandatory binary p exponent in the delta, then add the binary exponent to
the normalized ratio exponent; no host floating parser or executor action.
Check host-cc bit patterns at rounding/subnormal/overflow boundaries, and
full source through the actual model compiler. _Bool stays open: its width
and normalization must be represented across declaration/conversion paths.

Hexadecimal floating slice verified: 61 host-cc bit-pattern expectations
(decimal plus hex), table and threshold-network execution agree; 10 invalid
forms reject on both. Covers ties-to-even, normal/subnormal boundary, half
minimum subnormal, maximum finite/overflow, huge signed exponents and a
1001-digit hexadecimal significand whose exponent cancels its scale.
Old E3 248 plus C99/07 all equal, old source/network chain 101/101 retained;
new C99/07 separately equal through the network chain. Keep lists raised
only afterwards to E3 249 and chain 102. Two-slot gate queue: float bits
3.14 s, chain 9.86 s, total 9.90 s. No new executor action.
Rebuilt /tmp/unisacc-hex-candidate/unisacc-next.com SHA-256
048f9444d5bc86fabc88dd0b8857a6f006f1eea273c957c58f7528a7f7f620bf:
actual C99 45/57, wrong 0, refused 12, rc 1 against unchanged baseline 57.
The reference product was not changed; no release/default switch.

va_copy investigation: stdarg.h already expands it to ((destination) =
(source)); the missing model rule is parenthesized lvalues, not a va_copy
builtin. Add a syntax-only lookahead for parenthesized assignment/update,
then compute its address once using existing name/member/index/dereference
walkers. Share the assignment, compound-update and postfix emitters. Verify
ordinary parenthesized rvalues stay unchanged, nested/index side effects
occur once, and copied va_list cursors advance independently.

Parenthesized assignment/update slice verified: old E3 249 plus three
probes equal. The first keep run rejected exec/c/run.c because (I)++n was
misclassified as postfix update; type-name lookahead now preserves the
cast/prefix interpretation, with a dedicated regression in s57. Old chain
102/102 and the three new files independently pass network execution.
Keep sets raised afterwards to 252/105. Self-source tape 3933309 B is
identical; two queued gate jobs took 3.28/9.97 s, wall 10.02 s.
Actual network compiler matches host cc on va_copy, b_lvalue and s57 at
-O0/-O1/-O2: 14; 9 8 9 22 / 9 9; 1 6 7 5 18 respectively. s57 tests
copy after consuming one variadic argument, independent cursor advances,
nested parentheses, one index side effect and cast/prefix disambiguation.
This adds expression-entry parenthesized lvalue handling; it is not a claim
that every possible nested lvalue grammar is covered. No va_copy intrinsic
or executor primitive was added.
Actual /tmp/unisacc-lvalue-candidate/unisacc-next.com SHA-256
6d0e545a03ed637b69b359d17bd835231ad700e186b3ced9d28c5c31e5515aa8:
C99 46/57, 0 wrong, 11 refusals, rc 1 against baseline 57. No product
default switch, release, or platform-validation claim.

Next declaration slice: signal.h exposes a general missing shape, a function
returning a function pointer (`T (*name(params))(result_params)`). Reuse the
existing parameter/body walk and FP signature skipper; carry the wrapper
explicitly until the outer declarator closes. Enable void-return function
pointer casts through the existing FP descriptor. No signal-specific model
rule; test a renamed factory plus the actual header and preserve fixed lists.

Function-pointer return declarations verified: old E3 252 plus signal and
s58 factory = 254 equal. Old network chain 105/105 remains green; both new
files separately equal through threshold networks. Keeps raised afterwards
to 254/107. Self-source tape remains 3933309 B identical. Queue two jobs
3.33/10.12 s, wall 10.16 s. No executor primitive or signal-specific rule.
Actual new container at -O0/-O1/-O2 equals host cc: signal prints
1 / 15 / still here; factory prints 8 4 7 1 12. The factory includes a
prototype, callback parameter, immediate call of the returned pointer,
void callback return/cast and an ordinary function after the wrapped one.
/tmp/unisacc-fpret-candidate/unisacc-next.com SHA-256
11561d7b3c450faeecbd3556f9e2af7dd64a944bb833b345102a2c66edd44f19.
Fresh model C99 47/57, 0 wrong, 10 refusals, rc 1 against baseline 57.
This does not extend the bundled signal implementation to external OS
signal delivery. Product sources and the default product remain unchanged.

_Bool audit before model migration found reference-product defects. A valid
probe prints `1 1 1 1 1 1 0 1 1 0` with host cc but product prints
`1 0 0 0 2 2 0 2 2 0`: floating assignment, cast, function return/parameter,
aggregate initializer and increment fail to normalize. Fix the existing
conversion-kind 9 propagation at these boundaries first; preserve boolean
type metadata through typedefs and declaration lists, and preserve postfix
old values explicitly. Do not reproduce these errors in the delta.

Bool validation: native -O0/-O1/-O2 match cc on b_boolconv. The bounded gate stopped after 27/114 (one failure): fat exposed that Python resolves _Bool as int. Repair the same scalar conversion contract there, keeping its arithmetic axis u8; do not waive the probe.

Python bool repair keeps a boolean flag on the existing u8 type axis; conversions, constant aggregate initialization and old-value postfix semantics now agree with host cc. b_boolconv covers typedefs, float/int/pointer truth, argument/return, arrays/members and updates. Native O0/O1/O2 and Python fat arm64/x86_64 pass. Full gate restarts on the changed tree; earlier 27 results are not reused.

Bool fix acceptance: final unchanged-tree rolling queue /tmp/unisacc-bool-final,
114/114 passed, zero failed/pending, jobs=2, every scheduling window <=55 s
and each child bounded <=60 s. Longest suite exec-multi-ua: 49.655 s.
Product C99 57/57; difftest_o 393/393; fat 132/132 (arm64 and Rosetta
x86_64 executed); fixed model chain 107/107. New b_boolconv agrees with host
cc in native and rebuilt .com at O0/O1/O2; Python fat probe also agrees.
.com 1,354,144 B, SHA256
53556829bb402fe5ab7e2abbb4ee0a9103af71c538d5d67800b7b22ba6467bd4.
No push/release/default-route switch. Model _Bool migration remains pending;
model C99 remains 47/57, not the product's 57/57. This closes the measured
reference conversion defects, not all C99 conformance obligations.

Model bool slice: reserve descriptor 66 between f32 and function-pointer;
map its arithmetic axis to existing u8, its size to one byte. A shared
source-kind-aware TO.b comparison handles integer/pointer and f32/f64;
route declaration, aggregate, assignment, cast, return and parameter
conversion through it. Prefix/postfix updates normalize and preserve the
old postfix value. No executor primitive; validate fixed keeps before
adding bool probes, then actual network compiler against host cc.

Model bool checks: old E3 254 plus C99/13, C99/14 and b_boolconv all equal;
old network chain 107/107 remains green, three additions independently pass
network inference. Keeps raised only afterwards to 257/110. No executor
primitive. Float unary negation now flips the sign bit (needed for -0.0);
boolean floating compound updates use the existing arithmetic tape ops and
normalize their result. Self-source tape 3942700 B identical. Two-slot queue
chain/self/location all pass, 11.03 s wall. Descriptor size and arithmetic
reuse tyinfo u8; boolean conversion remains an explicit semantic rule.

Bool candidate /tmp/unisacc-bool-candidate/unisacc-next.com:
SHA256 6e535da2ca834f1dc7a826d7a5d538787d7ec133e571031e9f793e33c2dfd706,
5,877,754 B. Actual compiler C99 50/57, wrong 0, refused 7, rc 1 against
unchanged baseline 57. Unary floating negation also enables 54_math_c99.
Five probes (13,14,b_boolconv,54,s59) at O0/O1/O2 equal host cc in actual
network-compiler execution. s59 adds f32 conversion/compound update,
negative zero, minimum f32 subnormal, pointer stride and sizeof bool/array.
Math and s59 also independently pass the network source chain; fixed sets
become E3 259, chain 112. Current delta: 4,835 states, 533,503 B network,
1,247,174 finite observations match table including action/string identity.
This is added behavior, not code reduction (gen2 +73/-19, plus shared bool
procedures and static/unit hooks). No executor action, product switch,
release or push. Remaining C99 refusals: VLA, VLA parameter, flexible
array member, static array parameter, scalar/array compound literal,
atexit/div/labs aggregate-return case. Full reconstruction remains open.

### 模型数组形参接入（进行中）

在布尔切片后的模型 C99 50/57 基线上，下一步接入一维数组形参的指针调整。先支持空界、单个整数或标识符界及 static/const/restrict/volatile 修饰；不跳过任意表达式，带副作用的界与多维形参仍明确拒绝，避免复制参考前端忽略界求值的行为。复用现有 DECL、PDB 与指针寻址，不新增执行器动作；固定旧覆盖后再增加用例。

数组形参实现账：旧 E3 259 全保留，另两个 C99 用例 tape 相等；s60 覆盖 int/char/_Bool/double、static 与 const 界，三项经真实网络链路相等后加入固定清单（E3 262、chain 115）。复用参数描述符与 DECL，gen2 净增 10 行，无新执行器原语。候选 `/tmp/unisacc-arrayparam-candidate/unisacc-next.com` 为 5,880,954 B，SHA256 `d20f59d47b3b570b0898ccd39ae846b79985accb6f4fb46bac2ef24ee5c577b6`；三项在 -O0/-O1/-O2 与 host cc 同输出。原 C99 清单实跑 52/57、wrong 0、refused 5、rc 1，未降低 57 的门槛。解析网络 4,850 状态、534,393 B，1,251,044 个观测 network=table。

固定清单队列 `/tmp/unisacc-arrayparam-final` 双槽：chain 115/115（11.83 s）、self 3,942,700 B 相等（3.39 s）、unitlocations（3.43 s），3/3 通过，窗口 11.87 s。实际候选对 `a[n++]` 与 `a[2][3]` 均 rc 1 拒绝；前者在带位置诊断模式报 expected ]，不声称已实现 VLA 界表达式求值。下一项仍为柔性数组成员、自动 VLA、复合字面量及聚合返回局部初始化；本轮不改出货 `.com`。

### 柔性数组成员迁移（进行中）

复用成员布局表，MAR=-1 表示柔性数组、MSZ=0、MFLAT=0，保留元素对齐与类型；成员存在性不能仅看 MSZ。仅接入结构体最后一个、且前有命名成员的柔性数组，沿用数组到指针的访问路径。不新增执行器原语，先验证旧 E3 清单与真实候选运行，再登记覆盖。

柔性数组扩展探针发现参考产品缺陷：`struct P { int count; int *tail[]; }; p->tail[0]=&n` 的成员路径先 load64 再下标，`/tmp/ua_ref` 实跑 rc139。根因是 mbwidth==0 的数组退化错误地限定 mbptr==0，且 mbelem 已变为数组元素宽度8、丢失最终 pointee 宽度。先修产品：独立保存成员基类型宽度，数组成员增加一层指针深度且不提前加载，覆盖固定与柔性指针数组；不复制错误到模型。

产品修复后，固定/柔性指针数组与 s61（char/double/指针/struct 元素）在 host cc 和原生参考 -O0/-O1/-O2 同输出，三项网络链路与修复参考 tape 相同。旧262项在修产品前已通过；现冻结源码，完整 --com 队列验证后才结案。新增三项固定清单：E3 265、chain 118。

### 5.9 附记：getdents/execve 在 Windows 上暴露了一个既有的死数据缺陷（2026-09-27，后台 opus 代理在独立 worktree 里发现，未合并主树）

按 §5.9 的清单，第 1、2 项（`__execve`、`__getdents64`）已经在一个独立 git worktree（`.claude/worktrees/agent-a25e85490a837aeb3`，分支 `worktree-agent-a25e85490a837aeb3`，未合并、未 push、分叉点在 `3359cc5`，**已落后主分支约 40 个提交**，含本节前面的数组形参、柔性数组成员等工作，合并前需要重新对齐）里做出来并验证：Linux/macOS 四个目标 `-run`/`-O0`/`-O2` 均与 host cc 逐字节一致（目录列举内容相同，`execve` 真实执行 `/bin/echo hello` 输出 `hello`）；lnx/x86_64 因为本机 Lima 虚拟机停着没测。

**过程中发现一个真实缺陷，与这次改动本身无关，是既有数据**：`weights/gold/abi.tsv` 里 `clone`/`execve` 的 `win/x86_64`、`win/arm64` 行，`winimp` 字段是字面量 `none`——但 `winimp` 词表本身把字符串 `"none"` 登记为**合法词表项、下标 0**（`#head winimp - none ExitProcess WriteFile ...`），所以 `back_encode.c` 的 `bk_impof` 会正常"查到"它，返回下标 0，代码生成器就会去调 IAT 里下标 0 的导入槎——不对应任何真实 Windows API，运行时行为未定义。这两行数据在这次改动之前是**死数据**：没有任何前端内建能触达 `clone`/`execve`，所以从未被真正编译过。现在 `__execve` 接上前端后，这条路径**变得可达**：`-b win/x86_64` 编译一个用 `__execve` 的程序，编译器返回码 0（接受），没有拒绝，运行时结果未验证（没有 Windows 虚拟机手边）。这正是项目契约最忌讳的一类：**接受了输入却不能正确匹配参考行为，必须拒绝，不能悄悄接受**。`getdents` 的 win 行是这次新加的，抄了 `clone`/`execve` 的既有模式，所以带着同一个问题一起加了进去。

**已安排修复**（同一个后台代理续做，在同一个 worktree 里）：在 lowering 阶段加一道通用检查——`gate` 为 `winapi` 但 `winimp` 恰好是字面量 `"none"` 时一律拒绝（不止 `execve`/`getdents`/`clone` 三个，做成对任何 op 都生效的防御），C 端与 Python 端都要改、行为要一致。结果记入本节下一次更新，或由 cdx 接手核对。

**这一条本身给 FX-4"规格优先"提供了一个具体例证**：这个缺陷之所以潜伏了这么久没被发现，正是因为 `abi.tsv` 里的占位行本身看不出"哪些是真实现、哪些是占位"——`winimp=none` 既可能表示"这个目标真的不需要导入"，也可能表示"没人填"，两种语义共用一个值。分层覆盖或规则化规格（FX-4）如果要求"占位"必须显式区别于"真的是 none"，这类缺陷会在构造期就被查出来，不必等到有真实调用路径才暴露。

门禁来源事故：cc 会话确认其隔离 worktree 后台测试在15:59/16:05覆盖共享 `/tmp/ua_ref.c`/二进制。旧队列 `/tmp/unisacc-flex-final` 的70项记录含 difftest_o 旧版缺陷12错，不能当本轮完整验收；不篡改旧结果。改用本树独占 `/tmp/unisacc-flex-private`（SHA256 3f9a74fcf2118cfcca8cefa51a5ea4e2ac3821d53ef21131b4dd51e7ed3ddd9b），unitlocationcheck 尊重 UA。exec-driver-core 单独排队（此前负载超时53s、单独50.68s），其他113项双槽；两份清单的并集必须恰好等于114项。

队列效率修正：私有参考下 exec-driver-core 单独仍在53s预算处超时。源码检查确认 compilercheck.py 的 core 分支不读取 driver.com，仅 resources 分支使用；compilercheck.sh 现在仅 all/resources 打包 APE，删除 core 的无用准备，保留全部 core 断言及 resources 的包执行测试。

柔性数组收尾证据：新队列 `/tmp/unisacc-flex-isolated` 113/113 全绿，加独立 `/tmp/unisacc-flex-isolated-core2` 的 core=0，程序核对两份请求/结果并集恰好等于 `gate --com` 114 项，无缺无重；私有 UA 哈希始终为 3f9a74fc...。最大单项52.906s，所有调度窗口小于55s。difftest_o396/396、fat133/133、产品C9957/57、chain118/118；E3固定265项用私有token dumper/参考在C表执行器下全部相同。共享污染的旧队列不计入此结论。

产品 `.com` 已重建：1,355,008 B，SHA256 `b3ef68bc2e2e9941309ea394ca7b81138ebebd3cfffe95760ac736b51d61dc53`。独立模型候选 `/tmp/unisacc-flex-candidate/unisacc-next.com`：5,884,843 B，SHA256 `05dce8d8bbd154c46e7b75fe85850910e2c84955ad6e5689a1e3ccfd2025e7e4`。候选真实运行：s61与b_memberptrarray在O0/O1/O2均同host cc；union柔性成员、首个柔性成员、非最后柔性成员均rc1无输出。解析网络4,871状态、535,474 B、1,256,462个观测network=table（含动作与字符串）。本轮增加布局分支与产品基类型元数据，不称代码减少。

模型C99现为53/57、wrong0、refused4、rc1，保留原57基线。余项：自动VLA、结构体复合字面量、数组复合字面量、atexit/div/labs聚合返回初始化。没有切换默认产品路线、发布或推送；cc会话的系统调用扩展与Windows winimp=none修复仍在隔离分支，待本轮提交后另审，不能快进覆盖候选树。

### 系统调用隔离分支合入审查（9228faa，暂未合入）

审查发现：新增execve/getdents与目录头文件仅改abi_audit映射，无持久功能回归；需保存真实execve成功/失败、目录多批读取与Windows双目标拒绝、合法WinAPI不受影响的测试。C/Python guard方向正确，但 exec/lower/code.py 的两个abi循环仍只排除非Windows sysno=none，Windows winapi+winimp=none仍被生成；合入必须同步模型拒绝，不能只修参考。新DIR内部fd/pos/len/base/ent/buf字段未用实现保留名，重现既有头文件宏污染风险；需与先定义同名宏的输入核对。共享/tmp输出必须私有化。本轮不快进，不采用分支的unisacc.c覆盖主线成员指针数组修复，后续合入按最终源码重生成。

### 模型结构体表达式初始化（进行中）

剩余C99的div/ldiv用例拒绝于width：局部 `struct S x = expression` 错走标量STOREV，已有结构体赋值COPYSTRUCT未复用。按参考先保存目标地址，再求RHS并调用已有同类型结构体复制；初始列表保留原路径，无新执行器动作。先用私有参考固定265项回归，再测真实模型候选。

结构体表达式初始化固定清单：旧265项与新增5d/s62（带填充字段、函数返回、声明逗号、3字节结构体、独立副本、调用次数）共267项与私有参考tape一致；新增两项网络链路相同后加入chain，现120项。解析网络4,881状态、536,676 B、1,259,042观测与表相同。实现只将目标地址保留与RHS求值接到已有AS.struct/COPYSTRUCT，不新增动作或复制算法。

结构体表达式初始化收尾：双槽队列 `/tmp/unisacc-structinit-final` chain120/120（12.33s）、自身3,946,271 B tape相等（3.32s）、unitlocations（3.41s），3/3通过，窗口12.37s。真实模型候选 `/tmp/unisacc-structinit-candidate/unisacc-next.com` 5,887,318 B，SHA256 `50399aa6bf889ea83106ba36a400f327a275fd858c78556e3d6750e3595a0910`；5d/s62三个优化级别均同host cc。原C99清单54/57、wrong0、refused3、rc1，未降57基线；剩余自动VLA与两类复合字面量。本片只改模型生成与固定测试集，未改产品源/出货.com，无推送；全重构仍未完成。

### 产品边界越界复核（进行中）

cc报告switch超过256项静默误编译、-Wall的格式解码超过4096字节栈越界。暂停复合字面量扩展，先用私有临时目录复现产品，再为case写入及所有窄字符串decode调用补容量契约；超限明确诊断，不能继续写。同时检查switch嵌套表边界。参考与token dumper仍用私有路径，污染的共享/tmp/ua_ref不参与。

产品边界修复验收：decode 所有窄字符调用显式传入目标sizeof，逐字节写入前检查；switch case表256项与嵌套16层先检查再写。新增parserbounds普通/.com门禁：256/257/1000项switch，4096/4097/9000字节-Wall格式串，4096/4097字节字符串初始化；边界内通过，超限rc1诊断，8项各通过。我的旧产品复现：9000字节-Wall以信号退出；1000case样例出现错误诊断，未重复声称已复现cc的同一错误值。容量未扩大，宽字符串解码及其他报告缺陷未纳入此结论。

本轮冻结树私有UA `/tmp/unisacc-bounds-private` SHA256 e0c574ddb56036d3ffca04cfc364c612ab07610f46a5c4170e514c994e503cc1。完整门禁：双槽 `/tmp/unisacc-bounds-full`115/115，加独占 `/tmp/unisacc-bounds-core`1/1（48.31s）；程序核对请求与结果并集恰好116项、各一次、全rc0，UA哈希未变。每窗口≤55s。产品C9957/57、difftest_o396/396、fat133、chain120；新.com 1,356,272 B，SHA256 38c9ff7782c4607f91ace663fc43693582ca6c104ef7e011c227a1a45cc19c4c。定向首轮com-parserbounds因Python直接exec APE失败，已修为MZ走sh，并在新队列重验；旧失败保留。未发布、未推送。

后续独立审查：cc提供pf_dryrun的-Wall标签编号漂移复现，指出nlab/frameoff/framemax/curcall未回滚。当前仅确认报告，下一片需核实完整状态副作用，不能以只保存四项就宣称无遗漏；重复case诊断、系统调用隔离分支仍未合入。

### 格式警告单次解析（进行中）

审查pf_dryrun：表达式试走可修改标签、帧、类型/符号注册等状态，补四项回滚不足以覆盖。改为实际参数解析后读取格式中的对应转换并检查已有类型，不再为警告调用expr；普通printf与未声明fallback两条路径均接入。模型警告路径同步删除试走，保留既有诊断分类。用-Wall前后tape一致、嵌套调用/临时对象/匿名类型及warning对照验证，独立记录诊断顺序变化。

格式警告收尾：C删除pf_dryrun，pf_argcheck仅解码/扫描格式，实际参数expr后检查已有类型；fallback也在真实解析处检查。模型WF.entry只准备格式，WF.argcheck读取已有vt/vb，无EXPR试走、OCUT或池游标回滚；通用执行器未改。原警告分类保留，嵌套警告按真实解析顺序输出，不再重复试走。代码量不是减少：front_parse +23/-22；formatwarnings +29/-24，gen2 +17/-3。

新增formatonce（普通与.com）：标签分支、结构体临时量、复合字面量+匿名struct、嵌套printf、sizeof内enum，五类×O0/O1/O2，-Wall前后tape完全相同，实际输出同host cc。旧f004934私有参考运行该测试以1失败（labels/O0/-Wall changed tape），证明回归能抓住旧缺陷。最初argc用例因参考-run与原生argc不同而失败，已改volatile输入，不把它算产品缺陷。模型格式用例36→39，全部tape与完整诊断一致；所有警告检查额外要求quiet与-Wall tape相同。

冻结完整门禁：`/tmp/unisacc-format-full`117项双槽，加`/tmp/unisacc-format-core`独占1项47.87s，核对并集恰好118项各一次、全rc0。各窗口≤55s，最大单项51.15s。C9957/57、difftest_o396/396、fat133、chain120；私有UA SHA256 313bfba3b8f099ebc7765a67a9aa389153473cf38af857ecd39ae9702922e8b9。新.com 1,356,064 B，SHA256 fe72c95fa7392f69ff4dc9479f77960b8dd9ecb46292e3a6a4b99005abfee380。无推送/发布/默认模型切换。模型C99剩余VLA与复合字面量仍未闭合；继续迁移，不以本次警告修复作为重构完成。

### 模型复合字面量接入（进行中）

先接入函数内复合字面量，类型与空间取现有ELSZ/TYPECOUNT，匿名对象复用INITLIST的显式元数据入口；未知界数组复用INITCOUNT。结构体声明处参考会直接初始化目标，需复用同一初始化遍历，不能额外生成临时对象后复制。文件作用域与多维类型先保持明确拒绝，后续继续，不能据两项C99用例通过称完整语义完成。

复合字面量接入时发现参考缺陷（私有参考12be67c，非共享/tmp/ua_ref）：`(double){3.0}==3.0`，host cc退出1、参考退出0；`*(int *){&x}`及`*((int *[]){&x})[0]`（x=3），host cc退出3、参考SIGSEGV。参考initaggr的宽度/浮点初始化元数据及返回类型需整体修复，不能复制错误输出。此片先在模型明确拒绝指针元素与浮点元素的复合字面量并保存探针；这两类仍是重构待办，不声称复合字面量已完整。

本片实测：旧E3固定267项先通过，再加入C99的23/24与s63/s64，固定清单271；chain由120增124，124/124网络推理转储相同。匿名对象复用INITVALUE（INITLIST仅负责从符号取元数据）；声明中的结构体复合字面量用只读token识别进入同一遍历，不试解析表达式。s63/s64覆盖显式/推断长度、零补齐、设计符、嵌套标量与结构体、结构体数组、不同对象、递增只求值一次。原有地址获取、标量字面量赋值/后缀修改、文件作用域、多维匿名类型仍未宣称覆盖。
实际模型候选`/tmp/unisacc-compound-candidate/unisacc-next.com`：5,902,565 B，SHA256 441226e99997d39ba5ed3d23d976a62bc28abf1f5757434d9e63c07c2e31bac7；四个新增程序在O0/O1/O2的运行输出与host cc相同。C99为56/57、wrong0、refused1（19_vla），套件按原57基线仍退出1，未降低门槛。双槽队列`/tmp/unisacc-compound-gate`四项全绿，窗口14.31s：chain12.95s、自身3,950,564 B转储3.38s、parseloc7.22s、unitlocations3.49s。plain delta4964状态、1,276,743项；gen2 +46/-4，初始化模块+2/-1，规则增加而非代码减少，无新执行器原语。产品源码与出货.com未改，未推送、未切默认路线。下一项修复指针/浮点复合字面量参考语义，再推进剩余VLA与未覆盖形式；重构目标保持未完成。

### 复合字面量类型修复（进行中）
根因核对：cplitexpr把指针基类型的cw作为存储宽度，未设置initflt，且初始化表达式会覆盖declptr等全局类型状态；返回值又未完整建立curpd/curbase/curuns/curflt。修复按声明类型保存元数据，区分对象存储宽度与指针指向类型；初始化前明确给出转换类型，结束后重建结果描述，不按最后一个初始化表达式猜类型。保留聚合遍历与后缀逻辑。

本轮修复实测：tests/c/b_compoundkind.c覆盖指针/指针数组/二级指针、结构体指针、double/float、unsigned char、_Bool、double数组、嵌套结构体；host cc、私有C参考及重建.com的O0/O1/O2、Python前端均输出`3 3 3 7 / 1 1 1 1 1 4`。另九个小探针（含标量赋值/后缀、sizeof、结构体数组）在C参考三档优化与cc退出值一致。Python前端已正确，未改。模型解除此前指针/浮点字面量拒绝，三例转储一致；旧271项先通过后固定E3扩274、网络chain扩127且127/127通过。综合b_compoundkind模型仍在double operand处明确未覆盖，不算全模型通过。
完整冻结门禁：`/tmp/unisacc-cpkind-full`117项双槽与`/tmp/unisacc-cpkind-core`独占1项48.42s，并集对`gate.sh --list --com`精确118项各一次、全rc0，最大51.302s。两项曾因窗口剩余预算不足延期，后在新窗口通过，不把中止记pass。产品C9957/57，difftest_o399/399，fat134，模型自身转储3,954,383 B一致。私有UA `/tmp/unisacc-cpkind-private` SHA256 a029f11947b802dbb8eebeecbc83e7340f22205e790e39eb7075c01513acc99f；新.com 1,357,120 B，SHA256 c7e8543851c5a7258b7cb8c420c38cc5f817dec053838cdb9cf51841d14baa2f。未推送、未发布、未切默认模型；重构继续，VLA及其余模型边界仍待完成。

### 局部VLA模型接入（进行中）
复用EXPR计算运行时长度、ELSZ取元素宽度、现有指针访问路径；增加每符号字节数槽与每作用域栈快照，普通出块及break/continue按参考恢复。sizeof名字读取保存的字节数，不重新求值长度。常量/动态边界先按参考isconstdim的token判别（不试解析后回滚）。scope undo记录由15扩16并同步unused warning步长，新增元数据不塞进数组维度。无新执行器原语；多维VLA等参考未支持形态保持明确拒绝。

VLA元素类型审计：double/struct/enum边界转储一致，指针VLA发现参考symptrd漏加数组层，`int *a[n]; ... *a[0]`最终以8字节而非4字节加载int。与本次模型结果不同；修参考而不复制错误。VLA分支需在解析长度表达式前保存声明类型（长度表达式中的cast可能改写decl*），再以元素深度+1注册符号，并恢复基类型等元数据。

本轮进度（尚未提交，完整门禁待续）：实际模型候选`/tmp/unisacc-vla-candidate/unisacc-next.com`运行C99为57/57、wrong0/refused0；19_vla、b_vla、s65三档优化运行输出同cc。旧E3 274项先通过，加入三项VLA和b_vlakind后278项全部转储一致。模型九项队列`/tmp/unisacc-vla-gate`全绿35.08s（在VLA参考类型修复前），不能替代后续产品冻结验收。参考修复后b_vlakind在cc/C三档/Python以及新.com一致；旧330bc34私有参考运行同一回归退出139。产品已重建，完整队列`/tmp/unisacc-vla-full`使用UA=/tmp/unisacc-vlakind-private，首窗口4/117通过、退出75待续；独占exec-driver-core尚未跑。继续原队列，勿编辑输入或重用旧参考门禁结果。

VLA冻结验收完成：`/tmp/unisacc-vla-full`双槽117项与`/tmp/unisacc-vla-core`独占1项（48.93s），并集精确匹配`gate.sh --list --com`的118项，各一次、全部rc0，最长52.944s，无失败或跳过。四个新增VLA用例（19_vla、b_vla、b_vlakind、s65）实际模型编译器的-Wall退出码、转储与诊断全部同私有参考。chain固定131项通过，E3固定278项通过；模型自身转储3,956,608 B相同。产品.com为1,358,048 B、SHA256 a56523e877d215a7d5ccdfe5bc21e6d793c4d0174de1acde3b711aed9c14f564；私有UA SHA256 757002812b7840aee7335296c4cd7a30bcb18247f64d3cbad8a754b3d3296021。实际模型候选5,913,317 B、SHA256 387cb14e83dbb5da62bb650e8bf9d1f11dbf90ea00498256a7377101ee889ee7。C99的57个探针全过不等于完整C99或重构完成；多维VLA、其余模型未覆盖形式仍单列，未切默认路线、未发布或推送。

### 浮点二元公共类型迁移（进行中）
下一片移除OPX对float的整类拒绝，并替换double路径里写死的signed cvtid：按type.tsv的ck/res行选择公共浮点类型与合法性，复用TO.d/TO.s转换，irsel供给操作码与反向比较标记。指针与整数路径保持回归；新增f32/f64、混合整数与u64的独立运行证据。赋值/复合赋值等未迁移分支不在此片偷改，执行器不增加语言原语。

浮点二元片实测：type.tsv的ck/res选择f32/f64公共类型与合法性，TO.d/TO.s共享转换，irsel提供运算码及_rev。移除了NOFLT拒绝与写死signed cvtid、手写64位opcode清单；gen2净+2行（+25/-23），无执行器改动。发现并修复旧模型接受错误：u=9223372036854775808UL与double 2.25的u>d、d<u，上一候选输出0 0，本片及host cc输出1 1；s66_float_binary已固定覆盖。20种有浮点操作数的类型配对×10运算转储同私有参考；20程序加s66在实际网络候选O0/O1/O2共63次输出同host cc。旧278项先过，再加s66，E3固定279；网络chain固定132/132。
定向双槽队列`/tmp/unisacc-fpbin-gate`12/12全rc0，两窗口51.95/10.63s，最长43.012s：source-to-ELF、chain、自身转储、位置/错误/警告检查；自身3,956,608 B相同，3.46s。实际候选C9957/57、wrong0/refused0。plain delta5085状态、1,307,897项、28,371,917 B；候选.com 5,944,703 B，SHA256 568518181e26c058c7d27790b97989b5ec53de25c53a7707893cbf991ddde2e3。产品源码和出货.com未改，本轮不是新的118项全门禁。综合b_float/b_compoundkind仍有初始化等double operand拒绝，b_fconv仍有expression拒绝，继续迁移；未推送/发布/切默认。

### 初始化/赋值共用转换（进行中）
把SAMEDBL的局部double特判替换为ASSIGNCV：目标类型选择已有TO.d/s/i/u或BOOLCV，源描述与目标描述分别保留；局部标量初始化、聚合元素、全局值与普通赋值复用同一入口。不改执行器；逐项对照参考转储和host cc运行，旧固定清单先验收再扩充。

参考缺陷实证（私有UA为9da574d产品状态）：unsigned long数组、结构体成员、复合字面量由9223372036854779904.0初始化，cc输出1 1 1，参考输出0 0 0。initflt/slotflt只携带浮点与bool，漏掉u64槽位的转换kind。统一从宽度/指针/unsigned/float/bool生成kind，覆盖成员、数组及匿名对象；不把参考错误复制进模型。需产品回归、重建.com与完整冻结门禁。

初始化/赋值片完成验收：ASSIGNCV供局部标量、聚合槽、静态标量、全局值与普通赋值共用TO转换；删除ISDV及静态f32拒绝等重复分支，模型相关源码净-18行。产品valuekind供dkind和数组/成员/复合字面量槽共用，修复unsigned long大浮点初始化；新增b_aggrunsigned的9个宽无符号比较全为1，控制输出3 4 5 1，cc/C三档/Python/新.com一致。s67另覆盖f32/f64/int/u64/bool的声明、普通赋值、聚合与静态初始化；三个新增用例在实际模型三档优化9次输出同cc。旧279项先过，再纳入三项固定E3=282，网络chain=135全过。
冻结门禁：`/tmp/unisacc-assigncv-full`117项双槽+`/tmp/unisacc-assigncv-core`独占1项49.95s，精确并集118项各一次、全rc0，最大52.62s。difftest-2因窗口余6s延期，完整预算15.87s通过，延期不记pass。产品与实际模型的C9957/57，wrong0/refused0；difftest_o405，fat136；自身转储3,958,707 B一致。产品.com 1,358,336 B，SHA256 38f13433a6ab2bd3f3ebb2915bb527e657d34f1cc4fee66f05d2ae976e1a1e3d；私有UA SHA256 1fc4fdc26a4c881b44b38cb5e028983e33c9d86c25104c1b292d201fef4ff4b1。模型候选5,942,919 B，SHA256 552604d99d6baf6a3017a5aa6bc57732f6d812facbdeff2847ae6870460994e0；plain delta5054状态、28,200,821 B。未推送/发布/切默认。
后续返回转换缺口已实测：`double f(void){return 3;}`再打印(int)f()，私有产品输出3、当前模型候选输出0。S.re1只改目标描述并NARROW，没有转换返回表达式；这是既有模型缺陷，不是已修复的初始化问题。下一片让返回复用同一转换入口并保存回归，不能以本轮固定清单全绿声称全域正确。系统调用/procview隔离分支继续未合并。

### 返回值转换复用（进行中）
返回标量先保存表达式来源类型，再以声明返回类型调用ASSIGNCV；只有整数窄化继续NARROW，float/double/bool和指针不重复窄化。沿用结构体返回拷贝。不改产品源码，先验证既有282项，再以真实网络运行检查混合返回类型。按主人要求后续不再向cc-unisacc发协作消息，自主推进。

返回转换片验收完成：复用ASSIGNCV，删除返回路径的局部float/bool特判，gen2净-5行（+5/-10），无执行器或产品源码改动。s68覆盖int→double、double→float/int/u64/uchar/bool、u64→float/double、float→short。最小double f(){return 3;}旧模型输出0，新模型与cc输出3；两程序三档优化共6次真实模型运行同cc。旧282项先通过，再纳入s68，固定E3=283；网络chain=136/136。
定向双槽队列`/tmp/unisacc-returncv-gate`12/12全rc0，窗口50.97/12.97s，最长43.423s；自身3,958,707 B转储相同。实际模型C9957/57、wrong0/refused0。plain delta5045状态、1,297,615项、28,151,060 B；候选`/tmp/unisacc-returncv-candidate/unisacc-next.com`5,942,411 B，SHA256 01a562a374cfd8c49cafabe718cc647d39ab7a1920b892d3d7b18e546327164b。出货.com仍为ef17e27的产品构建，本片未重跑118项产品门禁；未发布、推送或切换默认路线，剩余转换与覆盖缺口继续推进。

### 十进制u64字面量边界（进行中）
复测b_fconv仍拒绝expression；逐句缩小至首行18446744073709551615UL。共享token读取器NUMD只接受19位，U.ulong已有正确的有符号位模式输出。扩至20位前逐位检查UINT64_MAX，不能只放宽长度令累加回绕；复用现有64位比较动作，不增执行器原语。浮点先由floatconst分类，仍走原精确转换。

十进制u64片实测：NUMD每次乘10前用UINT64_MAX与当前digit计算阈值，C64U拒绝溢出，长度放宽至20位；U.ulong原有NUMOUT打印位模式复用。旧283项先过，再固定b_fconv与s69，E3=285全同，网络chain=138全同。此前b_fconv拒绝的首因就是20位字面量；修复后完整转储同参考，两新增程序实际模型O0/O1/O2六次运行输出同cc。共享数值门禁增加6个整数精确位值、3个溢出token拒绝，table/net两路径都检查tk，避免只看回绕后的nv；原61个浮点位值及10个坏浮点输入保持通过。
双槽定向队列`/tmp/unisacc-u64literal-gate`6/6全rc0，窗口42.90s；source-to-ELF42.78s，自身3,958,707 B一致。plain delta5065状态、1,302,755项、28,248,181 B；候选5,944,748 B，SHA256 cce79501fbb67e55f78760828b5e5b9b62fb4c23ceda51e3df79b90f9715b285。模型共享reader净+4行；产品源码与出货.com未改，不宣称本片重跑完整产品门禁。b_float仍在混合?:类型处未覆盖，下一片审计公共类型及两分支转换；未推送/发布/切默认，整体重构未完成。

### 条件表达式公共浮点类型（进行中）
参考cond在两臂浮点kind不同后回到首臂重发，分别fconv到type.tsv公共类型；模型目前QT.int把f64结果送入仅整数的RESD而拒绝。采用相同的token/output重发边界，保留参考标签分配顺序，复用TO.d/TO.s；两臂运行时仍只执行一臂。嵌套与带副作用表达式需独立检查，不仅比较简单字节。

条件表达式浮点片验收：按type.tsv公共类型重发两臂并复用TO.d/s，结束明确设置公共结果描述；首次探针发现漏设结果类型导致多发cvtid，已修。s70覆盖double/int、float/int、float/double、u64/double、嵌套与函数副作用；实际网络编译器O0/O1/O2均同host cc，计数证明每次只执行选中的一臂。旧285项先过，再固定s70，E3=286全同、chain=139全同。保留参考的重发与标签分配顺序，不宣称消除了手写条件语法规则；gen2净+12行，无执行器原语新增。
双槽队列`/tmp/unisacc-qt-gate`12/12全rc0，窗口49.85/12.83s，最长42.42s；自身3,958,707 B转储相同。plain delta5105状态、1,313,057项、28,464,304 B；实际候选5,950,546 B，SHA256 f1d0a4c96a770e912c189e0aeaee1ee99cfbe05ea0aef5c8e3e4cd6b02409395。b_float已越过条件表达式，仍因浮点复合赋值/递增未覆盖而拒绝；下一片复用OPX与ASSIGNCV处理该边界。产品源码及出货.com未改，本片为定向模型门禁，不称完整产品验收；重构继续，未推送/发布/切默认。

### 浮点复合赋值复用（进行中）
LV.c先允许标量float/double的单位步长，指针仍由STEPTY决定。浮点任一操作数经TAX识别后，保存赋值目标、调用现有OPX处理公共类型与运算，再ASSIGNCV回目标并STOREV；移除仅服务bool目标的重复浮点opcode/转换序列。整数与指针原分支先保持回归，前后缀递增另验，不顺带声称完成。

浮点复合赋值验收：任一浮点操作数经TAX分派后，复用OPX公共类型/irsel运算和ASSIGNCV回目标；删除bool专用浮点opcode及signed转换序列。数组元素和成员复合赋值接到既有LV.c，地址不重复求值。gen2净-9行（+11/-20），状态5105→5072，无新执行器原语。s71覆盖double/float/int/u64/bool、四则复合赋值、成员、n++下标与RHS调用；实际模型三档输出均同cc：8 12 1 1 1 6 1 1 / 6，下标与调用各一次。旧286项先通过，新固定E3=287全同，chain=140全同。
双槽队列`/tmp/unisacc-fpcas-gate`12/12全rc0，窗口49.52/12.82s，最长42.07s；自身3,958,707 B转储一致。plain delta5072状态、1,304,610项、28,209,028 B；候选5,945,596 B，SHA256 6cdf379776cbe3ab89502a7cc1bf922bea85071b221d7d1a9a4a5629113d520c。b_float仍在浮点递增处未覆盖，未称完整通过。产品源码和出货.com未改，定向模型门禁不等于完整产品门禁；下一片处理前后缀浮点更新，重构继续，未推送/发布/切默认。

### 浮点前后缀更新（进行中）
前后缀复用CSTEP处理目标类别，新增共享浮点一步更新（IEEE 1.0位模式，irsel提供add/sub编码）。后缀保存并返回旧位模式，不能以反向运算恢复，因为舍入不可逆；地址只求值一次。前缀返回新值；整数/bool/指针继续既有路径。

浮点前后缀片验收：CSTEP复用目标分类；FPSTEP共享IEEE 1.0位模式与irsel加减opcode，前缀返回新值，后缀保存原位模式，不做逆运算恢复。s72覆盖float/double、前后缀加减、2^24/2^53加一舍入、数组n++和成员；s72与完整b_float在实际网络编译器O0/O1/O2共六次输出同cc。旧287项先过，新固定E3=289全同，网络chain=142全同。gen2净+9行（+17/-8），无新执行器原语。
双槽队列`/tmp/unisacc-fpstep-gate`12/12全rc0，窗口50.14/12.87s，最长42.59s；自身3,958,707 B转储一致。plain delta5104状态、1,312,844项、28,403,652 B；实际候选5,953,878 B，SHA256 07ee7ecdeb6d3b08e38dd5905c636388e07a723e887ed5d798b82bb5488c409f。产品源码和出货.com未改；本片仍是定向模型验证，不宣称新的跨平台运行或全产品门禁。浮点综合用例闭合后重新盘点S-17的剩余覆盖、CLI/错误契约、体积与默认切换阻挡，不以固定清单全绿替代重构完成。未推送/发布/切默认。

### 覆盖盘点与真实应用阻挡（进行中）
5c875eb候选在tests/c、tests/c99、examples及apps、parse2/probes共296份源文件上，以私有参考和双槽12秒子进程界限盘点：273同转储、22未覆盖、1参考拒绝、0 DIFF/工具失败。此清单不同于固定289，不能直接比较分母。结果存/tmp/unisacc-frontier-5c875eb/results.json。两个真实应用exeinfo/wordfreq在带位置的实际网络编译器中均定位到`(unsigned char)*p++`；UD/U标识符路径未接后缀更新。新增共享命名左值更新入口，复用LOOKUP/地址/CSTEP/POST，不另写更新算法。

新产品缺陷实证：struct P{char x;};struct P *v[4];long guard=77，写v[1]=&obj后旧私有参考输出guard地址值而非77、退出1。全局数组.bss分配仅看gstruct而不看gpd，1字节结构体使数组仅4B，指针实际需32B；24字节结构体反而多分配。sizeof已正确，错误在存储分配。修正isarr优先使用元素w（pointer已设8），仅非指针结构体用declsz。不是为字节对齐复制错误；需新的私有参考与完整产品门禁。

本片冻结前实测（尚未提交）：命名后缀更新复用POST后，wordfreq/exeinfo及b_globalstructptr三份转储均同新私有参考；旧289项先通过，清单扩E3=292/chain=145待完整门禁。三份程序在实际模型O0/O1/O2九次输出同cc，Python及重建产品O2的b_globalstructptr输出77 32 24 1。产品.com 1,358,320 B，SHA256 b0db8800a56a7c13c4e952eb02266b7e72d2fe7b3bf14efbd8da4ed11de4c9bb；私有UA=/tmp/unisacc-postoperand-private，SHA256 b20f125ac70101d966d6e5e83fc21e030b0e9d5de9ffbb93bca83b1294958e44。实际模型/tmp/unisacc-postoperand-candidate/unisacc-next.com为5,958,288 B。完整冻结队列使用/tmp/unisacc-postoperand-full，exec-driver-core另独占；所有步骤完成前不提交、不编辑冻结输入，不把局部九次运行称全门禁。

后缀操作数/全局结构体指针数组片冻结验收完成：`/tmp/unisacc-postoperand-full`117项全部执行，其中exec-multi-ua并发53.07s超时（142，保留记录）；`/tmp/unisacc-postoperand-multi`独占48.69s通过；`/tmp/unisacc-postoperand-core`独占50.68s通过。按gate.sh --list --com核对，使用明确记录的独占复跑结果替代超时判定后，并集精确118个不同套件全rc0，最长52.942s；不将原超时改写为pass。C9957/57，difftest_o408，fat137双架构实际运行；自身转储3,958,904 B一致；E3固定292全同、网络chain145全同。模型候选SHA256 5057c049b5303942164c79a8f66b8a9ed55a7e460d29c95a710c4a899ae7ec8e。gen2净+4行，复用既有后缀更新，无新执行器原语；产品只修正全局数组分配条件并重新生成unisacc.c。当前只据本地门禁范围报告，未新跑Linux/Windows VM原生套件。未推送/发布/切默认，剩余模型覆盖与整体切换继续。

### sizeof成员对象复用（进行中）
覆盖盘点中b_arr1memb/b_structarrmember停在sizeof成员数组：普通表达式路径已衰减成指针，无法再由vt/vb恢复完整对象宽度。提取MB.INFO共用成员声明元数据读取，sizeof直接沿命名成员链取得MSZ（不执行地址/载入），普通成员访问仍用同一读取入口；未匹配的表达式退回原路径，不把残留marr当通用类型事实。


sizeof成员片验收：复用MB.INFO读取成员声明元数据，新增命名成员链的sizeof路径；数组成员保留MSZ对象宽度，后续箭头访问仍按数组衰减处理。未增加执行器原语，gen2净增19行。原292项先通过，新增b_arr1memb、b_structarrmember和s73_sizeof_members后固定295项全部逐字节相同；chain固定148项全部通过。实际模型候选在三个用例、三个优化级别上共9次执行与宿主cc相同。相关12项门禁两槽排队，两个窗口50.67/12.97秒，全通过；源码到ELF43.13秒，E3自身源码3,958,904字节相同。候选5,961,195 B，sha256 f4d1a1639b4ca9d5b1d6ff24bac520463a28abb2c5318162e92cbdb73f73ddb1；仍在隔离目录，不替换出货产物。模型5171状态、1,330,083条目、JSON 28,797,582 B。此片为模型覆盖补齐，不宣称完整C99或S-17完成，产品源码未变，未重复整套产品门禁。


### 整数类型拼写收尾（进行中）
b_short仍停在sizeof(long int)：TSPEC已有long/long long和unsigned short的描述符，但未消费可选int。把这几条尾部接入共用的可选int消费状态；保持类型宽度来源与描述符不变。新增声明、参数、转换和sizeof探针，先检查参考字节与真实执行，再扩大固定清单。

整数类型拼写验收：共用TS.intopt/TS.intend消费short/long/long long尾部可选int，unsigned short/long沿同一路径；类型描述符不变，未增加执行器原语，gen2净增2行。原295项先通过，加入b_short与s74_int_spellings后297项全部参考tape相同，chain150项全部通过。两例在实际模型候选-O0/-O1/-O2共6次执行与宿主cc相同。12项相关门禁两槽队列50.37/13.11秒，全通过，源码到ELF42.40秒。模型5175状态、1,331,112条目、JSON28,822,967 B；候选5,961,557 B，sha256 4353fd82c49d224b70dce8da0460efc07833b81eefc6ca334d81370b038dadc9。产品源码与默认.com未变，未宣称任意声明拼写或完整C99已覆盖。


### unsigned一元负号覆盖（进行中）
剩余b_uzext停在unsigned int一元负号。参考在减法后把u32零扩展，模型此前直接拒绝；复用tyinfo派生的NARU掩码，不复制第二套宽度常量。先核对该文件与固定清单，再由实际模型候选执行。

unsigned负号验收：gen2净增1行，以已有NARU生成u32减法后的零扩展，无新执行器原语。b_uzext全文件与参考tape相同，原297项先通过，加入后298项全通过；chain151项全通过。实际模型候选-O0/-O1/-O2均与宿主cc相同。12项相关门禁两槽队列50.73/12.97秒，全通过，源码到ELF42.65秒。模型5177状态、1,331,627条目、JSON28,832,947 B；候选5,962,191 B，sha256 adc7e716a34776e4ab031726ff8fe9b929d3e3bf8c688e59c1b5d24cb9878d0d。产品源码未变，默认.com未切换。


### 函数指针typedef复用（进行中）
覆盖盘点中的b_declfn/b_anon在函数指针typedef处停止。复用已有FPDECL和类型别名表登记路径，不另写函数指针声明解析；数组别名仍需维度元数据，不在此处误当单指针接收。先跑真实文件确认后续阻挡，再扩固定集。

本片首轮对拍发现b_declfn被接收却索引步长51而非8：FPDECL解析数组参数后FN.pfp未执行数组到指针调整。补齐该描述符层数后全文相同；这是模型缺陷，未照搬错误输出。普通别名和函数指针别名共用TD.put，不重复登记逻辑。原298项已先通过。

函数指针typedef验收：gen2净增6行，复用FPDECL及TD.put，数组参数补一层指针；未增加执行器原语。b_declfn加入固定集后299项参考tape相同，chain152项全通过。实际模型候选-O0/-O1/-O2均与宿主cc一致；12项相关门禁两槽队列50.82/13.14秒，全通过，源码到ELF42.80秒。模型5187状态、1,334,198条目、JSON28,891,184 B；候选5,964,832 B，sha256 46da5c34e1985e34d291ccafe6f62cc817c42c4eec660970d83e2fdde3b3d1b0。b_anon后续停在匿名成员，b_typedef仍停在块内别名表达式，未扩大完成口径；产品源码与默认.com未变。


### 当前覆盖盘点与enum声明（进行中）
当前c9c7594模型对tests/c、tests/c99、examples及apps、parse2/probes共299个文件：283相同、15未覆盖、1参考拒绝、0DIFF/工具失败。此集合不含旧parse/probes，不能和固定299项混为同一计数。结果保存/tmp/unisacc-frontier-c9c7594/results.json。b_init3的全局enum对象被顶层EN误当枚举定义；区分定义与类型使用后转回共用FN/TSPEC，不重写对象初始化路径。

enum对象片验收：区分顶层enum定义和类型使用，后者回到共用FN/TSPEC，gen2净增2行，无新执行器原语。原299项先通过，新增b_init3后固定300项参考tape相同，chain153项全通过。实际模型候选-O0/-O1/-O2与宿主cc一致；12项相关门禁两槽队列50.50/13.00秒，全通过，源码到ELF42.27秒。模型5189状态、1,334,713条目、JSON28,901,904 B；候选5,965,116 B，sha256 b87abd8863e96e387d49da3317e4ca1aadfd50ad3e9f6fcc58865a477adc3481。产品源码与默认.com未变；匿名成员、块内typedef等缺口继续保留。


### 结构标签作用域缺陷（进行中）
当前模型接受内层同名struct定义，却复用外层sid并覆盖SSZ：最小例外层int成员、内层long成员，内层结束后sizeof外层对象参考4、模型8。新增标签绑定撤销栈和作用域编号，结构ID改为单调分配；块退出恢复名字绑定但不改写已分配结构描述。与对象BIND分开，避免标签和普通名字混用命名空间。

结构标签片验收：标签有独立绑定撤销栈，函数体和嵌套块退出恢复STAG/TAGLEVEL；sidserial单调分配，nsid只表示当前结构，避免内层覆盖外层尺寸。支持块内单独结构声明。复用现有64槽成员键布局，63个结构接受、第64个明确structure id capacity拒绝（实际模型运行），不让ID溢入相邻名字。gen2净增13行，未新增执行器原语。原300项先通过，加入b_init4及s75_tag_scope后302项参考tape相同，chain155项全通过；新探针覆盖两层同名标签、块退出后新对象声明、函数间恢复。两例三个优化级别共6次实际模型运行与cc相同。12项相关门禁两槽队列49.80/15.13秒全通过，源码到ELF44.36秒。模型5209状态、1,339,861条目、JSON29,020,759 B；候选5,968,613 B，sha256 8b7b018a29bce35cbfdb58ffc6ad53e7a353dbf0e507b9cbf3b4f30c0fb7e0b3。未宣称完整标签作用域（原型作用域、内层纯前置声明等仍需核对），产品源码及默认.com不变。


### 结构标签容量与参考边界复核（29c727f之后）
实际独立复核两项，不把固定集全绿当完成：
- s76_struct128.c：128个不同结构、访问最后一个成员，宿主cc和私有产品参考均rc0，实际模型候选在第64个明确拒绝。原64槽成员键stride只容纳1..63，这是模型容量缺口；后续应统一成员键编码和全部读写/初始化器，再对齐产品MAXSTRUCT=128，不能只提高TAG.alloc上限。
- s76x_forward_scope.c：外层完整struct S、内层`struct S;`后`sizeof(*p)`。宿主cc以不完整类型诊断拒绝，私有产品参考和实际模型都rc0。src/front_parse.c的stparse只在定义带{时建立深层标签，没有处理内层纯前置声明遮蔽；模型忠实复制了这项错误。属于产品和模型共同的拒绝契约缺口，需同步修复并按产品改动重建/整套门禁，不计为已接受合法输入的相等证据。
两份探针已保留在exec/parse2/probes，不加入equal清单。命令均有15秒子进程超时；没有修改产品、模型或默认.com。下一步先处理成员键容量，再统一前置标签规则；不能称标签作用域已完整闭合。


### 成员键容量对齐（进行中）
统一STRUCT_MAX=128、MEMBER_STRIDE=256，覆盖模型ID1..128；写成员、普通读取和指定初始化器三处共用该步长。成员映射迁到独立1<<40间隔区域，避免扩大步长后越过旧的百万格区域；最大名字ID受输入长度域限制，成员键上界仍小于区域宽度。结构内成员序号表的64步长不是同一维度，本片保持原状。

成员键容量验收：三个成员键构造点统一MEMBER_STRIDE=256，并用A64I/A64算地址；6张成员映射各占独立1<<40区域，128结构上限与产品一致。模型状态/条目数不增（5209/1,339,861），JSON29,020,849 B。原302项先通过，扩展s76覆盖64/65/128号结构的本地及全局指定初始化、读取，加入后303项全相同，chain156项全通过。实际模型3例×3优化级别共9次与cc相同，第129个结构rc1且明确容量诊断。最终64位键版本12项相关门禁两槽队列49.63/15.83秒全通过；较早32位键版本的未完成队列不复用、不作为最终证据。候选5,968,805 B，sha256 fcb6375c77b596ee4834391ff556d02be695d39041ebc730658a401a63caeeb7。产品与默认.com未变；下一项为内层纯前置标签的产品/模型共同修复。


### 内层前置标签同步修复（进行中）
C stparse和Python tag_bind需要把标签后直接分号视为当前作用域声明；普通struct S *p仍向外查找。sizeof不完整结构在类型形式和表达式形式均拒绝；模型通过同一TAG.bind/撤销栈建立内层未完成类型。先做针对性验证，然后重建.com并跑产品门禁。

内层前置标签验收：C stparse对更深块的纯标签声明新建类型，Python tag_bind区分普通向外引用与当前作用域绑定，模型沿TAG.bind建立未完成类型并恢复；sizeof结构类型、表达式、常量表达式的不完整类型均拒绝。tests/tagforward.sh进入参考和.com门禁，三种非法形式×cc/Python/C三优化级别共15项编译拒绝（不运行非法输入）；同套件在实际模型候选也通过。b_tagforward合法完成定义与外层恢复，连同s75共6次实际模型运行与cc相同。原303项先通过，新增后固定304项全相同，chain157项全通过。
完整冻结门禁：/tmp/unisacc-forward-full执行119项，exec-multi-ua首次生成errorparse时ENOSPC失败，原记录保留；清理12个本会话过期候选目录的可重建中间文件（保留.com与日志）释放约2.1GB。该项在/tmp/unisacc-forward-multi独占重跑49.30秒通过；exec-driver-core在/tmp/unisacc-forward-core独占51.36秒通过。三份结果合并后严格核对gate --list --com恰好120项全有通过证据，最长51.629秒。普通队列保持两槽、逐窗口不超过55秒；不将原失败队列称为全绿。C99 57/57，difftest_o 411，fat 138双架构实跑，kernel23 stale0，nativeboot通过，自身E3 3,961,598字节相同。没有据此宣称新做了Linux/Windows虚拟机全量测试。
重建产品unisacc.com 1,359,312 B，sha256 42144a233904acd9a66a994344fd1507bd8aa21ee347d75c41c9b9e4233980ec；私有参考sha256 5accbaa49bd418ac5abe03df2fbbb52dc3973bfe2dec691de47282697ae20ca7。实际模型候选5,969,125 B，sha256 4993d8ae1a8820906572e5fd214ac2ab116391c868434ddfdc0d2085985773d9；模型5213状态、1,340,889条目、JSON29,043,953 B。默认产品仍是参考实现，S-17覆盖缺口仍在，不发布、不宣称重构全部完成。


### 可变参数函数指针迁移（进行中）
参考vcall按声明的variadic标记或实参>6选择栈调用；普通间接调用恰好6实参因r5留给callee而拒绝。模型新增独立函数指针基类FPV保存栈约定，沿现有BASE/typedef/成员/作用域撤销传播；共用已有实参反序，不新增执行器原语。先核对b_varargs2及参数、typedef、遮蔽和嵌套调用，再保留固定集。

首轮发现两项需按实证处理：输出数字辅助过程会改写临时t，因此栈清理长度从na重新计算；参考function前瞻把显式嵌套函数指针参数中的...也计入外层stacked约定，模型按该参考行为记录，不能宣称这是C标准要求。

可变参数函数指针验收：FPV沿既有BASE、typedef与作用域撤销传播，共享ISFP分类和CL.vdone反序路径；普通间接调用>6实参也复用栈路径，恰好6实参以明确callee寄存器限制拒绝。gen2净增17行，未新增执行器原语；warning分类复用ISFP。原304项先通过，加入b_varargs2与s77_fpvar后固定306项全相同；chain159项全通过。s77覆盖别名、显式函数指针参数、块内同名普通指针后恢复、嵌套间接调用和8实参；实际构造网络候选两文件×3优化级别共6次与cc一致，s77x合法C六实参探针由参考和模型均rc1拒绝，保留为产品限制不计equal。
12项相关门禁两槽队列50.79/14.81秒全通过，源码到ELF43.18秒，自身E3 3,961,598 B相同。最终模型JSON29,241,601 B；模型候选5,973,434 B，sha256 8cfc9201ac88999fd24bdbaa1a8ba3252a8c168f90df4bfcf321241976a19a3d。默认产品源码和unisacc.com未变；没有重复全产品门禁或宣称S-17完成。参考对嵌套...的调用约定按实测兼容，不外推到任意函数签名。


### 块内typedef与enum绑定复用（进行中）
b_typedef需要块内类型别名、内嵌枚举定义和块退出恢复。把顶层TD改为可调用声明过程，复用BIND/UNWIND保存类型别名三列；普通对象声明同时遮蔽同名typedef。enum定义并入TSPEC共用路径，枚举常量沿普通名字撤销，枚举标签沿标签撤销；不为每种声明另造一套作用域机制。

块内typedef/enum验收：TD.parse成为顶层与块内共用过程；ENUM由TSPEC调用，删除原顶层专用枚举定义路径。BIND/UNWIND记录从16扩至19列，保存TDN/TDB/TDD，对象声明清除当前typedef可见标记；枚举常量也复用BIND，枚举标签复用扩为4列的TAG撤销记录。gen2净增7行，无新执行器原语。原306项先通过，加入b_typedef和s78_typedef_scope后308项全相同，chain161项全相同。s78覆盖三层类型宽度恢复、变量遮蔽别名、枚举常量/标签遮蔽恢复、块内函数指针typedef；两文件×3优化级别共6次实际构造网络运行与cc相同。
12项相关门禁两槽队列50.54/14.69秒全通过，源码到ELF43.15秒，自身E3 3,961,598 B相同；最终固定集复验与候选构建使用两个独立槽并行、子进程均有超时。模型5251状态、1,350,665条目、JSON29,264,672 B；候选5,973,960 B，sha256 d52314c71e497d004d90fcf433ed56068bfa0e74a3172d6da963187c7f54f5d4。释放两个旧候选可重建中间目录约360MB，保留.com与日志。产品源码、默认.com未变；未宣称任意typedef声明形式或全部C作用域已覆盖。


### 匿名聚合成员迁移（进行中）
参考把匿名结构/联合的成员元数据复制到外层，偏移加匿名对象起点；成员顺序与初始化跳过信息一并保留。模型复用SSZ/SAL、成员映射、SMEM与MFLAT，不另写初始化器。该语法是产品支持的C11扩展，不归为C99必需能力；本轮为已支持产品行为的迁移。

匿名成员验收：匿名结构/联合成员的名字与布局映射提升到外层，偏移加对齐后的起点，MFLAT保留初始化跳过信息；普通成员和提升成员共用SB.memberindex。gen2净增16行，未新增执行器原语。现有每结构64项SMEM布局增加写入前检查；实际模型64成员接受且三优化级别与cc一致，65成员rc1明确structure member capacity，不把这一模型上限称为产品充分上界。
原308项先通过，加入b_anon与s79_anon_layout后固定310项全相同；chain163项全相同。s79覆盖对齐、嵌套匿名成员、union别名、数组成员与全局/局部初始化；两正式文件加64成员边界，三优化级别共9次实际构造网络运行与cc一致。b_init5已越过布局，但块内函数原型仍未覆盖，未加入equal。12项相关门禁两槽队列49.99/15.02秒全通过，源码到ELF44.15秒，自身E3 3,961,598 B相同。
模型5272状态、1,356,064条目、JSON29,395,866 B；候选5,979,116 B，sha256 a93c4ff3bc87740f0d2ce4289ee57de4590d38841d5b6a6f5368474c5d27797a。清理上一片旧候选的可重建中间文件约180MB，保留.com及日志。产品源码与默认.com未变；匿名聚合属于已支持的C11扩展迁移，不修改C99覆盖口径。


### 块内函数原型迁移（进行中）
参考block声明在名字后遇到参数括号时只做平衡扫描，既不分配局部槽，也不登记参数签名；模型复用函数指针参数列表扫描，结束后继续逗号声明或分号。该行为不等于完整原型类型检查，尤其不能据此证明默认转换以外的调用正确性。

独立反例s80x_block_float：宿主cc输出3.5，4a84274私有产品参考输出0.5（均rc0）。C局部原型完全跳过签名，Python也跳过参数列表但登记返回类型；不能将模型对参考一致提升为C语义正确。保留探针、不加入equal清单，下一项优先同步修复原型签名及实参转换。组合探针中的函数地址要求被取地址的函数已定义；仅有块内声明、定义在后时模型仍未识别函数值，另记未覆盖，未声称已经迁移。

块内原型迁移验收（不含浮点签名缺陷）：PARAMS由函数指针声明与块内原型共用，普通声明遇到参数列表不分配槽，逗号继续原声明路径；EOF明确拒绝，实际候选未闭合括号rc1报unterminated parameter list。gen2净增3行，无新执行器原语。原310项先通过，加入b_init5与s80_block_proto后固定312项全相同，chain165项全相同；两文件×3优化级别共6次实际构造模型运行与cc一致。仅执行受影响7项门禁，两槽单窗口45.84秒全通过，源码到ELF43.52秒。
模型5278状态、1,357,608条目、JSON29,428,415 B；候选5,980,518 B，sha256 eed6c1f793d347ab401cdd02e1c50621da2152ec6685eea6fe05c1971b4d2306。模型候选在s80x浮点原型反例上也输出0.5，与参考共同偏离cc的3.5；明确登记为未修复，不计入通过证据，下一项同步修复。默认产品源码与.com未变；本提交只完成参考已有块内声明行为的迁移，不称完整原型语义完成。


### 块内原型签名修复（进行中）
修复真实浮点调用错误：C抽出函数定义已有的单参数声明解析，由块内原型共用，只登记函数返回与参数转换信息、不创建参数局部槽或输出指令；Python同样抽取现有参数列表解析。块内逗号声明继续时恢复外层声明描述符。随后模型同步使用签名，重建产品与完整冻结门禁，不以参考旧错误作为答案。


### S-17 设计校正（2026-09-27，用户指出模型设计漂移）
停止以新增equal用例数驱动语法补丁。核对当前源码：gen2.py的开头把目标写成已实现事实；research/e3-structured.md规划的独立grammar.txt/templates.tsv并未成为实际输入。现有路径包含可复用的构造器、运行时模型，以及部分真实表来源，但大量编译规则仍由生成器中的命名状态与动作手写，不能称声明式文法迁移已完成。
当前未提交的块内原型签名修复保留并收尾，不继续扩语法。已实测修复浮点反例与私有参考tape相同，原312项固定集全同；这不等于完整产品验收。之后按声明器、类型转换、绑定/作用域、控制流四个共用机制核对规则来源和组合边界，优先合并重复规则，不能只把分支搬进另一个文件或自创DSL便称模型化完成。运行时模型替代与规则来源简化分别验收，旧参考作为回退，不能偷偷承担模型未覆盖输入而宣称完成。
有限语法规则不等于有限程序集合；C的typedef消歧、类型与作用域需要显式属性/存储规则。撤回无条件LL(1)与“新构造只需一条产生式加模板”的承诺，规模和收益以实现测量为准。该校正服务既定S-17，不新增FX研究工程。


纠偏咨询回报与核对：cc-unisacc只读审阅指出，type/tyinfo已有共用来源，应保留；主要重复在C/Python/显式状态机的识别与控制流程。不能把这两张表的共用外推为所有语义已统一，初始化、布局与调用约定仍有手写规则。核对其候选后，DIMS/DIMSAVE和ELSZ事实上已被多处共用，DECLN只是DECL的名字适配入口，不能为了凑重构数量再抽一层。gen2头部已列出现有共用入口，防止继续绕过它们。当前FN.params/parameter_decl/parameters签名修复正是已确定的重复消除；先收尾它，不新增grammar.txt/DSL项目。咨询的“一次审计结束”只作为这次整理的边界，绝不代替S-17默认模型产物的最终验收。


### 用户重申的权威链路与 E1 实查（2026-09-27）
目标是 *.tsv规则 → 构造模型权重 → 字节流经阶段模型推导 → 字节流，覆盖C99兼容编译全链路。仅运行时使用模型不等于规则来源已完成迁移；不再把新增手写状态当作该目标的直接完成证据。

E1实际链路：exec/lex/gen.py → JSON → exec/c/tbl.py → exec/c/net.py → e1.net；exec/c/buildcompiler.sh将其装入六目标共享包，阶段接口为pp.text→tokens.typed。exec/lex/net.py是旧UNS2实验（推理后回填DENSE），不是当前候选构建入口，不能混作当前运行时证据。

| E1规则 | 当前权威输入 | 当前缺口 |
|---|---|---|
| 字符分派 | weights/gold/lex.tsv | handler把动作类别解释为具体状态/动作，仍手写 |
| 字节分类、空白、特殊词前缀 | lexcls.tsv、lexword.tsv | 部分字符串前缀转移仍直接编码 |
| token种类/词表顺序 | unisa.gold.TOKS、front.lex.TYPEKW | 不是独立TSV输入；还读取旧kernel词表校验 |
| 数字扫描 | build_num/num_start | 转移条件、回退位置和后缀规则均在Python |
| 字符串、字符与注释 | build_str/build_cmt | 转移、EOF和发射动作均在Python |
| 标点最长匹配、关键字识别 | 词表加trie构造 | trie可作为通用构造算法保留；词法专属动作须显式声明 |
| 阶段发射、计数与终止 | emit_kind/build_dispatch | typed/位置选项与输出格式仍在生成器 |

纠偏实现边界：先让完整E1的有限转移/动作、词表与输出格式成为可独立读取的数据，再复用已有权重构造与执行机制。不能只导出一次JSON/改名TSV而继续把gen.py当权威；验收须生成过程不调用原词法规则生成器、不读旧kernel，并删除被替代的手写规则。全域转移/动作对比使用冻结旧生成结果作为迁移裁判，字节流测试另验组合；不得把E1完成外推为E2—E6完成。这是当前整阶段边界核对，不是新增文法框架或继续解析补丁。


E1纠偏实施首步：exec/lex/number.tsv以44条互斥字节范围/默认转移声明完整数字扫描（十/十六进制、小数、指数回退、后缀、入口）。删除gen.py对应条件分支，净-47行；通用byterules.py只展开有限字节集合、检查冲突/全定义并链接动作序列，没有数字语义。动作使用既有执行器原语，number发射序列仍由旧输出格式代码供给，明确未完成整阶段数据化。冻结旧typed生成结果与新结果按状态名和动作内容比较85,123个观测，全部相同；序号不作为语义判据。本步尚未运行端到端候选，不复用此前产品门禁为证据。


E1字符串/注释规则迁移：literal.tsv 29条规则，涵盖字符串拼接/前缀、字符常量、行/块注释及EOF行为；空白集合引用lexcls.tsv，不复制分类答案。与number.tsv共用57行有限规则读取器，gen.py累计+23/-120（净-97）；计入读取器后的Python净减少40行，规则数据74行含两个表头。完整typed E1的85,123个生成域观测转移与动作逐项不变。exec/c/netcheck.py读取新.tbl，实际C推理对86,625个编码域观测/336状态全同，动作/字符串一致，返回0；此编码域含转换后统一结果域，不能与生成域观测数混称。尚有分派、词表、token输出、标识符专属流程未数据化，未切换默认产品、未宣称完整E1或重构完成。


E1词表输入校正：TOKS改读weights/gold/parse.tsv的tok字段（恰好一行、非空唯一），TYPEKW改读iterate/kernel/typekw.tsv（kw行、非空唯一）。默认生成不再读取kernel/unisa_model.inc或src前端文本；--check-declarations显式开启旧词表/源码兼容裁判，并接入原lex/run.sh，缓存补入全部新TSV及裁判输入。带裁判typed生成已通过，85,123个转移/动作与原token顺序全部相同。注意unisa.tsvgold仍通过gold.Stage导入Python模块，尚不能宣称独立于gold.py；仅已移除对Python词表值和旧kernel内容的生成依赖。未修改旧生成物权威或宣称全E1完成。


E1 token输出声明：output.tsv列位置前缀、名字、SPAN/SPAN2及结束动作，spelling.tsv列普通/typed模式是否附原文。emit_kind不再硬编码token编号集合或字节格式，只实例化动作参数；通用@bytes展开静态字符串。四模式两槽比对：plain/typed/positions各85,123、locations100,920个转移及完整动作全部一致，含位置寄存器与字符串输出；每个生成子进程10秒限时，总运行约1秒。该步骤并未迁移CNT计数/EOF输出和标识符控制流程，也未证明完整E1独立输入闭合。


E1标识符固定流程：Unicode转义4/8位消费、非法回退以及属性括号跳过/栈清空，迁入ident-byte.tsv与ident-stack.tsv；同一有限规则读取器支持字节范围和命名栈符号，生成器删除对应条件链与专用循环。完整typed E1的85,123个转移/动作全同。trie构造和词表分派仍在生成器，不能称标识符规则已全部迁完。


E1计数/终止迁移：CNT0字节状态、CNT1余数状态和CNTP栈状态均由count-*.tsv声明，包括计数字符输出及ACCEPT；通用install_rules同时装载byte/result/stack域，复用于字面量和标识符固定流程。生成器删除计数算法专属转移循环。完整typed E1的85,123个转移和动作逐项同旧结果；词法分派/trie专属处理仍待完成，不宣称全部E1来源已闭合。


E1分派动作迁移：entry.tsv声明lex.tsv全部动作类别的入口、字节例外和动作序列，EOF发射在output.tsv中；构造器校验动作集合精确相等，动态trie/数字入口通过显式链接注册。handler不再有词法类别if链，仅查声明并连接机器。四模式全域转移/完整动作同迁移前（85,123×3及100,920），两槽约0.9秒。identifier/punctuator链接内部仍有专属控制，须继续逐项落实而非用链接隐藏剩余规则。


E1 trie边界迁移：ident-flow.tsv声明继续字节类/UCN/结束及特殊词后空白/括号处理；ident-end.tsv按显式优先顺序链接lexword词类到结束动作，保留@next继续匹配，普通token为末项。标点最长匹配的scan.start/advance/accept/rewind及拒绝动作移入output.tsv。构造代码保留词表trie查找、最长接受前缀和声明链接，不再内嵌对应字节行为。四模式全域转移/动作全部与迁移前一致（85,123×3+100,920）。尚须解除gold.Stage间接模块依赖、核对locations附加流程及隔离生成，不能提前称全部E1独立来源完成。


E1独立输入验证：unisa.tsvgold拆出不导入gold的load_table，既有load_stage作为惰性Stage适配器保留；没有复制第二份TSV解析器。适配器18阶段8,484键schema/corpus与gold全同。隔离临时根只复制E1构造源码/locations、通用TSV读取器、E1规则TSV和parse/lex/lexcls/lexword/typekw声明，没有gold.py、kernel或参考编译器；从临时cwd生成plain/typed/positions/locations，四模式全部转移/动作与冻结旧结果一致（两槽约0.8秒）。此证据只证明输入独立，locations.py仍含封装协议控制，尚未称所有规则均为TSV。


E1位置封装迁移：location-byte/result.tsv声明magic、长度解码、边界、记录循环、输出封装与切换输入，locations.py由协议实现缩为14行装载适配。有限规则读取器仅增加观测键左移代入，参数限定0..63，不解释协议语义。四模式生成域转移/动作全同；位置模式实际C网络与表核对104,045观测、403状态，actions/strings一致，netcheck返回0。该数字是编码域，生成域仍100,920。当前未提交改动中，gen.py净-133、locations.py净-54、tsvgold.py净+5、新通用读取器76行，合计Python净-106行（run.sh不变行数）；新增规则数据315行含表头。尚需集成字节流检查、规则完整性和缓存输入核对，不以表相同替代最终默认产物验收。

E1定向字节流集成：双槽队列/tmp/unisacc-e1-rules-gate 2/2返回0，窗口3.32秒；exec-lexpos的9项参考/Python位置用例通过，exec-lexloc的6个拼接输入通过、11个畸形封装拒绝。使用私有UA，不碰/tmp/ua_ref；该结果仅覆盖两项定向门禁。


E1收尾依赖修正：共享models.identity补入iterate/kernel/typekw.tsv；旧pipeline/run.py生成依赖纳入本阶段TSV及四张gold声明；gatequeue冻结指纹补入typekw.tsv，lex/run.sh此前已纳入全部E1 TSV。临时假根分别修改typekw与entry规则，cache identity均变化，无共享输入修改。exec/lex/rules.md列完整输入、有限规则格式、trie/ABI构造边界和验证范围；不是宣称任意词法协议无需适配代码。

E1收尾定向队列/tmp/unisacc-e1-rules-chain两项全rc0，双槽42.25秒：实际网络chain固定165项全部相同，0拒绝/未覆盖/丢项（17.55秒）；exec-srcelf通过（42.21秒）。加此前词法位置与封装两项、全域转换/推理及隔离生成，形成E1声明迁移证据。产品签名修复仍未完成完整冻结门禁，默认产物切换与E2—E6规则来源统一仍未完成。

原型签名收尾：tests/c/b_blockproto覆盖double/float/_Bool参数、逗号原型与块作用域恢复。实际产品.com与实际模型候选各-O0/-O1/-O2均同宿主cc（8.0 2.2 1），Python同；原312固定项先通过，随后将此回归与s80x纳入E3/chain。当前.com为1,361,760 B，尚待本树全门禁，不作发布结论。


### 声明迁移后的冻结验收进度（2026-09-27）
当前HEAD为42a3c56（E1声明规则迁移），原型签名修复仍未提交，源码在门禁期间冻结。主队列/tmp/unisacc-protosig-full以双槽和55秒窗口完成38/119项；exec-driver-core另留独占运行，完整清单共120项，尚未全绿。主队列原始36通过、2失败不改写：exec-multi-ua并发53.05秒超时，独占复跑50.76秒通过；exec-container在net写文件时报ENOSPC，清理本轮旧候选目录的可重建中间物（保留各.com）后独占32.52秒通过。复跑分别存于同名前缀retry-multi/retry-container目录；合计38个不同套件已有通过证据，仍待82项。后续续跑原主队列，不重做已通过项；接近上限的重项独占，不能把负载超时或磁盘失败涂成原轮通过。
本轮产品unisacc.com：1,361,760 B，sha256 a61a882d506cb621c8c101370e78c6f3e0110bcff544e51b007a65b58ddc02ed，来自当前未提交签名修复；不是发布或默认模型切换。实际模型候选/tmp/unisacc-protosig-candidate/unisacc-next.com的sha256为11329391d00a2949d837f5ba008edaa1321f0b14b8574fda5f859dc641089c8c，建于E1声明迁移之前，不能称为最新规则源码重建产物。
E2后续输入审计：exec/pp/gen.py仍从kernel/unisa_model.inc提取DIRV，预定义宏按目标的分支、P0拼接和P1去注释规则仍由Python手写。这是待迁移来源，不以E1完成替代；下一阶段应复用有限规则读取与构造机制，消除旧kernel生成依赖和被替代的专属分支，不另起语法框架。


冻结验收发现测试夹具遗漏：gate-infra的cache identity假根未提供新增的iterate/kernel/typekw.tsv，导致FileNotFoundError。修复限于tests/queuecheck.py：补显式声明并验证改变它会使identity变化，不改变模型生成、缓存算法或编译器。旧队列已完成46/119项，含该失败；因检查源码改变，新冻结队列重新登记，旧记录保留为此前树证据，不冒称新树全通过。


原型签名验收收尾：43bc914补齐cache测试夹具后，/tmp/unisacc-protosig-full2跑完116个套件，原始112通过、fat超时及三个ENOSPC失败保留。磁盘清理只删除本轮旧候选中间物和未占用的旧生成缓存，保留候选.com与日志；disk-retry2的container/tableself/native-stages分别34.36/15.38/19.71秒通过；fat2独占37.30秒通过。预留core2独占52.61秒通过；heavy2的multi-ua/memory-ua/memx86-ua独占50.41/41.87/47.24秒通过。
原始full2与disk-retry2最终未返回整体绿色：协作者在运行期间新增并提交21aa027（examples/apps/colorpack.c及其README），广域指纹因而拒绝整轮结论。没有重写这些退出状态。独立输入审计以43bc914的apps README内容并排除新增colorpack，重算得到完全相同的原指纹3a6c0afaf0eb7feea0f05771f74b014725c29209b750186ca435d3df8d2743bc；证明其余全部冻结文件（含编译器、生成器、测试及.com）未变。所有120个套件命令逐项与当前--plan --com相同，新增colorpack不在固定列表或examples/*.c输入中，apps README也不是构建/运行输入。最终以实际单项rc0并集核对120/120、无遗漏；这是附非输入变更审计的分批验收，不宣称原队列单次rc0。完整对账保存在/tmp/unisacc-protosig-final-evidence.json，原记录目录保留。
本次只闭合已开始的块内原型签名修复与E1迁移回归；默认产品仍非完整模型路线，未发布，E2—E6声明来源迁移与最终默认产物验收继续。


### E2声明输入迁移（进行中）
先冻结六目标、普通/位置两种模式的全部状态转移与动作，作为旧实现裁判；实现去掉kernel/unisa_model.inc的DIRV提取，改用pp.tsv自身schema与已验证的通用读取器。目标预定义宏改成OS/架构/公共项声明，保留当前顺序与当前兼容行为（Windows上的__LP64__仍是现有子集契约，不外推系统ABI）。同步真实构建缓存依赖；之后迁移P0/P1等有限控制规则，不能把本步仅移除输入依赖称为完整E2声明化。

E2首步实测：六目标×普通/位置模式12份JSON的完整状态、转移与动作均与迁移前一致；Linux/macOS普通180,466、位置183,036观测，Windows普通179,951、位置182,521。隔离临时根只放pp构造源码、通用TSV读取器、pp.tsv、predefines.tsv和include头声明，无kernel、gold.py、src；从/tmp生成12份均相同。临时声明把__linux__改为__linux_variant__，只有一个初始化动作序列变化，状态映射不变，证明宏名称确实由声明控制。当前改变的是输入来源，P0/P1和宏控制规则仍手写；Python行数未减少，不称完整E2声明迁移。
E2输入迁移集成：/tmp/unisacc-e2-declarations-gate双槽2/2通过，总2.74秒；宏展开86个完整结果与4个明确拒绝，位置封装9个参考用例与5个Python裁判用例通过。缓存来源同步到pp/run.sh与旧pipeline/run.py，共享models.py原已包含exec TSV和weights。未改产品源码，不把此定向结果写为新一轮全产品门禁。

E2下一步：P0（shebang/续行）和P1（注释/字面量保留）转移与动作迁入有限规则TSV，删除原Python分支。复用E1读取器并移为exec/finite_rules.py公共模块，仅增加显式常量绑定替换；SPLB是执行器存储布局常量，出口是阶段链接，不由回调藏词法决策。普通、位置、自动头开启/关闭均核对完整转移和动作；顺序号可能改变，比较动作内容而非序号。

E2 P0/P1实现：text-byte.tsv与text-result.tsv共46条规则声明shebang清空、续行拼接、字符串/字符保护、行/块注释、换行保留与未终止拒绝。删除gen.py对应分支；E1有限读取器移到exec/finite_rules.py供两阶段共用，增加显式constant绑定替换，不调用语言相关Python回调。六目标×两模式的完整观测/动作同旧结果；关闭自动头的两模式120,842/123,412观测亦同。共享模块迁移后E1四模式85,123×3及100,920观测同旧结果。隔离根无kernel/gold.py/src的12模式生成通过，宏改名输入依赖检查仍通过。
cc-unisacc的死函数清理建议已收到但未实施；其“经典walker不得模型化”的措辞适用于旧产品T-1边界，不代替用户重申的S-17全阶段模型推理目标。本轮不增加产品修改范围。
E2 P0/P1定向集成收尾：/tmp/unisacc-e2-text-gate双槽4/4返回0，总6.05秒；exec-macros、exec-pploc、exec-lexpos、exec-lexloc分别2.84/2.44/3.52/2.72秒，实际网络路径覆盖宏展开与位置封装。计入共享读取器移动及新增常量绑定，本片Python净减少29行，46条规则成为显式输入；没有复制旧读取器。未改产品源码、未重跑全产品门禁；E2剩余宏/指令控制仍待迁移，不称完整重构完成。

E2宏存储规则迁移决定：MFIND/MDEF的定义可见区间、历史链遍历及新记录初始化迁入有限转移声明；字段偏移与存储基址显式绑定，继续使用现有通用读取器。删除对应Python状态分支，不把规则搬到新的生成脚本。以冻结的完整E2转移/动作及实际宏用例验证。

用户再次确认核心：表（*.tsv）构造确定性网络权重，模型推理替换程序逻辑；重构验收要同时记录实际权重输入与删除的专用程序逻辑，不能保留第三套生产规则后只报表覆盖增长。规则本身的信息仍需存在，迁入声明不代表语义复杂度消失；应收缩的是语言专用程序实现及其重复来源。
E2宏存储迁移实测：macro-byte/result.tsv共27条声明（含不可达默认），删除MFIND/MDEF的Python分支，gen.py净减少19行；复用有限读取器，无新增专用框架。十二目标/位置模式的全部转移与完整动作同冻结参考；实际网络exec-macros与exec-pploc双槽2/2返回0，窗口2.71秒（分别2.67/2.49秒）。产品源码未变，不称全产品重验或完整E2完成。

E2自动头扫描迁移决定：将AISTART—APW调用/定义识别及括号扫描、ACP复制收尾移入有限规则声明；沿用通用读取器。头文件导出名字的提取与按头展开的构造暂保留并明确列为剩余逻辑，不宣称整个自动包含已声明化。

E2自动头扫描迁移实测：autoinc-byte/result.tsv共31条规则，删除调用/定义扫描及复制收尾分支；gen.py +5/-31，净减少26行，无新增构造器框架。六目标×普通/位置模式全部状态转移和动作同冻结参考。实际网络双槽exec-macros与exec-pploc 2/2返回0，窗口2.67秒；位置用例含printf自动头路径。头名字提取及按头展开仍在Python，后续继续消除专用控制，不能把本片当整个E2完成。

E2自动头展开收尾方案：复用有限规则读取器，增加显式状态名链接（$name由绑定给出），把逐名字called判定和条件输出头行做成可重复实例化的声明模板。生成器只枚举头/名字并绑定连接、数据字节和布局；不生成第二份展开答案表，不新增语法框架。

E2自动头展开模板实测：autoinc-name/emit的byte/result声明替换按名called判定及按需输出分支；状态链接和字段绑定复用finite_rules，text/macro/autoinc装载统一。gen.py净-5行、共享读取器+6行，本片Python净+1行，不能称本片总代码减少；增长仅为通用显式状态绑定，专用判定已删除。六目标×两模式完整转移/动作不变，E1四模式及E2无自动头两模式也全同。双槽实际网络四项检查4/4返回0，窗口6.09秒。头名字提取、头顺序与输出文本构造仍为剩余来源，尚未完成整个E2声明化。初次替换保留了一段旧代码导致IndentationError，已删除残段后重新完成上述全部验证，未把失败记为通过。

E2表达式归约迁移决定：运算符编号/拼写/优先级/元数成为声明数据；XRED及比较、短路毒值传播、除余零标记、三元和一元归约由有限规则声明给出，删除Python按运算符分派。值栈布局只作为显式绑定。该迁移保持现有行为，不把参考一致性当作完整C预处理语义证明。

E2表达式归约完成本片：operators.tsv声明25个运算符的固定机器编号、拼写、优先级、元数；reduce-byte/result.tsv的67条规则负责运算分派、比较真值、短路毒值、除余零标记及一元/三元归约。编号仍是与语法转移共享的机器ABI，不能任意单独改号；语法扫描与运算栈调度仍待迁移。gen.py净-40行；集中回归测试净+11行，Python合计净-29行。十二目标/位置模式及无自动头两模式完整转移/动作同旧结果。实际网络双槽2/2通过，3.04秒；新增11个独立真假预期覆盖各运算家族与未求值除零，两种封装都与参考完整字节、宿主预处理token和Python执行器一致。共88个完整结果及4个拒绝；不扩大为全C语义证明。

E2表达式控制收尾决定：把剩余XE扫描/defined/宏帧/优先级弹栈调度完整转移迁入声明；优先级仍绑定operators.tsv，不复制优先级答案。通用装载器登记声明中的PUSH返回标签，生成器只绑定存储布局/优先级并装载。删除xpushop/xpopv/XPUSHV/XTOP和build_xe的手写实现；参考实现由已有提交与冻结转移保留，不在生成路径运行。

E2表达式控制迁移实测：expression-byte/result.tsv声明78个状态、241条互斥/默认规则，涵盖数字扫描、defined、对象宏帧、操作符识别、优先级弹栈和括号/三元调度。删除xpushop/xpopv/XPUSHV/XTOP及对应控制分支；build_xe只绑定布局与operators.tsv优先级后装载，返回标签由通用PUSH动作登记。gen.py +8/-139净-131行，回归测试净+1，Python合计净-130。十二目标/位置及无自动头两模式完整转移/动作同冻结参考；E1四模式未变。实际网络两项并行2/2返回0，窗口2.89秒，表达式独立真假用例扩到13项，含defined双形式、嵌套对象宏、未定义名字取零；88完整结果和4拒绝。运算符编号/拼写仍是该机器固定ABI，声明变化须连同相应语法/归约一起改；不宣称任意新增运算符自动成立。E2其他宏/指令流程仍待迁移，未完成整个重构。

E2宏字符串化/拼接迁移决定：完整HSCAN/HX状态规则由TSV提供，保留通用原语和显式字段布局绑定，删除build_hx内部pre/plook及所有专用分支。先以冻结全转移/动作确认等价，再复用实际网络的C99字符串化、拼接、空参数、变参和hide标记用例；不扩原先拒绝的拼接域。

E2字符串化/拼接迁移实测：hash-byte/result.tsv声明68状态、221规则，字段偏移仅由8个布局常量绑定；build_hx及pre/plook等专用控制删除，gen.py +4/-167净-163行，无新执行器原语。十二目标/位置及无自动头两模式完整转移/动作均同冻结参考；E1四模式仍一致。实际网络exec-macros/exec-pploc双槽2/2返回0，窗口3.00秒，88完整结果和4拒绝均保持。规则信息移入声明，网络规模与能力未改变；支持域外的拼接仍拒绝，不宣称新增C99覆盖。E2生成器现968行，指令扫描与宏重扫主体等仍待迁移。

E2命令行资源迁移决定：-D/-U/-include/-nostdinc的有限扫描与宏记录动作迁入声明；位置模式仅绑定附加计数动作，不保留命令行语义分支。复用现有构造器与实际driver-resources门禁，使用私有UA且独占重项，不增加新套件框架。

E2命令行资源迁移实测：cli-byte/result.tsv共19状态、43规则，生成器净-37行；位置模式只绑定行数计数动作，字段/基址显式绑定。十二目标/位置及无自动头两模式完整转移/动作同冻结参考。实际exec-driver-resources独占31.37秒通过（队列窗口31.50秒），覆盖宏参数、头资源、printf/math与隔离容器；未并发重项。一次性迁移脚本首轮误写至hash文件名导致缺cli声明而拒绝生成，随后把生成内容移到cli、从HEAD还原hash（无差异），补全间接ea布局绑定后重新验证；无失败隐藏。指令与宏重扫主体仍待迁移。

E2宏重扫迁移决定：P4对象/函数宏展开、参数帧、跨帧调用、hide标记、UCN及_Pragma控制整体迁入声明；字段布局/容量显式绑定，保留当前行为与拒绝域。删除对应Python流程，不调用旧生成器补答案；位置封装与指令阶段仍单列。

E2宏重扫迁移实测：rescan-byte/result.tsv声明144状态、404规则，覆盖原P4全部对象/函数宏、参数帧、跨帧续接、隐藏标记、UCN、标点与_Pragma处理；删除原流程和无剩余调用的PUSHM/PUSHMB/CRC动作代码。字段/容量26项显式绑定，未增加执行器原语。gen.py +2/-322净-320行，现611行。十二目标/位置及无自动头两模式全部转移/动作同冻结参考，E1四模式同旧。实际网络双槽3/3返回0：固定源码→tape链167/167相同、无拒绝/未覆盖/错误/丢项（17.06秒）；宏88完整结果与4拒绝、位置封装均通过，窗口17.10秒。只改变规则来源与删除重复程序控制，不宣称新增覆盖或默认产品切换；E2指令阶段仍待完成。

E2指令宏体/包含流程迁移决定：DEF0至INCH的参数声明、宏体保存、include资源搜索与拼接流程迁入有限规则；位置附加记录单列声明并显式连接。pp.tsv决策读取和act连接暂保持，不能把生成时选定的决策复制进新表后切断pp.tsv输入。

E2指令宏体/包含迁移实测：directive-body的33状态80规则、include-location的3状态5规则来自显式声明，位置体入口由绑定连接，IRNAME/IRLN/IRNL等保持布局参数。gen.py +4/-91净-87行。十二目标/位置、无自动头两模式的完整转移/动作同冻结参考。资源重项独占31.88秒通过，随后宏2.94秒、位置2.57秒通过，队列3/3返回0，窗口37.66秒；使用私有UA。剩余act及指令条件连接仍保留pp.tsv为决策来源，下一步迁移其动作模板，不复制PP答案。

E2指令动作迁移决定：以（指令名、pp.tsv输出标签）选择有限规则模板；共享读取器增加声明分节选择，不含指令语义。仅登记当前声明域使用的模板，未声明的组合明确失败，不能默默回落。删除act条件链；通过临时修改ifdef结果验证实际表输入仍控制输出。

E2指令动作迁移实测：directive-action两份分节声明共14个（指令、标签）模板、45规则；pp.tsv输出标签直接选择模板，act条件链及无剩余调用的ea动作帮助函数删除。生成器净-43行，共享读取器净+6行，Python合计净-37行。十二目标/位置和无自动头两模式完整转移/动作不变，E1四模式亦同；实际宏/位置网络双槽2/2返回0，3.08秒。隔离临时声明仅改ifdef/0 skip→take，构造tbl/net并核对网络=表，实际预处理结果NO→YES；改为未声明的ifdef/macro则生成失败且无输出。未复制PP决策答案，原读取路径保留。一次性迁移脚本最初字符串格式替换失败，修正后才删除旧act并完成全部验证。

E2指令扫描收尾决定：P3行扫描、条件入口和pragma拒绝规则迁入声明；DSW仅按pp.tsv字段顺序连接到具名状态，动作仍按pp.tsv输出标签选择。目标预定义名字的逐项装配保留为数据连接，扫描分支不保留Python副本。

E2指令扫描迁移实测：directive-scan的41状态86规则替代P3行扫描、指令条件入口与pragma处理；DSW按pp.tsv字段顺序连接具名入口，动作标签选择仍读取pp.tsv。gen.py净-84行（含两个无剩余用途的定义清理），现397行。十二目标/位置模式完整转移/动作同冻结参考，无kernel/gold.py/src的隔离生成十二模式亦同；预定义宏改名只改变相应初始化序列。共享检查确认无自动头两模式及E1四模式仍一致。实际网络双槽三项全rc0，窗口17.01秒，chain固定167/167相同，宏88结果与4拒绝、位置检查通过。剩余为数据/模板装配及locations封装等，不将当前结果称为整个编译器重构完成。
