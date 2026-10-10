# Paper A · 真缺口钉刷新：§8.1 第 3 条同身份六目标平台矩阵 · Latest 身份漂移

- **拍点：** 2026-10-10 ~21:15 Asia/Shanghai（论文心跳）
- **本拍切口：** **C. 真缺口钉**
- **仓库 tip（只读核对 / before）：** `87eb16eeebc196c0be1f70649a9c73abb2cc2cab`（= origin/main 于本拍 PR 开拍／rebase 基线；工作中曾见 `f81274a89e9d574037adae1772866a493dd4efa6` 与 scout `4dfbac368dec040c22ce842005632fb42a2fbd5c`，其间合入产品 observation／queue 修复，**非**本钉正文）
- **四 blob：** CN `74893e3df1f9663b5d11f5bad6dc3cf0ca59e884` · EN `95657738e0b447ee4e40ea9ca3dd961329aebffc` · TeX `3830ede13556e8c9673f994fd6d843937476ecf5` · abstract `43d7ea7e7297ed8be9a96e3950e014e359467973` — **全部 SAME**（本钉**零** Paper A 正文 diff；与 ~20:52 钉同 tip 正文身份）
- **Softguess：** 仍 **NONE×4**（本钉不粘 Softguess；封口地位仍 OUT-OF-SCOPE）
- **状态：** §8.1 item 3 同身份六目标平台执行矩阵仍是 **seal-blocking / blocked-on-identity**；相对 ~16:45 钉，**Latest 已从 v0.0.38 漂到 v0.0.39**——产品 tip 前进**不**闭合矩阵；**不**改正文键数；**不**新跑 bench；**不**叠 ~17:08 Ο1/Ο2 决策卡；父节点仍是 A

---

## 主张一句（本页唯一）

**tip HEAD 上 GitHub Latest 已发布身份现为 v0.0.39（公开 `unisacc.com` SHA-256 `6b2b9680…`），相对 ~16:45 钉的 Latest v0.0.38 已漂移；§8.1 第 3 条同身份六目标平台执行矩阵仍是 seal-blocking／六行仍 `blocked-on-identity`——产品 tip 前进与 runner-demo／六格法院字节核对均不等于原生同身份矩阵闭合；本钉不选定身份、不叠 Ο1/Ο2。**

---

## 相对 ~16:45 钉的新证据（本拍 WHY NEW）

| 项 | ~16:45 钉 | 本拍 ~21:15（verified） |
| --- | --- | --- |
| tip | `aed37ad9…` | `f81274a8…`（含 PR#62 后产品 observation／queue 修复） |
| GitHub **Latest** | **v0.0.38** | **v0.0.39**（`gh release list`；`r39.published.latest: true`） |
| 公开 `unisacc.com` SHA-256 | `579f6525…`（v0.0.38） | `6b2b9680…`（v0.0.39；asset digest 同） |
| 矩阵 fill_status | 六行 `blocked-on-identity` | **仍** 六行 `blocked-on-identity`（未填满） |
| 正文 §8.1 冻句 | 仍名义 v0.0.19 | **仍**名义 v0.0.19（本钉不改） |

**结论：** Latest 身份刷新是相对 1645 的**可引用新事实**；它**加强**「纸冻 vs 产品漂移」披露，**不**把矩阵从 seal-blocking 降级。

---

## 仍挡封口（seal-before · IN SCOPE）

1. **单一冻结投稿产物身份尚未选定**（政委批：仍以 §8.1 名义 v0.0.19，或改绑某已发布 tag／`unisacc.com` SHA——现候选池含 v0.0.39）。
2. **同身份六行矩阵未填满**：每行须绑定**同一** release tag **或** 同一公开 `unisacc.com` SHA-256 **或** 同一 sealed candidate digest；并标明 native vs emulation／suite scope。
3. **禁止**把「六个 runner 演示矩阵 green」或「six-native-cells 法院对最终字节」写成 §8.1 第 3 条已闭合（CN/EN §6.2 已否定 runner-demo≡原生矩阵）。
4. 正交仍开（仅引用）：**测量身份 8509/8769**（R87/R74 已钉配方，选定仍开）；Ο1/Ο2 产物轴决策卡仍待批——**本钉不叠卡**。

### OUT OF SCOPE

- 本拍新跑任何平台 bench／gate／release-check，或发明 timings／pass counts
- 重写 Table 1 键数或互换 CN/EN 8509/8769；重钉 Δ260 为主主张（仅引用 ~20:52）
- 叠一张与 ~17:08 同选项的新决策卡
- 改正文 CN/EN/TeX/abstract；改 kernel／weights／facts／产品代码
- Softguess 粘贴／ADD

---

## 六目标表（evidence_class + fill_status；仍 blocked-on-identity）

证据类别仍来自 2026-10-09 inventory（回执字段，本拍不重跑）。`fill_status` 在单一投稿身份选定前一律 **`blocked-on-identity`**。

| Target | evidence_class（inventory） | fill_status | 仍缺的同身份单元格（scaffold 语言） |
| --- | --- | --- | --- |
| `osx/arm64` | `hosted-runner demo`（+ 开发机 native 烟测口径） | `blocked-on-identity` | 投稿身份下：host metal vs hosted `macos-15`；comdemo ≠ 全 gate |
| `osx/x86_64` | `native-run`（Rosetta on arm64）+ `hosted-runner demo` | `blocked-on-identity` | 标明 Rosetta／仿真 vs Intel 真机 |
| `lnx/arm64` | `native-run`（Lima guest）+ `hosted-runner demo` | `blocked-on-identity` | Lima native arm64 vs hosted arm64 runner |
| `lnx/x86_64` | `hosted-runner demo`（本地多为仿真烟测） | `blocked-on-identity` | 必须标 cross／emulator-only；ubuntu-latest ≠ 本地全套件 |
| `win/arm64` | `VM self-rebuild` + `hosted-runner demo` | `blocked-on-identity` | UTM 自重建 vs hosted ARM；-run/winposix 常为 CI-only |
| `win/x86_64` | `VM self-rebuild` + `hosted-runner demo` | `blocked-on-identity` | 同上；不得与其它版本机制行混拼 |

**跨切：** 六 runner sealed-candidate + demo = `hosted-runner demo`（≠ 逐宿主原生全矩阵）。v0.0.39 回执中 `courts.six-native-cells-final-bytes`（draft-bytes 六格对最终 SHA）只核**同一最终字节**可达性，**不得**单独充当 §8.1 item 3 同身份原生执行矩阵。

---

## 候选身份刷新（CURRENT tip · 2026-10-10 ~21:15 · **不选定**）

| Label | 角色 | 本拍 tip 事实（候选 only；引自文件／`gh`） |
| --- | --- | --- |
| Paper freeze **v0.0.19** | CN/EN §8.1 仍写「本文冻结的发布身份为 v0.0.19」；§6.2／item 3 点名 r19 | `archive/research/r19-release-acceptance.json`：source `d95176a`；公开 SHA-256 `ad1d87e947089bb7b7618300b87ad194d88b6a4c550dcca294d9e27878ce68e5`；`public_bytes` 1922560 |
| **v0.0.38**（~16:45 当时 Latest） | 已发布；**本拍已非** GitHub Latest | `research/r38-release-acceptance.json`：rc `4790de9d`；candidate sha256 `ba3f40cb4fbad9f632bfb2aefeed355789ebb05fd06c6aee84cc8301fbff94b5`；公开 SHA-256 `579f6525ce101ef35100269bcd57ea74e2e3b38bded38ab9dcf014856640d9da`；回执内 `published.latest` 字段仍写 `true`（**文件未改派**，与 GitHub Latest 标签不一致——以 `gh release list` + r39 为准） |
| **Latest published v0.0.39**（本拍 NEW） | `gh release list`：**Latest**；非 draft | `research/r39-release-acceptance.json`：status `published`；`published.latest: true`；`published_at_cst` `2026-10-10T18:16:17+08:00`；rc tag `rc/v0.0.39` sha `e31eaddaf99513eb2b7f707f45b1168068098268`；candidate sha256 `2b20f4b2fd9a496d2e36d47577741420f9e4aba47be0d377e3866f5f91aa5ed9`；公开 SHA-256 **`6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a`**（与 release asset digest 一致；after_bytes 2259392）；`release_eligible: true`；`unverified` 含 6× rc77 host-unavailable 等 |
| Tip 产品观察 **0.0.40** | tip 有 controlled first-window observation 提交；**非** GitHub Latest 发布 | tip 消息可见；**无** `gh release` Latest 标签；本钉**不**把 observation 当投稿身份 |
| Tip gold remeasure | 键域 pin only | **不是**产品投稿身份（见词表轴钉） |
| Historical **v0.0.9** smoke | 机制标签 only | 不得混入投稿矩阵行 |

**产品 vs 纸冻漂移（刷新，非决策）：** §8.1 名义身份仍停在 **v0.0.19**；~16:45 Latest 曾是 **v0.0.38**；本拍 Latest 已是 **v0.0.39**。投稿级平台矩阵必须在**选定身份**上重导出；不得把 r9／r19／r38／r39 机制拼成一行表。

**本钉不挑选**上述任一行为投稿身份。

---

## 封口清单剩什么（政委选身份之后）

1. **政委选定**单一投稿产物身份（维持 v0.0.19 纸冻并只补矩阵；或改绑 Latest **v0.0.39**／其它具名 tag／公开 SHA——见 ~17:08 卡 Ο1/Ο2，**不叠新卡**）。
2. 在**该身份**上跑／导出：release-check 六 runner + 本机 Linux／Windows 必跑路径（scaffold 列）；**禁止**混用其它版本回执填单元格。
3. 产出六行同身份表；显式标出仅交叉编译／仅仿真／仅 CI 的目标。
4. 与 Table 4 同身份工作对齐**同一** tag（正交）。
5. **仅在政委批后**三联改写正文（若需改「v0.0.19」冻句）；本拍**禁止**预写正文。

**本拍未伪造新测量：** 只刷新 tip 上 Latest＝v0.0.39 候选行，并重申矩阵仍 seal-blocking。

---

## 与先验钉／卡的关系（不叠卡）

| 先验 | 本拍用法 |
| --- | --- |
| [`paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md`](paper-a-gap-nail-platform-matrix-same-identity-20261010-1645.md) | **直接前进**：升格页仍有效；本页刷新 Latest 行 |
| [`../seal-platform-matrix-inventory-20261009.md`](../seal-platform-matrix-inventory-20261009.md) | 引用：Cannot be closed from receipts alone |
| [`../seal-same-identity-matrix-scaffold-20261009.md`](../seal-same-identity-matrix-scaffold-20261009.md) | 引用：六行仍 `blocked-on-identity` |
| [`paper-a-decision-card-remeasure-identity-20261010-1708.md`](paper-a-decision-card-remeasure-identity-20261010-1708.md) | **仍有效；不叠**新决策卡（卡内 Latest 举例曾写 v0.0.38——事实已漂，选项结构不变） |
| [`paper-a-gap-nail-vocab-vs-product-identity-axes-20261010-2045.md`](paper-a-gap-nail-vocab-vs-product-identity-axes-20261010-2045.md) | 引用：词表轴 ⊥ 产物轴；选 release 不自动统一键数 |
| [`paper-a-gap-nail-vocab-track-recipes-delta260-20261010-2052.md`](paper-a-gap-nail-vocab-track-recipes-delta260-20261010-2052.md) | 引用：R87/R74；本钉不重钉 Δ260 |
| Softguess ~12:30 | OUT-OF-SCOPE；本拍 **NONE×4** |
| 8509/8769 ~12:39 | **仍开**（仅引用） |

---

## 本拍明确不做

- 不改正文 CN/EN/TeX/abstract 任何单元格或 §8.1 冻句
- 不改 kernel/weights/facts/产品代码/投稿主张方向
- 不新跑 bench；不发明 pass／timing
- 不叠决策卡；不重写 inventory／scaffold 正文（仅引用）
- 不把六 runner demo 或 six-native-cells 法院升格为原生矩阵闭合

---

## 证据与机核

- tip（before）：`87eb16eeebc196c0be1f70649a9c73abb2cc2cab`
- Paper A blobs：`74893e3d…` / `95657738…` / `3830ede1…` / `43d7ea7e…`（本钉 **SAME**）
- Softguess：**NONE×4**
- Latest on tip：**v0.0.39**（`published.latest: true`；公开 SHA-256 `6b2b96803b23fc19c578bb511cddd9434b0c64d7743adc99fa2f6e174eb6c57a`）
- 先验：1645 钉；`seal-platform-matrix-inventory-20261009.md`；`seal-same-identity-matrix-scaffold-20261009.md`；1708 决策卡；2045／2052 词表钉
- 正交硬缺口：8509/8769 **仍开**
- 父节点：**A**（neural-network-based compiler；构造非训练；unisacc 实证）
