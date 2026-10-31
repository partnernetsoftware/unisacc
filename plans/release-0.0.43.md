# 0.0.43 发布：重新来（2026-10-30）

## 目标

可发布的 `unisacc.com` 0.0.43 要同时满足三条：

1. 由 HEAD 源码用新 CX 入口自己构造出来。
2. 两代判词：第一代编出的产品再编一次，五片逐字节相同。
3. 同一候选上 `tests/run.cx -- --all` 的结果为 QUALIFIED。

签名、Draft、公开等主人授权。

## 原则（主人定）

核心是 nn 推理字节流，用它把 8 个流和 18 张网串起来，产品应当优雅、简约。
- 能写成模型步的差异，就不在代码里写特判。
- 平台差异只放进 L0（cx/sys）或表里。
- 构造链也照这条办：不在旧链上打补丁，写一条短的新链。

## 主刀与分工

- **grk 一人主刀发布链**（AGENTS.md：慢车道一次只一个主刀）。构造、安装、产品源都由它改。
- **cc 只做三件事**：给方向、审每一个提交（看是否符合范式、有无回退）、按 grk 的请求补 L0 和工具。构造和安装不另派子代理。
- **bdy、csih**：不碰发布链的文件。

## 新链（取代 package-refresh、product-build、candidate-install 这条旧链）

旧链读的是已归档的 C 源（exec/c/core.c、asmcompiler.c），收的是旧的五片收据。它不再修，等新链跑通后归档。

1. **hostnative 变薄**：clock、utimes、loader 都转到 cx/sys，去掉只认 Linux 的 `#error`。osx 和 win 片要能原生运行。
2. **新写一个工具 `tools/release-build.cx`**，一个命令走完以下五步，每一步都写出身份（sha256）：
   1. **P3**：模型加上 `include/` 携带的头，其中 `\0hdr/cx-flat.h` 必须与树里的文件逐字节相同。复用现有的打包模块，不经 package-refresh。
   2. **五片**：用 `tools/entry-build.cx` 每个目标编一片。lnx 走 33 个对象；osx 和 win 在 object 模型步落地之前先用整只 `-o`，并在记录里写明 route。
   3. **wrap**：用 `seed/ape.c` 包装，生成 `unisacc-next.com`。
   4. **第二代**：用第一代的产品再编一遍，五片两两逐字节比较。
   5. **安装**：两代相同，且 `tools/check-install-fixes.cx` 4/4 通过，才替换 `./unisacc.com`。新身份写进 prd.md。
      安装后恢复这三处 `use`，删掉手抄原型：`cx/driver/input.cx` 的 `sys.fs`，`cx/runtime/model_decode.cx` 和 `cx/runtime/model_lifetime.cx` 的 `mem.bytes`。三处注释标了 `TEMP-UNTIL-INSTALL`。根子是已装产品不按真实路径去重模块（安装修复 dedupe）。
3. **osx 和 win 的 object 模型步**：在模型里补上路由，走"事实表 → 生成器 → 网"。不阻塞第一次安装，装好以后再做。
4. **测试**：在新产品上跑 `tests/run.cx -- --list`，用 all-waves 跑所有不需要材料的批次，再补材料、实现 PLANNED 的 19 行，最后跑 `--all`。

## 任务板（`sh ./unisacc.com tools/release-board.cx` 按此表计数；每轮巡检对比 open 有没有下降）

发布缺口以域矩阵里的必需格计，不靠估算。2026-10-30 起点：必需 825 格，已覆盖 463，MISSING 211，NEEDS_FOREIGN_HOST 79，NEEDS_PRODUCT_CHANGE 20。

- MISSING 的格：写批次或断言，在 lnx 上实跑 PASS 后改标 COVERED_LOCAL，带 batch@candidate 证据。
- PRODUCT 的格：先修产品。
- FOREIGN 的格：安装之后由 CI 在各宿主上实跑。

grk	stream-e3,stream-elf,stream-lower,stream-e1,stream-e2,stream-e4,stream-prune,bootstrap,candidate,phase-timing,optimization,formats
cc	ri,resources,negative,hosts,library,interfaces,properties

- **grk**：先做发布链（hostnative → release-build → 安装），再做所负责域的 PRODUCT 格和 MISSING 格。libunisacc 的 us_* 库 API 在活跃区没有实现（实现随 e16028ffa1 一起归档了），这是产品缺口，也归 grk。
- **cc**：做 ri、resources、negative、library、interfaces、properties 这几个域的 MISSING 格；安装之后负责在 CI 上跑 FOREIGN 格。
- **bdy 不在发布板上**：主人安排它专做 cx 核心底层和上层封装，cc 不给它派发布格。

### bdy 域格的材料依赖（2026-10-30，bdy 实测）

- **properties 已清零**（missing=0）。
- **library 30 格卡在「产品缺口 + P3 材料」**：`tests/cases/p3_library_members.c` 第 15 行 `#include "exec/c/libunisacc.c"`，但 `exec/c/libunisacc.c` 与 `exec/c/run.c` 已被 e16028ffa1（10-29）归档到 `archive/reference-c/exec/c/`，活跃区无 `us_new`/`us_free`/`us_add_*` 等库 API 的任何实现（symindex `def us_new` count=0）。14 格断言已写在 p3-library-members.cx 的 api_expect，另 16 格缺断言；全部要等 grk 重新提供 libunisacc 活跃实现（产品缺口，归 grk）+ P3 包（release-build 第一步）才能实跑。
- **interfaces 11 格缺的材料**（等 P3 或材料打包，reason 按域矩阵）：
  - draft（2）：if.attr.sidecar-ownership、if.edge.consumer-frames —— 对应流/网草案未定。
  - 缺语料（1）：if.edge.corpus —— 需要语言形态语料，非两个 fixture。
  - 缺 tapecheck 材料（1）：if.route.tapecheck-pass —— streams.json tapecheck_included=false。
  - 缺注册批次（1）：if.route.tapecheck-neg001 —— 无批次；产品现报 `unknown opcode`（非 spec 的 `Tape lexical form`），产品诊断文本待 grk 对齐。
  - units/multi 未打包（1）：if.route.units。
  - memory entry 未打包（1）：if.route.memory。
  - 材料仅 O2（1）：if.route.optimization-levels —— 需 O0/O1 链材料。
  - 缺断言（1）：if.route.prune-placement。
  - 无六目标链批次（1）：if.target.foreign-chains —— 需外宿主 CI。
  - 本地列不全（1）：if.host.lnx-x86_64 —— 同一候选汇总行。

## 进度（grk 每完成一步，在这里写上提交号）

STATE43：**当前状态（cc 维护，2026-10-31 中午；下面的流水记录是历史，以这一块为准）**
STATE43：已装 0.0.43 = `fc1d4513`（10-31 14:19，5,025,194 字节，五片都是整程序 -o，冻结 rev cd1a95ebcc，P3 aec1a0bf；上一版 43fbbb81 备份在 out/unisacc.com.43fbbb81，再上一版 79fb84e6 在 out/）。结构和体积用 `tools/com-tree.cx` 看。
STATE43：[x] 1 hostnative（28d0cba49c）。
STATE43：[x] 2 构造链：release-build 五片、release-ape 打包与冒烟、安装（check-install-fixes 4/4，`--smoke` 16/16，冒烟 5/5）。
STATE43：[x] 3 **RI 迁移**（fb89faddde 防呆、db6938d868 严格重放与捕获、fbd9cb64c7 与 f6c3ef3dd0 trace 与 multi）：已装的 fc1d4513 上，tools/affected.cx 跑 resources、ri-trace、ri-trace-output、ri-multi 四个批次 4/4 PASS（断言 74、15、17、10，全部 0 失败），cc 独立复跑确认。旧产品对同一测试 rc2、捕获不写文件，对照成立。rf-capture-guard 作废（见文末），不登记。
STATE43：[x] 4 **两代比较，五片全部逐字节相同**（冻结 rev cd1a95ebcc；第一代构造器 79fb84e6，第二代构造器已装的 fc1d4513，各片 tools/bound 60 内编完，空闲机器上只跑一次）：lnx/x86_64 0ae1986b（53.2 秒）、lnx/arm64 14abd693（54.9 秒）、osx/x86_64 d4e43786（56.8 秒）、osx/arm64 16b6dd58（58.9 秒，离界只差 1 秒）、win/x86_64 1b0c3353（39.0 秒）。cc 对五对文件逐一 cmp 确认。注意 osx 两片已经贴着 60 秒界，构造器再慢就编不完，这是 C44-SPEED-01 的急事。
STATE43：**待产品改 20 格分诊（cc，2026-10-31，grk 列的清单）**：
STATE43：(a) 小改，攒成一个候选一起做（0.0.43 内）：resources 2 格（UNISA_MAXSTEPS 的 abc、-1、0 要按名字拒绝；输出限额比镜像小要报错而不是留下截断文件还报成功）、phase-timing 7 格（构造冷热、热路径写明跳过了模型解压和推理、输出耗时冷热，都是往 RR_TIME 里补行，不能拖慢）、library 的 invalid-options 1 格（私有运行时已修，要进产品）。
STATE43：(a+) cc 在已装 fc1d4513 上实测又找到两个产品缺口，加进 (a)：① 编译时地址空间限制（ulimit -v）低于约 225 MB，产品段错误（rc 139，旧 79fb84e6 也一样）；格要求具名拒绝 `run: out of memory` rc2、没有输出文件，信号死亡算失败（res.mem.compile-as-limit）；250 MB 以上都正常。② `-run` 的用户程序在地址空间限制下 malloc 失败时，运行时崩溃（rc 139），格要求程序看到 NULL 并用自己的退出码退出（res.mem.user-malloc-null，复现：ulimit -v 1500000，程序 malloc(3GB)）。
STATE43：(b) 位置映射 2 格（stream-e3 与 negative 各 1 格：宏展开里的错误，列号要指到用户源码上的列，现在钉在展开后的 5:34，合同要 5:24）。grk 估了大小：约一天，要改公开格式 UNIPP1（头上多一个计数，宏替换时记「展开偏移对上源偏移」）、parse2 的 tokenlocations 十二张、diagnostics 五张、pp 的 location 三张，诊断摘录要改读原始源，改完 SID 会变，不是快车道。**待主人定**（和 (c)(d) 一起）。
STATE43：(c) 库产品（libunisacc）3 格：递归回调签名、union/位域/long double 过 API、恰好导出 29 个 US_API 符号的产物。活实现缺失，是整块工作，**待主人定范围**。
STATE43：(d) win/arm64 原生 6 格（host.phase-report、host.diag-and-loader-corpus、ci.native、ri.windows-snapshot-and-e3-companion 的 win/x86_64 与 win/arm64 两条、E12.native-chain）：产品现在发布的五个目标是 win/x86_64、lnx/x86_64、lnx/arm64、osx/x86_64、osx/arm64，没有原生 ARM64 PE；矩阵要求的外平台主机集合含 win/arm64。**待主人定：做原生 ARM64 PE，还是把必需主机集合改成产品真正发布的五个目标。** 在主人定之前不改矩阵。
STATE43：**范围决定（2026-10-31，主人原话「你来安排」，cc 采纳下面的评估）：0.0.43 以五片单文件 unisacc.com 的范围发布；A、B、C 三组明文列为已知缺口，各挪到 0.0.44（C44-LIBUNISACC-01、C44-WIN-ARM64-01、C44-DIAG-COLUMN-01）。矩阵里 54 条豁免记录在 tests/waivers-0.0.43.tsv，应用后必需格 825 变 768，已覆盖 509，缺 136，待外平台 73，待产品改 9；由 tools/matrix-waive.cx --check 保证记录和矩阵一致；发布说明用 [release-0.0.43-known-gaps.md](release-0.0.43-known-gaps.md)。**
STATE43：**范围评估原文（fable 子代理只读评估，cc 核对了其中两处事实）**
STATE43：A libunisacc → 建议明文列为已知缺口挪到 0.0.44。依据：公开发布物历来只有 unisacc.com（release/README.md:17、research/r42-release-acceptance.json:151-154）；库矩阵头写明它测的是宿主 GCC 加 libffi 编的私有 C 库、没有 recipe 造 libunisacc 产物（tests/library-domain-matrix.tsv:2-7）；tests/p3-library-members.cx:3-4 写明已装 APE 没有库路线；活实现随旧 C 归档，头文件自己写着递归回调、union/位域/宽浮点仍在开发中（exec/c/libunisacc.h:20-25）；工作量以周计，会重开冻结候选。发布说明一句：libunisacc 嵌入式 API 为开发期接口，未随 0.0.43 发布。cc 核对：cx-library-build-check 批次在已装产品上是 PASS，评估里「应当会失败」的推断不成立。
STATE43：B win/arm64 → 建议挪 0.0.44，并把矩阵必需主机集合改成产品真实发布的五片（需主人批准）。依据：论文 A 的五目标（含 win/arm64）是自举身份测量过的宿主集合，产品五片是另一个集合，论文自己写清了（main.tex:90、191）；spec T-5 是六个编译目标；seed/ape.c、ape-pack.cx、release-build.cx、CI（.github/workflows/candidate-native.yml:63-68）都已按「Windows on ARM 跑 x86_64 PE 模拟」设计，矩阵是唯一按原生 ARM64 PE 写的地方；矩阵里含 win/arm64 的约 22 行，不止 6 格；做原生要 ARM64X 混合 PE 或另发 exe，约 1 至 2 周并触及签名链。ri.windows-snapshot-and-e3-companion 的 win/x86_64 一条与 ARM64 无关，是 Windows 快照桩。建议改法是 required=no 加明文理由，不删行。发布说明一句：Windows on ARM64 运行 x86_64 PE（系统模拟），原生入口为 0.0.44 目标；-b win/arm64 写出的用户镜像是原生 ARM64。
STATE43：C 宏展开列号 → 建议挪 0.0.44，发布说明写一句。依据：spec.md 里没有 A-40 正文条款，只有 197 行脚本摘要一行；exec/pipeline/formats.md:126-128 明文禁止按 token 文本回找位置，所以没有不触 SID 的小做法；唯一的做法是 plans/dev-methodology-2026-10-28.md:60 的三步，约一天加一次纪元换证。现状行号正确，列号和摘录行按展开后文本。发布说明一句：宏展开内的诊断列号与摘录行按展开后文本计数（行号正确），0.0.44 改为用户源码位置。
STATE43：(a) 的更正：library invalid-options 不放进 (a)（产品里没有库路线，随 A 走）；phase-timing 在 lnx 上是 6 格（host.phase-report 的 win/arm64 一格属于 B）；resources MAXSTEPS 一格，runexec.cx:11,71 里有具名拒绝文本，但 cc 在已装 fc1d4513 上实测 UNISA_MAXSTEPS 为 abc、-1、0 时 -run 照常 rc7、-S 照常 rc0，没有拒绝，所以要查那段检查为什么没走到。
STATE43：[ ] 5 object 模型步（lnx 回到 33 对象依赖 C44-ENTRY-HOSTIMPORT-01，不挡 0.0.43）。
STATE43：[ ] 6 **测试到 QUALIFIED**。判定（tests/run.cx:28-33）：`tests/run.cx -- --all` = 本地 smoke 加 `tests/all-aggregate.cx` 对一次 `tools/all-waves.cx` 会话的汇总，要求每个 IMPLEMENTED 批次在**同一个候选**上有 PASS 回执，且没有未满足或未解决的覆盖行。矩阵头也写着：COVERED_LOCAL 的证据是候选专属的，旧候选（@94b549d1、@da4e7ef1、79fb84e6 等标签）的证据只是历史，不盖当前候选。所以步骤（cc 2026-10-31 理的）：
STATE43：6a 把还缺的格做完：当前必需 736 格，已覆盖 543，缺 73，待产品改 7，待外平台 73（10-31 傍晚；grk 在做 elf.pos、lower.pos 和普查红批次的分类，我做 resources、ri、interfaces、negative、library 剩余）。豁免记录 R1–R10，见 tests/waivers-0.0.43.tsv。
STATE43：**普查（10-31，tools/census.cx，报告 [census-2026-10-31.md](census-2026-10-31.md)）**：已装 fc1d4513 上 166 个无材料批次，120 PASS，46 FAIL（19 个 full-* 汇总 + 27 个真红）。真红里：ri-image-single/multi/pem 是 RI 迁移漏了 image 路线（source 阶段 + e3-identity.tsv），已判 A 补进最后候选；negative-model-loader 的措辞/退出码已改到新模型驱动，仍有 3 处加载器接受坏包（页脚长度翻位、D 行阶段名、terminal elf 行）是产品缺口；negative-diagnostics 是 -ferror-limit 被忽略；其余 formats-objects/formats-tapebin/link-*/pp-*/ucn-identities 等待 grk 分类 A/B/C；cx-* 六个待 bdy 判定。引用红批次的矩阵格已降 STALE-RED（matrix-stale），批次转绿后翻回。批次 negative-model-qprefix 已退役（依赖已归档的 run.c），格 loader.qprefix-malformed 并入 R10。
STATE43：6b 最后一个候选要收的产品改动（10-31 晚，按风险排序，每项走快车道、对应红批次转绿为验收、≤1 小时，做不动就豁免进 0.0.44）：① link-undefined-function：直接 -o 用到未定义函数时退出 0、出镜像、运行时 127（悬空成功）；② image 路线补 source 阶段 + e3-identity.tsv（ri-image-single/multi/pem）；③ 加载器校验页脚长度、D 行阶段边界、terminal elf 行（negative-model-loader、-terminal；cx/runtime 里，不加 drive.cx）；④ -ferror-limit=N 生效（negative-diagnostics）；⑤ -nostdinc 不再展开自带头（pp-cli-includes）；⑥ **6a（新发现，最高）已装产品不接受 .tape/.tapebin 作 -run 与 -b … -o 的输入（79fb84e6 rc7，现报 top-level construct，磁带被当成 C 送进前端），link-malformed-input 的坏磁带断言因此是假绿**；6b -S --tapebin 写 UTAPEBIN（formats-tapebin；cx 里没有编码器，一小时接不上就豁免 tapebin.roundtrip 进 C44-TAPEBIN-01）；⑦ -run bad.tape 报 unknown opcode（link-malformed-input）；⑧ phase-timing 六个 lnx 机内打点；⑨ 启动器 ulimit -v 预检（res.mem.compile-as-limit）。普查九批分类已出：formats-objects、pp-cli-macros、pp-header-locations、ucn-identities 是测试钉着旧退出码/句子（A，已改，断言条数不减，提交 3281ba27c2）。**这是最后一个候选**：之后冻结，冻结后对同一候选再跑一次普查。已决定不做：user malloc（R5）、MAXSTEPS 校验（R6）、-Werror（C44-WERROR-01）。
STATE43：6c 实现 19 个 `full-*` 汇总批次（现在是 PLANNED、NOT_READY，tests/pending 里文件还不存在；每个负责一个域：该域所有必需格都是 COVERED_LOCAL，外平台格都有同候选的 CI 观察）。cc 来写一个通用的 `tests/full-domain.cx DOMAIN`，读矩阵、逐格报哪些没盖上。
STATE43：6d 对冻结候选跑一次完整的 `all-waves` 会话：314 个 IMPLEMENTED 批次（其中有一批要先准备材料，176 个标 UNPREPARED），每个批次一次作业 60 秒内，轻量批次一轮四个并发；然后用 `matrix-flip-gen --migrate` 把证据标签从旧候选迁到冻结候选。这一步会跑一两个小时，机器要独占。
STATE43：6e 五个外平台（osx/x86_64、osx/arm64、lnx/arm64、win/x86_64 加 CI 上的 lnx）在同一候选上实机观察，需要把候选推到 CI（主人已批准推送）；73 格。
STATE43：6f 最后 `tests/run.cx -- --all` 输出 QUALIFIED。
STATE43：[ ] 7 主人授权发布（签名、Draft、Latest、公开）。目前一律不碰，等 0.0.43 做完再安排。
STATE43：不计入 0.0.43 完成度：名表铺设（nametab-gen 290 行规格、约 580 个模块，产品还没用）、通用工具、体积优化，都是 0.0.44 的准备，见 [v0.0.44.md](v0.0.44.md)。
**已安装（2026-10-31 01:19）：** `./unisacc.com` sha=43fbbb81…，4631130 字节，冻结 rev c98f757f，五片都走 whole-o，P3 是 aec1a0bf；check-install-fixes 4/4、`--smoke` 16/16、release-ape smoke 5/5、七个棘轮全绿。旧版备份在 out/unisacc.com.79fb84e6。
**头号待办（cc 复核时发现，挡第 4 步 QUALIFIED）：** CX 入口还没迁移资源身份（RI）。
- `--strict-resource-flow` 和 `--resource-ledger` 报 "model driver: option not migrated"（src/entrybind.cx:86）。
- 设置了 `UNISACC_RF_CAPTURE`，编译照常 rc0，却一个文件都不写，等于悄悄绕过账本，违反 ri 合同。
- resources、ri-trace、ri-trace-output、ri-multi 四个批次在新产品上全红。ri、resources 两个矩阵里的 COVERED_LOCAL 证据都是在 79fb84e6 上实跑的，新产品上不成立。
要做的：把 RI 的选项、捕获和严格重放迁移进 CX 入口；在迁移完成之前，带 `UNISACC_RF_CAPTURE` 的运行必须按名字拒绝，不能悄悄忽略。
已作废：rf-capture-guard 是 RI 迁移完成之前的防呆（设了 UNISACC_RF_CAPTURE 就按名字拒绝），捕获迁移之后它的断言不再成立（设了就真的捕获；空值等于没设，和 resources.cx 的用法一致），2026-10-31 删除，不登记；捕获的拒绝路径由 resources 批次覆盖。
**裁定（cc，2026-10-31，主人授权 cc 决定）：先做完发布链，CX 面的广度铺设移到 0.0.44（C44-CX-SURFACE-01）。** 已经落地的 stream-gen、net-gen、sys.fd 保留。从现在起，grk 只推第 2 步剩下的部分，顺序是：osx/x86_64 和 win/x86_64 收进 release-build 并做第二代 → lnx/arm64 和 osx/arm64 两片 → seed/ape 包装 → 安装（`check-install-fixes` 四项全绿）；然后是第 3、4 步。src/stream.cx 和 src/network.cx 在安装之后用 `--write-src` 收尾。
- [x] 1 hostnative 28d0cba49c（sy_reg_size 与 check-cx-sys 那一行 sy_eprintf 在 08da2e8863，索引竞态先被别的提交带走；precommit blocked=0）
- [ ] 2 release-build：P3 / 五片 / wrap / 二代 / 安装
  lnx/x86_64 一片已跑通，route=33 个对象。P3 只拼在 candidate 上，entry.elf 是链接结果。osx/win、其余四片、wrap、安装未开始。candidate 直接 --version 打印 unisacc 0.0.43，rc=0。runcache 未跑。
  p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf bytes=2039850 flat=match ns=373396397
  entry.elf sha=332756bb1662080520bb3c9973bedd027bc89227bcda1d191fcc67f76b855cf4 bytes=1398144 link_ns=7907732090
  candidate sha=99ea6d0e85734f15c99de4c8272c3d39d3afc30ef967cd581d6d513ec5b24126 bytes=3437994
  工具时钟：六只 -O1 --only ns=30663464644；其余 27 只 --only ns=24263170737；--link-only ns=9189543474
  二代：用候选 99ea6d0e 再编 lnx/x86_64，entry.elf 与 332756bb 不同。g2 entry.elf sha=0b56f314eb36db3d6706327f099dd3ba2b5875ebdedc340e88cb6363c0e0fb02 bytes=1398144，长度相同，从第 24 字节起不同。33 个对象里 32 个与第一代逐字节相同，只有 drive.o 不同（92814bf2 bytes=1373880 → 724e2bb8 bytes=1378472），多出来的全局符号是 tx_count。cx/text/text.cx 的 tx_count 在 ce3f7018fd（23:04）落地，晚于第一代 drive.o（22:59）。drive.o 是唯一携带 text.text 的对象。
  同一棵树再对一次 drive.o（text.cx 停在 476a09a592）：已装编译器与候选编出的 drive.o 逐字节相同，sha=556f24115e28c3077bad3be9aeeb63752184a84c5be3790e299ae9910a6174f6 bytes=1382392。
  二代工具时钟：drive.o ns=13246925759；其余五只 -O1 ns=21998369411；14 只 ns=11757854979；13 只 ns=14514585606；link ns=8151159408。同树对照：已装 ns=7294579227，候选 ns=8587396907。
  冻结 rev 两代（`--rev`，git archive 到 out/release-snap-<sha10>/，不读共享工作区）：rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。P3、第一代、第二代同一个 rev。33 个对象、entry.elf、candidate 都逐字节相同。
  entry.elf sha=fb2237f23f4b7a50dd8d3d2e92ad2f56da623fdd5c0ec2f12193973004337775 bytes=1402240
  candidate sha=a6ddb03bd6dd356bc194ede1ed7c66a350bd011f4020de2e6634443061ddb975 bytes=3442090
  p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf
  第一代构造器是快照里的已装 unisacc.com（via=sh，sha=79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32）。第二代构造器是第一代 candidate（via=exec）。
  工具时钟：第一代六只 -O1 ns=31485383890，27 只 ns=24720043367，link ns=9093827509。第二代六只 -O1 ns=29852339534，27 只 ns=26229984423，link ns=8744013468。
  同一冻结快照上手跑整程序 -o（还没收进 release-build，收据外记 route=whole-o，镜像不带 P3）。33 个源含 hostnative.cx。osx/x86_64：Mach-O，sha=52dcc0252ee6d71324df36cc637d34774e4c0e0a4ef29633c9271cb7c95460af bytes=1552354，rc=0，落在 55 秒界内。win/x86_64：PE32+，sha=9ae0cf6d034ad1cea71ddde2eb6c790acfc8de298e67a25034daf9a381e86034 bytes=1589760，formats-macho-pe --pe 12/0，rc=0。arm64 未跑。
  WHOLE-O-TOOL：slice --target 已收 osx/x86_64 与 win/x86_64，route=whole-o。冻结 rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。工具写 OUT/receipt.txt。镜像不带 P3。子作业 tools/bound 55，外层不再套 bound。
  WHOLE-O-TOOL：osx/x86_64 sha=52dcc0252ee6d71324df36cc637d34774e4c0e0a4ef29633c9271cb7c95460af bytes=1552354 ns=36426580344 rc=0。与手跑整程序 -o 逐字节相同。
  WHOLE-O-TOOL：win/x86_64 sha=9ae0cf6d034ad1cea71ddde2eb6c790acfc8de298e67a25034daf9a381e86034 bytes=1589760 ns=22000334071 rc=0。与手跑整程序 -o 逐字节相同。
  ARM64-SLICE：lnx/arm64 走 33 个对象，osx/arm64 走 route=whole-o。冻结 rev=5ed7ba69bf9e86b91843c608ce00a1daff231dc8。工具写收据。外层不再套 bound。
  ARM64-SLICE：lnx/arm64 entry.elf sha=dabcfa2337de8036b47c528210c4d851128d8f0e011fe33815e49258d9c3bee0 bytes=1385808。candidate sha=8b138088558666154bec3ac59aa3ecbb5dad10f8b2f397f6aa55f24789e69170 bytes=3425658，已按 entry-build 拼上 P3（p3 sha=aec1a0bf27b9d1f2e26a1b86b52cb3694bc66a906415bcc0435c11278c6deabf bytes=2039850）。六只 -O1 ns=25754945902，27 只 ns=23086225727，link ns=8472766756，rc=0。
  ARM64-SLICE：osx/arm64 sha=2a6c571cf24d62494ca403049e8b233e772a7b41a9da925ea363d264a5c182ab bytes=1981666 ns=37489425891 rc=0。镜像不带 P3。
  WHOLE-O-LNX：cc 裁定 0.0.43 的 lnx/x86_64 与 lnx/arm64 也走 route=whole-o，rev=c98f757f4c93ea9cfe91bef18fd88a929ad7ba96。收据写 route=whole-o，镜像不带 P3。lnx/x86_64 sha=477393de82de27764b714502db8cc410e3f5daf213afa99cc278b3ca2b1ce49a bytes=1537472 ns=35346772463 rc=0。lnx/arm64 sha=75d2d4b7659e8cfa91acfbd6bf775762d79e7f56c896216a807a8813d975e1ba bytes=1959313 ns=36257376370 rc=0。pack：out/release-ape-c98/unisacc-next.com sha=43fbbb819aae028e8151a9f8eec621514a1264259d1b7a926d85f7dd952a5f00 bytes=4631130。smoke ok=4 failed=1（hostc.cx 写在 out/release-ape-smoke/，use 在该文件旁边找 sys/io.cx）。同一源放在 cx/ 下再跑，打印 release-ape-hostc，rc=0。未安装。
  INSTALL-C98：已装 ./unisacc.com sha=43fbbb819aae028e8151a9f8eec621514a1264259d1b7a926d85f7dd952a5f00 bytes=4631130 mode=755。备份 out/unisacc.com.79fb84e6 sha=79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32 bytes=3035300。check-install-fixes failed=0（dupmod、dedupe、iproto、uses48）。tests/run.cx --smoke assertions=16 failed=0。
- [ ] 3 object 模型步
- [ ] 4 测试到 QUALIFIED
- [ ] 主人授权发布
