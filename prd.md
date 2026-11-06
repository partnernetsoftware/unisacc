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

**一句话**：unisacc.com 是 C99 编译器（带部分 POSIX 库子集），打包成单个类 APE 文件——核心需求是六个 ISA 切片（lnx/osx/win × x86_64/arm64，每个目标同候选原生证据；现状缺口见下），能为 {Linux, macOS, Windows} × {x86-64, arm64} 六目标直接写出可执行文件，并以默认 `-run` 直接运行 `.c` 和 `.cx`。cx 是参照 Rust 设计的模块化方言（`#pragma unisa module/use`），所有权与边界目前为**运行期**检查，不等于编译期借用检查。编译内部是 7 个字节流阶段，表状决策交给 18 张事实网（16 张出货，isel/combo 为对照），网络权重由真值表构造并穷举验证、不经训练，由一个通用执行器推理——以网换逻辑。

**一个文件** `unisacc.com`（APE，即 αcτµαlly pδrταblε εxεcµταblε），一个文件同时是「编译器」和「程序」，但「它能写出的目标」和「它自己运行在哪些原生宿主」是两回事：

- **写出六目标程序**（真）：能编出 {Linux, macOS, Windows} × {x86-64, arm64} 六个目标的可执行文件（ELF / Mach-O / PE），**映像不经汇编器和链接器**（模型路由直接编码写出）。
- **六 ISA 切片是核心需求，不可降级**（主人 2026-11-06 两次重申）：lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64、win/x86_64、win/arm64，每个目标都要同候选原生证据。**现状缺口**：当前 0.0.44-rc3 的 APE 只有五片；win/x86_64 的片在 Windows 上崩溃（argv 缺陷，WIN-DEBUG-LOOP / NATIVE-WIN-PE 在做）；win/arm64 已能 `-b win/arm64` 交叉编出 PE（machine 0xAA64），release-build 已能出 slice-win-arm64（提交 `0b22d546e3`），发布打包第 6 槽在做，目标是 0.0.45 候选 c45-2 出六片。
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
| 记录已知缺口 | `sh ./unisacc.com tools/matrix-waive.cx -- . tests/waivers-0.0.44.tsv --check` | R 行理由 + W 行豁免 |
| 看组成结构树 | `sh ./unisacc.com tools/com-tree.cx` | 每段占文件百分比 |
| 仓级棘轮（13 项） | `sh ./unisacc.com tools/ratchets.cx` | cxraw/cxinclude/cxapis/cxtrunc/cxlibc/cxplatform/srcshape/coreapi/toolsloc/verwriter/aluiwidth/outdep/check-literal（2026-11-06 主树 `RATCHETS ok=13 fail=0`，见 plans/rc1-gates-2026-11-06.md 门槛 2） |
| tools 行数只降不升 | `sh ./unisacc.com tests/toolsloc.cx -- --check`（`--write` 只下调） | tracked regular `tools/*.cx` 总行数，基线 `tests/toolsloc.tsv`；新增 tool 必须先改基线并经 cc 批准 |
| 改了文件重跑哪些批 | `sh ./unisacc.com tools/affected.cx -- --files F` | 按 use 闭包算受影响批次 |
| 波次哪些批不能同跑 | `sh ./unisacc.com tests/check-wave-domains.cx`（批次 `wave-domains`） | `tests/wave-domains.tsv` 每行 `id<TAB>domains<TAB>reason<TAB>evidence`；domains 取 `serial`（独占一波）/`wt`（写共享路径）/`hs`（写 harness 三树）/`mi:<材料名>`，空=无约束；all-waves 按掩码两两不相交放置 |
| 表单元里的 SBOUT 字节反解成文本 | `sh ./unisacc.com tools/decode-sbout.cx [-- FILE] [--min N]`（批次 `decode-sbout`） | 只读；把一段 `["SBOUT",N]` 还原成 `@bytes` 标签要携带的字符串，含 `\0`/`\xNN` 转义；默认扫 `exec/pp/*.tsv`，`--min` 按段长过滤；校验 `tools/check-decode-sbout.cx` |
| 网表裸立即数换成常量引用 | `sh ./unisacc.com tools/lit-const.cx -- FILE OLD NEW [--apply]`（批次 `lit-const`） | OLD/NEW 是整条动作原文，只换完全相同的那条，找不到退出 1；配合 `tools/facts-gen.cx` 派生常量 + manifest 绑定，改后用现装 `out/rtl3/ctx/jobs/tool-{gen,tbl,net}` 重打相关网，JSON 与网 `cmp` 全等才算行为不变；校验 `tools/check-lit-const.cx` |
| 运行库函数只出一份(RT-DEDUP) | `sh ./unisacc.com tools/rt-linkage.cx -- HEADER [--apply] [--skip NAME,...]`（批次 `rt-linkage`） | 把头文件里守卫中恰好一个单行签名的运行库函数改成三态:默认 `static`(字节不变)、`RT_UNIT` 出全局定义、`RT_EXTERN` 只留 extern 声明;变参/多函数/数据/`--skip` 的守卫原样保留并列出 `LEFT`;改后用 `tools/cx-flatten.cx -- --write ROOT` 重生成 cx-flat.h;校验 `tools/check-rt-linkage.cx` |
| 模型解码产物的逐模型 sha(解码器改写的 oracle) | `sh ./unisacc.com tools/model-decode-sha.cx -- EXPLODED_DIR [--reps N]`（批次 `model-decode-sha`） | EXPLODED_DIR 由 `tools/p3-explode.cx -- PKG OUT` 产出;每个模型 inflate 后 `cxmd_decode`,把 CxModel 逐字段渲染后取 sha,打 `MDSHA` 行与总 `all=`;`--reps` 只改计时(`decode_ns`,不含 inflate);改 `cx/runtime/model_decode.cx` 前后 `all=` 必须相同;校验 `tools/check-model-decode-sha.cx` |
| 同候选原生资格（六目标证据表） | `sh ./unisacc.com tests/candidate-qualify.cx -- cell|aggregate ...` | 由 `.github/workflows/candidate-native.yml` 调度；缺证据或身份不符 rc1，有 NOT_RUN rc3，全 PASS 才 rc0 |
| 单阶段过滤器（分段编译管道） | `./unisacc.com stream STAGE [--route R] [--res NAME[=FILE]] [--models PKG] [--raw] < in > out`；`stream frame --in L=F...`；`stream sidecar` | 同一 st_span/包/`\0cli/*` 跑一个阶段；属性表随流（USLATTRT）；`e2\|e1\|e3\|sidecar\|prune\|lower\|elf` 与 `-o` 逐字节同；校验 `tests/check-stream-pipe.cx`；改表→`tools/lab-loader/p3-swap.cx --model`→`--models` 实测 ≈8s |
| stream 路由目标一致性 | `sh ./unisacc.com tools/check-stream-target.cx`（批次 `stream-target`）；开发可指定 `PRODUCT/ROOT/BASELINE_PRODUCT` | route-only 从事实表 cell 前缀导出目标，显式 target 冲突一行拒绝；六 cell 宏与 direct 相等且互异、E3 long 字宽、显式一致及默认宿主逐次实跑；旧公开预发布的 route-only 曾错误沿用宿主宏，新候选重建后生效，未供旧产品伪报通过 |
| streams 旧合同壳材料 (unisacc-stream1..7.com) | `sh ./unisacc.com tools/streams-material.cx -- ROOT NEW_OUTDIR [LEVEL [TARGET]]`;校验 `tests/check-streams-material.cx`(批次 streams-material) | 7 个 <=40B 薄壳转调 `unisacc stream STAGE`,分派器 `s` 烘入 ROOT;e1 经 `tools/streams-e1-frame.cx` 把 USLATTRT 转 USLATTR1;收据 streams.json scope=product-stream-shim + NOT_CLAIMED,不是独立 runner,与 streampack 8 容器无关;钉子由 tests/cases/stream_pin.cx `stream_receipt_pin` 取自收据。lower 阶段产品会追加运行库尾部,依赖精确 lower 输出的用例仍红 |
| 分段单阶段用例（逐段字节 golden） | `sh ./unisacc.com tests/check-stages.cx [-- PRODUCT ROOT]`（批次 `stages`） | `tests/stages/<段>/` 每段 `args/in/expect`，可选 `bad/bad.err/bad.args`，喂 `unisacc stream STAGE`；无拒绝用例的段记 GAP。tape 入口校验撤回，本批如实红（见 CHANGELOG 已知限制） |
| 两代不动点（读 chain-step 产物） | `sh ./unisacc.com tests/chain-fixed-point.cx`（批次 `chain-fixed-point`，材料 `chain-gen1`、`chain-fixedpoint`） | F1 gen2 sha==gen3 sha；F2/F3 receipt 的 constructor_sha 依次指向上一代，防复制同一文件冒充；不重跑构造。材料必须绑到候选文件而非其父目录（RC1-MATERIAL-BIND） |
| 矩阵 BLOCKED 格归属 | `sh ./unisacc.com tests/check-roll-up-open.cx`（批次 `rollup-open`，输出 `ROLLUP_OPEN domains=19 failed=0`） | 19 张域矩阵 roll-up；lnx/x86_64 必需格不得 MISSING，每个 BLOCKED 格必须引用 `work/items.tsv` 里存在的条目；`--self-test` 含 unowned-blocked-refuse |
| 已知 FAIL 必须登记 | `sh ./unisacc.com tests/check-known-fail.cx -- AGGREGATE.out WAIVERS.tsv`（`-- --self-test`） | rc1 门槛 1：聚合输出里非 PASS 的批次都得是 `tests/waivers-0.0.44.tsv` 的 `F` 行；F 行只说"已知有主有计划"，不把 FAIL 变 PASS |
| 全量会话（all-waves） | `sh ./unisacc.com tools/all-waves.cx -- init/material/wave/pending`（每条外包 `tools/bound 55`） | 会话是 ROOT/out 下新目录，候选必须是 ROOT/unisacc.com；light 波最多 4 个无材料批并发，heavy 波一次 1 批；要点：init 前取最终 MAP_READY 材料，init 后 diff `materials.tsv`，按批次参数形态（文件/目录）写 map——会话内材料绑错会造出假 FAIL，单跑才是 HEAD 事实（ledger RC1-MATERIAL-BIND） |
| 产品改动链预检 | `sh ./unisacc.com tools/chain-precheck.cx -- ROOT CHAIN_OUT NEW_OUT init --files LIST`（或 `--range A..B`），随后 `reuse`、逐项 `job`、`identity/routes/p3`、`probe include/drive/use/entry`、`slice all`、`finish` | 根/旧链/新输出均收绝对或相对路径，表清单收根内绝对路径并精确去重；旧控制器 ENOENT 时才回退旧链真实 `sl-lnx/candidate`，不吞权限/目录错误。未影响输入逐字节核对后显式复用；新作业逐状态、新包探针、lnx片全实跑才 `CHAIN_PRECHECK PASS`，不等于发布资格；检查器 `tools/check-chain-precheck.cx` |
| 生成代码运行时性能三档对照 | `sh ./unisacc.com tools/check-bench-gencode.cx -- ROOT [N] [1\|8]` | 同一 `tools/bench-gencode.c` 用 unisacc / gcc -O0 / gcc -O2 各编，1MB+8MB 两档，每项 N 次取中位与 cv；三档 RESULT 校验值必须逐字节相同，否则该项 INVALID；cv>10% 标 NOISY。批次 `cx-bench-gencode` |
| arity 表与规则对拍 | `sh ./unisacc.com tools/arity-gen.cx -- --check PWD`（`--write` 重生成，`call P N M` 查单键） | `weights/arity/arity.tsv` 243 键＝proto×npar×nargs，与 `seed/gold_arity_label.c` 的 `table` 输出逐字节相等；不计入 18 网，产品 parse 尚未读它 |
| libc99posix 契约表 | `sh ./unisacc.com tools/libc99posix-gen.cx -- --check`（`--write` 生成 `cx/compiler/libc99bind.cx`）；校验 `tools/check-libc99posix.cx` | 20 函数：原八项加 P0 十二项；win 导出按 run42196947660 双架构 UCRT 实物核名及 ABI/errno 约定，pipe/write/getcwd/ftruncate 因 ABI 不同保留 cx-own；cx-own 核头文件自带 static 体，两 win 目标 `-c -funit` 实编，不把存在导出等同于兼容绑定 |
| libc99posix 缺口扫描 | `sh ./unisacc.com tools/libc99posix-audit.cx`；校验 `tools/check-libc99posix-audit.cx` | 扫 include/*.h 的 static 函数名对照契约表；GAP 按头文件；UCRT? 为下划线导出候选(未读 PE) |
| 目标信号宏事实 | `sh ./unisacc.com tools/signal-targets-gen.cx -- --check`（默认生成）；检查器 `tools/check-signal-targets.cx -- ROOT`（批次 `signal-targets`） | `exec/facts/signal-targets.tsv` 派生头宏，补 SIGPIPE，Darwin SIGUSR1/2 为 30/31；六目标编译核值，Windows 数字只属进程内模拟兼容空间。未改变 signal/raise 的模拟语义，也不宣称能忽略内核投递 |
| 未初始化局部扫描 | `sh ./unisacc.com tools/uninit-scan.cx [-- FILE...]`；校验 `tools/check-uninit-scan.cx` | 流敏感 UNINIT/SUSPECT；正例 `int n=0, owner;`（e91eb31bcf^ kernel.cx）；第一版清单不进棘轮 |
| include 驱动器号绝对路径 | `sh ./unisacc.com tools/inc-abspath-gen.cx -- --check`；校验 `tools/check-inc-abspath.cx` | `exec/facts/inc-abspath.tsv` 六目标 drive_letter；INC2 字母+':' + '/'或'\\' 且事实为真才绝对 SBFIND；win=1 lnx/osx=0。改 exec/pp 图表须先跑 seed-gen pp --shared-predefines（及 tokenpp/warnpp-shared），已装 -E 不经 seed-gen |
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
| 督导观察 agent 面板 | `sh ./unisacc.com tools/observe-agents.cx -- [--panes a,b,c] [--no-git] [--ratchets] [--fixture FILE]` | 逐个 tmux 面板读 `PANE`（state 取 mind 块首行 `▶`=busy / `○ 空闲`=idle，busy_for 取底部栏相对时长）/`PLAN`/`TOPIC`/`TASK`，并按 `work/items.tsv` 第 9 列标 `STALE`（DONE、CLOSED…、SUPERSEDED）与 `PARTIAL`（DONE-\*，尾巴未收）；`--fixture` 用文本面板，`tests/check-observe-agents.cx` 即其批次；输出 ≤30 行 |
| 切片冻结门（改薄接口层必须带回归用例+批准+字节对比;advisory,6 格 CI 原生通过后转 enforce） | `sh ./unisacc.com tools/slice-freeze.cx -- check BASE [HEAD] --receipts DIR [--baseline DIR] [--mode advisory\|enforce]`；`-- list` 列出冻结面×批次 | 冻结面清单 `tests/slice-freeze.tsv`（唯一写者 omp-cc-opus）;缺收据报 UNMEASURED,永不 ok;批次 `slice-freeze-tool` |
| 仓根换成真 APE（旧脚本留为 unisacc-dev,可回退） | `sh ./unisacc.com tools/install-ape.cx -- ROOT APE SHA256 [--write]`；回退 `-- ROOT --rollback [--write]`；检查 `tools/check-install-ape.cx` | 校验整文件 sha、UNIPKG1 尾、`--version`；同目录临时文件复核后原子 rename；旧启动脚本存 `unisacc-dev`、`.dev/` 让位为 `.dev.split-prev`、记录备份 `.split-prev`。build.json 同步 `ape1`/digest/**实际 bytes**；`ALREADY --write` 修陈旧记录，干净记录不重写，不带 `--write` 不改。两源包生产入口共享 `productpkg.pp_execution_record_error` 规范校验；更新生产者自钉后旧代材料必须重新准备；批次 `install-ape` |


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
- 现状(2026-11-06):L1/L2/L3 的外置与 booter 分离([B3][B4][B5c])未完成,发布形态仍是单文件 APE(尾部一份包);已落地的是 resolver/explode 工具与 `unisacc stream STAGE` 阶段过滤器。三层口径(loader / ISA 切片 / 模型)与 CORE-MIN 见 §9.1。

## 5. 源码结构（src/ 六层 + cx/ 树）

`src/unisacc.cx` 是入口，`use drive`。六层（文件数与行数以 `tests/srcshape.tsv` 为准）：

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

- `cx/sys/`（L0）：对接 libc/POSIX 的唯一层（env/fs/proc/time/net/poll/tty/posix/sha/mem/dl/sig…）。
- `cx/mem/`（L0）：所有者缓冲、arena、slice（Rust 式 Vec/String/所有权）。
- `cx/text/`（L0）：带长度字节串比较/格式化/拼接（`tx_*`）。
- `ext/crypto/`（包层 EXT）：纯 cx 密码学（sha256/b64/hkdf/aes-gcm/chacha20poly1305/x25519/bn/der/p256/rsa/x509/tls13）。
- `cx/core/`（L1）：执行器（kernel/execute/stack/output/bytes/memory）。
- `cx/driver/`（L1）：path/compat/args/output/deps/cli/route。
- `cx/runtime/`（L1）：model_decode/package_*/stream_*/source_frame 等。
- `cx/compiler/`（L1）：cxmod（模块解析）/asmtext（汇编文本）/streamname/netname 等名表。

分层铁律：L0 才对接 libc/POSIX，上层禁 `#include` 和裸 libc（`tests/cxinclude.cx`、`tests/cxlibc.cx`、`tests/cxraw.cx` 三个棘轮，只降不升）。

## 6. 事实表与权重（exec/ 与 weights/ 与 seed/）

- `exec/facts/`：编译各阶段的**事实表**（pp-targets、lex-consts、lower-abi、image-elf/macho/pe…），经 `tools/facts-gen.cx` 生成，只经「事实表 → 生成器 → 表」修改。
- `exec/`（enc/lex/lower/opt/parse/pp/prune/tapecheck…）：各阶段网络的事实与构造（含 `exec/c/` 旧 C 参考，只供对照）。
- `weights/gold/`：十八网金表；`weights/*.f32/i8.unisa`：构造出的权重。
- `seed/`：C 种子构造器（`gold_<阶段>_label.c` 规则形式、`ident.c` 身份、`ape.c` 打包）。

## 7. 测试套件设计

### 7.1 入口

```
sh ./unisacc.com tests/run.cx -- --smoke | --all | --list
```

- `--smoke`：本地冒烟（product 层 smoke 批次，条数以 `--list` 为准）。
- `--list`：列全部批次 + 覆盖行 + 计划计数。
- `--all ABS_SESSION`：全量资格判定（`tools/all-waves.cx` 会话 + `tests/all-aggregate.cx` 聚合）。

### 7.2 注册表（两份 TSV）

- `tests/batches.tsv`（行数以 `tests/run.cx -- --list` 为准）：每个测试批次一行——id、implementation、layer、source、mode、platform、budget、materials、entry、args、coverage、expect。`IMPLEMENTED` 才进 QUALIFIED，`PLANNED` 不进。
- `tests/coverage.tsv`（行数以 `--list` 为准）：每个覆盖行一行——id、layer、implementation、expected_or_gap、source。一行被至少一个 IMPLEMENTED 批次 PASS 即 satisfied。

### 7.3 分层（coverage.tsv 的 layer 列）

| 层 | 覆盖行数 | 内容 |
|---|---|---|
| contract | 140 | 语言/流/网合同（正反例、拒绝） |
| interfaces | 112 | 流间接口、模块接口、L0 接口 |
| negative | 30 | 具名拒绝、坏输入、语义级坏包 |
| product | 22 | 产品端到端（编译/运行/镜像字节） |
| library | 11 | 库/ABI（libunisacc、头布局） |
| platform | 6 | 跨平台（六目标写程序；六 ISA 切片需求与现状缺口见 §1） |
| properties | 9 | 优化/property 性质 |

### 7.4 19 张域矩阵（tests/*-domain-matrix.tsv）

每张矩阵是一个「域」的逐格合同：`bootstrap`、`candidate`、`formats`、`hosts`、`interfaces`、`library`、`negative`、`optimization`、`phase-timing`、`properties`、`resources`、`ri`、`stream-e1/e2/e3/e4`、`stream-elf`、`stream-lower`、`stream-prune`。

格状态：`COVERED_LOCAL`（本机批次断言了）/ `OBSERVED`（原生宿主观测）/ `DECLARED_DEPENDENCY`（有意声明的宿主依赖）= 满足；`MISSING` / `NEEDS_PRODUCT_CHANGE` / `NEEDS_FOREIGN_HOST` / `BLOCKED` = 开着的。

- `full-domain.cx -- . DOMAIN` 逐格判域 PASS（开着的格为 0 才 rc0），19 个 `full-<域>` 批次就是它。
- **外平台格**（`NEEDS_FOREIGN_HOST`）只能在**该原生宿主**上跑同一候选关掉——模拟/容器/缺席客机一律 BLOCKED/NOT_RUN，永远不算 PASS。逐格 oracle 见 [plans/foreign-host-runbook-cc.md](plans/foreign-host-runbook-cc.md)，原生 runner 见 [.github/workflows/candidate-native.yml](.github/workflows/candidate-native.yml)（ubuntu-24.04 / ubuntu-24.04-arm / macos-15-intel / macos-15 / windows-2025 / windows-11-arm）。
- **已知缺口豁免**：`tests/waivers-0.0.44.tsv`（R 行理由 + W 行豁免 + F 行已知 FAIL 批次，F 行不改结论），`matrix-waive.cx --check` 保证记录和矩阵一致。

### 7.5 QUALIFIED 判定

`--all` 跑 `tests/all-aggregate.cx`，QUALIFIED = **每个 IMPLEMENTED 批次有同候选 PASS 收据 + 0 unsatisfied + 0 unresolved 覆盖行**。材料身份绑定候选（换候选须重构造材料）。FAIL/REFUSED 粘住会话。

## 8. 目录与文件结构（顶层）

| 路径 | 内容 |
|---|---|
| `src/` | 产品六层入口（规模以 `tests/srcshape.tsv` 为准，srcshape 棘轮只降不升） |
| `cx/` | `.cx` 模块树（sys/mem/text = L0；core/driver/runtime/compiler = L1）；包层在 `ext/<package>/`（crypto 在此） |
| `exec/` | 各阶段事实与构造（facts/enc/lex/lower/opt/parse/pp/prune/tapecheck；c/ 旧 C 参考） |
| `weights/` | 十八网金表 + 构造权重 |
| `seed/` | C 种子构造器（gold label 规则、ident.c 身份、ape 打包） |
| `include/` | 头文件（cx.h 前导 + 69 个 std 头） |
| `tests/` | 测试套件（batches/coverage 注册表 + 19 域矩阵 + 各层测试 .cx） |
| `tools/` | 开发工具（个数以 `ls tools/*.cx` 为准：生成器/棘轮/refactor/symindex/all-waves…） |
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

当前节点(2026-11-06,事实取自 plans/release-notes-0.0.45.md、work/ledger.tsv)。节点后的 `[ID]` 是 `work/items.tsv` 的条目号,状态以 items 为准。
今日主线:**0.0.45 已于 2026-11-06 作为公开预发布 `v0.0.45` 发布**(APE sha256 `ae0b12d8c822efa4…`,6,128,709 字节,源 `fa0a747c7f`,非 Latest;六格原生运行 42191390455 `pass=36 fail=0 not_run=6`,发布后用同一 APE 补测 42196947660 `pass=40 fail=2`;全批会话 NOT_QUALIFIED:215 PASS / 41 FAIL / 9 UNKNOWN / 162 无收据),事实以 [plans/release-notes-0.0.45.md](plans/release-notes-0.0.45.md) 为准。0.0.44-rc3 仍是更早的公开 prerelease,0.0.44 没有正式标。**0.0.46 本体计划见 [plans/v0.0.46-unisacc.md](plans/v0.0.46-unisacc.md)。**
当前裁定：先完成 **0.0.45 稳定版**，上一行数字仅是公开预发布历史统计，不绑定当前候选。原本体三件已合入（`2a742f41fe`、`7cfab9bc0f`、`5ac7784dcb`）；c45-3 原生 E 失败已定位 Win 启动 argv 静默截为 63，c45-4 仅并 argv 修复。私有预检03包 `2a43173d` 三步全过，两Win文件不膨胀而虚拟各增131072B，原生4000参数仍待新候选CI。`table-lint`按cc暂停；BOM只登记下一版P2，不并本轮。旧预发布资产不变，未宣称稳定资格。

```text
0.0.45 公开预发布 v0.0.45 → 0.0.45 稳定版完成线（全批资格、六格原生、逐片固定点、独立评审；原0.0.46本体队列已前移）
├─ 已知限制以 plans/release-notes-0.0.45.md 为准;外平台格需六个原生 runner 同候选证据(见 §1 缺口)
└─ rc1 阶段的门槛与身份已过期,见发布说明与归档

三层布局与 CORE-MIN(方向:主人 11-05 定 CORE-MIN [P1-0.0.45];实测依据 plans/layout-measure-2026-11-05.md)
├─ 现状三层:loader(APE 头,识别 OS/ISA、取包) / ISA 切片(每 ISA 一个 entry ELF,全是机器码,lnx/x86_64 entry 1,789,094B)/ 模型(尾部一份 UNIPKG1 包,平台无关,1,659,574B;APE-ONEPKG 已落:APE 8,512,083→5,477,539B 实测,发布 APE 为 5,494,758B)
├─ CORE-MIN 目标:能流与模型化的都做成流+网,切片只留不能模型化的真核心(执行器、模型解码、解压校验、系统调用层)。已落地的只有 `unisacc stream STAGE` 单阶段过滤器;
│  按单元搬迁与删除原 .cx 实现([CORE-MIN]/[CORE-MIN-ASMTEXT])、运行库去重([RT-DEDUP])、包懒校验([PKG-LAZY-CHECK],整体校验约 65ms)均为 READY 未做;"切片非常小"的字节目标 UNMEASURED
├─ "TSV 表达动态逻辑"范式现状:事实表→生成器(带 --check)→表→网 是已成立的路径(见 §4.2、§6;例 tools/facts-gen.cx、arity-gen.cx、abidispatch-gen.cx、rt-shared-gen.cx);
│  未成立的部分:RT-SHARED 的单元宏注入仍写在驱动里(src/stages.cx st_rt_shared),搬进表/网是 [RT-SHARED-MODEL](P2-0.0.45);fwdmerge 方言表化是 [SPEED] 杠杆1(未完成)
└─ 提速 [SPEED]:SPEED-02 已落作业身份=声明输入闭包(ptrace 观测),reuse-batch 131 作业 226 s→12.7 s,复用默认仍关(UNISACC_WAVE_REUSE=1);empty.c 12.2ms / host.c e3 120ms 的旧基线本轮未复测(UNMEASURED)

0.0.44 其余方向(主人 11-01 定:清理干扰、修 bug、提速)
├─ [~] DOCK 系列 [DOCK1]/[DOCK2*]/[DOCK3]、基座外置/APE 头 [B3]/[B4]、build.json 新格式 [B5c]:状态以 items 为准(DOCK2* 在 items 记为 P2)
├─ [ ] 寄存器分配 [REGALLOC]:档1 实测 2.9% 不值得,BLOCKED 待主人要性能再开
├─ [ ] 身份机制统一(一个内容寻址身份替代 SID/PC/GEN/context/pins/receipts 多套)——设计级,未立项
└─ [ ] 其余见 plans/v0.0.44.md、plans/v0.0.45.md 与 work/items.tsv(DRAFT 项需督导转 READY 才开工)
```

<a id="memory-palace"></a>
### 9.2 记忆宫殿（当前事实）

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 任务队列 | 当前任务与认领人在 `work/items.tsv`(owner 列=tmux 窗口名;只有 READY 项被 omp-workflow 自动续跑);事实与进度追加在 `work/ledger.tsv` | [WORKFLOW.md](WORKFLOW.md) |
| 上一发布 0.0.43 | 身份见 §3 | `tools/com-tree.cx` |
| 当前发布与候选 | 0.0.45(公开 prerelease `v0.0.45`,APE `ae0b12d8…`,6,128,709 B,源 `fa0a747c7f`,非 Latest);更早:0.0.44-rc3(`54bb97a2…`,prerelease) | [plans/release-notes-0.0.45.md](plans/release-notes-0.0.45.md)、§9.1 |
| 新入口 | `src/unisacc.cx` 六层（host/packmodel/executor/stages/drive/bind）；`src/core`、`src/runtime` 指向 `cx/` | [plans/cx-modules.md](plans/cx-modules.md) |
| 流/网 | 7 流（e2/e1/e3/e4/prune/lower/elf）+ 18 网（真源 `exec/pipeline/image-stages.tsv`、`weights/gold/*.tsv`） | §4 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | `tests/batches.tsv`、`tests/coverage.tsv` |
| 分层 | L0 `cx/sys`·`cx/mem`·`cx/text`；包层 `ext/crypto`；L1 `cx/core`·`cx/driver`·`cx/runtime`·`cx/compiler` | [plans/cx-layers.md](plans/cx-layers.md) |
| 棘轮 | 13 项(以 `tools/ratchets.cx` 末行为准,不在此列名),只降不升 | `tools/ratchets.cx` |
| 事实表 | 只经 `tools/facts-gen.cx --check/--write` | `exec/facts/` |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | `archive/` |

### 9.3 上一发布 0.0.43(历史)

历史状态见 [plans/release-0.0.43.md](plans/release-0.0.43.md);身份见 §3。

### 9.4 路线

- **0.0.43**：已发布（历史，主计划 [plans/v0.0.43.md](plans/v0.0.43.md)）。
- **0.0.44**（rc3 已作公开 prerelease 发布，正式标条件见 §9.1 / rc3-verify）：方向三条——**清理干扰信息、持续修 bug、改进速度**（主人 2026-11-01 定）。编译加速是第一优先级（C44-SPEED-01），另有体积（C44-SIZE-01）、win/arm64（C44-WIN-ARM64-01）、libunisacc（C44-LIBUNISACC-01）、`.cx` 模块化/内存安全（C44-CX-MODULE-01 / C44-CX-MEMORY-01）、去 .py/.sh（C44-NATIVE-01）、测试结构（C44-TEST-01）。正文见 [plans/v0.0.44.md](plans/v0.0.44.md)。
- **0.0.45**：四个一级目标（TSV 无 hardcode、loader 优化、切片流+网、持续加速）加六 ISA 切片需求（见 §1）；公开预发布 `v0.0.45` 保持不变，平台完整性与资格债按主人最新裁定纳入本版稳定完成线（原草案 [plans/v0.0.46-unisacc.md](plans/v0.0.46-unisacc.md) 的本体队列已前移）。**0.0.46** 的 agent harness（cx-csih）仍归 csih 车道，见 [plans/v0.0.46.md](plans/v0.0.46.md)。
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
