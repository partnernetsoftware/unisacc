# c41 K6-26：unisa.gold pp_label → C 参考对拍（0606 授；cc 实现+证据）

## claim / 预算
- claim = **K6-26**（`pp_label` 的**有限域** DIRS × DEFINED 全表 + 命名点）；代裁依据 `ruling-k6-25-pass-accept-0606.md`（授 K6-26）；K6-25 已由 `cdx-k6-25-ruling-0548` 限定 PASS。
- 基：开工时 HEAD = `57e75c17`（main）。
- 墙钟：22:10Z 开工；直跑 22:10:34Z 附近（见 run log）；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra` 无告警）；夹具首跑即全绿。

## 范围
- `pp_label(d, f)`：on 当且仅当 f 恰为 `"1"`；endif→pop；define/undef/include→macro；ifdef→on 时 take；ifndef→off 时 take；if/elif/else→on 时 take；其余（含 ifdef/if 在 off 时）→skip。函数自身无拒绝分支。
- 有限域：`DIRS`（9）× `DEFINED`（2）= **18 行全表**，C 与 python 逐字节相等。
- 命名与边界点 18 个：9 种指令在 DEFINED 两值上的代表组合（ifdef/ifndef/if/elif/else/endif/define/undef/include），以及域外 f 值 `x`（落 off 分支）与域外指令 `bogus`（落 skip）。
- **不覆盖**：reloc、irsel、parse 等其它标签与 Stage 表、`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `DIRS`、`DEFINED`、`pp_label` 三个定义，并断言抽取集合恰为此三个。

## 结果（直跑原始输出）
- `SAME 19 of 19 points`：全表 1 点（18 行）+ 18 个命名与边界点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`ifdef 1 → take`、`ifdef 0 → skip`、`ifndef 0 → take`、`else 1 → take`、`endif 0 → pop`、`include 1 → macro`、`ifdef x → skip`（x 不为 "1"）、`bogus 1 → skip`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-defined`（`call ifdef`，缺 f）：python 包装层缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。
- 说明：`pp_label` 对域外值不抛错，不以域外值作函数级负例；两个负例属包装层，与 K6-20 至 K6-25 一致。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_pp_label.c` | C99 参考：DIRS/DEFINED、pp_label（on 仅为 "1"）；table/call 两个命令 |
| `tests/unisagoldpplabelcheck.sh` | 夹具：python 以 ast 抽取三个定义执行；18 行全表 + 18 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-pp-label` 的 req 行与 job 行 |
| `research/c41-k6-26-gold-pp-label.md` / `research/c41-k6-26-gold-pp-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `seed/unisa_gold_pp_label.c` = `71bdcb29e6cec5e400cac21f0728698cedd0e91ec9f39bda1e731aa5d48a194e`
- `tests/unisagoldpplabelcheck.sh` = `baa94f70ff5030552d269e8efc6fd40f5163f31d377d0718451545b899b8a0cc`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldpplabelcheck.sh`；rc=0；`unisagoldpp same 19 diff 0 neg 2/2`；日志 `research/c41-k6-26-gold-pp-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-pp-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..25 的 seed/夹具闭包；未 regen gold 表、未写 weights；未并刀 reloc、irsel、parse；未触 CX 主叶；未预填 PASS；未跑旧全量。
