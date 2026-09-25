# 仓库地图：种子、可复用数据、自迭代

unisacc 分三层，外加验证与工具。本文是索引，逐个说明文件的角色；规格在 [`prd.md`](prd.md)，工作规则在 [`AGENTS.md`](AGENTS.md)，方法论在 [`research/`](research/)。

```
  ┌──────────────── 种子（第 0 代）：unisa/，全是 Python ─────────────────┐
  │  表与权重的构造器                        编译器种子                     │
  │  gold.py catalog.py ─construct─▶ 权重    front/ ir lower emit image vm │
  └──────────┬───────────────────────────────────────┬───────────────────┘
             │ build-weights / gold-export / emit-kernel
             ▼                                       │ 造出第 1 代
  ┌──── 可复用（与语言无关的数据）────┐                │
  │ weights/built.json  built.uns2    │                │
  │ weights/gold/*.tsv  真值表        │                ▼
  │ kernel/*.inc        C 形态        │ ─inf()─▶ ┌── 自迭代（第 1 代起）：C ──┐
  └───────────────────────────────────┘          │ src/*.c include/ → unisacc.c │
                                                 │ 编译自己：N1 = N2 = N3        │
                                                 └──────────────────────────────┘
        种子与第 1 代的输出逐字节相同，由 tests/ 保证（closure、stages、optpy …）
```

判别规则：**要运行才能得到结果的是程序（种子或迭代），拿来读的是数据（可复用）**。种子里造权重的脚本属于种子，它们造出来的东西才是可复用的。

## 1　可复用：数据，与语言无关

| 位置 | 内容 | 由谁写出 |
|---|---|---|
| `weights/gold/<阶段>.tsv` | **真值表本身**，一行一个键：先是键的各字段值，然后是每个输出头的类。18 个阶段共 8,484 行 | `python3 -m unisa gold-export`（`build-weights` 会顺带执行） |
| `weights/built.json` | 构造出的权重，JSON，便于阅读和调试 | `python3 -m unisa build-weights`（约 20 s） |
| `weights/built.uns2` | 同一份权重的紧凑二进制，发行时用 | 同上 |
| `kernel/unisa_model.inc` | 权重、各阶段维度、词表、编码器操作码表的 C 形态，自迭代编译器读的就是它 | `python3 -m unisa emit-kernel` |
| `kernel/unisa_headers.inc` | 随身携带的 C 库（`include/*.h` 原文） | 同上 |
| `kernel/unisa_cases.inc` | 每个阶段每个键的正确答案，供 C 端自测 | 同上 |

`tests/kernel.sh` 会把以上内容全部重新生成一遍，并要求与仓库里的逐字节相同。

### 文件格式

- **`.tsv`**：普通的制表符分隔文本，任何语言都能读。
- **`.uns2`**：本项目**自定义**的格式，不属于任何现成系统。名字取自 "UNISA net, version 2"，文件以 4 个字节 `UNS2` 开头。构造出的网络很稀疏：第一层只是每个单元对每个字段的一张位掩码，第二层是少量小整数，按稠密矩阵存储会大半是零，所以另定格式。完整布局写在 [`unisa/uns2.py`](unisa/uns2.py) 开头的注释里。全部权重合计约 9 KB。
- **`.unisa`**（`weights/*.f32.unisa`、`*.i8.unisa`）：同样是自定义格式（UNS1，稠密张量），属于 SGD 对照臂的旧权重，不在发布路径上。
- **`.inc`**：C 源码片段，经拼接进入 `unisacc.c`。每个文件开头列出它的输入文件和 sha256。

## 2　种子（第 0 代）：`unisa/`，全部是 Python

它的职责是**造出第一代**编译器和第一批权重，之后退为参考实现与回退手段（见 [`research/paper-c-intent.md`](research/paper-c-intent.md)）。它的每个表状决策都经 `oracle.ask`。

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

## 3　自迭代（第 1 代起）：C，由 unisacc 编译自己

| 文件 | 作用 |
|---|---|
| [`src/front_pp.c`](src/front_pp.c) | 词表工具、预处理器（`#if`、`#include`、按需补头、宏展开）、诊断、词法 |
| [`src/front_parse.c`](src/front_parse.c) | 解析与 tape 生成（walker）：类型、符号、表达式、调用、语句、初始化 |
| [`src/opt.c`](src/opt.c) | tape 优化器：H1 栈顶缓存、块级活跃性、`peep` 表 |
| [`src/main.c`](src/main.c) | 命令行 |
| [`src/back_lower.c`](src/back_lower.c) | 后端前半：解析 tape、向网络问 facts、lowering |
| [`src/back_encode.c`](src/back_encode.c) | arm64 与 x86-64 编码器、汇编器 |
| [`src/back_image.c`](src/back_image.c) | ELF/Mach-O/PE 写出、SHA-256 签名、Windows 导入、`-run` |
| [`include/*.h`](include/) | 随身携带的 C 库，以 `static` 定义，按需补头 |
| [`unisacc.c`](unisacc.c) | `kernel/` 与 `src/` 的拼接：单个 C 文件，cc 或 unisacc 都能直接编译，不需要 Python，也不读外部文件 |
| `unisacc.com` | `make com` 用 `-O2` 构建的六目标单文件发行版 |

[`iterate/construct/`](iterate/construct/) 是迭代层的**独立工具**（J10）：用 unisacc 能编译的 C 从 `weights/gold/*.tsv` 构造权重，由 unisacc 构建、单独运行，不进编译器的热路径。目前只覆盖 prec 和 reloc，与 Python 构造器逐字节对照（`iterate/construct/check.sh`）。

`src/` 的七个文件没有头文件，也不互相 `#include`：`tests/build_ref.sh` 按上表顺序把它们拼进 `unisacc.c`，顺序即依赖。

这一层只读第 1 节的数据，不依赖任何 `.py`。判据：`tests/nativeboot.sh`（N1 = N2 = N3，全程无 Python）、`tests/bigclosure.sh`（编译器自身在六个目标上与种子后端逐字节相同）。新逻辑优先落在表里或这一层；种子只做孪生检验。

## 4　验证与工具

| 位置 | 作用 |
|---|---|
| [`tests/`](tests/) | 套件：`all.sh` 汇总；`snap.sh` 在冻结快照上后台跑；单次 ≤ 60 s（AGENTS.md） |
| [`examples/`](examples/) · `tests/c/` · `tests/c99/` | 探针程序 |
| `corpus/` | 外部语料（c-testsuite、工具库），只读 |
| [`Makefile`](Makefile) | `make`（快检）、`make test`、`make com`、`make release` |
| README、`prd.tree.md`、`prd.map.md` 里的阶段表 | `python3 -m unisa docs` 生成，`tests/docs.sh` 检查 |
| [`archive/`](archive/) | 旧版 prd，只作历史 |
| `dist/` | 发布产物的暂存 |

## 5　布局上的取舍

- **种子不再细分目录**：构造器和编译器种子都留在 `unisa/`。它们被两个前端、两个后端、测试和 `ujs`（另一条产品线，直接 `import unisa.intnet`）共同引用，挪进子包会同时改动别人的代码。分类靠本文，不靠目录。**UJS 产品线**按同口径分了板块目录，见 [`ujs/ARCHITECTURE.md`](ujs/ARCHITECTURE.md)。
- **对照臂单独成包**（`unisa/control/`）：它不在发布路径上。
- **可复用层只放数据**：真值表有了 `.tsv` 形态，任何语言都能读取和核对，不必带上 Python。
- **生成物提交进仓库**：`kernel/` 与 `unisacc.c` 是自举的起点（见 AGENTS.md 的 Generated files 一节）。
