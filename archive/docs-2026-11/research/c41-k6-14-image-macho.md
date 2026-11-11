# c41 K6-14：unisa.image.macho → C 参考对拍（2350 授；cc 实现+证据）

## claim / 预算
- claim = **K6-14**（`write()` 静态字节路径）；代裁依据 `ruling-k6-14-image-macho-auth-2350.md`；K6-13 已由 `cdx-k6-13-ruling-2348.md` 限定 PASS。
- 基：开工时 HEAD = `45de2a75`（main）。
- 墙钟：15:52Z 起（代裁）；C 首次字节相等 15:53:47Z；最终直跑 15:54:10Z–15:54:16Z；余量内完成。
- 首红：**无**实质红。两次夹具失败均为 cc 自身的代码/夹具错误，在提交前修正，未涉及期望改写或负例改写：
  1. C 初版 `die` 在 SHA-256 块之后才声明（隐式声明），补前向声明；
  2. C 的 LC_DYLD_INFO_ONLY 字段多两个 uint32，且 bind size 误为 0（应为 DLBIND 长度），按 python 的 `<II`+10×I 布局修正（首个规格随即字节相等）；
  3. 夹具布局检查的 load command 末尾期望误用 hdrs（含 SLACK），改为 `32 + sizeofcmds`。
  以上三项都是检查代码或 C 参考的实现错误，属于"首跑前的代码修正"，不是对 python 行为的改判。

## 范围
- write(arch, text, data, entry, full)：Mach-O 64 头（magic、cpu、subcpu、MH_EXECUTE、ncmds=12、sizeofcmds、flags 0x200085）；12 条 load command（__PAGEZERO；__TEXT/__text；__DATA/__data+__bss；__LINKEDIT；LC_LOAD_DYLINKER；LC_LOAD_DYLIB；LC_MAIN；LC_BUILD_VERSION；LC_DYLD_INFO_ONLY（四个 eager bind，DLBIND 字节）；LC_SYMTAB；LC_DYSYMTAB；LC_CODE_SIGNATURE）；SLACK 156 字节；text 页对齐（PAGE 0x4000）；data 与 __bss；__LINKEDIT 的字符串表与签名；ad-hoc SuperBlob（CodeDirectory，SHA-256 按 PAGE4 = 0x1000 分页覆盖 out[0:sigoff]，标识 `unisa\0`）。
- SHA-256 实现复用 K6-11 已验证的常量与代码块（`seed/unisa_tapebin.c` 中的 SK/sha256），常量由素数派生并已与 hashlib 对拍过。
- 点采样 5 个规格（python `macho.write` 对拍，字节相等）：x86_64 bss（full > len(data)）；arm64 非空 text/data；x86_64 text 跨 16 KB 页（20000 字节，sigoff 0xc050，13 个签名页）；arm64 空 text；arm64 有符号负 entry（地址合法）。
- 布局与签名性质（对 C 输出，独立 python 检查，**签名页哈希由 hashlib 重算比对**）：头、SLACK 零填、12 条 load command 顺序与大小、四段字段、__text 地址与字节、LC_MAIN entryoff、DYLD_INFO bind 偏移与长度、LC_CODE_SIGNATURE 偏移与长度（延伸至文件末尾）、SuperBlob 魔数与槽、CodeDirectory 魔数/版本/标志/codeLimit/槽数、标识、每页哈希、文件长度。5 点全部通过。
- C 双跑确定性（5 点）。
- 负例 3 个，两侧同拒且 stdout 空：未知 arch（`riscv`，python KeyError）；bss 为负（full < len(data)，python struct.error）；入口地址为负（`-(HDRS+1)`，python struct.error）。

## 边界（不称）
- 本叶只证"写出的字节与 python 相等、布局与 ad-hoc 签名字段自洽"。**不称**可加载、可运行、可被 codesign 验证或被真实内核接受；不称 dyld 加载证明；不称产品闭包。
- 未采样：任意 PAGE 边界上的 __bss 额外页、LC_* 的其它参数组合、非 ASCII 标识、32 位长度上限。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_image_macho.c` | C99 参考：Mach-O 64 静态写出（头、load command、页对齐段、DLBIND、ad-hoc SuperBlob） |
| `tests/unisaimagemachocheck.sh` | 夹具：python 驱动 `macho.write` 基准；5 点字节对拍；布局/签名检查（hashlib 重算页哈希）；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-image-macho` 的 req 行与 job 行 |
| `research/c41-k6-14-image-macho.md` / `research/c41-k6-14-image-macho-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/image/macho.py` = `397e238aad449a42547d3ea056bbee3874207b0f04311b32d290ee8378fb80b4`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_image_macho.c` = `8e88e46d663317f13b725a0a1b5fa1b430e4d7abae454d66c2ecb9592479cd0a`
- `tests/unisaimagemachocheck.sh` = `e734dd3ac5d418085488bccd4b4971e1a77f3f34837d47a3aa527e41ed9d062f`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisaimagemachocheck.sh`；UTC 15:54:10Z–15:54:16Z；rc=0；`unisaimagemacho same 5 diff 0 neg 3/3`；日志 `research/c41-k6-14-image-macho-run.log`。
- gate：`./tests/gate.sh --suite unisa-image-macho`；rc=0；`failed 0 unverified 0`（UTC 起止见 gate 输出）。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..13 的 seed/夹具闭包；未开 pe / ape；未跑旧全量。
