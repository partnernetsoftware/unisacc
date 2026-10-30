# 候选换成产品：慢车道步骤（未执行）

2026-10-30。给 cc 审。这份只列步骤。没有跑 `source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`selfhost`，没有安装，没有换标，没有改已装 `unisacc.com`。

## 现在是什么

- 已装产品：3035300 字节，sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。
- 入口候选是显式 `--o1-drivers` 的 `out/grk-opt2-o1/candidate`，2959636 字节，权限 0755，sha256 `e477a4c70be28c11b86f9fce129fae2acf4e6c6813aa7d8436a8881d75160970`。148 格全 SAME，表在 `out/grk-opt2-mx`，记在 [cx-modules.md](cx-modules.md) 第 164 节。这不是产品，也不能拿去覆盖域矩阵。它后面接的是 build-14 的旧 P3（sha256 `b976719e…`），包里的 `\0hdr/` 还是旧嵌套头。
- `tools/entry-build.cx` 不带旗标已经编那六只驱动对象 `-O1`。这只改的是入口构造的默认，不是下面的产品两代。build-12 和 build-13 的两代逐字节，也只说明当时那只入口候选，不代替产品级两代。

## 按哪条顺序

按 [dev-methodology-2026-10-28.md](dev-methodology-2026-10-28.md)：正规 context 107 → PC → 两代 GEN → kind2 → 安装 → 全部 SOURCE 批 → 矩阵换标。一次只由一个主刀做。矩阵只绑定这一次纪元的候选 sha。中间的快车道提交不把旧格搬过来。

下面四步是这次点名要审的。它们落在这条顺序里，不是另外一条捷径。前一步的身份没核完，不要做下一步。

### 1. 包里的 `\0hdr/` 换成展平的 cx-flat.h

这一步落在 PC（源包刷新），不新造打包命令。

现在的情况：

- 树里 `include/cx.h` 在 `__UNISA__` 下已经读 `include/cx-flat.h`（`4f4a2ae2b3`）。生成器是 `tools/cx-flatten.cx --check`，源是 `include/cx-nested.h`。宿主编译器仍读嵌套那份。
- 已装产品包里的 `\0hdr/` 还是旧嵌套头。工作目录不在仓库根时，E2 从包资源取头，空 main 的 e2 要 1.6 s。见 [perf-bottlenecks-2026-10-30.md](perf-bottlenecks-2026-10-30.md)。
- 按这个键找文件的地方：`tools/cxlib/e2stream.cx`（键里含 `\0hdr/`），`tools/sim.cx`（键以 `\0hdr/` 开头）。
- 头怎么进包：`tools/cxlib/sourcepackage.cx` 和 `tools/source-package-pack.cx` 把 `root/include` 整棵记成 `CARRIED_HEADERS`。`tools/source-package-mixed.cx` 也把 `include` 记成 `CARRIED_HEADERS`。下一次正规包刷新会把现在的 `include/` 打进 `\0hdr/`，其中有 `cx-flat.h`，也有 `__UNISA__` 下改读它的 `cx.h`。

做完要核，这次没做：

- 新包里 `\0hdr/` 下的 `cx-flat.h` 与树里 `include/cx-flat.h` 逐字节相同。
- 工作目录不在仓库根的空 main，e2 不再是 1.6 s 那一档。没跑之前不能记通过。

### 2. 两代自举

这一步落在「两代 GEN」。是产品级的两代，不是 entry-build。

- 第一代用当时已装的产品构造。第二代用第一代的产物再构造。判词是对象和整包逐字节相同。墙钟相同不算。
- 现成工具：`tools/product-build.cx` 的用法是 `ROOT KERNEL_GENERATION PACKAGE_DIR slice-TARGET|wrap`。五个目标各一片，再 wrap 成 `unisacc-next.com`。
- `tools/selfhost-mixed.cx` 的四个角色是 `prior`、`fresh`、`after-prior`、`after-fresh`。compare 核两代的核、对象、五片和 `unisacc-next.com` 逐字节相同，并核第二代产品等于仓库根里的产品。它要求包是 mixed（package kind 4）。
- 若这一纪元的包收据是 `source-package-v1` 而不是 mixed，不要拿 selfhost-mixed 的 kind 4 去套。走哪一种由那次包收据的 schema 决定。这份清单不新选。
- 目录和 run id 等审过再填。这次不跑。

### 3. 安装

落在 kind2 准入之后。这里的 kind2 是源包准入，不是新工具名。`tools/candidate-install.cx` 按包收据认 schema：`source-package-v1` 是 mode 2，mixed 是 mode 4。用这一纪元自己的 context 和包。旧纪元的收据不能充数。

- 这个工具会换仓库根的 `unisacc.com`。文件头写明：只装本地开发候选，不签名，不发布。装上去的文件必须是这一代目录里的 `unisacc-next.com`，包字节要和准备好的包一致。
- 参数形状（这次不调用）：`ROOT KERNEL_GENERATION CANDIDATE_COM PREPARED_PACKAGE`，另有 `prepare`、`finish`、`verify-prepared`。`finish` 是 `finish ROOT KERNEL_GENERATION CANDIDATE_COM PREPARED_PACKAGE`。
- 装完核字节数和 sha256，并确认 `sh ./unisacc.com` 读到的是新文件。安装收据只记身份，不把日志当成功能通过。
- `e477a4c7` 那只不能直接装。它是入口 `entry.elf` 接旧 P3，不是这一纪元的 `unisacc-next.com`，包里的 `\0hdr/` 也还没换成展平头。

### 4. 换标

全部 SOURCE 批在这一纪元的已装产品上跑完之后才换。

- `tools/matrix-flip-gen.cx` 只在这次纪元之后用。
- 格式见 [foreign-host-runbook-cc.md](foreign-host-runbook-cc.md)：`COVERED_NATIVE@<sha>`，证据 `batch@<sha>:N/0;out/...`。
- `out/grk-opt2-mx` 的 148 格是入口对拍，不是 `tests/*-domain-matrix.tsv`。不能拿这张表覆盖域矩阵。
- 证据只写这一纪元的候选 sha。没跑的和失败的保持原样。

## 这次明确不做

- 不跑 `source-job-wave`、`pc-wave`、`kernel-build`、`product-build`、`selfhost`。
- 不调用 `candidate-install`，不改 `unisacc.com`。
- 不跑 `matrix-flip-gen`，不改五张域矩阵。
- 不把 148 格 SAME 写成产品已换，或矩阵已换。
