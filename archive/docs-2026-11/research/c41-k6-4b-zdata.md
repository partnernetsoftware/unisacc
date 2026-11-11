# c41 K6-4b：unisa.padded.ZData → C 参考对拍（1948 代裁授；cc 实现+证据）

## claim / schema（先于跑）
- claim = **K6-4b**；基 HEAD = `c70c8c966f837afe456f50b7a33c74ed95a9039e`；version.h = 0.0.40；Latest = v0.0.40（未动）。
- 预算：墙钟 ≤30 min；单步 bound ≤ 60 s；首红停（编译/DIFF/NEG 一侧不拒/越域）。
- 不同于 K6-4：本刀**只**覆盖 ZData（段式数据类）；未碰 `seed/unisa_padded.c` / `tests/unisapaddedcheck.sh`。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_zdata.c` | C99 参考：按脚本解释 ZData 段语义（extend/zeros/extzero/set/setlen/read/count/len/dump/copy；负例 stepread/countx/setlen 按名拒绝） |
| `tests/unisazdatacheck.sh` | 夹具：4 个脚本（A/B/C/D）C 与 Python 逐行 cmp → C 双跑 → 3 负例两侧同拒 → 身份打印；首红即停 |
| `tests/gate.sh` | 仅加 `unisa-zdata` 的 req 行与 job 行 |
| `research/c41-k6-4b-zdata.md` | 本页 |
| `research/c41-k6-4b-zdata-run.log` | 直跑原日志 |

## 源件 sha256
- `unisa/padded.py` = `7f8968fd69ed5c0bd6d3af0d653f0936ed36f484a0689a94081800c24183758f`（只读）
- `seed/unisa_zdata.c` = `2a86a64709a7b14260a72ec31bdd4434196cbbe26245e87148b049127327458b`
- `tests/unisazdatacheck.sh` = `b269507f3fcb086d1bb561759c76231c7fc620f0bbc419212863bd903954cf92`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisazdatacheck.sh`；UTC 11:56:06Z–11:56:06Z；**rc=0**；`unisazdata same 4 diff 0 neg 3/3`。
- gate：`./tests/gate.sh --suite unisa-zdata`；UTC 2026-10-11T11:56:32Z–2026-10-11T11:56:44Z；rc=0；`failed 0 unverified 0`。
- 脚本覆盖（逐行与 Python 相等）：A（大零段合并、零段切片写拆段、跨段读、count、copy_into）；B（两段大零合并、零写不拆段、非零写拆段、count）；C（小零走 bytearray 数据段、大零段、拆段、copy 到偏移、读）；D（前后数据段夹零段、零写、copy）。
- 首红停：本刀内 0 次（首跑即绿）。
- 负例（两侧 rc≠0，无 stdout）：stepread step=2；countx x=1；setlen 长度不符。

## 限定（不得外推）
- 点采样：只覆盖上述 4 个脚本的操作序列；不证任意输入穷举，不证 ZData 全部方法语义，不证 copy_data 的非 ZData 分支。
- 不证 ape_stage / layout 全路径；不称产品闭包；`seed/unisa_zdata.c` 不进 release 候选身份。
- 切片写 / copy_into 的越界与 mem 扩容行为未测（Python 侧 copy 用足够大的 mem）。

## 禁项自检
- 未改 src/version.h；未删 `padded.py`；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-4 闭包文件；未跑旧全量。
