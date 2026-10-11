# c41 K6-23：unisa.gold opinfo_label → C 参考对拍（0507 授；cc 实现+证据）

## claim / 预算
- claim = **K6-23**（`opinfo_label` 的**有限 op 域**全表 + 命名/边界点）；代裁依据 `ruling-k6-22-pass-accept-0507.md`；K6-22 已由 `cdx-k6-22-ruling-0500` 限定 PASS。
- 基：开工时 HEAD = `ebd93cc2`（main）。
- 墙钟：21:09Z 开工；直跑 21:09:57Z–21:09:59Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra`，无告警）；夹具首跑即全绿。
- 命令层失误（非源码/编译红）：一次为编译输出路径写到 `$TMPDIR`（空）导致链接失败，属本次 shell 命令错误，改用 scratchpad 路径后编译通过；不计为源码首红，已如实记录。

## 范围
- `opinfo_label(op)`：simple = 1 当且仅当 op 在 `OPINFO_SIMPLE`；acls = op（限于 store64/load64/imm/mov/jump/jumpz），否则 `alu`（op 在 `OPINFO_ALU`），否则 `other`；bcls = op（当且仅当 op 在 `PEEP_B` 且不为 `other`/`none`），否则 `other`。
- 有限域：`OPINFO_OPS` = `tape.SHAPE` 的 71 个算符（含 fp）+ `other` = **72 行全表**，C 与 python 逐字节相等。C 端的算符名表由 python 从 `tape.SHAPE` 按插入序生成后嵌入源码；若两者漂移，全表对拍会报 DIFF。
- 命名与边界点 18 个：`imm`、`load64`、`store64`、`mov`、`jump`、`jumpz`、`add64`、`mul64`、`sub64`、`eq`、`slt64`、`ult64`、`.ld`、`.lea`、`other`、`fadd64`（fp 算符）、`none`（PEEP_B 的排除项）、`bogus`（域外名字，走默认分支）。
- **不覆盖**：peep / irsel / parse / 其它标签、整个 gold Stage 表、`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `_tape_ops`、`OPINFO_OPS`、`OPINFO_SIMPLE`、`OPINFO_ALU`、`PEEP_B`、`opinfo_label` 六个定义，并断言抽取集合恰为此六个。
- `_tape_ops` 内的相对导入 `from .tape import SHAPE` 在私有命名空间中改写为绝对导入 `unisa.tape`（同一只读模块，未修改 tape.py）。

## 结果（直跑原始输出）
- `SAME 19 of 19 points`：全表 1 点（72 行）+ 18 个命名与边界点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`imm → 1 imm imm`、`add64 → 1 alu add64`、`fadd64 → 0 other other`、`other → 0 other other`、`none → 0 other other`、`bogus → 0 other other`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-op`（`call` 缺算符参数）：python 包装层缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。
- 说明：`opinfo_label` 对域外名字返回默认分支而非抛错，因此不以域外名字作为函数级负例；这两个负例属于包装层，与 K6-20/21/22 的同类负例一致。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_opinfo_label.c` | C99 参考：OPS（72，由 tape.SHAPE 生成）、SIMPLE/ALU/PEEP_B、opinfo_label；table/call 两个命令 |
| `tests/unisagoldopinfolabelcheck.sh` | 夹具：python 以 ast 抽取六个定义执行（_tape_ops 相对导入改绝对）；72 行全表 + 18 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-opinfo-label` 的 req 行与 job 行 |
| `research/c41-k6-23-gold-opinfo-label.md` / `research/c41-k6-23-gold-opinfo-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/tape.py` = `2292a69dbe7e9e7662066af8e90bec2ba59317ddc539862a2acbef41f37bb674`（只读）
- `seed/unisa_gold_opinfo_label.c` = `22c455cc19f1040e02da8f4c9e96e62b26ec72590a4fb751808b6fa4294b074c`
- `tests/unisagoldopinfolabelcheck.sh` = `870a278859c558f1d7feb3fc0115bce0c1e2e6199e93108bd1a1a7af86797621`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldopinfolabelcheck.sh`；UTC 21:09:57Z–21:09:59Z；rc=0；`unisagoldopinfo same 19 diff 0 neg 2/2`；日志 `research/c41-k6-23-gold-opinfo-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-opinfo-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..22 的 seed/夹具闭包；未 regen gold 表、未写 weights；未并吞 peep/irsel/parse、未开 front.lex、emit_x86、17b；未预填 PASS；未跑旧全量。
