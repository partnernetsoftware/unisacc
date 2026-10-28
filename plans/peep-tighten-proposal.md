# peep.tsv 收紧提案（cc，2026-10-28，交表主人裁定；不改 unisa/gold.py、weights/、kernel/）

## 事实

`weights/gold/peep.tsv`（1632 键）由 `unisa/gold.py:peep_label` 经 `gold-export` 生成，再经 `build-weights --from-tsv --check` → `emit-kernel` 进入 kernel/ 与 SID。tools/peep-cases-gen.cx 把每键实现为两指令 tape 在 E4 容器上验证（方法学 §10，批 stream-e4-peep-0..7，d65 与 661c 均 277 PASS / 0 FAIL）。

16 个键原记 TABLE_SUSPECT（金表给非 keep 的 y，而生成器实现不出该关系）。按 src/opt.c 复核后分两类：

### A. 参考分类器永不产生的键（12 个）——建议 y 改为 keep

| 键 | 原 y | 不可达原因（src/opt.c） |
|---|---|---|
| alu.none.a_dead, imm.none.a_dead, load64.none.a_dead, mov.none.a_dead | drop_a | `ol_scan` 到 tape 末尾返回"活"（l ≥ ol_n → 1），b=none 时 rD 永不死 |
| alu.other.a_dead, imm.other.a_dead, load64.other.a_dead, mov.other.a_dead | drop_a | OK_OTHER 行 `ol_rm=255`（读全部寄存器），rD 永不死 |
| mov.imm.copy_into | fold_copy | imm 不读任何寄存器，`ol_rm[i+1] & (1<<ya)` 不成立 |
| mov.jump.copy_into, mov.jumpz.copy_into | fold_copy | copy_into 要求 `ol_k[i+1] == OK_SIMPLE`，jump/jumpz 不是 |
| mov.other.copy_into | fold_copy | 同上，OK_OTHER 不是 SIMPLE |

这些键无行为影响（永不被查询），改为 keep 只是让表与分类器一致。`peep_label` 的最小改法：
`a_dead` 分支加 `and b not in ("none", "other")`；`copy_into` 分支把 `b != "none"` 改为 `b in ("store64", "load64", "mov", <alu 九种>)`。

### B. 原误判为不可达、实为可达且金表正确的键（4 个）——金表不改，生成器已补

| 键 | y | E4 容器实测（out/c43-cc-peep-probe） |
|---|---|---|
| jump.none.to_next, jumpz.none.to_next | drop_a | `jump L1 / L1:`（标签在 tape 末尾，`pk_bcls` 越界即 "none"）→ 跳转被删 |
| other.mov.dest_to_mov | retarget_dest | `.lea r2, g / mov r0, r2` → `.lea r0, g`（.lea 是 acls=other 的 simple 指令） |
| mov.load64.copy_into | fold_copy | `mov r3, r2 / load64 r0, [r3+0]` → `load64 r0, [r2+0]`（load 读基址寄存器） |

## 后果与路线

改 A 的 12 个 y 会改变 weights 字节、kernel/ 与 SID，须走慢车道纪元（gold-export → build-weights --check → emit-kernel → 构造 → 准入）；不在快车道做。建议与下一次需要重建权重的纪元合并，而不是单独为此开纪元。
