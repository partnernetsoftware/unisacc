# 试点 m14.o / rts_at（rtsharedname）

不改产品、不碰 loader。asmtext 不作首试点。旧页 `plans/rtshared-nn-pilot.md` 作摘要，以本页为准。

## 单元身份

| | |
|---|---|
| 对象 | `m14.o` .text **8420** B（k2-precheck-04 sl-lnx，镜像 UNATTRIBUTED） |
| 源 | `cx/compiler/rtsharedname.cx`（`tools/rt-shared-gen.cx` 生成） |
| 表 | `exec/facts/rt-shared.tsv` 2 group |
| default_path | YES |
| 可迁性 | FINITE |
| 已跑 | `RT_SHARED_GEN SAME groups=2 bytes=3191 unlisted=0` |

## 输入 → 输出字节流（提案）

四种 ask，均有限键，输出定长整数或定长 ASCII 字段：

1. **count**：空输入 → 1 字节 `rts_count`（现恒 2）。
2. **at**：1 字节 `i` → `group\0 probe\0 provider\0 unit_macro\0 extern_macro\0`；失败则空载荷 + 状态码。
3. **covers**：`i` + name 字节（最长表内名）→ 1 字节 `{0,1,2,4}`。
4. **used**：同上，对 `used_symbols`。

不把 `rts_same` 循环做成无界扫描网；name 先哈希/定长垫到表内最大名长（`_unisa_natexit` 等），域仍有限。

## gold 全键穷举

域（闭合、可列出）：

- `i ∈ {0,1}` 正例；`i ∈ {255,2}` 越界。
- 指针类用哨兵：缺 name / 缺 out 槽。
- covers 名：tsv `state_symbols` 全员（14+2）+ 1 个表外名 `_not_in_table`。
- used 名：tsv `used_symbols` 全员（2+2）+ 表外名。

键数 ≤ 2×(1+5) + 2×(16+1) + 2×(4+1) + 越界 ≈ **80**，全枚举与 `rts_*` 现实现逐字段/逐返回码 **cmp**。生成器 `--check` 继续钉 C 文本与 tsv。

**不是** 往返等式。

## 拒绝合同（与现函数相同）

| 调用 | 码 |
|---|---|
| `rts_at` 任一 out 指针空 | 4 |
| `rts_at` i∉{0,1} | 2 |
| `rts_covers`/`rts_used` name=NULL | 4 |
| 同上 i 越界 | 2 |
| `rts_symbol_count`/`rts_used_count` 坏 i | -1 |

## 网增量与净收益判定

- 回收上界（对象 .text，**非镜像**）：8420 B。镜像贡献 UNATTRIBUTED，不得把 8420 写成 APE 下降。
- 网增量：**UNMEASURED**（2 组 + ~80 键的 decide 网未构造）。构造后量：新包 `p3` 字节 − 旧包字节 = `Δpkg`；切片 gzip 下降 = `Δslice`（同候选、五片分列）。
- **判定：仅当 `Δpkg < Δslice` 且 `Δslice>0` 才做**。`Δslice` 未知或 `Δpkg≥Δslice` → 不做，保留生成 C 表。
- 执行器 8B/输入字节：本域输入 ≤ 约 32B 名垫，属性约 256B，不触及 32MiB 放大；OOM panic 合同仍独立（CORE-INPUT-MEMORY-OOM）。
- 每字节步数：UNMEASURED。

## 可迁性类别 × .text 合计（unit-text.tsv 81 单元）

口径：对象 `.text` 节，不是镜像。MIXED 并入「需大状态」（未证明有限）。

| 主人四类 | 表标签 | n | .text 合计 | 占 1845680 |
|---|---|---:|---:|---:|
| 纯变换 | TRANSFORM | 3 | 24361 | 1.3% |
| 有限状态 | FINITE | 6 | 48146 | 2.6% |
| 需大状态 | UNBOUNDED+MIXED | 16+41 | 1362321 | 73.8% |
| 系统 IO | SYS-IO | 15 | 410852 | 22.3% |

compiler 大头多在 UNBOUNDED/MIXED（cxmod/tapegate/asmtext/source_frame），FINITE 全表仅 48KB 级，本试点 8420 是其中一块。

## 不做

不实施直到 fable 批本页且能量出 `Δpkg`/`Δslice`。不迁 host/runexec/execute/model_decode。
