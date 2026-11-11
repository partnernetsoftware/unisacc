# c41 K6-2：unisa.hostabi → C 参考对拍（1857 代裁授；cc 实现+证据）

## 范围与口径
- 目的：`unisa/hostabi.py` 的导出常量（ARM_FN、ARM_ARGV、ARM_BODY、X86_BODY、WIN_ARM_BODY、WIN_X86_BODY）由独立 C 实现从同一 TSV 解析并逐字节对拍。
- **点采样等价**：仅对 `exec/enc/hostbridge.tsv` 这一张表；不称产品闭包；`seed/unisa_hostabi.c` 不进 release 候选身份。
- **不做**：删 `hostabi.py`；改 `exec/` 生成路径；改 src/version.h；bump；Draft；Latest；签名；改验收。
- 只读：`unisa/hostabi.py`、`exec/enc/hostbridge.tsv`。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_hostabi.c` | C99 参考：读 TSV、导出上述常量（`NAME COUNT HEX`）；缺行/坏 hex/奇数字节即按名拒绝，**拒绝时 stdout 为空**（先缓冲后输出） |
| `tests/unisahostabicheck.sh` | 夹具：编译 → C 双跑确定性 → 与 Python 导出逐行 cmp → 3 条负例两侧同拒 → 身份打印；首红即 exit≠0 |
| `tests/gate.sh` | 仅加 `unisa-hostabi` 的 req 行与 job 行 |
| `research/c41-k6-2-hostabi.md` | 本页 |
| `research/c41-k6-2-hostabi-run.log` | 本刀直跑原日志（夹具输出） |

## 源三件 sha256（写本页时）
- `unisa/hostabi.py` = `081e3599afca848141f71bcad36bee82a2d55214f413659a17c2e174ad127602`
- `exec/enc/hostbridge.tsv` = `6d7b63684fa95cee2efc1aebdc796781565545d1809c09877d63945bf790dcf0`
- `seed/unisa_hostabi.c` = `ab6a78a39d736e6f26176754f03a4b573411dae5932dc97ee3ec346b190a4ca0`
- `tests/unisahostabicheck.sh` = `3d4c9c791761ced15de83efd4bdfb4a58d59a006bd57bcaaebdd5f5cd3981bb3`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`
- 基：HEAD `2bc1d4688b336b0cfca39e3351c106c22060c0fc`

## 原证（直跑 + gate）
- 直跑：`sh tests/unisahostabicheck.sh`；UTC `10:59:31Z`–`10:59:31Z`；**rc=0**；`unisahostabi same 6 constants-lines diff 0 neg 3/3`。
- gate：`./tests/gate.sh --suite unisa-hostabi`；UTC `10:59:38Z`–`10:59:38Z`；**rc=0**；`unisa-hostabi rc=0 0s :: same 6 constants-lines diff 0 neg 3/3`；`failed 0 unverified 0`。
- 逐项：C 双跑 SAME；C vs Python SAME（6 行常量）；NEG-missing-arm-fn 两侧拒（c_rc=1 py_rc=1）；NEG-bad-hex-fn 两侧拒（c_rc=1 py_rc=1）；NEG-odd-x86-body 两侧拒（c_rc=1 py_rc=1）。
- C 导出的 dump 摘要 sha256：`d8dd4b24…76208`。

## 首红停（本刀内发生一次，已修）
- 首次直跑：NEG-odd-x86-body 报 `C wrote stdout: first red`——C 在拒绝前已打印前几行常量。
- 修复：C 改为先将全部行写入缓冲区，全部校验通过后才输出；拒绝时 stdout 为空。修复后三条负例均通过。
- 这不是放宽：修复使“拒绝即无输出”成立；判定标准未降低。

## 常量表（点采样；与 Python 同值）
- ARM_FN = `aa0003f0`；ARM_ARGV = `aa0003f1`（各 1 字）
- ARM_BODY：TSV 中 arm64 body 行（按出现顺序）
- X86_BODY：TSV 中首条 x86_64 body（字节）
- WIN_ARM_BODY：win_arm64 body 行（按出现顺序）
- WIN_X86_BODY：首条 win_x86_64 body（字节）

## 限定（不得外推）
- 只证本 TSV 的常量导出一致与负例同拒；不证 hostabi 的宿主 ABI 行为，不证产品 ELF/Mach-O/PE 的调用正确性。
- Python 侧仍为产品路径；本刀不删也不切换。
- TSV 解析的 C 版本仅覆盖：`#` 注释行、三列制表符行、arch/part 首匹配（fn/argv/x86 body）与全量匹配（arm body）。其它 TSV 形态未测。

## 预算
- 墙钟：约 10:58–11:00 UTC（远低于 30 min 上限）。首红停 1 次（已修）。

## 禁项自检
- 未改 src/version.h；未删 `hostabi.py`；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec 生成路径；未跑旧全量。
