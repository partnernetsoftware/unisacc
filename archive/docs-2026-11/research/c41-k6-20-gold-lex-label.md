# c41 K6-20：unisa.gold lex_label → C 参考对拍（0344 授；cc 实现+证据）

## claim / 预算
- claim = **K6-20**（`lex_label` 有限域）；代裁依据 `ruling-k6-20-gold-lex-auth-0344.md`（以 `ruling-k6-19-pass-accept-0344.md` 所载授权为准）；K6-19 已限定 PASS（0344 接纳）。
- 基：开工时 HEAD = `c4a8ff8b`（main）。
- 墙钟：19:46Z 开工；直跑 19:47:08Z–19:47:10Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra`，无告警）；python 首跑即全绿。

## 范围
- `lex_label(c, peek)`：先处理注释起始（`slash` 后接 `slash` → `linecmt`，接 `star` → `cmt`）与点规则（`dot` 后接 `d` → `num`，其它 → `op`），其余由类表给出。
- 有限域：`CHARC` 的 12 个类 × peek ∈ {`slash`、`star`、`d`、`other`}，共 48 行全表；这四个 peek 值覆盖 lex_label 唯一区分的分支（注释起始、数字起点、其它）。
- 命名点 15 个：注释起始组合、点规则（`d` 与非 `d`）、类表各类。
- **不覆盖**：gold 的 Stage 表、其它标签函数（type/parse/peep/irsel/opinfo/binsel/prec 以外的 lex 相关表）、`build()`、表的 regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 与 K6-18 相同：不 import `gold.py`（其模块级 `build()` 不被调用），用 `ast` 抽取 `CHARC` 与 `lex_label` 两个定义，执行并断言抽取集合恰为这两个名字。

## 结果（直跑原始输出）
- `SAME 16 of 16 points`：全表 48 行一次对拍（`table` 模式，C 与 python 逐字节相等），加 15 个命名点逐一对拍；C 双跑确定。
- 代表值：`slash slash → linecmt`、`slash star → cmt`、`dot d → num`、`dot other → op`、`slash other → op`、`ws other → skip`、`other other → bad`。

## 负例（2 个，两侧同拒，stdout 空）
- `unknown-class`（`call bogus slash`）：python KeyError；C 拒绝（rc 1）。
- `missing-peek`（`call dot`）：python 缺参（IndexError）拒绝；C 用法错误（rc 2）。

## 边界（不称）
- 点采样：有限域全表与 15 个命名点；不证 gold 其它标签与 Stage 表；不证 build()、权重或训练；不称产品闭包。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_lex_label.c` | C99 参考：CHARC（12）、PEEKS（4）、lex_label、table/call 两个命令 |
| `tests/unisagoldlexlabelcheck.sh` | 夹具：python 以 ast 抽取 CHARC/lex_label 执行；48 行表 + 15 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-lex-label` 的 req 行与 job 行 |
| `research/c41-k6-20-gold-lex-label.md` / `research/c41-k6-20-gold-lex-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_gold_lex_label.c` = `93abb6507270acf7f4a13e7eb22793e4f13621aadcbd373ec645e36dc0a266ae`
- `tests/unisagoldlexlabelcheck.sh` = `86021945c360aa6e6cf719a26b577ae94d055420e41c98d7959aaa16c2732eec`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldlexlabelcheck.sh`；UTC 19:47:08Z–19:47:10Z；rc=0；`unisagoldlex same 16 diff 0 neg 2/2`；日志 `research/c41-k6-20-gold-lex-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-lex-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..19 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 front.lex、emit_x86、17b；未预填 PASS；未跑旧全量。
