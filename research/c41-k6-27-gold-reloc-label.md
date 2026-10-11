# c41 K6-27：unisa.gold reloc_label → C 参考对拍（0631 授；cc 实现+证据）

## claim / 预算
- claim = **K6-27**（`reloc_label` 的**有限域** JMPKIND × ARCH 全表 + 命名点）；授权依据 `ruling-k6-27-gold-reloc-auth-0631.md`（串行 reloc→irsel→parse 的首叶，本拍只授本叶）；K6-26 已由 `cdx-k6-26-ruling-0610` 限定 PASS。
- 基：开工时 HEAD = `bd03986a`（main）。
- 墙钟：22:40Z 开工；直跑 22:40:24Z–22:40:25Z；在 30 min 内完成。
- 首红：**无**。C 编译一次通过（`-Wall -Wextra` 无告警）；夹具首跑即全绿。

## 范围
- `reloc_label(k, arch)`：x86_64 恒为 `rel32`；其它架构中 `jz` 为 `arm19`，`jmp` 与 `call` 为 `arm26`。函数自身无拒绝分支。
- 有限域：`JMPKIND`（3：jmp、jz、call）× `ARCH`（2：x86_64、arm64）= **6 行全表**，C 与 python 逐字节相等。`ARCH` 取自 `unisa/catalog.py`，`JMPKIND` 取自 `unisa/gold.py`。
- 命名点 6 个：三种跳转在 x86_64 上的 `rel32`，以及 arm64 上的分裂（`jz → arm19`，`jmp`、`call → arm26`）。
- **不覆盖**：irsel、parse（串行队列后续叶，本拍不并刀）；`build()`、regen、权重与训练；不称 gold 全闭包；不切换产品路径。

## python 侧的导入方式
- 不 import `gold.py` 或 `catalog.py`（模块级 `build()` 不被调用）。用 `ast` 抽取 `gold.py` 的 `JMPKIND`、`reloc_label`，以及 `catalog.py` 的 `ARCH`，并断言抽取集合恰为此三个名字。

## 结果（直跑原始输出）
- `SAME 7 of 7 points`：全表 1 点（6 行）+ 6 个命名点，C 与 python 逐字节相等；C 双跑确定。
- 代表值：`jz arm64 → arm19`、`jmp arm64 → arm26`、`call arm64 → arm26`、`jz x86_64 → rel32`。

## 负例（2 个，两侧同拒，stdout 空）
- `missing-arch`（`call jz`，缺 arch）：python 包装层缺参拒绝（rc 1）；C 用法错误（rc 2）。
- `unknown-mode`（`build`）：两侧拒绝（rc 1 / rc 2）。
- 说明：`reloc_label` 对域外值不抛错，不以域外值作函数级负例；两个负例属包装层，与 K6-20 至 K6-26 一致。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_gold_reloc_label.c` | C99 参考：JMPKIND、ARCH、reloc_label；table/call 两个命令 |
| `tests/unisagoldreloclabelcheck.sh` | 夹具：python 以 ast 抽取 gold.py 与 catalog.py 中的定义执行；6 行全表 + 6 点字节对拍；C 双跑；2 负例 |
| `tests/gate.sh` | 仅加 `unisa-gold-reloc-label` 的 req 行与 job 行 |
| `research/c41-k6-27-gold-reloc-label.md` / `research/c41-k6-27-gold-reloc-label-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/gold.py` = `25721a152d0d1e4ad7ebc7d314890223e312ef45bf40d97c497d63db021e7b91`（只读；未导入、未调用 build()）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_gold_reloc_label.c` = `737edda01c6ad08f1c53ffa6cffdab8a86f79ac3a21c1050eaca52f5082baf40`
- `tests/unisagoldreloclabelcheck.sh` = `d22cf6aca427880c85075e9895742a4c710a3c34a0bab65242093ad04981f3f5`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisagoldreloclabelcheck.sh`；UTC 22:40:24Z–22:40:25Z；rc=0；`unisagoldreloc same 7 diff 0 neg 2/2`；日志 `research/c41-k6-27-gold-reloc-label-run.log`。
- gate：`./tests/gate.sh --suite unisa-gold-reloc-label`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..26 的 seed/夹具闭包；未 regen gold 表、未写 weights；未并刀 irsel、parse；未触 CX 主叶；未预填 PASS；未跑旧全量。
