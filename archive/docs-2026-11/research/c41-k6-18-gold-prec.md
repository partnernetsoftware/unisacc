# c41 K6-18：unisa.gold prec_label → C 参考对拍（0239 授；cc 实现+证据）

## claim / 预算
- claim = **K6-18**（`prec_label` 有限域）；代裁依据 `ruling-k6-18-gold-prec-auth-0239.md`；K6-17 已由 `cdx-k6-17-ruling-0223.md` 限定 PASS（窄修，0226 闩）。
- 基：开工时 HEAD = `fb5733a5`（main）。
- 墙钟：0239 代裁后 18:41Z 开工；直跑 18:42:48Z–18:42:49Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra`），唯一告警（未用函数 `die`）在提交前删除。python 首跑即全绿。

## 范围（窄切，按代裁）
- `prec_label(op)`：`str(_PREC[op])` 若 op 在 _PREC，否则 `"none"`；`PREC_OPS` 为 19 个域元素（含 `other`），`PREC_LEV` 为 `none`、1..10。
- 覆盖：`PREC_OPS` 全枚举（19 项）；命名算符点 9 个（`*`、`/`、`%`、`+`、`-`、`||`、`&&`、`==`、`other`）；边界字符串 5 个（空串、`**`、`<<=`、尾随空格的 `|| `、近似的 `o`），均应为 `none`。
- **不覆盖**：gold 的 Stage 表、全部标签函数（type/parse/lex/peep/irsel/opinfo/binsel）、`build()`、表的 regen、任何权重写出或训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式（重要）
- `unisa/gold.py` 在模块级执行 `STAGES = build()`。按代裁不调用 `build()`，因此 python 夹具**不 import gold.py**：用 `ast` 读取源文件，仅抽取 `PREC_OPS`、`PREC_LEV`、`_PREC`、`prec_label` 四个定义，编译并在私有命名空间中执行，并断言抽取到的名字集合恰为这四个。
- 因此本叶的 python 基准是 gold.py 源码中这四个定义的语义，而非经由模块导入的对象。

## 结果（直跑原始输出）
- 全表（19 行，`OP<TAB>LABEL`）：`||`→1，`&&`→2，`|`→3，`^`→4，`&`→5，`==`→6，`!=`→6，`<`→7，`>`→7，`<=`→7，`>=`→7，`<<`→8，`>>`→8，`+`→9，`-`→9，`*`→10，`/`→10，`%`→10，`other`→none。
- 点：`* → 10`、`other → none`、边界字符串均 `none`。
- 15 点（表 1 + 命名与边界 14）C 与 python stdout 逐字节相等；C 双跑确定（15 点）。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-operator`（`op` 缺参数）：C 用法错误 rc 2；python 抽取后按调用缺参 rc 1（非 0 且 < 128）。
- `unknown-mode`（`build`）：C 用法错误 rc 2；python 拒绝未知模式。

## 边界（不称）
- 点采样：有限域全表与 14 个命名/边界点；不证 gold 的其它标签与 Stage 表；不证 build()；不证权重或训练；不称产品闭包。
- python 基准取自源码定义（经 ast 抽取执行），不依赖模块导入。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_prec.c` | C99 参考：PREC_OPS（19）、_PREC 表、prec_label、table/op 两个命令 |
| `tests/unisagoldpreccheck.sh` | 夹具：python 以 ast 抽取 prec 定义执行；15 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-prec` 的 req 行与 job 行 |
| `research/c41-k6-18-gold-prec.md` / `research/c41-k6-18-gold-prec-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_gold_prec.c` = `340ba2c39eef5469ee994eb737054157601f4f262e5605b71d595709ca5b19d6`
- `tests/unisagoldpreccheck.sh` = `7ce4baba0d2770f84634c9937f62b338e1b747fc34e54c02200b6d15283cf9f6`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldpreccheck.sh`；UTC 18:42:48Z–18:42:49Z；rc=0；`unisagoldprec same 15 diff 0 neg 2/2`；日志 `research/c41-k6-18-gold-prec-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-prec`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..17 的 seed/夹具闭包；未 regen gold 表、未写 weights；未开 17b、emit、front、ir、opt；未预填 PASS；未跑旧全量。
