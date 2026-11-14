# 记忆宫殿(唯一协同入口)

项目所有者 2026-11-14 裁定:agent 协同只用本文件。不再用 `agentwork/` 的任务表、账本、看板记过程;`agentwork/` 退役,现存文件只作工具读取与历史,不再追加。本文件由 prd.md §9 索引;每轮先读本文件,事实、阻塞、下一动作原位更新,旧进度删掉或改写,不追加流水。

## 写法规则

- 只写当前事实:谁在做什么、卡在哪、下一步命令。过程、重跑记录、单个批次 PASS 不写。
- 三类才写进来:别的 agent 必须遵守的决定、不可逆的对外动作、阻塞发布的未验证项。
- 每条事实带证据入口(命令或路径),过期就改,不留"曾经"。
- 速度数字只来自 tools/run-startup-bench.cx、coldcompile.cx、check-bench-gencode.cx、speed-probe.cx。

## 当前发布与版本

- 已公开:0.0.61(2026-11-14,Latest;tag v0.0.61 与 v0.0.61-src 均指冻结提交 d4b3bf72f7ae;APE sha256 c4c47ecb97ee70865edaf39d91e29f5b22b6c8efdbcfb81c4482517ffc0224c3,7022820 B)。裁决 NOT_QUALIFIED unknown=1(s9 未跑),CI 六目标原生作业全 success(run 43554326355),发布说明 plans/release-notes-0.0.61.md,撤回办法 release/rollback.txt。
- 已装根:本机根已是公开 0.0.61 发布件(sha256 同上)。real5 整链(同 REV b418cb5d86)打包产物 sha256 与之逐字节相同,两次独立整链可复现。
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
- 整链:real5(g2/g3 linux 对并发,G_LNX_PAIR=2)3538 s,real4 为 4759 s(-26%);只有这一组对照,未控制机器负载,不作结论。pack 37 s。
- 已入库待新产品验证:cx-namespace CX-U-2 三缺口具名拒绝(57c7427f6f,cx.plan 0.1.2、cx.compile 0.2.6),旧产品上标 UNVERIFIED,下一轮整链后复验。
- 已入库工具:tools/derive.cx 派生物总闸(六个生成器 --check 全 OK,22 s)、tools/status.cx(--fast 实测 0.97 s 热态)、affected --iface-narrow 按包判定(cx/text/text.cx 实测 903 -> 765 选中)。tools/status.cx 的测试夹具由子代理用过 sed -i,属违规已记于此,内容未重审。
- affected-tool、affected-list 批次 FAIL(tape-gap-literal、fixture-literal 两条),HEAD 版同样失败,根因未查;affected-iface2 批次未取得实跑结果。
- cx 模块语法缺口(prd_cx_spec.md 实测,2026-11-14):`use m::sym as alias` 能通过规划但运行报 `no host function slen`(rc 127),设计文档称已端到端实现,与实测不符;`#pragma unisa export` 未实现(`unknown unisa pragma export`);`use sys.*` 通配从未设计(`bad module name`);仓外脚本找不到包根,需脚本目录放仓根链接与自备 cxroot.tsv。0.0.62 若做 export/通配,属 cxmod 三源(cx/cx0/plan.cx、src/cxmod.c、cx/compiler/cxmod.cx)产品输入,先修别名。
- cx 模块身份缺口(2026-11-14 实测):同一文件经两条路径(软链 tools/cxlib/sys -> cx/sys)被 cxmod 当成两个模块,报 `sy_env exported by both env (tools/cxlib/sys/env.cx:11) and sys.env (cx/sys/env.cx:11)`,使 tools/osx-sentinel.cx 等经 cxlib.toolselfpath 与 cxlib.netpack 的工具编不过。修法:cxmod 解析模块文件时按 realpath 规范化身份(cxmod 三源产品输入,排在别名修复 T5 之后,同写域串行);冻结的 cx/sys 不改。
- wt-gate 更正(2026-11-14 复验):`wt-gate-clean` 批次实跑 PASS(marker=WT_GATE),下一条"5/8 红"的前提不成立(check 已自建原生 run-batch,直接无参运行会 REFUSE usage);`wt-gate-probe` 为 PLANNED(链路超 55 秒批次上限),其 6 条断言未验证。win 批次 `wt-gate-clean-win` 已登记 PLANNED/NOT_READY,无 win 证据。以下为旧记述,留作线索:
- (旧)既有红 wt-gate(5/8 断言失败,与退役和 ext.net 无关,干净工作树也红):tools/all-waves.cx 的 wave 在 `-run` 方式下要求自身可执行文件同目录有原生 `run-batch`(`executor-run-batch-required /run-batch`,tools/all-waves.cx:339),check-wt-gate 以 `sh ./unisacc.com tools/all-waves.cx` 起它,不满足。需改 check 的起法或 all-waves 的执行器发现,未做。slice-migration-gen 另有 source-missing cx/compiler/frames.cx(该文件 HEAD 未跟踪)。
- 仓库根 out/:五个工具已迁到 $TMPDIR;仍写 out/ 的:tools/gen-model-decode-material.cx(依赖 extract-p3-models 要求 ROOT/out)、tools/csih-ape.cx(csih 车道,须由其维护者迁)、tools/chain-run.cx(--out 须在 ROOT/out 之下)。

## 下一动作(0.0.62,fable 规划;取舍已定见第 5 条,可派任务树见本节末表)

主题:库包由内往外,网络栈封口(cx.sys netraw/netpoll -> ext.net 清零 new 行;表中 new 共 16 行已落 14 行后以 build/std-surface.tsv 为准)。cx.stream/tlib 重构排 0.0.63。

1. 主题线:L2 netraw 补六参 sendto/recvfrom 与 IPV6_V6ONLY(fable);L3 UDP/unix/resolve(sonnet);L4 带截止时间五个 TCP 行(fable);L5 cx.sys 三行加 np_exec;L6 osx/win 只交有名 ENOSYS 拒绝加 CI 证据。L1(netpoll/sock)已在 main 并复验 PASS。
2. 速度线:冷启动 first_e2e 中位 704.9 -> 670 ms 以下;冷编译 entry-build 13624 -> 12300 ms 以下、whole-front 12846 -> 11600 ms 以下;生成代码先重测 1MB sort 再定目标(120 ms 以下为建议值);run-startup-bench、coldcompile、check-bench-gencode、speed-probe 四个工具对 0.0.61 发布件补跑入 speed-baseline。
3. 发布线(冻结到公开 100 分钟内)。real5(g2/g3 并发,rev b418cb5d)整链实测 3538 s(/tmp/unisacc-session-build-lr0060/log/real5.start 与 real5.end 之差,不含打包续段;与 real4 的对照见阻塞节整链一条,单组对照不作结论);首段以 rc=2 结束,原因是 `REFUSE pack needs --cache-manager-osx`,续段 real5p rc=0。real5 各相分解未入 build/phase-times.tsv(任务 A3)。real4 整链实测分解(RECEIPT 墙钟求和,工具 tools/zz 式 .cx 分析,日志 /tmp/unisacc-session-build-lr0060/log/real4.out):g2/g3 linux 对象相四段约 2480 s(约占 51%,最大头);jobs-gen/tbl/net 三族 801 s(gen 416、tbl 233、net 152)加 jobs-tool 53、final-net 85、p3-guard 181。结论:g2/g3 并发整链已实测(8d70af5101,结果见本条开头),下一步查对象相单批约 320 s 的构成;jobs 三族改按模型流水线(C1 第二段方案,build/chain-phase-outputs.tsv)至多省约 380 s,排其后。R2 拆 g2 对象相(限期 2026-11-30);R3 起链一条命令(根工作树与 REV 不同提交即拒);R4 osx cache-manager 自产(借用副本在 ~/.cache/unisacc/borrowed-cm/);R5 CI 前后零手工;R6 s9 三档首次跑通。
4. 基础设施(不入产品输入):I1 wt-gate 修红;I2 slice-migration-gen 定因;I3 agentwork 归档第 4 批起(第 3 批已退役 archive-records、aw、check-aw、cursor-fleet、dag 及各自检查;awpath 留第 4 批;分类表见提交 b10474a8f0 说明);I4 out/ 写入点收尾(gen-model-decode-material、chain-run --out、csih-ape 须由其维护者迁);I5 装发布件后复验 FTRIM 类红与 DEFERRED 批。
5. 已决定(2026-11-14,主调度按推荐定,项目所有者要求能做尽量做):(a) SPEED-11 运行库扫描族进 0.0.62;(b) 发布门不等 osx/win 的 ext.net 实现,只交有名 ENOSYS 拒绝加 CI 证据;(c) s9 只让 commit 档卡发布,daily/release 档夜跑并如实写进说明;(d) 0.0.62 主线为"构造链与批次分治"(C1 相清单化、C3 批次纯函数化、C4 接口哈希表),封网(ext.net/netraw)与 cx 模块语法(别名运行失败修复、export、通配)作并行线,产品输入变更合并为同一类(库包与运行库),构造链/批次工具不入产品输入。

可派任务树(2026-11-14;每项 15 分钟内可交,写域互不相交;tests/batches.tsv 登记行一律走 `tools/commit-own.cx --keyed`;新建工具先 `git log --all --oneline -- tools/NAME`;预期值是验收门,未实跑不记 PASS)。开工条件:今=已装产品即可;候选=补丁今天可写,验收要含它的新产品;链=要整链窗口(同一时刻只一条)。

| 号 | 目标 | 写域 | 验收命令 -> 预期值 | 分钟 | 开工 |
|---|---|---|---|---|---|
| A1a | 冷启动基线补到 0.0.61 发布件 | build/speed-baseline.tsv 的 startup 行 | `sh ./unisacc.com tools/run-startup-bench.cx` 三次独立调用 -> 9/9 断言三次均过,新增 1 行 n=3 | 10 | 今(机器安静) |
| A1b | 冷编译基线由单样本补到 n>=3 | 同表 coldcompile 行 | `tools/coldcompile.cx -- OUT.tsv SCRATCH tools/ledger-add.cx tools/entry-build.cx tools/whole-front.cx` 跑三轮 -> 三文件各 1 行 n=3,带中位与最大 | 12 | 今 |
| A1c | 生成代码与探针基线 | 同表 gencode、probe 行 | `tools/check-bench-gencode.cx -- ROOT 5 1` -> 三档 RESULT 逐字节相同,无 INVALID;`tools/speed-probe.cx` 出数入表 | 12 | 今 |
| A2 | 已装 0.0.61 发布件上复验红批(I5);另查 affected-tool、affected-list 两条字面量失败的根因 | 阻塞节前两条与 affected 一条 | `tools/affected.cx --files tools/tsv-lint.cx --run` -> `_u_so_put` 不再出现;FTRIM 类按 4 片并行复验,每片一条命令,仍红的逐批留名 | 15 | 今 |
| A3 | real5 各相墙钟入表,定下一个最大头 | build/phase-times.tsv | 对 real5.out 的 RECEIPT wall_ms 用 .cx 求和 -> 整链行 measured_s=3538,对象相、jobs 三族、p3-guard 各有实测行 | 10 | 今 |
| A4 | 起链前置门(R3) | tools/candidate-chain.cx 的参数检查段及其 check | 根工作树提交 != REV -> REFUSE 且 rc 非 0;未设 UNISACC_IDENT_HELPER -> REFUSE;缺 --cache-manager-osx 在起链第 1 秒拒绝而不是跑完 3538 s 才拒 | 15 | 今 |
| A5 | osx cache-manager 链内自产(R4)先定因 | 只读,产出写回本表本行 | 说明借用副本的构造命令、输入身份与缺的链相;给出改 build/chain-phases.tsv 的补丁草案 | 15 | 今 |
| A6 | cxmod 三源一致性门 | tools/check-cxmod-tri.cx(新)加批次行 | failed=0;常量分叉(CXM_MAXU 128 对 96)与 `cxm_checkcalls` 只在产品源两条输出具名 GAP,不记 PASS | 15 | 今 |
| A7a | 符号级选批第一步:改动行归属到文件作用域条目 | tools/affected.cx 的归属段 | 夹具只改 `tx_trimc` 函数体 -> 输出归属 1 个导出函数;改宏或类型 -> 退回整模块 | 15 | 今 |
| A7b | 符号级选批第二步:按归属选批 | tools/check-affected 的符号段 | `tools/affected.cx -- --files cx/text/text.cx --list-only --iface-narrow` 在 A7a 夹具下 selected 小于 765,打印 DEFERRED_BY_SYMBOL 计数;--full 与 release 档仍全选 | 15 | 今,A7a 后 |
| A8 | dock 审计表(M1) | build/dock-audit.tsv、tools/check-dock-audit.cx(新) | `sh ./unisacc.com tools/dock-map.cx` 模块数 == 表行数(现 56) | 12 | 今 |
| A9 | win 批次登记与 osx 行核对(T16) | tests/batches.tsv 的 win 行 | win 行数 0 -> 至少 1,本机状态 DEFERRED 不是 PASS;`tests/run.cx -- --list` 可见 | 10 | 今 |
| A10 | wt-gate 修红(I1) | tools/check-wt-gate.cx | 该 check 8/8 断言通过,不删断言,不改 all-waves 合同 | 15 | 今 |
| A11 | text.text 拆面只读实验(P4 先行) | prd_cx.md 10.4 的三行 | $TMPDIR 副本内拆面后 `tools/affected.cx` 给出改查找族的选批数,UNMEASURED 改成实测值 | 15 | 今 |
| A12 | 命中路定因 D0(0.0.63 前置) | prd_cx.md 第 5 节一行 | `tools/speed-probe.cx` 分相读数,指出首次与重复之差落在哪一相 | 15 | 今 |
| B1 | 封网 L2 netraw 六参与 V6ONLY(T6) | ext/net 的 netraw 文件 | `tools/check-ext-net-sock.cx` failed=0 | 15 | 今 |
| B2 | 封网 L3 UDP/unix/resolve、L4 带截止时间 TCP(T7) | ext/net 其余文件、build/std-surface.tsv 的 net 行 | `tools/check-ext-net-tcp.cx` failed=0;表内 new 行数 0 | 15 x 2 | 今,B1 后 |
| B3 | SPEED-11 运行库扫描族(T8) | 运行库对应源(产品输入)加其 check | 补丁先交集成者;字节等价回归输出不变 | 15 | 候选 |
| B4 | 0.0.62 候选整链 real6 | 私有构造根 | `tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12 --cache-manager-osx X64 ARM64` -> RELEASE_APE PACKED,墙钟不高于 3538 s,超 5400 s 即停 | 后台有界 | 链 |
| B5 | 需新产品的断言转真 | tests/cx-namespace.cx、tests/module/units.cx 的 UNVERIFIED 标注 | `sh ./unisacc.com tests/cx-namespace.cx -- ROOT ROOT/unisacc.com OUT` failed=0;`osx-sentinel -funit` 实编 rc=0 | 15 | B4 后 |

,2026-11-14;按"分而治之":工作流按耦合边界切,每版只含一类产品输入大改)

版本里程碑:
- 0.0.62(库包与运行库类):封网清零 new 行(netraw 六参、UDP/unix/resolve、带截止时间 TCP);SPEED-11 运行库扫描族;别名运行失败修复(cxmod 三源,按缺陷修复并入);分治工具线并行(C1 相清单化、C3 批次纯函数化、C4 接口哈希选批,不属产品输入);平台审计表扩面加有名 ENOSYS 加 CI 证据。
- 0.0.63(模型与网类):模型装载零解码(mmap 已解码布局,先定因 D0:最小 C 命中路 46.6 ms 的构成)、speed12b 类重建 e2 网;冷启动目标 first_e2e 500 ms 以下。
- 0.0.64(cx 设计类):export、通配、cx.stream/tlib 重构、紧凑前导。0.0.65 候选:osx/win netraw 真实现。
已定三项取舍(主调度按推荐定):(1) 0.0.63 唯一大改类为模型装载零解码,cx.stream/tlib 重构与 export/通配挪到 0.0.64;(2) speed12b 不并入 0.0.62;(3) 平台策略:事实表扩面加有名拒绝先行,不做 NT 上的 fork 仿真,osx/win netraw 真实现推到 0.0.65。
第一周任务状态(2026-11-14 实测,已落 main 的才算数):T5 别名运行修复已入库(4aee1dfb4a,三源,断言需新产品验证)、cxmod 模块身份规范路径判同已入库(e576000457,cxm_bind_real;cxm_deps_same 三源对齐在做)、T9 libc 声明清点(build/libc-decls.tsv 1748 条,in_libc99posix=no 从 97.7% 降到约 85%)与 T11 ENOSYS 点清点(口径修正后 70 个点,无拒绝原因 41 个,全在冻结的 cx/sys,须走事实表集中拒绝原因,不逐点补注释)已入库、T10 libc99posix.tsv 30 -> 129 行(stdio 41、string 20、stdlib 38,全 cx-own)已入库、ext 下 34 个 ENOSYS 点补了原因(b0a44e212d)、C1 链相度量(phase-graph)与 C4 接口哈希选批(--iface-narrow)与变异测试补断言已入库。待做:T1 速度基线补跑(整链结束后安静机器)、T2 命中路定因、T3 g2/g3 并发整链已实测(real5,3538 s,见下一动作第 3 条)、T6/T7 封网、T8 SPEED-11、T13 git 查询去重、T14 text 拆面、T15 包索引、T16 win 批次登记。
工具层与反应器(fable 推演,2026-11-14,已定顺序):(1) tools/derive.cx 加 exec/facts/derive.tsv(事实表生成器 -> layer-api -> iface-hash -> batch-packages-gen -> tools-compile --register -> product-map -> tool-index 的依赖序,--check 只读汇总,--write 失败还原),已派;(2) tools/status.cx --fast(状态摘要,10 秒内,主调度每轮只读它),已派;(3) 之后:tools/ask.cx 用量闸(build/ask-budget.tsv,防递归环境标记、去重、断路器、16KB 上限、审计行,只接桩,不接真实后端)与 tools/triage.cx(先 exec/facts/red-fingerprints.tsv 确定性命中,未命中才升级,模型输出只作 SUGGESTION);黑名单:产品输入提交、判决规则、授权与预算 raise、断路器复位、任何写仓内文件的动作不得调用模型。(4) 运行缓存:缓存根与 TMPDIR 解耦(UNISACC_CACHE_DIR)与路径无关键、头文件私有目录(产品输入,0.0.63);热进程价值低(命中约 110 ms)暂不建。(5) 变异测试工具化 tools/mutate.cx(表驱动,先做 affected/precommit/sessionroot/check-archive-refs 四点)。
平台现状事实(2026-11-14 实测):tests/batches.tsv 平台列 lnx/x86_64 1046 行、osx/arm64 36 行、win 0 行;exec/facts/libc99posix.tsv 只有 30 行而 include/ 约 45 个头;cosmocc 参考:errno 等常量走链接期符号、运行库可按平台裁剪(-msysv、-mtinylinux)、跨平台产物字节确定(APE 格式细节与 NT 层未取到原文,UNVERIFIED)。

预见的瓶颈与风险(2026-11-14 前瞻,均未发生;每条给证据入口与提前动作):

| 号 | 很可能出现的问题 | 证据入口 | 提前动作 |
|---|---|---|---|
| F1 | 返工即破 2 小时:整链 3538 s,0.0.62 候选并入三条产品输入线(封网、SPEED-11、cxmod 三源),任一条红就重跑整链,两次即超 | real5.start/end;分治时限铁律 | A4 前置门、A6 三源门、`tools/chain-precheck.cx` 进冻结前必跑;候选冻结前三线各自探针绿 |
| F2 | 打包单点:整链跑完才因缺 osx cache-manager 被拒,且借用副本只在 ~/.cache 一处,丢了就发不了版 | real5.rc=2、real5.out 末行 REFUSE;起整链配方第 4 步 | A4 把拒绝前移到第 1 秒;A5 定因后自产(R4) |
| F3 | 三源漂移放大:手工对齐已漏过一轮,0.0.64 的 export 与通配每条新拒绝文案都要三处同现 | 4367e1b0e5;prd_cx.md 10.3、11.6(`cxm_checkcalls` 只在产品源)、E13 | A6 现在就建门,0.0.64 开工前必须为绿或具名 GAP |
| F4 | 装新产品时红批集中爆发:别名、模块身份、CX-U-2 三拒绝、约 80 个 FTRIM 批、63 个 DEFERRED 批都压在"需新产品" | 阻塞节前两条;prd_cx.md 第 10 节开头 | A2 今天在已装的 0.0.61 发布件上先消一批;B5 清单化,B4 后一次并行跑 |
| F5 | osx/win 无基线:win 批次 0 行、osx 36 行,0.0.65 做 netraw 真实现时没有可比的"之前" | 平台现状事实行;prd_cx_spec.md 末节 UNVERIFIED 汇总 | A9 先登记批次(DEFERRED);CI 平台作业结果回收成表,ENOSYS 拒绝按表断言 |
| F6 | 缓存根耦合 TMPDIR:并发子代理各用全新 TMPDIR 即全冷,源身份首测 5.8 s 在并行下越 8 s 硬限,判定依赖机器安静;解耦排在 0.0.63,而 0.0.62 并发更多 | 起整链配方第 5 步;工具层与反应器第 (4) 项 | A4 强制 UNISACC_IDENT_HELPER;子代理简报统一共享只读缓存根;UNISACC_CACHE_DIR 不早于 0.0.63 候选 |
| F7 | 波及面拖慢快车道:改 cx/text/text.cx 仍选 765 批(窄化前 903/1069),SPEED-11 若动扫描族则每次改动近全量 | 提交 1420444e98 实测;prd_cx.md E6、E7 | A7a、A7b 符号级选批先于 B3 落地 |
| F8 | 速度门假红假绿:冷编译三项目标都以单样本为基线 | build/speed-baseline.tsv 末三行(n=1) | A1b 补到 n>=3 后再把 12300、11600 当门 |
| F9 | 0.0.64 一版两候选(cxmod 三源与运行库头不同类)即两次整链,约 2 小时只花在构造上 | prd_cx.md 10.3"一版一类" | 门:整链降到 2400 s 以内才允许同版两候选,否则 P4 物理拆面顺延 0.0.65 |
| F10 | 通配让闭包依赖目录列表,新增一个文件就改变全部通配使用方的计划字节 | prd_cx.md 10.2 P2"代价如实" | 产品层禁用规则先进 cxapis 类棘轮,早于 P2 实现 |

0.0.63 与 0.0.64 任务(都依赖 0.0.62 新产品已装;开工前逐项拆到 15 分钟):
- 0.0.63:C1 零解码布局设计稿(prd_cx.md 第 5 节,今天可写,依据 A12);C2 UNISACC_CACHE_DIR 与路径无关键(产品输入,验收:两个不同 TMPDIR 下第二次运行命中缓存);C3 mmap 已解码布局实现(验收 `tools/run-startup-bench.cx` first_e2e 500 ms 以下,n=3);C4 整链对象相拆分到整链 2400 s 以内(F9 的门)。
- 0.0.64:D1 export(三源,验收 `tests/cx-namespace.cx` 门面正例 2、拒绝 4,`tools/cx0-extract.cx -- --check` EXACT)-> D2 通配(正例 1、拒绝 5)-> D3 符号级导入收敛(`tests/module/check.cx`)-> D4 text.text 物理拆面(第二候选,受 F9 门约束)-> D5 层感知前导(末位,超 2 小时移 0.0.65)。同写域串行,不并发。

三个可测速度杠杆(数字只取自 build/speed-baseline.tsv 中四个工具的行):
- (1) 冷启动:run-startup-bench first_e2e 三次 684.051/704.882/714.267 ms(real4 c55533c7,n=3),同工具 repeat_median3 约 27 至 29 ms(0.0.59 与 local-0060 组);首次与重复相差约 650 ms,是零解码的可得上限。度量:同工具 first_e2e,目标 0.0.62 为 670 以下、0.0.63 为 500 以下。
- (2) 冷编译:coldcompile 的 entry-build 13624、whole-front 12846、ledger-add 6468 ms(real4,各单样本),对 0.0.59 为 -27%、-24%、-32%。杠杆是每单元前导与前端结果复用;度量:同三文件 n>=3 中位。
- (3) 生成代码与分相:check-bench-gencode 与 speed-probe 对 0.0.61 产物 UNMEASURED,先由 A1c 取数再定目标,不预设数字。

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
