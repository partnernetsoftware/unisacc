# Paper A · 真缺口钉：测量身份 / 键数双身份（8509 vs 8769）

- **拍点：** 2026-10-10 ~12:39 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读核对）：** `158ae864`（相对上一心跳 Softguess 钉记忆 tip `a1ac9287` / 探针记忆 `158ae864`；**Paper A 四 blob 相对锚 tip `04a5087b` 仍 SAME**）
- **四 blob：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abstract `9e883892` — **全部 SAME**
- **Softguess：** 仍 **NONE×4**（本钉不粘 Softguess；不叠 Softguess 板）
- **状态：** 双身份是 **seal-blocking 硬缺口**（可引用）；**不**改正文键数；**不**互换 CN/EN；**不**叠 ~03:44 决策卡；父节点仍是 A

---

## 主张一句（本页唯一）

**8509（op-74）与 8769（op-87）是两套已标注并存的词表测量身份，不是同一身份的笔误；免责声明已在正文，但「单一冻结测量身份 + sha256 pin」尚未闭合，故本双身份是 Paper A 封口前硬缺口（与 Softguess 命名卫生不同类）。禁止在重测前把 CN 8769 换成 EN 8509，或反过来。**

---

## 各数是什么（定义 / 出处）

| 数字 | 身份标签 | 出现位置（tip 正文） | 含义 |
| --- | --- | --- | --- |
| **8,769 / 569** | **op-87** 词表身份 | CN `research/unisacc-paper.md`：摘要 § 决策网络实例句；表 1 合计行；表 1 下「键数身份说明」blockquote；§5.1 规模对照行 | 中文内部稿 Table 1：18 阶段键域积合计 **8769**、单元合计 **569**；`op` 词表长度 **87**（enc/isel/abi/combo） |
| **8,509 / 517** | **op-74** 词表身份 | EN `research/unisacc-paper.en.md` 摘要 / Table 1 题注 / Total 行 / §5.1；`research/arxiv-paper-a/main.tex` abstract + Table 1 caption + Size 行；`research/arxiv-paper-a/abstract.txt` | 英文与 arXiv 稿 Table 1：**8509** 键 / **517** 单元；标注为历史/并存的 op-74 身份；`op` 词表长度 **74** 于同四阶段 |
| **9,174** | A2 时代键数回忆 / 维护面 `wc -l` 巧合 | 预注册与维护面笔记；**非** tip frozen gold 键域积；稿内 **9,124 B** 是权重字节不是键数 | **无** tip 上可复现的 frozen gold 键合计=9174；不得与 8509/8769 混读 |
| **9,124 B** | 权重字节 | CN/EN § 决策网络权重合计 | 与键数身份正交 |

### 机械差（已有重测回执，本拍复核 pin）

来源：[`seal-remeasure-20261009.md`](../seal-remeasure-20261009.md) + [`seal-remeasure-tip.json`](../seal-remeasure-tip.json)（原 tip `873cc031`）。

本拍在 tip `158ae864` 对 `weights/gold/{pp…combo}.tsv` 只读重算：

- 18 阶段 `#field` 域积合计仍 **8769**
- 18 阶段 Table 1 顺序 SHA-256 pin 仍 **`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`**（与 2026-10-09 回执一致）
- 键差 **8769−8509 = 260**，仅 **enc / isel / abi / combo**（`op` 87 vs 74）
- 单元差 **569−517 = 52**，仅 **isel / abi / combo**（`enc` 两侧单元同为 5）

**活产物侧：** tip gold + `built.json` 对齐 **op-87 / 8769 / 569**（CN）。EN/arXiv 表 1 的 **8509 / 517** 是稿内**标注的** op-74 身份行加总，**不是**当前 tip `built.json` 的 live 求和。

---

## 正文已安全 vs 仍挡封口

### 已安全（disclosure / labeled dual identity — tip 已有）

| 位点 | 已有纪律 |
| --- | --- |
| CN 表 1 下说明（约 L144） | 「本表为 op-87…英文与 arXiv…op-74…两套数字在封口重测前并存，**禁止互相替换**」 |
| CN 摘要 / §5.1 | 写 8769（op-87），并声明 EN/arXiv 仍报告 8509/517，不可互换 |
| EN 摘要 / Table 1 题注 / §5.1 | 写 8509（op-74），并声明 CN 为 8769/569，**do not collapse until one sealed measurement** |
| TeX abstract / Table 1 caption / Size 行 | 同 EN 免责 |
| abstract.txt | 同 EN 免责 |

因此：**摘要键数「差」本身已披露为双身份，不是未标注笔误。** Softguess 钉已把 Softguess 剔出硬缺口清单；**本钉把测量身份钉回硬缺口清单。**

### 仍挡封口（seal-before · 本钉 IN SCOPE）

1. **单一冻结投稿测量身份尚未选定**（政委批：投稿用 op-87 还是永久双标，或另一次具名 tip 重测后统一）。
2. **封口级 pin 尚未绑到「投稿身份」**：现有 `b8244bd8…` 钉的是 tip gold 的 **op-87 活表**；EN op-74 没有对称的 tip frozen gold 复现（也无 9174 frozen gold）。
3. **CN/EN/TeX/abstract 三联数字在选定身份前不得互换或单侧改写**（政委授权前本拍亦不改正文）。
4. 与同族硬缺口正交但仍开：表口径、跨平台同身份矩阵（见 platform scaffold）——本钉只钉键数双身份，不代裁那两块。

### OUT OF SCOPE（本钉明确不做 / 不升格）

- 把 8509 写进中文表 1，或把 8769 写进英文投稿合计（**互换**）
- 把 9174 升格为 Table 1 键数
- Softguess 字面粘贴（P0 命名卫生，另板）
- 新开第二套编译器理论；本钉父节点仍是 **A**
- 改 kernel / weights / facts / 产品代码

---

## 封口清单剩什么（可执行下一步）

**Seal checklist remaining（测量身份）：**

1. **政委选定**投稿测量身份（路线Ι）：(a) 以 tip gold 的 op-87 / 8769 / 569 为唯一投稿身份并三联改 EN/arXiv；或 (b) **永久双标**并冻结两侧 pin（op-87 tip pin 已有；op-74 须另给具名历史 tip/artifact 的可复现 pin）；或 (c) 另开具名 tip 全量重测后再选 (a)/(b)。
2. **具名 tip 上**重测词表+键+单元：  
   - 键：对 Table 1 十八阶段 `weights/gold/<stage>.tsv` 的 `#field` 域积求和，并算 18-stage 顺序 sha256（方法同 `seal-remeasure-20261009.md`）。  
   - 单元：`python3 -m unisa acc`（constructed `weights/built.json`，`IntNet.nunits()` / H 行加总）。  
   - 可选 docs 对齐：`python3 -m unisa docs --check`（对 CN 表）。
3. **Pin 写入封口回执**（research note + JSON）：tip SHA、gold pin、built.json sha256、身份标签（op-74 或 op-87）、是否允许双标。
4. **仅在政委批后**三联改写 CN / EN / TeX / abstract 数字与题注；本拍**禁止**预写正文。

**本拍未伪造新测量：** 只复核既有 pin 在 tip `158ae864` 仍成立，并钉定义/来源缺口。

---

## 与 Softguess 钉的对照（为何本拍真硬）

| | Softguess（~12:30 C） | 测量身份（本拍 C） |
| --- | --- | --- |
| 封口地位 | **OUT OF SCOPE**（对偶名；NONE×4 预期） | **IN SCOPE · seal-blocking** |
| tip 义务 | Def2/3·Alg1.8 已盖 | 双身份免责已盖披露，**未盖单一冻结身份** |
| 挡粘 | 命名卫生 | 键数身份选定 + sha256 |
| 叠卡 | 不叠 ~03:44 | **不叠** ~03:44（第二板「路线Ι 测量身份」仍有效；本钉是证据页，不是新决策卡） |

---

## 与待批板关系（不叠卡）

- ~03:44 两板（**先批 P0 粘贴**；之后**路线Ι 测量身份**）**仍有效**。
- 本钉是路线Ι 的**可引用证据钉**，不催同口号、不代裁选 op-87 vs 永久双标、不重开第三块板。
- Softguess / 「权重就是」 **仍 NONE×4**；不推进 Softguess ADD。

---

## 本拍明确不做

- 不改正文 CN/EN/TeX/abstract 任何键数/单元单元格
- 不改 kernel/weights/facts/产品代码/投稿主张方向
- 不粘 Softguess；不叠决策卡；不重写 seal-remeasure 正文数字
- 不发明 op-74 的 tip gold（当前 tip 无）

---

## 证据与机核

- tip：`158ae864b8acfb141c6dab2b05fb641e867b4285`（= origin/main 于本拍）
- Paper A blobs：`000b7ef1` / `93fdf052` / `8122353c` / `9e883892`（相对 `04a5087b` / `a1ac9287` **SAME**）
- Softguess：**NONE×4**
- 双身份免责：仍在（CN op-87 8769/569；EN/arXiv op-74 8509/517）
- 先验重测：`research/seal-remeasure-20261009.md`、`research/seal-remeasure-tip.json`
- 本拍 tip 复核：18-stage domain sum **8769**；pin **`b8244bd8d9422dd32bb21af7053c38ef009d9d5a027c0924b1cac167fa454e70`** match
- 正交：`research/notes/paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`（Softguess ≠ 硬缺口）
- 父节点：**A**（neural-network-based compiler；构造非训练；unisacc 实证）
