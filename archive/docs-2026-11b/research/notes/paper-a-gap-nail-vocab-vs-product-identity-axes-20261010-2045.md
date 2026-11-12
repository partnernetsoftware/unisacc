# Paper A · 真缺口钉：词表双身份轴 ≠ 产物身份轴（8509/8769 tip 复核）

- **拍点：** 2026-10-10 ~20:45 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读核对 / before）：** `88846e8b991bace2d0953b340038b312981d586f`（= origin/main 于本拍开拍）
- **四 blob：** CN `74893e3d` · EN `95657738` · TeX `3830ede1` · abstract `43d7ea7e` — **全部 SAME**（相对 ~20:30；本钉**零** Paper A 正文 diff）
- **Softguess：** 仍 **NONE×4**（本钉不粘 Softguess；不重写 Softguess 封口边界钉）
- **状态：** 词表双身份（8509/8769）与产物身份（Ο1/Ο2）是 **两条正交的 seal-blocking 轴**；本拍 tip 复核 gold pin 仍绑 op-87；**不**代裁任一轴；**不**叠 ~17:08 决策卡；父节点仍是 A

---

## 主张一句（本页唯一）

**选定投稿产物身份（Ο1 保 v0.0.19 / Ο2 换封 Latest）并不能自动闭合 8509/8769 词表双身份：tip `88846e8b` 上 gold 18 阶段域积仍 = 8769、pin 仍 = `b8244bd8…`（op-87 活表），EN/arXiv 表 1 仍是标注的 op-74 / 8509 / 517；两轴必须各自封口，禁止把「已选 release」误读成「已统一键数身份」。**

---

## 为何相对 ~12:39 / ~17:08 仍真（本拍新证据）

| 已有材料 | 已钉什么 | 本拍补什么 |
| --- | --- | --- |
| 测量身份钉 ~12:39 | 8509≠8769 笔误；免责已盖；单一冻结词表身份未盖 | tip 已从 `158ae864` 前进到 `88846e8b`；**再核对** gold/built 未漂 |
| Softguess 钉 ~12:30 | Softguess = OUT-OF-SCOPE 命名卫生 | 本拍只扫：仍 NONE×4；**不**重钉 Softguess |
| 决策卡 ~17:08 | 产物轴 Ο1/Ο2/Ο3（推荐 Ο1；Latest 当时 **v0.0.38**） | **不叠新卡**；记录 Latest 已刷新为 **v0.0.39**（候选漂移事实，不代裁） |
| 平台矩阵钉 ~16:45 | 同身份六目标矩阵 blocked-on-identity | 本钉不代填矩阵；只钉「产物身份 ≠ 词表身份」 |

**缺口一句：** 心跳易把「Ο1/Ο2 批了」或「Latest 又前进」当成 8509/8769 已解。本钉证明：**产物轴前进（v0.0.38→v0.0.39）与词表轴 pin 冻结（仍 8769 / `b8244bd8…`）同时成立**——二者正交。

---

## tip 复核（本拍机核 · 不发明新键数）

方法同 [`seal-remeasure-20261009.md`](../seal-remeasure-20261009.md)：Table 1 十八阶段顺序，对 `weights/gold/<stage>.tsv` 取 `#field` 域积并算顺序 SHA-256。

| Check | 本 tip `88846e8b` | 与先验 |
| --- | --- | --- |
| 18-stage field-domain sum | **8769** | = CN op-87；≠ EN 8509 |
| 18-stage SHA-256 pin | **`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`** | **match** `seal-remeasure-tip.json` / ~12:39 复核 |
| 18 行 per-stage file sha | 0 mismatch vs `seal-remeasure-tip.json` rows | gold 字节未漂 |
| `weights/built.json` SHA-256 | **`5e45bf5e018c9e1f0417fdbeb17cbf3b87463c761542f676686a7c5fc03d665a`**（44,094 B） | **match** 隐藏单元回执 |
| enc/isel/abi/combo `#field op` 长度 | **87** | op-87 活身份；键差仍仅此四阶段 |
| Softguess /「权重就是」/ are the logic | **NONE×4** | 预期态（引 Softguess 钉） |

**未伪造：** 未改 gold / built / 正文键数；未声称 op-74 frozen gold 存在于本 tip（仍无）。

---

## 两轴对照（封口清单）

| 轴 | 问什么 | 本 tip 状态 | 谁批 | 闭合条件（摘要） |
| --- | --- | --- | --- | --- |
| **A · 词表测量身份** | 投稿表 1 / 摘要用 op-87·8769·569 还是永久双标 / 另重测 | **仍开**：活表=op-87；EN/arXiv=标注 op-74；免责已在正文 | 政委（路线Ι 精神；引 ~12:39） | 选定一侧或双标+两侧 pin；**仅批后**三联改写 |
| **B · 产物发布身份** | §8.1 / 平台矩阵 / 表 4·5 同身份重测钉 v0.0.19 还是 Latest | **仍开**：纸冻仍 v0.0.19；公开 Latest 本拍刷新为 **v0.0.39**（`published.latest: true`；公开 `.com` SHA-256 `6b2b9680…`；rc `e31eadda…`） | 政委（引 ~17:08 卡；**本拍不重开卡**） | Ο1 或 Ο2；Ο3 混读仍否决 |

**非法坍缩（本钉禁止）：**

1. 「已选 Ο1/Ο2」⇒「可把 CN 8769 换成 EN 8509」（或反过来）
2. 「Latest 已是 v0.0.39」⇒「词表身份已随产品统一」
3. 「gold pin 仍 8769」⇒「EN 表 1 必须立刻改成 8769」（无政委批 = 预写正文）
4. 把 Softguess NONE×4 重新升回与本双身份同级的 seal-before 硬缺口

---

## 产物轴候选刷新（事实 only · 不选定）

| Label | 角色 | 本拍 tip 事实 |
| --- | --- | --- |
| Paper freeze **v0.0.19** | §8.1 名义冻结身份（正文） | 仍写在 tip 正文；Ο1 默认 |
| ~17:08 卡时 Latest **v0.0.38** | 当时 Ο2 点名候选 | 已被更新 Latest 超越 |
| **Latest published v0.0.39**（本拍 NEW vs 17:08 卡） | `research/r39-release-acceptance.json`：`published.latest: true` | tag **v0.0.39**；公开 SHA-256 **`6b2b9680…`**；published ~18:16 CST；**候选 only** |
| tip gold remeasure | 词表轴 pin | **8769** / `b8244bd8…`；**不是**产物投稿身份 |

Ο2 若日后重开，点名对象应读**当时** GitHub Latest（本拍快照为 v0.0.39），而不是把 17:08 卡内的 v0.0.38 冻成永久 Ο2 标签——**这是候选名漂移说明，不是新决策卡。**

---

## 与 Softguess / 平台矩阵

| | Softguess（~12:30） | 词表轴（本钉续 ~12:39） | 产物轴（~17:08 卡） | 平台矩阵（~16:45） |
| --- | --- | --- | --- | --- |
| 封口地位 | OUT OF SCOPE | **IN SCOPE · seal-blocking** | **IN SCOPE · 待批** | **IN SCOPE · seal-blocking** |
| 本拍动作 | 只扫 NONE×4 | tip pin 复核 + 两轴正交钉 | 记录 Latest→v0.0.39；**不叠卡** | 仅引用；不填表 |

---

## 本拍明确不做

- 不改正文 CN/EN/TeX/abstract 任何键数/单元/§8.1 冻句
- 不改 kernel / weights / facts / 产品代码 / 投稿方向
- 不粘 Softguess；不叠 / 不重开 Ο1·Ο2 决策卡；不代裁
- 不发明 op-74 tip gold；不把 9174 升格为表 1 键数
- 不重登记已关闭的 E 项（Algo1 / bdiff / maint / DENSE / verification-beyond-NN）

---

## 证据与机核

- tip（before）：`88846e8b991bace2d0953b340038b312981d586f`
- Paper A blobs：`74893e3d` / `95657738` / `3830ede1` / `43d7ea7e`（SAME）
- Softguess：**NONE×4**
- 双身份免责：仍在（CN op-87 8769/569；EN/arXiv op-74 8509/517）
- pin 复核：18-stage sum **8769**；pin **`b8244bd8…`** match；built.json **`5e45bf5e…`** match
- 先验：`research/seal-remeasure-20261009.md`、`research/seal-remeasure-tip.json`
- 正交：[`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)、[`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md)、[`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md)、[`paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md)
- Latest 候选：`research/r39-release-acceptance.json`（v0.0.39；不选定）
- 父节点：**A**（neural-network-based compiler；构造非训练；unisacc 实证）
