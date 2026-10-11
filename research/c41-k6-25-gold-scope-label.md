# c41 K6-25：unisa.gold scope_label → C 参考对拍（0545 授；cc 实现+证据）

## claim / 预算
- claim = **K6-25**（`scope_label` 的**有限域** CTX × KIND 全表 + 命名点）；代裁依据 `ruling-k6-24-pass-accept-0545.md`（授 K6-25）；K6-24 已由 `cdx-k6-24-ruling-0529` 限定 PASS。
- 基：开工时 HEAD = `f2806d67`（main）。
- 墙钟：21:47Z 开工；直跑 21:48:12Z–21:48:13Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra` 无告警）；夹具首跑即全绿。

## 范围
- `scope_label(ctx, kind)`：top 上 TYPEKW→type_name、id→bind_global、lparen→fn_name；param 上 id→bind_param；local 上 id→bind_local、TYPEKW→type_name；sizeof 上 TYPEKW→type_name；field 上 id→field；其余一律 lookup。函数自身无拒绝分支。
- 有限域：`CTX`（6）× `KIND`（5）= **30 行全表**，C 与 python 逐字节相等。
- 命名与边界点 17 个：top 上五种 kind、param/local/sizeof/field/expr 的代表组合，以及域外 kind `bogus`（落 lookup）。
- **不覆盖**：gold 的 Stage 表、其它标签、`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `CTX`、`KIND`、`TYPEKW`、`scope_label` 四个定义，并断言抽取集合恰为此四个。

## 结果（直跑原始输出）
- `SAME 18 of 18 points`：全表 1 点（30 行）+ 17 个命名与边界点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`top lparen → fn_name`、`field id → field`、`expr id → lookup`、`param type_kw → lookup`（param 只对 id 绑定）、`sizeof type_kw → type_name`、`top bogus → lookup`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-kind`（`call top`，缺 kind）：python 包装层缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。
- 说明：`scope_label` 对域外值返回 lookup 而不抛错，不以域外值作函数级负例；两个负例属包装层，与 K6-20 至 K6-24 一致。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_scope_label.c` | C99 参考：CTX/KIND/TYPEKW、scope_label（按上下文规则，默认 lookup）；table/call 两个命令 |
| `tests/unisagoldscopelabelcheck.sh` | 夹具：python 以 ast 抽取四个定义执行；30 行全表 + 17 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-scope-label` 的 req 行与 job 行 |
| `research/c41-k6-25-gold-scope-label.md` / `research/c41-k6-25-gold-scope-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `seed/unisa_gold_scope_label.c` = `1e991bdb8611a02984296d7a5a2ba32f566e310ec12b90c84fbda6146d7fa26d`
- `tests/unisagoldscopelabelcheck.sh` = `e1be7063f978b9e1a7fa0210a3db0415568c8b100390806e9e39d095ea4de72e`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldscopelabelcheck.sh`；UTC 21:48:12Z–21:48:13Z；rc=0；`unisagoldscope same 18 diff 0 neg 2/2`；日志 `research/c41-k6-25-gold-scope-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-scope-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..24 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 front.lex、emit_x86、17b、assemble、parse；未触 CX 主叶；未预填 PASS；未跑旧全量。
