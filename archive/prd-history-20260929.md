# prd 归档（第二轮，2026-09-29）：早期规格、完成度盘点、巡查记录、v0.0.11 回执全文、旧附录

prd.md 只描述当前与将来；这里是移出的历史全文，编号保持原样。

<a id="v0-0-11"></a>
## v0.0.11 计划与逐项回执全文

### v0.0.11 计划（2026-09-29 开工并发布，tag 8b5abc9；主人 /goal：开发并发布 0.0.11，新经验/新问题记入 v0.0.12）

**基线**：v0.0.10（tag ae6d512，签后 `unisacc.com` f6e8e090…，未签候选 4ba24140…，本地 332/332）。**执行顺序**：R11-0 顺延项按“先加回归、再修、再重构造候选”逐片闭合 → R11-8 剩余 → R11-3 → R11-1 → R11-4 → R11-7 → R11-2 → R11-5 → R11-6。每片：私有候选 + 受影响门禁 + 根产物更新 + 回执；不把局部通过写成整体完成。**停止条件**：R11-0..R11-8 逐项有 [v]/[-] 回执后走 R11-6 快链发布；超出本轮的项显式顺延到 v0.0.12 并写明理由。**新经验/新问题**一律先记到下文“v0.0.12 计划（收集中）”再处理。

**主题：库做完整、研究数据做实、论文可投稿。** 基线为 v0.0.10 发布版；沿用“表 → 构造确定性网络 → 通用执行器”，不新增训练，不另写编译逻辑。v0.0.10 未完成的项先按其停止条件显式顺延到这里，再开新项。

| 项 | 交付 | 完成判据与边界 |
|---|---|---|
| R11-0 v0.0.10 顺延项（先于新项闭合） | 库 ABI：V3 variadic 跨来源、pointee 完整身份、packed / native wide-FP / general BANK 载体、Windows SEH、六平台库生命周期矩阵、公共 origin0 refinement；平台：Windows 编译器自举（nativeboot --windows，需可用的 Terminal/utmctl 通道）、Linux x86_64 本机客机；测试：tools11（tiny-regex test2）拆分、hosted runner 余量；发布：GHCR 候选封存（主人一次 `gh auth refresh -s write:packages` 即可，本周内补，不等 0.0.11）| 每项有 [v]/[-] 回执；未闭合项不得从 README 能力边界移除 |
| R11-0 切片顺序（2026-09-29 只读研究后定；四份计划见 scratch，要点如下） | **① V3 variadic 跨来源**（进行中）：模型 `LV.native` 在 V3 下改用 MG.compatible；宿主接受 V3 变参模板（无 ffi 位），每个 V3 具体调用点在 E3 结束后由 nativeabi 认证固定图并配 carrier CIF（`ffi_prep_cif_var`）；公开探针 `source-variadic-import.c`（`--mode variadic`，两 ISA ×900，prefix 位宽与 pack(1) 拒绝）。**② pointee 完整身份**：V3 指针 descriptor 增加一个子描述（自引用用回引），MG.equal/compatible 比较子边，宿主只解析不比较；先红例（`void f(double*)` 绑到 `void f(int*)` 今天会成功），再改；V2 字节不变。**③ 宽 FP 第一步**：仅 IEEE64 profile（osx/arm64、win/x86_64、win/arm64）接受 rank3/format2 且宽度 8，其他 profile 显式拒绝、禁止把折叠到 F64 的 rank3 认证；**④ packed（仅 origin2 外部显式布局）**：AAPCS64/Win64 用 uint8[N] 载体，SysV ≤16B 且有非对齐成员保持拒绝直到 BANK；源码 `#pragma pack` 仍拒绝。**⑤ 出站 general BANK**（SysV x86_64 与 AAPCS64）：新 `USLNPLN1` 复制计划证书 + 每 ISA 一个汇编入口（装 GP/FP 银行、栈字、捕获 rax/rdx/xmm0-1/st0 或 x0/x1/q0-3/x8），入站 callback 的 BANK 与 Windows BANK 顺延 0.0.12。**⑥ Windows SEH/生命周期**：调用帧内 scoped vectored handler（按客机映像/栈地址过滤，`RtlRestoreContext` 回到帧），`windowslibraryfaultnative.c` 五例，生命周期探针加 Windows 分支与线程/回调/exit 模式，矩阵驱动逐格写 JSON；不依赖 Terminal 的通道（launchd gui 会话或客机 SSH）。 | 每片：红例→模型/宿主→私有候选→受影响门禁→根产物→回执；必须继续拒绝：源码 packed、非 IEEE64 的 long double 算术/字面量、vector/SSEUP/_Complex/__int128、变参载体的宽 FP、携带 BANK/packed/宽 FP 的入站 callback、origin0 投影 |
| R11-0 ① 回执 [v]（2026-09-29，源 1e0d0fb） | V3 variadic 跨来源导入：候选 `unisacc.com` 5fdd00b1…（版本 0.0.11，字节与产品闭包同步）；公开探针 `source-variadic-import.c` 在 osx/arm64 与 qualified libffi3.5.2 Rosetta x86 各 O0/O1/O2×100×3=900 次 ASan/UBSan 通过（三种尾：double,int / 零尾 / unsigned,double,long），prefix 位宽不匹配与 `#pragma pack(1)` 拒绝；红例先于实现保留（0.0.10 包报 library import binding or signature）。31 项受影响门禁 rc0（旧 V2 变参 host/native/rosetta/resolver、callable-variadic 三项、fixed/callable/callbacks 导入、catalog/model/carrier、run12/0、C99 57/57、chain167/167、e3self、kernel、gate-infra、ape-version）。证据 `research/r11-variadic-import-evidence.json`。不含变参函数指针/callback、宽 FP 尾、六平台。 |
| R11-0 ② 回执 [v]（2026-09-29） | 一层 pointee 身份：V3 数据指针 descriptor 带 tag5 浅 pointee（标量含 FP rank/format 与 void/unknown 完整；聚合为 struct/union tag+size/align、无成员；再下一层指针不透明；无递归无环，V2 字节不变）。MS canonical/MG equal+compatible/nativeabi/宿主解码与身份比较均含该边，carrier 配对按指针形状。公开探针 `source-pointee-import.c`（`--mode pointee`）两 ISA 各 900 次 ASan/UBSan 通过；external 声明 `double*` 对源码 `int*`、以及不透明外部指针均被模型拒绝。80 项受影响门禁：78 rc0，两项独立源码事实 oracle（sig3-source、fp-rank-source）因断言指针 payload=0 而红，按源码期望更新后直接重跑通过（`long double*` 现与 `double*` 可区分）。候选 `unisacc.com` b61ca965…。证据 `research/r11-pointee-identity-evidence.json`。**顺延 0.0.12**：递归 pointee 图（回引/环）、限定符、`void*` 通配策略。 |
| R11-0 ③ 回执 [v]（2026-09-29） | long double（rank3）跨 V3 导入：只在 long double 本身是 IEEE64 的 profile（osx/arm64、win/arm64、win/x86_64，`rules.tsv` 每 profile `long_double_format`）上认证为 F64 载体；x87（osx/x86_64、lnx/x86_64）与 IEEE128（lnx/arm64）目标显式拒绝 rank3，宿主与调用方都不做选择。公开探针 `source-longdouble-import.c`（`--mode longdouble`）osx/arm64 900 次逐位相等 ASan/UBSan 通过；Rosetta x86_64 探针作为拒绝对照：0 次原生调用、三个 opt 级别 `us_compile` 皆失败。82 项受影响门禁：81 rc0，`lib-ordered-carrier-model` 因独立 oracle 仍把 rank3/format2 列为“必拒”而红，改为“IEEE64 profile 期待 F64 标量载体、其余拒绝”后直接重跑通过（b6ba299）。候选 `unisacc.com` aa1177b2… 已装根，字节账本已刷新。证据 `research/r11-longdouble-evidence.json`。**顺延 0.0.12**：x87 80 位与 IEEE128 的存储/载体、long double 聚合与 HFA。 |
| R11-0 ④ 回执 [v]（2026-09-29） | packed（仅 origin2 外部显式布局）：`OL.meta` 对 natural>alignment 的 origin2 聚合置粘性 packed 标志，普通成员允许 entryalign<calign（放置与对象对齐按有效对齐核对），走 OL.gp 发“声明对齐整型数组”载体（pack(1)→uint8[N]、pack(2)→uint16[N/2]）；SysV（族 0）拒绝（非对齐字段为 MEMORY，待 BANK），packed 内 FP 叶、降对齐位域、origin1 pack、偏移/尺寸不一致均拒绝。宿主不改（V3 无回调聚合按 width+alignment 配对，ffi 已有 uint8/16 元素）。独立 oracle 加 4 夹具 6 坏例：六 profile 118 接受/106 拒绝、全域 net=table。新探针 `packed-external.c`（四个真实 `__attribute__((packed))`/`#pragma pack(2)` 布局由系统编译器实测）→ 证书与独立期望及 C 网络逐字节同 → osx/arm64 3 档压力×100 次原生 1200 + SCRIPT 1200，ASan/UBSan；同驱动 osx/x86_64 断言模型与 C 网络拒绝全部 12 例。受影响 84 项门禁首轮 84/84 rc0（含新增 `lib-packed-carrier-native/-refusal`）。候选 `unisacc.com` cccee896… 已装根，字节账同步。证据 `research/r11-packed-evidence.json`。首败两处（CMPI 相等分支值写成 0；夹具把 pack(1) 下 short 的有效对齐写成 2）均在队列前由既有 oracle/实测布局抓住。**顺延 0.0.12** 见收集表“R11-0 ④ 顺延”。 |
| R11-0 ⑤ 回执 [-]（2026-09-29，显式顺延 0.0.12） | 出站 general BANK：按脱敏简报取得设计报告 `research/r11-bank-design.md`（计划记录 `BNK1` 32 字节头+8 字节移动项+4 字节结果项；SysV/AAPCS64 分配器仅用 5 个计数器与有界循环，整体溢栈不推进寄存器计数；每 ISA 约 60–70 行网关，由“见证被调函数”对照程序检验；首绿两族：SysV 5 字节 packed 按 MEMORY、SysV {u64,double} 在 GP 用尽时整体进栈而尾随 double 仍进 xmm0；负例清单）。顺延理由：需要模型分配器、x64/arm64 网关汇编、宿主计划装载器、oracle 与三档压力探针四个新部件，超出本轮“先加回归、再修、再重构造候选”的单片规模；且其收益（SysV packed/混合 union/寄存器耗尽）不影响 0.0.11 已列出的公开能力边界。0.0.12 起点：先做报告推荐的两项（计数器分配器 + Python oracle；x64 网关 + 见证对照程序），mac-a64 子 8 字节栈标量在探针前保持拒绝。 |
| R11-0 平台回执 [v]（2026-09-29） | **Windows 编译器自举**：`tests/nativeboot.sh --windows win/arm64` 与 `--windows win/x86_64` 在 UTM 客机 `minicon-win-arm-64`（Windows 11 26200，x64 为系统模拟）各自用私有 UA 交叉写出的编译器重新编译 flat 源，产物与主机写出逐字节相同（各约 10 秒，含传输）。**crossnative** Windows 双目标：8 个 examples 在 win/arm64、win/x86_64 输出与 `unisa run` 相同（整轮 17.7 秒，此前每轮 300 秒空转是 `--hide` 之故）；lnx/arm64（Lima default）与 Rosetta 同轮通过。**lnx/x86_64 客机**（Lima `minicon-lnx-x86_64`，qemu 模拟）：同 8 例 crossnative 8/8 通过；完整套件在模拟机上仍不跑（CI ubuntu 原生已覆盖 lnx/x86_64 ELF 实跑）。发现并修正 crossnative 的空通过：Windows 目标跳过未计入 SKIPFILE，汇总曾打印“no target skipped”。证据：本行与提交 8c2f629、da8705a 及本次提交。 |
| R11-0 ⑥ 回执 [-]（2026-09-29，显式顺延 0.0.12） | Windows SEH 与六平台生命周期矩阵：计划（scratch plan-windows-seh-lifecycle）要求调用帧内 scoped vectored handler、`windowslibraryfaultnative.c` 五例、生命周期探针 `_WIN32` 分支与矩阵驱动。顺延理由（2026-09-29 实测）：客机 `minicon-win-arm-64` 没有任何 C 工具链（where clang/gcc/cl 皆空），Windows DLL 只能交叉构建，故障/生命周期探针无法在客机编译；客机 PowerShell 只回显命令文本，现有 `windowslibraryvmcheck.py` 的 powershell 驱动也需改为 cmd 批处理；本轮不再新增“单机主机端断言”冒充六平台矩阵。0.0.12 路线：在 CI `windows-latest`（有 MSVC）与 ubuntu/macos runner 上跑故障与生命周期探针（并入 R11-8 三平台冒烟），本机客机只做 `.com` 与产物实跑。 |
| R11-3 回执 [v]（2026-09-29，默认切换部分） | `-ftrim-libc` 在经典（`src/main.c`，UA）与模型（`exec/c/compiler.c`，`.com`）两条路线同时改为默认开启，新增 `-fno-trim-libc`；**默认住在 E2 网络里**（规则改为“无 `\0cli/fno-trim-libc` 资源即裁剪”），驱动只表达例外，裸执行器对拍自动同步；`libneed.sh`/`ftrimcheck.py` 的对照组改用显式关。同机同法计时（各 7 次新进程取中位数）：calc `-run` 188.66→142.69 ms（约 −25%），最小程序启动 36.03→35.08 ms，calc 默认 tape 标签 691→291（`-fno-trim-libc` 仍 691）。候选 `unisacc.com` 6a3dfce2…经全量发布队列 338 项全绿（首轮 337/338，唯一红项 exec-pp-literals 为夹具，修后重建重跑）；根产物与字节账同步；GHCR 重封 0.0.11-dev。**未做**：编码网络按 ISA 分片（跨 OS 共享编码器）与按需解压，顺延 0.0.12（R11-2 关系账先给出哪些网络按 ISA 相同）。 |
| R11-1 回执 [-]（2026-09-29，显式顺延 0.0.12，绑定部分 [v]） | 现状：`us_sym` 已经过 callables 句柄间接（`us_callable_pointer`），但 `us_compile`/`us_relocate` 都 `discard_image`，bindings/resolver/carrier exports/callsites 各子系统都绑定当前映像；`us_eval`（按“代”保留旧映像、新代接管查找）与 `us_reload`（换代时改写句柄目标）需要把这些子系统改成按代持有，是跨 6 个头文件的重构，不适合在发布收口中做；`us_opt_verify` 依赖 FX-1 的 tape 包格式（尚未定）。Python 绑定：`tests/libunisacccheck.py` 等六个门禁已用 ctypes 作为宿主直接驱动 `.dylib`（compile/relocate/us_sym/run_main），即示例宿主；Rust 绑定顺延。0.0.12 起点：先做“代”对象（image+exports+callables+carrier）与 `us_sym` 的句柄重定向，再做 `us_eval`。 |
| R11-2 回执 [v]（2026-09-29，关系账） | `research/r11-package-relations.json`（脚本从候选构建目录按 sha256 直接提取，可复现）：9 个阶段网络中 7 个（e1、e2、e3、e4、prune、o1、nativeabi）在共享包中一份供六目标，2 个（lower、elf）六目标各不相同（按 OS 与 ISA 都不同：lower 143–221 KB，elf 113–151 KB），**没有任何网络按 ISA 相同**；六目标网络总量 3,346,189 B 已无可去重。原生比较：crossnative 8 例在 lnx/arm64、lnx/x86_64（Lima）、osx/x86_64（Rosetta）、win/arm64、win/x86_64（UTM）与本机 osx/arm64 输出一致；逐平台标注：osx/arm64 原生、win/x86_64 与 lnx/x86_64 为仿真、其余原生。 |
| R11-4 回执 [-]（部分） | 经典表具名裁判：pp（exec/pp/macrocheck.py）、parse（tests/warn.sh）已登记为 external，ledger 外部裁判 7/18；33 个部署网络的裁判归属表未做（需先给 P3 内 24 个网络定名，本轮关系账给出 9 个阶段名），顺延 0.0.12。 |
| R11-7 回执 [-]（本轮只登记） | 目录与文档职责梳理按主人要求只登记，未实施；0.0.11 内已做的相关修正：README 的 `-ftrim-libc` 默认说明、手册 §4/§5 的 Windows 客机与 GHCR 重封规则。 |
| R11-5 回执 [-] | 论文可投稿：本轮新增可绑定产物身份的数字（trim 默认计时、关系账、Windows 自举），未做图 2/消融/终稿，顺延 0.0.12。 |
| R11-1 libunisacc 扩展 | `us_eval`（REPL 与增量编译）、`us_reload`（经跳板热替换单个函数）、`us_opt_verify`（FX-1 加载前校验器，只允许声明过的 syscall 与注入符号）；Rust 与 Python 绑定作为示例宿主 | 增量定义后可调用；热替换后下一次调用走新代码；开校验时恶意 tape 被拒、合法程序结果与不开校验逐字节相同；绑定在六目标中至少三个原生验证 |
| R11-2 按架构出包的研究账 | 六个单架构包与统一包之间逐网络、逐阶段的关系账：哪些共享、哪些按 OS 或 ISA 区分、差分大小；各架构原生机器上与本机编译器结果逐项比较 | 关系账由脚本从包中直接提取、可复现；原生比较 100% 一致，逐平台标明原生、仿真或未运行；v0.0.10 若已完成原生比较，这里只补关系账 |
| R11-3 速度 | `-ftrim-libc` 在经典与模型两条路线同时改为默认开启；编码网络按 ISA 分片（跨 OS 共享编码器）；按需解压 | 默认切换前后全门禁通过；同一产物身份上测 calc `-run` 与最小程序启动并写入账本；只报告实测，不预设倍数目标 |
| R11-4 外部裁判第一批 | 预处理与解析各登记一个具名外部裁判；33 个部署网络逐一登记裁判归属（参考实现、宿主编译器或测量） | 经典表具名裁判从 4/18 增加；网络的裁判登记表由门禁检查存在性 |
| R11-5 论文可投稿 | 图 2（七阶段与执行器一步）；算法 1 的运行时间与 5 个小表的逐表最优性差距；消融（前缀求值、声明返回、剪枝、trim-libc 各自贡献）；决策代码行数账本首期数据；规范化参考文献；中英文终稿 | 每个新数字绑定产物身份；参考文献逐条核对；按目标会议或期刊定篇幅与匿名要求 |
| R11-6 发布 | 与 v0.0.10 相同的发布规则：本地全门禁、平台矩阵、苹果与微软签名、签后资产复验 | 同 R10-7 |

| R11-7 目录与文档职责梳理（主人 2026-09-29 要求纳入，本轮只登记） | 主轴“规则→构造→网络→执行→验证”。1) 文档权威：README 管用户入口/发布版本/限制，ARCHITECTURE 管文件归属/生成者/消费者，PRD 管流水线规格/计划/验收；尺寸、网络数、平台结果引用同一生成账本，修正 ARCHITECTURE 与 exec/README 过期状态。2) 每阶段列规则来源→构造器→网络产物→执行入口→独立裁判，区分 TSV 事实与构造器手写控制流程。3) exec/c 先建职责清单：runtime（通用执行/推理/解码/存储）、driver（CLI/路由/装载）、library（嵌入库/调用/回调）、packaging（模型包/单目标构建）；测试与设计文档归各自区域，稳定后一次机械迁移并同步引用。4) unisa/src/kernel 保留种子/经典参考/生成输入职责；旧 parse/control/训练产物先标历史参考，确认活跃依赖后再归档；删除须核查调用与生成消费依赖。5) research 分论文、设计、按版本封存证据，失败记录保留但与当前状态分开；构建目录与候选定义生命周期和清理规则。顺序：文档去重纠错→职责清单→少量机械迁移→确认后删除重复或死代码 | 能追踪任一阶段从规则到运行，引用/生成链完整，相关门禁通过；搬目录不算逻辑减少，消除重复规则另计；不影响 0.0.10 发布收口 |

**R11-8 发布链提速（主人 2026-09-29 要求，向 minicon 学习；0.0.10 发布后实施）**。minicon 实测：candidate 1.5 分钟 → company-signing 2 分钟 → release 1 分钟，全链十来分钟；unisacc 0.0.10 每次文档/工作流提交都重买一张 30 分钟的 ci.yml 票，共付 4 次。差异与对策：
1. **上游不是全量测试 CI，而是“精确源的一次构建 run”**：minicon `company-signing.yml` 只要求 `minicon-com.yml`（workflow_dispatch 的一次打包 run）成功且 head_sha 一致；对 main 的比较用 `scripts/source-fingerprint.py` 的**产品源树指纹**，不是 HEAD 相等，因此文档提交不打断签名。unisacc 改法：新增 `unisacc-com.yml`（dispatch：在 macos runner 上 `make com` 或直接接收本机候选 + `provenance.py check`，≤5 分钟），`windows-signing.yml` 的上游改绑它；main 比较改为 `exec/pipeline/models.py:closure` 的 `sources_sha256` 相等。`ci.yml` 保留为 push 触发的异步安全网，不再是签名前提。
2. **候选一次构建、按摘要复用（GHCR/oras）**（[v] 2026-09-29 落地：`ghcr.io/partnernetsoftware/unisacc-candidate@sha256:20fc0bed…`，`release/candidate.json`，release-check 在 ubuntu 12 s / macos-15 13 s 拉取并实跑 version/hello/五例程与 Python 参考一致，run 36560707120）：minicon `six-grid-cloud-build.yml` 用 `oras` 把六格产物作为 OCI index 推到 `ghcr.io/<owner>/<pkg>`，`six-grid-runtime.yml` 在各平台 runner 以 `@sha256:` 摘要拉取并核对 `source_sha`/`source_tree_sha256` 后只做运行验证。unisacc 改法：本机 pack 完成后 `oras push ghcr.io/partnernetsoftware/unisacc-candidate:<sources_sha256[:16]>`（成员：unisacc.com、build.json、model-audit/models.json、kernels），签名/平台 smoke/发布全部按摘要拉取，不再靠草稿 Release 中转 `unisacc-unsigned.zip`；CI 的 Linux/macOS 三台 runner 增加“拉取候选 + com-run/c99/apps 冒烟”，把本机 Lima/UTM 的部分平台义务搬到云上并行（本机仍做原生 ARM 与 Rosetta 全量门禁）。
3. **发布批次冻结顺序**：代码→候选→本地门禁→一次 push→签名→发布→文档；中途零提交。若 main 已前进，按 minicon 惯例用一次性 `candidate-src-<v>` 分支 dispatch，用后删除。
4. **签名 step 首跑安装客户端**：`cache-dependencies: true`、step 4 分钟、服务 timeout 200 s（已在 0.0.10 落地）。
验收：从本机 pack 完成到 Release 发布 ≤ 15 分钟（不含本地全量门禁）；签名/发布 run 各 ≤ 5 分钟；任何 docs 提交不触发重签；候选摘要在 GHCR、Release 资产、回执三处一致。

**探索指针**：FX-6（int32 权重 + SIMD 推理提速，用户直觉，待剖面验证）见探索区；只做最小实验，不自动立项。

**编排**：R11-2 的关系账复用 v0.0.10 的分片与单目标闭包；R11-3 的默认切换必须两条路线同步；R11-5 只消费已封存身份上的数字。

**v0.0.11 收口（2026-09-29）**：R11-0 六片中 ①②③④ [v]、⑤⑥ [-] 显式顺延（设计与理由已记）；R11-3 默认切换 [v]（默认住在 E2 网络）、R11-2 关系账 [v]、R11-1/4/5/7 [-] 部分或顺延、R11-8 在 0.0.10 已落地并在本轮补“每片重封”。发布：源 `8b5abc9`，候选 `6a3dfce2…`（1,154,605 B，全量 338 项绿），release-check 36578604287；Windows qualification 36578789928、company 36578976300（签后 `unisacc.com` 1,170,384 B，SHA `e86cc61c…`）；Apple app zip `5039e4d4…`、dmg `7001ea5d…` 公证 Accepted 并 staple。验收 `research/r11-release-acceptance.json`；发布状态见该文件 `publication_status`。


<a id="0-3"></a>
## 0.3 交付边界（早期种子/训练对照规格）

### 0.3 交付边界（早期种子/训练对照规格，保留 U-5）

以下 Python/训练终点是历史设计，不是当前发布方式。当前产品为 `unisacc.com`；运行不启动 Python，构造/打包允许离线 Python，训练仅是显式对照。当前交付以 R9 清单和 §0.4 为准。

**交付**：命令行的「编译器 + 训练器 + 推理器」。训练微型表网络 → 把 C99 子集编译到通用 tape → lower 到 6 条 ISA → 各自执行产生完全一致的 stdout → 落盘真实权重二进制与目标文件镜像。

**不交付**：Web 应用、React、演示页。

**语言**：Python 3.11+，**仅标准库**。单一包 `unisa/`。可选后续：极小的 C gemv kernel。Python CLI 通过全部验收之前不碰 Rust。

> **[U-5] 「仅标准库」只约束出货路径。** 出货的是**构造**权重 + 推理 kernel，它们必须零依赖、可复现、能塞进 `unisacc.c`。**SGD 对照组不出货**，所以它想用 numpy / torch / Metal / Rust 调底层都可以——真要跑大规模对照实验时再换，届时按需租算力（本机的 Apple GPU/NPU 也是选项）。当前所有测试、所有镜像、自举全部走构造路线，对照组只由 `tests/baseline.sh` 显式触发。

**终点**：`unisa train && unisa run examples/hello.c --fold` 打印 6/6 match，然后停。

<a id="pipeline-design"></a>

<a id="5-5"></a>
## 5.5 完成度盘点 S-1..S-17（2026-09-21..26 的分层判据与 0.0.7/0.0.8/S-17 计划）

### 5.5 完成度盘点 [S-*]

盘点分四层，因为**难的部分和多的部分不是同一部分**：论点层基本做完了，产品层才走了一半。每层的"100%"是一条可测的判据，不是一个百分比。

| 层 | 目标 | 完成判据 | 实测（2026-09-21 起，2026-09-25 复核） |
|---|---|---|---|
| **S-1 命题** | 每个表形状的决策点都是网络，结构性代码不神经化 | 全 stage `acc = 1.000`（FULL gold 穷举）+ [P-1] 不透明性 | **已达**。18 个 stage，8,484 key 全枚举，零分歧（2026-09-25 复核） |
| **S-2 目标** | 一条 tape → 六份镜像，stdout/exit 一致 | 六个目标**在真机上**逐例一致 | **已达**。`unisa fat` 是**一个 OS 内**的多架构；cosmo 式一文件多目标（MZ + shell 自选切片）**已达**，见 S-7 #9；真正同时合法为 ELF/Mach-O/PE 的**单一字节序列**仍未开工 |
| **S-3 语言** | C99 子集覆盖别人写的代码 | 外部语料 `unsupported 0`、`wrong 0` | **已达当前语料，含浮点**（2026-09-22）。`corpus 220 pass 214 wrong 0 unsupported 0 knownfail 6`；余下 6 个全是 C99 之外的扩展（GCC 语句表达式、空结构体、`push_macro`、C11 `_Generic`）与 [G-2] 的 64 位整数求值，见 E-54 |
| **S-4 产品** | 对标 tcc：能编译常见 C99 工具；真正自举 | ① 工具语料棘轮（**未建**）② unisacc 自己造出自己的可执行文件 | **最远**。见下 |

**S-5 为什么"推到 100%"没有理论风险。** [P-8] 的构造式存在性定理给了上界 `h = |K|`，所以"能不能到 100%"**不是开放问题**，开放的只有最小性；[F-5] 说到不了 1.000 是 **key 编码错了**，不准加宽网络；[D-7] 说网络与 gold 不一致是**其中之一有 bug**，不是模型方差；[D-1] 推理期无 RNG。合起来：**这个项目里任何一处"差一点"都是缺陷，没有一处可以赖给随机性**。剩下的全是确定性工程量，可枚举、可验收。

**S-6 自举闭环已达：没有 Python 的自举。**（2026-09-22，E-55）unisacc 带着自己的后端（`src/unisacc_back.c`：lowering、两个编码器、ELF/Mach-O/PE），`unisacc FILE -b os/arch` 直接写出可执行文件。判据是最严的那种：对每个探针、每个目标，它写出的镜像与 Python 后端从同一条 tape 写出的**逐字节相同**（[A-34] closure 540/540）；cc 编出的 unisacc 用 `-b` 造出 unisacc，那个原生 unisacc 再造自己，**N1 = N2 = N3 逐字节**，并且交叉写出的其余五个目标也一致（[A-35] nativeboot，osx/arm64 与 lnx/arm64 两台真机）。[A-23] 的 B=C=U 仍保留，它证明的是 tape 经 Python 后端的不动点；这里证明的是镜像经自己的后端的不动点。

**S-7 "能编译常见 C99 工具"的卡点是形态，不是语言特性。** 按"离判据多远 ÷ 成本"排：

| 序 | 事项 | 状态 |
|---|---|---|
| 1 | **自举差距上棘轮** —— 止血，否则后面每一步都在加深 | **已做**，[A-29] |
| 2 | **多翻译单元** —— 不必做目标文件格式与链接器 | **已做**，[W-14] [A-30] |
| 3 | **libc 地板** `<ctype.h>` `<limits.h>` `<assert.h>` + `exit` | **已做**，[W-15] |
| 4 | **运行期格式串的 `printf`** —— `<stdio.h>` 本来就有完整的 `_u_vfmt`，缺的只是把非字面量格式串从内建路径放行 | **已做**，[W-9] |
| 5 | **工具语料棘轮** —— 真实库代码，多文件，自带已知答案测试 | **已做**，[A-31]，三个仓 `pass 11/11`，见 E-45、E-46 |
| 6 | **自举前端追平**（由第 1 项驱动） | **已达**（2026-09-22）：probes 90/90、`ccrun` 90 一致 / 0 已知分歧 / 0 拒绝，语料接受 216/220（覆盖 Python 前端通过的全部 209），词法器 83/0，两平台全绿，见 E-47..E-53 |
| 7 | **自举闭环**：lowering/编码/镜像进 C | **已达**（2026-09-23 扩充）：镜像与 Python 后端 540/540 逐字节相同，原生 N1=N2=N3；osx/arm64、lnx/arm64、lnx/x86_64、win/arm64、win/x86_64 **五个目标**真机自举，全程无 Python。见 E-55、E-60、E-61 |
| 8 | **浮点** | **已达**（2026-09-22）：两个前端、两个 ISA、VM 与目标解释器；`%f/%e/%g` 与平台 libc 逐位一致；`<math.h>` 为 fdlibm，1 ulp 以内。见 E-54 |
| 9 | cosmo 式三格式单文件 | **已达**（2026-09-23）：`unisacc.com` 一个文件，对 Windows 是 PE、对 Unix shell 是脚本，内含四个切片；macOS/arm64、macOS/x86_64（Rosetta）、Linux/arm64、Windows/arm64（x64 仿真）都跑通。见 E-56 |
| 10 | **编译即运行**（`tcc -run` 那一类） | **已达**（2026-09-23）：`unisacc -run FILE.c` 不落盘，直接在内存里编译并运行。见 E-56 |
| 11 | **像 tinycc 一样可用**：自带头文件、`-I`、`-D`、shebang | **已达**（2026-09-23）：头文件嵌在二进制里（2026-09-25 为 19 个；实现私有名改为保留的 `__u_*`，用户宏不再能打坏头文件），任意目录可用；`unisaccrun` 并入 `unisacc`，产物是单文件 `unisacc.com`。见 E-59 |

**S-15 0.0.7 计划**（2026-09-24 定，2026-09-24 按摸底扩充）。

**0.0.7 切版（2026-09-25，主人定：按已落地内容切版）。** 进 0.0.7 的是下表标“已达”的各项，加上 J9 提速（自编译 0.777 → 0.646 s）、`-O2`、两处误编译修复（三维数组下标、经函数指针调用的结果类型）、libc `fwrite` 修复、头文件私有名修复。未完成的 A1、A2、B2、B3、C1、C2、F2、G1–G3、H0、H3（旗标）、H4、J2–J4、J6、J7 **移到 0.0.8**，本表保留原文作为 0.0.8 的起点。产品边界不变：`-c` 输出 tape 而不是 .o，没有链接器、`-l`/`-L`；多文件在一次调用里编译。

主线三条：**把"正确性证据"延伸到 gold 本身**（枚举只证明网络等于表，00200 证明表会错）；**拉动体积与速度的真杠杆**（0.0.6 实测找到的那两个）；**让编译器能直接塞进现成的 Makefile**。每项带可测判据，按依赖与性价比排序；P0 必做，P1 应做，P2 视余力。

*摸底依据（2026-09-24 实测）*：C99 标准头 24 个里有 13 个；常用 libc 缺 `qsort bsearch strtok strncat sscanf fseek ftell remove rename perror getenv atexit labs div`；遇第一个错误即停、零警告（同一文件 `cc -Wall` 给 5 条）；`-lm -l -L -U -include -MD -x -nostdinc` 与 stdin 输入均被拒；x86 text 的 36% 是 tape 栈机经 r10 的 push/pop，`call` 平均 19 字节；`.com` 约 62% 是未压缩的 PE；出货二进制编译自己比 cc -O2 构建慢 9.4×。

**A. 正确性：审计 gold 本身（P0）**

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| A1 | **type 表对照 cc 的审计** | [G-2] 那条错规则（`int + int` 是 long）在 gold 里待了整个项目，枚举 1.000、全部套件绿——因为枚举只能证明网络等于表。以系统 cc 为裁判：对 4,275 个键中的数值组合生成 `sizeof((T1)0 op (T2)0)` 与结果符号性探针，一次编译、逐键对照 | 新套件 `gold_audit`：type 表与 cc 分歧为 0，或每条分歧进 `gold.knownfail` 并写明理由（例如我们有意不区分 `long` 与 `long long`） |
| A2 | **abi 表的系统调用号对照系统头** | 六个目标的 syscall 号是手录的；在有 `<sys/syscall.h>` 的机器上逐个比对 | Linux（Lima 两台）、macOS 本机分歧 0 |
| A3 | **fuzz 扩面** —— **已达**（2026-09-24）。`tests/gen_prog.py` 分 8 类（int/unsigned/narrow/struct/pointer/float/recursion/mixed），`tests/fuzz.sh` 每类 N 个种子。7 个新类**第一次跑**就在 C 前端抓到三个 90 个手写探针从未碰到的错：形参的 `sizeof` 是 8 字节槽位（于是 `unsigned p` 在类型轴上是 u64，`(b*p)/13u` 从未截到 32 位）；带 `u` 后缀、超过 INT_MAX 的十进制常量当成 long（C99 6.4.4.1 说是 unsigned int）；`*p`（结构体指针）传值时装入前 8 字节当地址（callee 段错误）。Python 前端三处都对——这正是 selfhost/stages 只有在探针碰到时才看得见的那类缺口；`tests/c/b_fuzzfound.c` 固定它们。修后 **8×60 = 480/480** 一致；生成器自己的一个 UB（`int *` 指向 `unsigned char`）也是这轮抓出来的 | ✅ 8 类各 ≥60 种子全部一致；`N=125`（8×125=1000，种子 1000–1124）长跑 **999/1000**，唯一不一致（m_01021）是两个前端共有的规则错：窄无符号**结果**从不零扩展（`21 - (v & 1023)` 是 u32，扩到 long 时带着符号），操作数在下一步才掩码。修法是把不变量立起来——寄存器里的 u8/u16/u32 永远零扩展——两个前端各五处（二元结果、复合赋值（并且在公共类型里做：`h >>= 1` 曾把符号位移进来）、`++x`、`-x`、`~x`）；`tests/c/b_uzext.c` 固定 |

**B. 体积与速度（P0）**

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| B1 | **regmap：x86_64 的 tape 栈指针映射到 `rsp`** —— **已达**（2026-09-24）。改的是 gold 的 regmap 表（r7 → rsp），重建权重、枚举全 1.000。两个后端各自把前端的 `.frame 8; store64 [r7+0], r` / `load64 r, [r7+0]; .frame -8` 融合成 `push`/`pop`（仅当第二半不是任何标号的目标），`call`/`callr`/`ret` 用机器自己的形式；`[rsp+d]` 加 SIB 字节。Windows 的 x86_64 改在进程栈上跑 tape（WinAPI 门自己对齐并恢复 rsp），arm64 维持自有栈（x7 不是 sp）。实测：编译器自身 lnx/x86_64 text **750,137 → 475,534 B（−36.6%）**；closure 540/540、fat 90/0（Rosetta 真 x86 执行）。途中两个后端各漏改一处（Python 的 `.frame`、C 的 `.div` 溢出基址）——都是 closure 逐字节比对当场抓出来的 | ✅ −36.6%，判据 ≥20% |
| B2 | **`.com` 的 Windows 部分自解压** | PE 占 `.com` 约 62% 且原地执行。一个小 PE 存根 + **用 C 写、由 unisacc 自己编译**的 inflate，解出真正的 PE 再执行（顺带是一次对编译器的真实负载测试） | `.com` 降 ≥30%；win/arm64、win/x86_64 实机跑通 `-run` 与 `-b` |
| B3 | **B1 之后重测速度与体积，写进 bench 与 README** | 不量不算 | bench 记录新基线；README 的体积与速度数字与实测一致（生成或检查） |

**C. 工具面：能塞进 Makefile（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| C1 | **接受并正确处理**：`-lm`/`-l*`/`-L*`（libm 本就在头文件里，接受即可）、`-U`、`-include FILE`、`-x c`、`-nostdinc`、输入 `-`（stdin）、`-o -` | `cli` 套件逐个覆盖；一个真实的 `make CC=unisacc` 能构建 tools 语料里的一个库 |
| C2 | **`-MD`/`-MF`** 写依赖文件 | 生成的 `.d` 与 `cc -MD` 列出的头文件集合一致（按我们实际读到的） |
| C3 | **多错误** —— **已达**（2026-09-24）：没有 longjmp，所以出错不是回退而是把 token 指针**停在文件尾**：走查器的每个循环都在 EOF 停（hostile 套件的截断输入早就逼出了这一点），递归自己退回顶层；`unit()` 重新同步到出错构造之后的下一个 token 继续。停机期间的错误是级联、不报。`-ferror-limit=N`（clang 的拼法，默认 20，0 不限）。`diag` 加三个错三处都报（行号 3 7 10）、限流、以及**损坏语料**仪器：前 40 个 c-testsuite 程序各删掉第三个 `;`，要求有诊断、退出 1、不挂不崩 Python 前端同样：每个顶层构造 `try`，出错记录、重同步、丢掉函数内状态、继续；两个前端对同一文件报同样三处。**顺带**：损坏语料仪器第一次跑就抓到两种不走 `文件:行:列: error:` 形制的诊断（词法的“bad char at 字节偏移”、作用域表与走查器不一致的内部告警）和 `for` 头部出错后的一个死循环——660 个损坏程序（220 × 3 个删点）现在全部干净诊断 | ✅ diag 14/14；hostile 21/21；损坏语料 660/660 |
| C4 | **一组高价值警告** —— **已达**（2026-09-25）：`-Wall`（或 `-Wextra`）之下报四类，形制同 clang 含 `[-W…]` 标签：非 void 函数可能掉出末尾（无流图：每条语句退出时算“能否落出”——return/死循环/不返回的调用不能，块取末句，带 else 的 if 取两支，其余能；main 豁免）；printf 字面量格式与实参不符（无论走编译期展开还是运行时 printf，都对实参做一次**只取类型的干跑**再回退发射器，像 `expr()` 回退误起那样）；整数隐式转指针（赋值与初始化；`0`/NULL 与函数指示符豁免）；未使用的局部（写而不读也算，clang 叫 set but not used；`x = …` 作为语句开头才算只写）。头文件内的警告不报，如 cc 对系统头。新套件 `warn`：四个探针与 `cc -Wall` 的 (行, 类) 集合**双向相等**；再对 220 个语料程序要求**零个 cc 不报的警告**。校准过程 16 → 0 个假阳性，顺带修了五处类型事实：`*p` 的宽度是被指对象的（曾沿用指针的 8 字节，`unsigned *u` 的 `*u*3` 从未截窄）、`c ? p : 0` 是指针、`&&`/`\|\|` 是 int、枚举常量是 int、以及一个**真 bug**：`char x[1]` 成员按 `n > 1` 判定成了标量，传值结构体参数的 `a.x` 装的是那个字节——c-testsuite 00204 在 `-Wall` 下段错误；`tests/c/b_arr1memb.c` 固定。Python 前端不产警告（套件只判 C 前端） | ✅ warn 探针 4/4，语料假阳性 0/220 |

**D. 库（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| D1 | **头文件** —— **已达**（2026-09-24）：`<errno.h>`（`errno` 是镜像里一个普通 int，库函数在 C 规定处设置）、`<float.h>`（IEEE binary32/64 的常量，字面量与 cc 的一致）、`<iso646.h>`、`<signal.h>`（进程内的一半：`signal` 记录、`raise` 调用，SIG_DFL 以 128+n 退出；**没有外部信号会到达**，写明而不是假装）、`<time.h>`（类型与 `difftime`；`time`/`clock` 需要 catalog 在每个目标上都没有的系统调用——macOS 没有 clock_gettime，A2 审计发现过冒名的号——所以**不声明**，调用它的程序在编译期被拒而不是拿到编造的时间）。`<setjmp.h>` 评估结论：tape 没有间接跳转也读不到 arm64 的 LR，需要新的 tape 操作族并在两个后端六个目标上 lower，**不做** | ✅ 七个 c99 探针（59、5a–5f），两个前端都与 cc 一致 |
| D2 | **函数** —— **部分已达**（2026-09-24）：`qsort`（堆排序，最坏情况有界）`bsearch strtok strncat sscanf`（d i u x o c s f e g、h/l、宽度、`*`、`%n`）`perror strerror atexit labs llabs div ldiv`。`atexit` 要求 main 返回也算 `exit`：两个前端的入口存根在 `<stdlib.h>` 定义了 `exit` 时经它返回（C 前端把 `_start` 的收尾挪到所有单元走完之后才知道有没有）。**顺带**：第一次写文件作用域的函数指针数组（atexit 的表）就发现 C 前端把它当一个 8 字节标量按字节索引——`tests/c/b_fparr.c` 固定。**2026-09-25 补齐**：`fseek ftell rewind remove rename`——catalog 新增 `lseek unlink rename`（Linux/arm64 无 unlink/rename，号是 unlinkat 35 与 renameat2 276，目录 fd 由 lowering 补，renameat2 的第五参数经新的一次性通道进 x4），Windows 门为 SetFilePointer（DWORD 符号扩展）、DeleteFileA、MoveFileExA(REPLACE_EXISTING)，BOOL 换成 POSIX 的 0/-1；两个前端的 `__lseek __unlink __rename`，VM 同步。探针 `60_fseek_remove_rename`：c99 56/56；**真机**：osx/arm64、lnx/arm64（Lima）、win/arm64 与 win/x86_64（UTM）在 -O0/-O2 下都与 cc 一致；六目标 closure 逐字节；abi_audit 22/0。`getenv` 也已达：环境在 Unix 内核放的位置——argv 的 NULL 之后，`__argv(k)` 越过 argc 就读到；`-run` 把编译器自己的环境接在 runargv 的 NULL 后，VM 同样；Windows 上答 NULL（写明）。探针 `61_getenv`，c99 57/57；osx/arm64 原生与 -run、lnx/arm64（Lima）与 cc 一致。**顺带**：新套件 `kernel` 重新生成 kernel/ 下的生成文件并要求与提交的一致——改了 include/ 没重新 emit，出货编译器带的是旧 stdlib.h | ✅ 每个已做函数有探针且对 cc 一致；tools 11/11 不退 |

**E. 文档与论文（P1）**

| # | 事项 | 完成判据 |
|---|---|---|
| E1 | 论文：刷新数字；新增一节"**gold 本身会错**" —— **已达**（2026-09-25）：表 2 按当日实测（15 阶段、corpus 216、C99 57/57、闭环 564/564、编译器自身 867 KB/2,079 符号、各 -O 级 279/279、两前端优化 190/190、出货 4.0×）；新增 §6.2（优化器也走表：H1 结构 + `peep` 表，pow2 改表的实例）与 §8.1（gold 本身会错：[G-2] 与 gold_audit 1,399/1,400） | ✅ |
| E2 | 论文表改为生成区 —— **已达**（2026-09-25）：表 1（阶段、键字段、键数、单元、**全域枚举精度**）由 `unisa docs` 生成进 `<!-- stages-zh -->` 区，`docs.sh` 过期即失败。表 2 的套件结果要跑全量才能得到，不放进 60 s 的检查，按日手刷 | ✅ 表 1 过期即失败 |
| E3 | prd §2 补**出货编译器**的 CLI 契约 —— **已达**（含 `-O0/-O1/-O2` 与 H1–H4 对齐；不再写「`-O` 忽略」） | ✅ 与 `tests/cli.sh` 对表 |

**F. 发布与分发（P1，含主人决定）**

| # | 事项 | 完成判据 |
|---|---|---|
| F1 | `make release` 自动起停两台虚机 —— **已达**（2026-09-25）：`tests/vms.sh up|down`，release.sh 在 STRICT=1 全量之前 up、`trap` 在退出（含失败）时 down；**只停自己起的**，原本在跑的保持原样；Windows 以 guest agent 应答为“起来了”，每步有界。实测：Lima default 原本在跑→未动；UTM Windows 起→应答→停。顺带：release 的 `.com` 改为 -O2 构建，与 `make com` 一致，界 1800→60 s | ✅ 一条命令从起机到关机，失败也会关机 |
| F2 | **签名策略——主人决定**。Windows 直接执行 `.com` 里的 PE，未签名会触发 SmartScreen；组织的 Azure Artifact Signing 支持 APE `.com`。macOS 的切片是解到临时目录再跑的，不带下载隔离标记 | 本项只交付选项与代价；签不签由仓库主人定 |

**G. C 扩展（P2）**

| # | 事项 | 需要什么 | 完成判据 |
|---|---|---|---|
| G1 | `_Generic`（00219） | 类型系统区分指针目标的限定符（`const int *` 不可匹配 `int *`）与 `char`/`signed char`/`unsigned char` | 00219 通过，两个前端 |
| G2 | GNU 语句表达式（00213/00214） | 块作表达式求值，含块内标签与 goto | 两个都通过 |
| G3 | 初始化器（00216） | tcc 的初始化器酷刑测试 | corpus 220/220 |

**H. 优化器 `-O1`/`-O2`（P1，主人 2026-09-25 决定做，推翻下文第 11 项的“只量不做”）**

对标 cc -O2。路线不变：**表状的优化决策走构造的网络**（窥孔：tape 指令窗口 → 改写类别，新 gold 表 `peep`，两个前端都问），结构性部分（栈顶缓存、寄存器分配、帧布局）是经典代码。优化是 tape → tape 的改写，所以 Python 后端与 C 后端吃同一条优化后的 tape，closure 逐字节判据照旧成立。

| # | 事项 | 为什么 | 完成判据 |
|---|---|---|---|
| H0 | **基线** —— 出货二进制编译自己 / cc -O2 构建编译自己 | 不量不算 | bench 记录：2026-09-25 为 1.0 s / 0.09 s ≈ 11×（-O0）；`make com` 改为 -O2 后 bench 量出货构建：**4.0×**，基线 229 → 122 ms 已记录 |
| H1 | **栈顶缓存** —— **第一步已达**（2026-09-25）：`-O`/`-O1`/`-O2` 生效（`-O0` 与缺省的 tape 与此前逐字节相同）。C 前端在所有单元走完后做 tape → tape 改写：`.frame 8; store64 [r7+0], rX; S; load64 rY, [r7+0]; .frame -8`，当 S 是只含显式操作数的直线代码（21 个操作的白名单）且不提 rY、r7 时，改为 `mov rY, rX; S`（S 空且 X=Y 时整对删去），迭代到不动点。`-O2` 另加：中间段用到了 rY 时借 r3–r5 中一个**死**寄存器承载（`mov rZ, rX; S; mov rY, rZ`）。死的判据来自 walker 的一个可逐 tape 检查的性质：r3–r5 在块内先读后写只出现在函数序言的参数保存处，所以它们在块边界都是死的，除非进入一个把它作参数的函数；call 跨过（被调者不收它为参数时）。两个后端吃同一条优化后的 tape，closure 照旧。死寄存器由**块级活跃性**判定（按标签与跳转切块，call 读 Z 当且仅当被调者入口块 Z 活跃，白名单外的操作视为读全部寄存器，求最小不动点；每轮先把每行预处理成类别与读写掩码）。`-O2` 还把局部变量读取 `imm r2, N / sub64 rD, r6, r2 / .ld rD, [rD+0], W` 在 r2 死时合成 `.ld rD, [r6-N], W`（8,216 处）。实测：编译器镜像 **974,370 → 693,666 B（−29%）**，-O2 编出的编译器编译自己 **1.04 → 0.47 s**，剩余 push/pop 约 2.2 万 → 5,466 对；-O2 编译整个编译器 0.59 s，写出的编译器与参考逐字节相同，-O2 下自身不动点成立；新套件 `opt`（31 s）：314 个探针/c-testsuite/c99 程序 -O0 与 -O2 运行一致、-O2 closure 在三个目标上逐字节、自编译与不动点——653/0 | 杠杆最大 | text −25%；bench 比值 ≤ 6×（现约 5×：0.46 s / 0.09 s） |
| H2 | **`peep` 表** —— **已达**（2026-09-25）：键 (A 类 8 × B 类 17 × 关系 10) = 1,360，动作 8 类；构造权重，**全域枚举 1.000**。结构性代码只找对子并说出关系（同槽 store→load / store→store、imm 0/1/2^n 作为第二源且该寄存器随后死、imm 后 mov 出死寄存器、跳到下一条、跳到跳转、写死寄存器），做不做、做哪种由网络答。-O2 在 H1 各轮之后跑。活跃性扩到 r0–r5（`ret` 处 r0/r1 活）。`[r7+..]` 槽不参与同槽：那是 B1 在 x86 上融成真 push/pop 的地方。**实测一次改表**：`pow2 → to_shl` 让 x86 镜像大了 24 KB（tape 的 shift 在 x86 上要经 cl），gold 改为 keep 并重建——决策在表里，量了就改表。**两个前端同问**：`unisa/opt.py` 是 C 优化器的孪生，经同一个 oracle 问 `peep`；`optpy` 套件：C 的 -O0 tape 经 Python 优化 = C 的 -O1/-O2 tape，190/190。closure、bigclosure 6/0、stages/selfhost 94、nativeboot、`--check-oracle` 16,582/0 | 表状决策，按路线走网络 | ✅ 枚举 1.000；两前端同问 190/190；closure 0 差异 |
| H3′ | **各级对 cc -O2** —— **已达**（2026-09-25）：新套件 `difftest_o`：93 个探针由 cc -O2 编出并运行作为参考（按源码+cc 版本哈希缓存），unisacc 在 -O0/-O1/-O2 下 `-run`，**279/279 一致**（冷 42 s，热 4 s） | 各级都得是同一个程序 | ✅ |
| H3 | **`-O0/-O1/-O2` 旗标生效**（今天接受但忽略） | 对接 Makefile 习惯 | 每级 closure、nativeboot N1=N2=N3、difftest 对 cc -O2 输出一致 |
| H4 | 叶函数内联、强度削减 —— **第一步**（2026-09-25）：`peep` 加两种关系、两种动作：`dest_to_mov`（A 算到 rD 只为 `mov rY, rD`，rD 随后死 → A 直接算到 rY）与 `copy_into`（`mov rY, rX` 后一条读 rY、rY 随后死 → 读 rX），键 1,632，枚举 1.000，两前端同问（optpy 192/192）。**实测**：编译器 -O2 镜像 osx/arm64 726,690 → 677,154 B（−6.8%），lnx/x86_64 629,985 → 576,737 B（−8.5%），tape 里的 `mov` 16,684 → 1,988；**自编译时间不变**（0.54 s）——乱序 CPU 上寄存器 mov 几乎免费，速度的差距在访存与调用，不在这里。回归：opt 659/0、difftest_o 282/282、closure 570/0、bigclosure 6/0、nativeboot、c99 57/57。**第二步**：局部变量写入融合（`imm r2,N / sub64 rA,r6,r2 / S / .st [rA+0],rV,W` 在 rA、r2 随后皆死时成 `S / .st [r6-N],rV,W`，读取融合的镜像），编译器 tape 里融合的写入 690 → 2,174 处，少约 2,000 行；两前端一致（optpy 192/192），回归全绿（含 corpus 216+4、datashape、fuzz） | 余下的差距 | bench 比值 ≤ 3×（现 4.0×） |

**I. 抽象与复用：把仍然手写或重复的表状逻辑收进表（P1，2026-09-25 后台勘察）**

出发点是“表状决策走网络、代码只做结构”。勘察（只读）发现四类仍是手写、且在 Python/C 或 x86/arm64 间重复的表状逻辑：

| # | 事项 | 现状（重复处） | 提案 | 风险 | 判据 |
|---|---|---|---|---|---|
| I1 | **`prec` 表**：二元运算优先级 —— **已达**（2026-09-25）：字段 op 19，头 lev 11（none, 1=`||` … 10=`* / %`），构造、枚举 1.000。实为**四份**拷贝：C 的 `bop()`/`BOP`/`BLEV` 与常量折叠的 `cprec()` 18 行比较链，Python 的 `PREC` 与 `CPREC`——全部删去，两前端按记号种类问一次并记住。**顺带修了一个 bug**：Python 常量折叠从 `&&` 那级开始，`int a[1 || 0]` 被拒（C 接受）；探针 `b_const_oror`。对 HEAD 编译器，探针/c99/corpus/编译器自身的 -O0 tape 逐字节相同；stages/selfhost 94、closure 564/0、bigclosure 6/0、nativeboot、optpy | C `bop()`/`BOP`/`BLEV`（setup_tables）；Python `PREC` 与 `CPREC`（同文件两份） | 字段 op(19)，头 lev(11)；`binop_level` 与 `binary()`/`const_expr()` 改问表，三份删去 | 低 | stages、selfhost、closure、bigclosure 不变 |
| I2 | **`opinfo` 表**：优化器的操作分类 —— **已达**（2026-09-25）：字段 op 67（tape.SHAPE 全部 + other），头 simple(2)/acls(8)/bcls(16)，构造、枚举 1.000；C 的 `ol_simple`/`pk_acls`/`pk_bcls` 与 opt.py 的 `_simple`/`_acls`/`_bcls` 改问表（每个 op 词问一次并记住），三份清单删去。对 HEAD 编译器：95 个输入 × -O1/-O2 的 tape 逐字节相同；optpy 190/190、opt 655/0 | 21 操作白名单与 ALU 列表在 C `ol_simple`/`pk_acls`/`pk_bcls`、`opt.py` `SIMPLE`/`ALU`、gold `PEEP_A/B` 三处 | 字段 op(~40+other)，头 simple(2)、acls(8)、bcls(17)、kind(8) | 低 | optpy 190/190、opt、-O2 closure |
| I3 | **irsel 补行**：除/余/无符号比较 —— **已达**（2026-09-25）：irsel 的 alu 族加 div/mod/udiv/umod；新表 `binsel`（运算符 16 × 符号 2 → alu 变体，枚举 1.000）取代 C `emit_binop` 的 23 分支链与 ir.py 的 `ALU`/`UNS` 字典及 `divmod_` 的手写选择，指针差的 `.div` 两端也改经 irsel。对 HEAD：探针/c99/corpus/编译器自身 -O0 tape 逐字节相同。回归在冻结快照上跑（新 `tests/snap.sh`）：closure 570/0、bigclosure 6/0、stages/selfhost 95、opt 659/0、optpy 192/192、difftest_o 282/282、c99 57/57、corpus 216+4 | C `emit_binop` 直写 `.udiv .umod .div .mod` 与 `@alu.ult…` 的手写 if 链；Python `parse.py` 直写 `.div` | irsel 加 binop×unsigned 的 flavor 行，两前端同问 | 低 | stages、closure |
| I4 | **abi 加头**：argshape、retconv、winimp —— **已达**（2026-09-25）：catalog 派生三个事实，abi（与 combo）各加三头，构造、枚举 1.000。`.sys` 的参数形状（Linux/arm64 的 openat/unlinkat/renameat2）由 `argshape`（plain/atfd_1/atfd_1_zero/atfd_2_zero5）决定，两个后端的按名特例删去；Windows 门拆成**主体**（每个 op 自己的参数搬运与调用）加**尾巴**（`retconv`：none/wcount/bool_inv/bool_neg/dword_sx），主调用的导入由 `winimp` 给出——C 的魔数导入下标与 Python 的字面导入名删去。`winextra` 未做：rename 的 REPLACE_EXISTING、CreateFileA 的处置参数移位只各有一处，留作结构。**内核容量**：推理内核按 s×12 为每个阶段的输出头计数编址（用移位拼，内核无乘法），abi 第 13 个头写进了下一阶段——编译器 bus error；改为 s×16（`HEADS_MAX`），生成器断言不超。这是“换表不改内核”的第一个例外：内核的**容量常数**会随表长大，逻辑不变。字节：Python 后端 291 例、C 后端六目标全部例与 HEAD 逐字节相同。**顺带**：`.com` 的启动脚本把编译器作为子进程运行，看门狗杀脚本时子进程成了孤儿（01:19 起空转 9 小时，97% CPU）；改为转发信号并以 fd 3 交出 stdin，ape 套件加两项（超时不留孤儿；stdin 到达） | openat/unlinkat/renameat2 的参数移位与第五参数通道（lower.py 与 bk_lower 各一份）；Windows 门的导入名、BOOL→0/-1、DWORD 符号扩展、CreateFileA 处置参数（四份：x86/arm × Python/C） | 门的前后序保留为结构，其余查表 | 中（Windows 只能靠 closure 与 UTM） | closure 全部 os×arch、crossnative |
| I5 | **编码规格表**：机器码模板 —— **第一批已达，其余不做**（2026-09-25）：x86 的 alu2/setcc/shiftext 与 arm64 的 alu3/invcond 成为 `catalog.ENCSPEC` 一份数据；Python 编码器读它，C 的查找由 ckernel 生成进 unisa_model.inc，四条手打 if 链删去。对 HEAD：C 后端六目标 582 例逐字节相同、Python closure 0 差异。**其余批次不做**：D/E/F/G/I 族在两端已是参数化的小函数或结构性序列，挪表收益小于改动风险；剩余重复是 Python 与 C 两个后端本身，按既定方向由 Python 退为参考实现来消化 | `enc` 只答 form；字节模板手写四份（emit_x86 约 34 例、emit_arm 约 40 例、C `bk_x86`/`bk_arm` 与 x_*/a_*）；`ALU2/SETCC/ALU3/INVCOND` 在 C 里重打成 if 链 | 一份 `unisa/encspec.py`：(op, arch) → (族, opc, cc, 固定寄存器, 宽)，族选择走网络，操作码作为生成的数据表（如 ckernel 生成 .inc）；分批：A 寄存器 ALU+C 移位 → B 比较置位 → D 访存宽度 → E/F 立即数与地址 → G 分支（碰松弛，加 bigclosure）→ I 除法 → H 浮点；J（门、itoa 等不透明块）不迁 | 中高 | 每批 closure 逐字节不变；预计 Python −150~200 行、C −200~250 行 |

**顺序**：I2、I1（便宜、低风险，先证明“表 → 权重 → 两端同问”可复用）→ I3 → I4 → I5 分批。

**J. 速度：推理与编译（P1，2026-09-25，Fable 只读评审）**

结论：自建编译器慢于 cc -O2 的 4 倍几乎全在**生成代码**（栈机临时量、局部变量的地址计算、调用开销），不在模型；-O2 的剩余耗时在**文本式的优化轮次**（每轮重切行、重建标签、重扫字符）。推理已记忆化，内核只对自建编译器的冷查询有影响。

| # | 事项 | 位置 | 预期 | 闭环风险 | 论点 |
|---|---|---|---|---|---|
| J1 | **稠密表 = 运行构造网络遍历全域生成** —— **已达**（2026-09-25）：emit-kernel 以 `intnet.predict` 遍历每个阶段全域，写出 DENSE（每键每头一个字节）；`inf` = 定义域检查 + 一次查表，记忆缓存删去；`--check-oracle` 20,184 问与 `infer()` 0 差异；192 份 tape 与 HEAD 相同。**实测速度几乎不变**（自建自编译 0.48 → 0.47 s）：原记忆缓存已够好，推理不是瓶颈——与评审判断一致；收益在于去掉冲突回退、让“表 = 网络”成为每次构建的证明 | ckernel、`inf` | 实测 ~2% | 无 | 网络的编译形态，逐项验证相等 |
| J2 | **walker 里把表达式临时量放寄存器**（右操作数为叶子时左操作数进 r3–r5），替代 push/pop | `binary`/`emit_binop` | 运行 25–35% | tape 变，Python walker 同步 | 不变 |
| J3 | **局部变量在发射时就用 `[r6-N]`** | walker 多处 | 运行 10–15% | 同上 | 不变 |
| J4 | **优化轮次改为按行记录**：kind/掩码由发射器给出，每轮只修补改写过的行 | opt_round/peep_round | -O2 编译 20–30% | opt.py 同步，文本逐字节不变 | 不变 |
| J5 | 按 cop 缓存 `bk_facts`、`pk_act` 按名字记住 —— **已达**（2026-09-25）：485 个镜像（5 目标）与 HEAD 相同；速度在噪声内（J1 之后推理本就便宜）。`@family.flavor` 预解析未做：J1 之后它只剩两次已缓存的 vfind 与一次查表 | es、bk_facts、peep_round | 实测 <2% | 无 | 不变 |
| J6 | 位切片的 `infer()`（每字段每取值一张单元位集，按位与后按优先级取胜者）作参考实现 | ckernel | check-oracle 与冷查询变快 | 无 | 同一权重的转置布局 |
| J7 | SIMD / 向量 tape 指令 | — | J1、J6 之后可忽略 | 高 | — |

**顺带修的 bug**：`--check-oracle` 的 `oracle_pass` 仍按每阶段 12 个头编址（内核已是 16），从第二个阶段起核对错位——已改为 16。另记两个前端热点：预处理在每个 `#include` 后对整个缓冲区重跑 splice/decomment（随 include 数二次方增长），`tokclass` 每次询问都查 typedef 表。

**顺序**：J1（最便宜、论点最干净）→ J5 → J4 → J3 → J2；J6 视需要；J7 不做。

**容量（J8，2026-09-25）**：编译器自身的源码已到 1,042,362 / 1,048,576 字节（MAXSRC），再过几次提交就无法编译自己。放宽 MAXSRC 1→4 MB、MAXTOK 13万→52万、MAXOUT 4→16 MB、SX_MAXOCC、MAXPOOL、MAXLIT、BL_MAX、BK_MAXI 26万→100万、BK_MAXT 52万→200万、BK_MAXN；均为静态数组，只加 bss，镜像不变，自编译内存峰值 61 MB。新套件 `scale`：自身源码、tape 文本、tape 行、tape 指令过各自上限一半即失败（提前报警）；并以 -O2 编译运行一个生成的 80 万字节、6,000 函数的程序——首跑即撞上 MAXSYM 4,096（12,000 个全局名），提到 65,536、符号哈希 SH_SIZE 提到 65,536 后 1 s 通过。继续探：3.3 MB（24,000 函数）依次撞上 MAXTOK（→ 2M）与 MAXOUT（→ 32 MB）；1.6 MB（12,000 函数）撞上后端的 BK_MAXI（100 万条 tape 指令）——再放大后端每条指令十几个 int 的数组要上百 MB 虚拟空间，不做。**设计目标**：编译器自身规模的约两倍以内（tape ≤ 100 万条指令）；`scale` 默认 6,000 函数，并对自身用量过半报警。**待探索（主人 2026-09-25 提出，未排期）**：容量现在是静态数组、只占 bss，上限固定。可做一个小实验：改为用 `__mmap` 按需申请并增长（例如 tape、token、后端指令表在满时翻倍重映射），让上限随输入伸缩、空闲时不占虚拟空间；需两前端与 -run 路径一并考虑，先量一次内存与速度再定。

**J3、J2 的重新评估**（J1、J5 之后）：两个 walker 都把局部变量的地址先算进 r0 作左值、再经 `[r0+0]` 读写；在发射时直接用 `[r6-N]`（J3）要改左值表示，两前端同改。而 -O2 的读取融合与写入融合（H4）已消去绝大多数此类访问，出货构建（-O2）上 J3 的剩余收益小，降级。J2（临时量进寄存器）同理：H1 的栈顶缓存已消去约四分之三的 push/pop。余下差距的真正来源待下一次对自建二进制的指令级剖析确定，再定 J2 的范围。

**J9. 生成代码与编译器自身的速度（2026-09-25，opus 评审之后实测）**

- 已做：arm64 的 `call`/`callr`/`ret` 改为 `bl`/`blr`/`ret x17`，使返回预测能配对。字节数不变，自建编译器的 -O2 自编译从 0.81 s 降到 0.78 s。评审按指令条数估计能快 15–30%，实测只有约 4%。
- 实测剖析（`sample` 采样，再用 Python 后端算出的标签地址归并到函数）：自建编译器 -O2 自编译的时间，**约一半在 -O2 优化器本身**（ol_*/bl_*/opt_round/peep_round），词表查找约 16%，词法约 7%，后端约 6%，推理约 2%。-O0 为 0.42 s，-O1 为 0.47 s，-O2 为 0.80 s。
- 轮次：unisacc.c 上 opt 跑 4 轮（命中 21,070、4,695、932、202），peep 跑 4 轮（17,401、1,737、14、0）。每轮都要全量重建行表、标签表和 6 个寄存器的活跃性，行数都在十万以上。
- 试过、无效、已撤回：给 ol_mask、ol_firstreg 加逐行缓存（0.79 s，没有变化）；ol_word 改为比较编号（0.94 s，反而更慢，因为原来的文本比较在第一个字节就失败返回）。
- 按段增量跑后几轮（以 ret 分段；段签名包括段内各块、以及段内每个 call 目标入口块的活跃位，精确比较）：输出与 HEAD 逐字节相同，后几轮跳过的段数为 opt 3/76/224、peep 2/197/451（共 462 段），**但总耗时不变**（0.78 s）。每轮的固定成本（建行表、标签表、6 个寄存器的活跃性、整段复制）才是大头，逐行分析不是。已撤回。
- 结论：这条路上剩下的收益要靠减少轮数本身，或让活跃性和行表跨轮增量维护。两者都会改变优化器的结构，需要单独立项；在那之前，-O2 自编译的速度维持在 0.78 s。
- **跨轮沿用逐行事实（c5c8f67，保留）。** 拆分实测（cc -O0 带符号构建，5 次采样）：优化器 1,048 个样本中，ol_prep 占 360，活跃性 145，整段复制约 113，建行表 73，其余逐行分析约 350。选 ol_prep 切入。假设：原样照抄的行，其 kind、rm、wm 只取决于行文本，可以从上一轮沿用，只有跳转和调用行的 tg 按标签重算。结果：ol_prep 0.093 降到 0.037 s；A/B 用固定输入 a39f67c:unisacc.c，两个编译器都是 cc -O2 构建后再自编译 -O2，预热后交替 7 轮，自编译中位数 0.777 降到 0.702 s（-9.7%，波动 0.769–0.782 对 0.699–0.706），tools.sh 的 11 个多文件程序 0.989 降到 0.921 s（-6.9%）。918 镜像与 a39f67c 相同；closure 570/0，nativeboot，bigclosure 6/0，opt 659/0，optpy 192/0。只测了 osx/arm64。**内存代价**：新增 6 个 OPT_MAXL 大小的 int 数组（em_pos、em_src、ol_from、pv_k、pv_rm、pv_wm），各 4 MiB，共约 24 MiB 静态虚拟地址空间；实测最大 RSS（自编译 a39f67c:unisacc.c）52,641,792 增到 56,999,936 字节（+4.2 MiB，+8.3%）。镜像字节数不变，不代表内存不变。**覆盖**：提速只归属于 osx/arm64 和上面两个负载；Linux、Windows 未测，c5c8f67 上的 acceptance、native、linux.sh、crossnative 未跑，要在下一次正式汇总或发布前、在冻结的最终树上补跑。缓存的前提：kind、rm、wm 只取决于行文本和 opinfo 表；将来若 opinfo、ABI 或消融模式在一轮之内可变，这个缓存就要随之失效。
- **重新采样后的下一项：词法器的最长匹配（c12e308，保留）。** 出货形态编译器（c5c8f67）的采样：优化器约 50%，但已分散（建行表 6.0%，复制 6.5%，活跃性 6.1%，ol_prep 4.4%）；最大的单一机制是词表查找，占 15.4%，其中约 40% 来自词法器。原因：每遇到一个运算符，都要把 TOKV 全部条目扫一遍，每条还要查 vlen 和 voff。改为按首字节预先列出候选，保持 TOKV 原序，所以选中的是同一个 token。A/B（固定输入 a39f67c:unisacc.c，cc -O2 构建后再自编译 -O2，预热后交替 7 轮）：自编译中位数 0.703 降到 0.646 s（-8.1%，波动 0.701–0.707 对 0.641–0.654），tools 的 11 个程序 0.917 降到 0.799 s（-12.9%）。918 镜像与 c5c8f67 相同；closure 570/0，stages 95，nativeboot 通过，bigclosure 6/0。lexdiff 在 examples、tests/c、tests/c99 上 150 一致、2 不一致（tests/c99 的 05_pragma_operator、53_inttypes）。c5c8f67 上也是同样这 2 个，是早已存在的差异；all.sh 不跑 lexdiff，所以一直没有暴露，待另查。
- **后续事项（cdx-unisacc 复核 c12e308 后提出，单独处理，不并入本轮）：**
  1. lexdiff 的两项差异还不能断定是词法器的错：lexdiff 同时比较预处理输出，要先分清是预处理、头文件、目标宏还是记号序列不同。
  2. `tests/lexdiff.sh` 的仪表缺口：只有 `set -u`；C 侧输出走管道，Python 侧命令替换失败后仍可能继续比较；两侧执行都没有单独的看门狗。修法：任何一步编译、预处理或词法出错都立即失败，每次执行加上不超过 60 秒的看门狗；确认后的差异转为可追踪的回归项。不要为了全绿而排除这两个文件。
  3. `pm_build` 在候选数达到 1024 时会静默漏掉候选（当前词表远小于此）。下次改动这里时，改成显式的容量失败，或按词表大小派生上限。
- 两项合计（同一口径）：自编译 0.777 降到 0.646 s，tools 负载 0.989 降到 0.799 s。只测了 osx/arm64。
- 体积（arm64，同一输入）：imm 融合与返回截断融合使自身镜像 693,666 降到 660,642 字节（-4.8%）；运行速度在噪声内。x86 没有做，要做就作为体积分支立项。

**J10. 构造离开种子（已暂停，2026-09-25）**

主人校准的目标（原话）：“应该也没有啥进化的啊，从种子层得到 unisacc 种子版本，然后种子版本能编译自己和编译其它 c99 项目，基本对标 tinycc/tccrun/cc/gcc 的核心功能应该就行了啊。”所以产品就是 `unisacc.com`：能自举（无 Python）、能编译运行 C99 程序的编译器；J10 不在产品路线上。

已达并保留为开发工具（`iterate/`，不属于产品）：`iterate/construct/` 用 C 构造 18 个阶段的 UNS2 整包，与 built.uns2 逐字节相同；`iterate/kernel/genmodel` 生成 model.inc 的 MODEL/DENSE/维度、16 个词表、BF/BH/HD 与 TYPEV，与 Python 裁判逐字节相同。未做：ENC_*、cases.inc、headers.inc、模板、来源头部；ENC 切片（c78f7d0）不合入。全部过程、判据与回执见 [`archive/prd-history.md`](archive/prd-history.md) 的“J10 全文”。

**S-16 0.0.8 计划**（2026-09-26 定；0.0.7 已于同日发布，v0.0.7，unisacc.com 1,254,432 B，sha256 00d25edb…）。

目标不变：像 tinycc / `tcc -run` / cc 那样，一个能编译自己、也能编译其他 C99 项目的编译器，达到可用于生产的级别。0.0.7 的教训（主人，2026-09-26）：`.com` 启动 1.6 s 与“不带选项输出词法转储”都不是测试发现的，而是使用时发现的——**测试套件要持续扩展，去倒逼问题**。每项带可测判据，单次运行不超过 60 s。

| 序 | 项 | 为什么 | 完成判据 |
|---|---|---|---|
| T1 | **正式的分片门禁** `tests/gate.sh` | 0.0.7 的门禁靠临时脚本，两轮约 10 分钟且有重复 | 一条命令；每片 ≤60 s；独立套件 2 路并发；整套 ≤4 分钟；`release.sh` 调用它 |
| T2 | **对出货物测试**：门禁里一组套件以 `UA=unisacc.com` 运行 | 0.0.7 的两个缺陷只在 `.com` 上可见 | cli、run、multi、diag、hostile、closure 用 `.com` 跑；另加启动时间回归（第二次运行 < 0.1 s） |
| T3 | **像用户那样用**：cc 行为对照套件 `ccparity` | 退出码、错误输出、产物命名、失败不留产物，此前无人检查 | 与 cc 对照 ≥30 例（无选项、`-o`、多个 `-o`、缺文件、空文件、无 main、编译失败、stdin、argv、退出码…），差异 0 或写明理由 |
| T4 | **平台覆盖补齐** | 0.0.7 未在 Linux x86_64 与 Windows x86_64 实跑 | Linux x86_64（Lima，模拟）每片 ≤60 s 的最小套件；Windows x86_64 实机或 UTM 跑 `.com` 与 `-b win/x86_64` 产物 |
| T5 | **UB 与实现定义的探针审计** | Linux 上两处“失败”其实是探针本身依赖未定次序与实现定义行为 | 扫描 tests/c 与 tests/c99 中同一实参列表内的多次修改、依赖 libc 实现定义行为的输出；逐个改写 |
| P1 | C1 的余项：`-c` 越界时的提示 | `-c lib.c` 现在报“undefined function main”，用户看不懂 | 提示改为说明“一次调用编译整个程序” |
| A1 | type 表对照 cc 审计（S-15 A1） | 表本身会错（[G-2]） | `gold_audit` 分歧 0 |
| A2 | syscall 号对照系统头（S-15 A2） | 手录 | Linux 两架构、macOS 分歧 0 |
| B2 | `.com` 的 Windows 部分自解压（S-15 B2） | PE 占约 62% | `.com` 降 ≥30%；两台 Windows 通过 |
| R1 | **论文 A 的推导** | 主人：理论要支撑生产级 | 论文 A 只陈述已证的（T1：网络 = 表）；[`research/delta-framework.md`](research/delta-framework.md) 是未证草稿，不进论文；推导的每一步有对应的实测或证明 |

**不进 0.0.8**：G1–G3（C11/GNU 扩展，超出 C99 边界）；J7 SIMD；J10（已暂停）；delta 框架的实现（只做理论）。F2 签名策略仍待主人决定。

**S-17 模型化迁移路线**（2026-09-26 定，主人：“直接按整个编译器规划迁移”；按 cdx-unisacc 审阅修订）。**本条覆盖 S-16 中“delta 框架的实现不进 0.0.8”一句**：迁移是 0.0.8 起的主线，S-16 的测试项作为它的安全网照做。

**目标**：每个架构的切片只是一个**几 KB** 的通用执行器（主人：先用 C 实现，再用汇编实现）；编译的动作由模型推理完成；模型在 `.com` 里只存一份。理论框架与未证之处见 [`research/delta-framework.md`](research/delta-framework.md)：已证的只有 T1（网络 = 表）。几 KB 是目标，不是定理。

**参考与证据的边界**：
- 现有手写编译器是**行为参考**，固定在一个参考提交上，不删除。
- 新旧在已测输入上的一致，只是经验性证据，**不构成**全域等价，也不构成 C99 语义正确性的证明。只有将来证明了“新旧对全部声明输入等价”，并且旧实现有相应的语义保持定理，两者组合才得到 T3；目前两者都没有。
- 旧编译器是整个参考过程，不直接就是有限的 δ_ref。每层都要说明：局部的状态、观测、动作如何定义，参考转移如何从旧过程得到。T1 只证明网络等于这张局部表，不证明“从旧过程提取表”是对的。多步动作序列要定义有限编码与解码。

**执行器**（选项 A，低层通用原语）：读字节、比较、定宽算术、栈、字典读写、输出追加、调用 δ。定宽算术必须写明宽度，以及溢出、移位、除零的规则，并且各目标一致。以下东西**不能**悄悄变成执行器的语言相关原语：宏摘要（E2）、类型与符号摘要（E3）、活跃性（E4）、模板选择（E5）、重定位布局（E6）。它们要么由 δ 调度通用原语计算（计算摘要的算法本身也在迁移范围内），要么明确列为暂未迁移的可信组件。

**模型的口径**：报告时写明实际运行的是什么——网络推理、经全域验证的 DENSE 查表、还是状态机字节码。构造成网络，不等于运行时执行了网络。

**产物账**（每个里程碑报告，压缩前与压缩后都报）：通用执行内核；OS 启动与系统调用适配；加载器与解压器；随带的 C 库；模型；模板；`.com` 总字节。编译特有的代码不能搬进模板后漏记。“几 KB”指通用执行内核，**按未压缩的机器码字节计**。目标靠结构达成——切片只做“读取 → 推理 → 执行动作”，编译的每个环节都由模型推理完成——**不靠压缩**；压缩后的字节只作参考列出。

| 里程碑 | 层 | 比较对象与判据 |
|---|---|---|
| **E0** | 执行器 + 玩具 δ（delta 草稿 §4 的 LL(1) 表达式文法） | 执行器先用 C，cc 与 unisacc 双构建；报告内核字节；再给一个 ISA 的汇编版本并报告字节 |
| **E1** | 词法 | 对 lexdiff 语料逐记号相同，含拒绝的输入与诊断；只回答词法层的规模，不外推 |
| **E2** | 预处理 | `-E` 输出、退出状态、诊断与参考提交相同 |
| **E3** | 解析与语义 | tape、退出状态、诊断相同 |
| **E4** | 优化 | -O1/-O2 tape 相同 |
| **E5** | lowering 与编码 | 六目标镜像相同；失败路径另行比较（镜像相同只覆盖成功路径） |
| **E6** | 镜像写出 | 同上 |
| **E7** | `.com` 布局 | 模型只存一份；按上面的产物账报告每一段 |

**阶段接口（2026-09-26，主人：流水线以后可能调整，接口要灵活）**：每个阶段是一对（δ，执行器），对外只有一种形状——**字节流进，字节流出**，外加一个退出状态（接受 / 拒绝 k / 未覆盖）。阶段之间不共享内存、不共享符号表；需要传递的信息必须编码进输出流（例如 E3 需要类型拼写，就由上游在记号流里带出来）。每个阶段声明自己的输入格式与输出格式（一个格式名 + 一份可执行的格式说明/检查器），流水线由一份清单（阶段名、δ 文件、输入格式、输出格式、顺序）描述，运行器按清单把阶段串起来，相邻阶段的格式必须一致才允许连接。这样增删、拆分、合并、重排阶段，只改清单和相邻两端的格式，不改执行器。现有阶段的格式：源文本 →（E2）→ 预处理后文本 →（E1）→ 带类型拼写的记号流 →（E3）→ tape 文本 →（E4–E6，待定）→ 镜像字节。

**切换规则**：指定语料与门禁通过后，才把默认路径切到新实现；旧实现保留在参考提交里，随时可对照。

**速度**：固定参考提交、相同输入、目标与优化级、相同计时边界，重复 5 次取中位数，每个里程碑照常测量并记录。**2026-09-26 主人定：速度等功能完善后再统一优化**，慢于参考 3 倍不再是停下来的门槛；E1 实测为 cc 构建执行器约慢 3.7 倍、unisacc 构建约慢 17 倍（计时边界不同，待同口径重测）。

**论文 A** 只吸收已完成、有证据的结果。

**S-17 进展与决定（2026-09-26）**

| 层 | 实际运行的形态 | 规模（实测） | 与参考的一致性 |
|---|---|---|---|
| E0 执行器 | C，通用原语 | cc -Os `__text` 4,748 B | T1 穷举通过 |
| E1 词法 | 构造式整数网络 → 全域求值的稠密表 | 335 个状态、1,215 个单元、UNS2 58 KB | 语料 243/243、lexdiff 110/110 |
| E2 预处理 | 状态机查表（Python 执行器） | 621 个状态、稀疏约 72 KB | 示例 99/107、语料 235/249，0 不一致；含函数宏、#/##、#if 求值、自动 include |
| E3 解析与 tape | 状态机查表（Python 执行器），手工逐写法铺成 | 5,230 个状态、约 130 万表项 | 随带头文件 stdio/stdlib/string/ctype/inttypes/wchar/assert 全部声明一致；语料 32/110，0 不一致 |
| 流水线 | `exec/pipeline/run.py`，E2→E1→E3 端到端 | — | fib/hello/fact 从源码到 tape 逐字节一致，不借助参考 |

**决定：E3 改为结构化生成**（主人：“不够聪明，一直在穷举，没有设计出聪明的结构”）。扁平状态机把文法、属性（类型/宽度/转换）、模板三种知识揉在一起，规模是乘积。改为三张声明数据 + 一个通用 LL 驱动：文法（产生式）、属性表（直接用 gold 的 type/tyinfo/pfconv/binsel/irsel）、tape 模板（每条产生式一段带槽位的模板，实测一次），规模变为三者之和。设计见 [`research/e3-structured.md`](research/e3-structured.md)，原型在 `exec/parse2/`；旧 E3 保留为已验证的参照，新结构在同一批输入上做到 0 不一致且更小之后才替换。

**向 tinycc 学的三点**（主人：“好的地方一定要学”）：
1. **流式衔接**：阶段按需拉取字节，不先物化整份文本（接口不变）。
2. **值描述符**：栈符号带有限的值描述（常量 / 局部槽 / 全局 / 左值地址 / 已在 r0 × 类型），模板按“运算符 × 操作数描述”选；先生成与参考相同的冗长 tape，稳定后再换成延迟落地（那时判据改为行为一致，属 E4）。
3. **回填**：执行器新增通用原语 ORES/OFILL（预留字节、之后回填）。实测参考自己就是这样做的——`.frame` 的数字总是右对齐占 7 个字符——所以不再需要把函数扫两遍。

**迁移逼出的产品缺陷（均已修复、配了与 cc 对照的探针）**：预处理嵌套宏调用丢实参、缺 hide set、`#if` 不认十六进制与括号；`!p` 的结果被当成指针；调用结果的有无符号与宽度取自最后一个实参而非返回类型；字面量到文件尾未闭合时越界读 1 字节；随带 malloc 只有 64 KB 且 free 不回收。

**已知未决**：头文件里定义的 printf——参考即使在 stdio.h 定义了 printf 仍走内建降级（每个实参占一个帧槽），多头文件时正文顺序与插入顺序相反；E3 目前明确拒绝。E2、E3 的表还不是网络（只有 E1 是）。C 执行器还没补上 E2/E3 的原语。

**结构化 E3 原型（`exec/parse2/`，2026-09-26 晚）**：模板表（实测一次的 tape 片段）+ 由 prec.tsv 生成的运算符层级 + 编译成过程的文法 + 回填帧大小。现覆盖 int 函数与参数、局部变量与块作用域、全部运算符（含 && ||）、赋值与复合赋值、++ --、if/while/for、调用、printf 内建展开：**843 个状态，探针 10 个、语料 15 个与参考逐字节一致，0 不一致**。诚实对比：旧 E3 在相近覆盖时约 1,009 个状态、语料 18 个——状态数只小一些，目前的收益主要是“知识变成数据”（模板行、由表生成的层级）和去掉两遍扫描。真正要验证的是下一步：类型（指针、char/long、数组、struct）走属性表后，规模是否按加法增长而不是乘法；若否，这个结构就没有达到目的，要如实记录。新增的实测：参考的 `.frame` 字段固定 7 个字符宽（它自己回填）；块结束后栈槽被复用，帧取最深点；常量池里 `"` 写作 `\"`、`\` 写作 `\\`，其余不可打印字节写作 `\xHH`。

**加法增长的第一个证据（2026-09-26 晚）**：给结构化原型加上类型（char/short/long/void、指针层数、& * 下标、强制转换、指针加减按被指类型缩放、按类型截断的返回与调用结果）只增加了 281 个状态（843 → 1,124），因为读写宽度由一个按“值描述”分派的共享过程决定，而不是每种写法按每种类型各铺一遍。对照：旧 E3 在加完同类功能时约 2,128 个状态。探针 17 个、语料 17 个一致，0 不一致。新实测：语句只写 `p[i];` 时参考只算地址不读取。

**结构化原型进度（2026-09-27）**：加上全局变量与数组（+163）、typedef/static/原型/名字随函数撤销与自动 include 检查（+76）后为 1,363 个状态，探针 23、语料 22 个一致，0 不一致。新实测：函数帧最终向上取整到 8；全局变量在声明处输出 `.bss g_名 大小`，int 初值统一在 `__init` 里写入；原型不产生任何输出，所以函数头与参数保存要推迟到看见 `{` 才写。下一步：struct、unsigned（随带头文件里剩下的主要卡点）。

**结构化原型进度（2026-09-27 续）**：struct、unsigned char/short/long、long 常量、系统调用内建、__argc/__argv、do/break/continue、?:、字符串字面量、变参定义与 va_*、double 存储依次接入，2,147 个状态；探针与语料合计 70 个一致，0 不一致；随带 string.h 19/19、stdio.h 44/48。唯一一处明显的非加法增长：unsigned 让每个运算符的有/无符号分支在两条层级链里各复制一次（+296），待把“选指令”收成一个共享分派。新实测：原型不占返回标签；va_arg 取 unsigned char/short 不掩码。

**结构化原型进度（2026-09-27 再续）**：函数指针、~、void return、exit 的返回路径、double（常量、四则、比较、cvtid/cvtdi）接入；随后把每个运算符的尾部逻辑收成一个被两条层级链共享的过程，**同样的输出从 2,549 降到 2,279 个状态**——这就是“结构化”的收益在起作用。当前 2,279 个状态、84 个一致、0 不一致；随带头文件 string/ctype/inttypes/time 全过，stdio 44/48、stdlib 21/40。新实测：双精度二元运算的固定形式（右值 cvtid 后压栈、左值从 [r7+8] 取回并 cvtid、mov r1,r0、.frame -16、f 指令，> 与 >= 用操作数颠倒的 flt64/fle64）；函数指针的局部被调方用 `load64 r0, [r6-N]`；本文件定义了 exit 时 __main_ret 先 `call exit`。

**工作方法**（实测教训）：需要读懂 gen.py 的改动由主会话直接做（一轮验证约 8 秒）；子代理只做测量、独立脚本和其他目录的事，用 sonnet、10 分钟以内；共享 /tmp 的产物一律按内容哈希打戳（`exec/stamp.sh`）。

**远期设想（主人，2026-09-26，明确“完全不应该急”）**：推理替代程序与函数会带来少量性能损耗；将来可对性能极敏感的少数环节，用 FFI 一类的外部调用做针对性封装。理论上的位置：被外部调用的原生代码就是 delta 草稿 §3 选项 B 的“可信组件”，必须逐个列出、计入产物账，并说明它替代了哪一段 δ 以及两者的等价证据；不能悄悄进入执行器。在功能完成、速度统一优化之前不做。


<a id="5-7-5-9"></a>
## 5.6–5.10：2026-09-27 巡查与评估（5.7 C99 子集措辞核查与缺口；5.9 真实系统调用缺口）及指针

### 5.6 路线建议与完成度评估（2026-09-27 研究员估计；已归档）

两位研究员按脱敏简报给出的完成度区间与路线建议，非规范、未独立复核，已被 v0.0.10/v0.0.11 的实际回执取代；原文见 [归档](archive/prd-r9-r10-receipts-20260929.md#5-6)。

### 5.7 "C99 子集"措辞核查与两处未记录缺口（2026-09-27，cc-unisacc 回报；cdx 核对清单）

**背景**：README 称 "A C99-subset compiler"。核查了它为什么不叫 "C99-compatible"，兼容度多少、缺什么、缺的东西有没有排进计划。

**已有的两个量化**（现成套件，非新测）：
- `tests/c99.sh`：按标准自身变更清单写的 57 个探针，实测 **57/57（100%）**。
- `tests/corpus.sh`（c-testsuite，外部语料 220 个程序）：本轮有界分片实测 216 pass、0 wrong、0 unsupported、4 knownfail；`tests/corpus.knownfail` 的四项是两个 GCC 语句表达式、一个空结构体、一个 C11 `_Generic`。此前回报的 214/6 是旧数字。57 个特性探针和 220 个语料程序都不是 C99 完整性证明。

**"subset" 而不是 "compatible" 的三层原因**：
1. 构建系统不兼容，不是语言问题：`-c` 不产目标文件，没有链接器、没有 `-l`/`-L`（README 已写明）。
2. 明确排除了不属于 C99 的东西：C11 `_Generic`、GCC 扩展（语句表达式、空结构体、inline asm，README 已写明）。
3. 真正的语言/库缺口，下面两条由 cc-unisacc 回报；cdx 核对了头文件清单，未独立复跑三字符组探针：
   - **三字符组（trigraphs）未实现**：`??<`/`??>` 之类直接语法错误。
   - **标准头文件缺 6 个**：C99 要求 24 个标准头，现在有 18 个（`assert ctype errno float inttypes iso646 limits math signal stdarg stdbool stddef stdint stdio stdlib string time wchar`），缺 `complex.h`、`fenv.h`、`locale.h`、`setjmp.h`、`tgmath.h`、`wctype.h`（覆盖 75%）。头文件数量只描述文件清单，不表示已有头文件的接口与语义全部合规；不能用后续标准中的可选特性宏来豁免 C99 的要求。

**`<setjmp.h>` 不算未记录缺口**：D1（本节前文，已有条款）写明是**评估过、主动决定不做**：tape 没有间接跳转，也读不到 arm64 的 LR，需要新的 tape 操作族并在两个后端六个目标上 lower；这是权衡过的架构决定，不在"未来安排"里，是永久性的。

**结论**：将缺失头文件与三字符组登记为覆盖限制；不因本次登记自动排期。`setjmp.h` 保留此前的明确非目标说明。

### 5.8 跨目标验证：examples/apps 四程序（2026-09-27；已由套件覆盖）

procview/winlayout/memmap/exeinfo 当时的逐目标实跑回报已归档（[归档](archive/prd-r9-r10-receipts-20260929.md#5-8)）；现由 `apps` 门禁与 `tests/crossnative.sh`（六目标 examples）持续覆盖，不再单独维护。

### 5.9 真实系统调用缺口——对标 tinycc/cc 的一个真实短板，需要修正（2026-09-27，主人定性）

**状态（2026-09-29）**：目录列举已补（Linux `getdents64` 与有界 dirent 适配，见 R9 状态归档）；`fork`/`exec`/`popen` 仍未暴露给用户代码，登记为 R12-2 子项 ⑥，不再在此处扩写。

**纠正一（方法论）**：不同编译器产出的二进制本来就不会完全一样，这正是测试套件重要的原因——TDD 把"效果"（可观测行为）限定为一致，不要求内部实现或内存布局一致。cc-unisacc 之前以"两个编译器产出的自身 /proc/self/maps 逐字节对不上"为理由，判定 memmap.c 不该自动读自身内存映射，这个判断用错了标尺：**逐字节对拍 cc 只是本项目众多验证手段之一**，不是唯一合法手段。对于"自身内存映射"这类天然依赖具体二进制布局（动态链接与否、缓存路径等）的输出，正确做法是换一种测试仪器（结构自检、与独立真实来源交叉核对），而不是因为"cc 比不了"就放弃这个功能。**结论未改**（这次仍未启用 memmap.c 的自动 `/proc/self/maps`——原因是还没设计出替代的验证仪器，不是因为它不可行），但理由记录纠正为：缺一种新测试仪器，不是缺一个可行的功能。

**纠正二（定性，主人 2026-09-27）：这不是"可选功能"，是对标 tinycc/cc 的一个真实短板，需要安排修正，不是记录后搁置。** unisacc 自己从零写的 C 库没有实现这些系统调用/绑定，tinycc 和系统 cc 链接完整系统 libc，天生就有；这是设计与实现的缺口，不是架构选择：
- **目录列举（`getdents`/`getdirentries64`）**：完全没有。procview 在 Linux 上只能靠对 pid 逐个探测（已实现，见 76bf9a0），不能真正列目录。**已核实的系统调用号**：Linux `getdents64`：x86_64 = 217，arm64 = 61；macOS `getdirentries64`：两个架构统一为 344（`0x2000158`，遵循 abi.tsv 里 osx 系统调用号的固定 `0x2000000` 前缀模式）。Windows 没有等价的原始系统调用，走 `FindFirstFileW`/`FindNextFileW` 这条 WinAPI 路（与现有 abi.tsv 里 `gate` 字段的 `winapi` 分支一致，不是 `syscall`/`svc0`/`svc80`）。
- **`fork`/`exec`/`popen`**：完全没有暴露给用户代码；但 S-17 迁移原型的 ABI 目录（`weights/gold/abi.tsv`）**已经配好 `clone`/`execve` 的系统调用号**（Linux：`clone` x86_64=56、arm64=220，`execve` x86_64=59、arm64=221；macOS：两者的 osx 行都是 `0x2000168`/`0x200003b`），只是没有接到现有前端的内建名字识别（`src/front_parse.c` 里 `__open`/`__read` 那一类）或头文件封装上——**这是四个缺口里工作量最小的一个，数据已经齐了**。Windows 同样没有 fork 语义，只有 `CreateProcessW`。
- **`sysctl`**（macOS 不用 `/proc` 拿进程列表就得靠它）：完全没有配号，是真正的新工作，需要先确定用哪个 `KERN_PROC_ALL` 变体和它的 BSD 系统调用号。
- **窗口系统访问**（X11/Wayland socket、Win32 API、CoreGraphics）：完全没有绑定，比系统调用封装更难（涉及协议或框架链接），四个缺口里工作量最大，其余三个应该先做。

**历史参考及后续纠正**：此前系统 cc 构建的窗口采集器仅证明系统接口可用，未达到用户要求；该采集器现已删除。macOS 的 `winlayout.c` 已由 unisacc 模型候选通过 dl/ffi 自行调用 CoreGraphics/CF；门禁原始快照也由该应用的 `--capture` 获取。cc 只分析同一快照作行为参考。Linux/Windows 窗口绑定仍未实现，不能以 macOS 结果外推。

**下一步（主人：安排修正，不是"按需再评估"）**：
1. **`fork`/`execve` 接上前端内建名字**，Linux 与 macOS 先做（数据已备好，见上）；Windows 需要 `CreateProcessW` 这条不同的路，可以后做。
2. **`getdents64`/`getdirentries64` 接上**，用于真正的目录列举（不是 procview 现在的逐 pid 探测）；Windows 走 `FindFirstFileW`/`FindNextFileW`。
3. 设计"结构自检"类测试仪器（不依赖与 cc 逐字节对拍），用于验证自身内存映射一类天然依赖具体二进制的输出，作为 memmap.c 自动读取 `/proc/self/maps` 的前置条件。
4. `sysctl`、窗口系统绑定排在后面，工作量更大。
5. 一个后台代理（opus，独立 git worktree，不碰 cdx-unisacc 正在用的工作树）已按此清单第 1、2 项开工，见 §6 或后续记录其结果；这两项不必等 cdx-unisacc 手上的大任务腾出手再做。

### 5.10 bit-field 存储单元宽度错误（2026-09-27 发现；已修复）

2026-09-29 复验：最小复现 `struct a{unsigned short r:5,g:6,b:5;}` / `struct b{unsigned char x:3,y:3;}` / `struct c{unsigned r:5,g:6,b:5;}` 的 `sizeof` 在 cc 与 `unisacc.com -run` 下均为 2 1 4；`examples/apps/colorpack.c` 输出逐字节相同。原巡查记录见 [归档](archive/prd-r9-r10-receipts-20260929.md#5-10)。


<a id="7-4"></a>
## 7.4 仍可重选（历史种子设计）

### 7.4 仍可重选（历史种子设计）

§3.3 bilinear 形式 · `SYMS` 拼写 · fib/ptr/struct 常量 · §3.7 亚字节打包顺序。**除此之外全部承重。**



<a id="7-1-7-2"></a>
## 7.1 构建顺序（早期 M1 分工）与 7.2 三条红线（历史经典路线）

### 7.1 构建顺序

```
M1 数学与网络     rng → linalg → net → gold → train      ⟦A-12, A-14⟧
M2 权重落盘       uns1 + quant                            ⟦A-11, A-13⟧
M3 tape 与 VM     tape → vm                               ⟦手写 tape 跑通⟧
M4 C99 前端       pp → lex → parse → sema → ir            ⟦七样例出 tape 且 VM 正确⟧
M5 Lowering       lower → exec_target                     ⟦A-5, A-8⟧ ★主目标
M6 镜像           emit_x86/arm → image/*                  ⟦A-9, A-10⟧
M7 Ship           ship + kernel/unisa_core.c              ⟦A-15, A-16, B-2⟧
M8 验收与度量     tests/acceptance.sh + bench             ⟦B-1..B-6⟧
```

### 7.2 三条红线（历史经典路线）

以下第 3 条是早期 T-1 的分工，不适用于 S-17 当前整阶段模型路线。当前约束是通用执行器不隐藏语言相关逻辑，规则/动作、模板与宿主边界见 §0.4.1；fold 与构造全域验证仍须按实际模式执行。

1. **fold 不许退化成「通用 tape 跑六遍自比」**（X-4）—— 那就不是测试
2. **acc 不达标不许加宽网络**（F-5）—— 是 key 编码错了
3. **走查器 / 符号表 / 文件头不许神经化**（T-1）—— 它们是代数，不是表

