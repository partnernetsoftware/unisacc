# 四目标可编性探针（2026-10-30）

快车道探针。不是产品，不记通过。没有链接，没有 wrap，没有改 `candidate-install.cx`、`rebuildcontext.cx`、`kernelcontext.cx`，没有换已装 `unisacc.com`。

## 输入

- 构造器是已装 `unisacc.com`，用 `sh` 拉起。3035300 字节，sha256 `79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`。
- 入口是 `tools/entry-build.cx`，提交 `76085cb99d`。`--target` 省略时仍是 `lnx/x86_64`。
- P3 参数是 `out/grk-entry-build-14/pkg.bin`，sha256 `b976719eb971fceae766398fe531c0cd16c25bbffab9ef2a7ab6d8184af70fd3`。这次只编译，没有把包接到对象后面。
- 环境去掉了 `UNISA_CONTAINER` 和 `UNISACC_CONTAINER`。
- 每个对象单独一次 `tools/bound`，单次不超过 55 秒。四个目标没有放进同一个进程。

## 默认目标的字节

`76085cb99d` 之后，不带 `--target` 重编，和改前的 `out/grk-opt2-default` 逐字节相同。

| 对象 | 字节 | sha256 |
|---|---:|---|
| path.o | 24736 | `8322dfd20e44f434000bb37af5cf43310cc87920f1959c4a204cb6bc2952574a` |
| tapegate.o | 478584 | `1310b3c403ad29d9bc3c8ababf50774f561d8f4acd9848b272957aaa16133d3d` |

## 四个目标

已装产品，33 个对象，只做 `-c -funit -b`。

| 目标 | 33 个对象 |
|---|---|
| lnx/arm64 | 都出了对象。开头是 `7f 45 4c 46`。目录 `out/grk-tgt-probe/lnx-arm64`（不提交）。33 个文件一共 8120752 字节，最小 17704，最大 2096768。没有 `entry.elf`，没有 `candidate` |
| osx/x86_64 | 都编译失败，目录里没有对象文件 |
| osx/arm64 | 都编译失败，目录里没有对象文件 |
| win/x86_64 | 都编译失败，目录里没有对象文件 |

失败的 99 格，第一条诊断都是这一句：

`unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects`

已装产品在写出对象之前就拒绝了非 Linux 的 `-c -b`。现在不能用这只产品去链接 osx 或 win 的片，也不能开始 wrap。

## 逐格

下面是实跑记录。`OK` 表示对象文件写出来了。`FAIL` 表示编译失败。都不是通过。

```
lnx/arm64	drive.o	OK	-
lnx/arm64	host.o	OK	-
lnx/arm64	stages.o	OK	-
lnx/arm64	routestep.o	OK	-
lnx/arm64	stagehandoff.o	OK	-
lnx/arm64	runexec.o	OK	-
lnx/arm64	attributeframe.o	OK	-
lnx/arm64	bind.o	OK	-
lnx/arm64	cli.o	OK	-
lnx/arm64	compat.o	OK	-
lnx/arm64	route.o	OK	-
lnx/arm64	deps.o	OK	-
lnx/arm64	input.o	OK	-
lnx/arm64	args.o	OK	-
lnx/arm64	path.o	OK	-
lnx/arm64	entrybind.o	OK	-
lnx/arm64	memlayout.o	OK	-
lnx/arm64	driveface.o	OK	-
lnx/arm64	driveroute.o	OK	-
lnx/arm64	cxmod.o	OK	-
lnx/arm64	tapegate.o	OK	-
lnx/arm64	fwdstub.o	OK	-
lnx/arm64	executor.o	OK	-
lnx/arm64	transition.o	OK	-
lnx/arm64	codec.o	OK	-
lnx/arm64	modeldecode.o	OK	-
lnx/arm64	modelnumber.o	OK	-
lnx/arm64	packmodel.o	OK	-
lnx/arm64	modelload.o	OK	-
lnx/arm64	bindstage.o	OK	-
lnx/arm64	entryexec.o	OK	-
lnx/arm64	hostnative.o	OK	-
lnx/arm64	tapelink.o	OK	-
osx/x86_64	drive.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	host.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	stages.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	routestep.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	stagehandoff.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	runexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	attributeframe.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	bind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	cli.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	compat.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	route.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	deps.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	input.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	args.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	path.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	entrybind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	memlayout.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	driveface.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	driveroute.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	cxmod.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	tapegate.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	fwdstub.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	executor.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	transition.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	codec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	modeldecode.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	modelnumber.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	packmodel.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	modelload.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	bindstage.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	entryexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	hostnative.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/x86_64	tapelink.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	drive.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	host.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	stages.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	routestep.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	stagehandoff.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	runexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	attributeframe.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	bind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	cli.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	compat.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	route.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	deps.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	input.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	args.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	path.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	entrybind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	memlayout.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	driveface.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	driveroute.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	cxmod.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	tapegate.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	fwdstub.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	executor.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	transition.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	codec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	modeldecode.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	modelnumber.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	packmodel.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	modelload.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	bindstage.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	entryexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	hostnative.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
osx/arm64	tapelink.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	drive.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	host.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	stages.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	routestep.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	stagehandoff.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	runexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	attributeframe.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	bind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	cli.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	compat.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	route.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	deps.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	input.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	args.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	path.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	entrybind.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	memlayout.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	driveface.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	driveroute.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	cxmod.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	tapegate.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	fwdstub.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	executor.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	transition.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	codec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	modeldecode.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	modelnumber.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	packmodel.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	modelload.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	bindstage.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	entryexec.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	hostnative.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects
win/x86_64	tapelink.o	FAIL	unisacc: model driver: object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects

```
