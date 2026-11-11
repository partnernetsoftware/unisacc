# c41 K6-4：unisa.padded → C 参考对拍（1930 代裁授；cc 实现+证据）

## 范围与口径
- 目的：`unisa/padded.py` 的 `Padded`（stored 前缀 + 逻辑长度）的**可观测值**由 C 参考在采样点上逐行相等：LEN、STORED、PREFIX、RSTRIP、SLICE、VIEW、ZERO_TAIL。
- 点采样等价；不称产品闭包；`seed/unisa_padded.c` 不进 release 候选身份；`padded.py` 未删、产品路径未切换。
- **不含** `ZData`（同文件的段式数据类）：超预算则拆 K6-4b，本刀未做。
- 只读：`unisa/padded.py`。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_padded.c` | C99 参考：HEX 前缀 + FULL 十进制，打印 7 行可观测值；FULL 取 max(FULL,stored)；不分配逻辑长度；非法输入按名拒绝且无输出 |
| `tests/unisapaddedcheck.sh` | 夹具：7 采样点逐行 cmp → C 双跑确定性 → 3 条负例两侧同拒 → 身份打印；首红即 exit≠0 |
| `tests/gate.sh` | 仅加 `unisa-padded` 的 req 行与 job 行 |
| `research/c41-k6-4-padded.md` | 本页 |
| `research/c41-k6-4-padded-run.log` | 直跑原日志 |

## 源件 sha256（写本页时）
- `unisa/padded.py` = `7f8968fd69ed5c0bd6d3af0d653f0936ed36f484a0689a94081800c24183758f`
- `seed/unisa_padded.c` = `20f2e5d4817bd6beedefdd6d695238924fae8568e960d028455391c6101d83ce`
- `tests/unisapaddedcheck.sh` = `3c6516be5765a8379486cbd59f4112cbe6492b3a78df542adf9fec2be3c319b5`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`；基 HEAD `24a9c6d52d3f102b07015e973df53261c9e86eb1`

## 原证（直跑 + gate）
- 直跑：`sh tests/unisapaddedcheck.sh`；UTC 11:31:36Z–11:31:37Z；**rc=0**；`unisapadded same 7 diff 0 neg 3/3`。
- gate：`./tests/gate.sh --suite unisa-padded`；UTC 2026-10-11T11:31:49Z–2026-10-11T11:32:00Z；rc=0；`failed 0 unverified 0`。
- 采样点（同 7 行）：hex=''/full=0；41/1；4100/10；00000000/4；deadbeef/3（full<stored）；ff00ff/-5（负 full 被 max 钳制）；00/2^40（不分配逻辑长度）。
- 负例（两侧 rc≠0，无输出）：FULL=x（非十进制）；前缀 abc（奇数长度）；前缀 zz（非 hex）。
- 首红停：本刀内 0 次（首跑即绿）。

## 限定（不得外推）
- 只证采样点的可观测值与负例同拒；不证 Padded 的 memoryview 写入、切片写入、拷贝语义全集；不证 ZData；不证产品。
- Python 侧的 `len(p[:])`、`memoryview` 长度等为 CPython 对 bytearray 子类的当前行为，本刀按该行为对拍。

## 禁项自检
- 未改 src/version.h；未删 padded.py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未跑旧全量。
