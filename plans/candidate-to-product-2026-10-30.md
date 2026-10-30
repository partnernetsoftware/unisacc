# 候选换成产品：慢车道步骤（未执行，以新 CX 入口为准）

2026-10-30 修订。上一版 `f091575044` 把 `tools/product-build.cx` 当成现成的两代工具。那一步跑不通：它编的是已经归档的旧 C，编出来也不是新入口。这份改成以新入口为准。仍然只写清单，没有跑 `source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`selfhost`、`entry-build` 的新目标，没有 wrap，没有安装，没有换标，没有改已装 `unisacc.com`。

## 现在是什么

- 已装产品：3035300 字节，sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。开头 8 字节是 `MZqFpD='`。这是 APE：`seed/ape.c` 把 win/x86_64 的 PE 放在偏移 0，壳脚本写在 DOS stub 里，另外四片（lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64）压缩后接在后面，包用 `--payload` 附一次。
- 入口候选 `out/grk-opt2-o1/candidate`：2959636 字节，权限 0755，sha256 `e477a4c70be28c11b86f9fce129fae2acf4e6c6813aa7d8436a8881d75160970`。开头是 `\x7fELF`。它是 `entry.elf`（1323616 字节，同样是 ELF）后面接 build-14 的旧 P3。只覆盖 lnx/x86_64。148 格全 SAME 记在 [cx-modules.md](cx-modules.md) 第 164 节。不能把它改名成 `unisacc-next.com` 拿去装。
- `exec/c/asmcompiler.c` 和 `exec/c/compiler.c` 已在 `e16028ffa1` 归档，活路径上不存在，副本在 `archive/reference-c/exec/c/`。`tools/product-build.cx` 第 75 行仍写 `exec/c/asmcompiler.c`。`tools/hostseed-step.cx` 的 `bootstrap_host`（第 45 行起）链接 `exec/c/compiler.c`，第 100 行编 `asmcompiler.c`。这两条链现在起不来。就算把归档文件搬回来，编出来的仍是旧 C 产品，不是 `src/unisacc.cx` 这条入口。

只换 lnx/x86_64、不包 APE 的后果（这份不采用）：这台机器上 `sh ./unisacc.com` 还能跑；另外四个目标没有片；非 lnx/x86_64 的机器不能跑；`candidate-install` 要的五片和 APE 包尾对不上；外宿主矩阵不能记 `COVERED_NATIVE`。

## 顺序

方法学的大顺序还在：[dev-methodology-2026-10-28.md](dev-methodology-2026-10-28.md) 的 context 107 → PC → 两代 GEN → kind2 → 安装 → 全部 SOURCE 批 → 矩阵换标。一次只由一个主刀做。矩阵只绑定这一次纪元的候选 sha。

GEN 的工具换成下面的新入口构造。现在的 `product-build.cx` 不担任这一步。前一步的身份没核完，不要做下一步。

### 1. 新 P3：`\0hdr/` 换成展平的 cx-flat.h

这一步是包，不是五片。不新造打包命令。

- 树里 `include/cx.h` 在 `__UNISA__` 下已经读 `include/cx-flat.h`（`4f4a2ae2b3`）。生成器是 `tools/cx-flatten.cx --check`，源是 `include/cx-nested.h`。
- 已装产品包里的 `\0hdr/` 还是旧嵌套头。工作目录不在仓库根时，E2 从包资源取头，空 main 的 e2 要 1.6 s。见 [perf-bottlenecks-2026-10-30.md](perf-bottlenecks-2026-10-30.md)。
- 按这个键找文件的地方：`tools/cxlib/e2stream.cx`，`tools/sim.cx`。
- 头怎么进包：`tools/cxlib/sourcepackage.cx`、`tools/source-package-pack.cx`、`tools/source-package-mixed.cx` 把 `root/include` 整棵记成 `CARRIED_HEADERS`。下一次正规包刷新把现在的 `include/`（含 `cx-flat.h`，以及 `__UNISA__` 下改读它的 `cx.h`）打进 `\0hdr/`。
- 做完要核，这次没做：新包里 `\0hdr/` 下的 `cx-flat.h` 与树里 `include/cx-flat.h` 逐字节相同；工作目录不在仓库根的空 main，e2 不再是 1.6 s 那一档。没跑之前不能记通过。
- 这只新包才是 wrap 的 `--payload`。build-14 的 `pkg.bin`（sha256 `b976719e…`）不能再接上去。

### 2. 新入口版的 product-build

没有这个工具。要新写的是步骤，不是去跑现在的 `product-build.cx`。五个目标：`win/x86_64`、`lnx/x86_64`、`lnx/arm64`、`osx/x86_64`、`osx/arm64`。

1. 每个目标用 `tools/entry-build.cx` 编一次。33 个对象的清单不变。`-b` 用该目标。六只驱动对象保持现在的默认 `-O1`，其余 `-O2`。
2. `lnx/x86_64` 的链接结果叫 `entry.elf`。现在这只是 ELF（开头 `\x7fELF`）。另外四个目标各留自己的映像，文件名沿用 `slice-win-x86_64`、`slice-lnx-arm64`、`slice-osx-x86_64`、`slice-osx-arm64`，加上 `slice-lnx-x86_64`（就是这份 `entry.elf`）。
3. 五片都是链接结果，不接 P3。P3 只在 wrap 时附一次。`entry-build` 现在的 `eb_splice` 是 ELF 候选的做法，不用它来生成 `unisacc-next.com`。
4. wrap 仍用 `seed/ape.c`。参数顺序是 `OUT WIN_X86_64 LNX_X86_64 LNX_ARM64 OSX_X86_64 OSX_ARM64`，加上 `--payload` 第 1 步的新包、`--product Unisacc` 和 `src/version.h` 里的版本。产物叫 `unisacc-next.com`。开头应是 `MZqFpD`。win 片必须是 PE，头在 64..1024，`seed/ape.c` 才能把它挪到偏移 0 并写入壳脚本。
5. 现在还编不了五片。`entry-build` 的编译和链接把 `-b` 写死成 `lnx/x86_64`。整表一次超过 60 秒。60 秒例外名单现在是 `source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`selfhost`（`AGENTS.md`）。把这条新链写进例外名单等于改规则，待主人裁定，cc 去报。裁定之前每批仍不超过 55 秒，不把五个目标一次进程跑完。
6. 对拍里用户程序的两个 Windows `-o` 两边都退出 1（[cx-modules.md](cx-modules.md) 第 161 节）。那是用户程序，不是编译器自己的 win 片。不能当成新入口已经能交出 `seed/ape.c` 要的 PE。win 片没实测通过之前，wrap 不能开始。

### 3. 两代判词

比的是 `entry.elf`，不是整只 APE，也不是 `tools/selfhost-mixed.cx` 那 11 个核对象。

- 第一代：构造器是已装产品，用 `sh` 拉起（sha256 `79fb84e6…`）。按第 2 步编出五片和 `entry.elf`，用第 1 步的新 P3 wrap 成第一代 `unisacc-next.com`。
- 第二代：构造器是第一代这只 APE，同样用 `sh` 拉起，再编一次同一批源、同一组目标。
- 判词：两代 `entry.elf` 逐字节相同。另外四片各自两代逐字节相同，文件名用各自的 slice，不把 PE 或 Mach-O 叫成 `entry.elf`。墙钟相同不算。
- `entry-build` 现在的显式构造器是直接执行，不经 `sh`。APE 开头是脚本，直接执行起不来。第二代要能把第一代 APE 交给 `sh`。不写构造器时，它只会 `sh` 仓库根的已装产品。这一处要改，否则第二代没有构造器。
- 两代用同一只新 P3。`entry.elf` 不含 P3，所以包换了不会单独把 `entry.elf` 比红。两只 APE 的包尾仍要和这只 P3 逐字节相同，那是安装那一步的事。

### 4. candidate-install 要改什么

工具是 `tools/candidate-install.cx`，准入在 `tools/cxlib/rebuildcontext.cx`，核上下文在 `tools/cxlib/kernelcontext.cx`。现在这份只接受旧构造链交出的 `unisacc-next.com`。新形态仍用这个文件名，内容是第 2 步的 APE。下面这些检查对不上，必须改。这次不改代码。

- 路径（`candidate-install.cx` 第 67 行）：候选必须是 `GENERATION/unisacc-next.com`。ELF 候选 `e477a4c7` 改名放进去也不行，后面的五片收据和壳格式对不上。
- 包尾（第 68 行）：最后 16 字节是 `UNIPKG1\n`，前面的包与准备好的 `compiler.pkg` 逐字节相同。这一条留下。包必须是第 1 步的新 P3，不是 build-14 的 `pkg.bin`。`rebuildcontext.cx` 的 `rb_embedded` 用的也是这个包尾。
- 五片：`rebuildcontext.cx` 要求 generation 里有五只 `slice-*`，以及各自的 `.native-build.tsv`（`PREPARED native-product`，里面是 product context 和该片的 sha）。片的字节改成 entry-build 对该目标的链接结果。收据里写 entry-build 和目标，不再写 `asmcompiler.c`。kind 2 的 wrap 收据还要 libz sha 和 loader trace。wrap 若仍是 `seed/ape.c` 加 `-DAPE_ZLIB`，这条留下。
- `native.tsv`（`candidate-install.cx` 第 73–74 行）：现在哈希 `tools/package-refresh.cx`、`tools/cxlib/kernelcontext.cx`、`tools/candidate-install.cx`、`seed/ape.c`，加上 `context.sha256` 和五片。要加上 `tools/entry-build.cx`。五片改成新入口的链接结果。
- `kc_check`（`kernelcontext.cx` 第 9–12 行）：安装时会再跑。它读 generation 里的 `identity-helper`，再把 `tools/kernel-build.cx`、`tools/asm-source.cx`、`tools/asm-material.cx`、`tools/kernel-link.cx`、`tools/cxlib/elfkernel.cx`、三份汇编测试和仓库根 `unisacc.com` 算进 `context.sha256`。新入口的片不由这些工具产生。context 要改成盖住 `entry-build.cx` 和那 33 个入口源；在改完之前，安装应拒绝并停下。`identity-helper` 仍要在，否则 `kc_check` 第一行就读不到文件。
- 换文件的动作可以留：第 82 行把候选 rename 成仓库根 `unisacc.com`。装完核字节数和 sha256，并确认 `sh ./unisacc.com` 读到的是这只 APE，开头是 `MZqFpD`。安装收据只记身份，不把日志当成功能通过。

### 5. 换标

全部 SOURCE 批在这一纪元的已装产品上跑完之后才换。

- `tools/matrix-flip-gen.cx` 只在这次纪元之后用。
- 格式见 [foreign-host-runbook-cc.md](foreign-host-runbook-cc.md)：`COVERED_NATIVE@<sha>`，证据 `batch@<sha>:N/0;out/...`。
- `out/grk-opt2-mx` 的 148 格是 lnx/x86_64 入口对拍，不是五片，也不是 `tests/*-domain-matrix.tsv`。
- 证据只写这一纪元的 APE sha。没跑的和失败的保持原样。另外四个目标没有同候选证据之前，不能记成已覆盖。

## 旧工具

| 工具 | 处置 | 依据 |
|---|---|---|
| `tools/product-build.cx` | 退场 | 第 75 行编 `exec/c/asmcompiler.c`。文件已归档。编出来的是旧 C 片，不是新入口 |
| `tools/hostseed-step.cx` | 退场 | 第 45 行起的 `bootstrap_host` 链接 `exec/c/compiler.c`，第 100 行编 `asmcompiler.c`。同样已归档 |
| `tools/hostseed-step-nativefirst.cx` | 退场 | 同样链接 `compiler.c`、编 `asmcompiler.c` |
| `tools/cx-driver-build.cx` | 退场 | 源列表含 `exec/c/asmcompiler.c` |
| `tools/check-cx-driver-target-unit.cx` | 退场 | 用已装产品编 `asmcompiler.c` 做驱动单元检查 |
| `tools/selfhost-mixed.cx` | 退场 | 两代比的是旧核对象、五片和 kind 4 的 `unisacc-next.com`。判词已改成 `entry.elf` |
| `tools/entry-build.cx` | 改写 | `-b` 写死 `lnx/x86_64`。要按五个目标各编一次。第二代 APE 要经 `sh` 当构造器。产品不走现在的 `eb_splice` |
| `tools/candidate-install.cx` | 改写 | 见第 4 节。替换仓库根那一步留下 |
| `tools/cxlib/rebuildcontext.cx` | 改写 | 五片和 `.native-build.tsv` 按旧片核对。`rb_package_refresh` 还摘要 `arm64.kernel`、`x86_64.kernel`。包尾仍要和 `compiler.pkg` 逐字节相同 |
| `tools/cxlib/kernelcontext.cx` | 改写 | `kc_check` 的 context 绑的是 kernel-build 那一组旧工具 |
| `seed/ape.c` | 留下当 wrap | 已接受五片和 `--payload`。不负责编入口。win 片必须是它能挪头的 PE |
| `tools/cx-flatten.cx` | 留下 | 展平头的生成器。包刷新前用 `--check` |
| `tools/source-package-pack.cx`、`tools/cxlib/sourcepackage.cx`、`tools/source-package-mixed.cx` | 留下 | `CARRIED_HEADERS` 把 `include/` 打进新 P3 |

工具退场与注册表里那几批同时处理。`hostseed-*`、`ri-material-*`、`package-controls` 依赖 `product-build`、`hostseed-step` 这些旧工具。归档还是标不可运行，等主人裁定，见 [archived-c-batches-2026-10-30.md](archived-c-batches-2026-10-30.md)。这份清单不改注册表行。

## 这次明确不做

- 不跑 `source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`hostseed-step`、`selfhost`。
- 不改 `entry-build.cx`，不按新目标编五片，不 wrap。
- 不改 `candidate-install.cx`、`rebuildcontext.cx`、`kernelcontext.cx`。
- 不调用安装，不改 `unisacc.com`。
- 不跑 `matrix-flip-gen`，不改五张域矩阵。
- 不把 148 格 SAME 写成产品已换，或五个目标已经能编。
