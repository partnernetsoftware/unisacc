# 表即网络：以构造而非训练得到权重的逐字节自举（N1=N2=N3）的 C 编译器

**Tables as Networks: A Byte-for-Byte Self-Hosting C Compiler Whose Weights Are Constructed, Not Trained**

2026-10-16 · 源身份 main `910f1e0a`，版本串 `0.0.42`（附录 A）。

**arXiv（投稿时）：** 主类 **cs.AI**；可选交叉 **cs.PL**。

## 摘要

unisacc 是 neural-network-based compiler：编译阶段由整数阈值网络与通用执行器运行，权重由 TSV 表 DSL 构造，不由训练得到。T1 是声明域上逐键相等（定义 2），不是样本拟合。决策网络实例是 18 个决策阶段。网络编译器实例是七个字节流阶段：preprocess (pp)、lex、parse、opt、prune、lowering、encode + image (enc)；parse2 是同合同的实现名，不另开阶段。域外键失败；不读 logits；拒绝不回退到表。N1 = N2 = N3 是五个具名目标（osx/arm64、lnx/arm64、lnx/x86-64、win/arm64、win/x86-64）上未签名连续代字节同一，不是语义自举。键身份：op-74 合计 8,509 键；op-87 标签合计 8,769 键；封口前不可互换。源身份 `910f1e0a` / `0.0.42` 不是密封发布测量。裁判台账为 `research/referee.tsv`。本文贡献是构造精确整数决策网络并以 T1 穷举核对；学习式训练对照不在本文，见预注册 A2。枚举证明网络 = 表，不证明表 = C。

**关键词**：neural-network-based compiler；constructed weights；T1；byte-for-byte self-hosting；N1=N2=N3

## 1　贡献

1. **方法。** TSV 表 DSL → 构造权重 → 声明域穷举（T1）。权重不是训练得到的。
2. **两层。** 网络编译器合同是七个字节流阶段，由一个通用执行器 E 运行有限转移 δ。决策网络实例是另外 18 个决策阶段，在询问点查构造网络（或由其穷举得到的 DENSE 表）。
3. **实现。** 六目标 C99 子集编译器 unisacc。决策网络实例不经 Python 在五个目标上逐字节自举。出货六目标含 osx/x86-64；N1 五名单不含它。网络编译器驱动自举不是包自构造。源身份见附录 A。

本文贡献是构造精确整数决策网络并以 T1 穷举核对；学习式训练对照不在本文，见预注册 A2。

不声称：表 = C99、整机语义保持（T3）、或速度优势。

## 2　定义

**定义 1（决策阶段）。** 阶段 $s$ 有字段 $(f_1{:}V_1,\ldots,f_m{:}V_m)$、输出词表 $Y$、真值表 $G_s:K_s\to Y$，$K_s=V_1\times\cdots\times V_m$ 有限且 $G_s$ 全定义。这 18 个阶段属于决策网络实例，不是七段字节流。

**定义 2（精确 / T1）。** $N$ 对 $s$ 精确，当且仅当每个 $k\in K_s$ 上 $\operatorname{argmax} N(k)$ 唯一且等于 $G_s(k)$。

**检查 / 产物。** `python3 -m unisa build-weights --from-tsv --check` 用 TSV 对照已出货权重，不写文件。`python3 -m unisa acc` 在声明域穷举；错或并列则失败。目标读法 T1 = 1.000。表是 `weights/gold/*.tsv`；构造权重是 `weights/built.json`。

**定义 3（询问纪律）。** 只用 `ask(s,k)`。不读 logits。拒绝时不回退到表。$k\notin K_s$ 即失败。正常 C 路径读的 DENSE 是网络穷举出的出货查找；拒绝与域外失败仍不回退 gold 表。

$$
\mathrm{Compile}(x)=\mathrm{dec}(\mathrm{Run}_{E,\delta}(\mathrm{enc}(x))).
$$

δ 的输入是控制状态加一个有限观察，输出是下一状态与动作序列编号。E 只做存储、栈与输出。TSV 行 `state, observation, next, actions` 展开为有限全函数 $G_s:(q,o)\mapsto(q',a^*)$；构造器写出 $N_s$，检查只证明 $N_s=G_s$。语法与 T2/T3 义务见 `research/paper-a-notes.md`。T2a/T2b/T3 未作为本文定理。

## 3　构造

对每个键一个隐藏单元的 one-hot 构造给出精确网络，宽度上界 $|K|$（Lean `naive_exact`）。出货决策阶段用立方体覆盖把隐藏层收成布尔合取、输出层收成整数投票（立方体覆盖；细节见 `research/paper-a-notes.md`）。notes 中的参数计数 796,490→71,828 不是本 tip 的新测。`decision_list_exact` 只覆盖全局单色的首次命中子情形，不证明出货网络。发布检查是声明域上的整数网络与严格 argmax，错或并列即拒绝。

op-74 合计 8,509 键与 op-87 标签合计 8,769 键是两套身份，封口前不可互换。本 tip 活 `weights/gold` 十八阶段 `#field` 域积为 8,770（相对 `research/seal-remeasure-20261009.md` 的 8,769，差在 opinfo 72→73；`weights/gold/opinfo.tsv` 头为 73 keys）。隐藏单元合计 569，与 README stages 表及 `weights/built.json` 一致。8,770 不是 8,509，也不与标签 8,769 混写。combo 是 isel∪abi 的对照，不在出货路径上；下表合计含该行。

下表由 `python3 -m unisa docs` 从 `unisa/gold.py` 与构造权重生成（`tests/docs.sh` 在不一致时失败）。

| stage | key | out | keys | units |
|---|---|---|---:|---:|
| pp | dir(9) x defined(2) | 4 classes | 18 | 6 |
| lex | c(12) x peek(12) | 10 classes | 144 | 11 |
| parse | nt(5) x tok(68) | 36 classes | 340 | 34 |
| type | t1(15) x op(19) x t2(15) | 16 classes | 4,275 | 62 |
| scope | ctx(6) x kind(5) | 7 classes | 30 | 9 |
| irsel | family(6) x flavor(70) | 70 classes | 420 | 75 |
| enc | op(87) x os(3) x arch(2) | 5 classes | 522 | 5 |
| reloc | kind(3) x arch(2) | 3 classes | 6 | 3 |
| regmap | treg(8) x arch(2) | 16 classes | 16 | 10 |
| tyinfo | t(16) | 3 heads | 16 | 6 |
| pfconv | conv(9) | 7 classes | 9 | 7 |
| peep | a(8) x b(17) x rel(12) | 10 classes | 1,632 | 18 |
| opinfo | op(73) | 3 heads | 73 | 16 |
| prec | op(19) | 11 classes | 19 | 11 |
| binsel | op(16) x sign(2) | 23 classes | 32 | 18 |
| isel | op(87) x arch(2) | 2 heads | 174 | 88 |
| abi | op(87) x os(3) x arch(2) | 13 heads | 522 | 69 |
| combo | op(87) x os(3) x arch(2) | 15 heads | 522 | 121 |
| total | | | 8,770 | 569 |

精确性由构造得到：权重从 gold 表派生（`weights/gold/*.tsv` 即表），`W1` 取 {0,1}，`b1` 可由 `W1` 恢复，`W2` 为 2 的小幂。隐藏激活恒为 0 或 1，推理无乘法、无浮点。验证是穷举：每个阶段在有限域上是全函数，枚举每个键判定网络 = 表（合计见上表）。这不证明表 = C；表对 C 是否成立另由系统编译器核对（`tests/gold_audit.py`，`difftest`）。

部署内核是整数：one-hot、整数仿射、ReLU、整数仿射、严格 argmax；无 softmax、浮点、乘法、libm。同一表构造出相同权重。本文不引用权重合计字节。

## 4　七段字节流与两个实例

决策网络实例是 18 个 ask 阶段；网络编译器实例是下表七段；二者不互换。每个阶段是构造出的整数阈值网络，由同一个通用执行器逐步运行。parse2 为同合同的实现名，不另开阶段。

| stage | input → output | role |
|---|---|---|
| preprocess (pp) | source bytes → preprocessed text | line continuation, comments, directives, macros, conditionals, includes |
| lex | preprocessed text → typed token stream | keywords, identifiers, operators, literals, longest match |
| parse | token stream → tape | declarations, scopes, types, precedence, statements, init, calls |
| opt | tape → tape | peephole / stack-to-reg style rewrites on tape |
| prune | tape → tape | drop unreachable whole functions on image/run routes (public tape out skips prune) |
| lowering | tape → target ops + data | ABI, regmap, syscalls, layout, reloc |
| encode + image (enc) | target ops → ELF / Mach-O / PE (or in-memory image) | encoding, branches, reloc, headers, Mach-O ad-hoc signature |

pp、lex、parse、enc 这些短名可以与 18 个决策阶段同名，但它们是七段字节流合同，与 ask 域表不可互换。

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

外部裁判只覆盖登记范围。18 个决策阶段中 8 个有具名裁判，台账 `research/referee.tsv`。表列 8 个为 ledger 的 external；另有 agreement 等见该文件。下表是指针，不是全域 C 证明。

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

下表为已记录结果，非本 tip `910f1e0a` 新封口重测；不证明表=C。

| 项 | 记录 |
|---|---|
| T1 | 18×1.000 |
| type↔cc | 1399/1400（1 有意偏离） |
| c-testsuite | 216/220 |
| C99 | 59/59 |
| difftest | 93/93 |
| opts | 279/279 |
| frontends | 190/190 |
| libs | 11/11 |
| closure | 564/564 + self 6/6 |
| `%f/%e/%g` | 与平台 libc 逐位一致 |

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

指针：`research/referee.tsv`，`research/formalization-roadmap.md`，`research/lean/`，`research/prior-art.md`，`research/paper-a-notes.md`。

```bash
python3 -m unisa build-weights
python3 -m unisa acc
python3 -m unisa run examples/hello.c --fold
./tests/closure.sh examples/*.c tests/c/*.c
./tests/nativeboot.sh
./tests/gate.sh --list --com
```
