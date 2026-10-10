# Paper A：待办与审稿清单

论文 A（[`unisacc-paper.md`](unisacc-paper.md)）由本仓库的 unisacc 会话直接维护。本文件只记录**尚未完成**的事项；完成即删除，历史由 git 记录。

## 研究范围

A 不以完整控制表自动合成为投稿前提；系统构表方法归 [Paper E](paper-e-intent.md)。A 仍须如实交代表来源、可信基、覆盖边界和已知缺陷，不能通过分题免除这些披露义务。

## 投稿前必须补齐（证据）

- **部分完成：同身份重测表 4（§8.1 第 1 条）。** 只读回执核对：第 1–2 行与 `archive/research/r9/r9-current-bench-20260928.json`（`c4993fd0…`）一致；第 3–4 行声称的 `c94cf5fe…` 与稿内 fib/self 经典列在仓库内**无**基准 JSON，最近 `candidate-bench` 为另一产物且经典列对不上。见 [`seal-table4-same-identity-20261009.md`](seal-table4-same-identity-20261009.md)。同身份四行重测仍阻塞于政委选定投稿产物；未改表 4 单元格。
- **部分完成：冻结最终身份。** 附录 A 已记录 v0.0.13 与 v0.0.14 的产物身份和回执；投稿所用最终身份尚未冻结，表 2 与 §7.1 的门禁数字仍需绑定到同一身份并补构建命令。
- **部分完成：平台执行矩阵。** 2026-10-09 已从发布回执盘点证据类别（[`seal-platform-matrix-inventory-20261009.md`](seal-platform-matrix-inventory-20261009.md)）+ 同身份脚手架（[`seal-same-identity-matrix-scaffold-20261009.md`](seal-same-identity-matrix-scaffold-20261009.md)）；六 runner 演示、Lima arm64 全套件、Windows VM 自重建、lnx/x86_64 本地套件未验、Windows -run 仅 CI 等均有回执字段。2026-10-10 ~16:45 已升格为封口硬缺口钉（[`notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](notes/paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)）；投稿仍缺**同一冻结身份**下的原生执行矩阵表（§8.1 第 3 条），不能单靠回执闭合；tip Latest 已漂至 v0.0.38，§8.1 名义仍 v0.0.19。
- **部分完成：表 5 重测或降级。** 正文已将 tcc/cc 对比注明为决策网络实例的历史测量；投稿前仍需决定是否补同负载的网络编译器数字。

## 投稿前应当补强（论证）

- **图 2：管线与执行器。** 现在只有图 1（立方体网络）。补一张“七阶段字节流 + 通用执行器一步”的结构图，对应 §4.2。
- **算法 1 的复杂度与最优性差距。** 给出贪心覆盖的运行时间，以及 5 个小表上贪心与精确最小值的逐表对比（现在只有合计 35 → 20）。§3.2 已将 35→20 与 796k→71k 贪心缩参、算法 1 最优性定理的阅读风险封口（[`seal-algo1-35-20-not-optimality-20261009.md`](seal-algo1-35-20-not-optimality-20261009.md)）；逐表对比与运行时间**仍未完成**。
- **维护面统计。** §8 声称“表示迁移不等于维护成本下降”；用 exec 生成器 `.py`、手写 `.c`、`.tsv` 分列的行数趋势给出实测（与 v0.0.10 计划中的行数账本同源）。tip 类别快照（≠ 同等覆盖趋势证明）见 [`seal-maint-surface-not-trend-20261009.md`](seal-maint-surface-not-trend-20261009.md)。
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

