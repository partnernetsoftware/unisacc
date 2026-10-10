# Paper A 衍生净化：表 5 历史耗时（DENSE 决策网络实例 vs tcc/cc）

- **拍点：** 2026-10-10 ~14:57 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对 / before）：** `691bf2a7a43dbcd32ced1357fb98ec5df3f37c52`
- **父节点：** Paper A（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立测量/预注册侧注；**不起稿正文**；**不改** A 根主张、键数身份、Table 5 单元格、product/kernel/weights/facts）
- **Softguess：** tip 四文件 **NONE×4**（本拍不重开）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠** ~03:44 路线Ι 卡

---

## 主张一句（本页唯一）

**表 5 把决策网络实例（DENSE 查表）在 osx/arm64 上相对 tcc/cc 的历史耗时写成对比表（及任何把表 5 读成「网络编译器速度」的软主张）只是跨产物/实例路径的历史测量侧注，不是同身份网络编译器管线速度定理；把该软主张负荷迁出后，A 只保留「代价真实 + §8.1 第 2 条同身份重测或降级仍开」。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **§7.2 表 5 / §8 速度段里对 tcc、cc 的历史对比，以及任何把表 5 读成网络编译器（七阶段管线）速度主张的软读法**，登记为未来独立的 **measurement / maintenance side note（或预注册测量稿）**，而不是 A 的核心理论或投稿级速度定理：

| 稿内位点（tip `691bf2a7`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN/EN §7.2；TeX 同段 | 表 5 = **决策网络实例**在 osx/arm64 上一次历史测量（三次取最小）；经典路径查 **DENSE** 答案表；**不是**表 4 七阶段通用执行器管线耗时；同任务以 -O2 编译 unisacc 自身并逐字节核对 | **migrate soft claim load**（tcc/cc「速度」对比读法）→ 衍生；实例/非管线免责句可暂留 A |
| CN/EN/TeX 表题 | 「决策网络实例（DENSE 查表）与 tcc、cc 的比较——单一负载，非管线耗时」 | **表题 keep as labeled historical**；**对比升格 migrate** |
| 表单元格（seal 摘录稿内印刷值） | cc -O2 / cc -O0 / tcc / unisacc -O2 / unisacc -O0 五行；vs_cc_O2 列含 1.0、3.3、3.7、5.4、13.8；叙事约 1.5×（0.81/0.55） | **表单元格 keep in A as labeled historical**；**倍率软读 migrate** |
| CN/EN/TeX §8 速度段 | 「Table 5 times only the decision-network instance (DENSE lookup), not the network-compiler pipeline」 | **keep boundary in A**；重测结果 migrate |
| §8.1 第 2 条 / `paper-a-notes` | 表 5 重测或降级；正文已注明历史测量；投稿前仍须决定是否补同负载网络编译器数字 | **keep obligation in A**；**闭合动作 migrate** 到衍生测量稿 |
| 先验钉 | [`seal-table5-same-identity-20261009.json`](../seal-table5-same-identity-20261009.json)：五行均 unpinned；无仓内 Table 5 JSON pin | **evidence parent**；本衍生不重跑基准 |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3/A2 训练对照、表 4 历史速度比区间（已登记 [`paper-a-derive-purify-table4-speed-band-20261010-1425.md`](paper-a-derive-purify-table4-speed-band-20261010-1425.md)）、自举不动点（已登记 [`paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md`](paper-a-derive-purify-bootstrap-fixedpoint-20261010-1440.md)）、目标参数化（已登记）、DENSE 部署形态命名（已另登记）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖表 5 的 tcc/cc 倍率。** 核心是表→构造网络→声明域穷举精确（T1）；表 5 是代价/对照叙述，不是存在性定理。
2. **正文已披露「决策网络实例 · DENSE · 非管线」。** 表题与 §7.2/§8 已写明不是七阶段网络编译器耗时；审稿人不能把表 5 当成未标注的管线速度表，但**可以**把 tcc/cc 对比读成软速度主张——故登记衍生以卸负担。
3. **§8.1 第 2 条义务仍挂在 A。** 同身份重测或降级是 A 的测量封口项；衍生课题是「表 5 不得当网络编译器速度定理」的叙述净化，**不替代**也不闭合该重测。
4. **本拍零正文 diff。** 不改正文数字、不删表 5、不改键数；只登记。表 4 衍生笔记已显式声明表 5 **不得**并进该 derive——本拍单独登记。

---

## 父节点如何回 A

- **实证回 A：** 表 5 历史行与 `seal-table5-same-identity` 缺口清单仍是 A 实证载体上的成本/对照证据；未来同身份重测仍在 A 选定的提交产物（DENSE 决策网络实例）上做。
- **理论句回 A：** 「方法有真实执行成本」仍服务根主张的诚实边界；衍生只搬走「用决策网络实例历史对照冒充网络编译器管线速度」的软读法。
- **DAG：** A →（未来）Table-5 same-identity DENSE timing remeasure / historical-band side note；**不是**第二根，也不是 A2（训练）、C（管道方法）或表 4 衍生（§8.1 第 1 条）。

---

## 切后 A 哪一句更硬

切出后，A 可把表 5 叙事收成一句硬边界：

> **方法的执行成本真实；表 5 仅作已标注的决策网络实例（DENSE）历史对照，投稿级速度主张须等 §8.1 第 2 条同身份重测或降级，本文不把表 5 的 tcc/cc 对比当作当前网络编译器管线倍率。**

攻击面从「表里有对 tcc/cc 的倍数，是不是速度论文？」缩回 **可度量义务**（同身份重测或附录降级 + 工具链版本钉），与根主张正交。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Same-Identity Remeasurement of Historical Decision-Network (DENSE) Timings vs tcc/cc (unisacc Table 5) |
| **一行主张** | On one frozen DENSE decision-network-instance identity and one labeled host (osx/arm64), remeasure Table 5's single workload with pinned cc/tcc versions; until then the printed rows remain a historical instance-path comparison, not a network-compiler pipeline speed claim. |
| **上游** | Paper A（§7.2 表 5；§8 速度段；§8.1 第 2 条；`seal-table5-same-identity-20261009.json`） |
| **协议指针** | 同身份协议已写在 `seal-table5-same-identity-20261009.json`（政委选定投稿产物后方可跑；`tests/bench_vs.sh`） |
| **状态** | **note only** — 无独立 TeX/中英草稿；无新预注册文件（若日后冻结，可另开 `research/table5-timing-prereg.md`，本拍不做） |

---

## 与既有衍生/硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 表 4 速度区间（~14:25 E） | **正交且刻意分离**：表 4=§8.1 第 1 条管线历史带宽；表 5=§8.1 第 2 条 DENSE 实例对照；表 4 derive **明确不并**表 5 |
| 自举不动点（~14:40 E） | 正交 |
| 目标参数化（~12:56 E） | 正交 |
| DENSE 部署形态命名（~11:40） | 正交；命名/T1 锚 ≠ 表 5 耗时侧注 |
| A2 / §7.3 Adam | 正交；训练对照不是表 5 耗时 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不删表 5 单元格、不改稿内印刷耗时/倍率数字）
- 不改键数身份；不跑新基准；不改 product/kernel/weights/facts
- 不起稿；不叠 ~03:44 决策卡；不重钉 8509/8769
- 不把表 5 并进表 4 衍生；不把网络编译器管线耗时顶替表 5
- 不发明数字——倍率与秒数仅引用稿内/seal 已印值

---

## 证据

- tip（before）：`691bf2a7a43dbcd32ced1357fb98ec5df3f37c52`
- Paper A blobs：CN `f4007083…` · EN `7878d807…` · TeX `eacae1a1…` · abs `43d7ea7e…`（相对 ~14:40 心跳 **SAME**）
- Softguess：**NONE×4**（四正文文件无 Softguess 字面）
- 先验：`research/seal-table5-same-identity-20261009.json`；`research/paper-a-notes.md` §8.1 第 2 条部分完成条；表 4 derive 对表 5 的「另条」声明
- 短报：[`paper-a-heartbeat-report-20261010-1457.md`](paper-a-heartbeat-report-20261010-1457.md)
- 父节点：**A**
