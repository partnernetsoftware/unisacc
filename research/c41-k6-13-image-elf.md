# c41 K6-13：unisa.image.elf → C 参考对拍（2331 授；2345 首红代裁选 (a)；cc 实现+证据）

## claim / 预算
- claim = **K6-13**（静态 write 路径）；代裁依据 `ruling-k6-13-image-elf-auth-2331.md`；首红代裁 `ruling-k6-13-first-red-neg-entry-2345.md`（选 (a)）；K6-12 已由 `cdx-k6-12-ruling-2321.md` 限定 PASS。
- 基：开工时 HEAD = `46cf167c`（main）。
- 墙钟：首轮 15:33Z 起，首红 15:33:39Z 停；代裁后续跑 15:46Z–15:47Z；余量内完成（代裁目标 ≤00:01 SGT）。
- 首红：**1 次**（NEG-negative-entry：C 解析拒绝 `entry -4`，python 接受并输出合法 Q）。按代裁 (a) 修正 C 解析与打包，不改 python、不改期望；6 个正例在修正前后均字节相等。

## 首红与修正（诚实记录）
- 首红原因：负例选择错误——`entry -4` 的地址 `VADDR+176-4` 合法，python 接受；C 早先在解析处拒绝。
- 修正：`entry` 按有符号十进制解析；`e_entry = VADDR + HDRS + entry`；仅当结果为负（超出 Q 无符号范围）时与 python 同拒（python: `struct.error`）。`full` 仍为非负十进制，超过 64 位同拒（修正中同时补上 ERANGE 检查）。
- 夹具：负例改为 (i) 未知 arch `riscv`；(ii) `entry -4194481`，即 `-(VADDR+176)-1`，地址为负；(iii) `full -1`。新增一个正例 `a-negentry`（arm64、entry -4，合法地址），证明有符号接受面。

## 范围
- 静态 write：ELF64 头（e_ident、ET_EXEC、EM_X86_64/EM_AARCH64、e_entry、phoff 64、phentsize 56、phnum 2）；两个 PT_LOAD：r-x 覆盖头与 text（p_offset 0），rw- 覆盖 data（p_offset = round_up(tend, PAGE)，p_vaddr = VADDR + p_offset），filesz = len(data)，memsz = full（默认 len(data)）。
- 点采样 7 个规格（均在 python `elf.write(..., dynamic=False)` 对拍）：x86_64 bss（full > len(data)）；arm64 非空 text 与 data（full 默认）；x86_64 跨页 text（5000 字节）；arm64 空 text；x86_64 text 恰好结束于页边界（3920 + 176 = 4096）；arm64 bss 点（full = 4096）；arm64 有符号负 entry。
- 布局性质（对 C 输出，独立 python 检查）：e_ident、机器码、e_entry、phoff/shoff、phnum、两个 PT_LOAD 的类型/标志/offset/vaddr/paddr/filesz/memsz/align、data 对齐（offset 为 PAGE 倍数且 vaddr ≡ offset mod PAGE）、文件长度、text/padding/data 字节；7 点全部通过。
- C 双跑确定性（7 点）。
- 负例 3 个，两侧同拒且 stdout 空：未知 arch；entry 地址为负；full 为负。

## 不覆盖（口径）
- `dynamic=True`（_write_dynamic：interp、动态段、GLOB_DAT 槽）**未实现、未采样、不称闭包**；本叶按代裁不做可选 1 点（余量与口径考虑）。
- 不扩称 macho / pe / ape。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_image_elf.c` | C99 参考：静态 write（头、两段、页对齐、有符号 entry 解析与 Q 范围检查） |
| `tests/unisaimageelfcheck.sh` | 夹具：python 驱动 `elf.write(dynamic=False)` 基准；7 点字节对拍；布局检查；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-image-elf` 的 req 行与 job 行 |
| `research/c41-k6-13-image-elf.md` / `research/c41-k6-13-image-elf-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/image/elf.py` = `c54fd7244eecdde7f109a6b396a93e414e8ed060d35b3433d79fa872dc9c5a95`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_image_elf.c` = `d5c6ced69d2d67f016c598096f5867090a2d99e4d6b00a87d6c5beaddb465796`
- `tests/unisaimageelfcheck.sh` = `fa6a1db804c019a6aafded5a9edd49b6b5598709612bbdeb34099b6c5fda6c63`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisaimageelfcheck.sh`（修正后）；UTC 15:47:23Z 附近；rc=0；`unisaimageelf same 7 diff 0 neg 3/3`；日志 `research/c41-k6-13-image-elf-run.log`。
- gate：`./tests/gate.sh --suite unisa-image-elf`；rc=0；`failed 0 unverified 0`。

## 限定（不得外推）
- 点采样：7 个静态规格、3 个负例。不证 elf.py 的动态路径、更大段、其它架构或 64 位长度边界。
- 不称产品闭包；不切换产品路径到 seed。
- `seed/unisa_image_elf.c` 不进 release 候选身份；elf.py / bits.py 未删、未改。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..12 的 seed/夹具闭包；未开 macho / pe / ape；未跑旧全量。
