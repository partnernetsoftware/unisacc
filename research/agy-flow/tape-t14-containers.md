# T1.4 tapepack 容器供给与校验报告(4 工具闭包 + 102 个 g2 产品单元)

- 基线: `tape/m0` (`HEAD=a9725e53d`) / `0.1.2` 产品单元 (`g2` 102 单元)
- 打包工具: `tools/tapepack.cx` (默认 DCE 开启, `UTAPEBIN v1.3`, 72 操作码表对齐 `cx/compiler/tapeshape.cx` 与 `tools/tape/asm/tape_x86_64.S`)
- 容器目录: `~/.local/share/unisacc/research/agy-flow/t14-slices/`
- 解码工具: 基于 `unisacc + cx` 实现的只读二进制解析器(`tapeslice-inspect.cx`), 解析容器 `HOST` 段真实可达宿主槽与 `CODE` 段全部 72 操作码分布。

## 1. 总结论

(1) **4 个 T1.4 真实工具闭包全部打包并通过 `--check` 往返校验**: `check-crypto-sha256`、`check-packagefooter`、`chain-tplsha`、`check-tsv-lint` 均由 `unisacc.com --target lnx/x86_64 -S` 生成 `.tape` 汇编, 再由 `tapepack.cx` (默认 DCE) 产出 `.tapeslice` 容器并通过 `--check` 逐字节与指令位图往返验证。
(2) **宿主槽关键发现(M1 冻结 8 槽 `{0,1,2,3,4,5,6,8}`, `mask <= 383`)**: 全部 4 个真实工具及 `g2` 102 单元的 `subset_m1_383` 均为 `0`。即使最小的 `tools/check-crypto-sha256.cx` (仅 4 个可达宿主槽) 也因隐式前导 `include/stdio.h` 静态可达 `slot=10(lseek)` 与 `slot=36(isatty)` 而超出冻结 8 槽集合; 若 T1.4 探针须严格落在 8 槽冻结集内, 需在 `stdio.h` / 工具最小入口侧剪裁或由 M1x 执行器额外接通 `{10=lseek, 36=isatty}` (最小增量 2 槽即可覆盖 `check-crypto-sha256`, 加 `12=spawn` 覆盖 `chain-tplsha`, 再加 `9=stat` 覆盖 `check-packagefooter`)。
(3) **指令操作码关键发现(`T1.2` 已实现 32 操作码 vs 旧 26 操作码基线)**: `tape/m0` 当前 `tape_x86_64.S` 已实现 32 个操作码(`0..10`, `15..29`, `37`, `38`, `42`, `43`, `44`, `47`; 旧 26 槽基线不含 `8,9,10,29,37,38`)。这 32 个操作码已覆盖全部 4 个 T1.4 工具 **99.59% 至 99.75%** 的静态指令以及 `g2` 102 单元 **99.74%** 的静态指令, 零浮点操作码(`50..71` 全为 0)。4 个工具仅缺同一组 **7 个整数/间接调用/参数操作码**: `11(.div)`、`12(.mod)`、`13(.udiv)`、`14(.umod)`、`30(callr)`、`45(.argc)`、`46(.argv)`; `g2` 102 单元在此基础上仅多出 1 个操作码 `31(callm)` (共 7 条指令)。
(4) **`tapepack.cx` 大规模容量缺口(实测暴露)**: 102 个 `g2` 产品单元整程序链接后的 `.tape` 达 `9,541,830` 字节, 含 `3,386` 条 `.str` 定义(经 DCE 后留 `3,189` 条), 超出当前 `tools/tapepack.cx` 静态常量 `TP_MAX_STRS=2048`, 导致原版 `tapepack.cx` 在第 `352309` 行报 `TAPEPACK_REFUSE unresolved-symbol __u74_S18 line=352309`。仅将静态表上限扩至 `TP_MAX_FUNCS=16384, TP_MAX_STRS=8192, TP_MAX_BSSS=4096, TP_MAX_LABELS=131072, TP_HASH_SZ=65536, TP_LHASH_SZ=262144` 后, 102 单元整程序一次打包成功并通过 `--check` 往返校验。建议在 `tapepack.cx` 落库 `tape/m0` 后的下一小步直接抬高上述 6 个静态上限(或改为动态增长)。

## 2. 容器总览表(全部 DCE=1 且 `--check` PASS)

| 容器 | 容器路径 | 大小(B) | insns | funcs | relo | host_cnt | mask | subset_m1_383 | hit26 | hit32 | miss32_ops | SHA-256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `check-crypto-sha256` | `~/.local/share/unisacc/research/agy-flow/t14-slices/check-crypto-sha256.tapeslice` | 64596 | 13795 | 48 | 476 | 4 | 68719477765 | 0 | 11901 (86.27%) | 13756 (99.72%) | 7 | `b773e7952bc0736ce697c1f2820d7d996bb1f9d38e6fa854a51c22a1ee3da752` |
| `check-packagefooter` | `~/.local/share/unisacc/research/agy-flow/t14-slices/check-packagefooter.tapeslice` | 117168 | 23011 | 89 | 1402 | 11 | 68719482495 | 0 | 20196 (87.77%) | 22916 (99.59%) | 7 | `31e4aa6ebee9d85b71d2a7b4035931b17ff01149ebaf4eebab77d780f48ec971` |
| `chain-tplsha` | `~/.local/share/unisacc/research/agy-flow/t14-slices/chain-tplsha.tapeslice` | 70600 | 15158 | 70 | 593 | 10 | 68719481983 | 0 | 13117 (86.54%) | 15098 (99.60%) | 7 | `80768238568c16517517a5e15e786b3bbc86bdf568b0a421729432e58232f5f2` |
| `check-tsv-lint` | `~/.local/share/unisacc/research/agy-flow/t14-slices/check-tsv-lint.tapeslice` | 313938 | 61868 | 256 | 3004 | 17 | 68845836159 | 0 | 52775 (85.30%) | 61713 (99.75%) | 7 | `e3d3e6decad9b5be313f16c71aab85210d98dfab72c8224550a5ab0fd0385469` |
| `g2-012-102units` | `~/.local/share/unisacc/research/agy-flow/t14-slices/g2-012-102units.tapeslice` | 2326748 | 445029 | 2283 | 24926 | 18 | 69315598335 | 0 | 379875 (85.36%) | 443894 (99.74%) | 8 | `b255243bd30512c3496527fd6fcb79f7b624d995884434736d1ef16ec4476b5e` |

## 3. 各容器 `TAPEPACK_CHECK` 原行与 `HOST` 槽明细

### 3.1 `tools/check-crypto-sha256.cx`

- 容器路径: `~/.local/share/unisacc/research/agy-flow/t14-slices/check-crypto-sha256.tapeslice`
- `TAPEPACK_CHECK` 原行:
  `TAPEPACK_CHECK	PASS	dce=1	bytes=64596	insns=13795	funcs=48	relo=476	host_slots=4	sha256=b773e7952bc0736ce697c1f2820d7d996bb1f9d38e6fa854a51c22a1ee3da752`
- `HOST` 可达槽(4 槽, `frozen8=2`, `extra=2`, `mask=68719477765`, `subset_m1_383=0`):
  - 冻结集内(2): `0(exit)`, `2(write)`
  - 冻结集外(2): `10(lseek)`, `36(isatty)`
- `CODE` 摘要: `CODE_SUMMARY	insns=13795	unique_ops=36	hit26=11901	miss26=1894	miss26_ops=13	hit32=13756	miss32=39	miss32_ops=7`
- 缺口操作码(相对 T1.2 32 槽, 共 39 条): `11(.div)=5`, `12(.mod)=6`, `13(.udiv)=6`, `14(.umod)=6`, `30(callr)=2`, `45(.argc)=5`, `46(.argv)=9`

### 3.2 `tools/check-packagefooter.cx`

- 容器路径: `~/.local/share/unisacc/research/agy-flow/t14-slices/check-packagefooter.tapeslice`
- `TAPEPACK_CHECK` 原行:
  `TAPEPACK_CHECK	PASS	dce=1	bytes=117168	insns=23011	funcs=89	relo=1402	host_slots=11	sha256=31e4aa6ebee9d85b71d2a7b4035931b17ff01149ebaf4eebab77d780f48ec971`
- `HOST` 可达槽(11 槽, `frozen8=7`, `extra=4`, `mask=68719482495`, `subset_m1_383=0`):
  - 冻结集内(7): `0(exit)`, `1(read)`, `2(write)`, `3(mmap)`, `4(munmap)`, `5(open)`, `6(close)`
  - 冻结集外(4): `9(stat)`, `10(lseek)`, `12(spawn)`, `36(isatty)`
- `CODE` 摘要: `CODE_SUMMARY	insns=23011	unique_ops=38	hit26=20196	miss26=2815	miss26_ops=13	hit32=22916	miss32=95	miss32_ops=7`
- 缺口操作码(相对 T1.2 32 槽, 共 95 条): `11(.div)=10`, `12(.mod)=8`, `13(.udiv)=7`, `14(.umod)=7`, `30(callr)=1`, `45(.argc)=21`, `46(.argv)=41`

### 3.3 `tools/chain-tplsha.cx`

- 容器路径: `~/.local/share/unisacc/research/agy-flow/t14-slices/chain-tplsha.tapeslice`
- `TAPEPACK_CHECK` 原行:
  `TAPEPACK_CHECK	PASS	dce=1	bytes=70600	insns=15158	funcs=70	relo=593	host_slots=10	sha256=80768238568c16517517a5e15e786b3bbc86bdf568b0a421729432e58232f5f2`
- `HOST` 可达槽(10 槽, `frozen8=7`, `extra=3`, `mask=68719481983`, `subset_m1_383=0`):
  - 冻结集内(7): `0(exit)`, `1(read)`, `2(write)`, `3(mmap)`, `4(munmap)`, `5(open)`, `6(close)`
  - 冻结集外(3): `10(lseek)`, `12(spawn)`, `36(isatty)`
- `CODE` 摘要: `CODE_SUMMARY	insns=15158	unique_ops=38	hit26=13117	miss26=2041	miss26_ops=13	hit32=15098	miss32=60	miss32_ops=7`
- 缺口操作码(相对 T1.2 32 槽, 共 60 条): `11(.div)=7`, `12(.mod)=5`, `13(.udiv)=6`, `14(.umod)=6`, `30(callr)=1`, `45(.argc)=12`, `46(.argv)=23`

### 3.4 `tools/check-tsv-lint.cx`

- 容器路径: `~/.local/share/unisacc/research/agy-flow/t14-slices/check-tsv-lint.tapeslice`
- `TAPEPACK_CHECK` 原行:
  `TAPEPACK_CHECK	PASS	dce=1	bytes=313938	insns=61868	funcs=256	relo=3004	host_slots=17	sha256=e3d3e6decad9b5be313f16c71aab85210d98dfab72c8224550a5ab0fd0385469`
- `HOST` 可达槽(17 槽, `frozen8=8`, `extra=9`, `mask=68845836159`, `subset_m1_383=0`):
  - 冻结集内(8): `0(exit)`, `1(read)`, `2(write)`, `3(mmap)`, `4(munmap)`, `5(open)`, `6(close)`, `8(fstat)`
  - 冻结集外(9): `9(stat)`, `10(lseek)`, `12(spawn)`, `19(sleep)`, `23(rename)`, `24(unlink)`, `25(mkdir)`, `26(readdir)`, `36(isatty)`
- `CODE` 摘要: `CODE_SUMMARY	insns=61868	unique_ops=38	hit26=52775	miss26=9093	miss26_ops=13	hit32=61713	miss32=155	miss32_ops=7`
- 缺口操作码(相对 T1.2 32 槽, 共 155 条): `11(.div)=37`, `12(.mod)=19`, `13(.udiv)=22`, `14(.umod)=21`, `30(callr)=3`, `45(.argc)=18`, `46(.argv)=35`

### 3.5 `0.1.2` `g2` 102 个产品单元整程序链接容器

- 构造路径: 先经 `unisacc.com --target lnx/x86_64 -funit -c -b` 编译全部 102 个 `g2` 单元, 再经 `unisacc.com --target lnx/x86_64 -S <102个.o> -o g2-102.tape` 生成 `9,541,830` 字节整程序 tape, 再由扩表版 `tapepack-g2.cx` (仅将 6 个静态容量上限放大以容纳 `3,386` 条 `.str`, 编码/检验逻辑零改动) 打包为单容器并 `--check` 往返校验。
- 容器路径: `~/.local/share/unisacc/research/agy-flow/t14-slices/g2-012-102units.tapeslice`
- `TAPEPACK_OK` 原行:
  `TAPEPACK_OK	dce=1	total=2326748	code=1881398	data=46117	bssz=8	syms=32	relo=398816	host=164	insns=445029	funcs=2283	strs=3189	host_slots=18	bss_bytes=18643348	sha256=b255243bd30512c3496527fd6fcb79f7b624d995884434736d1ef16ec4476b5e`
- `TAPEPACK_CHECK` 原行:
  `TAPEPACK_CHECK	PASS	dce=1	bytes=2326748	insns=445029	funcs=2283	relo=24926	host_slots=18	sha256=b255243bd30512c3496527fd6fcb79f7b624d995884434736d1ef16ec4476b5e`
- `HOST` 可达槽(18 槽, `frozen8=8`, `extra=10`, `mask=69315598335`, `subset_m1_383=0`):
  - 冻结集内(8): `0(exit)`, `1(read)`, `2(write)`, `3(mmap)`, `4(munmap)`, `5(open)`, `6(close)`, `8(fstat)`
  - 冻结集外(10): `7(mprotect)`, `9(stat)`, `10(lseek)`, `12(spawn)`, `19(sleep)`, `23(rename)`, `24(unlink)`, `25(mkdir)`, `29(chmod)`, `36(isatty)`
- `CODE` 摘要: `CODE_SUMMARY	insns=445029	unique_ops=38	hit26=379875	miss26=65154	miss26_ops=14	hit32=443894	miss32=1135	miss32_ops=8`
- 缺口操作码(相对 T1.2 32 槽, 共 1,135 条): `11(.div)=215`, `12(.mod)=118`, `13(.udiv)=136`, `14(.umod)=131`, `30(callr)=221`, `31(callm)=7`, `45(.argc)=103`, `46(.argv)=204`

## 4. 给 M1x / T1.4 执行器与 `tapepack.cx` 的落地建议

(1) **T1.4 最小操作码补齐包(7 个操作码即可全量运行 4 个真实工具)**: 在 `tools/tape/asm/tape_x86_64.S` 补齐 `11(.div)`、`12(.mod)`、`13(.udiv)`、`14(.umod)`、`30(callr)`、`45(.argc)`、`46(.argv)` (再加 `31(callm)` 即可全量覆盖 102 单元 `g2` 编译器本体, 无需任何浮点操作码)。
(2) **T1.4 宿主槽梯度**:
  - 梯度 A(10 槽 = 冻结 8 槽 + `lseek(10)` + `isatty(36)`): 即可放行 `check-crypto-sha256.tapeslice`;
  - 梯度 B(12 槽 = 梯度 A + `stat(9)` + `spawn(12)`): 即可放行 `chain-tplsha.tapeslice` 与 `check-packagefooter.tapeslice`;
  - 梯度 C(18 槽 = 梯度 B + `mprotect(7)` + `sleep(19)` + `rename(23)` + `unlink(24)` + `mkdir(25)` + `readdir(26)` + `chmod(29)`): 全量覆盖 `check-tsv-lint.tapeslice` 与 102 单元 `g2` 本体。
(3) **`tools/tapepack.cx` 静态上限扩容**: 将 `TP_MAX_FUNCS` (`4096 -> 16384`)、`TP_MAX_STRS` (`2048 -> 8192`)、`TP_MAX_BSSS` (`2048 -> 4096`)、`TP_MAX_LABELS` (`32768 -> 131072`)、`TP_HASH_SZ` (`16384 -> 65536`)、`TP_LHASH_SZ` (`65536 -> 262144`) 一并抬高(或在 `tape/m0` 后续小步改为堆动态表), 以支持自举编译器 `g2` 整程序打包。
