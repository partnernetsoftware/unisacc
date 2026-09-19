# 先行研究调研 —— UNISA SH 的定位

> 调研日期：2026-09-19。方法：WebSearch / WebFetch，覆盖 arXiv、ACM DL、IEEE Xplore、
> USENIX、NeurIPS/ICLR proceedings、PMLR、VLDB/SIGMOD、Springer LNCS、Semantic Scholar、
> 实验室主页 PDF、GitHub、Hacker News、个人博客。检索词见 §5。
>
> **本文的立场是"尽量证伪我们自己"**。凡是我们以为新的、实际上早已有人做过的，本文直接指出并给出引用。
> 凡是"没找到"的，本文明确区分 **"不存在先行研究"**（几乎从不这样断言）与 **"我没找到先行研究"**（默认表述）。

调研对象（README + prd.md §1、§6.0 所声称的五条）：

| # | 我们的声称 | 一句话结论 |
|---|---|---|
| C1 | 权重由 gold 决策表**构造**得到，精确性是推导的不变量而非搜索的结果 | **不新**。1996 年起就有，且 Tracr(2023) 做的是同一件事 |
| C2 | 等价性由**全域枚举**判定，无统计估计，**无回退路径** | 枚举验证法**不新**（Jia & Rinard 2021）；"无回退作为硬契约"**未找到**先例 |
| C3 | 纯整数：二值权重、二值隐激活、2 的幂输出权重、无乘无移无浮点、int8 累加 | **不新**，全是成熟技术；唯一略新的是它是构造的**推论**而非量化训练的产物 |
| C4 | 自举：编译器编译自身，bootstrap 不动点逐字节成立，模型 blob 嵌在编译器源码里 | **未找到**任何先例 |
| C5 | 经典/神经的显式切分：无界状态留经典，有界离散全函数变网络 | 作为**原则**散见于 neuro-symbolic 文献；作为**编译器的架构契约**未找到先例 |

---

## (a) 最接近的工作

按"接近程度"排序。前三条是真正会被审稿人拿来砸我们的。

### a.1　ACAS Xu 策略压缩线（2016–2026）—— **最接近的一条线，且它已经走完了一整个循环**

**做了什么。** Julian, Lopez, Brush, Owen, Kochenderfer, *Policy Compression for Aircraft
Collision Avoidance Systems*, DASC 2016。ACAS Xu 的决策逻辑是一张由 MDP 动态规划求出的**数值查表**，
约 2 GB，机载航电装不下。他们用 45 个小型 DNN 去逼近这张表的 value function，存储降到 MB 量级
（文献中引用的数字有 2.4 MB / 0.5 MB 两种口径，取决于版本），并声称在若干指标上**超过**原表。

**这条线随后发生的事，正是我们要引用的部分：**

1. **验证需求催生了整个 NN 形式化验证领域。** Katz, Barrett, Dill, Julian, Kochenderfer,
   *Reluplex: An Efficient SMT Solver for Verifying Deep Neural Networks*, CAV 2017 ——
   Reluplex 就是为验证 ACAS Xu 网络而造的，ACAS Xu 至今是 VNN-COMP 的标准 benchmark。
2. **压缩被证明不安全。** Bak & Tran, *Neural Network Compression of ACAS Xu Early Prototype
   is Unsafe: Closed-Loop Verification through Quantized State Backreachability*, NFM 2022
   (arXiv:2201.06626)：用 Reluplex 在少数网络上验过的若干 domain property，**拿回原始查表去对照时不成立**。
3. **枚举验证被提出来当作对策。** Jia & Rinard, *Verifying Low-dimensional Input Neural Networks
   via Input Quantization*, SAS 2021 —— 在网络前面加一层输入量化层，使输入空间有限，
   然后**直接枚举输入状态**做验证，复杂度只由量化空间大小决定，并且"免疫浮点误差"。应用对象正是 ACAS Xu。
4. **2026 年的结论是：干脆别用网络。** Boniol, Brunel, Chaudron, Garion, Thirioux,
   *Compressing ACAS-Xu Lookup Tables with Binary Decision Diagrams*, NFM 2026 (arXiv:2604.27008)：
   用 BDD 做**精确无损**压缩，"preserves the exact semantics of the ACAS-Xu LUTs"，表示是
   "canonical, deterministic, and fully equivalent"。论文明确把 NN 方案批评为
   "inherently introduce approximation errors and complicate formal verification"。

**我们与它的差别（精确到点）：**

- 他们的 key 域是**连续的**（相对距离、角度、速度），表只是连续域上的一个网格采样；网络必须在网格**之间**插值，
  所以逼近误差是本质的，验证是 NP-hard 的，回退/safety net 是必需的。
  我们的 `K_s` 是**有限闭合的离散积**，不存在"网格之间"，所以 P-3 是判定问题而不是逼近问题。
- 他们的目标是**压缩**（2 GB → MB），我们的 §6 E-3′ **已自行证伪**"神经比表省空间"。
  这一点必须主动说，否则审稿人会拿 Boniol 2026 直接把我们的压缩论证打掉。
- 他们的验证是"抽样性质 + SMT"，我们的是"全域枚举"。但请注意 **Jia & Rinard 2021 已经做过全域枚举验证**，
  所以 C2 的"枚举即判定过程"这个方法本身**不是我们的贡献**。

> **定位含义**：这条线是我们最好的对照组，也是最危险的对照组。
> 我们能说的是：*他们必须逼近，因为域是连续的；我们不必，因为域是有限全函数。
> 我们把他们十年才解决不了的验证问题，通过选择问题而不是选择工具，变成了平凡问题。*
> 我们**不能**说：我们第一个用枚举验证网络，或者我们第一个用网络替代查表。

### a.2　Tracr —— **把程序编译成权重，构造而非训练；其 MLP 构造与我们的 P-8 实质相同**

Lindner, Kramár, Farquhar, Rahtz, McGrath, Mikulik, *Tracr: Compiled Transformers as a
Laboratory for Interpretability*, NeurIPS 2023 (arXiv:2301.05062)，代码 google-deepmind/tracr。
把 RASP 程序（Weiss, Goldberg, Yahav, *Thinking Like Transformers*, ICML 2021）编译成 transformer 权重。

**为什么这是对 P-8 的直接冲击**：Tracr 编译 elementwise 操作的办法，就是
"generating matrix lookup tables for arbitrary functions with a **finite domain**" ——
即：对有限域上的任意函数，用 one-hot 编码 + 一层 MLP 精确实现它。
这**就是**我们 §1.4 P-8 的构造（每个 key 一个隐单元，`b1 = −(m − 0.5)`，`W2[u_k, f(k)] = 1`）。

**我们与它的差别：**
- Tracr 的目标是**可解释性研究的 ground truth**，不是造一个能用的系统；它不做穷举等价验证，
  也不主张"没有回退"（它压根没有需要回退的东西）。
- Tracr 编译的是 RASP（一个受限的、为 transformer 设计的 DSL），不是"真实系统里的决策表"。
- Tracr 不自举，不产出机器码，不关心整数/无乘法。

> **定位含义**：**P-8 必须降级为"已知构造"，标注 folklore 并引用 Tracr 与 Omlin–Giles。**
> prd.md §6.0 把 P-8 列为"已证明（构造式，11/11 机器验证）"是对的，但把它放在结果总表第一行、
> 暗示它是本工作的定理，会被审稿人当场指出。

### a.3　Omlin & Giles 1996 —— **"构造而非训练、并证明在离散域上精确"的原始文献**

Omlin & Giles, *Constructing Deterministic Finite-State Automata in Recurrent Neural Networks*,
Journal of the ACM 43(6), 1996。把 DFA 的转移关系**直接编码**进二阶 RNN 的一小部分权重
（取 ±H），并**证明**内部状态表示稳定、对任意长度输入分类正确。

**这是 C1 的真正先行者，比 Tracr 早 27 年**。同一条线上的近作：
- Dhayalkar, *Neural Networks as Universal Finite-State Machines: A Constructive Deterministic
  Finite Automaton Theory*, arXiv:2505.11694 (2025)：构造式证明有限深度 ReLU/threshold 网络
  可以**精确模拟** DFA，给出 depth/width/state-compression 的显式界。**完全是构造，不训练。**
- *Symbolic Feedforward Networks for Probabilistic Finite Automata*, arXiv:2509.10034 (2025)。

**我们与它的差别**：他们做的是**一个**离散对象（自动机）的精确神经化；
我们做的是**一整条编译流水线上十个决策点**的精确神经化，并且把它接进一个真实系统、自举、跑出可执行文件。
差别是"系统 vs 定理"，不是"新定理 vs 旧定理"。

### a.4　Learned Index Structures 及其后裔 —— **我们自己点名的近亲，差别确实成立**

- Kraska, Beutel, Chi, Dean, Polyzotis, *The Case for Learned Index Structures*, SIGMOD 2018
  (arXiv:1712.01208)。"索引就是模型"：B-tree 是 key→position 的模型，hash index、bitmap index 同理。
- Mitzenmacher, *A Model for Learned Bloom Filters, and Optimizing by Sandwiching*, NeurIPS 2018
  (arXiv:1803.01474)。
- RMI / PGM-index / ALEX / CARMI 等后续。

**关键事实，我们的差异点在这里，而且站得住：**
learned index 的正确性**从来不来自模型**。RMI/PGM 在叶子处一定有一次
**"last-mile search"**（在 `2ε+1` 的窗口里做二分/线性查找）来纠正模型预测误差；
learned Bloom filter 一定有一个 **backup filter** 装下被模型误判为不存在的 key，
否则会出现假阴性。换句话说：**模型提供速度，回退提供正确性。**

我们的 P-1 / P-1a 明确禁止任何形式的回退、ensemble、"低置信度回退 gold"、读 logits。
在我判断的范围内，**这是一条真实的、可陈述的差别**，而且 learned index 社区自己不会反对这个陈述。

> 注意分寸：这条差别之所以成立，**不是因为我们的模型更好，而是因为我们的问题更小**。
> Learned index 面对的是任意大的、开放的、随时插入的 key 空间；我们面对的是 6–960 行的封闭表。
> 论文里必须自己先说出这句话，否则会被审稿人说成"用玩具问题去碰瓷"。

### a.5　"网络 ⇄ 离散表示"的精确互译 —— **双向都早已建立**

- Aytekin, *Neural Networks are Decision Trees*, arXiv:2210.05189 (2022)：任意分段线性激活的
  前馈网络（含卷积、skip、normalization）可以**精确等价**地写成决策树，不是逼近。
- Choi, Shi, Shih, Darwiche, *Compiling Neural Networks into Tractable Boolean Circuits*, 2019：
  二值输入 + step 激活的网络 → Boolean circuit → 知识编译成 OBDD/SDD，之后解释和验证都变廉价。
- Lal et al., *NN2Rules: Extracting Rule List from Neural Networks*, arXiv:2207.12271 (2022)：
  抽出的规则对**任意输入**都与网络预测相同。
- 更早：TREPAN、Sato & Tsukimoto (2001) 等 rule extraction 传统。

**含义**：「小网络 ≡ 表 ≡ 决策树 ≡ 布尔电路」在文献里是**双向已知**的常识。
我们做的是把其中一个方向（表 → 网络）用在一个具体系统上，并把等价性**机器检查**了一遍。
不要把"网络能精确表示表"当成发现。

### a.6　LUT-based NN / 可微逻辑门网络 —— **"网络和查表是同一个东西"的硬件侧证据**

- Umuroglu, Preußer, Blott 等，*LogicNets*（arXiv:2004.03021）；NullaNet；LUTNet；PolyLUT；NeuraLUT。
  把量化后的神经元**完整枚举**成 L-LUT 真值表，"ensures **no accuracy loss** during the mapping
  from the quantized theoretical model to the actual hardware"。方向与我们相反（网络 → 表），
  但精确性诉求一模一样。
- Petersen, Borgelt, Kuehne, Deussen, *Deep Differentiable Logic Gate Networks*, NeurIPS 2022：
  学习逻辑门组合，离散化后是纯逻辑电路，CPU 单核 >1M MNIST 图/秒。
- Ghoukasian & Kratsios, *Certifiably Interpretable Training of ReLU-MLPs for Boolean Tasks with
  Guaranteed Truth-Table Generalization*, arXiv:2609.13439 (2026)：MACCHIATO 算法把 Boolean 函数
  的残差投影到 {AND,OR,XOR} 低维电路类上，再**编译成 ReLU-MLP**，并给出 truth-table error 的界。
  注意：它是**训练 + 编译**的混合，而且给的是 `O(√(m(B+log(m/δ))/T))` 的**统计界**，不是精确等价。

**含义**：我们的 C3（二值权重、无乘法、int8）在这个社区里是**入门配置**，不构成贡献。

---

## (b) 相关但不同（按研究线分组）

### b.1　ML for Compilers —— 调的是启发式，不是正确性

- Trofin et al., *MLGO: a Machine Learning Guided Compiler Optimizations Framework*,
  arXiv:2101.04808 (2021)。已进 LLVM 主干并在生产部署：inlining-for-size（RL，最多 7% size 降低）
  与 register allocation。**第一个在真实工业编译器复杂 pass 中完整集成 ML 的工作。**
- Cummins et al., *CompilerGym*, CGO 2022 (arXiv:2109.08267)；
  Huang et al., *AutoPhase*, FCCM 2019 / MLSys 2020 (arXiv:1901.04615, 2003.00671)：phase ordering。
- Mendis et al., *Ithemal*, ICML 2019 (arXiv:1808.07412)：basic block 吞吐量预测，最坏平均误差 10.53%。
  GRANITE (arXiv:2210.03894) 是其 GNN 后继。
- Neural Instruction Combiner (NIC)，LLVM Dev Meeting 2022：seq2seq 做 instruction combine。
- Marcus et al., *Neo: A Learned Query Optimizer*, VLDB 2019；*Bao*, SIGMOD 2021。
  两者都明确说明：**正确性由 rewrite rule / hint set 保证，不由模型保证**
  （"Given a set of query rewrite rules to ensure semantic correctness"）。

**差别（这是我们最干净的一句话）：** 上述全部工作里，**任何一个选择都是正确的**，
模型只影响快慢/大小；选错了代码依然能跑。我们替换的决策点**只有一个选择是正确的**，
选错了 = 编译出错误的程序。这两类问题的验证义务完全不同，**这个区分在文献里没有被系统地提出来过**
（我没找到一篇把 "performance-critical decision" 与 "correctness-critical decision" 作为
ML-for-compilers 的分类轴来立论的论文）。这可以作为论文的一个组织性贡献。

### b.2　Learned Systems Components —— 平均情况，全都带回退

- Jiménez & Lin, *Dynamic Branch Prediction with Perceptrons*, HPCA 2001 —— 系统里最早的 ML，
  但预测错了只是流水线冲刷，**没有正确性后果**。
- Teran, Wang, Jiménez, *Perceptron Learning for Reuse Prediction*, MICRO 2016；
  Shi et al., *Applying Deep Learning to the Cache Replacement Problem* (Glider), MICRO 2019；
  Hashemi et al. 的 LSTM prefetcher；Ipek 的 RL memory controller。
- 综述：*A Survey of Machine Learning Applied to Computer Architecture Design*, arXiv:1909.12373。

**共同点**：这些位置在架构上**天然允许出错**（cache miss、误预测、无效 prefetch 都只是性能损失）。
这解释了为什么"精确性"在这个社区不是一个被追求的性质 —— 不是做不到，是不需要。

### b.3　Algorithms with Predictions —— 把"必须有回退"这件事形式化了

Lykouris & Vassilvitskii；Mitzenmacher & Vassilvitskii, *Algorithms with Predictions*
(arXiv:2006.09123)；Wei & Zhang, *Optimal Robustness-Consistency Trade-offs*, NeurIPS 2020
(arXiv:2010.11443)。核心概念 **consistency**（预测准时的表现）与 **robustness**（预测任意坏时的最坏界），
并证明两者之间存在**必然的 trade-off**。

**为什么要引它**：这是整个"学习组件 + 正确性"领域的理论骨架，
它精确地说明了 learned index / learned Bloom filter 为什么必须保留回退。
我们应该引用它并说：*在有限全函数域上，consistency–robustness trade-off 退化 —— consistency = 1，
robustness 由同一份权重承担，因为定义域上不存在"预测错"的点可以被对抗性地挑出来。*
这是一句可以写进 abstract 的话。

### b.4　神经网络形式化验证 —— 我们的验证之所以平凡，值得说清楚

- Katz et al., *Reluplex*, CAV 2017（已下载）；Katz et al., *Marabou*, CAV 2019 /
  Wu et al., *Marabou 2.0*, CAV 2024；α-β-CROWN（VNN-COMP 2021–2025 连续冠军）；ERAN / DeepPoly；
  nnenum；MN-BaB；NNV。
- 复杂度事实：即便单输入单输出、有界连续域，NN 验证仍是 NP-hard。
- **例外，也就是与我们同法的那篇**：Jia & Rinard, SAS 2021（见 a.1）—— 输入量化 → 状态枚举 → 精确结论。
- 量化网络验证：*Verifying Quantized Neural Networks using SMT-Based Model Checking*
  (arXiv:2106.05997)；CEG4N (arXiv:2207.04231)。
- ReLU 网络的 model checking 视角：Zhang et al., JCST 2020；可判定性结果（EXPSPACE 等）。

**我们要说的话**：我们的 P-3/P-4 不需要 PAC bound、Lipschitz 常数、鲁棒半径 —— prd.md 已经这么写了，
这个说法是对的，但**它不是我们的发现**，而是"有限离散域"这个前提的直接后果，
Jia & Rinard 已经在 ACAS Xu 上演示过。我们的贡献是**主动把系统设计成让这个前提成立**，
而不是事后发现它成立。

### b.5　神经程序执行 / 神经 CPU

- de Jesus Silva, *A Symbolic Neural CPU for Quantization-Simulated Writeback and Interpretable
  Program Execution*, arXiv:2607.10021 (2026)：trace-supervised 的学习型执行器，
  operation router + 固定可微 ALU bank。非量化执行器"reproduces reference execution exactly"，
  8-bit 版本在 1000 条指令内保持 symbolic operation path。**但它是训练出来的**
  （behaviour cloning + actor-critic），且正确性以"matched fixed-point replay"对照验证。
- 更早的线：Neural Turing Machines / DNC；Neural Programmer-Interpreters；
  *Neural Networks and the Chomsky Hierarchy*；*Algorithmic Language Models with Neurally
  Compiled Libraries* (arXiv:2407.04899)。
- *Neural Computers*, arXiv:2604.06425 (2026)。

**差别**：这些把**执行**神经化（我们的 [T-1] 明确禁止的部分），我们只把**选表**神经化。
我们应该主动引用它们来界定 [T-1] 的边界：不是做不到，是刻意不做，因为走查器是代数不是表。

### b.6　"Programs as Weights" 这个说法本身

Zhang, Hotsko, Kim, Nie, Shieber, Deng, *Program-as-Weights: A Programming Paradigm for Fuzzy
Functions*, arXiv:2607.02512 (2026)：4B compiler 把自然语言 spec 编译成 adapter，
交给冻结的 0.6B 解释器执行；0.6B + PAW ≈ 直接 prompt 32B，内存 1/50。

**注意**：这篇明确把"program 就是权重本身，而不是含 API 调用的文本"当作卖点，
并承认 "Representing programs in neural networks is a long-standing direction, with some work
compiling formal code into network weights"（即 Tracr 一系）。

**差别**：PAW 处理的是 **fuzzy function**（日志告警、JSON 修复、意图排序）—— 恰恰是
"没有唯一正确答案"的任务；我们处理的是有唯一正确答案的任务。
**"Shell = 推理器 + 执行器 + 模型数据" 这个口号与 PAW 的口号高度重合，论文里必须显式区分，
否则会被认为是同一个想法的弱化版。**

### b.7　神经 LR parser（我们的 `parse` 阶段有直接先例）

Chan 及合作者，*Extracting error productions from a neural network-based LR parser*,
Neurocomputing 2002（Elsevier，**付费墙，我只读到摘要**）：
NNLR —— 用前馈网络模拟 LR parser 的 shift-reduce 决策；只用少量合法句训练，却能 parse 大量错误句；
"erroneous sentences are recovered as if the parser had filled in some of the empty slots in the
original LR parsing table"。另有 SARDSRN (Mayberry & Miikkulainen, IJCAI 1999)。
近作方向相反：Findings of ACL 2021 的 *Grammar-Constrained Neural Semantic Parsing with LR Parsers*
是用 LR 表去**约束**神经解码器。

**差别**：NNLR 的目标是**鲁棒性/错误恢复**（故意让网络去填表里的空格），
我们的 D-7/F-5 恰恰相反：网络与 gold 不一致 = bug，不是"泛化"。方向是对立的，值得引用作对比。

### b.8　二值/量化/无乘法网络（C3 的先行技术）

Courbariaux & Bengio et al., *Binarized Neural Networks*, arXiv:1602.02830 / NeurIPS 2016；
Zhou et al., INQ（2 的幂权重）；Gudovskiy & Rigazio, ShiftCNN；
Elhoushi et al., *DeepShift: Towards Multiplication-Less Neural Networks*, arXiv:1905.13298；
DenseShift (arXiv:2208.09708)；bitshift quantization 综述。

**结论**：二值权重、2 的幂权重、纯整数累加、无乘法 —— 每一条都有 5–10 年的成熟文献。
C3 不能作为贡献陈述，只能作为"构造的副产品"提一句。

### b.9　最小性问题（P-8a / P-8c）= 经典的两级逻辑最小化

这一点 prd.md §1.4 已经自己点到了（"最小 DNF 是 NP-hard"），但引用需要补全：

- Quine (1952) / McCluskey (1956)：最小 DNF 问题的提出与精确算法。
- Masek (1979)：给定完整真值表求最小 DNF 是 **NP-complete**。
- Brayton, Hachtel, McMullen, Sangiovanni-Vincentelli, *Logic Minimization Algorithms for VLSI
  Synthesis* (1984) —— **Espresso**；以及 Rudell & Sangiovanni-Vincentelli 的
  *Multiple-Valued Minimization for PLA Optimization*（多值最小化，**这正好对应我们的多字段 key**）。
- Umans；Allender, Hellerstein, McCabe, Pitassi, Saks，
  *Hardness of approximate two-level logic minimization and PAC learning with membership queries*,
  JCSS 2008 —— 连**近似**都难。
- 记忆容量侧的上下界：Baum (1988)；Yun, Sra, Jadbabaie, *Small ReLU networks are powerful
  memorizers*, NeurIPS 2019 (arXiv:1810.07770) —— 3 层 ReLU 只要 **Θ(√N)** 宽即可记住 N 个点，
  且这个界是**紧的**；L 层网络 W = Ω(N) 个参数即可。
  另有 *Memorization Capacity of Neural Networks with Conditional Computation* (arXiv:2303.11247)、
  *Generalizability of Memorization Neural Networks* (arXiv:2411.00372)。

**这对我们意味着什么（重要）：**
1. P-8 给出的上界 `h = |K|` 是**朴素上界**，文献里已知可以降到 `Θ(√|K|)` 量级（对一般标注）。
   我们在总表里把 `h_构造` 当成"已知实例，精确"是诚实的，但不能暗示 `|K|` 是自然上界。
2. P-8a "h_min 的组合刻画"**就是**多值两级逻辑最小化的一个变体，不是一个全新的开放问题。
   它是一个有 70 年历史、已知 NP-hard、已知难近似、已有工业级启发式（Espresso）的问题。
   写论文时应当说"我们把 h_min 归约到/联系到 two-level logic minimization"，而不是"这是开放问题"。
3. P-8c "是否存在 O(h_min) 的确定性构造算法" —— 在 Allender et al. 的难近似结果下，
   答案在一般情况下是**否**（除非复杂性类塌缩）。这一条应该直接改写成
   "对我们这十张表的实例，Espresso 类启发式能压到多小"这种可回答的问题。

### b.10　P-8b（精确解存在但 SGD 不可达）—— 这是文献里最成熟的一块，不是"论文里最硬的一块"

prd.md §6.0 把 P-8b 称为"论文里最硬的一块"。**这个判断需要修正**：

- Shalev-Shwartz, Shamir, Shammah, *Failures of Gradient-Based Deep Learning*, ICML 2017
  (arXiv:1703.07950)：梯度在大范围目标函数上**几乎相同**，因此不携带关于正确方向的信息。
- Shamir, *Distribution-Specific Hardness of Learning Neural Networks*, JMLR 2018：
  梯度方法"卡在"次优点。
- Abbe & Sandon (2018)：parity 对梯度类算法难学。
- Malach & Shalev-Shwartz, *When Hardness of Approximation Meets Hardness of Learning*,
  arXiv:2008.08059：一条 hardness 性质同时蕴含逼近难与梯度学习难，覆盖 parity / DNF / AC0。
- Barak et al., *Hidden Progress in Deep Learning: SGD Learns Parities Near the Computational
  Limit*, NeurIPS 2022 (arXiv:2207.08799)。
- *Superpolynomial Lower Bounds for Learning One-Layer Neural Networks using Gradient Descent*
  (arXiv:2006.12011)；*Intractability of Learning the Discrete Logarithm with Gradient-Based
  Methods* (arXiv:2310.01611)。

**结论**："存在精确解但 SGD 在任意预算内不可达"**不是开放问题，是已建立的定理家族**。
我们的 E-18（s2：12.8× 参数仍卡在 0.8143）与 E-25（活板门机理）是**一个具体实例与一个具体机理**，
价值在于"在真实编译器表上观察到了这个现象"，而不是"我们发现了这个现象"。
论文里必须把 Shalev-Shwartz 2017 / Malach 2020 放在这一段的第一句，否则会被当成不读文献。

### b.11　其他扫过但关系较远的

- 神经网络 ↔ 沙箱 / 安全策略：*Sandboxing (AI-based) Unverified Controllers in Stochastic Games:
  Safe-visor Architecture* (arXiv:2203.14924)、*Proof-Carrying Neuro-Symbolic Code*
  (arXiv:2504.12031)。这些是"用形式化方法把不可信的神经控制器**包起来**"，
  即架构性回退（shield / safe-visor），与我们的无回退立场相反，是很好的对照。
- 自举编译器传统：NELIAC (1958)、Burroughs B5000 Algol (1961)、LISP (1962)、GCC/tcc 的 bootstrap，
  以及可复现构建（reproducible builds）社区对"逐字节相同"的既有要求。
  我们的 D-5/D-6 在这个传统里是标准要求，不是新要求；新的只是"里面装了模型"。
- Weight Agnostic Neural Networks (arXiv:1906.04358)、Universal Neural Functionals (NeurIPS 2024)：
  与"换权重即换能力"表面相似，实质无关。

---

## (c) 我们的主张里哪些是新的、哪些不是

**直说版本。**

### 不新的（必须放弃或降级为"已知"）

| 我们的说法 | 实际情况 | 必引 |
|---|---|---|
| P-8 存在性定理（one-hot + 每 key 一个隐单元） | 教科书级构造。Tracr 明确做"finite domain 的 matrix lookup table"；embedding 层本来就是 one-hot × 矩阵，即查表本身 | Lindner 2023；Omlin & Giles 1996；Baum 1988 |
| `h_构造 = |K|` 是自然上界 | 朴素界。已知 Θ(√N) 宽度紧界 | Yun et al. 2019 |
| "能不能 100%"不是开放问题 | 对，但这一点早就不是开放问题，不是我们关闭的 | 同上 |
| P-8a：h_min 的组合刻画是开放问题 | 它**是**（多值）两级逻辑最小化，NP-complete 且难近似，有 Espresso 等工业启发式 | Masek 1979；Brayton et al. 1984；Allender et al. 2008 |
| P-8c：是否存在 O(h_min) 确定性构造 | 一般情况下已被难近似结果排除 | Umans；Allender et al. 2008 |
| P-8b：精确解存在但 SGD 不可达，"论文里最硬的一块" | 已建立的定理家族，不是开放问题 | Shalev-Shwartz et al. 2017；Shamir 2018；Malach & Shalev-Shwartz 2020；Barak et al. 2022 |
| 全域枚举验证 = 决策过程，不需要 PAC/Lipschitz | 正确，但方法已被做过（输入量化 + 状态枚举，对象正是 ACAS Xu） | Jia & Rinard, SAS 2021 |
| 纯整数、二值权重、2 的幂、无乘无移、int8 累加 | 全部是成熟技术 | Courbariaux 2016；INQ；ShiftCNN；DeepShift；LogicNets |
| 用网络替代查表 | 2016 年 ACAS Xu 就是这么做的，并且引出了整个 NN 验证领域 | Julian et al. 2016；Katz et al. 2017 |
| "网络能精确等价于表/树/电路" | 双向已知 | Aytekin 2022；Choi et al. 2019 |
| "程序即权重" | 同名 paradigm 已存在（fuzzy function 方向） | Zhang et al. 2026 |
| 神经化 parse 表 | NNLR 早有（但目标相反：错误恢复） | Chan et al., Neurocomputing 2002 |

### 可能是新的（我搜索范围内未找到先例）

1. **系统层面的组合**：一个**自举的、真实的 C99 编译器**，其**全部**表形状决策点（10 个，
   预处理 / 词法 / 语法 / 类型组合 / 作用域 / IR 选择 / 指令选择 / ABI / 编码形式 / 重定位）
   都由**同一个 kernel** 的不同权重实现，**全域枚举等价**，**全链路无回退**，并产出 6 目标可执行文件。
   我搜了 PLDI/POPL/OOPSLA/CGO/ASPLOS/OSDI/SOSP 方向的关键词组合，没找到任何接近的系统。
   这是**工程/系统贡献**，不是理论贡献。
2. **"无回退"作为硬契约**（P-1 / P-1a / P-1b：禁读 logits、禁 ensemble、禁低置信度回退、
   权重加载时一次性解析门禁）。learned systems 文献里**普遍存在回退**，而且
   algorithms-with-predictions 从理论上说明了为什么必须有。
   把"无回退"写成规格条款并据此得到组合等价（P-5 由同余直接成立，无需对走查器归纳）——
   我没找到先例。这更像是一个**方法论贡献 / 设计纪律**。
3. **嵌模型的自举不动点**：`A = cc(unisacc.c)`, `B = A(unisacc.c)`, `C = B(unisacc.c)`, `B == C`
   逐字节成立，且编译器源码里带着 10,514 字节的模型 blob。
   自举编译器和可复现构建都是老传统，**但"编译器带着自己的神经权重自举"我没有找到任何先例**。
4. **correctness-critical vs performance-critical 决策的分类轴**，用来组织整个
   ML-for-compilers 领域。这个区分是显然的，但我没找到有人把它作为立论的组织原则写出来。
5. **把 consistency–robustness trade-off 在有限全函数域上的退化说清楚**（见 b.3）。
   这是一句可以写进 abstract 的、有理论味道但成本极低的话。

### 明确站不住的

- **"神经比表省空间"** —— §6 E-3′ 已自行证伪。而且 Boniol et al. NFM 2026 用 BDD 对 ACAS-Xu 做
  **精确**压缩，并直接论证"NN 方案因为逼近而不保语义"。任何以体积为卖点的段落都会被这篇打掉。
  README 里"比它替代的手写 switch 更小"这句话**必须删掉或加限定**。
- **"每个隐单元一个 key"的网络本质上就是那张表，只是换了编码。** 审稿人一定会说这句话，
  而且他说得对。我们唯一的防线是**接口统一性**（一个 kernel、换权重即换能力、可热插拔），
  而不是压缩、不是学习、不是泛化。但"表驱动编译器"本身是古老技术（yacc/LALR 表、
  PLA 驱动的微码、指令编码表），所以这条防线也要小心地说：
  *我们把十张异构的表统一成一种同构的数据 + 一个 22 行的整数 kernel*，这是可以量化的（kernel 行数、
  接口数量），也是可以被接受的贡献；*"我们用了神经网络"不是*。

---

## (d) 应该怎么定位

### d.1　一句话定位（建议）

> 我们不是在展示神经网络能做编译器的决策；**那是 1996 年就证明了的（Omlin–Giles），
> 2023 年被 Tracr 做成了工具**。我们展示的是：**当一个系统的决策点被刻意限制成有限离散全函数时，
> 学习系统一贯需要的"模型 + 纠错回退"结构可以被完全取消，正确性义务从统计问题塌缩成模型检验问题，
> 而这件事在一个自举的、六目标的真实 C99 编译器上端到端成立。**

### d.2　三条不该讲的话

1. 不要讲"我们证明了存在性"（P-8）。改讲"我们采用已知构造，并给出十张真实表上的实例化"。
2. 不要讲"我们第一个用枚举验证网络"。改讲"我们把系统设计成使枚举验证充分"。
3. 不要讲体积优势，除非旁边同时给出 BDD / 原表 / tcc 三条基线，并诚实报告 E-3′。

### d.3　三条该讲的话

1. **问题选择即验证策略**（design-for-verifiability）：正确性不是事后验的，是靠把定义域限制成
   有限全函数**设计出来的**。对照 ACAS Xu 十年的反例（Bak & Tran 2022）。
2. **零回退契约**及其推论：P-5 由 P-3 + P-1 同余直接得到，**不需要对走查器做归纳**。
   这是一个干净的、可被形式化社区理解的论证结构，也是与 learned index / learned Bloom filter
   的结构性差异。
3. **一个 kernel、十个能力**：把"换权重即换能力"落到可度量的指标上
   （kernel 代码行数、接口数、模型 blob 字节数、覆盖的决策点数），并与手写 switch 的
   行数/分支数对比 —— 比体积更有说服力的是**接口熵**而不是**字节数**。

### d.4　投稿方向

- **不适合** NeurIPS / ICLR：理论部分（P-8、P-8b）全是已知结果，会被直接拒。
- **适合**：CC / CGO / Onward! / OOPSLA 的 experience / artifact 方向；或 PLDI 的短文；
  或 NFM / CAV 的 case-study 方向（因为 ACAS Xu 那条线在那里，对照最锋利）。
  标题方向建议：*"Design for Decidability: An Exactly-Verified, Fallback-Free Neural Decision
  Layer in a Self-Hosting C99 Compiler"*。

### d.5　一篇论文所需的最小引用集

**必引（不引会被认为没读文献）**
Kraska et al. SIGMOD'18 · Mitzenmacher NeurIPS'18 · Julian et al. DASC'16 · Katz et al. CAV'17 ·
Bak & Tran NFM'22 · Jia & Rinard SAS'21 · Boniol et al. NFM'26 · Lindner et al. NeurIPS'23 ·
Weiss et al. ICML'21 (RASP) · Omlin & Giles JACM'96 · Trofin et al. (MLGO) ·
Shalev-Shwartz et al. ICML'17 · Yun et al. NeurIPS'19 · Masek'79 + Brayton et al.'84 (Espresso) ·
Mitzenmacher & Vassilvitskii (algorithms with predictions)

**应引（用于界定边界）**
Aytekin'22 · Choi et al.'19 · Umuroglu et al. (LogicNets) · Petersen et al. NeurIPS'22 ·
Courbariaux et al.'16 · Marcus et al. (Neo/Bao) · Cummins et al. (CompilerGym) ·
Mendis et al. (Ithemal) · Jiménez & Lin HPCA'01 · Allender et al. JCSS'08 ·
Malach & Shalev-Shwartz'20 · Dhayalkar'25 · Zhang et al.'26 (Program-as-Weights) ·
Chan et al. (NNLR, Neurocomputing'02)

---

## (e) 还没找到的（负面证据，逐条列明搜了什么）

以下每一条都是 **"我搜索了 X、Y、Z，没有找到"**，**不是** "不存在"。
2026 年的新工作、非英语文献、未索引的个人项目、公司内部工作都不在覆盖范围内。

1. **一个把编译器流水线上所有表形状决策点都神经化的系统。**
   搜索：`neural compiler decision table replaced by network exact`、
   `neuro-symbolic compiler neural lexer tokenizer exact deterministic C compiler neural network stages`、
   `neural network ABI calling convention relocation code generation learned decision table exact compiler`、
   `neural network instruction selection compiler backend learned instruction decoder`。
   **结果：没有找到。** 找到的最接近的是单点的 NNLR（parse 表）与 NIC（instruction combine），
   两者都是统计性的、以鲁棒性或性能为目标。

2. **自举的、内嵌神经权重的编译器。**
   搜索：`self-hosting compiler that embeds a neural network model bootstrap fixed point`、
   `neural compiler hobby project bootstrap`、GitHub / Hacker News 方向的组合词。
   **结果：没有找到。** 只找到常规自举编译器文献与 `mattrickard.com/self-hosted-compilers-and-bootstrapped-ai`
   一类的类比性博客（讲 LLM 自举，不是权重嵌进编译器）。

3. **在生产系统中部署的、无任何回退路径的学习组件。**
   搜索：`learned system component without fallback exact correctness by construction`、
   `neural surrogate replaces table verified by exhaustive enumeration no fallback`。
   **结果：没有找到。** 相反地，找到的每一个（learned index 的 last-mile search、
   learned Bloom filter 的 backup filter、ACAS Xu 的 safety net、Neo/Bao 的 rewrite rules、
   safe-visor / shield 架构）都显式保留回退。这是一个**强负面证据**，可以写进论文。

4. **"correctness-critical vs performance-critical 决策"作为 ML-for-compilers 的分类轴。**
   搜索：ML for compilers 综述（`awesome-machine-learning-in-compilers` 清单全表扫过）、
   MLGO / CompilerGym / AutoPhase 的 related work。
   **结果：没有找到**有人把这个区分作为组织原则明确提出。这个区分本身很显然，
   所以"没找到"更可能意味着"大家默认如此、不值得写"，而不是"这是新洞见"。

5. **"一个 kernel + 可热插拔权重 = 能力层"作为系统架构命题。**
   搜索：`one kernel swap weights change capability neural network universal interpreter
   model data capability layer`。
   **结果：没有找到**。搜到的 Universal Neural Functionals / Weight Agnostic NN 与此无关。
   Program-as-Weights (2026) 在口号层面最接近，但针对 fuzzy function。

6. **对"有限全函数域上 consistency–robustness trade-off 退化"的显式陈述。**
   搜索：algorithms with predictions 的 trade-off 文献
   （arXiv:2010.11443、2501.12770、2006.09123）。
   **结果：没有找到**有人把"定义域有限且封闭 ⇒ trade-off 消失"单独提出来。
   但这在该领域是平凡推论，不应作为贡献声称，只宜作为一句 framing。

7. **对编译器决策表做最小 DNF / Espresso 式最小化并与神经网络宽度对照的工作。**
   搜索：`two-level logic minimization Espresso minimal DNF NP-hard PLA microcode table`、
   `minimal DNF neural network hidden units lower bound`。
   **结果：没有找到**把 `h_min(网络)` 与 `最小 DNF 项数` 建立起显式对应的文献。
   这可能是 P-8a 真正可做的一小块：**把 h_min 归约到多值两级逻辑最小化，然后用 Espresso 给上界、
   用已知的 DNF 下界技术给下界。** 如果这条对应关系成立且没被写过，它比"h_min 是开放问题"值钱得多。

8. **付费墙 / 未能读到全文的条目**（只记录引用与摘要，未捏造内容）：
   - Chan et al., *Extracting error productions from a neural network-based LR parser*,
     Neurocomputing 2002（Elsevier）—— 仅读到摘要。
   - Teran, Wang, Jiménez, *Perceptron Learning for Reuse Prediction*, MICRO 2016 ——
     读到作者主页 PDF 的片段，未通读。
   - Marcus et al., *Bao*, SIGMOD 2021（ACM DL）—— 读到摘要与二手描述。
   - Julian et al., DASC 2016 —— PDF 已下载但为图像/编码型 PDF，本机无 poppler，
     未能抽取正文；结论基于摘要、Semantic Scholar 条目与多篇引用它的论文的转述。

---

## 已下载论文索引

目录：`research/papers/`

| 文件 | 一行说明 |
|---|---|
| `1996-omlin-constructing-dfa-in-rnn.pdf` | Omlin & Giles, JACM'96：把 DFA **构造**进 RNN 权重并证明稳定精确 —— C1 的原始先行者 |
| `2016-julian-policy-compression-acasxu.pdf` | Julian et al., DASC'16：用 DNN 压缩 ACAS Xu 的 GB 级查表 —— "网络替代表"的经典起点 |
| `2017-katz-reluplex.pdf` | Katz et al., CAV'17：Reluplex，为验证 ACAS Xu 网络而生的 SMT 求解器 |
| `2018-kraska-learned-index-structures.pdf` | Kraska et al., SIGMOD'18：learned index —— 我们点名的最近亲，但保留纠错路径 |
| `2018-mitzenmacher-sandwiching-learned-bloom.pdf` | Mitzenmacher, NeurIPS'18：learned Bloom filter 必须有 backup filter 才无假阴性 |
| `2019-choi-compiling-nn-boolean-circuits.pdf` | Choi/Shi/Shih/Darwiche：网络 → 布尔电路 → OBDD/SDD，精确互译的反方向 |
| `2019-yun-small-relu-memorizers.pdf` | Yun/Sra/Jadbabaie, NeurIPS'19：记忆 N 点只需 Θ(√N) 宽且为紧界 —— 我们的 h 上界太松 |
| `2020-umuroglu-logicnets.pdf` | LogicNets：量化神经元**完整枚举**成 LUT 真值表，映射零精度损失 |
| `2021-jia-input-quantization-verification.pdf` | Jia & Rinard, SAS'21：输入量化 + **状态枚举**验证 ACAS Xu —— 与我们 P-3 同法，且更早 |
| `2021-trofin-mlgo.pdf` | MLGO：ML 替换 LLVM 启发式并进入生产 —— "任何选择都正确"的对照组 |
| `2022-aytekin-nn-are-decision-trees.pdf` | 任意分段线性网络**精确等价**于决策树（非逼近） |
| `2022-petersen-deep-differentiable-logic-gate-networks.pdf` | NeurIPS'22：学习逻辑门网络，离散化后是纯电路，CPU 上 >1M img/s |
| `2023-lindner-tracr.pdf` | Tracr：把 RASP 程序**编译**成 transformer 权重；其有限域 lookup 构造 = 我们的 P-8 |
| `2025-dhayalkar-nn-universal-fsm.pdf` | 构造式证明 ReLU/threshold 网络可**精确**模拟 DFA，给出 depth/width 显式界 |
| `2026-boniol-acasxu-bdd-compression.pdf` | NFM'26：用 BDD **精确无损**压缩 ACAS-Xu 表，并直指 NN 方案"不保语义" —— 我们体积论证的直接威胁 |
| `2026-ghoukasian-truth-table-generalization.pdf` | MACCHIATO：Boolean 任务的 ReLU-MLP 训练 + 电路编译，但给的是**统计界**不是精确等价 |
