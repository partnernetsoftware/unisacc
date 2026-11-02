# 0.0.43 已知缺口（发布说明用）

2026-10-31：主人把发布范围交给 cc 安排（原话「你来安排」），依据一份只读评估。0.0.43 的范围是**五片单文件 `unisacc.com`**（lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64），六个编译目标。下面三条是明文已知缺口，各自在 0.0.44 有编号。记录和矩阵的一致性由 `tools/matrix-waive.cx --check` 保证（记录在 [tests/waivers-0.0.43.tsv](../tests/waivers-0.0.43.tsv)，批次 `matrix-waive`）。

| 缺口 | 用户看到的 | 发布说明一句话 | 0.0.44 编号 |
|---|---|---|---|
| Windows on ARM64 没有原生入口 | Windows on ARM64 上运行的是 x86_64 的 PE，靠系统模拟；能用，慢一些。`-b win/arm64` 写出的用户镜像是原生 ARM64 | Windows on ARM64 runs the x86_64 PE under system emulation; a native entry is a 0.0.44 target. Images written with `-b win/arm64` are native ARM64. | C44-WIN-ARM64-01 |
| libunisacc 嵌入式 API（us_*）没有 | `unisacc.com` 本身的所有功能都在（`-run`、`-o`、`-S`、FFI）；库形态的 us_* 接口没有随 0.0.43 发布，历来也没有随任何版本发布 | The libunisacc embedding API (us_*) is a development interface and is not part of the 0.0.43 release. | C44-LIBUNISACC-01 |
| 宏展开里的诊断列号 | 宏调用内部的错误：行号正确，列号和摘录行按展开后的文本计数 | Diagnostic columns and excerpts inside macro expansions count in the expanded text (line numbers are correct); 0.0.44 reports user-source positions. | C44-DIAG-COLUMN-01 |
| 无汇编文本输出（`-S -b`） | 0.0.43 产品对 `-S -b lnx/x86_64`（或 `lnx/arm64`）`-o FILE.s` 报 `NOT_IMPLEMENTED object`、退出码 2，不写文件；裸 `-S` 只写 tape IR（`unisacc as` 拒绝该文本）。汇编文本在产品路线里没有对应步：`route=…/object/O{0,1,2}` 的步集只有 e1–e4/lower/prune/units/elf。`-S -b osx/*` 或 `win/*` 另按名拒绝（“written for Linux targets in this version”） | Assembly text (`-S -b os/arch -o FILE.s`) is not on the 0.0.43 product route for any target: Linux targets stop with `NOT_IMPLEMENTED object` (exit 2) and write no file, and the product route has no assembly-text step. The archived reference compiler wrote GNU assembly for all six targets. | C44-ASM-TEXT-01 |

另有一条措辞上的已知差异，不算缺口（格是覆盖的）：结构体里含自己（`struct S{struct S inner;};`）按拒绝处理，报 `not covered: a struct member of struct type`，退出码 1、无输出，不是标准 C99 6.7.2.1p2 的 `incomplete type` 措辞。改措辞要改 e3 模型的拒绝表（exec/parse2/control-reject.tsv），会动 SID 走慢车道，列入 0.0.44（跟 C44-DIAG-COLUMN-01 一起做）。另外 `directory-input`（目录当输入文件）退出码是 1，旧版 79fb84e6 是 2，和缺文件一致。

不属于用户可见缺口、但影响本版测试覆盖：`tools/hostseed-step.cx` 无法被产品编译（`reject: tape domain: unknown opcode`），因此 hostseed-* 的 61 批材料链造不出来，这些批次在 0.0.43 记 UNPREPARED 而不是 PASS。同目录 `object-material.cx`、`net-material.cx` 能正常编过，说明这是该文件独有的构造；诊断在 `plans/release-0.0.43.md`。用户可见功能不受影响。

不属于缺口、但仍然开着的：`ri.windows-snapshot-and-e3-companion` 的 win/x86_64 一条（Windows 快照和 E3 伴随目前是不支持平台的桩，要在 Windows 实机上做）。

另有三条小的已知行为（都有 0.0.44 编号，不单列为缺口）：

| 行为 | 说明 | 编号 |
|---|---|---|
| `-run` 的用户程序在地址限制下 malloc 失败 | 用户程序在 `ulimit -v` 限制下分配失败时，运行时把 mmap 的 -ENOMEM 当成地址，程序崩溃（退出 139）而不是看到 NULL；分配器在包里的 `stdlib.h`（`_unisa_map`），修它要新包 | C44-MALLOC-01 |
| `-Werror` | 旗标被解析但没人读，警告仍是警告，退出码 0 | C44-WERROR-01 |
| 独立单流容器 | 非产品构建，0.0.43 不随发；每流行为通过产品的路线覆盖 | C44-STREAM-CONTAINER-01 |
| 阶段位置格 | Darwin carry、Windows gate 保存/恢复、ARM narrowing 不在产品镜像里；`-S -b` 的汇编文本整条路线不在产品里（Linux 目标同样是 `NOT_IMPLEMENTED object`，见上表「无汇编文本输出」行），Mach-O/COFF 另按名拒绝；没有独立整镜像编码器 | C44-STAGE-POS-01 |
