# Paper A · 真缺口钉：§8.1 第 3 条同身份六目标平台执行矩阵

- **拍点：** 2026-10-10 ~16:45 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读核对 / before）：** `aed37ad91c238ba82ed35719850cb26275ebafa5`（= origin/main 于本拍开拍；上拍 E 1629 合入后 tip）
- **四 blob：** CN `f4007083` · EN `7878d807` · TeX `eacae1a1` · abstract `43d7ea7e` — **全部 SAME**（本钉**零** Paper A 正文 diff）
- **Softguess：** 仍 **NONE×4**（本钉不粘 Softguess；不叠 Softguess 板）
- **状态：** §8.1 item 3 同身份六目标平台执行矩阵是 **seal-blocking 硬缺口**（可引用）；库存与脚手架证明模式，**不能**单靠回执闭合；**不**改正文键数；**不**新跑 bench；**不**叠 ~03:44 决策卡；父节点仍是 A

---

## 主张一句（本页唯一）

**§8.1 第 3 条「同身份六目标平台执行矩阵」是 Paper A 封口前硬缺口：既有 inventory／scaffold 回执只证明各目标证据类别与填表形状，在政委选定单一冻结投稿身份并填满同身份矩阵之前不得视为已闭合；禁止把六 runner 演示矩阵当成原生执行矩阵。**

---

## 正文已安全 vs 仍挡封口

### 已安全（disclosure — tip 已有）

| 位点 | 已有纪律 |
| --- | --- |
| CN §6.2 | v0.0.19 回执记录六托管 runner 演示矩阵、六目标 ABI 机器模型折叠、本机 VM 中 Windows 两目标逐字节自重建；**未给出**「本机 / Linux VM / Windows VM 逐目标原生运行」完整矩阵（指向 §8.1 第 3 条） |
| EN §6.2 | 同口径：receipt「does not publish」per-target host / Linux VM / Windows VM native-run matrix（§8.1 item 3） |
| CN/EN §8.1 item 3 | 明列「平台矩阵」为投稿前未满足条件；「登记不等于完成」；当前稿仍点名整理 **v0.0.19** 回执 |
| CN/EN §8.1 开篇 | 本文冻结发布身份为 **v0.0.19**（并注明 v0.0.20 已发布、投稿级重测时改用最新身份） |

因此：**缺口本身已在正文披露为未满足条件，不是未登记疏漏。** Softguess 钉与 8509/8769 钉已把命名卫生／键数双身份钉入清单；**本钉把平台同身份矩阵升格为同系列可引用 seal-blocking 证据页。**

### 仍挡封口（seal-before · 本钉 IN SCOPE）

1. **单一冻结投稿产物身份尚未选定**（政委批：仍以 §8.1 名义 v0.0.19，还是改用 tip 上最新已发布身份，或另具名 tip／SHA）。
2. **同身份六行矩阵未填满**：每行须绑定**同一** release tag **或** 同一 `unisacc.com` SHA-256 **或** 同一 sealed candidate digest；并标明 native vs emulation／suite scope（见 scaffold 列）。
3. **禁止**把「六个 runner 演示矩阵 green」写成 §8.1 第 3 条已闭合（CN/EN §6.2 已否定）。
4. 与同族硬缺口正交但仍开：**测量身份 8509/8769**（仅引用既有钉，本钉不代裁）；表 4／表 5 同身份口径——本钉只钉平台矩阵。

### OUT OF SCOPE（本钉明确不做 / 不升格）

- 本拍**新跑**任何平台 bench／gate／release-check，或发明 timings／pass counts
- 重写 Table 1 键数或互换 CN/EN 8509/8769
- 叠一张与 ~03:44 同选项的新决策卡（本钉是**证据页**，不是决策卡）
- 把 8509/8769 重钉一遍（仍开，仅引用 [`paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`](paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md)）
- 改正文 CN/EN/TeX/abstract；改 kernel / weights / facts / 产品代码
- 把 rowcov / `UNISACC_EDGE_LOG` 当成平台矩阵闭合（scaffold 已声明正交）

---

## 先验：inventory + scaffold（证明模式，不闭合）

| 文档 | 角色 | 对本钉的结论 |
| --- | --- | --- |
| [`seal-platform-matrix-inventory-20261009.md`](../seal-platform-matrix-inventory-20261009.md) (+ JSON) | 回执只读盘点六目标 evidence_class | **Cannot be closed from receipts alone** |
| [`seal-same-identity-matrix-scaffold-20261009.md`](../seal-same-identity-matrix-scaffold-20261009.md) (+ JSON) | 同身份最小表形；六行 `fill_status=blocked-on-identity` | 政委选身份后才可填；禁止混身份拼行 |

Oct 9 候选曾写 Latest published **v0.0.36**（`r36` 当时 `latest: true`）。**本拍刷新：** tip 上 GitHub Latest 已是 **v0.0.38**（见下）；产品相对纸冻 v0.0.19 已明显漂移——这是相对 Oct 9 的**新证据**，但仍**不**代裁投稿身份。

---

## 六目标表（evidence_class + fill_status；仍 blocked-on-identity）

证据类别来自 2026-10-09 inventory（回执字段，本拍不重跑）；`fill_status` 在单一投稿身份选定前一律 **`blocked-on-identity`**。

| Target | evidence_class（inventory） | fill_status | 仍缺的同身份单元格（scaffold 语言） |
| --- | --- | --- | --- |
| `osx/arm64` | `hosted-runner demo`（+ 开发机 native 烟测口径） | `blocked-on-identity` | 投稿身份下：host metal vs hosted `macos-15`；comdemo ≠ 全 gate |
| `osx/x86_64` | `native-run`（Rosetta on arm64）+ `hosted-runner demo` | `blocked-on-identity` | 标明 Rosetta／仿真 vs Intel 真机；六 runner comdemo |
| `lnx/arm64` | `native-run`（Lima guest）+ `hosted-runner demo` | `blocked-on-identity` | Lima native arm64 vs hosted arm64 runner；guest-known reds 口径 |
| `lnx/x86_64` | `hosted-runner demo`（本地多为仿真烟测；全源套件未验） | `blocked-on-identity` | 必须标 cross／emulator-only 本地路径；产品在 ubuntu-latest ≠ 本地全套件 |
| `win/arm64` | `VM self-rebuild` + `hosted-runner demo` | `blocked-on-identity` | UTM 自重建 vs hosted ARM；-run/winposix 在较新回执常为 CI-only |
| `win/x86_64` | `VM self-rebuild` + `hosted-runner demo` | `blocked-on-identity` | 同上；不得与其它版本机制行混拼 |

**跨切：** 六 runner sealed-candidate + demo programs = `hosted-runner demo`（非逐宿主原生全矩阵）。ABI 机器模型 6/6 折叠与 v0.0.9 platform_smoke 只服务列语义／机制标签，**不得**单独充当投稿同身份矩阵。

---

## 候选身份刷新（CURRENT tip · 2026-10-10 ~16:45 · **不选定**）

| Label | 角色 | 本拍 tip 事实（候选 only） |
| --- | --- | --- |
| Paper freeze **v0.0.19** | CN/EN §8.1 仍写「本文冻结的发布身份为 v0.0.19」；§6.2／item 3 点名 r19 | 源 `d95176a`；公开 `unisacc.com` SHA-256 `ad1d87e9…`（附录 A／§5.6）；`archive/research/r19-release-acceptance.json` |
| 历史 Latest（Oct 9 scaffold）**v0.0.36** | 当时 inventory／scaffold 的「最新已发布」行 | `research/r36-release-acceptance.json`：status published；rc `29728cc6`；candidate sha256 `b4607639…`；`unverified`: Windows -run/winposix CI-only；**本拍已非 GitHub Latest** |
| 记忆中的 **v0.0.37** | 中间发布；无 `research/r37-release-acceptance.json` | tag `v0.0.37` @ `7a8764fc`；已被 v0.0.38 超越 |
| **Latest published v0.0.38**（本拍 NEW） | `gh release list`：**Latest**；`published.latest: true` | `research/r38-release-acceptance.json`：status published；rc `4790de9d`；candidate sha256 `ba3f40cb…`；sealed_from `2298c31e`；公开 `unisacc.com` SHA-256 `579f6525…`（签名后）；`release_eligible: true`；`unverified`: 6× rc77 host-unavailable + 6× RESOURCE_UNKNOWN；post_release_smoke success |
| Draft **v0.0.39** | GitHub Draft（court dry-run） | **非**投稿候选；本钉不讨论 |
| Tip gold remeasure | 键域 pin only | 18-stage pin `b8244bd8…`；**不是**产品投稿身份 |
| Historical **v0.0.9** smoke | 机制标签 only | 不得混入投稿矩阵行 |

**产品 vs 纸冻漂移（新证据，非决策）：** §8.1 名义身份仍停在 **v0.0.19**，而 tip 上 Latest 已是 **v0.0.38**（公开签名 `.com` 约 2.26 MB 量级 vs v0.0.19 的 1.92 MB）。投稿级平台矩阵必须在**选定身份**上重导出；不得把 r9／r19／r36／r38 机制拼成一行表。

**本钉不挑选**上述任一行为投稿身份。

---

## 封口清单剩什么（政委选身份之后）

**Seal checklist remaining（平台矩阵 §8.1 item 3）：**

1. **政委选定**单一投稿产物身份（例如：维持 v0.0.19 纸冻并只补矩阵；或改绑 Latest v0.0.38／其它具名 tag／`unisacc.com` SHA；并同步是否改写 §8.1 开篇「冻结身份」句——**正文改写仅在批后**）。
2. 在**该身份**上跑／导出一次：release-check 六 runner + 本机 Linux／Windows 必跑路径（按 scaffold 列：native vs emulation、suite scope）；**禁止**混用其它版本回执填单元格。
3. 产出一张六行同身份表（可进 research note + 日后 §6.2／§8.1 引用）；显式标出仅交叉编译／仅仿真／仅 CI 的目标（尤其 `lnx/x86_64`、Rosetta、Windows -run）。
4. 与 Table 4 同身份工作对齐**同一** tag（见既有 `seal-table4-same-identity-20261009.md`）——正交，本钉不代做。
5. **仅在政委批后**三联改写正文（若需把「v0.0.19」冻句改成新身份或降级措辞）；本拍**禁止**预写正文。

**本拍未伪造新测量：** 只刷新 tip 上 Latest／r38 候选行，并把 §8.1 item 3 升格为与 Softguess／8509 钉同系列的 seal-blocking 证据页。

---

## 与 Softguess／8509 钉的对照

| | Softguess（~12:30 C） | 测量身份 8509/8769（~12:39 C） | 平台同身份矩阵（本拍 C） |
| --- | --- | --- | --- |
| 封口地位 | OUT OF SCOPE（命名卫生；NONE×4） | **IN SCOPE · seal-blocking** | **IN SCOPE · seal-blocking** |
| tip 义务 | Def2/3·Alg1.8 已盖 | 双身份免责已盖；单一冻结未盖 | §6.2／§8.1 已披露未满足；同身份表未填 |
| 挡粘 | 命名 | 键数身份 + sha256 | 投稿产物身份 + 六行同身份矩阵 |
| 叠卡 | 不叠 ~03:44 | 不叠 ~03:44 | **不叠** ~03:44（证据页，非新决策卡） |

---

## 与待批板关系（不叠卡）

- ~03:44 既有板（P0 粘贴；路线Ι 测量身份）**仍有效**；本钉**不**重开同口号第三块「身份选择题」卡。
- 平台矩阵身份与键数测量身份相关但**不等同**：选产物 tag／SHA 与选 op-74 vs op-87 是两道题；本钉只钉前者之矩阵义务。
- Softguess／「权重就是」仍 **NONE×4**；不推进 Softguess ADD。
- 8509/8769 **仍开**，仅引用 1239 钉。

---

## 本拍明确不做

- 不改正文 CN/EN/TeX/abstract 任何单元格或 §8.1 冻句
- 不改 kernel/weights/facts/产品代码/投稿主张方向
- 不新跑 bench；不发明 pass／timing
- 不叠决策卡；不重写 inventory／scaffold 正文（仅引用）
- 不把六 runner demo 升格为原生矩阵闭合

---

## 证据与机核

- tip（before）：`aed37ad91c238ba82ed35719850cb26275ebafa5`
- Paper A blobs：`f4007083` / `7878d807` / `eacae1a1` / `43d7ea7e`（相对上拍 1629 **SAME**）
- Softguess：**NONE×4**
- Latest on tip：**v0.0.38**（`published.latest: true`；公开 SHA-256 `579f6525…`）；Draft v0.0.39 忽略
- 先验：`research/seal-platform-matrix-inventory-20261009.md`、`research/seal-same-identity-matrix-scaffold-20261009.md`
- 正交硬缺口：`research/notes/paper-a-gap-nail-measurement-identity-8509-8769-20261010-1239.md`（仍开）
- 父节点：**A**（neural-network-based compiler；构造非训练；unisacc 实证）
