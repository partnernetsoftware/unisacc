# c41 K6-4c：ZData 漏证收口（1959 代裁授；cc 实现+证据）

## 申报的 4b 漏证 → 本刀处置
| 4b 漏证（cdx 限定 PASS 明示） | 处置 | 证据位置 |
|---|---|---|
| `copy_data` 未直接调用 | 新增脚本 E：`copy_data M BASE` 直调模块级 `unisa.padded.copy_data`（ZData 分支），C 与 py 逐行相等 | run.log 「SAME E (7 lines)」 |
| stepread/countx 仅包装器 assert | 负例改为触达 ZData **真实方法**：py 侧 `z[s:e:step]`（经 `_range` 断言 step==1）与 `z.count(x,s,e)`（经 count 断言 x==0）；C 侧经 `zd_range` / `zd_count`（与 ZData 同一拒绝条件） | run.log 两条 NEG both refuse；py stderr 为 ZData 真实方法的 assert |
| NEG 两侧 stdout 空未强制断言 | 夹具显式 `[ ! -s … ]` 断言 C 与 py 两侧 stdout 均空，失败即 fail | 脚本 neg() |
| C 双跑 | 保持（脚本 A–E 各双跑 cmp） | run.log |

## claim / 预算
- claim = **K6-4c**；基 HEAD = `252679f0b9aa7a9c9fef58dc1f6143413d1e82ed`；version.h 0.0.40；Latest v0.0.40 未动。
- 预算墙钟 ≤30 min；首红停；本刀内首红 0 次。

## 文件（本刀写）
- `seed/unisa_zdata.c`（新增 zd_range/zd_count/copy_data 分支；其余不变）
- `tests/unisazdatacheck.sh`（新增脚本 E、copy_data 直调、真实方法负例、stdout 空断言）
- `tests/gate.sh` 未改（unisa-zdata job/req 沿用 4b）
- `research/c41-k6-4c-zdata-gaps.md`、`research/c41-k6-4c-zdata-gaps-run.log`
- 未碰 `seed/unisa_padded.c`、`tests/unisapaddedcheck.sh`、`unisa/padded.py`（只读）。

## 源件 sha256
- `unisa/padded.py` = `7f8968fd69ed5c0bd6d3af0d653f0936ed36f484a0689a94081800c24183758f`（只读）
- `seed/unisa_zdata.c` = `c00bc69bda2b80b58460887de1696e9b34a5c580360d8d4f723fec7bb8f59988`
- `tests/unisazdatacheck.sh` = `6e7c08f9bea2ad49da104900c0e559c2fa3b2afeab0cd08ca9e0aab118afc045`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisazdatacheck.sh`；UTC 12:02:06Z–12:02:06Z；rc=0；`unisazdata same 5 diff 0 neg 3/3`。
- gate：`./tests/gate.sh --suite unisa-zdata`；UTC 2026-10-11T12:02:33Z–2026-10-11T12:02:34Z；rc=0；`failed 0 unverified 0`。

## 限定（不得外推）
- 点采样：仅脚本 A–E 的操作序列；不证任意输入穷举；不证越界写、mem 扩容；不证 ape_stage/layout 全路径；不称产品闭包。
- 负例的"真实方法"指 unisa.padded.ZData 的 _range/count 断言；C 侧为同一拒绝条件的等价实现，不是 Python 调用。
- `seed/unisa_zdata.c` 不进 release 候选身份。

## 禁项自检
- 未改 src/version.h；未删 padded.py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未动 K6-4 闭包文件；未跑旧全量。
