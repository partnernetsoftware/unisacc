# Paper A 衍生净化：表 4 历史速度比区间（3.9×–16.6× / ~4–17×）

- **拍点：** 2026-10-10 ~14:25 Asia/Shanghai（UTC+8）
- **切口：** **E. 衍生净化**
- **仓库 tip（只读核对）：** `15630ed53cc4f8b89df81473fdc1c7601ad9af33`
- **父节点：** Paper A（基于神经网络的编译器；TSV 表构造网络与权重，非训练；实证载体 unisacc POSIX C99 跨架构）
- **状态：** **note only**（登记未来独立测量/预注册侧注；**不起稿正文**；**不改** A 根主张、键数身份、Table 4 单元格、product/kernel/weights/facts）
- **Softguess：** tip 四文件 **NONE×4**（本拍不重开）
- **测量身份：** 8509/8769 **仍开** — 引用既有钉 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)；本拍**不重钉、不叠** ~03:44 路线Ι 卡

---

## 主张一句（本页唯一）

**表 4 把两套 v0.0.9 产物身份的比值概括成 3.9×–16.6×（约 4–17×）只是跨产物历史区间，不是网络编译器管线的同身份当前测量；把该区间升格成独立测量课题后，A 只保留「速度成本真实 + §8.1 同身份重测仍开」，不再被「摘要里的 4–17 倍像投稿级速度主张」随便打脸。**

---

## 切什么（从 A 的主张负担里切出）

切出对象是 **摘要 / §7.2 / §8 / 结论里反复出现的「约 4–17 倍 / 3.9×–16.6×」历史速度比区间**，登记为未来独立的 **measurement / maintenance side note（或预注册测量稿）**，而不是 A 的核心理论或投稿级速度定理：

| 稿内位点（tip `15630ed5`） | 摘句要点 | 分类 |
| --- | --- | --- |
| CN/EN 摘要；`arxiv-paper-a/abstract.txt` | 表 4 历史各行混合 `c4993fd0…` / `c94cf5fe…`；比值（3.9×–16.6×）概括成约 4–17 倍 = 跨产物历史区间，**不是**管线一次当前测量 | **migrate soft claim load**（区间本身）→ 衍生；免责边界句可暂留 A |
| CN/EN §7.2 表 4 + 速度段 | 四行非同身份；比值列概括为约 4–17 倍只是历史跨产物区间；表单元格 16.6× / 9.2× / 3.9× / 12.8× | **表单元格 keep in A as labeled historical**；**区间概括 migrate** |
| CN L206 / EN L204 | fib/self 从 10.2×、25.0× 降到 3.9×、12.8×（§7.2） | **historical narrative keep or appendix**；不升投稿主张 |
| CN/EN §8 速度段；§8.1 第 1 条 | 概括为 4–17 倍不是同身份或 v0.0.14 结论；须同机同身份重测 | **keep obligation in A**（封口清单）；**重测结果 migrate** 到衍生测量稿 |
| 先验钉 | [`seal-table4-same-identity-20261009.md`](../seal-table4-same-identity-20261009.md)：行 1–2 回执 OK；行 3–4 无仓内 JSON pin | **evidence parent**；本衍生不重跑基准 |

**不切：** 根主张（构造非训练、T1、神经编译器命名）、表 1 键数、§7.3/A2 训练对照、七阶段管线方法（属 Paper C 意向）、目标参数化（已登记 [`paper-a-derive-target-parameterization-20261010-1256.md`](paper-a-derive-target-parameterization-20261010-1256.md)）。

---

## 为何不挡 A 投稿

1. **A 根主张不依赖速度比区间。** 核心是表→构造网络→声明域穷举精确（T1）；速度是代价叙述，不是存在性定理。
2. **正文已披露双身份 + 非当前测量。** 摘要与 §7.2/§8 已写明混合 `c4993fd0…`/`c94cf5fe…`；审稿人不能把区间当成未标注笔误，但**可以**把反复出现的区间读成软主张——故登记衍生以卸负担，而不是用投稿前同身份重测去「修」区间本身。
3. **§8.1 第 1 条义务仍挂在 A。** 同身份重测是 A 的测量封口项；衍生课题是「区间不得当主张」的理论/叙述净化，**不替代**也不闭合该重测。
4. **本拍零正文 diff。** 不改正文数字、不删表 4、不改键数；只登记。

---

## 父节点如何回 A

- **实证回 A：** 表 4 历史行与 `seal-table4-same-identity` 回执仍是 A 实证载体上的成本证据；未来同身份重测仍在 A 选定的提交产物上做。
- **理论句回 A：** 「方法有真实执行成本」仍服务根主张的诚实边界；衍生只搬走「用跨产物区间冒充当前管线倍率」的软读法。
- **DAG：** A →（未来）Table-4 same-identity speed measurement / historical-band side note；**不是**第二根，也不是 A2（训练）或 C（管道方法）。

---

## 切后 A 哪一句更硬

切出后，A 可把速度叙事收成一句硬边界：

> **方法的执行成本真实；表 4 仅作已标注的历史混合身份回执，投稿级速度主张须等 §8.1 同身份重测，本文不把 3.9×–16.6× / ~4–17× 当作当前网络编译器管线倍率。**

攻击面从「摘要里带了 4–17 倍，是不是速度论文？」缩回 **可度量义务**（同身份四行重测 + 主机标注），与根主张正交。

---

## 衍生课题提案（不起稿）

| 项 | 内容 |
| --- | --- |
| **暂定题** | Same-Identity Remeasurement of Historical Network-Compiler Speed Ratios (unisacc Table 4) |
| **一行主张** | On one frozen `unisacc.com` SHA-256 and one labeled host, remeasure the four Table 4 workloads; until then the printed 3.9×–16.6× band remains a cross-artifact historical interval, not a current pipeline claim. |
| **上游** | Paper A（§7.2 表 4；§8.1 第 1 条；`seal-table4-same-identity-20261009`） |
| **协议指针** | 同身份协议已写在 `seal-table4-same-identity-20261009.md`（政委选定提交产物后方可跑） |
| **状态** | **note only** — 无独立 TeX/中英草稿；无新预注册文件（若日后冻结，可另开 `research/table4-speed-prereg.md`，本拍不做） |

---

## 与既有衍生/硬缺口的正交

| 主题 | 关系 |
| --- | --- |
| 测量身份 8509/8769 | **仍开硬缺口**；本拍只引用 12:39 钉，不重论 |
| 目标参数化（~12:56 E） | 正交；跨架构不变式 vs 速度区间 |
| A2 / §7.3 Adam | 正交；训练对照不是速度比 |
| Paper C 七阶段管线 | 正交；方法论文 ≠ 表 4 倍率侧注 |
| Softguess | **NONE×4**；非硬缺口 |

---

## 明确不做

- 不改正文 CN/EN/TeX/abstract（含不删表 4 单元格、不改 3.9/16.6 数字）
- 不改键数身份；不跑新基准；不改 product/kernel/weights/facts
- 不起稿；不叠 ~03:44 决策卡；不重钉 8509/8769
- 不把表 5（DENSE 实例耗时）并进本衍生（表 5 另条 §8.1 第 2 条）

---

## 证据

- tip：`15630ed53cc4f8b89df81473fdc1c7601ad9af33`
- Paper A blobs：CN `f4007083…` · EN `7878d807…` · TeX `eacae1a1…` · abs `43d7ea7e…`（相对 ~14:07 心跳 **SAME**）
- Softguess：**NONE×4**
- 先验：`research/seal-table4-same-identity-20261009.md`；`research/paper-a-notes.md` §8.1 第 1 条部分完成条
- 短报：[`paper-a-heartbeat-report-20261010-1425.md`](paper-a-heartbeat-report-20261010-1425.md)
- 父节点：**A**