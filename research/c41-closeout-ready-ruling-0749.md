# 代裁裁定：0.0.41「可进收口」（0749 SGT）— 不授下一窄叶；禁 bump/Latest

**依据**：K6-29 限定 PASS 已纳（`ruling-k6-29-pass-accept-0744.md` ← `cdx-k6-29-ruling-0735.md`）；产品叶 `e36b5962`；tip `2c909b26`；`version.h`=**0.0.40**；Latest=**v0.0.40**；archive/plans/v0.0.41.md；gold.py sha256 `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`；0744 observe 前瞻 paste（供参考，已独立核）。

## 结论（一句话）

**可进收口**——金标具名 `*_label` 串行战役（K6-18…K6-29）已尽，**无明确下一窄叶可授**；本拍**不授** K6-30/任何新实现叶；**不** bump、**不** Draft、**不**动 Latest、**不**签、**不**删 py。收口清单见同拍 `c41-closeout-checklist-0749.md`。

## 盘点：剩余 gold 未端口符号

### A. 具名 `*_label` 函数（串行队列对象）— **0 剩余**

| 符号 | 叶 | 态 |
|---|---|---|
| prec_label | K6-18 | 限定 PASS |
| lex_label | K6-20 | 限定 PASS |
| type_label | K6-21 | 限定 PASS（含 _farith/_promote/_uns/_arith） |
| binsel_label | K6-22 | 限定 PASS |
| opinfo_label | K6-23 | 限定 PASS（含 _tape_ops） |
| peep_label | K6-24 | 限定 PASS |
| scope_label | K6-25 | 限定 PASS |
| pp_label | K6-26 | 限定 PASS |
| reloc_label | K6-27 | 限定 PASS |
| irsel_label | K6-28 | 限定 PASS |
| parse_label | K6-29 | 限定 PASS（已纳） |

K6-27/28 钉死的「金标标签余序 **irsel → parse**」在 K6-29 后**已空**。

### B. `build()` Stage 残表 / 非 `*_label` 符号 — **仍在；非本战役已钉队列**

| Stage / 符号 | label 来源 | 备注 |
|---|---|---|
| tyinfo | gold 内联 lambda（_SZ） | 有限域；**未**入 irsel→parse 队列 |
| pfconv | gold 内联 lambda（_PF） | 有限域；同上 |
| enc | `catalog.enc_form` | K6-8 已点采样；非 gold 本地 def |
| regmap | `catalog.REGMAP` | catalog 表 |
| isel / abi / combo | `catalog.isel_two` / `nine` | STAGE_NETS / combo；体量大 |
| Stage / build / STAGES / KEYWORDS_C / TABLES / STAGE_NETS / ALL / _one / _type_weight | 机械/元数据 | 各叶明文「不称 build()/全闭包」 |

→ **不**把 B 自动升成「明确下一窄叶」。若要切 tyinfo/pfconv 等，须**另具名授**新战役切片。

### C. 计划「必须完成」对照 — 全 unisa C 参考**未**齐

仍无专用 seed C 的模块（非穷举验收，只列缺口）：`assemble` `ckernel` `construct` `control/{net,rng,segment,train}` `docgen` `driver` `emit_x86` `exec_target` `front/{lex,parse,pp,sema}` `intnet` `ir` `lower` `opt` `oracle` `prune` `tsvgold` `uns2` `__main__` 等。  
计划边界仍：**不删 Python**（删档另授）。  
**可进收口 ≠ 冒称 K6 全树逐字节闭包已齐 ≠ 可 freeze/bump。**

## 需否再开窄叶？

**否（本拍）**——无「明确」已钉下一叶（队列空；paste「若仍有…似可另开」为条件建议，非具名授权对象）。  
若政委后裁要开 Stage 残表（优先候选 **tyinfo** 或 **pfconv**，有限域、≤30m 友好），须新 ruling 具名文件域+预算+首红停；**本文件不授**。

## freeze / version bump 前提（未授执行）

| # | 前提 | 本拍态 |
|---|---|---|
| F1 | 收口清单入观察/research 且政委书面「收口可冻」 | **本拍起清单**；冻令**未**发 |
| F2 | H1 INHERIT/新建已批（archive/plans/v0.0.41.md 仍「待批」） | **缺** |
| F3 | 剩余 unisa 模块处置：顺延 0.0.42+ **或** 补战役 — 书面钉死 | **缺**（须裁） |
| F4 | gold Stage 残表（B）处置：顺延 / 另授窄叶 / 明示非本版义务 | **缺**（本裁定建议默认：非本版义务、另授） |
| F5 | 删 Python：**仍禁**（计划明文） | 保持禁 |
| F6 | `version.h`→0.0.41 **仅**在冻令后、候选构建前单独提交 | **未授**；现仍 0.0.40 |
| F7 | freezecheck → build_ref/candidate → queue/seal 链（照 c39/c40 序） | **未授** |
| F8 | Draft / 签 / 动 Latest v0.0.40 / 二次 publish | **禁**（本窗+计划） |

## 本拍明确授/不授

| 项 | |
|---|---|
| 授 | 写收口清单材料；口径「可进收口」（战役结算预备） |
| 不授 | 任何 K6-30+ 实现叶；bump；Draft；Latest；签；删 py；改验收；并刀 CX；预填产品 PASS |

*代裁 2026-10-12 ≈07:49 SGT / Asia/Singapore*
