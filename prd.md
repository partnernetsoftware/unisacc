# unisacc —— 产品与项目总览

本文回答五个问题：unisacc.com **是什么**、**有什么功能**、**结构组成**、**测试套件怎么设计**、**项目目录与文件结构**。术语歧义以 §0 为准；历史流水与逐版收口在 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md)（完整旧版），不在这里重复。

## 0. 概念与术语（先读；歧义以本节为准）

「流 / 网 / 段 / 阶段」四个词各只有一个意思。详细版与逐处核实见 [docs/glossary.md](docs/glossary.md)。

| 层 | 唯一名 | 含义 | 数量 | 真源 |
|---|---|---|---|---|
| L1 | **Stream**（别名 Flow） | 字节管道阶段：输入格式 → 输出格式 | unisacc **7**（e2/e1/e3/e4/prune/lower/elf） | `exec/pipeline/image-stages.tsv` |
| L2 | **Net** | ask 事实网：一张有限表 + 全键穷举 exact | **18**（8770 键） | `weights/gold/*.tsv`（权威；规则形式 `seed/gold_<阶段>_label.c`，由 `tools/gold-table.cx --check` 核对） |
| L3 | **Segment** | 训练合并段（多网合一的 masked 多任务模型） | 3（s1/s2/s3） | 负结果证据已归档 |
| L4 | **Book phase** | 一次推导中 ask 的记账阶段 | 由 contract 声明 | `flownet/contract.cx` |
| L5 | **Release phase** | 发布流程阶段 | 10 | 已归档 |

**六条常见误解**（不要按直觉读）：

1. 「7 流」与「18 网」不是同一批东西的两种叫法——流是**字节管道阶段**，网是 **ask 事实表**。
2. `class Stage`（旧 `unisa/gold.py`）表示**网**，不是流。
3. 旧 `unisa/control/net.py` 的 `Net` 是**神经网络模型**，不是事实网。
4. m4 的「4 流」是**四个字节段**（lex/front/select/encode），不是 4 张决策网。
5. `phase` 三义：book phase（记账）/ release phase（发布）/ 历史 compiler phase（实为 segment）。
6. `STAGE_NETS` 只是 isel、abi 两张不在 `TABLES` 里的网（15+2+1=18）。

**禁用写法**：裸 `stage` 一律写 `stream-stage`（流）或 `net`（网）；用 `net` 指神经网络一律写 `model`。

**RI 的固定含义**：本文 RI = **ResourceInput（具名资源输入）**，含名称、字节和长度、存在性/空值及跨流身份绑定，不是 "runtime integrity"。

## 1. unisacc.com 是什么

**一个文件** `unisacc.com`（APE，即 αcτµαlly pδrταblε εxεcµταblε），一个文件同时是「编译器」和「程序」，但「它能写出的目标」和「它自己运行在哪些原生宿主」是两回事：

- **写出六目标程序**（真）：能编出 {Linux, macOS, Windows} × {x86-64, arm64} 六个目标的可执行文件（ELF / Mach-O / PE），**映像不经汇编器和链接器**（模型路由直接编码写出）。
- **自身运行在五原生切片 + win/arm64 系统模拟**（注意）：文件里只有 5 个原生执行切片（win/x86_64、lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64），**没有 win/arm64 原生切片**——Windows on ARM 上靠 win/x86_64 PE 的系统模拟运行。win/arm64 原生入口是已知缺口 C44-WIN-ARM64-01（0.0.44 待做，见 §9.4）。
- 参考编译器可写三种格式的可重定位 `.o`，产品自带链接器与 `ar`；
- **自举**：宿主构建 `unisacc-seed.com` → 种子自建 → 再自建，三个 sha256 相等（N22）。

**语言**：C99 = ISO/IEC 9899:1999 + TC1–TC3（WG14 N1256）。覆盖度按逐条款账本 [archive/testing-c43-legacy/tests/c99/clauses.tsv](archive/testing-c43-legacy/tests/c99/clauses.tsv)，缺口在 [README.md](README.md) 限制表逐条点名。

**方法**（论文 A 的范式）：编译器里每个**表状决策**（词法类、预处理指令、优先级、类型、指令选择、ABI、窥孔……）由**构造**出来的小整数网络回答——权重从真值表派生，在表的全部定义域上**穷举验证**（不是训练）；结构性粘合是经典代码。范式一句话：

```
事实表 → 生成器 → 网 → 一个通用执行器 + 按包跑阶段
```

四条范式支柱：**有限表表达决策**、**共用执行器提供机制**、**显式流接口连接阶段**、**晚绑定合同隔离声明需求与宿主供给**。

**两条实现路线**：C 参考编译器（`archive/reference-c/`，行为裁判与回退）与模型产品（`exec/` 各阶段网络 + 通用执行器），逐字节对拍。

## 2. 有什么功能（已实现能力速查）

| 动作 | 现成用法 | 说明 |
|---|---|---|
| 动态执行 C／`.cx` 工具 | `./unisacc.com FILE.cx -- ARGS` | 默认即 `-run`；`.c` 同样适用 |
| 显式动态执行 | `./unisacc.com -run FILE.cx -- ARGS` | 保留的显式写法 |
| 从 tape 写目标镜像 | `./unisacc.com INPUT.tape -b OS/ARCH -o OUTPUT` | `.tapebin` 也有现成入口 |
| 写六目标程序 | `./unisacc.com FILE.c -b OS/ARCH -o OUTPUT` | 目标 `lnx/osx/win` × `x86_64/arm64` |
| 查 `.cx` 模块依赖计划 | `./unisacc.com -cx-plan FILE.cx` | 模块循环硬拒绝，报 `module cycle` |
| 用系统 GCC/Clang 编 `.cx` | `sh ./gcc_cx.com FILE.cx -o OUT` / `sh ./clang_cx.com` | 六格 APE 宿主驱动，不调 unisacc.com |
| 查模块层向 | `sh ./unisacc.com tools/check-cx-layers.cx` | 编号层 L0–L3 直接 `use` |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 新模块必须 0 |
| 上层禁 libc/POSIX | `sh ./unisacc.com tests/cxlibc.cx -- --check` | 名单不手写，按前导分类 |
| 查调用点 | `sh ./unisacc.com tools/refactor.cx -- sites NAME` | 全仓一次，跳过注释/字面量 |
| 符号索引 | `sh ./unisacc.com tools/symindex.cx -- def NAME` | 冷建 ~2.4s，查询 ~250ms |
| 批量机械改写 | `refactor.cx -- find/apply/sweep` | 每文件改前改后验证，不过即还原 |
| 看某域还差什么 | `sh ./unisacc.com tests/full-domain.cx -- . DOMAIN` | 逐格读矩阵，印 OPEN 行 |
| 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` | 无材料批次逐个实跑找回归 |
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.43.tsv --check` | R 行理由 + W 行豁免 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 七个仓级棘轮 | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape |
| 改了文件重跑哪些批 | `sh ./unisacc.com tools/affected.cx -- --files F` | 按 use 闭包算受影响批次 |
| 改一个文件看爆炸半径（一条命令） | `sh ./unisacc.com tools/flow.cx -- check FILE... [--run]` | 串起符号图受影响文件 + 受影响测试批次；`flow -- graph FILE` 出局部图 |
| 模块关系图（全图/子图/差分） | `sh ./unisacc.com tools/symindex.cx -- graph [FILE...]` / `-- delta FILE...` | 节点=文件、边=use/call；给 FILE 只出该文件的正向 use 闭包 + 反向 users |
| 文档断链 | `sh ./unisacc.com tools/doc-links.cx -- --tracked` | 归档后修链 |
| 只提交自己的行 | `sh ./unisacc.com tools/commit-own.cx -- -m MSG --keyed PATH KEY` | 多人同改文件（prd/batches/coverage） |
| 装前判产品能不能用 | `sh ./unisacc.com tools/check-dev-product.cx -- ROOT PRODUCT` | 六条：shell 头 / `sh --version` / 不带 `UNISA_CONTAINER` 能编译 / 写入产物 / 内嵌包是新头 |
| 懒分配表有没有漏守卫 | `sh ./unisacc.com tools/check-asmtext-tables.cx -- ROOT` | 碰表的 asmtext 入口必须调 `at_tables()`；删掉守卫即红 |
| 标准流写失败可见吗 | `sh ./unisacc.com tools/check-stdio.cx -- ROOT` | `/dev/full` 下 `puts`/`fflush`/`ferror` 必须全报，退出码 15 |
| 失控 `.cx` 进程看门狗 | `sh ./unisacc.com tools/procguard.cx [--once] [--dry-run]` | 每 10s（`PG_TICK_MS`）自己读 `/proc`，按命令行里的 `.cx` 路径分组；同组 >`PG_MAX`(100) 先 SIGSTOP 全组再 SIGKILL，绝不按 pgid/session 杀，不碰 tmux/claude/自身祖先；日志 `~/.cache/procguard/log`；每次 ENFORCE 后第一次 scan 带 `residue=KEY:N`（重新扫到的存活数，0 才是真收掉；pending 记在日志旁的 `.pending`，`--once`/重启也接得上） |
| 超时作业的整棵树清理 | `#pragma unisa use sys.process` → `sy_child_subreaper()`、`sy_kill_tree(root, ms, keep, nkeep, rep, cap)` | 先冻结根、反复扫 `/proc` 把新后代与同组进程也 SIGSTOP（被停进程不能再 fork），到不动点后全杀，返回残留数（0 才算干净）；starttime 防 pid 复用；subreaper 让双 fork 的孤儿仍可见；非 Linux 返回 -ENOSYS。检查：`sh ./unisacc.com tools/check-reaper.cx` |
| 子进程超时契约（run/capture + residue） | `sh ./unisacc.com tools/check-netpack.cx` | 超时先冻结整棵树再全杀，`np_result.residue` 0 才算干净；被别处收养的孤儿只有调用方先 `np_subreaper()` 才看得见，且收尸必须由调用方做（`np_run` 不回收，免得偷走调用方自己的子进程）。`UNISACC_NP_FAULT=residue` 是只给 `tools/` 与 `tests/run-batch.cx` 用的注入缝：默认关，`=residue` 把一个超时伪装成 1 个残留并打 `FAULT_INJECTED`，其它值具名拒绝 |
| e2stream 包装契约 | `sh ./unisacc.com tools/check-e2stream.cx` | 自建进程组 + 整树击杀（含 setsid 逃逸）+ `TimeoutExpired` 文案；干净超时不带 `survived)` 后缀 |
| 携带头与 glibc 的转换语义是否一致 | `sh ./unisacc.com tools/check-libc-probes.cx -- ROOT` | 七个自校验 C 程序，期望值是 glibc 字面量（运行时不靠宿主 gcc）：`putc` / `SIGUSR1,SIGUSR2` / `NAN` 符号 / scanf `%[...]`（含 16000 字符字段与宽度边界）/ printf `%n`（含截断不越界）/ scanf `%n`（含 `"%n%s"` 不跳空白）/ `strtod` 的 inf、infinity、nan。每条以 `.c` 与 `.cx` 两种输入各跑一次；`putc` 一行要产品重建（名字表在 `p3-package.bin`）才转绿 |

纯 cx 密码学（无宿主库，均过向量）：`crypto.sha256`、`crypto.b64`、`crypto.hkdf`、`crypto.chacha20poly1305`、`crypto.gcm`（AES-128/256）、`crypto.ct`、`crypto.x25519`、`crypto.bn`、`crypto.der`、`crypto.p256`、`crypto.rsa`、`crypto.x509`、`crypto.tls13`、`crypto.record`——用 `#pragma unisa use crypto.<模块>`。

完整入口清单（含每条的说明、既有依据与覆盖边界）见 [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) 的「已实现能力速查」；这里只列常用入口，具体输入结果与覆盖边界以实际核对为准。

<a id="model-function-bytes"></a>
## 3. 结构组成（unisacc.com 的内部）

用 `sh ./unisacc.com tools/com-tree.cx` 看实时结构。当前已装 `83343ec1`（5,145,706 字节，rev `edc2c7700b`）组成：

```
unisacc.com  5,145,706 B  sha 83343ec1…
├─ head: PE 镜像 + shell 启动器      1,924,096 (37.3%)  win/x86_64 原样存；DOS stub 放 sh 启动器
├─ slice lnx/x86_64                    290,976 ( 5.6%)  gzip → 解出 1,818,965 B
├─ slice lnx/arm64                     281,840 ( 5.4%)  gzip → 解出 2,318,630 B
├─ slice osx/x86_64                    307,100 ( 5.9%)  gzip → 解出 1,866,082 B
├─ slice osx/arm64                     301,825 ( 5.8%)  gzip → 解出 2,361,442 B
└─ package (P3)                      2,039,850 (39.6%)  32 模型 + 1479 阶段路由行 + 77 资源
   ├─ stage routing (D rows)             83,948
   ├─ models                          1,125,834        32 模型（deflate）
   └─ resources                         830,037        77 资源
      └─ @hdr/*（69 个头文件，未压缩）  814,573 (15.8%)  ← 0.0.44 体积优化头号线索
```

四片 gzip 的 Unix 编译器 + win/x86_64 PE 头 + 一份共用 P3 模型包（32 个网络模型、1479 行阶段路由、77 个资源）。

<a id="pipeline-design"></a>
## 4. 架构：模型流水线

<a id="flow-net-contracts"></a>
### 4.0 七流（字节管道）

真源 `exec/pipeline/image-stages.tsv`：

| 流名 | 输入 → 输出 | 功能 | 主要网 |
|---|---|---|---|
| **e2** | `src.c` → `pp.text` | 预处理：续行/注释/指令/宏/条件/include | pp |
| **e1** | `pp.text` → `tokens.typed` | 词法：关键字/标识/算符/字面量最长匹配 | lex |
| **e3** | `tokens.typed` → `tape.text` | 语法+类型等合成段 → tape（parse/type 等物理嵌此） | parse, type, scope, irsel, tyinfo, pfconv, binsel, prec |
| **e4** | `tape.text` → `tape.text` | opt：peephole / 栈→寄存器类改写 | opinfo, peep |
| **prune** | `tape.text` → `tape.text` | 按根/导出剪不可达整函数 | （无 18 网，政策在代码） |
| **lower** | `tape.text` → `target.text` | ABI/regmap/syscall/布局/reloc 降到目标 ops | abi, regmap, reloc |
| **elf** | `target.text` → `image.bytes` | 编码 + ELF/Mach-O/PE 写出 | enc |

### 4.1 十八网（ask 事实表）

真源 `weights/gold/*.tsv`（`tools/gold-table.cx --check` 核对）：pp、lex、parse、type、scope、irsel、enc、reloc、tyinfo、pfconv、regmap、isel、abi、combo、binsel、prec、opinfo、peep。每张是「有限表 + 全键穷举 exact」，由 `seed/gold_<阶段>_label.c` 规则形式构造。isel/combo 是**对照臂**（不出货主脊），其余 16 张是出货主脊。

### 4.2 构造链

```
有限表（.tsv）→ 生成器（tools/*-gen.cx，带 --check）→ 网权重 → 通用执行器（cx/core/kernel.cx 的 UNIKERN1）
```

- 有限表只经「事实表 → 生成器（--check）→ 表」修改，不手改。
- 通用执行器：定宽运算、栈/存储、字节/帧、资源与接受/拒绝。语言、ABI 与布局语义仍由阶段规则拥有。
- 模型包 P3 = 32 模型 + 阶段路由 + 资源，装载由 `cx/runtime/model_decode.cx` / `packagemodel` 处理。

## 5. 源码结构（src/ 六层 + cx/ 树）

`src/unisacc.cx` 是入口，`use drive`。六层（`src/` 共 15 文件、5352 行）：

| 层 | 文件 | 职责 |
|---|---|---|
| host | `host.cx`、`hostnative.cx` | 平台胶水（mmap/dlopen/进程），只在 L0 `cx/sys` 之下 |
| packmodel | `packmodel.cx` | 装载 P3 包、解压模型 |
| executor | `executor.cx` | 调 `core.kernel` 的 `core_run`（UNIKERN1 汇编核心） |
| stages | `stages.cx` | 按包里的阶段装模型，交给 `core_run` |
| drive | `drive.cx`、`driveroute.cx`、`driveface.cx`、`runcache.cx` | 驱动、路由、缓存 |
| bind | `bind.cx`、`entrybind.cx`、`entryexec.cx` | 入口绑定、晚绑定 |

`src/core`、`src/runtime` 是指向 `cx/` 的符号链接。

**cx/ 树**（`.cx` 模块体系，`#pragma unisa module/use`）：

- `cx/sys/`（L0，23 文件）：对接 libc/POSIX 的唯一层（env/fs/proc/time/net/poll/tty/posix/sha/mem/dl/sig…）。
- `cx/mem/`（L0）：所有者缓冲、arena、slice（Rust 式 Vec/String/所有权）。
- `cx/text/`（L0）：带长度字节串比较/格式化/拼接（`tx_*`）。
- `cx/crypto/`（L0）：纯 cx 密码学（sha256/b64/hkdf/aes-gcm/chacha20poly1305/x25519/bn/der/p256/rsa/x509/tls13）。
- `cx/core/`（L1）：执行器（kernel/execute/stack/output/bytes/memory）。
- `cx/driver/`（L1）：path/compat/args/output/deps/cli/route。
- `cx/runtime/`（L1）：model_decode/package_*/stream_*/source_frame 等。
- `cx/compiler/`（L1）：cxmod（模块解析）/asmtext（汇编文本）/streamname/netname 等名表。

分层铁律：L0 才对接 libc/POSIX，上层禁 `#include` 和裸 libc（`tests/cxinclude.cx`、`tests/cxlibc.cx`、`tests/cxraw.cx` 三个棘轮，只降不升）。

## 6. 事实表与权重（exec/ 与 weights/ 与 seed/）

- `exec/facts/`（226 文件）：编译各阶段的**事实表**（pp-targets、lex-consts、lower-abi、image-elf/macho/pe…），经 `tools/facts-gen.cx` 生成，只经「事实表 → 生成器 → 表」修改。
- `exec/`（enc/lex/lower/opt/parse/pp/prune/tapecheck…）：各阶段网络的事实与构造（含 `exec/c/` 旧 C 参考，只供对照）。
- `weights/gold/`：十八网金表；`weights/*.f32/i8.unisa`：构造出的权重。
- `seed/`：C 种子构造器（`gold_<阶段>_label.c` 规则形式、`ident.c` 身份、`ape.c` 打包）。

## 7. 测试套件设计

### 7.1 入口

```
sh ./unisacc.com tests/run.cx -- --smoke | --all | --list
```

- `--smoke`：16 条本地冒烟（product 层 smoke 批次）。
- `--list`：列全部批次 + 覆盖行 + 计划计数。
- `--all ABS_SESSION`：全量资格判定（`tools/all-waves.cx` 会话 + `tests/all-aggregate.cx` 聚合）。

### 7.2 注册表（两份 TSV）

- `tests/batches.tsv`（350 行）：每个测试批次一行——id、implementation、layer、source、mode、platform、budget、materials、entry、args、coverage、expect。`IMPLEMENTED` 才进 QUALIFIED，`PLANNED` 不进。
- `tests/coverage.tsv`（331 行）：每个覆盖行一行——id、layer、implementation、expected_or_gap、source。一行被至少一个 IMPLEMENTED 批次 PASS 即 satisfied。

### 7.3 分层（coverage.tsv 的 layer 列）

| 层 | 覆盖行数 | 内容 |
|---|---|---|
| contract | 140 | 语言/流/网合同（正反例、拒绝） |
| interfaces | 112 | 流间接口、模块接口、L0 接口 |
| negative | 30 | 具名拒绝、坏输入、语义级坏包 |
| product | 22 | 产品端到端（编译/运行/镜像字节） |
| library | 11 | 库/ABI（libunisacc、头布局） |
| platform | 6 | 跨平台（六目标写程序；原生宿主只有 5 切片 + win/arm64 模拟，见 §1） |
| properties | 9 | 优化/property 性质 |

### 7.4 19 张域矩阵（tests/*-domain-matrix.tsv）

每张矩阵是一个「域」的逐格合同：`bootstrap`、`candidate`、`formats`、`hosts`、`interfaces`、`library`、`negative`、`optimization`、`phase-timing`、`properties`、`resources`、`ri`、`stream-e1/e2/e3/e4`、`stream-elf`、`stream-lower`、`stream-prune`。

格状态：`COVERED_LOCAL`（本机批次断言了）/ `OBSERVED`（原生宿主观测）/ `DECLARED_DEPENDENCY`（有意声明的宿主依赖）= 满足；`MISSING` / `NEEDS_PRODUCT_CHANGE` / `NEEDS_FOREIGN_HOST` / `BLOCKED` = 开着的。

- `full-domain.cx -- . DOMAIN` 逐格判域 PASS（开着的格为 0 才 rc0），19 个 `full-<域>` 批次就是它。
- **外平台格**（`NEEDS_FOREIGN_HOST`）只能在**该原生宿主**上跑同一候选关掉——模拟/容器/缺席客机一律 BLOCKED/NOT_RUN，永远不算 PASS。逐格 oracle 见 [plans/foreign-host-runbook-cc.md](plans/foreign-host-runbook-cc.md)，原生 runner 见 [.github/workflows/candidate-native.yml](.github/workflows/candidate-native.yml)（ubuntu-24.04 / ubuntu-24.04-arm / macos-15-intel / macos-15 / windows-2025 / windows-11-arm）。
- **已知缺口豁免**：`tests/waivers-0.0.43.tsv`（R 行理由 + W 行豁免），`matrix-waive.cx --check` 保证记录和矩阵一致。

### 7.5 QUALIFIED 判定

`--all` 跑 `tests/all-aggregate.cx`，QUALIFIED = **每个 IMPLEMENTED 批次有同候选 PASS 收据 + 0 unsatisfied + 0 unresolved 覆盖行**。材料身份绑定候选（换候选须重构造材料）。FAIL/REFUSED 粘住会话。

## 8. 目录与文件结构（顶层）

| 路径 | 内容 |
|---|---|
| `src/` | 产品六层入口（15 文件，5352 行，srcshape 棘轮只降不升） |
| `cx/` | `.cx` 模块树（sys/mem/text/crypto = L0；core/driver/runtime/compiler = L1） |
| `exec/` | 各阶段事实与构造（facts/enc/lex/lower/opt/parse/pp/prune/tapecheck；c/ 旧 C 参考） |
| `weights/` | 十八网金表 + 构造权重 |
| `seed/` | C 种子构造器（gold label 规则、ident.c 身份、ape 打包） |
| `include/` | 头文件（cx.h 前导 + 69 个 std 头） |
| `tests/` | 测试套件（batches/coverage 注册表 + 19 域矩阵 + 各层测试 .cx） |
| `tools/` | 开发工具（326 个 .cx：生成器/棘轮/refactor/symindex/all-waves…） |
| `plans/` | 活跃计划（v0.0.4x 路线 + 架构/分层/缺陷；已完成的归档到 archive/plans） |
| `docs/` | 规格/术语（spec.md 在根、docs/glossary.md、docs/cx/） |
| `research/` | 论文五线（A 流网换逻辑、B ujs、C 管道、D cx 模块化、E 系统构表） |
| `archive/` | 归档：reference-c/reference-py（旧实现）、testing-c43-legacy（旧 1333 项套件）、plans/（历史计划）、prd 历史 |
| `apps/csih/` | csih 车道（归 csih 主人） |
| `ujs/` | 论文 B（归论文 B 主人，unisacc 侧不碰） |
| `adapter/`、`m4/`、`flownet/` | 流×网范式的适配/m4/通用流水线 |
| `out/` | 临时产物（gitignore，日志/候选/材料/会话） |
| `kernel/` | 生成物（禁止手改；权重由 gold 构造） |

<a id="current-plans"></a>
## 9. 当前状态与路线

<a id="work-tree"></a>
### 9.1 思维树（当前节点；每轮先读，事实/阻塞/下一动作原位更新）

当前节点（2026-11-01）。已装 `./unisacc.com` = `83343ec1`（5,145,706 B，rev `edc2c7700b`，0.0.43）。上一版 `33273544` 在 `out/unisacc.com.33273544`。

```text
0.0.43（收口）
├─ 已装 83343ec1：五片 win/18d2757f·lnx-x86_64/759c520a·lnx-arm64/6703dd4d·osx-x86_64/581300df·osx-arm64/7e3d267b；两代自举逐字节相同
├─ 门禁：七棘轮 ok=7、--smoke 16/0、release-ape smoke 5/5
├─ 提速：镜像缓存 -run 442→12ms、-o 477→13ms；ratchets 16.5→1.5s；AF_ALG 测试 digest 1477→330ms
├─ phase-timing 域清零：6 flip + 8 migrate + hit=2 amend + memory-bind R14 豁免，open 9→4
├─ hostseed 豁免到 0.0.44：harness 引入 coverage 层 DECLARED_DEPENDENCY，12 批 PLANNED，5 coverage 行 declared
├─ 材料重建闭环：object/net/pp-gold/ri-chain 用当前候选重建，19 批 PASS；sess-mat2 收 123 批 PASS（被共享 harness 改动作废）
├─ QUALIFIED 缺口（不是「没做完」，是结构性阻塞）：
│  ├─ 73 外平台格（lnx/arm64 19 + osx/arm64 18 + osx/x86_64 18 + win/x86_64 18）→ 需 CI 6 原生 runner，本机物理不可达
│  └─ ~24 BLOCKED（5 owner-ruling 需主人裁定范围：ri.class.memory/library、ri.route.run-object-supported、library.std.family-classification、resources.time.end-to-end-budget）
└─ 未签名、未公开（prd 门禁：先 QUALIFIED 再发布）
```

<a id="memory-palace"></a>
### 9.2 记忆宫殿（当前事实）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 已装产品 | `83343ec1`，5,145,706 B，0.0.43，rev `edc2c7700b` | `tools/com-tree.cx` |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md) |
| 流/网 | 7 流（e2/e1/e3/e4/prune/lower/elf）+ 18 网（真源 `exec/pipeline/image-stages.tsv`、`weights/gold/*.tsv`） | §4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | `tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | L0 `cx/sys`·`cx/mem`·`cx/text`·`cx/crypto`；L1 `cx/core`·`cx/driver`·`cx/runtime`·`cx/compiler` | [plans/cx-layers.md](plans/cx-layers.md) |
| 棘轮 | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape，只降不升 | `tools/ratchets.cx` |
| 事实表 | 只经 `tools/facts-gen.cx --check/--write`；名表经 `tools/nametab-gen.cx` | `exec/facts/` |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | `archive/` |

### 9.3 当前状态（0.0.43 收口）

- 已装 `./unisacc.com` = `83343ec1`（5,145,706 B，rev `edc2c7700b`，0.0.43）。五片：win/x86_64 `18d2757f`、lnx/x86_64 `759c520a`、lnx/arm64 `6703dd4d`、osx/x86_64 `581300df`、osx/arm64 `7e3d267b`；两代自举逐字节相同。
- 门禁：七棘轮 `ok=7`、`--smoke` 16/0、`release-ape smoke` 5/5。
- **QUALIFIED 缺口**（详见 [plans/release-0.0.43.md](plans/release-0.0.43.md)）：73 个外平台格（需 CI 6 原生 runner）+ ~24 BLOCKED（5 个 owner-ruling 需主人裁定范围）。本机 linux/x86_64 物理无法实跑其他 OS，故 QUALIFIED 未达成、未签名、未公开。

### 9.4 路线

- **0.0.43**：当前收口（主计划 [plans/v0.0.43.md](plans/v0.0.43.md)）。
- **0.0.44**：方向三条——**清理干扰信息、持续修 bug、改进速度**（主人 2026-11-01 定）。编译加速是第一优先级（C44-SPEED-01），另有体积（C44-SIZE-01）、win/arm64（C44-WIN-ARM64-01）、libunisacc（C44-LIBUNISACC-01）、`.cx` 模块化/内存安全（C44-CX-MODULE-01 / C44-CX-MEMORY-01）、去 .py/.sh（C44-NATIVE-01）、测试结构（C44-TEST-01）。正文见 [plans/v0.0.44.md](plans/v0.0.44.md)。
- **0.0.45 / 0.0.46**：模块外部共享 / agent harness（cx-csih）。见 [plans/v0.0.45.md](plans/v0.0.45.md)、[plans/v0.0.46.md](plans/v0.0.46.md)。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）。

## 10. 归档索引

历史 PRD 全文、逐版发布身份、旧测试套件、旧 C/Python 实现、历史计划与过程归档的入口：

- [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) —— 本文件重写前的完整旧版（含思维树/记忆宫殿全流水、能力速查全文、§3 架构全文、§5 路线全文、§6 未解决问题）。
- [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md) —— 0.0.43 历史收口。
- [archive/prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md) —— 逐版发布身份。
- [archive/testing-c43-legacy/](archive/testing-c43-legacy/) —— 旧 1333 项测试套件（已停调度）。
- [archive/reference-c/](archive/reference-c/)、[archive/reference-py/](archive/reference-py/) —— 旧 C/Python 参考实现。
- [archive/plans/](archive/plans/) —— 历史计划与设计探针。
