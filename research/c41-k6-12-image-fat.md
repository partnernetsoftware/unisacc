# c41 K6-12：unisa.image.fat → C 参考对拍（2312 代裁授；cc 实现+证据）

## claim / 预算
- claim = **K6-12**；代裁依据 `ruling-k6-12-image-fat-auth-2312.md`；K6-11b 已由 `cdx-k6-11b-ruling-2258.md` 限定 PASS。
- 基：开工时 HEAD = `23300830`（main）；本刀未动他人提交。
- 墙钟预算 ≤30 min；本刀约 15:13Z–15:15Z。
- 首红：**无**。编译无告警（`-Wall -Wextra`）。直跑一次即全绿；另有一次仅修正 fixture 自身的死行与字节数回显文字（不改期望，见下）。

## 范围
- `write(slices)`：CPU 表（x86_64：cputype 0x01000007、sub 3、align 12；arm64：0x0100000C、sub 0、align 14）；头 8 字节（FAT_MAGIC BE、切片数 BE）；每切片 20 字节描述符（cpu、sub、offset、size、align，均 BE）；体内按 `round_up(off, 1<<align)` 对齐并补零。
- 点采样 7 个规格（spec）：单 x86_64 切片；单 arm64 切片；双切片两种顺序（x86_64→arm64、arm64→x86_64）；页长整数倍切片；空切片镜像（arm64 空、x86_64 零长）；空切片列表（仅头）。输出以十六进制与 python 逐字节对拍（7 点）。
- 对齐性质（对 C 输出，独立 python 检查）：每个描述符的 offset 是 `1<<align` 的倍数、不早于前一切片末尾、size 与规格一致、载荷与规格一致；7 点全部通过。
- C 双跑确定性（7 点）。
- 负例 3 个，两侧同拒且 stdout 空：未知 arch（`ppc`，python KeyError）；负的 fill 计数（`-4`）；奇数长度十六进制（`abc`）。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_image_fat.c` | C99 参考：CPU 表、头与描述符（BE）、按 2 的幂对齐的体布局、规格解析（`x` 十六进制 / `f` 填充字节） |
| `tests/unisaimagefatcheck.sh` | 夹具：python 驱动 `fat.write` 产出基准；7 点字节对拍；对齐性质检查；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-image-fat` 的 req 行与 job 行 |
| `research/c41-k6-12-image-fat.md` / `research/c41-k6-12-image-fat-run.log` | 本页 / 直跑原日志 |

## fixture 修正（诚实记录）
首版夹具的对齐检查脚本含一行无用的死代码，且 "C bytes" 回显把十六进制字符数误标为字节数。两处均只影响输出文字与夹具内部代码，不改任何期望或比较；修正后重跑，7 点与 3 负例结果不变。该修正发生在首次全绿之后、提交之前。

## 源件 sha256
- `unisa/image/fat.py` = `97654b5de667d53c7f47fcb8c4f98406e3a7e617d1e2f91af7dc19bbb33dcb0a`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_image_fat.c` = `a45925304fd090f2cd7869ddbf32d9c8f3406abe1b376ff57bdc7db3ac6fe4e5`
- `tests/unisaimagefatcheck.sh` = `9d06bfe8fbda0eeb92be0a513c081cb13cb26223b30bf25de0a2bdf76b8d7ef9`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisaimagefatcheck.sh`（修正后的最终运行）；rc=0；`unisaimagefat same 7 diff 0 neg 3/3`；日志 `research/c41-k6-12-image-fat-run.log`。
- gate：`./tests/gate.sh --suite unisa-image-fat`；rc=0；`failed 0 unverified 0`（UTC 起止见 gate 输出）。

## 限定（不得外推）
- 点采样：7 个规格的字节对拍、7 个对齐性质检查、3 个负例。不证 fat.py 对更大切片、更多架构或 32 位长度上限的行为。
- **不称产品闭包**；不切换产品路径到 seed；不扩称 elf / macho / pe / ape 全路径（这些模块未开）。
- 负例的"空切片策略"：空镜像（`-`）与零长填充被当作合法输入，两侧一致接受；未设计为拒绝。
- `seed/unisa_image_fat.c` 不进 release 候选身份；fat.py / bits.py 未删、未改。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..11b 的 seed/夹具闭包；未开 elf / macho / pe；未跑旧全量。
