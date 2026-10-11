# c41 K6-28：unisa.gold irsel_label → C 参考对拍（0702 授；0704 同窗续跑；cc 实现+证据）

## claim / 预算
- claim = **K6-28**（`irsel_label` 的 **IRSEL_MAP 映射表**全表 + 命名点）；授权依据 `ruling-k6-28-gold-irsel-auth-0702.md`；同窗修正依据 `ruling-k6-28-first-red-stdlib-allow-0704.md`；K6-27 已由 `cdx-k6-27-ruling-0700` 限定 PASS。
- 基：开工时 HEAD = `8e1b8482`（main）。
- 墙钟：自 0702 授刀起计，首跑 ~23:03Z，同窗续跑 23:04:49Z–23:05:03Z，在 30 min 内完成。

## 首红与停刀（如实记录）
- **首红（编译失败，具名）**：首次 cc 编译 `seed/unisa_gold_irsel_label.c` 失败——第 101 行 `exit(1)` 使用时未包含 `<stdlib.h>`，gcc：`error: implicit declaration of function ‘exit’`，rc=1。
- **停刀**：按 0702 首红停条款立即停；未运行夹具、未对拍、未提交（落盘记录 `cc-k6-28-irsel-first-red-0702.md`）。
- **同窗续跑**：0704 代裁允许以单行 `#include <stdlib.h>` 修正后同窗重跑，不另起刀、不扩域。已仅补该行；IRSEL_MAP 语义、表域与对拍设计未改。修正后编译无告警，首跑 20 点全绿。
- 本页记录的是"首次编译红→停→授权同窗修正→续跑全绿"的完整序列，不把首红抹为无。

## 范围
- `irsel_label(fam, flav) = IRSEL_MAP[fam].get(flav, "bad")`：未知 family 为函数级拒绝（python KeyError）；已知 family 下未知 flavour（含属于其它 family 的 flavour）返回 `bad`。
- 映射全表：`IRSEL_MAP` 共 **70 条**（alu 24、mem 6、ctrl 3、call 5、lit 4、fpu 28），按 gold.py 插入序逐条对拍；**非笛卡尔积**。C 端的 70 行由 python 以 ast 从 gold.py 的字面量生成并嵌入源码。
- 命名与边界点 19 个：alu 的 add/gt/ge/neg/udiv/lshr；mem 的 load/zero；ctrl 的 jumpz；call 的 push/callr；lit 的 print/exit；fpu 的 dgt/sge/ssqrt；跨 family 的 `alu dadd`、`mem dadd`（均为 bad）；域外 flavour `alu nosuch`（bad）。
- **不覆盖**：parse（串行队列后续叶，本拍不并刀）；`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `IRSEL_MAP` 与 `irsel_label` 两个定义，并断言抽取集合恰为此二者。

## 结果（直跑原始输出）
- `SAME 20 of 20 points`：全表 1 点（70 行）+ 19 个命名与边界点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`alu gt → slt64_rev`、`alu neg → sub64`、`mem load → load64`、`call push → callpush`、`lit print → print`、`fpu sge → fle32_rev`、`alu dadd → bad`（跨 family）、`alu nosuch → bad`。

## 负例（3 个，两侧同拒，stdout 空）
- `unknown-family`（`call nosuch add`）：**函数级**拒绝——python `IRSEL_MAP["nosuch"]` KeyError；C 拒绝（rc 1）。这是本叶的函数语义负例。
- `missing-flavour`（`call alu`）：包装层缺参（python rc 1；C 用法错误 rc 2）。
- `unknown-mode`（`build`）：包装层（两侧拒绝）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_irsel_label.c` | C99 参考：IRSEL 70 行（由 gold.py AST 生成）、FAMILIES、irsel_label；table/call 两个命令；**含同窗补的 `<stdlib.h>`** |
| `tests/unisagoldirsellabelcheck.sh` | 夹具：python 以 ast 抽取 IRSEL_MAP 与 irsel_label 执行；70 行全表 + 19 点字节对拍；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-irsel-label` 的 req 行与 job 行 |
| `research/c41-k6-28-gold-irsel-label.md` / `research/c41-k6-28-gold-irsel-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `seed/unisa_gold_irsel_label.c` = `0de8fc2f05ef1bdb368477344c4796ba4bbefcb80d4e12c0dc9cc72862ab6fd6`（含同窗补的 `<stdlib.h>`）
- `tests/unisagoldirsellabelcheck.sh` = `0ef4432a89d563b4ca6c74043d59fe534b792c0bbfba79572c71054c8daf6fce`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldirsellabelcheck.sh`（同窗续跑）；UTC 23:05:02Z–23:05:03Z；rc=0；`unisagoldirsel same 20 diff 0 neg 3/3`；日志 `research/c41-k6-28-gold-irsel-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-irsel-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..27 的 seed/夹具闭包；未 regen gold 表、未写 weights；未并刀 parse；未触 CX 主叶；未预填 PASS；未跑旧全量。
