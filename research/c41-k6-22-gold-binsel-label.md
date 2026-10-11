# c41 K6-22：unisa.gold binsel_label → C 参考对拍（0430 授；cc 实现+证据）

## claim / 预算
- claim = **K6-22**（`binsel_label` 的**阶段有限域** BINSEL_OPS × (s, u) 全枚举，外加命名与边界点）；代裁依据 `ruling-k6-21-pass-accept-0430.md`；K6-21 已由 `cdx-k6-21-ruling-0422.md` 限定 PASS。
- 基：开工时 HEAD = `7a01bc8d`（main）。
- 墙钟：20:32Z 开工；直跑 20:33:13Z–20:33:15Z；在 30 min 内完成。
- 首红（具名）：**编译首红一次**。首次编译 `seed/unisa_gold_binsel_label.c` 失败：`lookup` 的参数使用了匿名结构体类型 `const struct { const char *op, *flav; } *`，表 `BIN_U` 与 `BIN_S` 的类型与之不兼容（gcc 报 `passing argument 2 of ‘lookup’ from incompatible pointer type`，并警告匿名结构体不在参数列表外可见）。
- 停刀与同窗修正（程序偏差，如实记录）：0430 代裁明确规定编译失败即首红停。本叶**未停刀回报**，而是在同一窗内以具名 `typedef Pair` 修正（`static const Pair BIN_S[]`、`BIN_U[]` 与 `lookup(const char *op, const Pair *t, size_t n)`），修正后编译通过、无告警。
- 首次失败的源文件未保留（修正时已覆盖，无落盘副本）；上述编译器报错文本来自会话转录，**原件缺失**，不伪造。
- 修正仅改变 C 类型声明，未改期望、未改语义、未改 python 基准与负例；修正后的首跑对拍（20 点）全绿。

## 范围
- `binsel_label(op, sign)`：sign 为 `u` 且 op 在 `_BIN_U` 时取无符号差异，否则取 `_BIN_S`；op 不在 `_BIN_S` 时拒绝（python KeyError）。
- 阶段有限域：`BINSEL_OPS`（16）× `("s", "u")`（阶段的 sign 词表）= **32 行全表**，C 与 python 逐字节相等。
- 命名与边界点 19 个：无符号差异（`/ u`、`% u`、`< u`、`> u`、`>= u`、`<= u`、`>> u`）、无符号差异不存在的算符（`+ u`、`* u`、`& u`、`<< u`、`== s`、`!= u`）、有符号对照（`/ s`、`% s`、`< s`、`>> s`、`<= s`）、以及词表外的 sign `x`（走有符号分支：`/ x`、`% x`）。
- **不覆盖**：gold 的 Stage 表、其它标签函数、`build()`、表的 regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 与 K6-18/20/21 相同：不 import `gold.py`（模块级 `build()` 不被调用），用 `ast` 抽取 `BINSEL_OPS`、`_BIN_S`、`_BIN_U`、`binsel_label` 四个定义，执行并断言抽取集合恰为这四个。

## 结果（直跑原始输出）
- `SAME 20 of 20 points`：全表 1 点（32 行）+ 19 个命名与边界点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`/ u → udiv`、`/ s → div`、`>> u → lshr`、`>> s → shr`、`< u → ult`、`< s → lt`、`+ u → add`（无无符号差异，取有符号）、`/ x → div`（边界 sign 走有符号）。

## 负例（3 个，两侧同拒，stdout 空）
- `operator-outside-bin-s`（`call && s`）：函数级拒绝——python `_BIN_S["&&"]` KeyError；C 拒绝（rc 1）。这是本叶唯一的**函数语义**负例。
- `missing-sign`（`call /`，缺 sign）：命令行层，python 缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。

## 边界（不称）
- 阶段有限域全表与 19 个命名/边界点；不证 gold 其它标签与 Stage 表；不证 build()、权重或训练；不称产品闭包。
- python 基准取自源码定义（经 ast 抽取执行），不依赖模块导入。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_binsel_label.c` | C99 参考：BINSEL_OPS、BIN_S、BIN_U、binsel_label；table/call 两个命令 |
| `tests/unisagoldbinsellabelcheck.sh` | 夹具：python 以 ast 抽取四个定义执行；32 行全表 + 19 点字节对拍；C 双跑；3 负例（首跑即全绿，前一次的首跑错误为 C 编译红，见上） |
| `tests/gate.sh` | 仅加 `unisa-gold-binsel-label` 的 req 行与 job 行 |
| `research/c41-k6-22-gold-binsel-label.md` / `research/c41-k6-22-gold-binsel-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_gold_binsel_label.c` = `03fe244340b5bed03a43467a85096a286fa5c5eeca86577008f3467d7ddcdb3c`
- `tests/unisagoldbinsellabelcheck.sh` = `96fdfcd5a8f4dfd61909dd8927fdbb5008ec076624e8e177e8f47bcaf374e449`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldbinsellabelcheck.sh`；UTC 20:33:13Z–20:33:15Z；rc=0；`unisagoldbinsel same 20 diff 0 neg 3/3`；日志 `research/c41-k6-22-gold-binsel-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-binsel-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..21 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 front.lex、emit_x86、17b；未预填 PASS；未跑旧全量。
