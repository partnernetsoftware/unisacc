# 表即网络：以构造而非训练得到权重的逐字节自举（N1=N2=N3）的 C 编译器

**Tables as Networks: A Byte-for-Byte Self-Hosting C Compiler Whose Weights Are Constructed, Not Trained**

2026-10-16 · 源身份 main `910f1e0a`，版本串 `0.0.42`（附录 A）。

**arXiv（投稿时）：** 主类 **cs.AI**；可选交叉 **cs.PL**。

## 摘要

unisacc 是 neural-network-based compiler：编译阶段由整数阈值网络与通用执行器运行，权重由 TSV 表 DSL 构造，不由训练得到。有限决策写成全函数真值表；T1 是声明域上逐键相等（定义 2），不是样本拟合。两层分开：决策网络实例是 18 个决策阶段；网络编译器实例是七个字节流阶段（pp、lex、parse/parse2、opt、prune、lowering、enc）。域外键拒绝，不回退。决策网络实例在五个具名目标上 N1 = N2 = N3（未签名连续代字节相同，不是语义自举）。键身份：op-74 合计 8,509 键，op-87 合计 8,769 键，封口前不可互换。源身份 `910f1e0a` / `0.0.42` 不是密封发布测量。系统训练对照见预注册 A2。枚举证明网络 = 表，不证明表 = C。

**关键词**：neural-network-based compiler；TSV 表 DSL；权重构造；T1；有限控制；N1=N2=N3

## 1　贡献

1. **方法。** TSV 表 DSL → 构造权重 → 声明域穷举（T1）。权重不是训练得到的。
2. **两层。** 网络编译器合同是七个字节流阶段，由一个通用执行器 E 运行有限转移 δ。决策网络实例是另外 18 个决策阶段，在询问点查构造网络（或由其穷举得到的 DENSE 表）。
3. **实现。** 六目标 C99 子集编译器 unisacc。决策网络实例不经 Python 在五个目标上逐字节自举。源身份见附录 A。

不声称：表 = C99、整机语义保持（T3）、或速度优势。

## 2　定义

**定义 1（决策阶段）。** 阶段 $s$ 有字段 $(f_1{:}V_1,\ldots,f_m{:}V_m)$、输出词表 $Y$、真值表 $G_s:K_s\to Y$，$K_s=V_1\times\cdots\times V_m$ 有限且 $G_s$ 全定义。这 18 个阶段属于决策网络实例，不是七段字节流。

**定义 2（精确 / T1）。** $N$ 对 $s$ 精确，当且仅当每个 $k\in K_s$ 上 $\operatorname{argmax} N(k)$ 唯一且等于 $G_s(k)$。

**定义 3（询问纪律）。** 只用 `ask(s,k)`。不读 logits。拒绝时不回退到表。$k\notin K_s$ 即失败。

$$
\mathrm{Compile}(x)=\mathrm{dec}(\mathrm{Run}_{E,\delta}(\mathrm{enc}(x))).
$$

δ 的输入是控制状态加一个有限观察，输出是下一状态与动作序列编号。E 只做存储、栈与输出。TSV 行 `state, observation, next, actions` 展开为有限全函数 $G_s:(q,o)\mapsto(q',a^*)$；构造器写出 $N_s$，检查只证明 $N_s=G_s$。语法与 T2/T3 义务见 `research/paper-a-notes.md`。T2a/T2b/T3 未作为本文定理。

## 3　构造

对每个键一个隐藏单元的 one-hot 构造给出精确网络，宽度上界 $|K|$（Lean `naive_exact`）。出货决策阶段用立方体覆盖把隐藏层收成布尔合取、输出层收成整数投票（算法 1；细节与 796,490→71,828 的参数计数见 notes）。`decision_list_exact` 只覆盖全局单色的首次命中子情形，不证明出货网络。发布检查是声明域上的整数网络与严格 argmax，错或并列即拒绝。

18 个决策阶段的键以 `weights/gold` 为准。op-74 合计 8,509 键与 op-87 合计 8,769 键是两套身份。combo 是 isel∪abi 的对照，不在出货路径上。

部署内核是整数：one-hot、整数仿射、ReLU、整数仿射、严格 argmax；无 softmax、浮点、乘法、libm。同一表构造出相同权重。本文不引用权重合计字节。

## 4　七段字节流与两个实例

网络编译器合同是下表七个字节流阶段。每个阶段是构造出的整数阈值网络，由同一个通用执行器逐步运行。

| 阶段 | 输入 → 输出 |
|---|---|
| 预处理（pp） | 源字节 → 预处理文本 |
| 词法（lex） | 预处理文本 → 记号流 |
| 解析（parse/parse2） | 记号流 → tape |
| 优化（opt） | tape → tape |
| 剪枝（prune） | tape → tape（公开 tape 输出跳过） |
| lowering | tape → 目标指令与数据 |
| 编码与镜像（enc） | 目标指令 → ELF / Mach-O / PE 或内存镜像 |

tape：8 个 64 位寄存器与 `SHAPE` 中 71 种操作。执行器不含 C、tape、ISA 或镜像专用原语。

**决策网络实例。** 经典递归下降、lowering 与镜像写出在决策点 `ask`；无回退。**网络编译器实例。** 通用执行器跑上表七段；模型拒绝即报错。本 tip 无同身份规模或速度测量，本文不主张这些数字。

阈值前缀求值与声明式返回不改变网络答案：同一网络、同一求和；网络—表检查仍覆盖包括返回在内的每个观察。宿主代码保留装载、`src/tapelink.c`、`src/asmtext.c`、`src/fwdstub.c`。构建期 Python 属于可信基。

## 5　自举、T1 与裁判

决策网络实例，逐字节：

1. 前端不动点：A 编自己得 B，B 编自己得 C，B = C，且与 Python 前端相同。
2. 后端闭环：94 个探针 × 6 个目标，564/564；编译器自身 6/6。
3. 无 Python：N1 = N2 = N3，目标为 osx/arm64、lnx/arm64、lnx/x86-64、win/arm64、win/x86-64（后者经系统仿真）。

N1 = N2 = N3 只断言未签名连续代字节相同。网络编译器的驱动自举只记录 macOS arm64 上固定网络包的 N1 = N2 = N3，不含网络包自构造。

T1：每个立方体网络在声明域的每个键上按部署算术运行；错或并列则构造失败。C 端正常路径查 DENSE，并有命令比较查表与直接推理。

六目标折叠：机器模型只认 lowering 给出的（OS，调用号）。六份 (stdout, exit) 彼此一致并与 tape 参考 VM 一致。去掉 macOS 系统调用号的 class 位必须得到 4/6。本 tip 无原生/仿真矩阵，本文不填这张表。

外部裁判只覆盖登记范围。18 个决策阶段中 8 个有具名裁判，台账 `research/referee.tsv`。下表是指针，不是全域 C 证明。

| 裁判 | 核对 |
|---|---|
| 预处理 | `exec/pp/macrocheck.py`，`tests/ccparity.sh`，对 `cc -E -P` |
| 解析 | `tests/warn.sh`，对 `cc -Wall` 的 (行, 类别) |
| 类型 | `_Generic`；4,275 键中的 1,400 |
| ABI | 宿主系统调用头；仅实际宿主 OS/架构 |
| 优先级 | 324 对；291 可区分且一致，33 不可区分 |
| 类型信息 | 11 种大小；8 种整数的符号与窄化 |
| 二元运算选择 | `tests/binsel_audit.py`；16 个运算符中 7 个受符号性影响 |
| printf | `tests/pfconv_audit.py`；9 个转换字母 |

已记录的决策网络实例验收（不是本 tip 的新封口）：T1 为 18 阶段 1.000；类型表对 cc 为 1,400 键中 1,399 一致、1 个有意偏离；c-testsuite 220 中 216 通过、0 错误、4 个超出范围；C99 探针 59/59；差分 93/93；三优化级 279/279；两前端 190/190；已知答案库 11/11；后端闭环 564/564 与自身 6/6；`%f/%e/%g` 与平台 libc 逐位一致。

## 6　事实边界

系统的构造—训练对照见预注册 A2。本文没有训练表。

验证纪律曾在同身份下暴露表外组合缺口；细节见 `archive/research/r20/design-review-2026-10-01.md` 与 `research/paper-a-notes.md`，此处不复述。T1 只断言单表声明域上网络 = 表。

本 tip 上无同身份的七段规模、速度或平台矩阵。语料不能外推到任意 C99。构建期 Python 在可信基内。

相关工作只作定位：有限状态机的精确网络编码（Omlin 与 Giles，1996；Siegelmann 与 Sontag，1995）、由程序构造权重（Weiss 等，2021；Lindner 等，2023）、表驱动编译（Johnson，1975；Graham 与 Glanville，1978；Fraser、Hanson 与 Proebsting，1992）、训练得到的控制器（Graves 等，2014）、已证明的语义保持（Leroy，2009；Kumar 等，2014）与多样化双重编译（Wheeler，2009）。梯度方法的失败形态留给 A2。

## 7　结论

查表形状的决策上，精确性可由穷举判定。unisacc 用构造权重运行两层：18 个决策阶段，以及七段字节流。决策网络实例在 §5 的五个目标上 N1 = N2 = N3。表是否等于 C，只由具名裁判在其覆盖内回答。

## 附录 A

| 身份 | 内容 |
|---|---|
| 源树 | main `910f1e0a`；`src/version.h` 为 `0.0.42`。不是公开 Latest，也不是密封发布测量。 |

指针：`research/referee.tsv`，`research/formalization-roadmap.md`，`research/lean/`，`prior-art.md`，`research/paper-a-notes.md`。

```bash
python3 -m unisa build-weights
python3 -m unisa acc
python3 -m unisa run examples/hello.c --fold
./tests/closure.sh examples/*.c tests/c/*.c
./tests/nativeboot.sh
./tests/gate.sh --list --com
```
