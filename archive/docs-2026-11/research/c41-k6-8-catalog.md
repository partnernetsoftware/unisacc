# c41 K6-8：unisa.catalog → C 参考对拍（2120 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-8**；基 HEAD = `4769b4123de7aa5a6a3edf83ffc2dd8b9d929056`；version.h 0.0.40；Latest v0.0.40（未动）。
- 预算墙钟 ≤30 min；首红停；本刀内首红 0 次（首跑即绿）。
- 范围：OS/ARCH/TARGETS 常量；sysno（3 个 op × 2 个以上 target，含 osx 的 None 情形）；enc_form / symbol / gate 各 ≥2 点；argregs、nrreg、argshape、retconv、winimp 随同一行输出。
- **不含**：整个 SYSCALLS/OPS/MNEMONIC 表与九头（nine）的穷举（K6-8b）；C 参考仅内置采样 op 的表项。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_catalog.c` | C99 参考：按 (op, os, arch) 打印一行派生事实（form/symbol/gate/sysno/nrreg/arg0-5/ret/argshape/retconv/winimp）；未知 op 按名拒绝 |
| `tests/unisacatalogcheck.sh` | 夹具：常量比对（OS/ARCH/TARGETS）、15 个采样点逐行 cmp、C 双跑、2 负例两侧同拒且 stdout 空 |
| `tests/gate.sh` | 仅加 `unisa-catalog` 的 req 行与 job 行 |
| `research/c41-k6-8-catalog.md` / `-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_catalog.c` = `b60e524e12a9ccb1d8a25d8189e76e72089ec8a83c08cd48ae4e602e2a935570`
- `tests/unisacatalogcheck.sh` = `171c4863e5edc02f93ef25dfe4662c151ef77397dc9e9e3970e836470881505b`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisacatalogcheck.sh`；UTC 13:24:29Z–13:24:30Z；rc=0；`unisacatalog same 15 diff 0 neg 2/2`。
- gate：`./tests/gate.sh --suite unisa-catalog`；UTC 2026-10-11T13:24:46Z–2026-10-11T13:24:47Z；rc=0；`failed 0 unverified 0`。
- 采样（python 与 C 逐行相等）：exit lnx/x86_64、exit osx/arm64（0x2000001 / x16）、mmap win/arm64、open lnx/arm64（56 / x8 / svc0）、clock_gettime osx/x86_64（none）、getdents64 osx/arm64（none）、add64 lnx/x86_64（MOP）、syscall_gate win/arm64、fsqrt64 osx/arm64、unlink win/x86_64（bool_neg, DeleteFileA）、rename lnx/arm64（atfd_2_zero5）、write win/arm64（wcount）、ppoll lnx/arm64（zero4）、lseek win/arm64（dword_sx）。
- 负例（两侧 rc≠0，stdout 空）：未知 op `nosuchop` 于 lnx/x86_64 与 osx/arm64（python 由 SYSCALLS 的 KeyError 拒绝）。

## 限定（不得外推）
- 点采样：仅上述 op/target 点；不证整表、九头穷举、argregs 全 op、WinAPI 全表、enc 字节；不称产品闭包。
- 负例的"python 拒绝"为 nine()/SYSCALLS 的 KeyError（genuine，非包装 assert）。
- 注意：python 对未知 OS 不拒绝（sysno 按 osx 分支处理），故未把未知 OS 列为负例。
- `seed/unisa_catalog.c` 不进 release 候选身份；catalog.py 未删，产品路径未切换。

## 禁项自检
- 未改 src/version.h；未删 catalog.py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改既有 K6-1..7 闭包；未开 K6-8b / K6-7b / K6-6b / 其他叶子；未跑旧全量；未整表穷举。

## 窄修（2137 代裁；补证 cdx FAIL@2135）
- 根因①：consts 只比对 Python 字面，无 C 可观测对拍 → 修：C 增加 `consts` 命令（OS / ARCH / TARGETS 三行），夹具改为 C 输出与 Python 导出逐行 cmp（不再用字面文件）。
- 根因②：sysno 仅 exit 跨两 target → 修：新增 mmap（lnx/x86_64、osx/arm64）、open（osx/x86_64，并已有 lnx/arm64）、close（lnx/x86_64、osx/arm64）。覆盖 exit、mmap、open、close 四个 op，各 ≥2 target（sysno 为数值或 0x2000000|n）。
- 直跑（窄修后）：UTC 13:38:48Z–13:38:49Z，rc=0，`unisacatalog same 20 diff 0 neg 2/2`；gate：UTC 2026-10-11T13:38:58Z–2026-10-11T13:38:59Z，rc=0，failed 0 unverified 0。
- 窄修后源件 sha256：`seed/unisa_catalog.c` = `16f82c22ae914100fc19617bf705afe44ae560418bda2e8d56939ce4cd1a6921`；`tests/unisacatalogcheck.sh` = `1e953accd979aee14301b2c607e909dbfd6a7abb7f6bd1d48d3658b88bad0658`；`unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读，未变）。基 HEAD `bdec4bcbf823d0b527a15c40afe61f69942063fc`。
- 口径仍为点采样；不含 K6-8b 整表穷举；不称产品闭包。
