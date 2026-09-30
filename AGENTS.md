# 本仓库的工作规则

各文件归属（种子 / 核心表与权重 / 自迭代 C / 生成物）见 [ARCHITECTURE.md](ARCHITECTURE.md)。

## 输出与调度

**输出只有两种结构**：
1. 决策型：一段能通过 `node --check` 的 JS（闭包或对象字面量均可），例如
   `const 决策 = { 结论: '…', 依据: ['…'], 产物: ['path:line'], 待定: null };`
2. 工具调用：需要时带精简的中文注释（每个非空行以 `//` 开头）。

**默认自己处理简单任务。** 只在下面两种情况才起 `deep-debug` 子代理（高成本，如 opus-5-5）：
- 定位跨文件或隐蔽的 bug，且已经自己试过 2 次没成；
- 需要设计级决策（架构、技术选型）。

其余情况用 `code-review` 子代理（如 sonnet-5），或者直接自己完成。**不要为了“保险”升级模型。**

子代理的通用约束：每个子代理一个交付物，最长约 10 分钟（研究类 8 分钟），并行时各取不同视角；结论要标明“只是设计”还是“实际跑过”。

**把问题交给强模型前先脱敏**（2026-09-27 实测有效，两个 opus 都交了活）：强模型遇到满篇平台细节的简报，容易被安全检查误触发。做法：
- 父会话先自己调研，再写一份**自包含、抽象化**的简报，放在文件里；子代理只读这一个文件，不碰仓库、不跑命令，只把一份报告写到指定路径；
- 用一般形式陈述问题（例如“表驱动的语言处理器：有限决策表、通用执行器、与参考实现逐字节一致”），不写本仓库可执行文件写出、签名、系统调用、内存内加载的具体机制；
- 给出数字和硬约束（确定性、逐字节相等、穷举验证、执行器大小），回答才可用；要求给出诊断、5–8 个带先例和最小实验的思路、推荐两个、以及哪些诱人的想法与约束冲突；
- 事后用真实数据核对再记录：这次两份报告的泛化，被对 `weights/gold/*.tsv` 的一次粗测收窄了（覆盖层对 `enc` 有用，对 `abi`、`combo` 没用）。

这不是对谁隐瞒：子代理要解决的是一般性问题，一般性问题就是它需要的全部。

## 协作

- 新知识和决定先写进 `prd.md`，再做别的；`prd.md` 是主上下文文件（当前状态、架构、路线、问题索引），条款全文在 `spec.md`（编号稳定，只增不改）。
- **一律在 main 上开发**，不用分支或工作树（主人 2026-09-30）；共享主检出时只提交自己的路径，冲突细心解决。
- 提交一律带路径：`git commit -m … -- 路径`。别人已暂存的文件不能被顺带提交。
- 改代码用精确匹配加断言的 Python 替换，不用 `sed`；`os.path.exists` 会把 ENOTDIR/EACCES 当成“不存在”，要用 `stat` 并只捕获 `FileNotFoundError`。

## 测试：本机，不用 GitHub Actions

**不要为了测试而 push。** 仓库已公开，托管的 runner 免费，`.github/workflows/ci.yml` 会在 push 时触发，但 CI 只是**安全网**，不是测试循环：先把本机测试跑绿，再让 CI 在一台没人动过的机器上当第二意见。

规则的来由：仓库还是私有时，push 触发的一个 Linux 加两个 macOS 任务的矩阵（macOS 计费是 Linux 的 10 倍）在一次“每个增量都 push”的会话里耗光了组织的 Actions 预算。所以攒着做，一次 push。

三个平台都在本机：

| 平台 | 方法 | 覆盖 |
|---|---|---|
| macOS arm64 与 x86_64 | `./tests/all.sh`、`./tests/fat.sh`（Rosetta） | osx/arm64、osx/x86_64 |
| Linux | `./tests/linux.sh`：整套测试跑在本机 Lima 虚拟机里。`LIMA_VM=minicon-lnx-x86_64` 在 arm64 宿主上是**模拟**的，脚本发现架构不一致会把看门狗放大 10 倍（曾有七个套件因超时被误判失败）。虚拟机不挂载仓库，树是用管道送进去的 | lnx/arm64、lnx/x86_64 |
| Windows 11 | `./tests/crossnative.sh`：本机 UTM 虚拟机，用 `utmctl` 驱动 | win/arm64、win/x86_64 |

- `crossnative.sh` 在虚拟机没开时**跳过**该目标：先开机再看 Windows 结果，用完要**关机**（很耗 CPU）。`tests/vms.sh up` / `down` 两件事都做（down 只关 up 开的）；`make release` 自己会调用。
- 跳过不等于通过：`crossnative.sh` 会点名跳过了什么，`STRICT=1` 让跳过变成失败。每个套件在“什么都没检查”时也失败（`closure.sh` 没有探针时曾打印 `identical 0 differ 0` 然后退出 0）。
- **macOS 绿不等于绿**：`difftest` 对拍的是系统编译器，glibc 与 BSD libc 对“C 只说未指定”的行为不一致，这是一类真实的失败，`tests/linux.sh` 专门抓它。
- **为什么 Linux 走 Lima 不走 UTM**（2026-09-21 实测，不必再争）：UTM 里有 `minicon-lnx-arm-64` 和 `minicon-lnx-x86-64`，但都没装 QEMU 客户机代理，`utmctl exec`、`file`、`ip-address` 全部失败；它们的网络是共享模式、没有端口转发、MAC 进不了宿主的 ARP 表，也没有 SSH 路径。Windows 虚拟机有代理，所以 UTM 只驱动 Windows。将来 Linux 机器装了代理，就把 `tests/linux.sh` 和 `crossnative.sh` 的 Linux 目标一起改用 `utmctl`，去掉 Lima。

## 长命令

**每次运行上限 60 秒**（主人 2026-09-25 定）。所有看门狗最多 60 秒；需要更久的套件要拆分或收窄。

可能超过一分钟的命令放后台（`run_in_background`），或者带明确的超时。曾有一个前台套件把会话堵了两个小时。

## 跑套件

- **`all.sh` 跑完才打印。** 每个套件写各自的文件，最后才汇总；日志是 0 字节说明在跑，不是卡住。需要看进度就单独跑 `./tests/closure.sh …` 这类套件，它们边跑边打印。
- **套件运行时不要改树。** 一次与 `lower.py` 的修改重叠的运行报了 59 处不一致（镜像根本没写出来），整轮作废。改完、重建，再开跑。
- **每个内层步骤都要有自己的看门狗**，不只是外层命令。一次没有逐个编译超时的消融运行，因为一个死循环的编译挂了 20 分钟。热路径用 `tests/bound N COMMAND…`；冻结套件起点用 `tests/bound --helper` 解析一次原生工具。外层队列可用 `python3 tests/bound.py N COMMAND…`，N ≤ 60，同时清理所属子进程；不再引入 Perl 看门狗。
- **`alarm` 只约束它 exec 的那个进程**，不约束它启动的东西。既编译又运行的测试有两样要限时，第二样更危险：`cc -o p f.c && ./p` 里的 `./p` 没人管。一个不终止的生成程序，在启动它的脚本早已结束之后，还以 99.8% CPU 空转了将近一小时，发热还被怪到套件头上。机器发烫时**先找失控进程**：`ps -eo pid,pcpu,command | awk '$2 > 20'`。
- **`pkill -f` 在这个 shell 的 locale 下会失败**（“illegal byte sequence”），按 PID 杀：`ps -eo pid,command | grep …`。
- unisacc 自编译每个目标约 0.25 秒，所以套件跑几分钟，是在等 Python，或者 macOS 对新二进制的首次启动扫描：先测再怪编译器。
- **发热来自 XProtect，不是编译器**（2026-09-25 实测）：每个新写出的二进制**第一次** exec 要 0.5–0.9 秒（`XprotectService` 以约 35% CPU 扫描），同一个文件第二次只要 0.02 秒，而 unisacc 编译一个探针只要 0.01 秒。十三个套件对每个探针都是“写出、签名、执行”一个新二进制，整轮就是几千次扫描。`unisacc -run` 在内存里映射代码，从不被扫描。豁免（隐私与安全性 > 开发者工具）跟随**责任应用**，而长期存活的 tmux 服务器下的 shell 不属于任何人：`tests/term.sh CMD` 把 CMD 放进 Terminal.app 里跑，扫描就跳过了（每个新二进制 0.3–0.9 秒降到 0.00 秒）。环境变量用固定白名单，传参写成 `term.sh env JOBS=2 …`。

## 门禁

`./tests/gate.sh --list [--com]` 列出门禁；用 `./tests/gate.sh [--com] --suite NAME [--suite NAME...]` 分批执行。每批整体和每个套件最多 60 秒，超时清理所属子进程；不要把全部套件塞进一次后台运行。`--com` 包含出货的 `unisacc.com` 套件。完整验证要求所有列出的套件各通过一次；单批通过不等于全套通过。

推荐使用滚动队列：发布验收只用 `tests/release.sh --com`（jobs 4、window 50，经 `tests/term.sh env …` 传 UA/MODEL_COM/SEED_DIR/GATE_STATE）；日常可 `./tests/term.sh env UA=… MODEL_COM=… python3 tests/gatequeue.py --state /tmp/本轮队列 --com --jobs 4`。完整发布命令序列见 `release/RELEASE-PIPELINE.md` §9。每次调度窗口最多 55 秒，退出 75 表示尚有待办，用同一命令续跑；退出 0 才表示清单全通过，1 表示失败。结果逐项保存并实时打印，输入变更会拒绝复用旧结果。准备模型按输入内容哈希共享，缓存命中仍检查产物哈希；测试结果不跨源码变化复用。

## 构建与发布

**CI 只测试，不构建。** 每个出货产物都在本机**同一个环境**里交叉编译（一个能自己写出六个目标的编译器，意义就在这里），再经 GitHub release（进行中是草稿）中转。GitHub Actions 只是干净机器上的第二意见，绝不是产出二进制的地方。本机跨机器测试走 UTM 虚拟机（`tests/crossnative.sh`，以及仓库之外的 `utm-court` 辅助脚本）和 Lima Linux 虚拟机。

## 生成物：kernel/

`kernel/` 里的一切都由 `python3 -m unisa emit-kernel` 写出，**不要手改**。每个文件开头列出输入及其 sha256 前缀，每个数据块标明来源：

| 文件 | 内容 | 来源 |
|---|---|---|
| `unisa_model.inc` | 权重块、阶段维度、词表、各阶段字段与头的名字、编码器操作码表 | `weights/built.json`（由 `unisa build-weights` 从 `unisa/gold.py` 构造）、`unisa/gold.py`、`unisa/catalog.py`（`ENCSPEC`）、`unisa/front/lex.py` |
| `unisa_headers.inc` | 编译器内嵌的 C 库 | `include/*.h`，原样 |
| `unisa_cases.inc` | 每个阶段每个 key 及其 gold 类别，供自测用 | `unisa/gold.py` |
| `unisa_core.c`、`unisa_self.c` | 整数推理内核 | `unisa/ckernel.py` 里的 `KERNEL_BODY` |
| `weights/gold/*.tsv` | 各阶段的真值表，一行一个 key（由 `unisa gold-export` 写出，`build-weights` 会跑它） | `unisa/gold.py` |

它们**有意提交进仓库**：`unisacc.c` 是引用 `kernel/` 与 `src/` 的有序 include 入口，不混入权重字节。经典18阶段的连续 MODEL/DENSE 分别由生成的 `weight.<stage>.inc` / `dense.<stage>.inc` 字面量片段装配，默认产品的共享网络仍独立存于 P3 包。`tests/export_ref.sh OUTPUT` 无需 Python 就能导出完整独立 C 源；自举、跨机传送和规模测试使用该完整导出，不能只传入口或用入口行数冒充编译器规模。`tests/build_ref.sh` 只构建私有导出，不改根 `unisacc.c`；`tests/kernel.sh` 重新生成并检查片段及清单。

## 路线

是**构造**，不是训练。出货的权重由 gold 表派生（`unisa build-weights`，构造上精确，经穷举验证）；`unisa train` 只是对照组，不在出货路径上；所有 `--drive` 默认值都是 `built`。**没被要求就不要跑训练**：它是几分钟的满核计算，这台机器曾因此过热。

## 模型迁移：exec/

`exec/` 是 S-17 迁移（见 spec.md）：每个编译阶段做成一个有限 δ，由通用执行器运行，与现有编译器（行为参考）逐字节对拍。`exec/pipeline/run.py FILE` 按 `exec/pipeline/stages.tsv` 把各阶段串起来；`exec/stamp.sh` 重建输入变过的生成物，**不要信 /tmp 里缓存的文件**。δ **接受**了输入却与参考不同，就是 bug；不能精确匹配的，必须拒绝并报“未覆盖”。
