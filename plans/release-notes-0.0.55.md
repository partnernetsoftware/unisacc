# unisacc 0.0.55 -- 发布说明(未实跑一律写未跑)

发布提交 X `f9905732ecabfc5ee93f3a585ffa8d1b48090614`(含冻结点 `fae2818a82bced30cf65c1782f31158fde8fad03` 前全部工具提交)。候选记录 commit 列与构造源为 `0add718ff515efb02b4fe05f8028e92d9cd1a284`(0.0.54 整链真实构造源,源 SID 4001d3e2):本版工具提交改动了 SID 覆盖的工具侧文件,冻结点现算 SID 为 4076ca56,与复用的链产物不符,故按 0.0.53 先例采用「构造源与发布提交分离」,产品字节与 0.0.54 同源,仅版本槽不同。git 范围:`v0.0.54..fae2818a82`,共 21 条提交。

## 是什么

面向 C99 的编译器,自己写出 ELF、Mach-O、PE,可编译自身;产品形态为单文件 `unisacc.com`。0.0.55 是开发回路与构造链提速版:产品输入相对 v0.0.54 零改动(`git diff v0.0.54 REV -- src cx exec seed weights kernel include` 为空)。链产物复用 0.0.54 整链构造,两代不动点结论沿用 0.0.54,本版不重新声称。上一发布版见 `archive/docs-2026-11c/plans/release-notes-0.0.54.md`。

原计划唯一产品改动 P1(产品内单元前端缓存,两轮独立审查通过,u12 换根命中 9.8s -> 6.4s)因需整链重建未入本版,顺延 0.0.56。

## 身份与证据

| 对象 | 身份 | 说明 |
|---|---|---|
| APE `unisacc.com` | c05ff40bf99de9b1ece8310ecc2d7c5bb83f7a4ccf620162ca46c26a1e851fde,6529460 字节 | `--version` 实测输出 `unisacc 0.0.55`;与 0.0.54 APE 0ca23809 仅版本槽不同 |
| 记录 `unisacc.com.build.json` | f80f7874001b1ecc8754def8ee2d69dfbe03c648cf87f0713713b854a34a073d | SOURCES_SID VERIFIED 4001d3e2 |
| `SHA256SUMS` | 2c86ffa2d7446cbc9fe0a9d67b59f6399062badb1924a2a742d4fb0159204e35 | 本地自校 6/6 OK |
| 其余资产 | pkg.bin 56efea53(同 0.0.54),slice-win-arm64 b558ac8d,zip 81fb80f6 | 见 release/candidate.tsv |
| 链产物 | 复用 0.0.54 整链构造 | 本版未重跑整链 |
| s9 commit 档 | 候选 APE c05ff40b、发布提交 X:selected 137,PASS 137,FAIL 0,UNKNOWN 0,DEFERRED 0;qualification NOT_FULL(12 个 osx 批本机 NOT_APPLICABLE);首跑 --max-steps 2 步数不足 step-pending(3 批 watchdog UNKNOWN,未记 PASS),新目录 --max-steps 5 重跑全过,约 552s | 相比 0.0.54(130/132,UNKNOWN 2)全绿;不称 QUALIFIED |
| s9 release / daily 档 | 本版未跑 | 0.0.54 release 档首跑 15 批全域矩阵 OPEN 格 FAIL 未变;daily 未跑 |
| 原生 CI | 同候选运行 43010525199(检出提交 X f9905732ec,ape_sha256 c05ff40b):QUALIFIED pass=42 fail=0 not_run=0 invalid=0 | 六目标原生单元格实跑 |

## 行为变化(升级前请读)

产品输入零改动:编译器、运行库、驱动的语义相对 0.0.54 不变。本版变更全部在工具、检查、构造链辅助、批次表与文档侧。

## 本版已落地(`v0.0.54..fae2818a82`,共 21 条)

门与开发回路:
- `e754066842` integrate-patch prepare 编译步可续跑(prepare-done 记录、4 路并行波次、PENDING rc=3 与 --resume),新批次 integrate-patch-prepare-resume 11/11。
- `c93ef97617` D1 part 01:13 个棘轮测试文件按 selfcheck 接口调整,9 处无界子进程改有界。
- `e515cf9cd6` D1 part 02:check-pins 按 selfcheck 接口调整,保留全部合同。
- `ca19ca0ead` D3 pins 符号锚:432 行由 FILE:LINE 迁为内容锚,新增 pins-reanchor 与 check-pins-reanchor,新增行号锚即红。

构造链提速:
- `53dc832999` T3 主补丁:chain-phases-gen 与 chain-phases.tsv(475 相),candidate-chain 接 net-deps 选网与 adopt 复用;net-incremental-rehearsal 改读 recipes[]。
- `f66e3b977c` C3 p3 细相:p3 拆为 44 细相各 <=55s,check-release-ctor 改真实边界拒绝,批次迁为显式材料(无材料 DEFERRED)。
- `2b77b2aab9` C1 entry-build 收据头只写一次,头不符具名拒,修 0.0.54 MANUAL-3 与逐键多头收据不兼容。
- `9465b876ad` C1 chain-step fixedpoint 新增 --objects 批量键,check-chain-batchkeys 7 项。
- `dfc00b31c6` C1 根因修复 release-build 冷编:freeze 内容相同则跳过复制,同快照冷编 26-33s -> 约 0.3s。
- `bf21dd197c` C1 根因修复 Root41 重写:install_if_changed,内容相同不重写,不同则原子替换。

批次治理与基线红修复:
- `c8b81b0fb3` tools-compile 补登 8 个工具并补片 092,hostseed-step 诊断行号同步。
- `735076b461` tools-compile 二次补登 check-chain-batchkeys 与 check-entry-receipt-head。
- `3e1f0126d6` tools-compile 三次补登,批次表补 093。
- `389bfc0726` D7 slice-inventory 超时修复:夹具并行编译,修前 53.9s 被杀,修后 31.8-33.7s,断言与时限不变。
- `ad2c5ac422` refactor-tool 基线红修复:check-refactor sweep 夹具坏文件改为 undeclared_var,断言不改,并含 10 个工具有界化。

R2 全仓清理(无界子进程调用改 50s 有界,行为不变):
- `648b7fd9cd` 01/06:新增 r2-bound 与 check-r2-bound(6 项)。
- `44d0762ff1` 02/06,`76033d39d1` 03/06,`7a9861b0af` 05/06,`fae2818a82` 06/06(04/06 随 `ad2c5ac422`)。
- R2 全仓 150 处共清 85 处,余 56 处为测无界原语本身的检查文件,保持,整批由 run-batch 外层 bound。

其他:
- `19bae6698f` 本机根换 0.0.54,账本 REL054-PUBLISHED(公开读回 6/6)。

## 已知缺口(如实)

- P1 产品内单元前端缓存顺延 0.0.56。
- D6 槽位仲裁、G1 门本体(D2/D4/D5 main-health)、T55-INNERBOUND、G2、G3、G4 未入版。
- D1 selfcheck 03+04 与 E s9 治理门已 READY 但未落。
- 构造链提速项(B/C/A/D/T3)在真实整链上未实测:本版复用 0.0.54 链产物,耗时收益均为夹具或局部实测,整链为 UNMEASURED。
- BASE6 字节 oracle 未重建。
- p3 的 28 个 release-ctor case 材料未重建(DEFERRED),未记 PASS。
- 门内基线红漏检 7 次,靠集成者显式放行:pins、wt-gate、slice-inventory、tools-compile(三次)、refactor-tool、cx-procguard;cx-layers 的 holdframe 上行边仍在。
- 全域矩阵 release 档 15 红,需 CI 证据回填。
- 两代不动点、跨平台原生资格不在本版重新声称;s9 与 CI 结论见上表占位,未取得前不称 QUALIFIED。
