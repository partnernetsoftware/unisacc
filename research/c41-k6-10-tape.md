# c41 K6-10：unisa.tape → C 参考对拍（2159 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-10**；代裁依据 `ruling-k6-10-tape-auth-2159.md`；K6-9 已由 `cdx-k6-9-ruling-2154.md` 限定 PASS。
- 基：本刀开工时 HEAD = `2779b4bd`（main=origin）；运行时 HEAD = `aa46a603`（他人在 main 上的 cxif 研究提交，未触本刀路径）。
- 墙钟预算 ≤30 min；本刀约 13:59Z–14:10Z。
- 首红：**无**。夹具层的三处操作数写法错误在首次直跑时被两侧（或 C 侧）拒绝，已在夹具中改为与 python 一致的写法（见下"夹具修正"），未改期望值。
- version.h 未改（0.0.40）；Latest 未动；未 bump / Draft / 签 / 删 py。

## 范围
- 点采样覆盖：
  - build 脚本（`L` label / `E` emit / `S` string 按 align / `B` 按大小的零全局）→ `to_text` 与 `to_canonical_text`；5 个脚本。
  - parse 文本（`.unit 2`、`.global/.extern/.gdef`、`.str` 含转义与注释内的 `;`、`.bss`、`[base+off]`/`[base-off]` 括号操作数、`callm [..]`、`.zero [..], n`、`.st [..], src, n`）→ `to_text` 与 `to_canonical_text`；3 个文件。
  - parse 往返：对每个文件 `canon(parse(F))` 的 C 输出作为新输入，再与 python 对拍（text 与 canon 两种）。
- 输出字节逐一相等；C 双跑确定性；6 个负例两侧同拒且 stdout 空。
- 负例：未知 op（`frob`）；非法寄存器（`r9`）；base-0 整数前导零（`010`）；emit 操作数个数（`add64` 缺一）；未闭合 `.str`；`.gdef` 早于 `.unit 2`。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_tape.c` | C99 参考：SHAPE（非 fp 部分）、`_fmt`、`_quote`、`_unescape`、`_strip_comment`、`_regroup`、`parse`、`label/emit/string`、`to_text`、`to_canonical_text`；拒绝一律 stderr + 无 stdout |
| `tests/unisatapecheck.sh` | 夹具：python 驱动同一 Tape API 生成基准；22 个 stdout 对拍；C 双跑；6 负例 |
| `tests/gate.sh` | 仅加 `unisa-tape` 的 req 行与 job 行 |
| `research/c41-k6-10-tape.md` / `research/c41-k6-10-tape-run.log` | 本页 / 直跑原日志 |

## 夹具修正（诚实记录）
直跑前三次，夹具里的操作数写法不符合 SHAPE（`.sys6` 多写一个寄存器；`.st` 把基址写在括号外；`callm r2+4 0` 形式错误）。这些是**夹具**的写法错误，不是参考实现与 python 的分歧；修正为 python 也接受的形式后，两侧均通过。前三次运行中只看到了 C 侧拒绝，未对该输入跑 python，因此这三处未以 python 对照过，只是以修正后的 python 一致形式作为样本。

## 源件 sha256
- `unisa/tape.py` = `2292a69dbe7e9e7662066af8e90bec2ba59317ddc539862a2acbef41f37bb674`（只读）
- `unisa/fp.py` = `510f89bd337131eb103ff26c9f40625c21b2ef5e67040a4e583a625446e0e856`（只读）
- `unisa/padded.py` = `7f8968fd69ed5c0bd6d3af0d653f0936ed36f484a0689a94081800c24183758f`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_tape.c` = `2ff772d69f8c0238aebc58af61d1d05894fe854075717632bd59f62a69009ce0`
- `tests/unisatapecheck.sh` = `430a6416ae941f23e9309df5a3658e89ca9a22d6c670d4957b367d3f0c5bcf9b`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisatapecheck.sh`；UTC 14:09:24Z–14:09:26Z；rc=0；`unisatape same 22 diff 0 neg 6/6`；日志 `research/c41-k6-10-tape-run.log`。
- gate：`./tests/gate.sh --suite unisa-tape`；UTC 2026-10-11T14:09:40Z–14:09:43Z；rc=0；`failed 0 unverified 0`。

## 限定（不得外推）
- 点采样（22 个输出）；不证 tape.py 的全部 op、全部文本形式或 gold/vm/layout 全路径；不称产品闭包。
- C 侧显式拒绝而非接受的范围：fp.py 的 OPS3/OPS2（SHAPE 中）、超过 int64 的整数、非 ASCII 字节、NUL 字节、整数字面量中的下划线、1 MiB `.str` 上限。这些在 `seed/unisa_tape.c` 文件头已列出；不构成"接受但不同"。
- `seed/unisa_tape.c` 不进 release 候选身份；tape.py 未删、未改，产品路径未切换。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..9 的 seed/夹具闭包；未开 K6-10b/K6-9b/K6-8b 或其他叶子；未跑旧全量。
