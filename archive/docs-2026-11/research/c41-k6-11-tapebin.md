# c41 K6-11：unisa.tapebin → C 参考对拍（2233 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-11**；代裁依据 `ruling-k6-11-tapebin-auth-2233.md`；K6-10 已由 `cdx-k6-10-ruling-2220.md` 限定 PASS。
- 基：开工时 HEAD = `8ed49759`（main=origin）；本刀之前未动 main 上他人提交。
- 墙钟预算 ≤30 min；本刀约 14:34Z–14:39Z。
- 首红：**无**。编译无告警（`-Wall -Wextra`）；直跑一次即全绿。

## 范围
- encode（script 构造的 Tape）：`U` 单元标记、`G` 链接记录（global/extern/gdef）、`L` 标签、`E` 指令、`S` 字符串（按 align）、`B` 按大小的零全局；可选 SOURCE_SHA（section 4）。5 个脚本。
- decode：对上述 5 个 blob（C 产出，与 python encode 字节相同）解码，输出 ORIGIN / SHA / to_canonical_text。
- target_id：6 个目标名。check_origin：3 个点（origin=0；origin 与目标一致；origin 不一致但 force）。
- C 双跑确定性；6 个负例两侧同拒且 stdout 空：坏 magic、截断、头部 origin=7（hash 不变）、翻转正文字节（hash mismatch）、未知目标名、origin 不一致。
- 输出：19 个 stdout 对拍。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_tapebin.c` | C99 参考：ULEB/SLEB 规范性检查（与 `_u`/`_s` 重编码比较）、名字与常量池、`_collect`、`_encode_records`、头与目录（64 B 头、24 B 条目、8 字节对齐与零填充）、SHA-256（K/H0 由素数派生，见下）、decode 的全部结构检查与“重编码比较” |
| `tests/unisatapebincheck.sh` | 夹具：python 驱动同一 Tape/tapebin API 产出基准；19 个 stdout 对拍；C 双跑；6 负例 |
| `tests/gate.sh` | 仅加 `unisa-tapebin` 的 req 行与 job 行 |
| `research/c41-k6-11-tapebin.md` / `research/c41-k6-11-tapebin-run.log` | 本页 / 直跑原日志 |

## 实现要点（供审）
- SHA-256 常量：K 为前 64 个素数立方根小数部分的前 32 位，H0 为前 8 个素数平方根小数部分的前 32 位（python 整数运算派生；与标准值逐一相同）。digest 在编码中被写入头；每个 encode 点都通过字节对拍间接检验 SHA-256，且 decode 对 hash mismatch 的负例直接检验。
- 规范性：C 解码的 ULEB/SLEB 通过“用解出的值重编码后与原字节比较”判定规范性；这与 python 的判定等价（若解出的值重编码得到原字节，则原字节即该值的规范形式；反之亦然）。
- decode 末尾的“重编码比较”与 python 的 `encode(restored) != blob` 对应。

## 限定（不得外推）
- 点采样：5 个 encode 脚本、5 个 decode 输入、6 个目标、3 个 check_origin 点、6 个负例。不证 tapebin.py 的全部记录形式、全部操作码或大 blob。
- **未覆盖（C 显式拒绝，不称接受）**：
  - fp ops（操作码超出非 fp 表）；
  - decode 不重复 python 的文本往返（`parse(to_canonical_text) == records`）；代之以“池中名字必须为 `[A-Za-z0-9_.$]+`”，python 可能接受的名字在 C 侧被拒绝；
  - 名字位置上以整数存放的 L/s 操作数，C 拒绝（python 经文本检查拒绝，同为拒绝）；
  - 1 GiB 上限在代码中比较，但无样本触及。
- **tapebin_shape 不在本刀**（另叶 K6-11b，未授权）。
- `seed/unisa_tapebin.c` 不进 release 候选身份；tapebin.py / tape.py 未删、未改，产品路径未切换。
- 不称 image / gold / vm 全路径已绿。

## 源件 sha256
- `unisa/tapebin.py` = `85cac068f2df7badb8ad645b4345dc493855e22137cd90f31779a0c56ae128b0`（只读）
- `unisa/tape.py` = `2292a69dbe7e9e7662066af8e90bec2ba59317ddc539862a2acbef41f37bb674`（只读）
- `unisa/fp.py` = `510f89bd337131eb103ff26c9f40625c21b2ef5e67040a4e583a625446e0e856`（只读）
- `unisa/padded.py` = `7f8968fd69ed5c0bd6d3af0d653f0936ed36f484a0689a94081800c24183758f`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_tapebin.c` = `c925fc76b5cbaba67363789278791fa82f16c82cd56568040cb4b06a3aed2dff`
- `tests/unisatapebincheck.sh` = `7a7d933418af939e62db20ff7bf74905914c08ab55ba20dca4d063d6d7b873d2`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisatapebincheck.sh`；UTC 14:38:28Z–14:38:30Z；rc=0；`unisatapebin same 19 diff 0 neg 6/6`；日志 `research/c41-k6-11-tapebin-run.log`。
- gate：`./tests/gate.sh --suite unisa-tapebin`；UTC 2026-10-11T14:38:42Z–14:38:44Z；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..10 的 seed/夹具闭包；未写 tapebin_shape；未开 K6-11b/K6-10b 或其他叶子；未跑旧全量。
