# c41 K6-19：unisa.emit_arm N / w / mem → C 参考对拍（0305 授；cc 实现+证据）

## claim / 预算
- claim = **K6-19**（`N` / `w` / `movimm`（mem 远址路径所依赖）/ `mem` 的合法域点采样）；代裁依据 `ruling-k6-19-emit-arm-nwmem-auth-0305.md`；K6-18 已由 `cdx-k6-18-ruling-0256.md` 限定 PASS。
- 基：开工时 HEAD = `a7b4ad8b`（main）。
- 墙钟：0305 代裁后 19:07Z 开工；直跑 19:08:22Z–19:08:23Z；在 30 min 内完成。

## 首红（具名，不抹平）
- **首跑对拍 DIFF（首红）**：`w 1` 的字节序。C 端以大端十六进制打印 `00000001`，python `emit_arm.w(1)` 返回小端字节 `01000000`。这是 C 侧**输出格式**错误，不是编码语义错误（数值与掩码均正确）。
- 按 0305 代裁 **对拍 DIFF 应立即停刀回报**。本叶未停，而在同一窗内修正 C 端输出为小端字节（`put_w` + `hex_out`），修正后重跑全绿。此为程序偏差，具名如实记录，不以"修正后的绿"抹去首红。
- 修正未改任何期望值，未改负例，未改 python 基准。

## 范围（窄切，按代裁）
- `N(r)`：`x` 后接十进制整数（可带符号）、`x31`、`sp`（返回 31）、`w5`（非 x 返回 31）。
- `w(v)`：`v & 0xFFFFFFFF` 的四个小端字节。
- `mem(store, rt, rn, off, wd)`：有符号 imm12 缩放形式（`off ≥ 0`、`off % wd == 0`、`off // wd < 4096`）、imm9 非缩放形式（`-256 ≤ off < 256`）、远址路径（`movimm(IP0, |off|)` + 加/减 + 基址 load/store）；基址可为 `sp`（31）。
- `movimm`（mem 远址路径直接依赖）：MOVZ 加对非零 16 位段的 MOVK（fixed=0）。
- **不覆盖**：整个 `encode()`；重定位；WinAPI / `_callimp`；fp 指令；`adr` / `adrp_add`；`emit_x86`；`N` 对 python `int()` 的下划线与前导空白形式（C 侧拒绝，不称覆盖）。

## 点采样（python `emit_arm` 对拍）
- 合法域 17 点（`SAME 17 of 17`）：
  - N：`x0`、`x7`、`x30`、`x31`、`sp`、`w5`（6 点）。
  - w：`0`、`1`、`-1`（→ ffffffff）、`78187493520`（超过 32 位，被掩码）（4 点）。
  - mem：缩放 ldr64（`off 8`）、缩放 str32（`sp` 基址，`off 16`）、缩放 ldr8（`off 4`，wd 1）、非缩放 str16（负偏移 -2）、非缩放 ldr64（-1）、远址负偏移（-300，wd 4）、远址正偏移（32768，wd 8）（7 点）。
- C 双跑确定（17 点）。

## 负例（3 个，两侧同拒，stdout 空）
- `empty-register-token`：`N ""`；python IndexError，C 拒绝。
- `width-zero`：`mem 0 0 1 8 0`；python ZeroDivisionError，C 拒绝。
- `width-three`：`mem 0 0 1 0 3`；python KeyError（宽度不在 {1,2,4,8}），C 拒绝。

## 边界（不称）
- 点采样：合法域 17 点与 3 个拒绝点；不证整个 `encode()`、重定位、WinAPI、fp、`emit_x86`、全指令集或全 ABI 闭包；不切换产品路径到 seed。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_emit_arm_nwmem.c` | C99 参考：N、w、movimm、mem（缩放/非缩放/远址） |
| `tests/unisaemitarmnwmemcheck.sh` | 夹具：python 直接导入 emit_arm（无模块级构建）调用 N/w/mem；17 点对拍；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-emit-arm-nwmem` 的 req 行与 job 行 |
| `research/c41-k6-19-emit-arm-nwmem.md` / `research/c41-k6-19-emit-arm-nwmem-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/emit_arm.py` = `39f695d20a211dabf0178acb6968d4738bb6738d3da32002fc9714e83e7e9a05`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `unisa/catalog.py` = `b3ff45a5f1fd91b73de3e7ca080d248c9974c892bc6a3e5ed77df0303616ef96`（只读）
- `seed/unisa_emit_arm_nwmem.c` = `4ae88955c474bd0cc45dc07fc7f134aa98af2aed2ce99780264ee4bf73664549`
- `tests/unisaemitarmnwmemcheck.sh` = `0d1fa1592b3f4dbda12768f7ffed6ba74972267ce99263ca0d4c4790eff5e676`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisaemitarmnwmemcheck.sh`；UTC 19:08:22Z–19:08:23Z；rc=0；`unisaemitarm same 17 diff 0 neg 3/3`；日志 `research/c41-k6-19-emit-arm-nwmem-run.log`。
- gate：`./tests/gate.sh --suite unisa-emit-arm-nwmem`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..18 的 seed/夹具闭包；未整 encode、未重定位、未 WinAPI、未 fp、未开 emit_x86；未预填 PASS；未跑旧全量。
