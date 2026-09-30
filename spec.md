# unisacc 规格（条款全文）

> 由 prd.md 拆出（0.0.15 R15-0，2026-09-30）。代码与测试注释里引用的条款编号（`[S-9]`、`[W-16]`、`[A-44]`……）都在本文件；编号稳定，只增不改。prd.md 只写当前状态、路线与索引。

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
| **stage（阶段）** | 当前产品的字节流变换步骤，见 §0.3 路由表；经典 gold 的有限决策点清单另见 §3.0，二者不是同一模型粒度 |
| **key** | 某阶段的一个输入元组，取自该阶段声明的字段词表 |
| **K_s** | 阶段 s 的 key 全域 = 各字段词表的完整笛卡尔积 |
| **gold** | 阶段 s 上的参考标签函数 `G_s : K_s → Class_s`，全函数 |
| **FULL gold** | 在整个 K_s 上评估，而非在批次或子集上 |
| **Oracle** | 网络与 gold 之间的唯一分发点，见 §4.1 |
| **tape** | 目标无关的通用指令流，§4.3 |
| **TargetProgram** | tape 经 lowering 后得到的、携带目标事实的指令流，§4.5 |
| **fold** | 把同一份 tape 铺到 6 个目标各自执行并比对，§5.1 |
| **kit** | `unisa ship` 产出的交付包 |



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
**W-9**（2026-09-26 修订：printf **一律是对 `<stdio.h>` 里 printf 的普通调用**。C 前端见 991d337，调用时自动带入该头；Python 前端见 18c8f22，由 driver 补头，脱糖代码已删。C 前端的 `do_printf` 脱糖只在单元里**没有** printf 声明时兜底，也就是 `-nostdinc`。两个前端的 tape 不要求一致：closure 比的是同一条 C tape 过两个后端，ccrun 只比运行结果。下面是修订前的原文。）`printf` 在走查期按**静态格式串**脱糖（`%d %s %c %u %%`），这是快路径也是常见路径。**格式串不是字面量就根本脱不了糖**，此时放行到 `<stdio.h>` 里真正的变参 `printf`（它和 `fprintf`/`sprintf`/`snprintf` 共用同一个运行期格式化器 `_u_vfmt`）。一个坑：调用在**走查末尾**才解析，所以调用可以先于定义——对普通函数无害，但这几个函数的**调用约定不同**（参数全压 tape 栈，[W-16]），后到的定义救不回已经按另一种约定发出去的调用，所以它们的变参性写死在 `VARIADIC_LIBC` 里。`%d` 经发射的 `__itoa` 助手（纯 tape op，故可原生编码），`%s` 经 `__strlen`。
**W-10** C 字符串字面量**必须 NUL 结尾**。`write_literal` 传显式长度，故字面量块不会输出该字节；但 `%s` 走 `__strlen`，无结尾符会一路扫进相邻字面量。
**W-12** 相邻字符串字面量按 C 语义**拼接**（`"a" "b"` == `"ab"`）；扫描器逐个字面量出 token，在 token 层折叠。
**W-11** `static` 局部变量取**静态存储**（数据段、零初始化），不是栈槽。
**W-15** **libc 地板**：`include/` 带 `<ctype.h>` `<limits.h>` `<assert.h>`，以及 `exit`/`abort`。这三个头加 `exit` 几乎每个真实 C 程序都要，之前一个都没有。两条实现上的决定：① **`exit` 是唯一不能用 C 写的库函数**——它不能返回——所以它是 `__exit` 内建（`.sys` gate）的一行包装；② **`assert` 只打印表达式，不打印文件与行号**：这个预处理器没有 `__LINE__`/`__FILE__`，因为 include 是**原地展开且不插行标记**的，第一个 `#include` 之后两者都会是错的，而**错的行号比没有行号更坏**。`<limits.h>` 给的是**类型的极限**，不是本编译器的求值宽度——[G-2] 让 `int` 表达式在 64 位里算，这不改变 `INT_MAX` 是多少。
**W-13** **全局指针的启动期初始化**：文件作用域的指针变量若以地址常量（字符串字面量、全局/函数地址）初始化，其值在程序启动时由启动序列写入数据段，而不是编译期绝对地址（镜像加载地址不固定）。（编号早已分配并在 archive/prd-findings-20260929.md 的“补齐的 C 特性”表中使用，正文 2026-09-30 补入。）
**W-16** **unisacc 生成代码之间的私有调用约定**（不是 SysV/AAPCS64；跨 ABI 的进出由 lower 的桥与库调用桥承担，见 §4.5/§I）：调用者把全部实参**依次压 tape 栈**（第一个参数在最低地址、紧挨返回地址，自左向右递增），`r9` 为帧指针，返回值在 `rax`（arm64 对应寄存器由 regmap 表给出），调用者清栈。该约定**不稳定**：随 regmap/abi 表变化，外部代码不得依赖；需要与外部代码互调时走 libunisacc 的载体/计划路线。`examples/apps/xgui.c` 的 syscall 桩只是对现状的经验描述。
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

### 5.5 完成度（四层判据，当前）

盘点分四层，因为**难的部分和多的部分不是同一部分**：论点层基本做完了，产品层才走了一半。每层的"100%"是一条可测的判据，不是一个百分比。

| 层 | 目标 | 完成判据 | 实测（2026-09-21 起，2026-09-25 复核） |
|---|---|---|---|
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。18 个 stage，8,484 key 全枚举，零分歧（2026-09-25 复核） |
| **S-2 目标** | 一条 tape → 六份镜像，stdout/exit 一致 | 六个目标**在真机上**逐例一致 | **已达**。`unisa fat` 是**一个 OS 内**的多架构；cosmo 式一文件多目标（MZ + shell 自选切片）**已达**，见 S-7 #9；真正同时合法为 ELF/Mach-O/PE 的**单一字节序列**仍未开工 |
| **S-3 语言** | C99 子集覆盖别人写的代码 | 外部语料 `unsupported 0`、`wrong 0` | **已达当前语料，含浮点**（2026-09-22）。`corpus 220 pass 214 wrong 0 unsupported 0 knownfail 6`；余下 6 个全是 C99 之外的扩展（GCC 语句表达式、空结构体、`push_macro`、C11 `_Generic`）与 [G-2] 的 64 位整数求值，见 E-54 |

S-5..S-17（历史推理、0.0.7/0.0.8 计划、S-17 迁移决定）已归档：[prd-history-20260929.md](archive/prd-history-20260929.md#5-5)。当前版本状态见 §0 的 v0.0.11 摘要与 v0.0.12 计划树。

### 5.6 覆盖限制登记（当前）

- 三字符组（trigraphs）未实现；C99 24 个标准头缺 6 个（complex/fenv/locale/setjmp 之外的清单见归档 5.7）；`<setjmp.h>` 是评估过的永久非目标（tape 无间接跳转）。
- `fork`/`exec`/`popen` 未暴露给用户代码（目录列举已补）→ R12-2 ⑥。
- 位域 sizeof（2026-09-27 巡查）已修复并复验。

原巡查记录见 [归档](archive/prd-history-20260929.md#5-7-5-9)。


## 6. 实验发现 [E] —— 面向论文（已封存）

E-编号的已证实/待验证/已证伪/开放问题/复现/先行研究全表移至 [archive/prd-findings-20260929.md](archive/prd-findings-20260929.md)；论文按 E-编号引用时在该文件解析。新的实验结论直接写进对应版本的回执与 `research/*.json`，不再在 prd 累积叙事。

