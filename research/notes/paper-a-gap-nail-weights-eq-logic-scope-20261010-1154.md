# Paper A · 真缺口钉：「权重≡逻辑」合法作用域（非手算）

- **拍点：** 2026-10-10 ~11:54 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读）：** `e1a8c5c3`（相对上一拍 `e1a8c5c3` **未漂**；Paper A 四 blob 同锚 tip `04a5087b`）
- **四 blob：** CN `000b7ef1` · EN `93fdf052` · TeX `8122353c` · abstract `9e883892` — **全部 SAME**
- **机核：** Softguess / 「权重就是」 / “weights are the logic” **仍 NONE×4**（`/tmp/paper-a-heartbeat/paper-a-heartbeat-verify-20261010-1154.json` → `ALL_PASS True`）
- **状态：** 作用域钉（可引用）；**不**粘 tip；**不**改键数/产品/投稿方向；**不**叠 ~03:44 决策卡

---

## 主张一句（本页唯一）

「权重≡逻辑 / 权重就是逻辑」在 Paper A **只合法等于**：在声明域 \(K\) 与询问纪律下，由有限全函数表 \(G\) **构造**的阈值网 \(N\) 满足 \(\forall k\in K,\ \operatorname{argmax}_{\mathrm{strict}} N(k)=G(k)\)（T1）；且 Softguess（Softmax / 读 logits / 破平挑标签）被拒。它 **不**合法等于：表≡C99、权重≡全部编译器代码、每次产品 `ask` 必须现场 gemv、出货重叠网已 Lean 全证。

---

## 为何是真缺口（相对已有材料）

| 已有材料 | 已钉什么 | 本拍补什么 |
| --- | --- | --- |
| `paper-a-weights-eq-logic-verdict-20261009.md` | 部分成立 / 需收窄 | 把收窄收成**可粘可贴的作用域合同**（合法 / 非法两栏） |
| Softguess 逻辑钉 addon + P0 merged 包 | Softguess=纪律对偶名（UNAPPLIED） | tip 上 Softguess/权重就是 **仍 NONE×4**：作用域钉先钉「未粘前主张怎么读」，避免政委批粘时把大白话升格成过强全称 |
| DeployForm（11:40 E） | DENSE≠直播推理 | 本钉把「现场 gemv 非必要」收进**非法作用域**第 3 条，与 DeployForm 正交、不重复迁段 |
| 多页玩具手算 | 外延 / 确定性 / 往返 | **本拍禁止再堆手算**；只引用为 SUPPORT，不重做 |

**缺口一句：** tip 正文既没有 Softguess 对偶名，也没有「权重就是逻辑」大白话；若只靠口语对外说「权重就是逻辑」，审稿人可按过强全称攻击。本钉先把合法外延写死，供 P0 粘贴时对齐。

---

## 合法作用域（YES）

1. **T1 / 定义 2：** 声明域上严格唯一 argmax 等于表（tip 已 seal）。
2. **构造权：** 权重由表 DSL + 构造器导出，不是训练近似（摘要 / §3 已 seal）。
3. **询问纪律 / 定义 3：** 只经 `ask`；不读 logits；域外断言失败（tip 已 seal）。
4. **朴素存在性：** Lean `naive_exact` 子类 + 发布枚举证书（分层：定理 ≠ 出货全证）。
5. **口语缩写：** 「权重就是逻辑」= 上述 1–3 的大白话，**当且仅当**同时拒绝 Softguess。

## 非法作用域（NO · 审稿攻击面）

| # | 过强读法 | 反驳锚（tip / 已有钉） |
| --- | --- | --- |
| 1 | 表 ≡ C99 语义 | 摘要 / §2：枚举只证 \(N=G\)，表语义靠外部裁判；「表≡C」未证 |
| 2 | 权重 ≡ 全部程序 / 全部编译器代码 | §4.4：经典驱动、执行器存储/分派、tapelink/asmtext/fwdstub 仍在 |
| 3 | 命名要求每次产品 `ask` 现场 gemv | §4.4 DENSE 句；DeployForm 登记：命名钉在构造+T1 |
| 4 | 出货重叠+投票网已 Lean 全证 | §3.2：`decision_list_exact` 不覆盖出货；相等靠 enumeration-certificate |
| 5 | Softguess / Softmax 破平仍叫「精确」 | 定义 2 唯一性；定义 3 不读 logits；算法 1·8 并列拒绝；验证路径不含 softmax（Softguess addon） |
| 6 | 枚举 1.000 ⇒ 语言逻辑正确 | §7.4：表错则权重精确复制错误 |
| 7 | 跨阶段组合失败否定 T1 | §7.4：T1 只断言单表域；组合缺口在表外工程义务 |

---

## Softguess 与「权重就是」的耦合（本拍机核）

- tip 四文件 Softguess / 权重就是 / “weights are the logic”：**NONE×4**（机核 ALL_PASS）。
- 因此挡粘点仍是：**P0 未粘**（keep-keycounts Softguess AFTER + Softguess 逻辑钉 ADD 仍在 `/workspace/paper-a-p0-paste-keep-keycounts-softguess-merged-20261010.md` 与 addon），不是 tip 漂了。
- **耦合规则（本钉新增可引用句）：** 未粘 Softguess 对偶名之前，对外口语「权重就是逻辑」必须附带本页非法作用域 1–7 的收缩；粘贴后，Softguess 句负责封死非法#5，本作用域钉负责封死#1–4/#6–7。

---

## UNAPPLIED · 可随 P0 同粘的一句作用域锚（本拍不粘 tip）

> 粘贴条件：政委批 P0（或明确批「作用域锚」）；须 CN/EN/TeX/abstract 三联同改；**禁止**顺手改 8509↔8769。

**CN：**  
「权重就是逻辑」仅指：在声明域与询问纪律下，构造阈值网与表逐键一致（T1）；不等于表符合 C、不等于全部代码已网络化、也不等于每次询问都现场算权重；拒绝（含并列）不是 Softguess。

**EN：**  
“Weights *are* the logic” means only that, on the declared domain under ask-discipline, the constructed threshold net matches the table keywise (T1)—not that the table equals C, not that all compiler code is networked, and not that every product ask runs live gemv; refusal (including ties) is not Softguess.

**TeX：**  
``Weights \emph{are} the logic'' means only T1 under ask-discipline on the declared domain---not table$\equiv$C, not a fully networked compiler, and not live gemv on every ask; refusal (including ties) is not Softguess.

---

## 与待批板关系（不叠卡）

- ~03:44 两板（P0 粘贴；路线Ι 测量身份）**仍有效**。
- 本钉给 P0 粘贴提供**作用域合同**，相对上次卡有新信息（合法/非法两栏 + Softguess NONE 机核 + 与 DeployForm 正交），但**本拍不新开决策卡、不催同口号**。
- 双身份 8509/8769 免责**仍在** tip；本钉不触键数。

---

## 本拍交付与非交付

**已做：** tip/blob/Softguess NONE 机核；本作用域钉；短报证据。  
**未做：** 不粘正文；不改键数；不叠决策卡；不 push tip；不再堆手算。  
**镜像：** `research/notes/` 同名（本地）；`/workspace` 与 `/tmp/paper-a-heartbeat/`。
