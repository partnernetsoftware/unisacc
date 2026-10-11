# c41 K6-29：unisa.gold parse_label → C 参考对拍（0722 授；cc 实现+证据）

## claim / 预算
- claim = **K6-29**（`parse_label` 的**阶段域** NT × TOKS 全表 + 命名点）；授权依据 `ruling-k6-28-pass-accept-0722.md`（串行 irsel → parse 的次叶，本拍只授本叶）；K6-28 已由 `cdx-k6-28-ruling-0709` 限定 PASS。
- 基：开工时 HEAD = `503bb421`（main）。
- 墙钟：23:24Z 开工；直跑 23:25:20Z–23:25:22Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra` 无告警）；夹具首跑即全绿。

## 范围
- 阶段域：`NT`（5：top、stmt、unary、postfix、after_name）× `TOKS`（68）= **340 行全表**，按 gold.py 的 parse 阶段定义与插入序逐条对拍。这是阶段自身声明的域（`S["parse"]` 的 fields），**不**冒称为任何前端的全 token 宇宙。
- `parse_label(nt, tok)`：各非终结符的规则按 gold.py 序判定，无规则应答时回落到 `PARSE_DEFAULT`；`SAME_NAME`（11 个句首同名关键字）、`unary` 的映射（9 键，默认 `prim`）、`union` 别名（top→struct、stmt→decl）均具名覆盖。
- 标签集合：全表 340 个输出均须落在 `PRODS`（36 个）之内，由夹具断言。
- 命名与边界点 22 个：top 的 eof/struct/union/typedef/id；after_name 的 `(`、`{`、`;`（落回 var_def）；stmt 的 type/union/`{`/if/goto/num；unary 的 `-`、sizeof、`~`、域外 token `nosuchtok`（落 prim）；postfix 的 `[`、`++`、`.`、`)`（落 done）。
- **不覆盖**：前端的 parse 产品路径、全 token 宇宙、`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `NT`、`TOKS`、`PRODS`、`SAME_NAME`、`PARSE_DEFAULT`、`parse_label` 六个定义，并断言抽取集合恰为此六个。

## 结果（直跑原始输出）
- `SAME 23 of 23 points`：全表 1 点（340 行）+ 22 个命名与边界点，C 与 python 逐字节相等；C 双跑确定；全表标签均在 PRODS 内。
- 代表值：`top union → struct`、`stmt union → decl`、`stmt goto → goto`、`after_name ( → fn_sig`、`after_name ; → var_def`、`postfix ) → done`、`unary sizeof → sizeof`、`unary nosuchtok → prim`。

## 负例（3 个，两侧同拒，stdout 空）
- `unknown-nonterminal`（`call bogus eof`）：**函数级**拒绝——python `PARSE_DEFAULT["bogus"]` KeyError；C 拒绝（rc 1）。这是本叶的函数语义负例。
- `missing-token`（`call top`）：包装层缺参（python rc 1；C 用法错误 rc 2）。
- `unknown-mode`（`build`）：包装层（两侧拒绝）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_parse_label.c` | C99 参考：NT/TOKS/SAME_NAME/PARSE_DEFAULT（由 gold.py AST 生成嵌入）、parse_label（规则序与回落）；table/call 两个命令 |
| `tests/unisagoldparselabelcheck.sh` | 夹具：python 以 ast 抽取六个定义执行；340 行全表 + 22 点字节对拍；标签 ⊂ PRODS 断言；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-parse-label` 的 req 行与 job 行 |
| `research/c41-k6-29-gold-parse-label.md` / `research/c41-k6-29-gold-parse-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `seed/unisa_gold_parse_label.c` = `ab563bc1d8c31eb3994e848a4103d86c8081a2d1d856fcad21ea5c39622281a5`
- `tests/unisagoldparselabelcheck.sh` = `a7e0a53dd2fae31e615379397211bc813d6fae7a3a37b0a3cee6370fce14562d`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldparselabelcheck.sh`；UTC 23:25:20Z–23:25:22Z；rc=0；`unisagoldparse same 23 diff 0 neg 3/3`；日志 `research/c41-k6-29-gold-parse-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-parse-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..28 的 seed/夹具闭包；未 regen gold 表、未写 weights；未并刀其它 gold 标签或前端 parse 产品路径；未触 CX 主叶；未预填 PASS；未跑旧全量。
