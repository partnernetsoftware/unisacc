# 仓库地图

**产品**是 C99 子集编译器 `unisacc.com`（六目标单文件）及随带的 `include/`。当前模型路线的驱动、通用执行器与推理核在 `exec/c/`，各阶段声明与构造器在 `exec/` 对应目录；`unisacc.c`（引用 `kernel/` + `src/` 的经典入口）是经典参考与显式回退。编译与运行不需要 Python，离线构造和容器打包仍用 Python。下面区分产品、参考、数据与开发工具：

| 类别 | 位置 | 说明 |
|---|---|---|
| 模型产品 | `exec/c/`、各阶段声明/构造器、`include/`、`unisacc.com` | 构造网络、通用运行核与用户使用的编译器 |
| 种子与参考实现 | `unisa/`、`src/`、`kernel/`、`unisacc.c` | Python 种子、经典 C 参考/回退、生成静态事实与容器打包；与模型路线对照 |
| 静态输入与数据 | `weights/`（`built.uns2`、`gold/*.tsv`）、`include/*.h` | 构造与生成的输入；真值表的数据形态 |
| 开发工具与参考裁判 | `iterate/`（C 写的权重构造器与 kernel 数据生成器，**不属于产品**）、`tests/`、各 `*_check.py` | 验证与复现，不进用户流程 |

规格在 [`prd.md`](prd.md)，工作规则在 [`AGENTS.md`](AGENTS.md)，论文在 [`research/`](research/)。

## 模型路线：发布基线与当前本地候选

`make com` 构建模型产品；`make classic-com` 把经典显式回退写到
`out/unisacc-classic.com`。模型拒绝不隐式改走经典路线。规范入口是
[prd §0.3 当前流水线](prd.md#pipeline-design)，当前字节账见
[模型功能与物理字节账](prd.md#model-function-bytes)。

| 身份 | 尺寸 / SHA256 / 验证边界 |
|---|---|
| 已发布 v0.0.8 / `10672e3` | 未签名 5,388,402 B；`948232f00028170d2090983375fbca2a3829ef8f73235baada5deb9db174d737`；发布收据与限制封存 |
| 已发布 v0.0.11 / `8b5abc9`（2026-09-29） | 未签候选 1,154,605 B；`6a3dfce28aa05ca474442ebe9e6fc4d07f4da7c15d1d3b5a6c21e91290f920c8`；签后 `unisacc.com` 1,170,384 B `e86cc61c…`；本地全量 338 项、release-check 与 GHCR 摘要实跑；验收 `research/r11-release-acceptance.json` |

六目标实跑：本机 osx/arm64 与 Rosetta 全量门禁；Lima lnx/arm64、lnx/x86_64 与 UTM win/arm64、win/x86_64 跑 examples（`tests/crossnative.sh`）并完成 Windows 双目标编译器自举；完整套件在客机与 Windows 上仍是 0.0.12 义务（R12-3）。macOS/Rosetta 本地门禁不等于六平台全绿。完整身份、
历史单次memory计时与收据见 [单次 memory 集成证据](research/memory-once-integrated-bench-20260928.json)。
32 个共享网络被 938 条阶段行引用；模板、库源码、核与平台驱动另计，
不能把引用数当网络物理份数，也不能把 CRC 当发行签名。

旧候选 6MB/171 项、Q 编码/177 项与各平台自举、失败及性能记录均保留在
[S-17 迁移档案](archive/s17-migration-log-20260928.md) 和
[原始最终证据](research/s17-final-evidence.json)，不作为当前候选验收。
固定包驱动自举不包含模型、整包或 APE 容器自构造。

prune 由 cc-unisacc 负责、待实现，不在当前生产路由中。企业签名尚待最终发布验收：
此前macOS app/dmg携带c499载荷已完成公司签名、公证/staple及Gatekeeper资格验证；
Windows Authenticode 单列验收。Mach-O ad-hoc 签名仅满足运行格式要求。

### 经典参考的种子与自举关系

以下图与第 1–3 节描述经典参考链，不是当前模型产品的运行流水线：

```
  ┌──────────────── 种子（第 0 代）：unisa/，全是 Python ─────────────────┐
  │  表与权重的构造器                        编译器种子                     │
  │  gold.py catalog.py ─construct─▶ 权重    front/ ir lower emit image vm │
  └──────────┬───────────────────────────────────────┬───────────────────┘
             │ build-weights / gold-export / emit-kernel
             ▼                                       │ 造出第 1 代
  ┌──── 可复用（跨实现的决策数据）────┐                │
  │ weights/built.json  built.uns2    │                │
  │ weights/gold/*.tsv  真值表        │                ▼
  │ kernel/*.inc        C 形态        │ ─inf()─▶ ┌──── 经典参考：C ──────┐
  └───────────────────────────────────┘          │ src/*.c include/ → unisacc.c │
                                                 │ 编译自己：N1 = N2 = N3        │
                                                 └──────────────────────────────┘
        种子与第 1 代的输出逐字节相同，由 tests/ 保证（closure、stages、optpy …）
```

判别规则：**要运行才能得到结果的是程序（种子或迭代），拿来读的是数据（可复用）**。种子里造权重的脚本属于种子，它们造出来的东西才是可复用的。

## 1　经典参考的可复用决策数据

| 位置 | 内容 | 由谁写出 |
|---|---|---|
| `weights/gold/<阶段>.tsv` | **真值表本身**，一行一个键：先是键的各字段值，然后是每个输出头的类。18 个阶段共 8,486 行 | `python3 -m unisa gold-export`（`build-weights` 会顺带执行） |
| `weights/built.json` | 构造出的权重，JSON，便于阅读和调试 | `python3 -m unisa build-weights`（约 20 s） |
| `weights/built.uns2` | 同一份经典参考权重的紧凑二进制 | 同上 |
| `kernel/unisa_model.inc` | 权重、各阶段维度、词表、编码器操作码表的 C 形态，经典参考读取它 | `python3 -m unisa emit-kernel` |
| `kernel/unisa_headers.inc` | 随身携带的 C 库（`include/*.h` 原文） | 同上 |
| `kernel/unisa_cases.inc` | 每个阶段每个键的正确答案，供 C 端自测 | 同上 |

`tests/kernel.sh` 会把以上内容全部重新生成一遍，并要求与仓库里的逐字节相同。

### 文件格式

- **`.tsv`**：普通的制表符分隔文本，任何语言都能读。
- **`.uns2`**：本项目**自定义**的格式，不属于任何现成系统。名字取自 "UNISA net, version 2"，文件以 4 个字节 `UNS2` 开头。构造出的网络很稀疏：第一层只是每个单元对每个字段的一张位掩码，第二层是少量小整数，按稠密矩阵存储会大半是零，所以另定格式。完整布局写在 [`unisa/uns2.py`](unisa/uns2.py) 开头的注释里。全部权重合计约 9 KB。
- **`.unisa`**（`weights/*.f32.unisa`、`*.i8.unisa`）：同样是自定义格式（UNS1，稠密张量），属于 SGD 对照臂的旧权重，不在发布路径上。
- **`.inc`**：生成的 C 数据片段，由经典入口引用；`kernel/weight.<stage>.inc` 与 `dense.<stage>.inc` 分别保存构造权重与参考答案。每个文件开头列出它的输入文件和 sha256。

## 2　种子（第 0 代）：`unisa/`，全部是 Python

它的职责是造出第一代编译器和权重，之后作为参考实现与回退手段：C 编译器的输出要与它逐字节一致。它的每个表状决策都经 `oracle.ask`。

**表与权重的构造器**

| 文件 | 作用 |
|---|---|
| [`gold.py`](unisa/gold.py) | 真值表的定义：每个阶段的键字段、输出头和标签规则。增删一张表就改这里 |
| [`catalog.py`](unisa/catalog.py) | 目标事实：系统调用号、调用约定、参数形状、返回约定、导入名；`ENCSPEC` 是编码器的操作码数据 |
| [`construct.py`](unisa/construct.py) | 由真值表构造精确的整数网络（最小覆盖、决策列表），不训练 |
| [`intnet.py`](unisa/intnet.py) | 构造网络的整数推理，以及 `build_all` |
| [`uns2.py`](unisa/uns2.py) | `.uns2` 的读写 |
| [`ckernel.py`](unisa/ckernel.py) | 把权重导出成 C（`kernel/`），附推理内核的源码 `KERNEL_BODY` |
| [`oracle.py`](unisa/oracle.py) | 唯一的询问入口 `ask(stage, key)`：断言键在定义域内（P-2），并统计询问次数 |
| [`linalg.py`](unisa/linalg.py) | 整数原语，构造器与 oracle 共用 |

**编译器种子**

| 部分 | 文件 |
|---|---|
| 前端 | [`front/pp.py`](unisa/front/pp.py) 预处理 · [`lex.py`](unisa/front/lex.py) 词法 · [`parse.py`](unisa/front/parse.py) 递归下降 walker · [`sema.py`](unisa/front/sema.py) 类型与符号（结构，不进表） |
| tape | [`tape.py`](unisa/tape.py) 与目标无关的指令流 · [`ir.py`](unisa/ir.py) 发射器 · [`fp.py`](unisa/fp.py) 浮点语义 · [`opt.py`](unisa/opt.py) `-O1/-O2`（C 优化器的孪生） |
| 后端 | [`lower.py`](unisa/lower.py) tape 到目标指令 · [`emit_x86.py`](unisa/emit_x86.py) · [`emit_arm.py`](unisa/emit_arm.py) · [`assemble.py`](unisa/assemble.py) · [`image/`](unisa/image/)（ELF、Mach-O、PE、fat） |
| 执行与打包 | [`vm.py`](unisa/vm.py) tape 参考解释器 · [`exec_target.py`](unisa/exec_target.py) 目标机解释器 · [`ape.py`](unisa/ape.py) 单文件 `unisacc.com` |
| 入口 | [`__main__.py`](unisa/__main__.py) 命令行 · [`driver.py`](unisa/driver.py) 源码到 tape · [`docgen.py`](unisa/docgen.py) 生成文档里的阶段表 |
| 对照臂 | [`control/`](unisa/control/)：SGD 训练与它的网络、权重格式，保留作负结果的证据，只有 `python3 -m unisa train` 用它 |

## 3　经典 C 参考与自举

| 文件 | 作用 |
|---|---|
| [`src/front_pp.c`](src/front_pp.c) | 词表工具、预处理器（`#if`、`#include`、按需补头、宏展开）、诊断、词法 |
| [`src/front_parse.c`](src/front_parse.c) | 解析与 tape 生成（walker）：类型、符号、表达式、调用、语句、初始化 |
| [`src/opt.c`](src/opt.c) | tape 优化器：H1 栈顶缓存、块级活跃性、`peep` 表 |
| [`src/main.c`](src/main.c) | 命令行 |
| [`src/version.h`](src/version.h) | 参考与模型驱动器共用的产品版本声明 |
| [`src/back_lower.c`](src/back_lower.c) | 后端前半：解析 tape、向网络问 facts、lowering |
| [`src/back_encode.c`](src/back_encode.c) | arm64 与 x86-64 编码器、汇编器 |
| [`src/back_image.c`](src/back_image.c) | ELF/Mach-O/PE 写出、SHA-256 签名、Windows 导入、`-run` |
| [`include/*.h`](include/) | 随身携带的 C 库，以 `static` 定义，按需补头 |
| [`unisacc.c`](unisacc.c) | 经典参考的有序 include 入口，不含权重字节；cc 或 unisacc 读取仓库依赖直接编译。独立传送/自举使用 `tests/export_ref.sh` 导出的完整源码，无 Python |
| `out/unisacc-classic.com` | `make classic-com` 构建的经典六目标显式回退；根 `unisacc.com` 由模型路线构建 |

[`iterate/`](iterate/) 是**开发工具**，不属于产品：`iterate/construct/` 用 C 从 `weights/gold/*.tsv` 构造权重（18 个阶段的 UNS2 整包与 `built.uns2` 逐字节相同），`iterate/kernel/` 用 C 从声明数据生成 `kernel/unisa_model.inc` 的大部分内容。它们由 unisacc 编译、单独运行，不进编译器的使用流程，也不是自举的前提；经典参考的 `kernel/` 仍由种子生成。这条迁移路线（prd 的 J10）已暂停，工具保留。

`unisacc.c` 是经典源码唯一的有序装配入口，引用 `src/version.h`、生成数据及各 C 模块。`tests/export_ref.sh` 递归展开为私有独立源码；`tests/build_ref.sh` 加宿主适配后编译该导出，不改写根入口。自举和规模门禁始终使用完整导出，不以入口行数代替编译器规模。产品33部署网络仍属于P3包，经典18张表的片段不是产品包的另一份权重。

这一层只读第 1 节的数据，不依赖任何 `.py`。判据：`tests/nativeboot.sh`（N1 = N2 = N3，全程无 Python）、`tests/bigclosure.sh`（编译器自身在六个目标上与种子后端逐字节相同）。经典容器打包使用 `python3 -m unisa ape`，各目标镜像由 unisacc 编译；当前模型路线另由 `exec/` 离线构造、验证和打包，用户编译路径不运行 Python。

## 4　验证与工具

| 位置 | 作用 |
|---|---|
| [`tests/`](tests/) | 套件：`all.sh` 汇总；`snap.sh` 在冻结快照上后台跑；单次 ≤ 60 s（AGENTS.md） |
| [`examples/`](examples/) · `tests/c/` · `tests/c99/` | 探针程序 |
| `corpus/` | 外部语料（c-testsuite、工具库），只读 |
| [`Makefile`](Makefile) | `make`（快检）、`make test`、`make com`、`make release` |
| README、`prd.tree.md`、`prd.map.md` 里的阶段表 | `python3 -m unisa docs` 生成，`tests/docs.sh` 检查 |
| [`archive/`](archive/) | 旧版 prd，只作历史 |
| `dist/` | `tests/release.sh --com` 保留的 `unisacc.com`（不提交） |

## 5　布局上的取舍

- **种子不再细分目录**：构造器和编译器种子都留在 `unisa/`。它们被两个前端、两个后端、测试和 `ujs`（另一条产品线，直接 `import unisa.intnet`）共同引用，挪进子包会同时改动别人的代码。分类靠本文，不靠目录。**UJS 产品线**按同口径分了板块目录，见 [`ujs/ARCHITECTURE.md`](ujs/ARCHITECTURE.md)。
- **对照臂单独成包**（`unisa/control/`）：它不在发布路径上。
- **可复用层只放数据**：真值表有了 `.tsv` 形态，任何语言都能读取和核对，不必带上 Python。
- **生成物提交进仓库**：`kernel/` 与 `unisacc.c` 是自举的起点（见 AGENTS.md 的 Generated files 一节）。
