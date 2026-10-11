# c41 K6-5：unisa.linalg → C 参考对拍（2023 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-5**；基 HEAD = `68341b2eeb1e9e8fbf05b3ef75de1ed7739d1e42`；version.h 0.0.40；Latest v0.0.40（未动）。
- 预算墙钟 ≤30 min；首红停；本刀内首红 0 次。
- 范围：gemv（i 升序累加，不重排、不 FMA）、relu、argmax（并列取最低索引）、margin；**不含** gemv_bwd / relu_bwd / softmax_ce / Adam（未授，未做）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_linalg.c` | C99 参考，`-ffp-contract=off` 构建；输出浮点用 `%.17g` |
| `tests/unisalinalgcheck.sh` | 夹具：3 个脚本（A gemv+relu+argmax+margin；B 多位小数 gemv 与并列 argmax；C 边界 nin=0、nout=1、空 relu、并列负数、margin 并列）C/py 逐行 cmp → C 双跑 → 2 负例两侧同拒且 stdout 空 |
| `tests/gate.sh` | 仅加 `unisa-linalg` 的 req 行与 job 行 |
| `research/c41-k6-5-linalg.md` / `-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/linalg.py` = `1194ddb76531a2af556cdce02fef32871e0575afd4970a621c7daf2c88b39d74`（只读）
- `seed/unisa_linalg.c` = `11bc6fd0625e130f84f28e2b9e92a91c9c98445d933d8021f5352ea3dd96120d`
- `tests/unisalinalgcheck.sh` = `f66a8487fbc7d0e893a31a92b3a12db9b0f6472fee3eff2599af4ba369f04743`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisalinalgcheck.sh`；UTC 12:25:11Z–12:25:11Z；rc=0；`unisalinalg same 3 diff 0 neg 2/2`。
- gate：`./tests/gate.sh --suite unisa-linalg`；UTC 2026-10-11T12:25:32Z–2026-10-11T12:25:33Z；rc=0；`failed 0 unverified 0`。
- 负例（两侧 rc≠0，stdout 空）：`argmax 0`（python 由 `argmax` 自身 `v[0]` 抛 IndexError）；`gemv 3 2 1 2`（python 由 gemv 自身 `x[i]` 抛 IndexError）。

## 限定（不得外推）
- 点采样：仅脚本 A–C 的输入；不证任意矩阵穷举、不证 gemv_bwd/softmax_ce/Adam、不证训练路径；不证产品闭包。
- 浮点比较为 `%.17g` 文本逐字节相等；仅对本夹具的有限值；未测 inf/nan。
- `seed/unisa_linalg.c` 不进 release 候选身份；linalg.py 未删，产品路径未切换。

## 禁项自检
- 未改 src/version.h；未删 linalg.py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改既有 K6-1..4c 闭包文件；未开其他叶子；未跑旧全量。
