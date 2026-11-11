# c41 K6-21：unisa.gold type_label → C 参考对拍（0359 授；cc 实现+证据）

## claim / 预算
- claim = **K6-21**（`type_label` 的**全有限域**枚举，外加命名点）；代裁依据 `ruling-k6-20-pass-accept-0359.md`（授 K6-21）；K6-20 已由 `cdx-k6-20-ruling-0357.md` 限定 PASS，接纳见同一份 0359 裁定。
- 基：开工时 HEAD = `97dfabbe`（main）。
- 墙钟：20:00Z 开工；直跑 20:01:33Z–20:01:35Z（修正后重跑 20:01:44Z）；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra`，无告警）。夹具首跑 rc=2 的一次失败是**夹具自身**的 shell 通配展开问题（`*` 与 `[]` 被 glob 成仓内文件名），在提交前用 `set -f` 修正；未触及期望，未改 C 参考。

## 范围
- `type_label(t1, op, t2)`：完整分支序（sizeof、un*、call、[]、.、=、&、<</>>、|/^、逗号、比较、浮点算术、整数与指针算术）及其辅助 `_promote`、`_uns`、`_arith`、`_farith`、`RANK`、`NUM`、`FLT`。
- 覆盖：`TYS`（15）× `TOPS`（19）× `TYS`（15）= **4275 行全表**，C 与 python 逐字节相等；另 27 个命名点（整数与浮点算术、通常算术转换到无符号、指针加减、位运算与移位、比较得 i32、取址规则 `i64&i64→ptr`、逗号、以及边界名字 `bogus` 等落入 `illegal` 的情形）。
- 边界名字不是拒绝：`type_label` 对未知类型名或未知算符直接返回 `illegal`（函数自身的落空分支），C 参考同样返回 `illegal`。
- **不覆盖**：gold 的 Stage 表、其它标签函数、`build()`、表的 regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 与 K6-18/K6-20 相同：不 import `gold.py`（模块级 `build()` 不被调用），用 `ast` 抽取 `TYS`、`TOPS`、`NUM`、`FLT`、`RANK`、`_promote`、`_uns`、`_arith`、`_farith`、`type_label` 十个定义，执行并断言抽取集合恰为这十个名字。

## 结果（直跑原始输出）
- `SAME 28 of 28 points`：全表 1 点（4275 行）+ 27 个命名点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`i32 + i32 → i32`、`u8 + i8 → i32`（先提升为有符号 int）、`u32 + i32 → u32`、`i64 + u32 → i64`、`u64 + i64 → u64`、`f32 + i64 → f32`、`i32 < f64 → illegal`、`ptr - ptr → i64`、`ptr + i32 → ptr`、`i64 & i64 → ptr`、`i32 << u64 → i32`、`struct . i32 → i64`、`void = i32 → void`、`bogus + i32 → illegal`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-operand`（`call i32 +`，缺第三个操作数）：python 以命令行参数个数检查拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝。
- 说明：这两个负例属于**命令行/包装层**，与 K6-20 的 missing-peek 同类；函数本身对未知名字返回 `illegal` 而非抛错，因此不以它作为函数级负例。

## 边界（不称）
- 有限域全表与 27 个命名点的点采样；不证 gold 其它标签与 Stage 表；不证 build()、权重或训练；不称产品闭包。
- python 基准取自源码定义（经 ast 抽取执行），不依赖模块导入。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_type_label.c` | C99 参考：TYS/TOPS/NUM/FLT/RANK、_promote/_uns/_arith/_farith、type_label；table/call 两个命令 |
| `tests/unisagoldtypelabelcheck.sh` | 夹具：python 以 ast 抽取十个定义执行；4275 行全表 + 27 命名点字节对拍；C 双跑；2 负例（set -f 关闭通配展开） |
| `tests/gate.sh` | 仅加 `unisa-gold-type-label` 的 req 行与 job 行 |
| `research/c41-k6-21-gold-type-label.md` / `research/c41-k6-21-gold-type-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_gold_type_label.c` = `22a2a6fab7f928644893ac414a70934197ac3566c03cb6e764c3519338a3f46b`
- `tests/unisagoldtypelabelcheck.sh` = `2f4f68b97dbc85b7e12719bffff02042b4bf4b1c320937ccfef8143d4d8ebeca`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldtypelabelcheck.sh`；UTC 20:01:44Z 附近；rc=0；`unisagoldtype same 28 diff 0 neg 2/2`；日志 `research/c41-k6-21-gold-type-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-type-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..20 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 front.lex、emit_x86、17b；未预填 PASS；未跑旧全量。
