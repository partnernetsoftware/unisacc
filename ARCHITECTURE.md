# 仓库地图：种子、核心表与权重、自迭代

unisacc 的每一部分都属于以下三类之一，另有生成物与验证两类配套。本文是索引，逐个说明文件的角色；规格在 [`prd.md`](prd.md)，工作规则在 [`AGENTS.md`](AGENTS.md)，方法论在 [`research/`](research/)。

```
            ┌──────────── 核心：表与权重 ────────────┐
            │  unisa/gold.py  unisa/catalog.py        │  真值表（唯一的事实来源）
            │        │ build-weights（构造，不训练）   │
            │        ▼                                │
            │  weights/built.*  ──emit-kernel──▶ kernel/*  │
            └───────┬───────────────────────┬─────────┘
                    │ oracle.ask             │ inf()
          ┌─────────▼─────────┐     ┌────────▼──────────────┐
          │ 种子（第 0 代）     │     │ 自迭代（第 1 代起）      │
          │ unisa/ 的 Python   │ ──▶ │ src/*.c → unisacc.c    │
          │ 前端、后端、VM      │ 造出 │ 编译自己：N1 = N2 = N3  │
          └───────────────────┘     └───────────────────────┘
                 两边输出逐字节相同由 tests/ 保证（closure、stages、optpy …）
```

## 1　核心：表与权重（可复用，与语言和实现无关）

表状决策（有限键 → 有限类）的唯一来源。两边的编译器都只**询问**它们，从不各自复制一份规则。

| 文件 | 作用 |
|---|---|
| [`unisa/gold.py`](unisa/gold.py) | **真值表**：每个阶段的键字段、输出头与标签函数（`STAGES`、`ALL`）。增删一张表就是改这里 |
| [`unisa/catalog.py`](unisa/catalog.py) | 目标事实的目录：系统调用号、调用约定、门、参数形状、返回约定、导入名；`ENCSPEC`（编码器的操作码数据，不是决策） |
| [`unisa/construct.py`](unisa/construct.py) | 由真值表**构造**精确整数网络（最小覆盖、决策列表），不训练 |
| [`unisa/intnet.py`](unisa/intnet.py) | 构造网络的整数推理（Python 侧），以及 `build_all` |
| [`unisa/uns2.py`](unisa/uns2.py) | 构造权重的序列化格式（`weights/built.uns2`） |
| [`unisa/oracle.py`](unisa/oracle.py) | 唯一的询问入口 `ask(stage, key)`：定义域断言（P-2）与询问计数 |
| [`unisa/ckernel.py`](unisa/ckernel.py) | 把权重与词表导出成 C（`kernel/`），并附整数推理内核的源码（`KERNEL_BODY`） |
| [`weights/built.json`](weights/) · `built.uns2` | 构造出的权重（产物，`python3 -m unisa build-weights` 重建，约 20 s） |

验证：`python3 -m unisa acc`（全域枚举 1.000）、`tests/gold_audit.py`（表对照 cc）、`tests/abi_audit.py`（系统调用号对照系统头）。

## 2　种子（第 0 代，Python）

用来**造出第一代**编译器，此后退为参考实现与回退手段（见 [`research/paper-c-intent.md`](research/paper-c-intent.md)）。它的每个表状决策都经 `oracle.ask`。

| 部分 | 文件 |
|---|---|
| 前端 | [`unisa/front/pp.py`](unisa/front/pp.py) 预处理 · [`lex.py`](unisa/front/lex.py) 词法 · [`parse.py`](unisa/front/parse.py) 递归下降 walker · [`sema.py`](unisa/front/sema.py) 类型与符号（结构，不进表） |
| tape | [`unisa/tape.py`](unisa/tape.py) 与目标无关的指令流 · [`unisa/ir.py`](unisa/ir.py) tape 发射器 · [`unisa/fp.py`](unisa/fp.py) 浮点语义 · [`unisa/opt.py`](unisa/opt.py) `-O1/-O2`（C 优化器的孪生） |
| 后端 | [`unisa/lower.py`](unisa/lower.py) tape → 目标指令 · [`emit_x86.py`](unisa/emit_x86.py) · [`emit_arm.py`](unisa/emit_arm.py) · [`assemble.py`](unisa/assemble.py) · [`image/`](unisa/image/)（ELF、Mach-O、PE、fat） |
| 执行与打包 | [`unisa/vm.py`](unisa/vm.py) tape 参考解释器 · [`exec_target.py`](unisa/exec_target.py) 目标机解释器 · [`ape.py`](unisa/ape.py) 单文件 `unisacc.com` |
| 入口 | [`unisa/__main__.py`](unisa/__main__.py) CLI · [`driver.py`](unisa/driver.py) src → tape · [`docgen.py`](unisa/docgen.py) 生成文档里的阶段表 |
| 共用原语 | [`linalg.py`](unisa/linalg.py)：内核的整数原语，构造器与 oracle 都用 |
| 对照臂（不在发布路径上） | [`unisa/control/`](unisa/control/)：`train.py` · `net.py` · `segment.py` · `rng.py` · `uns1.py`，SGD 训练与其权重格式，保留作负结果的证据；只有 `python3 -m unisa train` 用它 |

## 3　自迭代（第 1 代起，C，由 unisacc 编译自己）

| 文件 | 作用 |
|---|---|
| [`src/unisacc_main.c`](src/unisacc_main.c) | 前端（预处理、词法、walker）、tape 优化器（H1 栈顶缓存、块级活跃性、`peep`）、命令行 |
| [`src/unisacc_back.c`](src/unisacc_back.c) | 后端：lowering、x86-64 与 arm64 编码器、ELF/Mach-O/PE 写出、`-run` |
| [`include/*.h`](include/) | 编译器随身携带的 C 库（以 `static` 定义，按需补头） |
| [`unisacc.c`](unisacc.c) | `kernel/` 与 `src/` 的拼接：**单个 C 文件**，cc 或 unisacc 都能直接编译，不需要 Python、不读外部文件 |

判据：`tests/nativeboot.sh`（N1 = N2 = N3，全程无 Python）、`tests/bigclosure.sh`（编译器自身在六个目标上与参考后端逐字节相同）。新逻辑优先落在表里或这里，由 unisacc 自己迭代；Python 端只作孪生检验。

## 4　生成物（不要手改）

| 位置 | 由谁生成 |
|---|---|
| [`kernel/`](kernel/) | `python3 -m unisa emit-kernel`；每个文件开头列出输入及 sha256，`tests/kernel.sh` 检查是否过期 |
| `unisacc.c` | `tests/build_ref.sh`（拼接） |
| README、`prd.tree.md`、`prd.map.md`、论文表 1 的阶段表 | `python3 -m unisa docs`；`tests/docs.sh` 检查 |
| `unisacc.com` | `make com`（`-O2`） |

## 5　验证与工具

| 位置 | 作用 |
|---|---|
| [`tests/`](tests/) | 套件：`all.sh` 汇总；`snap.sh` 在冻结快照上后台跑；每个套件单次 ≤ 60 s（AGENTS.md） |
| [`examples/`](examples/) · `tests/c/` · `tests/c99/` | 探针程序 |
| `corpus/` | 外部语料（c-testsuite、工具库），只读 |
| [`Makefile`](Makefile) | `make`（快检）、`make test`、`make com`、`make release` |
| [`archive/`](archive/) | 旧版 prd，只作历史 |
| `dist/` | 发布产物的暂存 |

## 6　布局上的取舍

- **对照臂单独成包**（`unisa/control/`）：它不在发布路径上，只被 `python3 -m unisa train` 与 oracle 的一个常量引用，挪开后 `unisa/` 顶层只剩种子与核心。
- **核心表模块留在原路径**（`unisa/gold.py` 等）：它们被两个前端、两个后端、测试和 `ujs/construct`（另一条产品线，直接 `import unisa.intnet`）共同引用。挪进子包会同时改动别人的代码，收益只是目录好看，所以用本文的分类代替目录分类。
- **生成物提交进仓库**：`kernel/` 与 `unisacc.c` 是自举的起点，编译它们不需要 Python（见 AGENTS.md 的 Generated files 一节）。
