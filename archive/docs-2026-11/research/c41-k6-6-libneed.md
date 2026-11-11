# c41 K6-6：unisa.libneed → C 参考对拍（2036 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-6**；基 HEAD = `708a717c3845327cf101aeaff56bf2b2c97cc9a4`；version.h 0.0.40；Latest v0.0.40（未动）。
- 预算墙钟 ≤30 min；首红停；本刀内首红 0 次。
- 范围：PREFIX、ROOTS、MACRO_NAME_LIMIT、guard(name)、bodies(text)（合成源）、table/roots（scratch 微 incdir；不扫仓内 include/）。**不含** apply_guards / check_guards（写 include/ 的路径未做；另列 K6-6b 若需）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_libneed.c` | C99 参考：consts / guard / bodies / table（递归扫描 *.h，DFS 闭包，宏与函数体图，ROOTS 闭包）；拒绝：重复携带体、过长需求名、未闭合体 |
| `tests/unisalibneedcheck.sh` | 夹具：常量、guard 两例、合成源 bodies、scratch 微 incdir 的 table 与子目录 table、3 负例两侧同拒且 stdout 空；C 双跑 |
| `tests/gate.sh` | 仅加 `unisa-libneed` 的 req 行与 job 行 |
| `research/c41-k6-6-libneed.md` / `-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/libneed.py` = `93285c30af38cc5fd05e7ad111fc8631a447472060832ea05fad1d02f82cb388`（只读）
- `seed/unisa_libneed.c` = `40d872a18ac6265903795f4840900a680db06573e6f9e2e0cb824324ac3bae2a`
- `tests/unisalibneedcheck.sh` = `ddd8fc604129128410ee232bd0b0b2c210871728f13d8658d90bdaebda2f5d98`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisalibneedcheck.sh`；UTC 12:38:44Z–12:38:45Z；rc=0；`unisalibneed same 6 diff 0 neg 3/3`。
- gate：`./tests/gate.sh --suite unisa-libneed`；UTC 2026-10-11T12:39:18Z–2026-10-11T12:39:19Z；rc=0；`failed 0 unverified 0`。
- 闭包非平凡性：scratch incdir 的 table 输出含 `KEY M fa fb fc`、`KEY exit exit fa fb fc`、`ROOTS_CLOSURE _unisa_environ_init exit fa fb fc`（与 Python 逐行相等）。
- 负例（两侧 rc≠0，stdout 空）：重复携带体 `fa`（两头文件）；过长需求名（60 字符）；未闭合体。

## 限定（不得外推）
- 点采样：仅上述夹具；不证整仓 include/ 穷举；不证 apply_guards/check_guards；不证 ckernel 与产品裁剪路径；不称产品闭包。
- C 的 bodies 扫描仅覆盖行首 `static` 的函数体（不含 __attribute__ 前缀形式、不含体内嵌套的行首 static）；夹具未覆盖的形式不作结论。
- `seed/unisa_libneed.c` 不进 release 候选身份；libneed.py 未删，产品路径未切换。

## 禁项自检
- 未改 src/version.h；未删 libneed.py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未手改仓内 include/；未改既有 K6-1..5 闭包；未开 K6-5b / 其他叶子；未跑旧全量。
