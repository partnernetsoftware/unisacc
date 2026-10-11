# c41 K6-3：unisa.peversion → C 参考对拍（1905 代裁授；cc 实现+证据）

## 范围与口径
- 目的：`unisa/peversion.py` 的 `add_version_info()`（unsigned PE32+ 头加 `.rsrc`/RT_VERSION）由独立 C 实现，在采样的 (PE 头, 产品名, 版本) 点上输出逐字节相等；负例两侧同拒且不写输出。
- **点采样等价**：4 个成功点 + 5 个负例；不称产品闭包；`seed/unisa_peversion.c` 不进 release 候选身份；`peversion.py` 未删、产品路径未切换。
- 只读对照：`tests/apeversioncheck.py`（未改语义）。
- **域限定**：产品名仅可打印 ASCII（C 拒绝 ASCII 外字符；Python 理论上接受，本刀不测该域）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_peversion.c` | C99 参考：读 PE 头、校验（与 Python ValueError 集同构）、构建 VERSIONINFO 树、先在内存组装、通过后才写 OUT；拒绝即不写输出 |
| `tests/unisapeversioncheck.sh` | 夹具（生成 unsigned PE32+ 基础头）→ C 与 Python 同点逐字节 cmp → C 双跑确定性 → 5 条负例两侧同拒 → 身份打印；首红即 exit≠0 |
| `tests/gate.sh` | 仅加 `unisa-peversion` 的 req 行与 job 行 |
| `research/c41-k6-3-peversion.md` | 本页 |
| `research/c41-k6-3-peversion-run.log` | 直跑原日志 |

## 源件 sha256（写本页时）
- `unisa/peversion.py` = `9f2e661f7f797833bd31a0a4c19c9692498a33e9f9b0b641e303f89a63db92d6`
- `seed/unisa_peversion.c` = `79255335bf68f6f502819313d0bf4a7ec8e323bf5af79a386c6c508b5322027f`
- `tests/unisapeversioncheck.sh` = `2003fc5ad1a71079e3bb77df3cf3b334021502e2d92eef3168a8e1679125e22d`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`
- 基 HEAD：`cfabc81bde2d16b10382a2ad29e7baefe014f3a3`（K6-2 `5ca3c09f` 之后；cdx 真审中）

## 原证（直跑 + gate）
- 直跑：`sh tests/unisapeversioncheck.sh`；UTC `11:09:48Z`–`11:09:48Z`；**rc=0**；`unisapeversion same 4 diff 0 neg 5/5`。
- gate：`./tests/gate.sh --suite unisa-peversion`；UTC `11:09:55Z`–`11:09:56Z`；**rc=0**；`unisa-peversion rc=0 1s :: same 4 diff 0 neg 5/5`；`failed 0 unverified 0`。
- 成功点（同字节）：base+`unisacc`+`1.2.3`（1536 B）；base+`unisacc`+`1.2.3.4`（1536 B）；base+`Unisacc Test App`+`65535.65535.65535.65535`（2048 B）；base+`Unisacc Test App`+`0.0.1`（2048 B）。
- 负例（两侧 rc≠0，无输出）：overlay（末尾多 1 字节）；PE32（魔数 0x10B）；已有 `.rsrc` 目录；版本 `1.2`（只两段）；产品名含控制字符。
- 首红停：本刀内 0 次红（首跑即绿）。

## 限定（不得外推）
- 只证采样点的输出一致与负例同拒；不证任意 PE 头、不证 APE/Mach-O/PE 产品的签名或加载行为。
- 夹具头为最小合成 PE32+（单节 `.text`，零 slack），不是真实编译器产物。
- 非 ASCII 产品名不在域内。

## 禁项自检
- 未改 src/version.h；未删 `peversion.py`；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec 生成路径；未跑旧全量。
