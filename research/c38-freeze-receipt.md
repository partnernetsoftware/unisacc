# 0.0.38 冻结身份回执（模板，逐项实测后填写；未填 = 未执行，不预写 PASS）

授权：董秘代裁 0.0.38 版本冻结可开（2026-10-10，授权到达时刻 09:57 +0800）。顺序：§24 生成器全矩阵预验 → 版本号提交 → seed 内存证据一次采 → build_ref → build_candidate → comboot 定点 → queue 出口。同机重活串行，单令牌。

| 字段 | 值 | 来源 / 时刻 |
|---|---|---|
| 授权前 HEAD | b8ed07cf | git |
| seed/gen.c sha256 前缀 | b802e4f2d2d7f10a | 72120ea8 起 |
| source_digest（版本提交前） | c7e75006 | exec/c/provenance.py |
| §24 矩阵预验 owner / begin / end / 结论 | cdx / — / — / — | |
| 版本提交 / source_digest | — / — | |
| 内存证据键（C_SHA、REFERENCE_KEYS、cc 实体/flags/并发）/ 采证时刻 | — | tests/seedmemory.py |
| 同源参考 UA 路径 / sha | — | build_ref.sh |
| 候选路径 / sha / sources_sha256 | — | build_candidate.sh |
| seed 对 / comboot stage2=stage3 定点 | — | comboot.py |
| queue state / 证据目录 / 最终 rc | — | ~/.unisacc/evidence/<run>-final（含 SHA256SUMS） |
| 出口表（exittable）PASS / RULED / H1 / H2 / UNKNOWN / 77 / NEEDS_RULING | — | release/tools/exittable.py |

出口表目标：除已裁 RULED/H1/H2/77 外无 NEEDS_RULING；旧 740007ef 与 ver038 证据不签新闭包。
