# 0.0.41 收口清单（对照 archive/plans/v0.0.41.md；0749 代裁材料）

**状态**：材料 only；配套 `ruling-c41-closeout-ready-0749.md`。**不**预填 freeze PASS；**不** bump；**不**动 Latest。

源：tip `2c909b26`；产品叶 `e36b5962`（K6-29）；version.h **0.0.40**；Latest **v0.0.40** assets=10；K6-29 限定 PASS 已纳（0744）。

---

## 1. 计划门闩对照

| 计划项 | 口径 | 现态 |
|---|---|---|
| 优化管道（域/预算/首红停；证据→复算） | 开工门闩 | **通过**（K6-1…29 节奏） |
| Latest≥0.0.40 / 收口可开 41 | 开工门闩 | **通过**（Latest=v0.0.40） |
| K6 盘点 K6-0 | 具名 py+依赖+窄切口 | **通过**（`research/c41-k6-inventory-cut.md`） |
| 不删 Python | 本版边界 | **守住** |
| 不二次 publish / 扩签 / 动 Latest | 本版边界 | **守住** |
| bump version.h→0.0.41 | 候选构建前必须；未授执行 | **未做（正确）** |
| H1 INHERIT/新建 | 待批 | **OPEN — 冻前须裁** |
| 不自动开 0.0.42 K7 | 边界 | **守住** |
| paper-a / CX 旁注 ≠ 产品义务 | 边界 | **分账**（E #217 等） |

## 2. 已落地 K6 叶（限定域；非全树）

K6-0 盘点；K6-1 bits；K6-2 hostabi；K6-3 peversion；K6-4/4b/4c padded+zdata；K6-5 linalg；K6-6 libneed；K6-7 uns1；K6-8 catalog（点采样；全表=K6-8b 另排）；K6-9 fp；K6-10 tape；K6-11/11b tapebin(+shape)；K6-12…15 image fat/elf/macho/pe；K6-16 ape；K6-17 vm；K6-18…29 **全部 gold `*_label`**；K6-19 emit_arm N/w/mem（窄）。

各叶回执在 `research/c41-k6-*.md`；gate job 已挂；Python **保留**。

## 3. gold 端口结算

- 具名 `*_label`：**11/11 齐**；串行余序 irsel→parse **已尽**。
- Stage 残表 tyinfo/pfconv/enc/regmap/isel/abi/combo + build() 机械：**未端口**；本版建议标「非本战役义务 / 另授」。
- 不称：gold 全闭包、`build()`、权重/训练、前端 parse 产品路径。

## 4. 全 unisa 缺口（计划「必须完成」张力）

约 24 个模块无专用 seed C（见 ruling §C）。  
**收口动作（须政委裁一）**：
- **(a)** 书面顺延 → 0.0.42+（推荐默认：与「不删 py」一致，本版只结算已授叶战役）；或
- **(b)** 开新战役补叶（须另授队列，非本清单自动开工）。

未裁前：**不可**把 0.0.41 写成「K6 主题产品闭包完成」。

## 5. freeze / bump 序（授权后；本清单不启动）

1. 政委书面：收口可冻 + §4 处置 + H1 批 + Stage 残表处置。
2. 单独提交 `src/version.h` → `0.0.41`（freezecheck 起点）。
3. build_ref / build_candidate / ident；comboot；gatedeps 冻点；queue；seal；rc tag。
4. Draft / 签 / 公开 — **另授**；本窗禁动 Latest v0.0.40。

## 6. 本拍禁项自检

未写 src/；未 bump；未 Draft；未 Latest；未签；未删 py；未授实现叶；未改验收；未预填产品 PASS。

*材料 2026-10-12 ≈07:49 SGT*
