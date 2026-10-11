# c41 K6-11b：unisa.tapebin_shape → C 参考对拍（2250 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-11b**；代裁依据 `ruling-k6-11b-tapebin-shape-auth-2250.md`；K6-11 已由 `cdx-k6-11-ruling-2247.md` 限定 PASS。
- 基：开工时 HEAD = `3c5c87fc`（main）；本刀未动他人提交。
- 墙钟预算 ≤30 min；本刀约 14:51Z–14:56Z。
- 首红：**无**。编译无告警（`-Wall -Wextra`）。直跑一次即全绿；另有一次仅为可移植性修正（见下）。

## 范围
- `render()`（opcode 表，71 行，含 fp.OPS3 的 14 个 rrr 与 fp.OPS2 的 10 个 rr；顺序为 tape.SHAPE 插入序）、`render_records()`（记录种类表，含 global/extern/gdef 与 unit）、`render_c()`（C 头：TB_OPCOUNT、记录常量、tb_opnames、tb_shapes）：C 输出与 python 字节对拍（3 点），C 双跑确定性。
- 仓内快照只读对照：C 渲染结果与 `docs/tapebin-v1.shape.tsv`、`docs/tapebin-v1.records.tsv`、`src/tapebin_shape.inc` 逐字节相等（3 点，只读 cmp）；python 自身 `--check`（只读）通过；C 的 `check` 比较器对三个仓内快照均 OK。
- 负例（4 个，两侧同拒、stdout 空）：
  - stale 期望：在临时副本中把 `ri` 改为 `rr`（不改仓内文件）后 `check render` 拒绝；
  - 未知 kind：`check bogus …`；
  - 未知模式：`write`（同时证明夹具与 C 均无写入路径）；
  - 缺失文件：`check c <不存在>`。

## 只读与禁项
- 夹具全程不调用 `--write`，不修改 `docs/`、`src/`、`exec/`；仓内快照未被改动（git status 仅见本刀路径与 gate.sh）。
- 未写 `src/`（含 `tapebin_shape.inc`/`version.h`）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 K6-1..11 的 seed/夹具闭包。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_tapebin_shape.c` | C99 参考：render / render_records / render_c 与 `check KIND FILE` 比较器；表按 SHAPE 插入序（非 fp 表 + fp.OPS3 + fp.OPS2） |
| `tests/unisatapebinshapecheck.sh` | 夹具：python 渲染对拍、仓内快照只读对照、python `--check`（只读）、4 负例；无 `--write` |
| `tests/gate.sh` | 仅加 `unisa-tapebin-shape` 的 req 行与 job 行 |
| `research/c41-k6-11b-tapebin-shape.md` / `research/c41-k6-11b-tapebin-shape-run.log` | 本页 / 直跑原日志 |

## 可移植性修正（诚实记录）
首版夹具用 GNU 专有的 `sed '0,/re/'` 生成 stale 副本；它在 Linux 上通过，但 BSD sed（macOS）不支持。改为 python 单行替换后重跑，仍全绿。该修正发生在首次全绿之后、提交之前，未改期望值。

## 实现要点（供审）
- 表与 tape.py 的 SHAPE 插入序逐项一致（71 项）；fp 部分由 fp.py 的 OPS3/OPS2 名单与 "rrr"/"rr" 形状派生，与 tape.py 的 `SHAPE.update` 一致。
- `render_c` 的宏名为 LINK_RECORDS 键的大写（GLOBAL/EXTERN/GDEF），与 python `k.upper()` 一致。
- 空形状（如 ret/nop）渲染为行末带制表符的空字段，与 python `"".join(())` 一致。

## 限定（不得外推）
- 点采样：3 个渲染的字节对拍、3 个仓内快照只读对照、4 个负例。不证 tapebin_shape.py 的全部行为，也不证 `--write` 路径（本刀刻意不实现也不调用）。
- **不称产品闭包**；不切换产品路径到 seed；**不称已重写 `src/tapebin_shape.inc`**（该文件只被读取比较）。
- `seed/unisa_tapebin_shape.c` 不进 release 候选身份；tapebin_shape.py / tape.py 未删、未改。
- 注意：K6-11 的 tapebin 解码器仍只覆盖非 fp 操作码（见 K6-11 页限定）；本刀的表覆盖全部 71 个操作码，但仅限于"表的渲染"，不涉及 tapebin 解码对 fp 操作码的处理。

## 源件 sha256
- `unisa/tapebin_shape.py` = `97412334d40c18194deba793b64f7a5aeeb4983fdf56c0a31582bcc9b44ae6f7`（只读）
- `unisa/tape.py` = `2292a69dbe7e9e7662066af8e90bec2ba59317ddc539862a2acbef41f37bb674`（只读）
- `docs/tapebin-v1.shape.tsv` = `69bd907482af21f1cf0eb864357659e91fe6b8a42fa11c75f6ab45ce6a96825e`（只读）
- `docs/tapebin-v1.records.tsv` = `3abf39251bc185d795ff85f1abf9d9aa351dea7c70cac006ceafccb244bc1e1a`（只读）
- `src/tapebin_shape.inc` = `2e605af83cb4d00184cde1f9f94ce7e15a56487ad2351790aa906342028b7f16`（只读）
- `seed/unisa_tapebin_shape.c` = `84b990fdef219afa8f5edab6c7a3ed695274c8a9056c5338b7e0825d77ff3f1b`
- `tests/unisatapebinshapecheck.sh` = `bfd684cbf610056ad4bd0c2dcafdff258d3f63afb0506254f500761ac3ded09f`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisatapebinshapecheck.sh`（可移植性修正后的最终运行）；UTC 14:56:2xZ 起止见 run log；rc=0；`unisatapebinshape same 6 diff 0 neg 4/4`；日志 `research/c41-k6-11b-tapebin-shape-run.log`。
- gate：`./tests/gate.sh --suite unisa-tapebin-shape`；UTC 2026-10-11T14:56:25Z–14:56:26Z；rc=0；`failed 0 unverified 0`。
