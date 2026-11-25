# 切片 NN 化深度实测与形态分析报告 (0.1.0 / 0.0.107 / 0.0.105 / 0.0.87)

一句话结论(给项目所有者): 现行 0.1.0 产品 (8,991,230 B) 中, 非瘦层原生逻辑并非都可以靠 nn 表化或字节码缩小——真正可无副作用迁成有限表(FINITE)的仅有 6 单元 (69,363 B .text, 占 102 个 x64 .o 的 2.7%, 其中仅 4 单元 57,012 B 在默认路径); 其余 76.2% (迁 VM 40.8% + 先拆分 32.7% + 0.1.0 新增 2.5%) 为带无界结构/图/堆/IO 编排的通用计算; 造成六目标映像总重达 5.84 MB (APE 占比 65%) 且 osx/win 体积达到 Linux 的 1.64 至 2.41 倍的主因, 是后端代码密度低(同源 cxmod 产品自编比 gcc -O0 大 2.57 倍、比 gcc -Os 大 4.55 倍)与 osx/win 仍走未裁剪、默认 -O1 的 whole-o 路线, 而非「逻辑未迁网」。

核心一张表(六片 x 五类字节与占比实测对照; 0.1.0 lnx/x86_64 与 lnx/arm64 取自 g2 102 个 .o 实测 ELF 节头, 其余四片按 whole-o 无逐单元 closures.tsv 标 UNMEASURED, 原表 0.0.87 与 0.0.93 基线并列):

| 目标切片 (版本) | 映像解压/原始字节 | 归并/汇总口径 | 留切片(薄层) | 迁表(FINITE) | 先拆分后迁 | 迁 VM 字节码 | 非默认外移池 U4 (与前四类重叠) | 新增未定类 (0.1.0) |
|---|---:|---|---:|---:|---:|---:|---:|---:|
| lnx/x86_64 (0.0.93 APE / 0.0.87 99 .o) | 2,379,494 B (0.0.93) | 99 .o .text 合计 2,441,257 B | 27 单元 / 515,445 B (21.1%) | 6 单元 / 48,602 B (2.0%) | 42 单元 / 837,694 B (34.3%) | 24 单元 / 1,039,516 B (42.6%) | 25 单元 (含表/拆分/VM/薄层) | 0 |
| lnx/x86_64 (0.1.0 APE / g2 102 .o) | 1,954,310 B (LOAD0 1,905,313) | 102 .o .text 合计 2,558,755 B (tl_dce 裁减 653,442 B) | 28 单元 / 540,591 B (21.1%) | 6 单元 / 69,363 B (2.7%) | 42 单元 / 836,866 B (32.7%) | 24 单元 / 1,046,280 B (40.8%) | 28 单元 / 796,071 B (31.1%) | 2 单元 / 65,655 B (2.5%) |
| lnx/arm64 (0.1.0 APE / g2 102 .o) | 2,105,806 B (LOAD0 2,057,944) | 102 .o .text 合计 3,138,672 B (tl_dce 裁减 1,080,728 B) | 28 单元 / 640,772 B (20.4%) | 6 单元 / 78,536 B (2.5%) | 42 单元 / 1,080,452 B (34.4%) | 24 单元 / 1,259,088 B (40.1%) | 28 单元 / 960,976 B (30.6%) | 2 单元 / 79,824 B (2.5%) |
| osx/x86_64 (0.1.0 APE) | 3,203,554 B (__text 3,117,419) | whole-o 无 closures.tsv | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED |
| osx/arm64 (0.1.0 APE) | 4,326,370 B (__text 4,234,764) | whole-o 无 closures.tsv | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED |
| win/x86_64 (0.1.0 g2 / APE 头) | 3,513,856 B (头 3,516,928; .text 3,455,176) | whole-o 无 closures.tsv | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED |
| win/arm64 (0.1.0 g2 / APE 隐含片) | 4,707,840 B (APE 解压 4,708,352; .text 4,649,064) | whole-o 无 closures.tsv | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED |

数据附件:
- `research/agy-h/slice-breakdown-ape.tsv`
- `research/agy-h/slice-breakdown-targets.tsv`
- `research/agy-h/slice-breakdown-units.tsv`
- `research/agy-h/slice-breakdown-categories.tsv`
- `research/agy-h/slice-breakdown-models.tsv`
- `research/agy-h/slice-breakdown-product-shape.tsv`

---

## 一、整体分解 (0.1.0 对 0.0.105 本机根 与 0.0.107 基线)

取证入口:
- 0.1.0 已公开 APE: `~/.cache/unisacc/old-product/b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f/unisacc.com` (`8,991,230 B`, sha256 `b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`), 以及同链 `~/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1/`.
- 0.0.105 本机根 APE: `./unisacc.com` (`8,842,611 B`, sha256 `934c7c0bc09a9a4d2e107b8d6bdc855553cf05f64dbb7d6c1e5fefdb42117dd1`).
- 0.0.107 APE 对照: `~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f/ape-final/unisacc-next.com` (`8,991,230 B`, sha256 `b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63`).
- 工具: `tools/com-tree.cx`, `tools/slice-audit.cx --no-gz`, `/tmp/slice-inspect/bin_audit.cx`. 全文件字节 100% 记账相符 (`SLICE_AUDIT ok=1`).

### 1.1 APE 总体段表 (逐段偏移、存储大小、解压大小、压缩率、占比)

| 段名 | 0.1.0 偏移 | 0.1.0 存储 B | 0.1.0 解压/内存 B | 压缩率 / 占 APE | 0.0.105 存储 B | 0.0.105 解压 B | 0.1.0 对 0.0.105 差 |
|---|---:|---:|---:|---|---:|---:|---:|
| 头整体 (`head: PE + DOS stub + sh`) | 0 | 3,516,928 | 3,516,928 (SizeOfImage 22,982,656) | raw 100% / 39.1% | 3,434,496 | 3,434,496 (Img 18,313,216) | +82,432 |
| Unix 片 1: `Linuxx86_64` | 3,516,928 | 425,524 | 1,954,310 | 21.8% / 4.7% | 412,524 | 1,892,302 | +13,000 (+62,008 raw) |
| 对齐零填充 1 | 3,942,452 | 12 | 12 | zero=1 | 4 | 4 | +8 |
| Unix 片 2: `Linuxaarch64\|arm64` | 3,942,464 | 408,748 | 2,105,806 | 19.4% / 4.5% | 394,879 | 2,043,798 | +13,869 (+62,008 raw) |
| 对齐零填充 2 | 4,351,212 | 4 | 4 | zero=1 | 1 | 1 | +3 |
| Unix 片 3: `Darwinx86_64` | 4,351,216 | 540,432 | 3,203,554 | 16.9% / 6.0% | 527,975 | 3,120,994 | +12,457 (+82,560 raw) |
| 对齐零填充 3 | - | 0 | 0 | - | 9 | 9 | -9 |
| Unix 片 4: `Darwinarm64` | 4,891,648 | 540,908 | 4,326,370 | 12.5% / 6.0% | 527,397 | 4,227,298 | +13,511 (+99,072 raw) |
| 对齐零填充 4 | 5,432,556 | 4 | 4 | zero=1 | 11 | 11 | -7 |
| 隐含片 5: `win/arm64` (U6 未被 sh 列入) | 5,432,560 | 562,589 | 4,708,352 | 11.9% / 6.3% | 549,060 | 4,595,712 | +13,529 (+112,640 raw) |
| 对齐零填充 5 | 5,995,149 | 3 | 3 | zero=1 | 12 | 12 | -9 |
| `cache-manager Darwin/x86_64` | 5,995,152 | 45,076 | 247,906 | 18.2% / 0.5% | 45,076 | 247,906 | 0 |
| 对齐零填充 6 | 6,040,228 | 12 | 12 | zero=1 | 12 | 12 | 0 |
| `cache-manager Darwin/arm64` | 6,040,240 | 43,278 | 280,930 | 15.4% / 0.5% | 43,278 | 280,930 | 0 |
| 对齐零填充 7 | 6,083,518 | 2 | 2 | zero=1 | 2 | 2 | 0 |
| P4 包主体 + 16 B `UNIPKG1` 尾 | 6,083,520 | 2,907,710 | body 2,907,694 | 32.3% | 2,907,875 | body 2,907,859 | -165 |
| APE 合计 | 0..8,991,230 | 8,991,230 | (含全零垫共 37 B) | 100.0% | 8,842,611 | (垫 51 B) | +148,619 |

注: `tools/com-tree.cx` 把 `win/arm64` 隐含 gzip 片 + 两份 `cache-manager` + 对齐零填充合并显示为 `(padding) 650,964 B` (0.1.0) 与 `637,451 B` (0.0.105); `tools/slice-audit.cx` 拆清后真实全零对齐仅 37 B (0.1.0) 与 51 B (0.0.105). 0.0.107 与 0.1.0 整文件字节数完全相等 (`8,991,230 B`), 仅 PE `.rsrc` 版本串、Linux x64 gzip (425,525 vs 425,524 B) 及零垫 (36 vs 37 B) 有版号重封引起的微小流差.

### 1.2 头部与 PE 逐节精确分解 (`0.1.0` vs `0.0.105`)

按 `tools/ape-pack.cx:329-420` 实装逻辑与 `bin_audit.cx` 实测:
- DOS 与 sh 启动器区域: DOS MZ 固定头 `64 B` (`e_lfanew = 2,896`), sh polyglot 启动文本区 `[64, 2,896)` 共 `2,832 B` (`dos_zero_pad = 0`, 启动脚本覆盖到 `2,896`), 后接 `PE\0\0` (4B) + COFF (20B) + PE32+ 可选头 (240B) + 5 节表 (200B) = `464 B`, 经 `224 B` 零填充对齐到 `SizeOfHeaders = 3,584 B` (7 个 512B 扇区; 比原始 `slice-win-x86_64` 的 `SizeOfHeaders = 1,024 B` 多 `2,560 B`).

| PE 节名 | 0.1.0 原始文件字节 (rawsz) | 0.1.0 内存虚拟字节 (virtsz) | 0.1.0 尾零 / RVA | 0.0.105 原始字节 | 0.0.105 内存字节 | 差异原因 |
|---|---:|---:|---|---:|---:|---|
| 头部 (DOS+sh+PE+5节表+垫) | 3,584 | 3,584 | e_lfanew=2896 | 3,584 | 3,584 | 两版相等 |
| `.text` (`0x60000020` 代码) | 3,455,488 | 3,455,176 | tail_zero=312; RVA `0x1000` | 3,374,080 | 3,373,919 | +81,408 raw (+81,257 virt): 0.0.106..107 新增 CXM_MAXU=256 / 转发桩 / 缓存逻辑 |
| `.rdata` (`0x40000040` 只读) | 1,536 | 1,256 | tail_zero=507; Import=40, IAT=216, LoadCfg=320 | 1,536 | 1,256 | 未变 (仅 KERNEL32.dll 12 符号 + cfg) |
| `.data` (`0xc0000040` 读写) | 55,296 | 19,508,224 | tail_zero=460; 未初始化 BSS 占 19,452,928 B | 54,272 | 14,920,904 | raw +1,024 B; virt +4,587,320 B (14.92 MB -> 19.51 MB, 主要因 `CXM_MAXU` 128->256 使 `cxmod` 静态数组翻倍) |
| `.reloc` (`0x42000040`) | 512 | 12 | tail_zero=502 | 512 | 12 | 未变 (仅单条 12B 空/最小重定位块) |
| `.rsrc` (`0x40000040` 资源) | 512 | 504 | tail_zero=8; VS_VERSION_INFO `"0.1.0"` | 512 | 512 | `ape-pack` 注入 `VS_VERSION_INFO`; 原始 `slice-win-x86_64` 无此节 (3,513,856 + 2,560 hdr扩 + 512 rsrc = 3,516,928) |
| 头合计 | 3,516,928 | SizeOfImage=22,982,656 | overlay=5,474,302 | 3,434,496 | SizeOfImage=18,313,216 | +82,432 raw (+4,669,440 virt) |

### 1.3 Darwin `cache-manager` 与 P4 包结构 (`0.1.0` vs `0.0.105`)

- 两份 `cache-manager` 在 `0.1.0` / `0.0.107` / `0.0.105` 中逐字节不变:
  - `Darwin/x86_64`: 文件偏移 `5,995,152`, 压缩 `45,076 B`, 解压 `247,906 B` (18.2%). 结构: `__TEXT` 229,376 (`__text` 225,948), `__DATA` 16,384 (`__data` 1,598 / `__bss` 915,714), `__LINKEDIT` 2,146 (`LC_CODE_SIGNATURE` 2,066 B).
  - `Darwin/arm64`: 文件偏移 `6,040,240`, 压缩 `43,278 B`, 解压 `280,930 B` (15.4%). 结构: `__TEXT` 262,144 (`__text` 252,732), `__DATA` 16,384 (`__data` 1,598 / `__bss` 915,714), `__LINKEDIT` 2,402 (`LC_CODE_SIGNATURE` 2,322 B).
- P4 模型包 (`2,907,710 B`, 0.1.0; `2,907,875 B`, 0.0.105):
  - `D` 阶段路由表: `1,823` 行, `104,052 B` (占 APE 1.1%).
  - `F` 资源表 (`258,454 B`, 78 项于 0.1.0; `258,201 B`, 77 项于 0.0.105): `@kernel/{x86_64,arm64}` 2 项 `15,515 B` (raw, 7,782 + 7,733), `@predefines/` 6 项 `432 B`, `@hdr/` 70 项 `240,492 B` (最大 `@hdr/cx-flat.h` 108,977 B 已 deflate). 0.1.0 较 0.0.105 多 1 个头文件资源 `@hdr/sys/_ffi_forward.h` (3,485 B).
  - `M` 模型区 (`2,545,173 B` 含 40 个 `M` 文本头 1,086 B; 载荷 `2,544,087 B`; 全 40 模型明细分见 `slice-breakdown-models.tsv`):
    - 7 个热路径模型未压缩原样存放 (`codec=0`, 对应 `tools/release-build.cx:1480` `rb_release_models` 对 `lnx/x86_64/image/O1` 七阶段去解压以省启动时延): 模型 0 (`e2`, 245,750 B)、1 (`e1`, 270,530 B)、4 (`e3`, 624,991 B; 0.0.105 为 625,590 B)、6 (`e4 O1`, 8,666 B)、8 (`prune`, 25,627 B)、12 (`elf lnx/x86_64`, 71,746 B)、13 (`lower lnx/x86_64`, 193,192 B), 合计 `1,440,502 B` (0.0.105 为 `1,441,101 B`), 占全部 40 模型载荷的 56.6%.
    - 其余 33 个模型经 deflate 压缩 (`codec=1`): 存储共 `1,103,585 B`, 展开原始大小 `4,003,009 B` (40 模型未压缩总原始载荷 = `5,443,511 B`, 含 7 个未压模型 `1,440,502 B`).

---

## 二、六目标映像对比 (为何相差 1.00x 到 2.41x)

以 0.1.0 链 `fixedpoint/g2` 六个实际产物切片及 PE/Mach-O/ELF 解析为准 (全数据列在 `slice-breakdown-targets.tsv`):

| 指标 | lnx/x86_64 | lnx/arm64 | osx/x86_64 | win/x86_64 (g2) | osx/arm64 | win/arm64 (g2) |
|---|---:|---:|---:|---:|---:|---:|
| 切片未压缩 B (相对 lnx/x64) | 1,954,310 (1.000x) | 2,105,806 (1.078x) | 3,203,554 (1.639x) | 3,513,856 (1.798x) | 4,326,370 (2.214x) | 4,707,840 (2.409x) |
| 构造路由与优化档 | `objects` `-O2`(96u)/`-O1`(6u) + `tl_dce` | `objects` `-O2`(96u)/`-O1`(6u) + `tl_dce` | `whole-o` (`multi/image/O1`) | `whole-o` (`multi/image/O1`) | `whole-o` (`multi/image/O1`) | `whole-o` (`multi/image/O1`) |
| 代码+只读段/节 (B) | 1,905,313 (LOAD0 R E) | 2,057,944 (LOAD0 R E) | 3,117,419 (`__text`) | 3,455,176 (`.text`) + 1,256 (`.rdata`) | 4,234,764 (`__text`) | 4,649,064 (`.text`) + 1,256 (`.rdata`) |
| 已初始化 RW 数据 (B) | 45,574 (LOAD1 FileSiz) | 45,518 (LOAD1 FileSiz) | 45,164 (`__data`) | 55,296 (`.data` raw) | 45,160 (`__data`) | 55,296 (`.data` raw) |
| 运行期 RW 总内存 / BSS (B) | 19,081,048 / 19,035,474 | 19,081,000 / 19,035,482 | 19,185,664 / 19,138,404 | 19,508,224 / 19,452,928 | 19,185,664 / 19,138,416 | 19,508,232 / 19,452,936 |
| 段页对齐与格式头开销 (B) | 3,423 (LOAD 4K 间隙) | 2,344 (LOAD 4K 间隙) | 15,913 (`__TEXT`/`__DATA` 对齐及头) | 1,840 (Hdr=1024 + 节对齐pad) | 12,684 (16K/4K 节对齐) | 1,916 (Hdr=1024 + 节对齐pad) |
| 导入 / 重定位 / 签名 (B) | 0 (纯内核 syscall, stripped无节表) | 0 (纯内核 syscall, stripped无节表) | 25,058 (`__LINKEDIT`: sig 24,978, bind 58, sym 8) | 512 `.reloc`(virt 12) + Import(40)+IAT(216)+Cfg(320) | 33,762 (`__LINKEDIT`: sig 33,682, bind 58, sym 8) | 512 `.reloc`(virt 12) + Import(40)+IAT(216)+Cfg(320) |
| 逐单元 `.o` / `closures.tsv` | 有 (102 个 `.o`) | 有 (102 个 `.o`) | UNMEASURED | UNMEASURED | UNMEASURED | UNMEASURED |

六目标体积差一倍以上及关键现象的实证归因:

1. 产品后端密度是最底层的决定因素:
   - 外部并入证据 (`backend-density-probe.md` 实测): 同源 `src/cxmod.c` 用 `gcc -Os` 仅 `52,477 B` / `gcc -O0` 为 `75,617 B`, 而产品自编 `cxmod.o` 达 `194,024 B` (`-O1`, 对 `gcc -O0` 为 2.57x、对 `-Os` 为 4.55x); 五个同名小函数对照指令数达 1.79x 至 3.08x (`cxm_backc` 80 对 26 条), 主因每函数 6 寄存器参搬迁 + 空跳 `jmp .+0` + 自建 `r9` 帧 + 参数全 spill + 局部内存往返 + 常量与地址重算 + 无窥孔.
   - 本次直查 `0.1.0` `g2/lnx-x86_64/m1.o` (`text.cx` `-O2`) `objdump -d` 亦直接见此模式: 不仅每处写回 `-0x8(%r9)` / `-0x10(%r9)`，还在地址 0x45..0x61 连续生成两次冗余栈往返符号扩展 (`sub $8,%rsp; mov %eax,(%rsp); movslq (%rsp),%rax; add $8,%rsp`). 代码密度基础放大后, 再叠加路由与目标差异倍增.

2. Linux 为何显著小于 macOS 与 Windows (`1.95 MB` 对 `3.20 / 3.51 / 4.33 / 4.71 MB`):
   - 并非格式头/签名/导入表膨胀: macOS `__LINKEDIT` 只有 `25 KB` (x64) / `33.8 KB` (arm64, 按 4 KB 页每页 32 B SHA-256 哈希的 ad-hoc `LC_CODE_SIGNATURE` 随 `__TEXT` 线性增长); Windows `.reloc` 仅 `512 B` (有效 12 B), `.rdata` 仅 `1,256 B` (仅静态引 1 个 `KERNEL32.dll` 共 12 个基础 API 桩, 运行时其余 API 走 `_ux_sym`), 页对齐损耗仅 `1.8 KB` 到 `15.9 KB`. 97% 到 99% 的体积差异全在 `.text` / `__text` 代码段本身.
   - 已证路由与优化差异 (`tools/entry-build.cx` 对 `tools/release-build.cx`):
     - `lnx/x86_64` 与 `lnx/arm64` 走 `objects` 路线: 102 个单元分别编为 `.o` (96 单元 `-O2` 含 `model 7 e4` 29.9 KB, 6 单元 `-O1`), 再由 `cx/compiler/tapelink.cx` (`tl_dce`) 做全程序符号可达性裁剪. 实测 `102` 个 `.o` `.text` 累加和本为 `2,558,755 B` (x64) 与 `3,138,672 B` (arm64)——经 `tl_dce` 分别砍掉 `653,442 B` (-25.5%) 与 `1,080,728 B` (-34.4%) 才降为 `1,905,313 B` 与 `2,057,944 B`. 其中仅每个 `.o` 都带有的隐含前奏/运行库 `_u_st_` 符号组 (`hostnative.o` 实测 `3,392 B` x64 / `4,048 B` arm64) 在 `102` 个 `.o` 中就占约 `346 KB` / `413 KB`.
     - `osx` 与 `win` 四格走 `whole-o` 路线 (`tools/release-build.cx:1275` `rb_whole`): 将全部 44 个根源文件一次传给 `-b TARGET -o IMAGE`, 未传 `-O2`, 按 `cx/driver/cli.cx:436` 默认落入 `-O1` (`multi/image/O1` 用简版 `model 6 e4` 8.7 KB), 且不走 `.o` 级 `tapelink` `tl_dce` (即 `backend-density-plan.md` 指出的 `构造档位 -O1 对 -O2、whole-o 无 tl_dce` 主因). 在 `whole-o -O1` 下, `arm64 / x86_64` 代码段比为 `1.358x` (`osx`: 4.23M / 3.12M) 与 `1.346x` (`win`: 4.65M / 3.46M), 与未裁剪 `lnx` `.o` 总和之比 `1.227x` (`3.14M / 2.56M`) 接近, 而 `tl_dce` 在 `lnx/arm64` 上多删了 427 KB 把 `arm64/x64` 压至 `1.080x`. Windows 另在调用点插入 Win64/ARM64 ABI 栈帧保护与 32 B shadow space (`0x140001000` 实见 `mov %rsp,%rbx; and $-16,%rsp; sub $0x20,%rsp ...`), 使 `win` 较 `osx` 再多 `338 KB`(x64) 至 `414 KB`(arm64). 四格中 `-O1/-O2` 与死代码未剪的各自精确分量暂标 `UNMEASURED`.

3. `.data` 内存 14.9 MB (0.0.105) 与 ELF RW 内存 19.1 MB / PE `.data` 19.5 MB (0.1.0) 是什么:
   - 遍历 `102` 个 `.o` 实测总 `.bss = 19,052,693 B`, 与 `entry.elf` RW `MemSiz - FileSiz = 19,035,474 B` (~19.1 MB)、macOS `__bss = 19,138,404 B`、Windows `0.1.0` `.data virtsz = 19,508,224 B` (0.0.105 为 `14,920,904 B`) 精准吻合:
   - (1) `cxmod.o` 独占 `13,375,734 B` (.bss 的 70.2%): `cx/cx0/plan.cx:83-140` 静态全局数组 (`cxm_renpool[256x48][512]` = 6.29 MB、`cxm_usym`+`cxm_ualias` = 1.57 MB、`cxm_src[1MB]`、`cxm_cmp/out/ift` 等). `0.0.105` 为 `CXM_MAXU=128`, `0.1.0` 合入 `CXM_MAXU=256` (`9199f8dbbf`), `cxm_` 前缀静态数组净增约 `4.58 MB`, 与 Windows 虚存 `14.92 MB -> 19.51 MB` (+4.59 MB) 完全一致.
   - (2) 每单元 37 KB 隐含前奏/运行库 BSS (约 3.80 MB): 空单元 `hostnative.o` 即含 `.bss = 37,304 B` (`g__unisa_envk/envv` 与 `_u_st_` 运行库缓冲); 102 个单元乘以 37 KB 约 `3.80 MB`.
   - (3) `driveface.o` `1,348,053 B`: `src/driveface.cx:9-10` 静态 `dv_in[262,144]` + `dv_obj[1,048,576]`. 其余 `posix.o` 194 KB、`asmtext.o` 184 KB、`fwdstub.o` 169 KB、`drive.o` 104 KB 为余项.

4. 各目标独有原生层与不可测缺口 (`build/pkg-gaps.tsv`):
   - Linux 依赖 `__syscall6` + `memfd` + `/proc/self/exe` + `AF_ALG`; macOS 依赖 `libSystem.B.dylib` (`dyld` 绑定) + Darwin `sysnums` + `tmpfile`; Windows 依赖 `KERNEL32.dll` (12 静态符号 + `_ux_sym`/`_ux_call` 动态查找表 + 480 B ABI 栈桥).
   - 不可测缺口: `osx` 与 `win` 四格走 `whole-o` 无逐单元 `.o` 与 `closures.tsv`, 其逐单元 `.text` 与 `-O1` 对 `tl_dce` 分量记为 `UNMEASURED`, 将记入 `build/pkg-gaps.tsv` (待 P0/候选 17 后提交, 不碰在飞窗口).

---

## 三、单元级对照 (0.1.0 `lnx/x86_64` 对 `lnx/arm64` 全 102 单元与功能归并)

基于 `0.1.0` `fixedpoint/g2/lnx-x86_64` 与 `lnx-arm64` 共 `102` 个 `.o` 文件 (`lnx-x86_64.closures.tsv` 58 个 `m0..m57` + 44 个具名 `.o`), 按规范源码路径与 `plans/slice-units.tsv` / `plans/unit-text.tsv` (0.0.87 `99` 单元) 对齐 (已按源码路径重对齐 0.1.0 发生变化的 `mN.o` 编号; 全 102 行见 `slice-breakdown-units.tsv`):

### 3.1 按九大功能类归并 (102 单元 = 2,558,755 B x64 / 3,138,672 B arm64)

| 功能类 | 单元数 | 0.1.0 x64 .text | 占 x64 比重 | 0.1.0 arm64 .text | 占 arm64 比重 | 0.0.87 x64 基线 | 非默认 `NO` 数 / x64 B | 主要迁移类别 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| 1. driver 与 route (`drive`, `route`, `cli`, `stages`, `objlink` 等) | 27 | 640,901 | 25.0% | 809,516 | 25.7% | 623,417 | 9 / 272,804 | 先拆分 21 / VM 3 / 表 3 |
| 2. host 与 sys 薄层 (`host`, `cx/sys/`, `input`) | 20 | 449,241 | 17.5% | 531,764 | 16.9% | 439,887 | 2 / 34,596 | 留切片 19 / VM 1 (`sha`) |
| 3. runexec 与 source_frame 运行/缓存层 | 14 | 416,776 | 16.2% | 548,968 | 17.4% | 413,048 | 2 / 8,278 | 先拆分 9 / VM 4 / 表 1 |
| 4. 磁带与转发层 (`tapegate`, `tapelink`, `tapebin`, `fwd`, `stream`) | 16 | 362,207 | 14.1% | 435,584 | 13.8% | 329,732 | 10 / 240,474 | 迁 VM 8 / 先拆分 6 / 表 1 / 新增 1 (`fwdreplay`) |
| 5. 编译辅助与文本层 (`asmtext`, `rtsharedname`, `cellname`, `cxscratch` 等) | 7 | 282,887 | 11.0% | 326,100 | 10.3% | 247,279 | 2 / 218,826 | 迁 VM 1 (`asmtext`) / 表 2 / 先拆分 3 / 新增 1 (`cxscratch`) |
| 6. cxmod 模块规划 (`cxmod.o`) | 1 | 194,024 | 7.5% | 240,828 | 7.6% | 191,885 | 0 / 0 | 迁 VM (或仅判定入表) |
| 7. 包解码与模型装载 (`codec`, `modeldecode`, `package`, `packmodel`) | 8 | 106,065 | 4.1% | 121,072 | 3.8% | 105,117 | 0 / 0 | 自举环应留片 (原表标 VM 4 / 拆分 4) |
| 8. 字符串与内存工具库 (`cx/mem/`, `cx/text/`) | 7 | 63,038 | 2.4% | 72,852 | 2.3% | 47,249 | 3 / 21,093 | 全留切片 (含 0.1.0 新增 `pinscan.o` 15,969 B) |
| 9. 执行器与状态转移 (`executor`, `transition`) | 2 | 43,616 | 1.7% | 51,988 | 1.6% | 43,643 | 0 / 0 | 留切片 (`executor` M10b / `transition` 汇编核回退壳) |
| 合计 | 102 | 2,558,755 | 100.0% | 3,138,672 | 100.0% | 2,441,257 | 28 / 796,071 | 含 `tl_dce` 前全部 102 个 `.o` |

### 3.2 按迁移策略五类归并 + `2.56 MB` 大于 `1.95 MB` 口径校准

| 迁移类别 | 单元数 | 0.1.0 x64 .text (占比) | 0.1.0 arm64 .text (占比) | 0.0.87 基线 x64 | 非默认 `NO` 单元数 / x64 B |
|---|---:|---:|---:|---:|---:|
| 留切片 (薄层) | 28 | 540,591 (21.1%) | 640,772 (20.4%) | 515,445 | 5 / 55,689 |
| 迁表 (FINITE) | 6 | 69,363 (2.7%) | 78,536 (2.5%) | 48,602 | 2 / 12,351 |
| 先拆分后迁 | 42 | 836,866 (32.7%) | 1,080,452 (34.4%) | 837,694 | 8 / 178,090 |
| 迁 VM 字节码 | 24 | 1,046,280 (40.8%) | 1,259,088 (40.1%) | 1,039,516 | 13 / 549,941 |
| 0.1.0 新增未归类 (`fwdreplay`, `cxscratch`) | 2 | 65,655 (2.5%) | 79,824 (2.5%) | 0 | 0 / 0 (均为 `YES`) |
| 合计 (全 .o 未链接前) | 102 | 2,558,755 (100%) | 3,138,672 (100%) | 2,441,257 | 28 / 796,071 (31.1%) |

为何 `plans/unit-text.tsv` 合计 `2.44 MB` (0.1.0 为 `2.56 MB`) 显著大于最终解压切片 `1.89 MB` (`0.1.0` 为 `1.95 MB`):
1. `plans/unit-text.tsv` 统计的是链接前逐单元 `.o` 的 `.text` 节之和 (`0.0.87` 99 个 `.o` 共 `2,441,257 B`; `0.1.0` 102 个 `.o` 共 `2,558,755 B`).
2. 编译产物向每一个 `.o` 都注入了约 `3,392 B` (x64) / `4,048 B` (arm64) 的隐含前奏/运行库 (`_unisa_fd`, `_u_st_raw`, `_u_st_wflush`, `_u_st_flushall`, `exit`, `_u_st_slot` —— 已在 4 行空占位文件 `hostnative.o` 用 `readelf -s` 实证实测), `102` 个 `.o` 仅此项就重复计入 `~346 KB` (x64) / `~413 KB` (arm64).
3. `entry-build.cx` 调用 `tapelink.cx` (`tl_dce`) 做跨单元分组死代码消除, 将 `2,558,755 B` 裁去 `653,442 B` (-25.5%) 得到 `entry.elf` `LOAD 0 = 1,905,313 B` (`arm64`: `3,138,672 -> 2,057,944 B`, 裁去 `1,080,728 B`, -34.4%).
4. 结论: 任何从 `plans/unit-text.tsv` 或未链接 `.o` 外推全片减量的模型都必须先扣除每单元 ~3.4 KB 重复前奏与死代码, 不能把 `0.745` (`1,905,313 / 2,558,755`) 线性乘到单个单元上.

---

## 四、nn 化经济学 (已 nn 化 40 个包内模型的体积账与对比)

依据 `0.1.0` 包结构实测 (`slice-breakdown-models.tsv` 与 `/tmp/slice-inspect/model_econ.out`):

| 模型族 | 模型数 | 0.1.0 未压缩模型 B (`0_raw`) | 0.1.0 存储(含 deflate) B | 存储占模型区比重 | 等价原生对照基线 | 原生:模型比例 (说明) |
|---|---:|---:|---:|---:|---|---|
| `e3` (普通 1 + warn 1) | 2 | 1,256,553 | 794,860 (M4 未压 624,991) | 31.2% | 无独立迁移前 C 原生切片对照 (C 参考原型合计 `exec/c/` 全目录仅 `313,385 B` C 源码) | UNMEASURED (就语义等价体量看, 纯状态转移网原始字节远高于手写 C `.o`) |
| `lower` (6 目标 x 2) | 12 | 2,041,370 | 665,815 (M13 x64 未压 193,192) | 26.1% | 同按目标分化 12 份; 无原生对照 | UNMEASURED (按目标复制 12 份, 不具备「单份网替六片」优势) |
| `elf` (6 目标 x 2) | 12 | 1,035,475 | 362,467 (M12 x64 未压 71,746) | 14.2% | 同按目标分化 12 份; 与原生 `asmtext` (214,880) 或 `tapelink` (50,306) 职责不同 | UNMEASURED (单目标 `elf` raw ~75K..102K, 存储 22K..29K/压缩项) |
| `e2` + `tokenpp` | 3 | 570,691 | 335,250 (M0 未压 245,750) | 13.1% | 预处理模型无原生对照 | UNMEASURED |
| `e1` + `tokenlex` | 2 | 318,850 | 282,802 (M1 未压 270,530) | 11.1% | 词法网无原生对照 | UNMEASURED |
| `e4` + `prune` | 3 | 64,152 | 47,366 (M6/M8 未压 34,293) | 1.8% | 无迁移前切片对照 | UNMEASURED |
| `tapecheck` | 1 | 54,966 | 19,723 (全压) | 0.7% | 唯一近似参考: `tapegate.o` (`75,395 B` x64 / `84,112 B` arm64) | 原始比 `75,395 / 54,966 = 1.37:1`, 压缩后 1 份 `19.7 KB` 对六片估 ~`90 KB` 有净省; 但与 `tapegate` 逻辑重叠度未普查 |
| 元信息 (`units`/`nativeabi`/`symbols`/`contract`/`ownership`) | 5 | 101,454 | 35,804 (全压) | 1.4% | 无原生对照 | UNMEASURED |
| 合计 (40 模型载荷) | 40 | 5,443,511 | 2,544,087 (+头 1,086=2,545,173) | 100.0% | 缺少任何同功能「迁前/迁后」配对收据 | 总体: 未压缩模型原始字节 (`5.44 MB`) 约为 `lnx/x64` 整片 (`1.95 MB`) 的 2.79 倍 |

经济学核心判断:
1. 按单个形态计, 状态网表原始展开字节 (`5.44 MB`) 通常大于甚至远大于等价原生机器码(哪怕考虑到 `0.1.0` 产品后端代码密度偏低, `e3` 单网即 `625 KB`, `e1`+`e2` `516 KB`, 单套目标 `lower`+`elf` `~265..300 KB`); 唯一能让包字节低于六目标总原生字节的条件是: (a) 跨六目标共享单一份网(而现包内占存储 40.4% 的 24 个 `lower`/`elf` 模型仍然按 6 目标 x 2 复制了 12 套), 以及 (b) 使用 deflate 压缩(现 `rb_release_models` 为了启动延时将 7 个热模型保留 `1,440,502 B` 原样未压; 若全压至约 27% 同类压缩率, 可回落约 1.05 MB, 否则 40 模型存储占 APE 28.3% 已超过全部四个 Unix gzip 切片之和 `1,915,612 B`).
2. `m14` `rtsharedname` 试点文档明确记全部收据为 `UNMEASURED`、`batch1` 亦全部 `UNMEASURED`, 因此不存在已证实的正向 `d_pkg < d_slice` 配对实测.

---

## 五、逐项可 nn 化性清单 (`0.1.0 lnx/x86_64` 最大 30 个单元)

注: 预估净节省列以实测 APE 口径区分为「按 `3.11x` (`6,083,520 / 1,954,310` 即实测六个原生载荷在 APE 中的存储字节比 `lnx` 解压) x `0.745` (`tl_dce` 后实际进片比例估) = 约 2.32x APE 字节」与「未扣模型/源外置增量的总上界」; 对 `osx`/`win` 因无逐单元 `closures.tsv` 与 `whole-o` 倍率实证差异, 按全片均值 `2.32x` 标为参考估计(实质含 `UNMEASURED` 分量), 不作为承诺值.

| # | 单元 (0.1.0) | x64 .text | arm64 .text | 功能 | 有界性 | 默认路径 (`default_path`) | 迁移分类与阻塞 | 预期 APE 减量估计 (未扣包增量) |
|---:|---|---:|---:|---|---|---|---|---|
| 1 | `m48.o` `asmtext.cx` | 214,880 | 246,536 | ELF 对象与 GNU 汇编文本互转 | UNBOUNDED (8MB 符号池) | NO (`-S`/dis) | U4 外移 (不入流); 阻塞: 与 enc 网格式不同 | ~498 KB (U4 整块移出) |
| 2 | `cxmod.o` `cxmod.cx` | 194,024 | 240,828 | CX0 pragma/模块闭合/CXIF | UNBOUNDED (图+递归) | YES | 只做判定/文案入表; 阻塞: 自举依赖 C 投影与热路径 | 整体迁不可行 (判定表 ~0 KB) |
| 3 | `m43.o` `source_frame.cx` | 95,045 | 105,884 | 源帧编码/账本/CAP 小整数判定 | UNBOUNDED + 小 FINITE 混合 | YES | 先抽有限小判定成稠密表; 阻塞: 粒度过细走 VM 必亏 | 若拆表约 20..80 KB |
| 4 | `host.o` `host.cx` | 89,594 | 104,912 | 容器定位/mmap/exec/forward | SYS-IO (host) | YES | 留切片 (薄层); 阻塞: ABI/syscall 本体 | 0 |
| 5 | `tapegate.o` `tapegate.cx` | 75,395 | 84,112 | 带子二进制校验与 LEB 解码 | UNBOUNDED (记录流) | NO (.tapebin/stream) | U4 外移 或 核与 `tapecheck` 重叠后并流 | ~175 KB (U4) |
| 6 | `m0.o` `drive.cx` | 65,722 | 94,272 | 主驱动编排 `dv_main` | MIXED | YES | 只抽路由判定入表; 阻塞: 全流程副作用与 IO 编排 | 约 10..15 KB (局部) |
| 7 | `runexec.o` `runexec.cx` | 59,274 | 87,968 | 预算/拒绝/子进程运行 | SYS-IO | YES | 留壳; 仅 `st_gap` 7 路入表; 阻塞: 0.1.3 进程写域 | 约 6 KB (单一源为主) |
| 8 | `driveroute.o` `driveroute.cx` | 58,924 | 73,436 | 前向重放合并与 lower 路线 | SYS-IO (堆 2) | YES | 只把 level 分叉入路线表; 阻塞: 堆/文件 IO | 约 12..14 KB (局部) |
| 9 | `driveface.o` `driveface.cx` | 58,534 | 71,528 | `as`/`ar`/`nm` 子命令门面 | MIXED | NO (`-S`/ar/nm) | 随 `asmtext` 走 U4 外移; 阻塞: 工具链依赖 | ~136 KB (U4) |
| 10 | `m15.o` `rcident.cx` | 52,167 | 69,936 | 运行缓存身份哈希与读集 | MIXED (sha+IO) | YES | 身份字段表入事实; 阻塞: 全文件 IO 与哈希 | ~0 |
| 11 | `tapelink.o` `tapelink.cx` | 50,306 | 62,256 | 多单元带子链接与 DCE | UNBOUNDED (开放符号) | NO (-c 链接) | 留后续批 U4 (A2 依赖族); 阻塞: 自链 `entry-build` 需要它 | ~117 KB (若能 U4) |
| 12 | `m3.o` `runcache.cx` | 48,553 | 65,588 | 运行缓存读写命中入口 | UNBOUNDED (IO) | YES | 不迁 (热 7ms 启动关键路径) | 0 |
| 13 | `driveunits.o` `driveunits.cx` | 47,163 | 64,548 | 多单元循环编排 | MIXED | YES | 不迁 (编排壳) | 0 |
| 14 | `m17.o` `unitcache.cx` | 46,560 | 63,408 | 单元对象缓存 (堆 6) | UNBOUNDED (堆/IO) | YES | 不迁 (文件/内存缓存) | 0 |
| 15 | `m34.o` `posix.cx` | 46,143 | 53,284 | POSIX syscall 封装 (`fork`/`termios`) | SYS-IO (host) | YES | 留切片 (薄层) | 0 |
| 16 | `m30.o` `objlink.cx` | 45,913 | 56,904 | split 模式下 ELF 对象合并 | UNBOUNDED (堆/IO) | NO (RUN_SPLIT) | U4 外移 (A2 次批) | ~106 KB (U4) |
| 17 | `stages.o` `stages.cx` | 42,981 | 52,152 | 阶段装载与属性交换 | MIXED (堆 1) | YES | 仅 `st_key`/`st_rt_shared` 小判定入表 | ~12..15 KB |
| 18 | `drivelink.o` `drivelink.cx` | 38,457 | 47,400 | `.o`/`.a` 与 `.tape` 链接门面 | MIXED (IO) | NO (.o/.a/.tape) | U4 外移 (A2 族) | ~89 KB (U4) |
| 19 | `executor.o` `executor.cx` | 37,301 | 45,008 | 模型转 CoreModel 适配与调起内核 | UNBOUNDED (自举环) | YES | 留切片 (`M10b` 对象, 跑字节码的宿主) | 0 |
| 20 | `m29.o` `unitobj.cx` | 36,816 | 44,508 | split 单单元 `.o` 包装 | UNBOUNDED (IO) | NO (RUN_SPLIT) | U4 外移 | ~85 KB (U4) |
| 21 | `m41.o` `runlaunch.cx` | 36,493 | 43,232 | 临时文件与平台 exec 启动 | SYS-IO (host) | YES | 留切片 (薄层) | 0 |
| 22 | `m42.o` `cxscratch.cx` | 35,284 | 42,792 | CX0 临时文件租约与句柄追踪 | MIXED (0.1.0 新增) | YES | 先拆分或留 IO 壳 (IO/fd 操作为主) | ~0..10 KB (UNMEASURED) |
| 23 | `m16.o` `modelcache.cx` | 35,278 | 52,256 | 模型映射缓存、锁与清扫 | MIXED (IO) | YES | 配额常数入表, IO 主体不迁 | ~0 |
| 24 | `hostout.o` `hostout.cx` | 31,387 | 37,116 | 宿主输出分配与写入 | SYS-IO (host) | YES | 留切片 (薄层) | 0 |
| 25 | `m28.o` `proc.cx` | 31,204 | 38,568 | 子进程/管道/kill | SYS-IO (host) | NO (仅 runsplit/unitobj) | 留切片 (但若随 `runsplit` U4 则不再入片闭环) | ~72 KB (随 U4 间接脱链) |
| 26 | `m13.o` `fs.cx` | 31,165 | 38,552 | 文件/目录/stat/realpath | SYS-IO (host) | YES | 留切片 (薄层) | 0 |
| 27 | `m9.o` `runsplit.cx` | 30,855 | 39,028 | `UNISACC_RUN_SPLIT` 编排 | UNBOUNDED (IO) | NO (RUN_SPLIT) | U4 外移 | ~72 KB (U4) |
| 28 | `m38.o` `fwdsig.cx` | 30,586 | 35,496 | 前向签名解码与未定义诊断 | MIXED (堆 6) | YES | 诊断文案入表; 解码流归 0.1.4/U2 后定 | ~15..20 KB |
| 29 | `m10.o` `fwdreplay.cx` | 30,371 | 37,032 | 前向运行缓存重放入口 | MIXED (0.1.0 新增) | YES | 先拆分/不迁 (与 `driveroute`/`runcache` 紧耦合) | ~0..10 KB (UNMEASURED) |
| 30 | `m6.o` `tapebin.cx` | 30,158 | 37,572 | `--tapebin -S` 二进制带子编码 | UNBOUNDED (线性编码) | NO (`--tapebin -S`) | U4 外移先导候选项 | ~70 KB (U4) |

Top 30 合计: `1,636,053 B` (占 102 个 x64 `.o` 总和 `2,558,755 B` 的 63.9%); 其中明确 `NO` (U4 可外移) 在 Top 30 中有 10 项共 `606,133 B`, 薄层 `留切片`/`自举` 7 项 `292,299 B`, 其余 13 项 `YES` 非薄层仅 `cxmod`/`source_frame`/`driver`/`cache` 等通用计算.

---

## 六、薄层的真实下限 (28 单元 `540,591 B` 逐一核对真 OS 绑定与可剥离项)

先按源码核对 `cx/sys`、`cx/mem`、`cx/text` 与 `cx/driver/{input,output}` 全部单元的真实依赖边界:

### 6.1 薄层边界核对: 真 OS/libc/syscall/asm/host ABI 对 纯便携逻辑

1. 真依赖 `libc`/`syscall`/`asm`/`host ABI` 与自举必需层 (`21` 个核心薄层单元):
   - `host.o` (89,594)、`m34.o cx/sys/posix.cx` (46,143)、`m41.o cx/sys/runlaunch.cx` (36,493)、`hostout.o` (31,387)、`m28.o cx/sys/proc.cx` (31,204)、`m13.o cx/sys/fs.cx` (31,165)、`m52.o cx/sys/kernel.cx` (29,410)、`m20.o cx/sys/io.cx` (26,366)、`m19.o cx/sys/time.cx` (26,181)、`hostres.o` (16,419)、`m40.o cx/sys/dl.cx` (11,666)、`m4.o cx/sys/env.cx` (11,187)、`m33.o cx/sys/sysinfo.cx` (10,399)、`m8.o cx/sys/hostid.cx` (8,826)、`m32.o cx/sys/sig.cx` (8,059)、`input.o cx/driver/input.cx` (7,490)、`m39.o cx/sys/mem.cx` (5,164)、`m46.o cx/sys/sysnums.cx` (3,392)、`executor.o` (37,301)、`transition.o` (6,315, 仅 `CoreModel.packed==NULL` 回退用)、`hostnative.o` (3,392, 空占位 0 符号, `tl_dce` 应裁除).
   - 合计上述 21 单元: x64 `.o` `.text = 477,553 B` (含 `21 x 3,392 = 71,232 B` 重复前奏, 净代码约 `406,321 B`).
   - 另须加自举环留片(包解码与模型装载, `8` 单元): `codec.o` (19,750)、`modeldecode.o` (26,498)、`package_decode.o` (18,361)、`packmodel.o` (12,660)、`packagefooter.o` (11,135)、`rescache.o` (8,204)、`model_lifetime.o` (7,953)、`package_lifetime.o` (5,473) — 原 M10a 误把其中 4 单元标为「迁 VM」与「先拆分」, 但执行器存在前就必须解码 P4/deflate/模型结构, 8 单元合计 `110,034 B` (净非前奏约 `82,898 B`).

2. 挂在薄层名录但实为纯算法/无 syscall 的「本可迁或属工具库」单元 (`7` 个 `+` `sha.cx`):
   - `m1.o cx/text/text.cx` (11,125)、`m31.o cx/text/pinscan.cx` (15,969)、`m2.o cx/mem/heap.cx` (8,810)、`m11.o cx/mem/bytes.cx` (6,041)、`m36.o cx/mem/grow.cx` (9,103)、`m37.o cx/mem/slice.cx` (8,598)、`m47.o cx/mem/buftype.cx` (3,392): 源码复核 0 条 syscall / `#ifdef`; 但其中 `text`、`heap`、`bytes` 被 `host`/`executor` 自身依赖(仍须留片作最小运行库, 且后三者 `grow`/`slice`/`buftype` 仅服务非默认 `stream`/`pipe`, 可随 U4 脱链共 `21,093 B`).
   - `m14.o cx/sys/sha.cx` (15,304): Linux x86_64 可走 `AF_ALG` socket 硬件路, 其余目标全靠软件 SHA-256 轮函数(已在 M10a 设为实验 1 候选项); 若作为启动自验/缓存身份仍留在片内约占用净 `~11,912 B`.
   - 另外 `cx/sys/` 中虽在仓内有 `errkind.cx`、`refusals.cx`、`reslive.cx`、`signalnums.cx`、`sigwatch.cx`, 但 0.1.0 驱动闭包并未使用或未编入独立 `.o`.

### 6.2 `lnx/x86_64` 与六目标最小映像下限估算

以 `0.1.0` `lnx/x86_64` 实测为准 (`540,591 B` 28 薄层 + `110,034 B` 8 解码层 + ~`70 KB` IO 调度壳如 `runexec`/`rcident`/`modelcache`/`runcache` 的不可拆系统调用外壳 + ~`45.6 KB` `.data` + ELF 头对齐 ~`5 KB`):
- 去重前 .o 口径总和: `540,591 - 55,689` (非默认薄层 `proc/grow/slice/buftype/hostnative` 且含 `sha`? 若留 `sha` 加 `15,304`, 扣 5 项 NO 留 `500,206`) + `110,034` (解码) + `~70,000` (IO 壳) = `~680 KB` (.text) (对应 `0.0.87` 估算 ~`710 KB`).
- 扣除每单元 `3,392 B` 重复前奏与死代码裁减后: 上述薄层 (`23` 个 DEFAULT 留片 + `8` 个解码 + `4` 个 IO 壳共 ~`35` 个单元, 约 `35 x 3,392 = 119 KB` 前奏) 经单一运行库合并/链接后, 真实最小原生映像下限在现行产品后端密度下为:
  - `lnx/x86_64` 最小原生下限: 约 `560 KB - 650 KB` (压缩后约 `120 - 140 KB`), 即目前 `1,954,310 B` 的 29% - 33%.
  - 若其余五目标也走与 `lnx` 同等 `-O2 + tl_dce` 且无密度差距(外推比: `lnx/arm64` 1.08x -> ~`605 - 700 KB`; 按现行 `whole-o` 倍率未改则仍为 `osx/x64` ~`920 KB`、`win/x64` ~`1.0 MB`、`osx/arm64` ~`1.25 MB`、`win/arm64` ~`1.4 MB`).
  - 若叠加后端代码密度改善至 `gcc -O0`/`-Os` 水平 (2.5x - 4.5x), 最小薄层本机映像才会进一步下探到 `150 KB - 250 KB`.

---

## 七、结论收口与三个最值得先做的赌注 (各自 <= 1 小时可证伪)

1. 总结一句话: 减体积的主杠杆是「后端代码密度 + U6 去重与包内热模型压缩复核 + U4 非默认外移 + osx/win 切出 `-O1` whole-o」, 不是把通用 C 编译调度硬塞进 VM 字节码; 真正该 nn 化的只是 2.7% (`69 KB`) 的有限判定表与文案/上限配置, 目标是「单一事实源」而非字节数.
2. 三个 <= 1 小时可证伪赌注:
   - 赌注 A (对应 fable R3 赌注二: 后端密度 + `whole-o` arm64 双倍定因): 用同一批已验证小单元 (`cxmod.cx`、`m1.o` 与 `source_frame.cx`), 在隔离 `/tmp` 里分别测试 (i) `entry-build -O2` vs `-O1`、与 (ii) `whole-o` (`multi/image/O1`) vs `objects` 路线对 `lnx/arm64` (及读 `win/x64` `.text` 模式) 的实际 `.text` 差, 在 30 分钟内分离「`-O1` 缺 `e4`」、「无 `tl_dce`」与「每操作多指令」各自占多少. 证伪门: 若 `objects -O2` 与 `whole-o -O1` 在同目标 `lnx/arm64` 上差距小于 20%, 则排除 `whole-o` 是主因; 若相同源码下 `cx` 后端仅因 `-O2` 密度差距已在 2 倍以上, 则确认汇编/寄存器/窥孔密度改造为结构缩体首位.
   - 赌注 B (U6 去重 + 7 个未压模型启动时延测试): 对已定位的 7 个未压缩热模型 (`1,440,502 B`, `rb_release_models`) 用 `cx/text/deflate.cx` 做单次冷解压计时 10 次并测实际 APE `gzip` 可回收量(若把 7 个模型按同类 27% 压进 P4 包约省 `~1.05 MB`, 叠加 U6 去重独立 `win/arm64` 省 `562 KB` -> 立省 `~1.6 MB` APE!). 证伪门: 若 7 个模型现场全解压耗时超过 10 ms 预算门(或加权冷启动退化超 5%), 则维持原样不压, 否则作为零语义风险首期减体项合入.
   - 赌注 C (对应 fable R3 赌注三 / 实验(1): 非默认小表外置与 `sha` VM 10x 门): 取 `cellname.cx` (`3,946 B` x64) 或 `tapeshape.cx` (`8,405 B` x64) + `sha.cx` (`15,304 B`) 做两条 1 小时对照实验: (1) 用 U4 外移方式移出 `tapeshape`/`cellname`, 实测六片压缩减量、包文本增量与首次冷编耗时; (2) 把 `sha.cx` 纯软件循环写为最小 VM 字节码并用 1 KB / 1 MB 语料对拍 `sy_sha256_hex`, 测步数/字节与墙钟慢速比. 证伪门: `sha` VM 若比 C 版软件实现慢超 10 倍即永久关闭 δ 类大状态转 VM 字节码路线; 外置实验若首次冷编增加超可用门限或六片净省不足, 则修正 U4 外移边界.

---

## 八、E1 产品形态表 (`build/product-shape.tsv`) 设计与原型实测

### 8.1 表架构与生成器契约设计

按 fable 第三轮 E1 建议设计 `build/product-shape.tsv` (由 `pack` 相或独立生成器 `tools/product-shape-gen.cx` 经 `--check` / `--write` 从发布 APE 与 `g2/lnx-x86_64/` 逐个 `.o` + `plans/slice-units.tsv` 产出; 先在 `/tmp/slice-inspect/product-shape-gen.cx` 完成只读原型验证, 并留副本于 `~/.local/share/unisacc/research/agy-h/product-shape-gen-proto.cx`, 待 P0 收口后走工具线规范入库):

| 列名组 | 字段列表 | 口径与来源 |
|---|---|---|
| 版本与整包 | `version`, `ape_total`, `head_bytes`, `pad_total`, `cm_gz`, `pkg_bytes` | APE 实际字节布局 (`slice-audit` 同源解析) |
| 六片解压与头重 | `lnx_x64_raw`, `lnx_arm64_raw`, `osx_x64_raw`, `osx_arm64_raw`, `win_arm64_raw` (PE `head_bytes` 即为 `win/x64`) | gzip trailer ISIZE 与隐含 `win/arm64` 探测 |
| 六份复制总量 `R` | `r_ape_native` (= `ape_total - pkg_bytes`), `r_raw_sum` (= 六目标未压字节之和) | 分别刻画发布包内原生载荷比重与六目标展开复制总量 |
| 薄层与非默认 `T`, `N` | `unit_x64_sum`, `thin_x64`, `thin_pct10` (`T`), `nondef_x64` (`N`) | 基于 `g2/lnx-x86_64` 逐 `.o` `.text` 与 `plans/slice-units.tsv` |
| 每单元类别汇总 | `mig_tbl_x64`, `mig_split_x64`, `mig_vm_x64`, `new_uncls_x64` | 覆盖 `迁表`、`先拆分`、`迁VM`、`新增未定` (五类与 `thin_x64` 求和恒等于 `unit_x64_sum`) |

### 8.2 对 0.0.107 与 0.1.0 两已公开 APE 的原型重算实测对平

实跑 `/tmp/slice-inspect/product-shape-gen.cx` 输出 (见 `slice-breakdown-product-shape.tsv`):

| 版本 | `ape_total` | `head_bytes` | `lnx_x64_raw` | `lnx_arm64_raw` | `osx_x64_raw` | `osx_arm64_raw` | `win_arm64_raw` | `cm_gz` | `pad_total` | `pkg_bytes` | `r_ape_native` | `r_raw_sum` | `unit_x64_sum` | `thin_x64` (`T`) | `nondef_x64` (`N`) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `0.0.107` | 8,991,230 | 3,516,928 | 1,954,310 | 2,105,806 | 3,203,554 | 4,326,370 | 4,708,352 | 88,354 | 36 | 2,907,710 | 6,083,520 | 19,815,320 | 2,558,755 | 540,591 (21.1%) | 796,071 (31.1%) |
| `0.1.0` | 8,991,230 | 3,516,928 | 1,954,310 | 2,105,806 | 3,203,554 | 4,326,370 | 4,708,352 | 88,354 | 37 | 2,907,710 | 6,083,520 | 19,815,320 | 2,558,755 | 540,591 (21.1%) | 796,071 (31.1%) |

实测完全对平 `slice-audit` 与 `g2` (`unit_x64_sum = 540,591 + 69,363 + 836,866 + 1,046,280 + 65,655 = 2,558,755`, 零误差; `0.0.107` 与 `0.1.0` 因零产品差异封存, `entry.elf` SHA 同为 `20f97a98248e28b5...`, 仅 gzip 零垫差 1 B).

### 8.3 `0.0.97` `tapebin` 外置收据核查结论

经检索 `archive/release-pre0100/notes-0.0.97.md`、`plans/v0.0.111.md`、`git log -- cx/compiler/tapebin.cx` 与 `0.1.0` `g2/lnx-x86_64`:
- `0.0.97` 实际合入交付的是 `e2` 重扫步数修复 (`ca2f342aab`) 与执行核 64B `CorePackedState` 内联转移 (`a353daf3c7`), 并未执行 `tapebin` 外移 (`tapebin` 是在 0.0.44 `C44-TAPEBIN-01` 从 C 迁入 `cx/compiler/tapebin.cx` 的, 而外移 U4 计划长期放在 `0.0.97`/`0.0.100`/`0.0.111` 中, 随 0.1.0 窗口后移至今仍未动手).
- 在当前 `0.1.0` 切片中 `m6.o /src/compiler/tapebin.cx` (`30,158 B` x64 / `37,572 B` arm64) 依然编在切片内, 故「`tapebin` 外置前后的切片减量、包增量、冷编时间与 run-cache 命中差」没有实测收据, 全部标 `UNMEASURED`. 这正好支撑将 `tapebin`/`cellname`/`tapeshape` 作为赌注 C 在 `0.1.x` 做真实对比测量.

---

## 九、与 `nn-study.md` 及 `backend-density-plan.md` 逐项对照 (证实、修正与证伪)

对照 `~/.claude/jobs/7f46e032/tmp/nn-study.md` 与 `~/.local/share/unisacc/research/fable-v01x-flow/backend-density-plan.md` 的关键数字与推断:

| `nn-study.md` / `backend-density-plan.md` 论断 / 数字 | 本报告实测检验 | 裁决: 证实 / 修正 / 证伪 | 说明与精确数字 |
|---|---|---|---|
| 7 个原样未压缩模型合计 `1,441,101 B` (占模型区一半以上; 推断「为省启动解压, 原因未核」) | `0.0.105` 实测确为 `1,441,101 B`; `0.1.0` 为 `1,440,502 B` (M4 `e3` 少 599 B); 读源码证实 `tools/release-build.cx:1480-1539` `rb_release_models` 有意只对 `lnx/x86_64/image/O1` 七阶段留 `codec=0` | 证实并补充定因 | 非打包器缺陷, 确为发布打包阶段刻意去除启动解压 overhead; 压缩它们按 27% 率可省 ~1.05 MB |
| `lower`/`elf` 24 个模型按目标分化共 `1.03 MB` (占模型区 40%) | `0.1.0` 实测 `lower` 12 个 `665,815 B` + `elf` 12 个 `362,467 B` = `1,028,282 B` (存储占比 40.4%, 原始未压 `3,076,845 B` 占 56.5%) | 证实并修正细节 | 需补充: `1.03 MB` 里有 `264,938 B` 来自未压缩的 `lnx/x86_64` M12+M13 (因属 `O1` 热7件); 其余 22 个已全压缩 |
| `osx arm64` 切片是 `lnx arm64` 的 `2.07x`, 原因未知 | `0.1.0` 实测 `4,326,370 / 2,105,806 = 2.055x`, 代码段 `__text` `4,234,764 / 2,057,944 = 2.058x` (`win/arm64` 更是 `2.236x`); 实测 `__LINKEDIT` 仅 33.8 KB 排除了格式头与签名主因; `lnx/arm64` `.o` 累加本为 `3,138,672 B`, 经 `tl_dce` 删去 `1,080,728 B` (-34.4%) 才得 `2.06 MB`, 而 `osx/win` 走 `whole-o` (`-O1`, 无 `tl_dce`) | 部分解因并修正 | 推翻「Mach-O 格式 bloat」猜想; 证实至少 ~1.08 MB 差源于 `lnx` 有 `tl_dce` 与 `-O2`, 其余 ~1.10 MB (`3.14M -> 4.23M`) 属 `-O1` 与目标代码生成差 (`UNMEASURED` 分量); 正对应 `backend-density-plan.md` M1(`whole-o -O2`) 与 M2(`tl_dce`) |
| `backend-density-plan.md`: `whole-o` `-O1->-O2` 可压 ~23.7% (`cxmod` 194K->148K), 但 `-O2` 仍缺寄存器分配且有未细看冗余 | 本报告实查 `g2/lnx-x86_64/m1.o` (`text.cx` `-O2`) `objdump -d` (`0x45..0x61`) 实见两次连续栈往返符号扩展 (`sub $8,%rsp; mov %eax,(%rsp); movslq (%rsp),%rax; add $8,%rsp`) 及局部 `-0x8(%r9)` 写回 | 互相印证 | 证实 `-O2` 虽省去 23.7% 冗余仍留模板级栈往返, 支持将 M1-M3 (0.1.5) 与 M4 窥孔 (0.1.6) 分离验证 (正交实验见 ccogweco `tier-prestudy.md`) |
| 用 `2,441,257 B` vs `1,892,302 B` 设全局线性系数 `0.775` 折算 B 表与薄层 | `0.1.0` 实测 `102` 个 `.o` `.text` 和 = `2,558,755 B`, `entry.elf LOAD 0` = `1,905,313 B` (`0.745x`), 差额 `653,442 B` 来自每 `.o` 固定 `3,392 B` 隐含前奏 (~346 KB) + `tl_dce` 死函数剪除 (~307 KB) | 证伪线性折算模型并校准 | 不能对所有单元均一乘 `0.775` 或 `0.745`; 如 `hostnative.o` 裁剪后为 0 B, 小工具函数留存接近 `max(0, .text - 3,392)` 而不是按比例缩小 |
| D 表沿用 `0.0.93` `win/arm64` `546,584 B` 旧数估 APE `5.84 MB` | `0.1.0` `slice-audit` 实测 `win/arm64` 压缩为 `562,589 B` (解压 `4,708,352 B`), 六目标 APE 内原生合计 `6,083,520 B` (`6,083,520 / 1,954,310 = 3.11x`) | 修正旧基线数字 | 总体比值 `3.11x` 高度稳健 |
| 薄层 `27` 单元 `515,445 B` (0.0.87) + 自举环留片 + `sha` -> 下限约 `710 KB` (0.0.87 口径) | `0.1.0` 102 个 `.o` 为 28 个薄层 `540,591 B` (新增 `pinscan.o` 15,969 B) + 8 个解码自举 `110,034 B` + `sha` `15,304 B`; 扣除 28+8 个单元 3.4 KB 重复前奏与 5 个 `NO` 单元后, 真实链接下限约为 `560-650 KB` | 基本证实并精确化 | 补充揭示 `0.1.0` `.bss` 由 0.0.105 的 `14.9 MB` 跃升至 `19.1-19.5 MB` (`cxmod.o` 占 13.38 MB, 源于 `CXM_MAXU=256`), 对运行时虚存开销不可忽略 |
