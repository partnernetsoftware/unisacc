# 论文体系笔记

2026-10-09 SGT。只记录思考，不改产品代码，不投稿，也不写入正在封的 0.0.x。

## 根

整个体系只有一个根主张：这是基于神经网络的编译器，网络由 TSV 表构造而来，不是训练出来的。unisacc 是实证载体，POSIX C99、跨架构，不是另一篇论文的题目。实验站在编译器外面，调用现成的 `unisacc.com` 自己出表，不改 kernel、weights、facts。

论文关系是有向无环图，A 是唯一的根。A 暂时没有新主张。让它更纯粹的办法，是把还不是核心的内容搬出去，而不是另起一个根。

## 已有节点

- A：基石。中文正文 `research/unisacc-paper.md`，修订中，未投稿。
- A2：预注册 `research/a2-preregistration.md`（2026-10-02 冻结），没有正文。把 A 的 §7.3 里「构造对训练」搬出去。SGD 只作对照组。结果可以回来收窄 A 的说法，但不能长成第二套理论。tip `2ec7664` 上该路径已不存在，正文在 `archive/research/a2-preregistration.md`（见 `research/seal-a2-prereg-presence-20261009.md`）。
- B：UJS 应用短文，已有草稿，父节点是 A。
- C：管道方法意向。D：可选内存安全节点，上游是 A 和 C。E：有限控制表的系统构造，不挡 A 投稿。优化还没起稿，上游是 A 和 E。wasm 还没起稿，只依赖 A。

## cx-lab 打开的主题空间

2026-10-09，m4pro 窗口 lab-unisacc 上未提交的 `examples/cx-lab/` 跑通。仓库根 `./unisacc.com --version` 是 0.0.35。`gen_tsv.cx` 用 `-run` 跑起来，再经 shell 回调同一二进制，对 `sample.c` 做 `-S` 和 `-E`，写出 `out/tape-ops.tsv`（op、count，13 个操作码，共 107 次）和 `out/pp-text.tsv`（line、len，12 行）。没有重编编译器。引用版本写 0.0.35，不写当时 HEAD 说明里的 0.0.37。

路宽在一件事上：实验可以站在编译器外面，把现成二进制已经能印出来的文本收成表。样品只用了两扇窗，`-S` 看见指令带，`-E` 看见预处理文本。

可探索的位置，都还不是结果：

1. 测量封口。同一只二进制上重出表 4、表 5 和跨平台矩阵，让摘要键数和表对得上。这是 A 的缺口，不是新主题。
2. 指纹。同一批 C 文件，操作码次数分布随版本、随目标架构变不变。
3. 预处理本身就是一张表。现在只记了行长，还可以记宏、包含和条件编译留下了哪一支。
4. 输入从 `sample.c` 换成一批文件。吃不下就记下来，不为此改内核。
5. 自举的影子。若 0.0.35 的 `-S` 吃得下编译器自己的 `.c`，表就是它看见自己时的指令分布。还没跑过。

A2 的 SGD 对照在仓外训练环境里，不并进这条路。

## 理论还能往哪扩

理论扩大不再发明第二种网络，只把 A 里已经碰到、但还不是根主张的边界说成可以单独成立的命题。下面每条都还是意向。

部分函数。构造法对没见过的键按设计拒绝，不去猜。A2 的第三问把这件事当成对照实验。理论上它可以独立成「编译网络是一张部分函数」：覆盖的键精确，未覆盖的键拒绝。A 只需留下这个定义。错误输出和拒绝哪个代价更高，预注册已经说是价值判断，留在讨论，不升成定理。 登记笔记：[`notes/paper-a-derive-purify-partial-fn-20261010-1512.md`](notes/paper-a-derive-purify-partial-fn-20261010-1512.md)（2026-10-10 ~15:12 心跳 E：切出「≡偏函数」独立定理与诊断产品表面，不起稿；A 只保留定义边界；[`seal-refusal-partial-fn-20261009.md`](seal-refusal-partial-fn-20261009.md) 不重做）。

局部性。改一条规则，权重改动是否只落在相关的那一组神经元里，原有的键是否零错。这是 A2 的第二问，判据已经冻结。若它成立，搬出去的是一条编辑定理，不是新的编译器。若不成立，A 就去掉「局部性为构造法独有」这句话。 登记笔记：[`notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md`](notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md)（2026-10-10 ~15:43 心跳 E：切出「局部性＝构造法独有编辑定理」不得搭乘 A；A 只保留 T1；局部性实验与「独有」主张留给 A2 RQ2；[`seal-rebuild-vs-locality-20261009.md`](seal-rebuild-vs-locality-20261009.md) 不重做）。

组合。把相邻阶段合成一个网络，是 A2 第一问里的合并条件，也是 C 那篇管道意向的理论核。A 保持「每张表各自构造」。两张表何时可以合成、合成后的宽度是否就是两张表的宽度之和，是子问题。预注册里这个宽度和是实验设定，不是已经证明的定理。 登记笔记：[`notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)（2026-10-10 ~15:52 心跳 E：切出「阶段组合／表合并＝可独立命题」不得搭乘 A；A 只保留「每张表各自构造」；合并／宽度和留给 A2 RQ1 与意向 C；[`seal-combo-row-not-merge-theorem-20261009.md`](seal-combo-row-not-merge-theorem-20261009.md)／[`seal-combo-fail-not-t1-20261009.md`](seal-combo-fail-not-t1-20261009.md) 不重做）。

有限性。E 问的是哪些编译阶段本来就是有限控制表，以及表怎么系统造出来。这是分类，不是新模型。A 用 unisacc 里已经造出来的表做实证即可。没造出来的阶段不要提前写进 A。 登记笔记：[`notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md`](notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md)（2026-10-10 ~16:29 心跳 E：切出「有限性／阶段分类 + 系统造表方法」不得搭乘 A；A 只保留已造表实证与 T1；分类与造表完备性留给意向 E；[`paper-e-intent.md`](paper-e-intent.md) 不重写）。

宽度作为一种复杂度。构造解的隐藏单元数 W* 已经是预注册里的基准。A 可以只声称「存在一组构造出来的权重，全域精确」。W* 和训练宽度的竞赛属于 A2。不要在 A 里把某次训练没跑到 1.000 写成构造法的理论优势，预注册写了推翻条件：三种条件若都被推翻，优势改写成确定性、可证明和成本。§7.3 Adam 表非宽度优势定理的文案封口见 [`seal-adam-width-not-theory-20261009.md`](seal-adam-width-not-theory-20261009.md)。 登记笔记：[`notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)（2026-10-10 ~16:15 心跳 E：切出「宽度作为一种复杂度／W* vs 训练宽度竞赛」不得搭乘 A；A 只保留「存在构造精确权重（T1）」；宽度竞赛与三条推翻留给 A2 RQ1；[`seal-adam-width-not-theory-20261009.md`](seal-adam-width-not-theory-20261009.md) 不重做）。

活板门。§7.3 立方体表示下的「活板门／trapdoor」仅为非形式机理示意：小步梯度难以协调联合离散跳的直观说明，不是 Paper A 的不可达性定理，也不证明 Adam/SGD 在编译器规模合并与扩表上无法达到精确。正式梯度不可达／构造—训练竞赛属 A2。文案封口见 [`seal-trapdoor-not-unreachability-20261009.md`](seal-trapdoor-not-unreachability-20261009.md)。 登记笔记：[`notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md`](notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md)（2026-10-10 ~17:27 心跳 E：切出「活板门＝不可达性／SGD 不可能性定理」不得搭乘 A；A 只保留立方体机理示意 + T1／构造；形式竞赛留给 A2；[`seal-trapdoor-not-unreachability-20261009.md`](seal-trapdoor-not-unreachability-20261009.md) 不重写）。

目标参数化。跨架构若只是换编码表、决策网络仍由同一套构造得到，这是「网络对目标参数化」，不是第二台编译器。cx 能贡献的是对照表。在表出来之前，A 不把跨架构写成已证明的不变式。 登记笔记：[`notes/paper-a-derive-target-parameterization-20261010-1256.md`](notes/paper-a-derive-target-parameterization-20261010-1256.md)（2026-10-10 心跳 E：切出未来独立课题，不起稿正文，不改 A 根主张/键数）。

表 4 历史速度比区间。摘要与 §7.2/§8 把混合身份比值概括成 3.9×–16.6×（约 4–17×）只是跨产物历史区间，不是同身份管线当前测量；把该软读法切出为未来测量侧注，A 只保留成本真实 + §8.1 同身份重测义务。登记笔记：[`notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md`](notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md)（2026-10-10 ~14:25 心跳 E：不起稿，不改表 4 单元格/键数）。

不动点。自举若成立，理论句子是：这张构造出来的网络可以读入生成它的那份实现。cx 主题空间里的「自举影子」只是指令分布，离这个不动点还远。没跑通之前不写进 A。 登记笔记：[`notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md)（2026-10-10 ~14:40 心跳 E：切出未来独立课题，不起稿正文；A 只保留五目标 N1=N2=N3 字节自举，不动点定理 OUT）。

表 5 历史耗时（DENSE 决策网络实例 vs tcc/cc）。§7.2/§8 表 5 是决策网络实例（DENSE 查表）在 osx/arm64 上的一次历史测量，不是网络编译器管线耗时；把 tcc/cc 对比或「速度」软读法切出为未来测量侧注，A 只保留成本真实 + §8.1 第 2 条同身份重测/降级义务。登记笔记：[`notes/paper-a-derive-purify-table5-timing-20261010-1457.md`](notes/paper-a-derive-purify-table5-timing-20261010-1457.md)（2026-10-10 ~14:57 心跳 E：不起稿，不改表 5 单元格/键数）。

## 应用还能往哪扩

应用扩大换的是语言、目标或管道上的一个节点，不换根主张。条件仍然是：表由构造得到，检查是确定的。

已经有位置的：B 是 Web 上一个封闭的 JavaScript 子集。wasm 是下一个同类位置，现在不起稿。D 是管道上可选的内存安全节点。优化要等 E 的造表方法，所以它有两个上游。

cx 这条路还暗示三类应用，都还没起稿：

诊断和拒绝可以变成产品表面。未覆盖就拒绝，而不是猜一个错误输出。这和部分函数是同一句话，应用上是编译器对用户的失败方式，不是新的优化。

编码和调用约定可以单独被看成应用。仓里的实证本来就有 abi、指令选择、重定位这类表。论文上它们仍是 A 的证据。只有当某一类表要面对另一种语言或另一种目标时，才升成应用节点，wasm 就是这种升法。

一批文件上的分布可以变成回归面。操作码直方图、诊断计数、哪份源文件还吃不下，都是实验表。这服务 A 的实证封口，本身不构成应用论文。

产品线上已有的分层意向是 minicon、unisa、tinyvm、ujs。论文不要跟着产品名各写一篇。能挂回 A 的才留：ujs 已经是 B，wasm 或一台表驱动的小虚拟机以后若写，父节点仍是 A。指不回 A 的，先并进实证，或者不起稿。

**封口：** 摘要拒绝措辞与定义 3 偏函数澄清见 [`seal-refusal-partial-fn-20261009.md`](seal-refusal-partial-fn-20261009.md)（仅 research 文案，不碰表数字与测量身份）。摘要「精确重建」= T1 全表重跑构造器，≠ A2/RQ2 单条编辑局部性，见 [`seal-rebuild-vs-locality-20261009.md`](seal-rebuild-vs-locality-20261009.md)。§7.3 Adam 行非宽度/可到达性优势定理，见 [`seal-adam-width-not-theory-20261009.md`](seal-adam-width-not-theory-20261009.md)。§7.3「活板门」仅为机理示意、非 Paper A 不可达性定理，见 [`seal-trapdoor-not-unreachability-20261009.md`](seal-trapdoor-not-unreachability-20261009.md)。§3.2 的 35→20 精确枚举与 796k→71k 贪心缩参、算法 1 最优性定理不可混读，见 [`seal-algo1-35-20-not-optimality-20261009.md`](seal-algo1-35-20-not-optimality-20261009.md)。表 1 combo 行非阶段合并定理，见 [`seal-combo-row-not-merge-theorem-20261009.md`](seal-combo-row-not-merge-theorem-20261009.md)。`b_compound`/`b_pp2` 发布身份台账 vs §5.7 tape 层进展，见 [`seal-bdiff-ledger-vs-tape-20261009.md`](seal-bdiff-ledger-vs-tape-20261009.md)。§7.4 构造组合产品失败不否定 T1，见 [`seal-combo-fail-not-t1-20261009.md`](seal-combo-fail-not-t1-20261009.md)。§8 维护面 tip 快照台账 ≠ 缩小定理、同等覆盖趋势仍开放，见 [`seal-maint-surface-not-trend-20261009.md`](seal-maint-surface-not-trend-20261009.md)。决策网络实例 DENSE 产品路径不收回「基于神经网络的编译器」命名、不削弱 T1，见 [`seal-dense-path-not-withdraw-nn-20261009.md`](seal-dense-path-not-withdraw-nn-20261009.md)。逐字节自举（N1=N2=N3）≠ 语义自举 / C99 完备 / 表≡C 语义，见 [`seal-bootstrap-bytes-not-semantic-20261009.md`](seal-bootstrap-bytes-not-semantic-20261009.md)。结论 §10 同边界收紧见 [`notes/paper-a-seal-conclusion-bootstrap-boundary-20261010-1313.md`](notes/paper-a-seal-conclusion-bootstrap-boundary-20261010-1313.md)。标题/关键词同边界收紧见 [`notes/paper-a-seal-title-bootstrap-boundary-20261010-1341.md`](notes/paper-a-seal-title-bootstrap-boundary-20261010-1341.md)。结论 §10 五目标自举 vs 六平台出货见 [`notes/paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md`](notes/paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md)。摘要决策网络五目标自举边界见 [`notes/paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md`](notes/paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md)。测量身份双身份（8509/8769）封口硬缺口钉见 [`notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)。 §8.1 第 3 条同身份平台执行矩阵封口硬缺口钉见 [`notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)（2026-10-10 ~16:45；inventory／scaffold 升格；不改正文；不叠决策卡）。 §8.1 冻结身份 vs Latest 漂移披露见 [`notes/paper-a-seal-section81-latest-drift-20261010-1659.md`](notes/paper-a-seal-section81-latest-drift-20261010-1659.md)（2026-10-10 ~16:59；冻结仍 v0.0.19，Latest=v0.0.38；不改派身份）。投稿/封口重测身份选择决策卡（冻结 v0.0.19 vs Latest v0.0.38；推荐 Ο1；Ο3 混身份否决）见 [`notes/paper-a-decision-card-remeasure-identity-20261010-1708.md`](notes/paper-a-decision-card-remeasure-identity-20261010-1708.md)（2026-10-10 ~17:08；notes-only；不改正文/键数）。表 4 历史速度比区间衍生登记见 [`notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md`](notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md)。自举不动点／bootstrap fixed-point 衍生登记见 [`notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md)。表 5 历史耗时衍生登记见 [`notes/paper-a-derive-purify-table5-timing-20261010-1457.md`](notes/paper-a-derive-purify-table5-timing-20261010-1457.md)。偏函数／partial-function（定义边界留 A、「≡偏函数」定理与拒绝代价判断 OUT）衍生登记见 [`notes/paper-a-derive-purify-partial-fn-20261010-1512.md`](notes/paper-a-derive-purify-partial-fn-20261010-1512.md)。局部性＝构造法独有编辑定理／edit-locality（T1 留 A、局部性实验与「独有」主张 OUT→A2 RQ2）衍生登记见 [`notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md`](notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md)。阶段组合／表合并＝可独立命题／combo-merge（各自构造留 A、合并定理与宽度和 OUT→A2 RQ1／意向 C）衍生登记见 [`notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)。宽度作为一种复杂度／W* vs training-width race（T1 存在性留 A、宽度竞赛与 Adam＝优势定理 OUT→A2 RQ1）衍生登记见 [`notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)。有限性／有限控制表系统构造（已造表+T1 留 A、阶段分类定理与系统造表完备性 OUT→意向 E）衍生登记见 [`notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md`](notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md)。活板门／trapdoor≠不可达性定理衍生登记见 [`notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md`](notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md)（2026-10-10 ~17:27；升格登记；不重写 seal／§7.3）。