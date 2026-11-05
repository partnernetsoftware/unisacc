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

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——自身原生运行在 5 个切片上（win/arm64 靠模拟），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（16 张出货，isel/combo 为对照），网络权重由真值表构造并穷举验证、不经训练，由一个通用执行器推理——以网换逻辑。

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

**动态逻辑进表的范式（2026-11-05，实例：arity、type、stmt）**：运行值先**分类**成有限键（机制行），再用**决策表**选标签（`weights/**/*.tsv` → `tools/decide-gen.cx` → 网行，逐键 T1，批次 `check-decide`），最后由机制把标签落成动作；值放 `exec/facts`，动作只写 `["constant",NAME]`（棘轮 `check-literal`，跨处一致 `check-limits`）。能进表：一步决策（选标签、选后继）。留在机制或生成器：循环与递归结构、带副作用的序列。T1 只证“网 = 表”，表是否对另靠产物比对和外部裁判。第四例（E4 窥孔，`weights/gold/peep.tsv` 1632 键）证明范式跨流成立：两级都动态时用 `decide-gen --each a=... --select REG`，第一级 `RLD pa_` 选态、第二级 `RLD b*12+rel` 选标签，START 的 1632 条 STX 池写入归零；opt 的生成器是手写清单解释器（`seed/gen.c construct_opt`），接入新 rows 要在白名单加 stem。

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
| 查模块层向与包边界 | `sh ./unisacc.com tools/check-cx-layers.cx [PRODUCT]`（`-- --fixtures` 跑夹具） | 编号层 L0–L3 直接 `use`，向上越层即红；`ext/` 是包层：产品层（`src/`、`cx/core`、`cx/runtime`、`cx/compiler`、`cx/driver`、`cx/cx0`、`cx/workspace` 与 `tests/cx-module-freeze.tsv` 的 core 模块）`use` 到 `ext/` 即红；core 模块文件集合按冻结表只许减（新增行即红、行无文件亦红） |
| core 导出面棘轮 | `sh ./unisacc.com tests/coreapi.cx -- --check`（`--write` 下调，`--fixtures` 跑夹具） | 26 个冻结 core 模块的每个非 static 定义各占一行：新符号红、行缺失红、签名变动红、`DEPRECATED-ALIAS` 的调用方文件数超上限红；`core_run` 一行标 `DUAL-IMPL linked=cx/sys/kernel.cx`，两条链接由 `cx-core-run-diff` 逐字节比对 |
| core_run 双执行器差分 | `sh ./unisacc.com tools/check-core-run-diff.cx -- ROOT KERNELS` | 同一模型与三组固定输入分别喂 `cx/core/execute.cx`（CX 参考实现）与 `cx/sys/kernel.cx`（产品链接的汇编内核），两行整体逐字节比较，末行 `CORE_RUN_DIFF`；KERNELS 是含 `x86_64.kernel` 的目录 |
| 拒新的模块 `#include` | `sh ./unisacc.com -run tests/cxinclude.cx -- --check` | 新模块必须 0 |
| 上层禁 libc/POSIX | `sh ./unisacc.com tests/cxlibc.cx -- --check` | 名单不手写，按前导分类 |
| 查调用点 | `sh ./unisacc.com tools/refactor.cx -- sites NAME` | 全仓一次，跳过注释/字面量 |
| 符号索引 | `sh ./unisacc.com tools/symindex.cx -- def NAME` | 冷建 ~2.4s，查询 ~250ms |
| `use` 的包根解析 | 仓根 `cxroot.tsv`（`R<TAB>PREFIX<TAB>DIR`）；工具侧 `use cxlib.cxroot` 调 `cxr_path`；`sh ./unisacc.com tools/check-cxroot.cx` | 旁边找不到时查最近祖先的 `cxroot.tsv`，最长前缀映射（cx/spec.md CX-U-7）；编译器侧已改 `plan.cx`/`cxmod.cx`，**已装产品要重建安装后才生效**，软链届时逐目录淘汰 |
| 源码所有权检查（CX-W-4 路线 B） | `sh ./unisacc.com tools/cx-own.cx -- FILE...` 或 `-- --tracked`；夹具 `tools/check-cx-own.cx` | 函数内局部 owner 的 DOUBLE-FREE / USE-AFTER-FREE / LEAK；保守（转给未知函数视作可能转出）。2026-11-03 全仓：9043 函数，LEAK 220（抽 10 全真，多为工具错误路径），DOUBLE-FREE 1（误报，互斥整数条件），产品目录只有 60 个局部 owner——产品所有权多在结构体字段与包装分配器里，函数内文本分析覆盖不到 |
| 批量机械改写 | `refactor.cx -- find/apply/sweep` | 每文件改前改后验证，不过即还原 |
| 看某域还差什么 | `sh ./unisacc.com tests/full-domain.cx -- . DOMAIN` | 逐格读矩阵，印 OPEN 行 |
| 普查红批次 | `sh ./unisacc.com tools/census.cx -- $PWD out/census` | 无材料批次逐个实跑找回归 |
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.43.tsv --check` | R 行理由 + W 行豁免 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 九个仓级棘轮 | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/toolsloc |
| tools 行数只降不升 | `sh ./unisacc.com tests/toolsloc.cx -- --check`（`--write` 只下调） | tracked regular `tools/*.cx` 总行数，基线 `tests/toolsloc.tsv`；新增 tool 必须先改基线并经 cc 批准 |
| 改了文件重跑哪些批 | `sh ./unisacc.com tools/affected.cx -- --files F` | 按 use 闭包算受影响批次 |
| 单阶段过滤器（分段编译管道） | `./unisacc.com stream STAGE [--route R] [--res NAME[=FILE]] [--models PKG] [--raw] < in > out`；`stream frame --in L=F...`；`stream sidecar` | 同一 st_span/包/`\0cli/*` 跑一个阶段；属性表随流（USLATTRT）；`e2\|e1\|e3\|sidecar\|prune\|lower\|elf` 与 `-o` 逐字节同；校验 `tests/check-stream-pipe.cx`；改表→`tools/lab-loader/p3-swap.cx --model`→`--models` 实测 ≈8s |
| 生成代码运行时性能三档对照 | `sh ./unisacc.com tools/check-bench-gencode.cx -- ROOT [N] [1\|8]` | 同一 `tools/bench-gencode.c` 用 unisacc / gcc -O0 / gcc -O2 各编，1MB+8MB 两档，每项 N 次取中位与 cv；三档 RESULT 校验值必须逐字节相同，否则该项 INVALID；cv>10% 标 NOISY。批次 `cx-bench-gencode` |
| arity 表与规则对拍 | `sh ./unisacc.com tools/arity-gen.cx -- --check PWD`（`--write` 重生成，`call P N M` 查单键） | `weights/arity/arity.tsv` 243 键＝proto×npar×nargs，与 `seed/gold_arity_label.c` 的 `table` 输出逐字节相等；不计入 18 网，产品 parse 尚未读它 |
| syscall 分派表（lower） | `sh ./unisacc.com tools/abidispatch-gen.cx -- --check`（`--write` 重生成）；网行 `tools/decide-gen.cx -- --check --each os=lnx,osx,win --each arch=x86_64,arm64 --sections weights/abi/dispatch.tsv SC SC.ask exec/lower/abidecide-result.tsv` | `weights/abi/dispatch.tsv` 522 键＝op×os×arch，是 `weights/gold/abi.tsv` 的投影（有 sysno／win 有 winimp 的 op 标自身，否则 `fail`）；lower 的 `SYSCALL.base` 不再逐 op CMP 链，而是 prelude 写 `mem[SCIX+sysid_op]=序号+1`，`SC.ask.{os}.{arch}` 一次 RLD 选 `SC.<op>`；每个 lower 网只装自己目标那一节（section os.arch）。批次 `check-decide` |
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

用 `sh ./unisacc.com tools/com-tree.cx` 看实时结构。上一发布 0.0.43 = `83343ec1`（5,145,706 字节，rev `edc2c7700b`）组成（当前工作树是 dev-lnx 薄壳 `dc0033f4`（B4 booter 1292B），见 §9.1）：

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

真源 `weights/gold/*.tsv`（`tools/gold-table.cx --check` 核对）：pp、lex、parse、type、scope、irsel、enc、reloc、tyinfo、pfconv、regmap、isel、abi、combo、binsel、prec、opinfo、peep。每张是「有限表 + 全键穷举 exact」，由 `seed/gold_<阶段>_label.c` 规则形式构造。**对照臂**（七流的 42 张网不读，变异实验改一键 0 网变化，2026-11-05 plans/gold-mutation-2026-11-05.md）：combo、isel、parse、pfconv、scope——仍进 weights/built.json 与 kernel/*.inc，供 18 网经典批次与 gold 对照工具使用，不删；parse 的语句分派已由 weights/stmt/stmt.tsv 决策表替代。其余 13 张（pp lex type irsel enc reloc tyinfo regmap abi binsel prec opinfo peep）及 weights/stmt、weights/arity 是七流出货主脊（改一键必改对应网）。

### 4.2 构造链

```
有限表（.tsv）→ 生成器（tools/*-gen.cx，带 --check）→ 网权重 → 通用执行器（cx/core/kernel.cx 的 UNIKERN1）
```

- 有限表只经「事实表 → 生成器（--check）→ 表」修改，不手改。
- 通用执行器：定宽运算、栈/存储、字节/帧、资源与接受/拒绝。语言、ABI 与布局语义仍由阶段规则拥有。
- 模型包 P3 = 32 模型 + 阶段路由 + 资源，装载由 `cx/runtime/model_decode.cx` / `packagemodel` 处理。

### 4.3 分层:booter / 基座 / 阶段权重 / 资源(架构方向,2026-11-02 主人定)

```
L0 booter(APE com,极小)   识别 OS/架构 → 按 manifest 找到并校验基座与阶段权重 → 交给基座执行
L1 基座(每架构一个文件)    通用执行器("权重换逻辑"的机器,即 kernel);外部文件,按内容哈希寻址
L2 阶段权重(7 流/18 网)    e2/e1/e3/e4/prune/lower/elf 各一个模型文件;外部文件,按内容哈希寻址
L3 资源                    头文件、事实表等
```

- **一切按内容哈希寻址;内置 pack 只是 resolver 的另一个来源**,与外部目录行为完全一致。
- `manifest.tsv`:`name kind(base|stage|resource) arch sha256 size order`。来源顺序:`UNISA_HOME` → com 旁目录 → 用户缓存 → 内嵌 pack(UNIPKG1 footer)。命中必校验 sha256,不符具名拒绝,**不静默回退**到别的来源。
- **产品身份 = manifest 的 sha256(Merkle 根)**。取代 `pp_product_digest` / `ape-receipt` / `build.json` artifact 段等多处各算一份的口径(见 `plans/identity-inventory-2026-11-02.md`、`plans/identity-unify-2026-11-02.md`)。kernel context 仍独立于它(判断 kernel 可否复用,不随全树变),两代 GEN 互证仍保留。
- 目的:每一层单独固化、单独开发、单独测试;阶段测试=输入流字节→输出流字节的 golden;换基座不动权重、换权重不动基座;慢车道缩成"只重算被改文件及其下游哈希"。
- 单文件发布(pack 内嵌)保留;外部目录形态用于开发。启动开销是主要风险(现热 `-run` ≈10ms),每步测量。
- 进度与分步:见 `work/items.tsv`(B0 P3 explode/implode → B1 resolver → B2 阶段测试台 → B3 基座外置 → B4 booter APE 头 → B5 身份收敛到 manifest)。

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
- `ext/crypto/`（包层 EXT）：纯 cx 密码学（sha256/b64/hkdf/aes-gcm/chacha20poly1305/x25519/bn/der/p256/rsa/x509/tls13）。
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
| `cx/` | `.cx` 模块树（sys/mem/text = L0；core/driver/runtime/compiler = L1）；包层在 `ext/<package>/`（crypto 在此） |
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

当前节点(2026-11-04 15:30)。节点后的 `[ID]` 是 `work/items.tsv` 的条目号,状态以 items 为准。
今日主线:E0 预演步7-11 全通(export 86ms / ctx 4.3s / wave 631s / kernel 39s / pack 264s);步12 release 需 git 检出,另立 E0-RELEASE。**下一步是真实重建+安装 [INSTALL44]**——DOCK 系列、B3/B4、B5c 剩余项都等已装产品认 `#pragma unisa dock` 与新 build.json。0.0.44 第一优先 [SPEED] 已开工(11-02 记的"0 进展"漂移今日纠正)。

```text
0.0.43 收口 + 体系整理(进行中)
├─ 本轮慢车道(2026-11-02,起点 a824200d77,omp-bdy 执行,runbook=plans/slowlane-runbook-2026-11-02.md)
│  ├─ [x] 步7 prepare-context  [x] 步8 source-job-wave 131 job + final
│  ├─ [x] 步9 kernel-build(收窄后字节==旧 blob 32cecc8a/4d6e45a1)
│  ├─ [~] 步10 source-package-pack(对齐 split2 身份:3 处与 source-package.cx 漏同步已修)
│  └─ [ ] 步11-18 p3/entry → 两代 GEN selfhost → kind2 安装 → 全批 → 换标
├─ 并入本轮慢车道的变更:A kernel 收窄(kernelinputs 单一来源) C host.cx cwd 头 D/E/F 补丁 06/05/07 H 撤销 --host-provider  G FFI 字节表入 include/(待做)
├─ FFI 桥:x86_64 四向量本机通过;arm64 白箱走查+栈参数已写,运行期待 CI arm64 runner;ELF .text 提取器已去 objcopy 依赖
├─ 下一轮慢车道:实参个数检查(arity,weights/arity + parse E3,预演全绿在 /tmp/arity-impl)
├─ W6 自身体系迁移:seed/{net,tbl,ident,compilerpack,pack} CX 预演通过(小样本);gen(9051 行)暂缓;P3 压缩口径待决
├─ 收缩调查结论:tools+tests 86.8k 行,可归档 0、重复断言<50 行——体量真实,靠减用例不可行;大头是身份机制重复(SID/PC/GEN/context/pins/receipts)
├─ QUALIFIED 缺口(未变):73 外平台格需 CI 6 原生 runner;~24 BLOCKED(5 项需主人裁定范围)
└─ 未签名、未公开(先 QUALIFIED 再发布)

0.0.44 方向(主人 11-01 定:清理干扰、修 bug、提速;提速第一)
├─ [~] 提速 [SPEED](omp-go):基线 empty.c 推理 12.2ms / host.c e3 推理 120ms;杠杆1=fwdmerge 方言表化(osx/arm64 免第二遍 e3),先写失败检查
├─ [~] 重建+安装 [INSTALL44](omp-bdy):E0 预演已通 [E0];安装后解锁下列
│  ├─ [~] dock 指令 [DOCK1] 已解析;lint 侧 [DOCK2a] [DOCK2b] 判据与夹具备好、代码改动等安装后同批;[DOCK3] 45 行 pragma 清单备好
│  └─ [~] 基座外置/APE 头 [B3] [B4]、build.json 新格式 [B5c]
├─ [x] 工具:dock-map(use 图共享)、refactor inject(插 pragma,死循环已修)[REFACTOR-PRAGMA]
├─ [ ] 寄存器分配 [REGALLOC]:档1 实测 2.9% 不值得,BLOCKED 待主人要性能再开
├─ [ ] 身份机制统一(一个内容寻址身份替代 SID/PC/GEN/context/pins/receipts 多套)——设计级,未立项
└─ [ ] 其余见 plans/v0.0.44.md 与 work/items.tsv(DRAFT 项需督导转 READY 才开工)
```

<a id="memory-palace"></a>
### 9.2 记忆宫殿（当前事实）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 任务队列 | 当前任务与认领人在 `work/items.tsv`(owner 列=tmux 窗口名;只有 READY 项被 omp-workflow 自动续跑);事实与进度追加在 `work/ledger.tsv` | [WORKFLOW.md](WORKFLOW.md) |
| 上一发布 0.0.43 | `83343ec1`，5,145,706 B，rev `edc2c7700b`（当前工作树 dev-lnx 薄壳见 §9.1） | `tools/com-tree.cx` |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md) |
| 流/网 | 7 流（e2/e1/e3/e4/prune/lower/elf）+ 18 网（真源 `exec/pipeline/image-stages.tsv`、`weights/gold/*.tsv`） | §4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | `tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | L0 `cx/sys`·`cx/mem`·`cx/text`；包层 `ext/crypto`；L1 `cx/core`·`cx/driver`·`cx/runtime`·`cx/compiler` | [plans/cx-layers.md](plans/cx-layers.md) |
| 棘轮 | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape，只降不升 | `tools/ratchets.cx` |
| 事实表 | 只经 `tools/facts-gen.cx --check/--write`（名表生成器 `tools/nametab-gen.cx` 已于 2026-11-02 停止生成并删除，零消费者，字节等价验证 648a47ca，见 [plans/w4-dedupe-2026-11-02.md](plans/w4-dedupe-2026-11-02.md)） | `exec/facts/` |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | `archive/` |

### 9.3 当前状态（0.0.43 收口）

- 上一发布 0.0.43 = `83343ec1`（5,145,706 B，rev `edc2c7700b`）。五片：win/x86_64 `18d2757f`、lnx/x86_64 `759c520a`、lnx/arm64 `6703dd4d`、osx/x86_64 `581300df`、osx/arm64 `7e3d267b`；两代自举逐字节相同。当前工作树是 dev-lnx 薄壳（见 §9.1 当前节点）。
- 门禁：七棘轮 `ok=7`、`--smoke` 16/0、`release-ape smoke` 5/5。
- **QUALIFIED 缺口**（详见 [plans/release-0.0.43.md](plans/release-0.0.43.md)）：73 个外平台格（需 CI 6 原生 runner）+ ~24 BLOCKED（5 个 owner-ruling 需主人裁定范围）。本机 linux/x86_64 物理无法实跑其他 OS，故 QUALIFIED 未达成、未签名、未公开。

### 9.4 路线

- **0.0.43**：当前收口（主计划 [plans/v0.0.43.md](plans/v0.0.43.md)）。
- **0.0.44**：方向三条——**清理干扰信息、持续修 bug、改进速度**（主人 2026-11-01 定）。编译加速是第一优先级（C44-SPEED-01），另有体积（C44-SIZE-01）、win/arm64（C44-WIN-ARM64-01）、libunisacc（C44-LIBUNISACC-01）、`.cx` 模块化/内存安全（C44-CX-MODULE-01 / C44-CX-MEMORY-01）、去 .py/.sh（C44-NATIVE-01）、测试结构（C44-TEST-01）。正文见 [plans/v0.0.44.md](plans/v0.0.44.md)。
- **0.0.45 / 0.0.46**：模块外部共享（含通用 APE loader 模板 C45-LOADER-01、ujs 复用、编译期所有权路线 C、范式命名）/ agent harness（cx-csih）。见 [plans/v0.0.45.md](plans/v0.0.45.md)、[plans/v0.0.46.md](plans/v0.0.46.md)。
- **0.1.x**：Clang/GCC/TinyCC 接入 `.cx`（共用前端 + 后端插件）。

<a id="optimization-leads"></a>
### 9.5 优化线索（2026-11-03 实测来源，已登记到计划）

| 线索 | 实测 | 去向 |
|---|---|---|
| libc 闭包被编进每个对象 | 60B 的 host.c：elf 推理 305ms、lower 135ms（empty.c 2.5ms）；host.o 227,704B 对 empty.o 3,016B，67 个对象各带一份相同的 printf 函数体 | C44-LIBCUNIT-01 |
| 整树 SID 让无关 job 一起作废 | 改一个头文件只影响 pp 一张网，却要全链重跑约 12 分钟（131 job 9m13s + pack 2m35s） | C44-IDENTITY-JOB-01 |
| 生成代码运行慢 | 同一 8MB SHA-256：生成代码 0.939s、宿主 0.068s（约 14 倍）、AF_ALG 0.091s | C44-PROFILE-01 |
| 模型解析也是固定成本 | `-funit` 热缓存后仍约 60ms 在模型解析（sh_apply）；解压由 modelcache 覆盖（`-o` 路线 8f8197c81b，对象路线 d1a6f36d06） | C44-PROFILE-01 之后定刀 |
| 资源段是最大单块 | 单流去重后资源段占 56.3%（830,037B，69 个头文件），7 个单流产物 8,271,508→1,474,726B（5.6 倍） | C44-SIZE-01、C45-STAGEBENCH-01 |
| 执行内核很小，周边很大 | UNIKERN 7,144B（arm64 7,096B），而 loader 模板每 ISA 约 406KB（boot 102,886 + base 302,684），重复 libc/sha + O0 runtime | C45-LOADER-01（目标 30–60KB/ISA） |
| FTRIM 读头文件原文 | 预处理等价的头文件改写会改变裁剪结果 | C44-FTRIM-ANCHOR-01 |
| 模块符号链接农场 | 约 170 个链接只为"use 只在旁边找"而存在；包根解析已实现（c3ee8ee247） | C44-CX-ROOT-01（待产品安装） |
| 所有权只靠运行期 | 函数内源码检查覆盖不到产品（产品仅 60 个局部 owner），所有权在字段与包装分配器 | C44-CX-OWN-01 → C45-CX-OWN-C-01 |
| 文档里的产品事实手写 | 一天内过期两次 | C44-FACTS-DOC-01 |

## 10. 归档索引

历史 PRD 全文、逐版发布身份、旧测试套件、旧 C/Python 实现、历史计划与过程归档的入口：

- [archive/plans/2026-11-01/prd-c43-full-2026-11-01.md](archive/plans/2026-11-01/prd-c43-full-2026-11-01.md) —— 本文件重写前的完整旧版（含思维树/记忆宫殿全流水、能力速查全文、§3 架构全文、§5 路线全文、§6 未解决问题）。
- [archive/prd-c43-history-20261030.md](archive/prd-c43-history-20261030.md) —— 0.0.43 历史收口。
- [archive/prd-release-and-development-history-20261001.md](archive/prd-release-and-development-history-20261001.md) —— 逐版发布身份。
- [archive/testing-c43-legacy/](archive/testing-c43-legacy/) —— 旧 1333 项测试套件（已停调度）。
- [archive/reference-c/](archive/reference-c/)、[archive/reference-py/](archive/reference-py/) —— 旧 C/Python 参考实现。
- [archive/plans/](archive/plans/) —— 历史计划与设计探针。
