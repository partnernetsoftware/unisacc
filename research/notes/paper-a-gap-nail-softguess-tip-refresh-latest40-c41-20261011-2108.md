# Paper A · 真缺口钉：Softguess tip-refresh @ Latest40+c41（仍 NONE×4 · 非 seal-before）

- **拍点：** 2026-10-11 ~21:08 Asia/Shanghai（UTC+8）
- **本拍切口：** **C. 真缺口钉**
- **主张一句：** **Softguess 字面与「权重就是逻辑」口号在 tip `68af41ff` 四 Paper A blob 上仍 NONE×4**（机扫方法见下）；「不含/no softmax」义务句仍在 CN/EN/TeX（abstract 无 softmax 字面）；**NONE×4 是预期态，不是封口前理论债**——义务已由定义 2/3、算法 1 步 8、§3 验证路径密封；Softguess 字面粘贴仍属 **P0 命名卫生（UNAPPLIED）**，不挡主张层封口。本拍**不**重溯 8509 Δ、**不**叠 Ο1/Ο2。
- **仓库 tip（核对）：** `68af41ff5d3812e66006e61bb03c31e7576c1cfb`（含 #152 Δ 溯源钉 `88bb2f7f` 之后的 tip c41 K6-7）
- **公开 Latest：** **v0.0.40**（unsigned @ `1db30835…`）；Paper A 冻结重测身份仍 **v0.0.19**（本拍不改）
- **四 blob @ tip（gh content sha）：** CN `c37b9291a6cc` · EN `fad7b3e0b517` · TeX `5c880b0bafd8` · abs `43d7ea7e7297` — 相对上一心跳（#152 / 2054 钉）**SAME**（本拍零正文 diff）
- **Softguess：** **NONE×4**（见机核）
- **角色：** Softguess **tip-refresh 可引用钉**（Latest40+c41 后机证）；升格/刷新 [`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md) 的 tip 证据，不改其 OUT-OF-SCOPE 判决
- **父节点：** **A**
- **DISTINCT：**
  - vs [`paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md`](paper-a-gap-nail-softguess-seal-boundary-20261010-1230.md)（**C** @ tip `bd352750`）：本页主对象是 **Latest40+c41+K6-7 后 tip `68af41ff` 的机扫刷新**，不是重写封口边界判决。
  - vs [`paper-a-gap-nail-weights-eq-logic-scope-20261010-1154.md`](paper-a-gap-nail-weights-eq-logic-scope-20261010-1154.md)（**C**）：本页主扫 Softguess/口号 NONE；作用域合同仍引用该钉，不重写合法/非法栏。
  - vs [`paper-a-gap-nail-8509-8769-delta-20261011-2054.md`](paper-a-gap-nail-8509-8769-delta-20261011-2054.md)（**C** Δ）：正交；本钉**不**重做 Δ。
  - vs [`paper-a-derive-purify-unisa-linalg-libneed-k6-5-6-not-a-gate-20261011-2046.md`](paper-a-derive-purify-unisa-linalg-libneed-k6-5-6-not-a-gate-20261011-2046.md)（**E**）：本钉是 Softguess 纪律对偶，不是旁支 hygiene。
  - vs [`paper-a-derive-purify-softguess-approx-product-path-20261010-2355.md`](paper-a-derive-purify-softguess-approx-product-path-20261010-2355.md)（**E**）：本钉不把 Softguess 升格为独立产品路径论文。

---

## 主张一句（本页唯一）

**@ tip `68af41ff`（Latest v0.0.40 + tip c41 后）：Softguess / 「权重就是」 / “weights are the logic” / 「权重≡|即逻辑」 / 「逻辑本身」在 CN+EN+TeX+abstract 上仍 NONE×4；「不含/no softmax」义务句仍在正文三件；NONE×4＝预期态（义务已 seal），≠ seal-before 硬缺口；字面粘贴仍等政委批 P0，本拍不粘、不叠测量身份卡。**

---

## 机核方法（可复现）

1. 自 `origin/main` tip `68af41ff…` 经 `gh api …/contents/{path}?ref=main` 拉取四文件：
   - `research/unisacc-paper.md`
   - `research/unisacc-paper.en.md`
   - `research/arxiv-paper-a/main.tex`
   - `research/arxiv-paper-a/abstract.txt`
2. 机扫字面（大小写不敏感仅对 ASCII 口号）：
   - `Softguess` / `softguess`
   - `权重就是`
   - `weights are the logic`（空白折叠）
   - `权重≡逻辑` / `权重即逻辑`
   - `逻辑本身`
3. **判据 Softguess NONE×4：** 四文件上 `Softguess` 字面命中数＝0。
4. **判据口号 NONE×4（历史 ALL_PASS 族）：** 上列其余口号命中数＝0。
5. **对照（非 Softguess 字面）：** `softmax` 仅允许出现在「不含 / uses no softmax」义务句；本拍 CN/EN/TeX 各 1 处（§ 验证路径），abstract＝0。**「no softmax」≠ Softguess 字面出现。**

### 机核结果 @ 2026-10-11 21:08 Asia/Shanghai

| 文件 | Softguess | 权重就是 | weights are the logic | 权重≡\|即逻辑 | 逻辑本身 | softmax（对照） |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| CN | 0 | 0 | 0 | 0 | 0 | 1（「不含 softmax」） |
| EN | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| TeX | 0 | 0 | 0 | 0 | 0 | 1（「no softmax」） |
| abstract | 0 | 0 | 0 | 0 | 0 | 0 |

- **Softguess_NONE_x4 = true**
- **weights_slogan_NONE_x4 = true**
- **ALL_PASS = true**
- 机核 JSON 镜像：`/tmp/paper-a-beat-2108/softguess-verify-20261011-2108.json`（本拍工作区；不入仓）

---

## tip 已 seal 义务 ↔ Softguess 对偶（只读摘 @ tip `68af41ff`）

| tip 位点 | 密封义务 | Softguess 对偶（若发生） |
| --- | --- | --- |
| **定义 2 / Definition 2** | \(\operatorname{argmax}\) 唯一且 \(=G_s(k)\) | 并列仍出标签 |
| **定义 3 / Definition 3** | 不读 logits；\(k\notin K_s\) 断言失败 | Softmax / 读 logits 选类 |
| **算法 1 步 8** | 有错或并列则拒绝发布 | 破平出货 |
| **§3 验证路径** | 「不含 / uses no softmax」 | Softmax 路径 |

因此：**REFUSE ≠ Softguess** 已由 tip 义务蕴含；字面是注释，不是新公理。封口边界钉（2026-10-10）判决 **仍有效**：Softguess 字面 ∈ OUT-OF-SCOPE · A seal-before。

---

## 与权重≡逻辑作用域的耦合（不重写合同）

- tip 上「权重就是逻辑」口号仍 NONE（上表）。
- 合法/非法作用域仍以 [`paper-a-gap-nail-weights-eq-logic-scope-20261010-1154.md`](paper-a-gap-nail-weights-eq-logic-scope-20261010-1154.md) 为准。
- **耦合规则（重申，不新开卡）：** 未粘 Softguess 对偶名之前，对外口语「权重就是逻辑」须附带该钉非法作用域收缩；粘贴后 Softguess 句封死「破平仍叫精确」。

---

## 与待批板关系（不叠卡）

- **不叠** Ο1/Ο2 测量身份决策卡（无新信息；荐 Ο1 仍有效，见 2054 Δ 钉）。
- Softguess **字面**粘贴仍等政委批 **P0**（keep-keycounts + Softguess AFTER）；本拍**不粘** CN/EN/TeX/abstract。
- 双身份 8509/8769：**Δ 已解释（2054）≠ 身份已闭合**；仍 seal-blocking；本钉不触键数。
- 真 seal-before 硬缺口仍是：§8.1#3 矩阵、测量身份选定、（披露纪律）——**不是 Softguess NONE**。

---

## 本拍明确不做

- 不粘 Softguess / keep-keycounts / 作用域锚进 tip 正文
- 不改正文 CN/EN/TeX/abstract；不改键数身份 / kernel / weights / facts
- 不重做 8509 Δ；不叠 Ο1/Ο2；不把 Softguess 升格为衍生课题
- 不声称 Latest v0.0.40 或 tip c41 已闭 Softguess「缺口」（因它本就不是 seal-before 缺口）

---

## 证据

- tip：`68af41ff5d3812e66006e61bb03c31e7576c1cfb`
- Paper A blobs：SAME（sha 见上）
- Softguess / 口号：**NONE×4**；softmax 义务句：CN/EN/TeX 各 1
- 正交钉：Softguess seal-boundary 20261010-1230；weights≡logic scope 20261010-1154；Δ provenance 20261011-2054
- 机核：`/tmp/paper-a-beat-2108/softguess-verify-20261011-2108.json`
