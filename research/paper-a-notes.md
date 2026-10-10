# Paper A：待办与审稿清单

论文 A（[`unisacc-paper.md`](unisacc-paper.md)）由本仓库的 unisacc 会话直接维护。本文件只记录**尚未完成**的事项；完成即删除，历史由 git 记录。

## 研究范围

A 不以完整控制表自动合成为投稿前提；系统构表方法归 [Paper E](paper-e-intent.md)。A 仍须如实交代表来源、可信基、覆盖边界和已知缺陷，不能通过分题免除这些披露义务。

## 投稿前必须补齐（证据）

- **部分完成：同身份重测表 4（§8.1 第 1 条）。** 只读回执核对：第 1–2 行与 `archive/research/r9/r9-current-bench-20260928.json`（`c4993fd0…`）一致；第 3–4 行声称的 `c94cf5fe…` 与稿内 fib/self 经典列在仓库内**无**基准 JSON，最近 `candidate-bench` 为另一产物且经典列对不上。见 [`seal-table4-same-identity-20261009.md`](seal-table4-same-identity-20261009.md)。同身份四行重测仍阻塞于政委选定投稿产物；未改表 4 单元格。同身份重测**仍未完成**（开放实证可做；§8.1 第 1 条四行重测工作仍保留；但**非**理论／根主张闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-table4-same-identity-remeasure-not-a-gate-20261011-0531.md`](notes/paper-a-derive-purify-table4-same-identity-remeasure-not-a-gate-20261011-0531.md)）。
- **部分完成：冻结最终身份。** 附录 A 已记录 v0.0.13 与 v0.0.14 的产物身份和回执；投稿所用最终身份尚未冻结，表 2 与 §7.1 的门禁数字仍需绑定到同一身份并补构建命令。冻结／绑定／构建命令程序**仍未完成**（开放实证可做；表 2／§7.1 同身份绑定工作仍保留；但**非**理论／根主张闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-final-identity-freeze-not-a-gate-20261011-0510.md`](notes/paper-a-derive-purify-final-identity-freeze-not-a-gate-20261011-0510.md)）。
- **部分完成：平台执行矩阵。** 2026-10-09 已从发布回执盘点证据类别（[`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md)）+ 同身份脚手架（[`seal-same-identity-matrix-scaffold-20261009.md`](seal-same-identity-matrix-scaffold-20261009.md)）；六 runner 演示、Lima arm64 全套件、Windows VM 自重建、lnx/x86_64 本地套件未验、Windows -run 仅 CI 等均有回执字段。2026-10-10 ~16:45 已升格为封口硬缺口钉（[`notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)）；投稿仍缺**同一冻结身份**下的原生执行矩阵表（§8.1 第 3 条），不能单靠回执闭合；tip Latest 已漂至 v0.0.38，§8.1 名义仍 v0.0.19。
- **部分完成：表 5 重测或降级。** 正文已将 tcc/cc 对比注明为决策网络实例的历史测量；投稿前仍需决定是否补同负载的网络编译器数字。表 5 同身份重测或降级**仍未完成**（开放实证可做；§8.1 第 2 条重测／降级工作仍保留；但**非**理论／根主张闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-table5-same-identity-remeasure-or-downgrade-not-a-gate-20261011-0545.md`](notes/paper-a-derive-purify-table5-same-identity-remeasure-or-downgrade-not-a-gate-20261011-0545.md)）。

## 投稿前应当补强（论证）

- **图 2：管线与执行器。** 现在只有图 1（立方体网络）。补一张“七阶段字节流 + 通用执行器一步”的结构图，对应 §4.2。
- **算法 1 的复杂度与最优性差距。** 给出贪心覆盖的运行时间，以及 5 个小表上贪心与精确最小值的逐表对比（现在只有合计 35 → 20）。§3.2 已将 35→20 与 796k→71k 贪心缩参、算法 1 最优性定理的阅读风险封口（[`seal-algo1-35-20-not-optimality-20261009.md`](seal-algo1-35-20-not-optimality-20261009.md)）；逐表对比与运行时间**仍未完成**（开放实验可做，但**非**投稿／理论闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md`](notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md)）。
- **维护面统计。** §8 声称“表示迁移不等于维护成本下降”；用 exec 生成器 `.py`、手写 `.c`、`.tsv` 分列的行数趋势给出实测（与 v0.0.10 计划中的行数账本同源）。tip 类别快照（≠ 同等覆盖趋势证明）见 [`seal-maint-surface-not-trend-20261009.md`](seal-maint-surface-not-trend-20261009.md)。 衍生净化升格登记（快照≠缩小／趋势定理；~18:12 E）见 [`notes/paper-a-derive-purify-maint-surface-not-trend-20261010-1812.md`](notes/paper-a-derive-purify-maint-surface-not-trend-20261010-1812.md)。 同等覆盖行数趋势实测**仍未完成**（开放实验可做，但**非**投稿／理论闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md`](notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md)）。
- **算法 1 的复杂度与最优性差距。** 给出贪心覆盖的运行时间，以及 5 个小表上贪心与精确最小值的逐表对比（现在只有合计 35 → 20）。§3.2 已将 35→20 与 796k→71k 贪心缩参、算法 1 最优性定理的阅读风险封口（[`seal-algo1-35-20-not-optimality-20261009.md`](seal-algo1-35-20-not-optimality-20261009.md)）；2026-10-10 ~19:30 已升格 derive-purify 登记（[`notes/paper-a-derive-purify-algo1-not-optimality-20261010-1930.md`](notes/paper-a-derive-purify-algo1-not-optimality-20261010-1930.md)）；逐表对比与运行时间**仍未完成**（开放实验可做，但**非**投稿／理论闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md`](notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md)）。
- **维护面统计。** §8 声称“表示迁移不等于维护成本下降”；用 exec 生成器 `.py`、手写 `.c`、`.tsv` 分列的行数趋势给出实测（与 v0.0.10 计划中的行数账本同源）。tip 类别快照（≠ 同等覆盖趋势证明）见 [`seal-maint-surface-not-trend-20261009.md`](seal-maint-surface-not-trend-20261009.md)。 同等覆盖行数趋势实测**仍未完成**（开放实验可做，但**非**投稿／理论闸门；GATE 升格已切见 [`notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md`](notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md)）。
- **消融。** 阈值前缀求值与声明式返回各自的贡献（现在只报合并效果）。
- **外部裁判扩展。** 目前登记表有 8/18 个决策阶段具名外部裁判，但覆盖范围各异；解析与预处理仍需扩展独立裁判。

## 格式与投稿

- 确定目标会议或期刊，据此决定语言（英文版）、篇幅与匿名要求；附录 A 的仓库链接在匿名投稿时替换为匿名制品。
- 参考文献改为规范的编号列表（作者、题名、会议或期刊、年份），与 `prior-art.md` 对齐。
- 图 1 的矢量源在 `figures/fig1-deterministic-intnet.svg`；新图按同一风格（黑白、无阴影）绘制。

- **待核对：摘要与表 1 的键数身份。** 审查发现摘要 8,509 与表内合计 8,769 不同；须核对绑定版本和统计口径，不直接替换。
- **2026-10-09 心跳（键数身份标注，未互换数字）：** 中文表 1 = **op-87 → 8,769 / 569**；英文/arXiv 表 1 = **op-74 → 8,509 / 517**（机械差来自 enc/isel/abi/combo 四阶段）。已改中文摘要与 §5.1，不再把 8,509 误标为「表 1 所列」；英文摘要/表 1 题注/§5.1 与 arXiv tex、abstract.txt 同步标明两套身份并存。表内键/单元单元格未改；封口重测前禁止把 8,769 写成英文投稿合计，也禁止把 8,509 写进中文表 1。

- **2026-10-09 心跳（平台矩阵盘点，未改稿内数字）：** 回执只读盘点见 [`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md)；§8.1 第 3 条仍待同身份原生矩阵。

- **2026-10-10 ~12:39 心跳（测量身份硬缺口钉，未改稿内数字）：** 双身份 8509(op-74 EN/arXiv) vs 8769(op-87 CN/tip gold) 为 **seal-blocking**；免责已在正文，单一冻结身份+sha256 未闭合；禁止互换。见 [`notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)。

- **2026-10-09 心跳（封口重测，未改稿内数字）：** tip `873cc031` 的 `weights/gold` 十八阶段 `#field` 域积合计 **8769**，逐行对齐中文表 1（op-87）；与英文表 1（op-74 / 8509）仅 enc/isel/abi/combo 的 `op` 87≠74。SHA-256 pin（18 阶段 Table 1 顺序）`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`；二十表 sorted pin `1ef9608742cded03d0a7c1acc74572379efeae228a8f34e15be047bf6ff581b3`（域积 9041=8769+257+15）。单元 517/569 本拍未重测；A2 的 9174 仍非本 tip frozen gold（预注册写 type 4281/peep 1638 等，与 tip 不一致）。详见 `research/seal-remeasure-20261009.md`。投稿身份仍待政委选定后再三联改写。

- **2026-10-09 心跳（DENSE 路径命名封口，未改数字与产物）：** 决策网络实例产品 DENSE 查表不收回「基于神经网络的编译器」主张，见 [`seal-dense-path-not-withdraw-nn-20261009.md`](seal-dense-path-not-withdraw-nn-20261009.md)。
- **2026-10-09 心跳（逐字节自举封口，未改键数与产物）：** N1=N2=N3 仅未签名连续代字节同一具名检查，非语义自举定理或 C99/表≡C 证明，见 [`seal-bootstrap-bytes-not-semantic-20261009.md`](seal-bootstrap-bytes-not-semantic-20261009.md)。
- **2026-10-09 心跳（§7.3 活板门封口，未改表数字）：** 「活板门」仅为立方体机理示意、非不可达性定理，见 [`seal-trapdoor-not-unreachability-20261009.md`](seal-trapdoor-not-unreachability-20261009.md)。

- **2026-10-10 ~12:56 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「目标参数化 / 跨架构换编码表」；父节点回 A；不挡投稿；不起稿。见 [`notes/paper-a-derive-target-parameterization-20261010-1256.md`](notes/paper-a-derive-target-parameterization-20261010-1256.md)。

- **2026-10-10 ~13:13 心跳（切口 A 结论自举边界，未改键数与产物）：** §10 裸「自举 / self-hosts」收紧为具名检查下逐字节自举（N1=N2=N3；§5.3；非语义自举定理）；见 [`notes/paper-a-seal-conclusion-bootstrap-boundary-20261010-1313.md`](notes/paper-a-seal-conclusion-bootstrap-boundary-20261010-1313.md)。

- **2026-10-10 ~13:41 心跳（切口 A 标题/关键词自举边界，未改摘要正文与键数）：** 标题与关键词裸「自举 / Self-Hosting」收紧为逐字节自举（N1=N2=N3）/ Byte-for-Byte Self-Hosting；摘要正文未动；见 [`notes/paper-a-seal-title-bootstrap-boundary-20261010-1341.md`](notes/paper-a-seal-title-bootstrap-boundary-20261010-1341.md)。
- **2026-10-10 ~13:58 心跳（切口 A 结论五目标自举 vs 六平台出货，未改摘要与键数）：** §10 将六平台与五目标 N1=N2=N3 分离表述；见 [`notes/paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md`](notes/paper-a-seal-conclusion-five-vs-six-selfhost-20261010-1358.md)。
- **2026-10-10 ~14:07 心跳（切口 A 摘要决策网络五目标自举边界，未改键数与产物）：** abstract.txt / TeX 摘要裸「the compiler self-hosts」收紧为 decision-network instance + N1=N2=N3 + five named targets（§5.3）；CN/EN 摘要本已 scoped，未改正文；见 [`notes/paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md`](notes/paper-a-seal-abstract-decision-net-five-selfhost-20261010-1407.md)。

- **2026-10-10 ~14:25 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立测量侧注「表 4 历史速度比区间 3.9×–16.6× / ~4–17×」；父节点回 A；不挡投稿；不起稿；不闭合 §8.1 同身份重测。见 [`notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md`](notes/paper-a-derive-purify-table4-speed-band-20261010-1425.md)。

- **2026-10-10 ~14:40 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「自举不动点 / bootstrap fixed-point theorem」；父节点回 A（及已封字节自举边界）；A 只保留五目标 N1=N2=N3；不动点 OUT；不起稿。见 [`notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](notes/paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md)。

- **2026-10-10 ~14:57 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立测量侧注「表 5 DENSE 决策网络实例 vs tcc/cc 历史耗时」；父节点回 A；不挡投稿；不起稿；不闭合 §8.1 第 2 条重测/降级。见 [`notes/paper-a-derive-purify-table5-timing-20261010-1457.md`](notes/paper-a-derive-purify-table5-timing-20261010-1457.md)。

- **2026-10-10 ~15:43 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「局部性＝构造法独有编辑定理 / edit locality as construction-unique theorem」；父节点回 A2 RQ2→A；不挡投稿；不起稿；seal-rebuild 不重做。见 [`notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md`](notes/paper-a-derive-purify-locality-edit-theorem-20261010-1543.md)。

- **2026-10-10 ~15:52 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「阶段组合／表合并＝可独立命题 / stage combo · table merge」；父节点回 A2 RQ1（和／或意向 C）→A；不挡投稿；不起稿；seal-combo-row／seal-combo-fail 不重做。见 [`notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md`](notes/paper-a-derive-purify-combo-merge-theorem-20261010-1552.md)。

- **2026-10-10 ~16:15 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「宽度作为一种复杂度／W* vs training-width race」；父节点回 A2 RQ1→A；不挡投稿；不起稿；seal-adam-width 不重做。见 [`notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md`](notes/paper-a-derive-purify-width-complexity-wstar-20261010-1615.md)。
- **2026-10-10 ~16:29 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「有限性／有限控制表的系统构造（阶段分类 + 系统造表方法）」；父节点回意向 E→A；不挡投稿；不起稿；paper-e-intent 不重写。见 [`notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md`](notes/paper-a-derive-purify-finiteness-control-tables-20261010-1629.md)。
- **2026-10-10 ~16:45 心跳（切口 C 真缺口钉，未改稿内数字）：** §8.1 第 3 条同身份六目标平台执行矩阵升格为 **seal-blocking**；inventory／scaffold 只证明模式；禁止把六 runner 演示当成原生矩阵；候选身份刷新 Latest=v0.0.38。见 [`notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)。


- **2026-10-10 ~16:59 心跳（切口 A 成篇定稿增量）：** §8.1 开篇披露冻结仍 v0.0.19、公开 Latest 已至 v0.0.38；投稿级同身份重测须另选单一密封 Latest，不得混读回执与更新二进制；**不**改派冻结身份。见 [`notes/paper-a-seal-section81-latest-drift-20261010-1659.md`](notes/paper-a-seal-section81-latest-drift-20261010-1659.md)。

- **2026-10-10 ~17:27 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「活板门／trapdoor ≠ 不可达性定理」；父节点回 A2→A；不挡投稿；不起稿；seal-trapdoor 不重写；不叠 ~17:08 Ο1/Ο2/Ο3。见 [`notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md`](notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1727.md)。
- **2026-10-10 ~19:15 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「活板门／trapdoor ≠ 不可达性定理」；父节点回 A2 RQ1/RQ3→A；不挡投稿；不起稿；seal-trapdoor 不重写；不叠 Ο1/Ο2。见 [`notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md`](notes/paper-a-derive-purify-trapdoor-unreachability-20261010-1915.md)。
- **2026-10-10 ~19:30 心跳（切口 E 衍生净化，未改稿内数字）：** 登记未来独立课题「算法 1／Algo1 35→20 精确枚举与 796k→71k 贪心缩参 ≠ 最优性定理」；父节点回 A；不挡投稿；不起稿；seal-algo1 不重写；csih 后 tip 重登记；不叠 Ο1/Ο2。见 [`notes/paper-a-derive-purify-algo1-not-optimality-20261010-1930.md`](notes/paper-a-derive-purify-algo1-not-optimality-20261010-1930.md)。
- **2026-10-11 ~01:59 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「五目标字节自举战役／六平台自举·出货完备程序文 ≠ A 理论闸门」；父节点回 A（经 §5.3/§10 五 vs 六诚实披露）；不挡投稿；不起稿；不重 seal 1358／1407；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md`](notes/paper-a-derive-purify-five-vs-six-selfhost-campaign-not-a-gate-20261011-0159.md)。
- **2026-10-11 ~02:38 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「C99 种子构造器／宿主 Python 构造可信基完稿 ≠ A 理论闸门」；父节点回 A（经 §8 可信基披露 + §8.1#6 边界）；不挡投稿；不起稿；不删 §8.1#6；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-c99-seed-constructor-python-tcb-not-a-gate-20261011-0238.md`](notes/paper-a-derive-purify-c99-seed-constructor-python-tcb-not-a-gate-20261011-0238.md)。
- **2026-10-11 ~02:58 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「C99 条款账本／规范性条款覆盖完稿 ≠ A 理论闸门」；父节点回 A（经 C99 子集实证 + §5.5 条款分母诚实披露）；不挡投稿；不起稿；不改 §5.5 条款数字；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-c99-clause-ledger-coverage-not-a-gate-20261011-0258.md`](notes/paper-a-derive-purify-c99-clause-ledger-coverage-not-a-gate-20261011-0258.md)。
- **2026-10-11 ~03:16 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「消融完稿（权重前缀求值 vs 声明式返回贡献；现只报合并效果）≠ A 理论闸门」；父节点回 A（经构造+T1+unisacc 实证+§4 合并效果诚实叙述）；不挡投稿；不起稿；不发明消融数字；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-ablation-not-a-gate-20261011-0316.md`](notes/paper-a-derive-purify-ablation-not-a-gate-20261011-0316.md)。
- **2026-10-11 ~03:38 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「图 2（七阶段字节流 + 通用执行器结构图）完稿 ≠ A 理论闸门」；父节点回 A（经构造+T1+unisacc 实证+图 1+§4.2 文字叙述）；不挡投稿；不起稿；不发明图／SVG；不改写 §4.2；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-fig2-pipeline-diagram-not-a-gate-20261011-0338.md`](notes/paper-a-derive-purify-fig2-pipeline-diagram-not-a-gate-20261011-0338.md)。

- **2026-10-11 ~03:58 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「格式与投稿完稿（目标会议／期刊选定 + 规范编号参考文献对齐 prior-art.md + 匿名制品替换）≠ A 理论闸门」；父节点回 A（经构造+T1+unisacc 实证+CN/EN/TeX/abstract 对齐义务）；不挡投稿；不起稿；不代裁 venue／公开；不重排参考文献；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-format-venue-bib-anon-not-a-gate-20261011-0358.md`](notes/paper-a-derive-purify-format-venue-bib-anon-not-a-gate-20261011-0358.md)。

- **2026-10-11 ~04:12 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「算法 1 五小表贪心-vs-精确逐表对比 + 贪心运行时间完稿 ≠ A 理论／投稿闸门」；父节点回 A（经构造+T1+§3.2 诚实合计披露 + seal 禁最优性定理）；不挡投稿；不起稿；不发明逐表数字；不重做 algo1-not-optimality-1930／seal-algo1；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md`](notes/paper-a-derive-purify-algo1-per-table-runtime-not-a-gate-20261011-0412.md)。

- **2026-10-11 ~04:59 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「同等覆盖维护面行数趋势实测完稿（`.py`／`.c`／`.tsv`）≠ A 理论／投稿闸门」；父节点回 A（经构造+T1+§8 诚实成本披露 + tip 类别快照台账 + seal 禁快照＝趋势定理）；不挡投稿；不起稿；不发明行数趋势数字；不重做 maint-surface-not-trend-1952／1812／seal-maint；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md`](notes/paper-a-derive-purify-maint-surface-equal-coverage-trend-measure-not-a-gate-20261011-0459.md)。

- **2026-10-11 ~05:10 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「冻结最终投稿身份完稿（附录 A 已有 v0.0.13／v0.0.14 回执；表 2 与 §7.1 须同身份绑定并补构建命令）≠ A 理论／根主张闸门」；父节点回 A（经构造+T1+unisacc 实证+诚实披露最终身份未冻／双身份 8509/8769）；不挡投稿；不起稿；不降级开放实证绑定工作；不重做 8509-8769 gap-nail／平台程序／Ο1·Ο2 决策卡；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-final-identity-freeze-not-a-gate-20261011-0510.md`](notes/paper-a-derive-purify-final-identity-freeze-not-a-gate-20261011-0510.md)。

- **2026-10-11 ~05:31 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「同身份重测表 4（§8.1 第 1 条：四行同身份重测）完稿 ≠ A 理论／根主张闸门」；父节点回 A（经构造+T1+unisacc 实证+诚实 §8.1#1 披露同身份重测仍开）；不挡投稿；不起稿；不降级开放实证重测工作；不重做 table4-speed-band／seal-table4／最终身份冻结 GATE／Ο1·Ο2 决策卡；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-table4-same-identity-remeasure-not-a-gate-20261011-0531.md`](notes/paper-a-derive-purify-table4-same-identity-remeasure-not-a-gate-20261011-0531.md)。

- **2026-10-11 ~05:45 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「表 5 同身份重测或降级（§8.1 第 2 条／paper-a-notes「部分完成：表 5 重测或降级」）完稿 ≠ A 理论／根主张闸门」；父节点回 A（经构造+T1+unisacc 实证+诚实 §8.1#2 披露表5重测／降级仍开）；不挡投稿；不起稿；不降级开放实证重测／降级工作；不重做 table5-timing／table4-same-identity／最终身份冻结 GATE／Ο1·Ο2 决策卡；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-table5-same-identity-remeasure-or-downgrade-not-a-gate-20261011-0545.md`](notes/paper-a-derive-purify-table5-same-identity-remeasure-or-downgrade-not-a-gate-20261011-0545.md)。

- **2026-10-11 ~05:53 心跳（切口 E 衍生净化，未改稿内数字）：** 登记「§8.1 第 4 条具名差异台账闭合（`b_compound`／`b_pp2` + 六目标镜像核对／R20-2·R20-3）完稿 ≠ A 理论／根主张闸门」；父节点回 A（经构造+T1+unisacc 实证+诚实 §8.1#4／§5.5『已列清单之外』披露）；不挡投稿；不起稿；不降级开放实证台账闭合工作；不重做 bdiff-ledger-vs-tape／table4·table5 GATE／最终身份冻结／平台程序／Ο1·Ο2 决策卡；不降级 §8.1／8509-8769／Ο1/Ο2。见 [`notes/paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md`](notes/paper-a-derive-purify-named-diff-ledger-closure-not-a-gate-20261011-0553.md)。
