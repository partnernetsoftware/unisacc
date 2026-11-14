# 记忆宫殿(唯一协同入口)

项目所有者 2026-11-14 裁定:agent 协同只用本文件。不再用 `agentwork/` 的任务表、账本、看板记过程;`agentwork/` 退役,现存文件只作工具读取与历史,不再追加。本文件由 prd.md §9 索引;每轮先读本文件,事实、阻塞、下一动作原位更新,旧进度删掉或改写,不追加流水。

## 写法规则

- 只写当前事实:谁在做什么、卡在哪、下一步命令。过程、重跑记录、单个批次 PASS 不写。
- 三类才写进来:别的 agent 必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 每条事实带证据入口(命令或路径),过期就改,不留"曾经"。
- 速度数字只来自 tools/run-startup-bench.cx、coldcompile.cx、check-bench-gencode.cx、speed-probe.cx。

## 当前发布与版本

- 已公开:0.0.61(2026-11-14,Latest;tag v0.0.61 与 v0.0.61-src 均指冻结提交 d4b3bf72f7ae;APE sha256 c4c47ecb97ee70865edaf39d91e29f5b22b6c8efdbcfb81c4482517ffc0224c3,7022820 B)。裁决 NOT_QUALIFIED unknown=1(s9 未跑),CI 六目标原生作业全 success(run 43554326355),发布说明 plans/release-notes-0.0.61.md,撤回办法 release/rollback.txt。
- 已装根:本机根是 real4 中间产物 c55533c7(版本串 0.0.59,非发布件);要装发布件:下载公开 unisacc.com 后 `tools/install-ape.cx -- ROOT APE SHA256 --write`。备份 ROOT/unisacc.com.059(未入库)。
- 实测(0.0.61 发布件 vs 0.0.59 基线):first_e2e 675/676/695 ms 对 796/815;冷编译 -24% 到 -32%;数字与样本数见 build/speed-baseline.tsv 与发布说明。整链 4829 s。
- 公开后大扫除已做:私有构造根只留 log/cxroot.tsv,chains/snap、local-0060、37 个无可执行位的 tlib kit 已清;借用的 osx cache-manager 副本在 ~/.cache/unisacc/borrowed-cm/。

## 起整链的做法(已验证的配方)

1. 私有根的工作树必须检出被构造的 REV(否则驱动工具与 REV 内工具拼法不一致,wf-prep-g2 收据 root 失配)。
2. BOOTSTRAP_FROM 要求祖先 2a46e1a4,而它不是 main 的祖先:用 `git commit-tree <main 树> -p fbdc99ca -p <main HEAD>` 造私有提交,先 `git fetch` main 到私有根。
3. `sh ./unisacc.com tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12`(输出目录必须全新,不能续跑;超时给 5400 s)。
4. 打包:同命令加 `--resume --cache-manager-osx X64 ARM64`(借用 0.0.59 的 osx cache-manager,副本在 ~/.cache/unisacc/borrowed-cm/)。5. 私有根配套文件按 rev 生成,不留旧版:根目录 unisacc.com 用 bootstrap 0.0.59(副本 ~/.cache/unisacc/bootstrap-0.0.59/unisacc.com,sha e43ef368)并配对应的 unisacc.com.build.json(commit 2a46e1a4、product_digest e43ef368);会话根的 cxroot.tsv 必须指向本 rev 的快照(chains/snap/REV10-H8);起链设 UNISACC_IDENT_HELPER 指向链内预构建的 identity-helper,否则首次源身份测量要现场编译 seed/ident.c,冷缓存下 5.8 s、并行下超 8 s 硬限会失败(判定依赖机器安静,是缺陷)。

## 阻塞与未验证(发布前清零或如实写进发布说明)

- 已装 0.0.59 落后于工作区:tsv-lint 在 affected 下报 `_u_so_put` 未定义(FTRIM-bootstrap 类),装新产品后应转绿,未验证。
- 约 80 个批次此前红,大半属 FTRIM-bootstrap(装新产品后复验);27 + 36 个批次因窗口 DEFERRED 未跑;osx/win 平台批次需对应主机,无证据。
- check-stdio-buffer 不覆盖 --link-only 与子集编译的缓存 identity 一致性;tty/pty 路径未测;六目标未跑。
- build/speed-baseline.tsv 未更新;run-startup-bench、check-bench-gencode、speed-probe 对新产物未跑。
- 整链 77 分钟(4625 s);g2/g3 的 objects 相按目标串行约 2480 s。fable 正在改相并发;预期节省未实跑。
- cx 模块语法缺口(prd_cx_spec.md 实测,2026-11-14):`use m::sym as alias` 能通过规划但运行报 `no host function slen`(rc 127),设计文档称已端到端实现,与实测不符;`#pragma unisa export` 未实现(`unknown unisa pragma export`);`use sys.*` 通配从未设计(`bad module name`);仓外脚本找不到包根,需脚本目录放仓根链接与自备 cxroot.tsv。0.0.62 若做 export/通配,属 cxmod 三源(cx/cx0/plan.cx、src/cxmod.c、cx/compiler/cxmod.cx)产品输入,先修别名。
- 既有红 wt-gate(5/8 断言失败,与退役和 ext.net 无关,干净工作树也红):tools/all-waves.cx 的 wave 在 `-run` 方式下要求自身可执行文件同目录有原生 `run-batch`(`executor-run-batch-required /run-batch`,tools/all-waves.cx:339),check-wt-gate 以 `sh ./unisacc.com tools/all-waves.cx` 起它,不满足。需改 check 的起法或 all-waves 的执行器发现,未做。slice-migration-gen 另有 source-missing cx/compiler/frames.cx(该文件 HEAD 未跟踪)。
- 仓库根 out/:五个工具已迁到 $TMPDIR;仍写 out/ 的:tools/gen-model-decode-material.cx(依赖 extract-p3-models 要求 ROOT/out)、tools/csih-ape.cx(csih 车道,须由其维护者迁)、tools/chain-run.cx(--out 须在 ROOT/out 之下)。

## 下一动作(0.0.62,fable 规划,主题待项目所有者对三个取舍拍板)

主题:库包由内往外,网络栈封口(cx.sys netraw/netpoll -> ext.net 清零 new 行;表中 new 共 16 行已落 14 行后以 build/std-surface.tsv 为准)。cx.stream/tlib 重构排 0.0.63。

1. 主题线:L2 netraw 补六参 sendto/recvfrom 与 IPV6_V6ONLY(fable);L3 UDP/unix/resolve(sonnet);L4 带截止时间五个 TCP 行(fable);L5 cx.sys 三行加 np_exec;L6 osx/win 只交有名 ENOSYS 拒绝加 CI 证据。L1(netpoll/sock)已在 main 并复验 PASS。
2. 速度线:冷启动 first_e2e 中位 704.9 -> 670 ms 以下;冷编译 entry-build 13624 -> 12300 ms 以下、whole-front 12846 -> 11600 ms 以下;生成代码先重测 1MB sort 再定目标(120 ms 以下为建议值);run-startup-bench、coldcompile、check-bench-gencode、speed-probe 四个工具对 0.0.61 发布件补跑入 speed-baseline。
3. 发布线(冻结到公开 100 分钟内)。real4 整链实测分解(RECEIPT 墙钟求和,工具 tools/zz 式 .cx 分析,日志 /tmp/unisacc-session-build-lr0060/log/real4.out):g2/g3 linux 对象相四段约 2480 s(约占 51%,最大头);jobs-gen/tbl/net 三族 801 s(gen 416、tbl 233、net 152)加 jobs-tool 53、final-net 85、p3-guard 181。结论:先实测 g2/g3 并发整链(8d70af5101,预期省约 1257 s),再查对象相单批约 320 s 的构成;jobs 三族改按模型流水线(C1 第二段方案,build/chain-phase-outputs.tsv)至多省约 380 s,排其后。R2 拆 g2 对象相(限期 2026-11-30);R3 起链一条命令(根工作树与 REV 不同提交即拒);R4 osx cache-manager 自产(借用副本在 ~/.cache/unisacc/borrowed-cm/);R5 CI 前后零手工;R6 s9 三档首次跑通。
4. 基础设施(不入产品输入):I1 wt-gate 修红;I2 slice-migration-gen 定因;I3 agentwork 归档第 4 批起(第 3 批已退役 archive-records、aw、check-aw、cursor-fleet、dag 及各自检查;awpath 留第 4 批;分类表见提交 b10474a8f0 说明);I4 out/ 写入点收尾(gen-model-decode-material、chain-run --out、csih-ape 须由其维护者迁);I5 装发布件后复验 FTRIM 类红与 DEFERRED 批。
5. 已决定(2026-11-14,主调度按推荐定,项目所有者要求能做尽量做):(a) SPEED-11 运行库扫描族进 0.0.62;(b) 发布门不等 osx/win 的 ext.net 实现,只交有名 ENOSYS 拒绝加 CI 证据;(c) s9 只让 commit 档卡发布,daily/release 档夜跑并如实写进说明;(d) 0.0.62 主线为"构造链与批次分治"(C1 相清单化、C3 批次纯函数化、C4 接口哈希表),封网(ext.net/netraw)与 cx 模块语法(别名运行失败修复、export、通配)作并行线,产品输入变更合并为同一类(库包与运行库),构造链/批次工具不入产品输入。

## 进化路线(fable 编排,2026-11-14;按"分而治之":工作流按耦合边界切,每版只含一类产品输入大改)

版本里程碑:
- 0.0.62(库包与运行库类):封网清零 new 行(netraw 六参、UDP/unix/resolve、带截止时间 TCP);SPEED-11 运行库扫描族;别名运行失败修复(cxmod 三源,按缺陷修复并入);分治工具线并行(C1 相清单化、C3 批次纯函数化、C4 接口哈希选批,不属产品输入);平台审计表扩面加有名 ENOSYS 加 CI 证据。
- 0.0.63(模型与网类):模型装载零解码(mmap 已解码布局,先定因 D0:最小 C 命中路 46.6 ms 的构成)、speed12b 类重建 e2 网;冷启动目标 first_e2e 500 ms 以下。
- 0.0.64(cx 设计类):export、通配、cx.stream/tlib 重构、紧凑前导。0.0.65 候选:osx/win netraw 真实现。
已定三项取舍(主调度按推荐定):(1) 0.0.63 唯一大改类为模型装载零解码,cx.stream/tlib 重构与 export/通配挪到 0.0.64;(2) speed12b 不并入 0.0.62;(3) 平台策略:事实表扩面加有名拒绝先行,不做 NT 上的 fork 仿真,osx/win netraw 真实现推到 0.0.65。
第一周任务(写域互不相交,P2/P3 同表串行):T1 四工具对 0.0.61 发布件补跑入 speed-baseline(含 1MB sort 118.8 ms 的 check-bench-gencode 行)、T2 命中路定因 D0、T3 g2/g3 并发整链实测(real5 在跑)、T5 别名运行失败修复、T6 netraw 六参、T7 UDP/unix/resolve、T8 SPEED-11、T9 libc 声明清点工具、T10 libc99posix 事实表扩面、T11 ENOSYS 清点表、T13 git 查询去重、T14 text 拆面方案、T15 包索引、T16 win 批次登记。
平台现状事实(2026-11-14 实测):tests/batches.tsv 平台列 lnx/x86_64 1046 行、osx/arm64 36 行、win 0 行;exec/facts/libc99posix.tsv 只有 30 行而 include/ 约 45 个头;cosmocc 参考:errno 等常量走链接期符号、运行库可按平台裁剪(-msysv、-mtinylinux)、跨平台产物字节确定(APE 格式细节与 NT 层未取到原文,UNVERIFIED)。

## 记忆宫殿(索引)

| 房间 | 当前事实 | 入口 |
|---|---|---|
| 当前协同 | 本文件;agentwork/ 退役 | 本文件 |
| 当前发布 | 已公开 0.0.61(Latest);0.0.62 规划中 | 本文件上节、plans/release-notes-0.0.61.md、prd_release.md |
| 结构总表 | 五层、六片、7 + 1 流、网、执行器、格式、事实表、构造链、测试 | prd.md §3;cx 侧 prd_cx.md |
| 新入口 | src/unisacc.cx 六层;src/core、src/runtime 指向 cx/ | plans/cx-modules.md、prd.md §5 |
| 测试入口 | `sh ./unisacc.com tests/run.cx -- --smoke/--all/--list` | prd.md §7、tests/batches.tsv、tests/coverage.tsv |
| 分层 | 术语层 T、结构层 L0-L4、模块层 cx-L0..3 | prd.md §0、plans/cx-layers.md |
| 棘轮 | 13 项,以 tools/ratchets.cx 末行为准 | tools/ratchets.cx、prd.md §2.2 |
| 事实表 | 事实 -> 生成器(带 --check)-> 产物 | exec/facts/、prd.md §2.4 |
| 构造图与相位 | build/graph.tsv、build/chain-phases.tsv | prd.md §3.7、§2.2 |
| .cx lint 与静态预检 | tools/cx-lint.cx、tools/tsv-lint.cx、tools/chain-precheck.cx | prd.md §2.2 |
| 产品全景表 | 生成块,tools/product-map.cx -- --write 重算 | [prd_productmap.md](prd_productmap.md)(原 prd.md §9.6) |
| 往期发布与历史 | 各版发布说明与历史索引 | prd_release.md、prd_hist.md、plans/release-notes-*.md |
| 归档 | 旧 C/Python 实现、旧 1333 项套件、历史计划与 PRD | prd.md §10、archive/ |
