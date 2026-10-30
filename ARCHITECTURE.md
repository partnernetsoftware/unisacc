# 仓库地图

**0.0.43当前入口（2026-10-25重规划）：** 旧测试树与旧门禁已移入
`archive/testing-c43-legacy/`。新套件由`tests/run.cx`直接默认-run；当前smoke
只有16项局部断言，完整验收仍明确NOT_READY。产品构造支持独立在`tools/`，
其中`tools/sourceflat.cx`已原生替代源码导出逻辑。下文涉及历史探针的条目仅描述来源，
不能据此恢复旧队列或借用旧通过结果。

**产品**是 C99 编译器 `unisacc.com`（C99 = ISO/IEC 9899:1999 含 TC1–TC3，文本 WG14 N1256；条款覆盖账本见 archive/plans/v0.0.14.md R14-6；`#line` 主文件内已支持（0.0.14）；随带 C 库是有清单的子集，见 README 限制表）（六目标单文件）及随带的 `include/`。编译器入口是 `src/unisacc.cx`（六层，见第 2 节），由 `tools/entry-build.cx` 构造；各阶段声明与构造器在 `exec/` 对应目录。原 Python 种子 `unisa/` 已归档到 `archive/reference-py/unisa/`（清单见 [archive/reference-py/README.md](archive/reference-py/README.md)），经典 C 参考（`unisacc.c`、`src/*.c`，以及 `exec/c/` 的驱动与运行核 C 源如 `compiler.c`、`asmcompiler.c`、`run.c`）已归档到 `archive/reference-c/`（见 [archive/reference-c/README.md](archive/reference-c/README.md)），两者只供对照，活路径不回调。生成表与 `kernel/` 改由第 3 节的 CX 工具写出；活路径上剩余的 Python 见 [plans/python-residue-2026-10-30.md](plans/python-residue-2026-10-30.md)。下面区分产品、参考、数据与开发工具：

| 类别 | 位置 | 说明 |
|---|---|---|
| 模型产品 | `src/unisacc.cx` 及 `src/` 六层、`cx/`、各阶段声明/构造器（`exec/`）、`include/`、`unisacc.com` | 入口、通用执行器、构造网络与用户使用的编译器 |
| 生成数据与归档参考 | `kernel/`（生成物）、`archive/reference-py/`、`archive/reference-c/` | `kernel/` 由 `tools/kernel-emit.cx` 等写出，禁止手改；Python 种子与经典 C 参考已归档，只供对照 |
| 静态输入与数据 | `weights/`（`built.uns2`、`gold/*.tsv`）、`include/*.h` | 构造与生成的输入；真值表的数据形态 |
| 开发工具与参考裁判 | `tools/`构造支持、`tests/`新原生合同、`iterate/`构造器；旧套件在`archive/testing-c43-legacy/` | 工具与测试分离；现有Python协调器的债务不因迁目录消失 |
| 文档、计划与历史 | `docs/`（`tapebin-v1.md` 格式契约、`tape-vs-wasm.md`、`dogfood-targets.md`；`docs/uxe/` 与 `docs/index.html` 属 ujs 子项目，不在本产品范围）、[prd.md](prd.md)（唯一主入口；[§2当前计划](prd.md#current-plans)、[§3七流/十八网/进度](prd.md#flow-net-contracts)、§5路线、§6问题）与 `spec.md`（条款全文，编号稳定）、`plans/v0.0.43.md`（当前执行计划，唯一索引在 `prd.md` §2；路线/构想摘要合入 §5，旧稿归 `archive/plans/`）、`tests/INVENTORY.md`（每个测试/检查脚本在哪个门禁，`script-inventory` 门禁保证无孤儿）、`research/`（论文与仍被引用的证据）、`archive/`（已发布版本的计划、prd 叙事与证据，按版本分目录） | 只读参考；发布后把本版计划与证据移入 `archive/` |

`.tapebin` 的格式见 [v1 契约](docs/tapebin-v1.md)：原种子编解码器
（`archive/reference-py/unisa/tapebin.py`）与经典参考读器
（`archive/reference-c/src/tapebin.c`）均已归档；
`src/tapebin_encode.inc` 仍在活路径上作容器编码，由
`exec/c/tapebin_emit.h` 引用；`exec/c/tapebin.h`、`exec/c/tapebin_emit.h` 是模型驱动的容器适配层。
后端编译决策仍由路由网络执行。

当前状态与路线在 [`prd.md`](prd.md)，条款全文在 [`spec.md`](spec.md)，工作规则在 [`AGENTS.md`](AGENTS.md)，论文在 [`research/`](research/)。

## 模型路线：发布基线与当前本地候选

`Makefile` 的 `make com` 仍走 `exec/c/buildcompiler.sh`，该链因 Python 与参考 C
归档已起不来（见 [python-residue](plans/python-residue-2026-10-30.md) 第 5 行）；经典回退
`classic-com` 已不在 `Makefile` 中。新入口换成产品的步骤见
[candidate-to-product](plans/candidate-to-product-2026-10-30.md)。模型拒绝不隐式改走经典路线。规范入口是
[prd §0.3 当前流水线](prd.md#pipeline-design)，当前字节账见
[模型功能与物理字节账](prd.md#model-function-bytes)。

| 身份 | 尺寸 / SHA256 / 验证边界 |
|---|---|
| 已发布版本及其字节 | **不在本文件重述**：权威表在 [README 的 artifact 行](README.md)与 [prd §2 计划索引](prd.md#current-plans)，每个版本带签后大小、SHA-256 与验收收据 |

平台验收按当前候选分别记证，完整状态见 [PRD 思维树](prd.md#work-tree)，发布证据边界见 [发布流水线](release/RELEASE-PIPELINE.md)。历史 macOS/Rosetta、Lima/UTM 实跑与 Windows 双目标自举只证明当时的输入与环境，不代表当前候选六平台通过。
历史单次 memory 计时、身份与收据见 [单次 memory 集成证据](archive/research/20260928/memory-once-integrated-bench-20260928.json)及下列 S-17 迁移档案。
共享网络数与阶段行数**不在此重述**：权威是 [prd 的模型字节账](prd.md#model-function-bytes)（生成自
`research/model-bytes.json`）；模板、库源码、核与平台驱动另计，
不能把引用数当网络物理份数，也不能把 CRC 当发行签名。

旧候选 6MB/171 项、Q 编码/177 项与各平台自举、失败及性能记录均保留在
[S-17 迁移档案](archive/s17-migration-log-20260928.md) 和
[原始最终证据](archive/research/20260928/s17-final-evidence.json)，不作为当前候选验收。
固定包驱动自举不包含模型、整包或 APE 容器自构造。

prune 是生产路由中的一个构造网络阶段，位于 lowering 之前
（`exec/pipeline/image-stages.tsv`：`prune  tape.text  tape.text  prune.net`；
实现与检查在 `exec/prune/`）。企业签名尚待最终发布验收：
此前macOS app/dmg携带c499载荷已完成公司签名、公证/staple及Gatekeeper资格验证；
Windows Authenticode 单列验收。Mach-O ad-hoc 签名仅满足运行格式要求。

### 数据与生成器的关系

旧图（Python 种子 → 经典 C 参考 → 自举）描述的两端都已归档，见第 3 节末尾。现在的关系是：

```
  ┌──── 可复用（跨实现的决策数据，拿来读）────┐
  │ weights/gold/*.tsv   真值表               │
  │ weights/built.json   built.uns2  权重     │
  │ exec/facts/*.tsv     事实表               │
  │ include/*.h          随身携带的 C 库      │
  └──────────────┬────────────────────────────┘
                 │ tools/*.cx，一律 sh ./unisacc.com FILE.cx -- ARGS
                 ▼
  gold-table.cx --check           核真值表
  facts-gen.cx --check|--write    exec/facts 的 30 张生成表
  kernel-emit.cx --check|--write  kernel/ 的 41 个文件（生成物，禁止手改）
  carried-header-export.cx        kernel/unisa_headers.inc
  cx-flatten.cx --check|--write   include/cx-nested.h → include/cx-flat.h
```

判别规则：**要运行才能得到结果的是程序（工具或迭代），拿来读的是数据（可复用）**。造数据的工具属于程序，它们造出来的东西才是可复用的。

## 1　可复用决策数据

| 位置 | 内容 | 由谁写出 |
|---|---|---|
| `weights/gold/<阶段>.tsv` | **真值表本身**，一行一个键：先是键的各字段值，然后是每个输出头的类。经典事实网的数量与键数见 [PRD 术语表](prd.md#0-概念与术语先读歧义以本节为准)及 [README 真值表](README.md) | `tools/gold-table.cx --check ROOT` 核定义域、类与列行；有规则形 `seed/gold_<阶段>_label.c` 的 11 张可用 `--write ROOT STAGE` 重写。原 `python3 -m unisa gold-export` 已归档 |
| `weights/built.json` | 构造出的权重，JSON，便于阅读和调试 | 冻结数据：原构造器（`archive/reference-py/unisa/construct.py`、`intnet.py`）已归档，整份文件目前没有原生写出者。单网私有重构造走 `tools/gold-partition-material.cx` 与 `tools/net-built-private.cx`，只写私有目录 |
| `weights/built.uns2` | 同一份经典参考权重的紧凑二进制 | 冻结数据；C 开发工具 `iterate/construct/construct.c -u` 可从真值表写出 UNS2，见 [iterate/construct/README.md](iterate/construct/README.md) |
| `exec/facts/*.tsv` 中的 30 张生成表 | 编码、镜像、tape 形状等事实（`exec/facts/` 下其余 `.tsv` 不由它写） | `tools/facts-gen.cx --write ROOT [TABLE...]`，`--check` 另核 bank 归属；每张表头注明写出者与输入 sha256。原 Python 写出者已归档到 `archive/reference-py/exec/facts/export.py` |
| `kernel/unisa_model.inc`、`unisa_cases.inc`、`unisa_core.c`、`unisa_self.c`、`weight.<阶段>.inc`、`dense.<阶段>.inc` | 权重、维度、词表、编码器操作码表、每键正确答案与推理核的 C 形态 | `tools/kernel-emit.cx --write ROOT OUTDIR`（`--check` 逐文件报 SAME/DIFF）；输入是 `weights/`、`exec/facts/kernel-emit.tsv`、`enc-literals.tsv`、`exec/kernel/body.c` 与 `include/` |
| `kernel/unisa_headers.inc` | 随身携带的 C 库（`include/*.h` 原文与 `-ftrim-libc` 闭包） | 仓内这份由 `tools/carried-header-export.cx`（`prepare`／`--verify`／`install`）写出，文件头记有该工具；`kernel-emit` 也写同名文件 |
| `include/cx-flat.h` | `.cx` 的单文件前导（`include/cx.h` 在 `__UNISA__` 下读它） | `tools/cx-flatten.cx --write ROOT`／`--check ROOT`，源是 `include/cx-nested.h` |

原 `tests/kernel.sh`（归档于 `archive/testing-c43-legacy/tests/kernel.sh`）的再生成对比，现由上表各工具的 `--check` 承担。仓内 `kernel/` 41 个文件中有 40 个的溯源注释仍写着 `python3 -m unisa emit-kernel`：这些已提交字节还没有用 `kernel-emit --write` 重写过。

### 文件格式

- **`.tsv`**：普通的制表符分隔文本，任何语言都能读。
- **`.uns2`**：本项目**自定义**的格式，不属于任何现成系统。名字取自 "UNISA net, version 2"，文件以 4 个字节 `UNS2` 开头。构造出的网络很稀疏：第一层只是每个单元对每个字段的一张位掩码，第二层是少量小整数，按稠密矩阵存储会大半是零，所以另定格式。完整布局写在已归档的 [`archive/reference-py/unisa/uns2.py`](archive/reference-py/unisa/uns2.py) 开头的注释里。全部权重合计约 9 KB。
- **`.unisa`**（`weights/*.f32.unisa`、`*.i8.unisa`）：同样是自定义格式（UNS1，稠密张量），属于 SGD 对照臂的旧权重，不在发布路径上；对照臂代码已归档在 `archive/reference-py/unisa/control/`。
- **`.inc`**：生成的 C 数据片段（原由已归档的经典入口引用）；`kernel/weight.<stage>.inc` 与 `dense.<stage>.inc` 分别保存构造权重与参考答案。每个文件开头列出它的输入文件和 sha256。

## 2　编译器入口：`src/unisacc.cx`，六层

[`src/unisacc.cx`](src/unisacc.cx) 只有 `main`，经 `#pragma unisa use drive` 交给 `dv_main`。六层同在 `src/`，每层是一个 `#pragma unisa module`：

| 层 | 文件 | 作用（据各文件开头注释） |
|---|---|---|
| host | [`src/host.cx`](src/host.cx) | 宿主文件与时钟；`mmap`、`mprotect` 与装载器符号留在这里。它是唯一 include 宿主头的层（`<sys/mman.h>`、`<dlfcn.h>`、`<fcntl.h>`），在 `tests/cxraw-layers.tsv` 里算 L0 |
| packmodel | [`src/packmodel.cx`](src/packmodel.cx) | 找到已装容器的包尾并解码包目录；模型体留给要它的阶段 |
| executor | [`src/executor.cx`](src/executor.cx) | 模型生命周期与解码，调用汇编执行核 `core_run`（`core.kernel`，即 `cx/core/kernel.cx`） |
| stages | [`src/stages.cx`](src/stages.cx) | 按包内顺序跑一条路由：逐段装载、`core_run` 执行、把字节交给下一段；资源只经 `core_host_fetch` |
| drive | [`src/drive.cx`](src/drive.cx) | 命令行与路由；布局对应 `cx/driver/cli.cx`、`compat.cx`、`route.cx` |
| bind | [`src/bind.cx`](src/bind.cx) | 映像文件与内存入口的绑定；这一步还不写映像 |

`src/` 里其余 `.cx`（`driveface`、`driveroute`、`entrybind`、`entryexec`、`hostnative`、`runcache` 等）以及软链 `src/driver`、`src/runtime`、`src/core`、`src/compiler`（指向 `cx/` 同名目录）是入口用到的其他对象。完整对象清单（33 个）写在 [`tools/entry-build.cx`](tools/entry-build.cx) 里。`src/version.h` 是产品版本声明。

**构造**：`sh ./unisacc.com tools/entry-build.cx -- ROOT P3 OUT [CONSTRUCTOR] [--target T] [--only KEY]... [--link-only]`。每个对象与链接都在 `tools/bound` 下跑；产物是 `OUT/candidate`（链接出的映像后接 P3 包），收据是 `OUT/receipt.txt`。它不替换 `ROOT/unisacc.com`，也不调用旧构造脚本。五个目标切片、APE 封装与安装的步骤见 [candidate-to-product](plans/candidate-to-product-2026-10-30.md)；该计划写明截至 2026-10-30 入口候选只覆盖 lnx/x86_64，尚未换成产品。当前状态以 [prd.md](prd.md) 的思维树为准。

## 3　CX 工具与分层

**生成器**。每个都用 `sh ./unisacc.com tools/X.cx -- ARGS` 直接运行，用法以各文件开头注释为准：

| 工具 | 写出或核对 | 用法 |
|---|---|---|
| [`tools/facts-gen.cx`](tools/facts-gen.cx) | `exec/facts/` 的 30 张生成表 | `--check ROOT [TABLE...]`（全表时另核 bank 归属）、`--write ROOT [TABLE...]`、`--compare-body ROOT TABLE` |
| [`tools/kernel-emit.cx`](tools/kernel-emit.cx) | `kernel/` 的 41 个文件（18 阶段的 `weight`/`dense`，加 `unisa_model.inc`、`unisa_cases.inc`、`unisa_core.c`、`unisa_self.c`、`unisa_headers.inc`） | `--write ROOT OUTDIR`、`--check ROOT OUTDIR`，可加 `--built BUILT_JSON` |
| [`tools/gold-table.cx`](tools/gold-table.cx) | `weights/gold/*.tsv` | `--check ROOT`、`--write ROOT STAGE` |
| [`tools/carried-header-export.cx`](tools/carried-header-export.cx) | `kernel/unisa_headers.inc`（随身携带的头） | `prepare ROOT NEW_MAT LIBNEED_NATIVE`、`--verify ROOT MAT`、`install ROOT MAT`；本机 libneed 程序是显式的宿主 C 信任输入 |
| [`tools/cx-flatten.cx`](tools/cx-flatten.cx) | `include/cx-flat.h`，由 `include/cx-nested.h` 展开所有 `#include` 得到 | `--write ROOT`、`--check ROOT` |
| [`tools/entry-build.cx`](tools/entry-build.cx) | 编译器入口候选（见第 2 节） | `ROOT P3 OUT [CONSTRUCTOR] ...` |

`kernel/` 是生成物，禁止手改；改动先改事实或真值，再经生成器写出，并用 `--check` 核对。

**分层**（[plans/cx-layers.md](plans/cx-layers.md)）：

| 层 | 路径 | 可直接调用 libc/POSIX |
|---|---|---|
| L0 宿主对接 | `cx/text/`、`cx/mem/`、`cx/sys/`，以及 `tests/cxraw-layers.tsv` 列出的适配器（如 `src/host.cx`、`src/hostnative.cx`、`cx/core/kernel.cx`） | 是 |
| L1 机制 | `cx/core/`、`cx/runtime/` | 否，经 L0 |
| L2 驱动与编译器 | `cx/driver/`、`cx/compiler/`、`src/`（除 L0 文件） | 否 |
| L3 工具、测试、应用 | `tools/`、`tests/`、`apps/` | 否 |

使用方经 `#pragma unisa use text.text`、`mem.arena`、`sys.fs` 等引用 L0 包；`include/cx-nested.h` 里的 `cx_*` 帮手（`cx_read`、`cx_write`、`cx_run*`、`cx_fmt` 等）原地算 L0 接口。L0 各包的检查是 `tools/check-cx-text.cx`、`tools/check-cx-arena.cx`、`tools/check-cx-sys.cx`。

**`#include` 边界**：新 `.cx` 不手写 `#include`。只有 `include/`、`tools/cxcc-hostinc/` 与 `cx/sys/` 可以用，白名单是 [`tests/cxinclude-boundary.tsv`](tests/cxinclude-boundary.tsv)；白名单外的旧债记在 `tests/cxinclude.tsv`，只降不升。`src/host.cx` 的 3 个 `#include` 怎么处置尚未定，见 [python-residue](plans/python-residue-2026-10-30.md) 的“只有底层能 include”一节。

**棘轮**（各自基线只降不升，新文件为 0）：

| 检查 | 基线 | 计数对象 |
|---|---|---|
| [`tests/cxraw.cx`](tests/cxraw.cx) | `tests/cxraw.tsv` | L1–L3 每个 `.cx` 里直接调用的 libc/POSIX 函数 |
| [`tests/cxinclude.cx`](tests/cxinclude.cx) | `tests/cxinclude.tsv` | 模块 `.cx` 里的 `#include` 行 |
| [`tests/cxapis.cx`](tests/cxapis.cx) | `tests/cxapis.tsv` | `strcpy`、`strcat`、`sprintf`、`gets` 的调用 |
| [`tests/cxtrunc.cx`](tests/cxtrunc.cx) | `tests/cxtrunc.tsv` | 丢弃返回值、长度可随输入变化的 `snprintf`、`strncpy`、`memcpy` |

用法如 `sh ./unisacc.com -run tests/cxinclude.cx -- --check`。

**旧实现已归档**：Python 种子 `unisa/` 在 `archive/reference-py/unisa/`，连同依赖它的旧脚本（清单与归档依据见 [archive/reference-py/README.md](archive/reference-py/README.md)）；经典 C 参考 `unisacc.c`、`src/*.c` 与 `exec/c/` 的驱动和运行核 C 源在 `archive/reference-c/`（`exec/c/` 里仍留有检查与探针用的 `.c` 及头文件）（见 [archive/reference-c/README.md](archive/reference-c/README.md)）。两者都按原相对路径存放，只供对照，活路径不得回调。旧自举判据 `nativeboot`、`bigclosure` 等随旧测试树归档在 `archive/testing-c43-legacy/tests/`，不授权新入口。

[`iterate/`](iterate/) 仍是 C 写的**开发工具**，不属于产品：`iterate/construct/` 从 `weights/gold/*.tsv` 构造权重，`iterate/kernel/` 从声明数据生成 `kernel/unisa_model.inc` 的大部分内容。这条迁移路线（prd 的 J10）已暂停，工具保留。

## 4　验证与工具

| 位置 | 作用 |
|---|---|
| [`tests/`](tests/) | 全新模块化CX合同入口`run.cx`：`sh ./unisacc.com tests/run.cx -- --smoke\|--all\|--list`；注册表 `tests/batches.tsv`、覆盖清单 `tests/coverage.tsv`；`--smoke`局部，`--all`缺覆盖时拒绝；单次≤60秒。CX 棘轮见第 3 节 |
| [`examples/`](examples/) · `archive/testing-c43-legacy/tests/c/` · `archive/testing-c43-legacy/tests/c99/` | 示例与历史探针；历史夹具须按合同迁移 |
| `corpus/` | 外部语料（c-testsuite、工具库），只读 |
| [`Makefile`](Makefile) | `make`／`make smoke`（`tests/run.cx --smoke`）、`make test`（`--all`）、`make list`、`make export-ref`（`tools/sourceflat.cx`）；`make com` 仍调 `exec/c/buildcompiler.sh`，已起不来；`make release` 直接以 NOT_READY 退出 3 |
| README 里的阶段表 | 冻结文本：原生成器 `archive/reference-py/unisa/docgen.py` 与检查 `archive/testing-c43-legacy/tests/docs.sh` 已归档；原生一致性合同待实现 |
| [`archive/`](archive/) | 旧版 prd、已发布版本的计划与证据，以及 `reference-py/`、`reference-c/`、`testing-c43-legacy/` 三处旧实现与旧测试，只作历史 |
| `dist/` | 发行材料，不提交；新发布收据和跨平台闸仍待重建 |

## 5　布局上的取舍

- **旧实现整体归档、相对路径不变**：Python 种子（含对照臂 `unisa/control/`）与经典 C 参考按原路径挪进 `archive/reference-py/`、`archive/reference-c/`，对照时按同一相对路径找；活路径不回调归档。**UJS 产品线**按同口径分了板块目录，见 [`ujs/ARCHITECTURE.md`](ujs/ARCHITECTURE.md)。
- **可复用层只放数据**：真值表与事实表都是 `.tsv`，任何语言都能读取和核对；原 Python 字面量已移入事实文件（如 `exec/facts/kernel-emit.tsv`、`exec/kernel/body.c`、`exec/kernel/gold-tuples.tsv`）。
- **生成物提交进仓库**：`kernel/` 与 `include/cx-flat.h` 由第 3 节的工具写出并提交；`kernel/` 禁止手改（见 AGENTS.md）。
