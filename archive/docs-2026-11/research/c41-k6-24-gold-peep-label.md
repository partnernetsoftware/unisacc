# c41 K6-24：unisa.gold peep_label → C 参考对拍（0517 授；cc 实现+证据）

## claim / 预算
- claim = **K6-24**（`peep_label` 的**有限域**全表 + 命名点）；代裁依据 `ruling-k6-23-pass-accept-0517.md`；K6-23 已由 `cdx-k6-23-ruling-0507` 限定 PASS。
- 基：开工时 HEAD = `16ba652a`（main）。
- 墙钟：21:20Z 开工；直跑 21:20:32Z–21:20:33Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra` 无告警）；夹具首跑即全绿。

## 范围
- `peep_label(a, b, rel)`：按 gold.py 的规则序判定；`store64` 块仅对 b ∈ {load64, store64} 作答，其它 b 继续落入后续规则（落空语义）；无匹配则 `keep`。函数自身无拒绝分支。
- 有限域：`PEEP_A`（8）× `PEEP_B`（17）× `PEEP_REL`（12）= **1632 行全表**，C 与 python 逐字节相等。
- 命名点 18 个：store64 的两条 same_slot 规则、store64 落空（`store64 mov same_slot` → keep）、`const0`/`const1`、`copy_dead`、`to_next`、`to_jump`、`a_dead`、`dest_to_mov`、`copy_into`（含 b=none 与 b=store64 的 a=mov 分支）、以及默认 keep。其中 `load64 none keep_probe` 用的 rel `keep_probe` 不在 PEEP_REL 内，作为域外边界，同样应落 keep。
- **不覆盖**：gold 的 Stage 表、其它标签、`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `PEEP_A`、`PEEP_B`、`PEEP_REL`、`peep_label` 四个定义，并断言抽取集合恰为此四个。

## 结果（直跑原始输出）
- `SAME 19 of 19 points`：全表 1 点（1632 行）+ 18 个命名点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`store64 load64 same_slot → load_to_mov`、`store64 load64 same_slot_same_reg → drop_b`、`store64 store64 same_slot → drop_a`、`store64 mov same_slot → keep`（落空）、`imm add64 const0 → to_mov`、`imm mov copy_dead → fold_imm`、`jump jump to_next → drop_a`、`jumpz jump to_jump → retarget`、`other mov dest_to_mov → retarget_dest`、`mov store64 copy_into → fold_copy`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-operand`（`call store64 load64`，缺 rel）：python 包装层缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。
- 说明：`peep_label` 对域外值不抛错（落空为 keep），因此不以域外值作为函数级负例；这两个负例属包装层，与 K6-20 至 K6-23 一致。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_peep_label.c` | C99 参考：PEEP_A/PEEP_B/PEEP_REL 元组、peep_label（规则序与落空）；table/call 两个命令 |
| `tests/unisagoldpeeplabelcheck.sh` | 夹具：python 以 ast 抽取四个定义执行；1632 行全表 + 18 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-peep-label` 的 req 行与 job 行 |
| `research/c41-k6-24-gold-peep-label.md` / `research/c41-k6-24-gold-peep-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `seed/unisa_gold_peep_label.c` = `7415b2a239bbe20b04ff294b8cd649ee7f302a0f230fa15b38058e7fed48248d`
- `tests/unisagoldpeeplabelcheck.sh` = `6d4b3a04c3e4d3124d47fa4c71c31336365b5ed085f6563449dfa4fa1fed2433`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldpeeplabelcheck.sh`；UTC 21:20:32Z–21:20:33Z；rc=0；`unisagoldpeep same 19 diff 0 neg 2/2`；日志 `research/c41-k6-24-gold-peep-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-peep-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..23 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 front.lex、emit_x86、17b；未触 CX 主叶；未预填 PASS；未跑旧全量。
