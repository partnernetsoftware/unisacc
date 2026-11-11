# c41 K6-15：unisa.image.pe → C 参考对拍（0048 授；cc 实现+证据）

## claim / 预算
- claim = **K6-15**（静态 write 路径，**stub 为空、dynamic=False**）；代裁依据 `ruling-k6-15-image-pe-auth-0048.md`；K6-14 已由 `cdx-k6-14-ruling-0046.md` 限定 PASS，接纳见 `ruling-k6-14-pass-accept-0048.md`。
- 基：开工时 HEAD = `482f9c15`（main）。
- 墙钟：16:48Z 代裁后开工；首个字节相等 16:55Z；最终直跑 16:58:08Z–16:58:09Z；在 30 min 内完成。
- 首红：**无**实质红。两处为检查代码自身的错误，提交前修正，未改期望、未改负例：
  1. 布局检查的重定位循环读取了按 512 字节补齐的 SizeOfRawData，尾部零块不前进导致死循环（被外层 58 秒看门狗打断，报出空消息）；改为读取 VirtualSize，并加块大小下限检查。
  2. 布局检查中两处无效的条件表达式（`if False else`）清理为直接断言。

## 范围（窄化，按 0048 允许）
- write(arch, text, data, entry, relocs, bss, full)：DOS 头（e_lfanew=64，无 stub）；PE 签名；COFF（4 节、240 字节 PE32+ 可选头、特征 0x22）；可选头标准字段与 Windows 字段（子系统 3、DllCharacteristics 0x8160）；数据目录 1（导入）、5（重定位）、10（load config，0x140）、12（IAT）；节表 .text/.rdata/.data/.reloc（节对齐 0x1000、文件对齐 0x200）；导入节（KERNEL32.dll，IMPORTS 的 22 个名字，INT/IAT 相同，hint/name 表，load-config 目录含栈 cookie 指针）；.reloc 由 cookie 指针与给定重定位生成（DIR64，奇数补 ABSOLUTE）。
- **不采样**：`stub`（APE 壳脚本嵌入）、`dynamic=True`（FORWARD_IMPORTS）。负的 relocs 在两侧按"规格规则"拒绝（python 本会接受部分负值，故本叶不称覆盖）。
- 7 个正例（python `pe.write` 对拍，字节相等）：x86_64 基本；arm64 十六进制 text/data；x86_64 full=64 bss；x86_64 text 跨 4 KB 页（5000 字节）+ 两页重定位（8 与 4096）；arm64 full=-1（python 以 max(1, full) 钳制，接受，C 同样接受）；arm64 空 text + bss 16；arm64 有符号负 entry（地址合法）。
- 布局与内容性质（对 C 输出，独立 python 检查）：DOS/PE 签名、COFF、可选头字段、四节头与 RVA 关系（.rdata = .text 之后，.data = .rdata 之后，.reloc = .data 之后）、.data 虚拟大小 = round8(max(1,full))+8+bss、数据目录、导入描述符的 OriginalFirstThunk/Name/FirstThunk、KERNEL32.dll 名字、22 个 hint 名字、load-config 大小与自身字段、栈 cookie 指针、.reloc 解码后的 DIR64 集合（与期望集合完全相等）、.text/.data 字节位置。7 点全部通过。
- C 双跑确定性（7 点）。
- 负例 3 个，两侧同拒且 stdout 空：未知 arch `riscv`（python KeyError）；入口地址为负（`-4097`，python struct.error）；入口地址超过 32 位（`4294967295`，python struct.error）。

## 边界（不称）
- 只证"写出的字节与 python 相等、静态布局与导入/重定位内容自洽"。**不称**可在 Windows 加载、可被 APE 壳使用、真实 Windows 加载证明、dynamic=True 闭包、stub 路径、产品闭包；不扩称 ape。
- 工作树中另有一个未跟踪的他人研究文件（`research/csih-cx-modularity-…`），未纳入本次提交。

## 文件（本刀写）
| 文件 | 作用 |
|---|---|
| `seed/unisa_image_pe.c` | C99 参考：PE32+ 静态写出（头、导入节、load config、.reloc、节表） |
| `tests/unisaimagepecheck.sh` | 夹具：python 驱动 `pe.write`（stub 空、dynamic False）基准；7 点字节对拍；布局与内容检查；C 双跑；3 负例 |
| `tests/gate.sh` | 仅加 `unisa-image-pe` 的 req 行与 job 行 |
| `research/c41-k6-15-image-pe.md` / `research/c41-k6-15-image-pe-run.log` | 本页 / 直跑原日志 |

## 源件 sha256
- `unisa/image/pe.py` = `655cd0d216f52ceb2569f39f52163ee07428bb3ae2eb7ae3b652335eb943cf2a`（只读）
- `unisa/bits.py` = `5a2cad1e71d96b776917fe476cc0b89c7d04768badc40612d11686e9f74b0671`（只读）
- `seed/unisa_image_pe.c` = `b72054efcfcf591c2a11cf7ee7b7a151dcbc7db26d5c72778099431693a5012f`
- `tests/unisaimagepecheck.sh` = `79d06ca35fcf1937cbe9643bc6041313b1494d8d89ddb977ba74e1b5c48b2d3e`
- host-cc：`cc (Debian 14.2.0-19) 14.2.0`

## 原证
- 直跑：`sh tests/unisaimagepecheck.sh`；UTC 16:58:08Z–16:58:09Z；rc=0；`unisaimagepe same 7 diff 0 neg 3/3`；日志 `research/c41-k6-15-image-pe-run.log`。
- gate：`./tests/gate.sh --suite unisa-image-pe`；rc=0；`failed 0 unverified 0`。

## 禁项自检
- 未写 src/（含 version.h）；未删 py；未 bump；未 Draft；未 Latest；未签；未改验收；未动 exec/kernel/weights/release；未改 K6-1..14 的 seed/夹具闭包；未开 ape；未切换产品路径；未跑旧全量。
