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

- check-candidate-chain 插入 `has(cc,...)` 行即 rc=139:已定因(2026-11-14),**不是编译器缺陷,是检查工具自身的越界读**,原「疑似编译器 lowering 缺陷」判断撤销。根因:tools/check-candidate-chain.cx 第 35 至 44 行 `find_sub` 写 `hl = (size_t)tx_len(h, 100000)`;`tx_len` 在 cap 内无 NUL 时按合同返回 -1(cx/text/text.cx 第 8 至 14 行),转 size_t 成最大值,循环 `i + nl <= hl` 失去上界;`cc` 是 tools/candidate-chain.cx 全文(175009 字节,超过 100000),针不存在时一直扫出缓冲区直到未映射页。证据:strace 落盘二进制,主进程读完 candidate-chain.cx 后立即 `SIGSEGV SEGV_MAPERR si_addr=页对齐地址`,之前无其它系统调用;变体实测 `has(cc,"QQ")`、`find_sub(cc,"QQ")` 崩,`has(pt,"QQ")`(pt 30037 字节,tx_len 正常)、`has(cc,"a")`(命中即返回)、`has("a","QQ")`、`ck(1,"QQ")`、`tx_eqz(cc,"QQ")` 均不崩,与调用点数、帧大小、字面量个数无关;`has(cc,"QR")` 并行跑一次 rc=0、串行两次 rc=139(越界后是否先撞到相同字节取决于堆布局),所以原记录的「独立小程序不崩」是因为其草堆不超过 100000 字节。「崩在新行之前」是输出缓冲未刷,不是执行位置。最小复现(19 行,实测 rc=139):同样的 find_sub,main 里 malloc 150001 字节填 'a' 加 NUL,打印 `tx_len(b,100000)`(得 -1)后调 `find_sub(b,"QQ")`。现有 969 至 976 行对 cc 的正向断言靠无界扫描碰巧命中 100000 字节之后的内容,结果目前正确但不受合同保护;任何对 cc 的否定断言(针不存在)必崩。建议修法(未做,属 check-candidate-chain 写域):find_sub 改为带长度的接口,调用处传 cx_read 返回的长度;或 tx_len 返回负值时直接返回 0 并让断言 FAIL,不得把 -1 当长度。同型写法 `tx_len(h, 100000)` 另见 6 个文件:tools/check-release-decide.cx、check-precommit-freeze.cx、check-enosys-sites.cx、check-precommit-coverage.cx、check-fixedpoint-sample.cx、check-libc-decl-scan.cx(只按该字面写法搜,其它写法未扫;各自草堆是否超 100000 未逐个核)。验收:修后在 974 行前插入 `ck(cc && !has(cc, "QQ"), "zz");` 运行 rc=0 且该行 PASS,原 78 行输出不变;七个文件同改后各自批次 failed=0。未入回归批次:缺陷在检查工具不在产品,tx_len 的 -1 合同本身无误。仓外复现须带 cxroot.tsv(CX-U-7 包根表)才能解析模块,原记录「须置于 tools/ 下」即此因。

- tsv-lint 状态不稳(2026-11-14 同日三次实跑):A2 与我复验 PASS,B5 分片跑中报 rc=1 FAIL,我随后单独复跑又 PASS;疑与并发写表(他人暂存的 build/*.tsv)有关,未定因,下次红时先留当时 git status 与失败行再判。
- 已复验转绿(2026-11-14,已装 0.0.61):tsv-lint 在 `affected.cx --ids tsv-lint --run -j 1` 下 PASS rc=0 22 s,`_u_so_put` 不再出现。affected-tool、affected-list 两条字面量失败已修:根因是 check-affected.cx 的 own() 只排除 5 个自有批次,漏了后加的 affected-iface、affected-iface2,它们被计入 others()(tape-gap 实得 14 对期望 12,strcmp-truth 实得 3 对期望 1),与负载无关;own() 补两项,两批各 PASS rc=0,断言未动。
- 红批复验(2026-11-14 实测,lnx IMPLEMENTED 共 1040 批,无现成红批清单,故按全集 4 片并行 `affected.cx --ids ... --run -j 1`):本轮实跑 15 批,PASS 14(precommit-toolsloc 三个、cx-source-frame、driver-token-target、delta-loader、modelcache-budget-concurrent、snapshots-cx、cx-stream-output、cx-packagefooter、check-edit-span、cx-driver-route、cx-unit-graph、pp-cli-targets、smoke),FAIL 1(tsv-lint rc=1 marker=CHECK_TSV_LINT),NOT_RUN 1025。NOT_RUN 原因:affected.cx 单次调用有约 56 秒的全局启动截止,每片只跑完约 4 批,其余 DEFERRED start-deadline,故一次 4 片仅得约 14 批;全集复验改走 all-waves 会话路径(2026-11-14 实测一小波,16 核,WAVE_JOBS_MAX=16):(1) 会话目录必须是 `/tmp/unisacc-affected-NAME/s`(run-batch 只认 ROOT/out 或 tmpdir/unisacc-affected- 下的输出父目录;`/tmp/unisacc-session-*` 会使 16 批全 runner-refused,rc=2);(2) `tools/bound 55 sh ./unisacc.com tools/all-waves.cx -- init $PWD S $PWD/unisacc.com -`(约 16 秒;1076 个 IMPLEMENTED 含 313 heavy、763 light-or-heavy);(3) `... -- executor $PWD S`(约 40 秒,编原生 all-waves/run-batch 到 S/executor,每会话一次);(4) `tools/bound 55 S/executor/all-waves wave $PWD S --light ID x16`(必须用原生执行器,`sh ./unisacc.com` 跑 wave 会 REFUSE executor-run-batch-required)。实测:16 个无材料 light 批一波 wall 约 20.4 秒(批在 20 秒内全跑完,吞吐约 47 批/分钟);但两次实测 ALL_WAVE 均为 identity=CHANGED(worktree-changed-during-wave),16 批全记 UNKNOWN、PASS/FAIL=0,原因是共享工作树在波内有他人改动(wt gate 前后两次扫描不稳),且一次 CHANGED 后该会话拒绝后续波(session-identity-changed-during-earlier-wave),须重 init+executor。故本轮没有任何有效 PASS/FAIL 判定。推算(非实测):全集 763 light 约 48 波、313 heavy 每波 1 批约 313 波(heavy 单批至多 55 秒,约 5 小时),light 部分单会话约 20 分钟;可行做法是在无人改工作树的窗口(或用 `git archive` 解出的快照作 ROOT,见 tests/check-wt-gate.cx 的 clone 做法)里跑,heavy 批另需 materials 绑定(s8c-bind)。未解决:共享检出上 wt gate 不稳,需冻结窗口或快照根,否则全集复验无法得出判定。仍红只确认 tsv-lint 一个;「约 80 个 FTRIM 红批」旧估算本轮既未证实也未证伪(清单仍缺,见 B5);27 + 36 个批次因窗口 DEFERRED 未跑;osx/win 平台批次需对应主机,无证据。
  - 快照根分波复验累计(实测,snap-root 会话,不含上文共享检出的 15 批):已跑 45 批,PASS 43,FAIL 2。FAIL:(1) ri-image-pem(REFUSE duplicate-key-row,他人在修);(2) phase-timing-stamps rc=1 marker=PHASE_TIMING_STAMPS,断言 62 失败 2:obj.miss/obj.hit.stamp-grammar-one-artifact(hit=0/1 mode=2),按确定性缺陷对待。定因(2026-11-14 读源加实测,已装 0.0.61):`artifact_hit` 戳自 ab090348ad(10-31)起只是标记,不是复用:cx/runtime/runexec.cx:1014-1036 在 E3 缓存目录写一个 `art` 文件,存在即报 hit=1,不跳过任何阶段;真正的结果复用查找只对 mode 3 与 0 开(src/drive.cx:545 `mode2 == 3 || mode2 == 0`),-c 从不查。戳消失的直接原因是 runexec.cx:763 `ex_obj_mode = mode == 2` 在现行 -c 路线上为假(cx/driver/route.cx:340 对象置 mode 5;究竟是 mode 5 还是 st_gap 未被走到,未逐行证实)。实测同一 probe.cx 连续三次 `-c -o`(UNISACC_RR_TIME=1、RUN_CACHE=1、独立 E3CACHE):每次 inference 7 行(e2、e1、e3、tapecheck、prune、lower、elf)、model 14 行、artifact_hit 0 行,端到端 338/288/280 ms,对象 sha 三次相同,缓存目录只有 7 个 mc- 模型项、无 art。结论:-c 最终产物复用在 cx 产品里不存在,丢的只是标记戳,没有因此新增的耗时回归;重复 -c 每次全跑约 280 ms 是既有状态。哪次提交让戳消失未二分(候选 a3a32d8a09、9b107888db,均未实证)。修法取"改测试期望为真实行为"而非恢复标记(恢复会让戳在没有复用时报 hit=1),真复用另立 0.0.63 任务,见下一动作表 P2、N15。
  - 超时定因(实测):all-waves wave 每批 bound 为 55 减波前后工作树扫描约 17 秒,得 37 秒;单批固定开销约 27 秒(1 批 wall 34 秒而批内仅 6 至 8 秒)。8 批波外壳 55 秒在「最慢批 + 约 27 秒」超 55 时被杀,整波收据全丢。上一轮两次超时是波内混入超 37 秒的慢批,不是整体并发:pp-cli-includes 被 run-batch 的 bound 杀(stderr `bound: timeout after 37 seconds`),pp-header-locations 同。modelcache-budget-lru/concurrent、precommit-toolsloc/-source/-symlink 单跑各约 34 秒 wall(批内 6 至 8 秒),均 PASS,不是超时源。
  - 耗时超 40 秒名单(应走 heavy/单独窗口):pp-cli-includes、pp-header-locations(均超 37 秒被杀);临界:snapshots-cx(批内约 30 秒,同波会拖满外壳,建议单跑)。其余已跑批均在 37 秒内。
  - 剩余波数推算(非实测):无 materials 的 light 池约 1000 批,去慢批后每波 8 批约 45 秒,约 120 波以上。
- 快照根复验(2026-11-14 实测,补充上条):用 `git archive origin/main`(45aff656c7)解到 $CLAUDE_JOB_DIR/tmp/snap-root,在其中 `git init` 并提交一次(wt gate 需要 git status),拷入 unisacc.com 与 unisacc.com.build.json;会话 /tmp/unisacc-affected-snap/s(父目录先建,s 必须不存在),init 约 21 秒、executor 约 37 秒。wave 参数形式是 `--light ID ID ...`(一个 --light 后跟多个 ID)。结果:每波 8 批,共 5 次调用,有效完成 3 波(wave-3、wave-6、wave-8),均 identity=STABLE、unknown=0,实测 24 批,PASS 23,FAIL 1;另两次调用(含 modelcache-budget、precommit-toolsloc 系与 cx0-unit 系)因 55 秒 bound 超时无 ALL_WAVE 行,不计结果(疑与串行域 cx-bounded 窗口有关,未证实)。FAIL:ri-image-pem rc=2 marker=RI_IMAGE_PEM,stderr 为 `REFUSE duplicate-key-row`,按规则当确定性缺陷(REFUSE,非负载)。已定因并修复(2026-11-14):tests/ri-image-pem.cx mutate_row 把 trace 型账本里重复出现的前端键 cli/source(resource-005 与 resource-019,同哈希)误当重复行;改为仅对 tip 键保持唯一、cli/source 允许多行并全部处理;共享检出实跑 PASS,ratchets ok=13 fail=0。推算(非实测):无 materials 的 lnx light 批 742 个,按 8 批/波约 93 波。
- check-stdio-buffer 不覆盖 --link-only 与子集编译的缓存 identity 一致性;tty/pty 路径未测;六目标未跑。
- build/speed-baseline.tsv 未更新;run-startup-bench、check-bench-gencode、speed-probe 对新产物未跑。
- 整链:real5(g2/g3 linux 对并发,G_LNX_PAIR=2)3538 s,real4 为 4759 s(-26%);只有这一组对照,未控制机器负载,不作结论。pack 37 s。osx cache-manager 借用定因(A5/R4,2026-11-14):它就是 `tools/cache-retain.cx` 按 `-b osx/x86_64`、`-b osx/arm64` 编出的两份 Mach-O,release-ape pack 不给 `--cache-manager-osx` 时本就会自产(收据记 BUILT_CURRENT_PRODUCT);借用是 candidate-chain 自己的硬拒(tools/candidate-chain.cx:2451 与 2865,引入于 83297e1560「构造未入链」),chain-phases.tsv 里没有构造它的相。实测(已装产品 sha c4c47ecb,HEAD 0e39f4de,cache-retain.cx blob ed70dc63):`sh ./unisacc.com tools/cache-retain.cx -b osx/x86_64 -o OUT` 4.7 s rc 0,sha 89d0b937(两次构造相同),arm64 5.0 s sha 5096089c;与借用副本(x86_64 452d9240、arm64 3f6e4cbd)不同,用 bootstrap 0.0.59(e43ef368)编当前源也不同(c16ec506、888bbbd2),即借用副本不能由现有输入重现,来源未证实。缺的是:pack 前一个链相,用本链候选编译器(不是根目录 bootstrap)编这两份并把路径传给 pack;候选 entry.elf 能否直接 `-b osx/*` 编 tools 源未实跑,自产件在 macOS 上的运行与签名未验证(UNVERIFIED,需 osx 主机)。
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

0.0.62 候选整链(real6)就绪判断(fable 第二轮,2026-11-14 15:50,基于 HEAD ebb91f2032;判断非实跑,P1 与前置条件须实跑后才算数):

- 结论:现在不起。等 B2c、B2e 入库且下列前置条件全绿即起;硬截止 16:40,到点 B2c/B2e 未入库就以当时 main HEAD 起链,未完行保留 new 并如实写进发布说明。不等 B2f、B2g、B3(SPEED-11):前两项只增文件,晚到不改变风险;B3 动既有运行库扫描族,补丁未写,等它会把候选里未经新产品验证的 cxmod 改动再压一小时(F4)。B3 若在 real6 起链前探针已绿则并入,否则作同类第二候选 real7,不与其它类合并。
- 理由:(1) 整链一次约 59 分钟,同一时刻只一条;现在起则 UDP 三行与 TCP 五行落在候选之外,封网主题要再跑一次整链。(2) 14:55 以来 pgrep 无在途 candidate-chain,load1 约 5,/ 空闲 98G(15:49 实测),资源不构成等待理由。(3) 工作树有未跟踪的产品输入 ext/net/udp.cx,此刻冻结会得到与在途工作不一致的 REV。
- 产品输入现有变更(`git diff --name-status v0.0.61 HEAD -- src cx ext exec/facts`,28 个路径):
  - 属库包与运行库类:cx/sys 新增 netmsg、netpoll、netraw、refusals 与只增的 sysnums;ext/net 新增 addr、sock、tcp、unix;ext/sysx 六文件与 ext/crypto 三文件(各 2 至 10 行);exec/facts 的 sys-syscalls(加 35 行)、libc99posix(加 111 行)、refusals(新)。sys-syscalls.tsv 的消费者只有 tools/target-gen.cx(`grep -rl sys-syscalls.tsv tools src cx exec build` 实证),产出 cx/sys/sysnums.cx,不进 kernel 事实,不属模型与网类;target-gen 同时写 cx/compiler/targetname.cx 与 cx/runtime/targetname.cx,diff 中两者未变。
  - 跨类(编译计划器,不是库包):cx/cx0/plan.cx(加 199)、src/cxmod.c(加 199)、cx/compiler/cxmod.cx(加 128)、src/driveplan.cx(1 行)。第 5 条 (d) 已决定按缺陷修复并入同一类,此决定只覆盖"修复",逐项判断如下。
- 逐项跨类风险:
  - CX-U-2(57c7427f6f):中。三条是新增拒绝(行为收紧),不是纯修复;链内 build 与 jobs-tool 段用候选编译器编 tools 与 src,任何一处踩到新拒绝即红。缓解:P1 扫描为 0 才起链。
  - T5 与 cxm_checkcalls 上移(4aee1dfb4a、68039f11f1、1a48fc9a0a):中。同样新增拒绝面;a415413ac2 只验了 cxcc 语料 11 文件,cx-host-run 内联夹具未枚举;build/cx0-three-source-diff.tsv 仍有 14 行标"是"的语义分叉(共 181 行),产品件是手写 cx/compiler/cxmod.cx,源级测试经 plan.cx,源级绿不等于产品绿。缓解:P1 加 `tools/check-cxmod-tri.cx` failed=0。
  - 模块身份 realpath(e576000457):低。可选回调,未绑定或解析失败退回原文本比较;driveplan 只多一条绑定语句。
  - cache-manager 相(5c4ec0e227、b54d7db899):不是产品输入,但改了链本身;它是第 497 相(build/chain-phases.tsv 第 500 行,compare 之后),第一次实跑要等到约 3500 s;候选 entry.elf 以 `-b osx/*` 编 tools/cache-retain.cx 未实跑(已装 0.0.61 实跑 rc 0 不等于候选)。缓解:首链不带 `--cache-manager-osx`(验证自产),该相失败则 `--resume --cache-manager-osx` 用借用副本续段(real5p 先例 rc=0),损失只有 pack 段。
  - netmsg 与号表(832d52a8d3):低。只增文件加两行号;lnx/arm64 的 msghdr 布局未实跑(F15)。
- 仍缺项(build/std-surface.tsv 的 new 行,15:49 为 14 行):ext.net 10 行(UDP 3 在途、TCP 3、recv_until/send_all_until 2、resolve 2)、ext.procx np_exec 1 行、cx.sys 3 行;SPEED-11 未动;osx/win 有名 ENOSYS 的 CI 证据未取。
- 起链前置条件(全部实跑,任何一条不过不起):(1) REV 为 main HEAD,`git status --short` 无未跟踪或未提交的 src、cx、ext、exec/facts 路径;(2) `tools/derive.cx -- --check` 全 OK,`tools/ratchets.cx` 末行 ok=13 fail=0;(3) `tools/check-cxmod-tri.cx` failed=0 且 GAP 具名,`tools/cx0-extract.cx -- --check --all` EXACT;(4) P1 新拒绝数 0;(5) `tools/check-candidate-chain.cx` failed=0(含前置门 3 条与 cache-manager argv 3 条);(6) 改过的 .cx 经 `tools/affected.cx --files ... --run` 无新红;(7) 配方第 1 至 5 步(私有根检出 REV、commit-tree 私有提交、UNISACC_IDENT_HELPER、全新 --out、超时 5400 s);(8) `pgrep -af candidate-chain` 无在途,同 REV 无已完成证据;(9) 起链前在本文件记一行:REV、PID/PGID、截止、输出目录。
- 失败的取证与重跑(整链 3538 s,两次即 7076 s,贴近 2 小时线):
  - 取证顺序:读 real6.out 末尾的 REFUSE 或 rc 行 -> 找到发出该文案的源码行再定性(rc 2 与 REFUSE 当确定性缺陷)-> 按落点分五档:(a) 前置门(第 1 秒);(b) build、build-front、jobs-tool(约前 5 分钟,cxmod 新拒绝最可能落在这里);(c) g2/g3 对象相(real5 各约 700 s);(d) compare(固定点不等);(e) cache-manager 与 pack。
  - 重跑:(a)(b) 修后全新目录重起,损失 5 分钟以内;(e) 用 `--resume` 续段,不重跑前段;(c)(d) 属产品输入缺陷,先用 `tools/chain-precheck.cx -- ROOT CHAIN_OUT NEW_OUT init --files LIST` 以失败链的 ctrl/unisacc.com.dev/entry.elf 对改动文件局部预检,过了再起整链。
  - 止损:同一失败两次即停;第二次整链失败且落点在 cxmod 三源,则候选 REV 改为不含那四个提交的私有提交(用 commit-tree 造,不在 main 上 reset 或 revert 他人路径),0.0.62 只带库包,cxmod 改随 0.0.64 的 D0;第三次整链不在当天起。
  - 观察:起链后第 5、20、45、60 分钟各做一次有界检查(读 checkpoint 与真实 PID),不派子代理干等。

可派任务树(2026-11-14;每项 15 分钟内可交,写域互不相交;tests/batches.tsv 登记行一律走 `tools/commit-own.cx --keyed`;新建工具先 `git log --all --oneline -- tools/NAME`;预期值是验收门,未实跑不记 PASS)。开工条件:今=已装产品即可;候选=补丁今天可写,验收要含它的新产品;链=要整链窗口(同一时刻只一条)。

| 号 | 目标 | 写域 | 验收命令 -> 预期值 | 分钟 | 开工 |
|---|---|---|---|---|---|
| 已落地 | A1a、A1b、A1c(基线 6bcd7a98fb,冷编译标 noisy,speed-probe 仍 UNMEASURED)、A2、A3、A4(9a97992301)、A5(cache-manager 相 5c4ec0e227)、A6、A7a、A7b(7c7124c12c)、A8、A9、A11、A12、B1(c945e3a153)、B2a、B2b(832d52a8d3,cx.sys 0.1.7)、B2d(3502c3966d,ext.net 0.1.5) | - | `git log --oneline v0.0.61..HEAD` 对应提交 | - | 完 |
| A10 | wt-gate-probe 6 条断言转真(wt-gate-clean 已 PASS) | tools/check-wt-gate.cx | probe 批次拆到 55 秒内,6 条断言实跑通过,不删断言 | 15 | 今 |
| B2c | ext.net UDP 三行(在途:ext/net/udp.cx 与 tools/check-ext-net-udp.cx 15:49 尚未跟踪) | ext/net/udp.cx、tools/check-ext-net-udp.cx 加批次行、std-surface 三行 | check failed=0;超长报文 -EMSGSIZE;截止到期 -ETIMEDOUT;三行 new -> exists | 15 | 在途 |
| B2e | 带截止时间 TCP 三行(sy_tcp_accept_from、sy_tcp_connect_to、sy_tcp_dial)与 sy_recv_until、sy_send_all_until | ext/net/tcp.cx、tools/check-ext-net-tcp.cx、std-surface 五行 | `tools/check-ext-net-tcp.cx` failed=0;五行 new -> exists | 15 | 今 |
| B2f | L5:cx.sys 三行(sy_sig_watch、sy_sig_taken、sy_res_live)与 ext.procx np_exec | 各自新文件加 check 与批次行(冻结层只增文件) | 各 check failed=0;std-surface 的 new 行 14 -> 余数如实 | 15 x 2 | 今,不卡 real6 |
| B2g | sy_resolve、sy_resolve4 定因(表写 impl:getaddrinfo 而 cx 层无 libc) | 只读,结论写回本行 | 给出做法或"本版保留 new"及原因 | 10 | 今,不卡 real6 |
| P1 | 起链前新拒绝扫描(CX-U-2 三拒绝、cxm_checkcalls 上移,对链内会编的源) | 只读;结果写回"0.0.62 候选整链就绪判断"前置条件 (4) | 对 tools/*.cx、src/*.cx 及 cx-host-run 内联夹具经 cx0_plan 规划,新拒绝数 == 0(a415413ac2 只扫了 cxcc 语料 11 文件) | 15 | 今,卡 real6 |
| P2 | phase-timing-stamps 的 -c 两条断言改为真实行为(不恢复 art 标记戳);真复用 N15 另做,属驱动与运行缓存类,不随 0.0.62 | tests/phase-timing-stamps.cx(仅测试,非产品输入) | obj.miss 与 obj.hit 各断言:artifact_hit 0 行、lookup 0 行、inference 恰 7 行且阶段名依次为 e2 e1 e3 tapecheck prune lower elf、output kind=image 1 行、两次对象 sha 相同;STAMP artifact 两行改报 UNMEASURED 加原因 no-final-artifact-reuse-on-c;批次 rc0 断言总数不少于 62;runexec.cx 的 ex_ri_artifact 死代码留给 N15 处理 | 15 | 可开工,不卡 real6 |
| N15 | -c 最终对象真复用(rc_open/rc_lookup 对对象模式开键,键绑 -o 路径与目标,命中跳过 e2 至 elf,戳报真实 mode) | src/drive.cx、src/runcache.cx、cx/runtime/runexec.cx(产品输入,补丁先交集成者) | 第二次 -c inference 0 行且对象同字节;改源后未命中;phase-timing-stamps 恢复 artifact_hit 0 然后 1 的断言 | 需拆 | 0.0.63,与 L1 同类 |
| B2 证据 | (1) 内建只有号加五参:exec/facts/gen2-literals.tsv:13 `["__syscall6","syscall",6]`,exec/facts/k2-call.tsv:8 `sys25 kind=sys6 op=syscall`,tools/opinfo-cases-gen.cx:61 `.sys6 syscall, r1, r3, r4, r5, r2, r0` 共六个操作数;cx/sys/netraw.cx:41 `if (f) return -ENOSYS`。(2) 产品头早已同法绕过:include/sys/socket.h:136 注明 gate 只带五个,sendto/recvfrom 经 sendmsg/recvmsg。(3) 本机实跑探针(lnx/x86_64,已装产品,`sh ./unisacc.com /tmp/b2probe/p.cx`,rc=0):`sendmsg=4`、`recvmsg=4 namelen=16 from_family=2 from_port_eq=1 data=ping`。未验证:arm64 上的 msghdr 布局(按 LP64 同为 56 字节,未实跑);`.sys6` 各目标的寄存器分配与溢出区未读到编码行。(4) 不选扩内建:加七操作数形态要动 gen2-literals 的 intrinsics、k2-gen2 的 syscalls、k2-call 的 sysrows 新 kind、lower-code/lower-abi 六目标与 tools/facts-gen.cx,属模型与网类,须重建 kernel 与两代 GEN,和 0.0.62 的库包与运行库类不同类,不得同候选;回滚办法:本方案全是新增文件加两行号表,撤对应提交即回现状 | - | - | - | - |
| B3 | SPEED-11 运行库扫描族(T8) | 运行库对应源(产品输入)加其 check | 补丁先交集成者;字节等价回归输出不变 | 15 | 候选 |
| B4 | 0.0.62 候选整链 real6 | 私有构造根 | `tools/candidate-chain.cx -- --rev REV --out 全新目录 --parallel 12 --cache-manager-osx X64 ARM64` -> RELEASE_APE PACKED,墙钟不高于 3538 s,超 5400 s 即停 | 后台有界 | 链 |
| B5 | 需新产品的断言转真 | tests/cx-namespace.cx、tests/module/units.cx 的 UNVERIFIED 标注 | `sh ./unisacc.com tests/cx-namespace.cx -- ROOT ROOT/unisacc.com OUT` failed=0;`osx-sentinel -funit` 实编 rc=0 | 15 | B4 后 |

,2026-11-14;按"分而治之":工作流按耦合边界切,每版只含一类产品输入大改)

版本里程碑:
- 0.0.62(库包与运行库类):封网清零 new 行(netraw 六参改为新增 cx/sys/netmsg.cx 走 sendmsg/recvmsg 三参,不扩 `__syscall6` 内建,不改冻结的 netraw.cx,仍属库包与运行库类,依据见任务表 B2 证据行;UDP/unix/resolve、带截止时间 TCP);SPEED-11 运行库扫描族;别名运行失败修复(cxmod 三源,按缺陷修复并入);分治工具线并行(C1 相清单化、C3 批次纯函数化、C4 接口哈希选批,不属产品输入);平台审计表扩面加有名 ENOSYS 加 CI 证据。
- 0.0.63(驱动与运行缓存类,2026-11-14 按 A12 重排,详见下方 0.0.63 条目):主杠杆是运行库闭包结果复用(上界约 379 ms,成因待 M1 取数证实),零解码只剩约 49 ms 降为次要;speed12b 类重建 e2 网不并入;目标 first_e2e 500 ms 以下,M1 否定则改口径,不放宽。
- 0.0.64(cx 设计类):export、通配、cx.stream/tlib 重构、紧凑前导。0.0.65 候选:osx/win netraw 真实现。
已定三项取舍(主调度按推荐定):(1) 0.0.63 唯一大改类为驱动与运行缓存(原定模型装载零解码,A12 实测后改),cx.stream/tlib 重构与 export/通配挪到 0.0.64;(2) speed12b 不并入 0.0.62;(3) 平台策略:事实表扩面加有名拒绝先行,不做 NT 上的 fork 仿真,osx/win netraw 真实现推到 0.0.65。
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
| F11 | 新链相在链尾才第一次实跑:cache-manager 是第 497 相,失败要到约 3500 s 才知道;以后每加一个尾部相都重演 | build/chain-phases.tsv 第 500 行;阻塞节整链一条"候选 entry.elf 能否 -b osx 未实跑" | 首链失败走 `--resume` 续段;立规:新相入链前须用上一条链留存的 entry.elf 单独预演一次并留证,链目录清扫时保留最近一条的 ctrl/ |
| F12 | 源级绿不等于产品绿:三源差异表仍有 14 行语义分叉,产品带手写 cxmod.cx,测试走 plan.cx;CXM_MAXU 96 对 128 | build/cx0-three-source-diff.tsv(181 行);4a94e2b141 | 起链前 check-cxmod-tri 必跑;装新产品后 B5 第一批就跑 cx-namespace 与 module-units;D0 的 T6 至 T8 提到 0.0.64 首位 |
| F13 | 共享检出上全集复验得不出判定:wt gate 在波内遇他人改动即整波 UNKNOWN,而提交速率约每小时 50 次;装新产品后的红批清单(F4)只能来自快照根,现为手工五步 | 阻塞节红批复验与快照根复验两条 | 把快照根会话做成一条命令的工具(每波 8 批、慢批单列),在 real6 跑的 59 分钟内造好,B5 直接用 |
| F14 | 冻结点落在派生物过期的提交上:derive 派生物今天三次事后重生(028e7da9d5、3fc367fef1、eea4d03204),另有两条说明为"x"的提交含产品输入(f0030a16a8 含 netmsg.cx),发布说明按提交说明取材会漏项 | `git log --oneline v0.0.61..HEAD` | 冻结 REV 前跑 `tools/derive.cx -- --check`(已列前置条件 2);发布说明的变更清单改由 `git diff --name-status` 生成而不是读提交说明 |
| F15 | lnx/arm64 的 netmsg 只有 CI 能证:msghdr 布局按 LP64 推为 56 字节未实跑,`.sys6` 寄存器编码未读;红了要到发布后的 CI 才知道 | 任务表 B2 证据行"未验证" | 起链前确认 check-cx-sys-netmsg、check-ext-net-udp 批次在 CI 的 lnx/arm64 作业清单内;不在则先登记,不记 PASS |

0.0.63 与 0.0.64 任务(都依赖 0.0.62 新产品已装;开工前逐项拆到 15 分钟):
- 0.0.63(2026-11-14 按 A12 重排;依据与每个杠杆的证据入口在 prd_cx.md 第 5 节 L1 至 L5):零解码上界只有约 49 ms,不再承载 500 ms 门;主杠杆改为运行库闭包结果复用(L1,上界约 379 ms,n=1 且成因为推断)。类别:L1、L2、L4 都落在驱动与运行缓存(`src/runcache.cx`、`src/driveroute.cx`、`cx/runtime/unitcache.cx`、`cx/runtime/modelcache.cx`),与 0.0.62 的库包与运行库类不同类,也不是原定的模型与网类;speed12b 重建 e2 网与跳过 prune 属模型与网类,不得与之同候选。本条与进化路线"0.0.63 唯一大改类为模型装载零解码"冲突,待主调度改那一行。回滚:各项都是纯加速器,撤对应提交即回现状;UNISACC_RUN_CACHE=0 可随时关闭。
  - 今天可只读取数(不改源,互不相交,各 15 分钟内,结果原位写入 prd_cx.md 第 5 节对应行;新源须每次换内容,否则命中运行缓存):
    - 状态(2026-11-14 取数,load1 10.1 至 12.0,非安静机,绝对值偏高;M1 以每次换源内容注释而非换 TMPDIR 避免整程序命中,换 TMPDIR 会令 modelcache 全 miss 污染 model 相;M1 与 M3 已取,M2 部分,M4 部分,M5 未取):M1 部分成立,表与结论见 prd_cx.md L1 行;M2 命中组 model 相 50 至 54 ms、关缓存组 106 至 193 ms(n=2),读入/校验/建布局三分未取;M3 n=3 全为 0 行;M4 同内容两目录 lookup_hit=0,E3CACHE 一组未取;M5 未取(缺 strace 分摊时间)。
    - M1 推理随规模的成因:`UNISACC_RR_TIME=1 sh ./unisacc.com 新源.cx 2>&1 | grep inference`,对 (a) `return 7`、(b) 基准一行 printf、(c) 调一个非 printf 运行库函数,各 n=3 取中位。预期:(a) 合计约 150 ms、(b) 约 530 ms;(c) 若接近 (b) 则 L1 的闭包推断成立,接近 (a) 则推断不成立、L1 须重定。
    - M2 model 相 49 ms 的构成:同命令 `grep -e modelcache -e 'RR_TIME.model'`,对照加 `UNISACC_RUN_CACHE=0` 的一组,n=3。预期:命中组六行 result=hit 且合计约 49 ms;关缓存组更大,差即真 inflate;读入、校验、建布局三分仍 UNMEASURED 则在 `build/pkg-gaps.tsv` 记缺口。
    - M3 单单元路有无单元缓存:`UNISACC_RR_TIME=1 sh ./unisacc.com 新源.cx 2>&1 | grep -c unitcache`。本次 n=1 观察为 0 行;预期 n=3 仍为 0,即单单元路未接单元缓存,L1 须新接。
    - M4 路径无关命中率:同一内容放两个目录各跑一次,读第二次的 `RR_TIME lookup_hit=`。预期现状为 0(键含 cwd 与路径);再设 `UNISACC_E3CACHE=同一目录` 换 TMPDIR 跑,预期 lookup_hit=2(根已可解耦)。
    - M5 残差约 58 ms 细分:`strace -c -f` 与 `strace -r` 各一次首次运行(只读系统工具),按 exec、读产品件、写缓存、子进程分摊;预期给出四项毫秒数,合计与 58 ms 相差 20% 以内,否则仍记 UNMEASURED。
  - 实现项(都动产品输入,等 0.0.62 新产品已装;写域互不相交,各项开工前再拆到 15 分钟):
    - C1 运行库闭包结果复用设计稿,依赖 M1、M3。写域:prd_cx.md 第 5 节 L1 行。验收:行内写明键的全部成分、失效条件、命中与未命中逐字节相同的检查命令。状态(2026-11-14):设计稿已写入 prd_cx.md 5.1 小节,只读读码,未跑任何新实验。要点:(a) 后四流整体按单元拆分再拼接不成立,只有 e3 有可拆先例(`cx/compiler/fwdmerge.cx:233`),prune 与 elf 需整程序,lower 未验证;(b) 方案 A 只复用闭包 e3 带子,约省 116 ms,671 到约 555 ms,单做过不了 500 门;方案 B(闭包 prune 加 lower 结果,键加引用面)再省约 177 ms,取决于只读实验 E2、E3;(c) 键 UNICL1 不含路径,不依赖 C2b;(d) C3 拆为 C3-1 至 C3-5,其中 C3-1 写域越出本树原列的两个文件(需 `src/driveunits.cx` 与入口选择),待主调度改派工行;(e) 未验证项:闭包进入后四流的形态、rc_hex_ident 是否覆盖后四流模型、带子落盘取数入口,均在 5.1 给了取数办法。E1 至 E4 进展(2026-11-14):E1 已测(闭包是同一 .unit 0 内按头文件来源标记交错的文本,非独立段;非用户行占比 97% 至 99.6%);E2、E3 已测(2026-11-14):E2 stream prune 对 -S 带子逐字节无变化且未用函数在 prune/lower 后仍在(标号数 404 对 404),可观察的后四流无函数级可达性裁剪;E3 按标号切 lower 输出比 (c)(d),共有 398 段中 385 同字节(96.7%,按字节 95.4%),差异在启动收尾桩、main、生成标号,@sym 地址只在头部;E4 未测(闭包切法未定)。方案 B 判断:前提 prune 按引用面裁剪不成立(prune 实际不裁),lower 逐函数基本可拆,故方案 B 不以"闭包 prune 结果"成立,可缩为"闭包 lower 逐函数结果缓存"(上界不超 lower 的闭包份,约 (b)到(c) 的 80 ms 量级,未测),拼接需处理头部与生成标号,未决但不再偏否定 prune 部分;500 门仍须 A 加 L5(a)。细节在 prd_cx.md 5.1 分期。
    - C2a 缓存根默认值与 `UNISACC_CACHE_DIR`。写域:`cx/runtime/modelcache.cx` 的 `mc_cache_dir`。验收:两个不同 TMPDIR、不设任何缓存变量,同源第二次运行 `RR_TIME lookup_hit=2`;`tools/check-unit-frontcache.cx` failed=0。
    - C2b 路径无关键,依赖 M4 与 `__FILE__`、诊断、相对 include 的取证。写域:`src/runcache.cx` 的 `rc_hex_sources`。验收:同内容两个目录,第二次 lookup_hit=2 且两次标准输出、标准错误、退出码相同;含 `__FILE__` 的夹具必须仍未命中或输出正确路径。
    - C3 闭包结果复用实现,依赖 C1。写域:`src/driveroute.cx`、`cx/runtime/unitcache.cx`。验收:`sh ./unisacc.com tools/run-startup-bench.cx` first_e2e 三次中位 500 ms 以下且断言 failed=0;关缓存(UNISACC_RUN_CACHE=0)与开缓存的产物字节相同。若 M1 否定推断,此门改口径并在此写明,不放宽不删。
    - C3z 零解码 mmap 布局(原 C3,降为次要),与 C2a 同写 `cx/runtime/modelcache.cx` 故串行在其后;另写 `cx/runtime/routestep.cx`。验收:RR_TIME 六流 model 相合计 10 ms 以下(现约 49 ms),命中与未命中同字节。
    - C4 整链对象相拆分到整链 2400 s 以内(F9 的门),不变。
- 0.0.64:D0 cxmod 三源归一(2026-11-14 决定:先归一再加语法,依据与拆分见 prd_cx.md 10.5;`cx/compiler/cxmod.cx` 改为 `cx/cx0/plan.cx` 的第二投影,验收 `tools/cx0-extract.cx -- --check --all` 两行 EXACT、`tools/check-cxmod-tri.cx` 末行 gaps=0;其中 T1 至 T4 不动产品输入、今天可开工且互不相交,T5 至 T8 动产品输入、都写 plan.cx、串行、等 0.0.62 新产品已装;回滚:T8 之前产品件仍是手写件,撤 T5 至 T7 的提交即回到现状)-> D1 export(依赖 D0:此后每条新拒绝只改 plan.cx 一处;若 D0 的 T8 未落地则 D1 不开工,不退回手工三处对齐;验收 `tests/cx-namespace.cx` 门面正例 2、拒绝 4,`tools/cx0-extract.cx -- --check` EXACT)-> D2 通配(依赖 D1;正例 1、拒绝 5)-> D3 符号级导入收敛(依赖 D0,与 D1、D2 同写 plan.cx 故串行;`tests/module/check.cx`)-> D4 text.text 物理拆面(依赖 D1 的 export;第二候选,受 F9 门约束)-> D5 层感知前导(不依赖 D0 至 D3;末位,超 2 小时移 0.0.65)。D0 至 D3 同属 cxmod 三源一类、同候选;同写域串行,不并发。
- 0.0.64 之后的优化候选(未排版本):prune 有 callr 时整带回退。实测凡带 stdio 闭包的程序 prune 恒等(`exit` 里一处 `callr`),用户死函数不裁,约每个小死函数多 1.1 ms、82 B(200 个:后三流多约 228 ms,ELF 36,894 对 53,278 B);对 hello 本身只差一个函数。属有意保守合同(`tools/prune-cases-gen.cx:111` 的 indirect-call-preserved),不是缺陷。改法是以取址集合为根继续裁剪,类别为模型与网类,不与驱动与运行缓存类同候选;不删现有断言。证据入口 prd_cx.md 5.1 "E2 定因"。回滚:未动产品,无需回滚。

0.0.63 至 0.0.65 任务树更新(fable 第二轮,2026-11-14 15:50;单项 15 分钟内,写域互不相交;"今"=不动产品输入、已装 0.0.61 即可开工;"装后"=等 0.0.62 新产品已装):

| 号 | 版 | 目标 | 写域 | 验收命令 -> 预期值 | 开工 |
|---|---|---|---|---|---|
| N1 | 63 | M5 残差约 58 ms 细分 | prd_cx.md 第 5 节 M5 行 | `strace -c -f` 与 `strace -r` 各一次首次运行 -> 四项毫秒数,合计与 58 ms 相差 20% 以内,否则记 UNMEASURED | 今 |
| N2 | 63 | M2 model 相读入、校验、建布局三分 | prd_cx.md 第 5 节 M2 行、build/pkg-gaps.tsv 一行 | `UNISACC_RR_TIME=1` 命中组 n=3 -> 三分各有毫秒数,取不到则 pkg-gaps 记缺口 | 今 |
| N3 | 63 | C2b 前置取证:`__FILE__`、诊断路径、相对 include 对路径无关键的影响 | prd_cx.md 第 5 节 L4 行 | 三个夹具各在两个目录运行 -> 列出输出含路径的情形数,给出键须保留路径的条件 | 今 |
| N4 | 63 | E4 闭包切法取数 | prd_cx.md 5.1 的 E4 行 | 按 5.1 给的取数办法 -> UNMEASURED 改实测或写明缺的落盘入口 | 今 |
| N5 | 63 | speed-probe 加 run 分相(读 UNISACC_RR_TIME 六流行) | tools/speed-probe.cx 及其 check | `tools/speed-probe.cx` 输出含六流 inference 与 model 行;check failed=0;pkg-gaps 对应缺口行销掉 | 今 |
| N6 | 63 | C2a 缓存根默认值与 UNISACC_CACHE_DIR | `cx/runtime/modelcache.cx` 的 mc_cache_dir | 两个 TMPDIR 同源第二次 `RR_TIME lookup_hit=2`;`tools/check-unit-frontcache.cx` failed=0 | 装后 |
| N7 | 63 | C3-1 单文件路接单元缓存入口(写域须含 `src/driveunits.cx`,原派工行已不够) | `src/driveunits.cx` 与入口选择 | `UNISACC_RR_TIME=1` 单文件新源 `grep -c unitcache` 大于 0(现为 0) | 装后,N6 不同写域可并行 |
| N8 | 63 | C4 对象相拆分第一步:定 closure-m*.o 单键 349.8 s 的构成 | build/phase-times.tsv 新增行 | 对 real6 的 checkpoint 用 .cx 求和 -> 单键内编译、推理、落盘三项各有实测行 | real6 后 |
| N9 | 64 | 通配的产品层禁用规则进棘轮(F10 提前动作) | tools/ratchets.cx 的 cxapis 项及其 check | 夹具含 `use m::*` 的产品层文件 -> 棘轮 fail;现仓 ok=13 不变 | 今 |
| N10 | 64 | D0 的 T6:三源差异表 14 行语义分叉逐行归一方案 | prd_cx.md 10.5 的 T6 行 | 14 行各有"上移到 plan.cx"或"宿主片段"的归属,无未归属行 | 今(只读) |
| N11 | 64 | D0 的 T6 至 T8 实现(同写 plan.cx,串行) | cx/cx0/plan.cx、cx/cx0/projection.tsv | `tools/cx0-extract.cx -- --check --all` 两行 EXACT;`tools/check-cxmod-tri.cx` 末行 gaps=0 | 装后 |
| N12 | 65 | CI 平台作业结果回收成表(F5 提前动作) | tools 下新工具加 check 与批次行(先 `git log --all --oneline -- tools/NAME`) | 对 run 43554326355 出表,行数 == 六目标作业数,每行带结论与作业号 | 今 |
| N13 | 65 | osx/win 有名 ENOSYS 按表断言 | exec/facts/refusals.tsv 的平台列与其 check | 生成器 `--check` OK;41 个无原因点减到 0 或余数具名 | 今(冻结 REV 之后提交,不混入 real6) |
| N14 | 65 | lnx/arm64 netmsg 布局 CI 探针(F15) | tests/batches.tsv 的 arm64 行(commit-own --keyed) | CI lnx/arm64 作业日志含 netmsg 断言 21 条 failed=0;本机状态 DEFERRED | 今登记,CI 后转真 |

速度杠杆下一个可测动作(第二轮,每档一个):(1) 冷启动:N1 的 M5 残差细分,之后才能判断 500 ms 门靠方案 A 加 L5(a) 是否够;(2) 冷编译:现有 n=3 行标 noisy(6bcd7a98fb),在 load1 低于 2 的窗口重跑 `tools/coldcompile.cx` 三文件各 3 轮,极差除以中位低于 10% 才把 12300 与 11600 当门,否则门不生效并写明;(3) 生成代码与分相:N5 让 speed-probe 出 run 分相,出数后入 build/speed-baseline.tsv 的 probe 行,UNMEASURED 才能销。

三个可测速度杠杆(数字只取自 build/speed-baseline.tsv 中四个工具的行):
- (1) 冷启动:run-startup-bench first_e2e 三次 684.051/704.882/714.267 ms(real4 c55533c7,n=3),同工具 repeat_median3 约 27 至 29 ms(0.0.59 与 local-0060 组);首次与重复相差约 650 ms,其中推理约 531 ms、解码约 49 ms(A12 实测,零解码上界只有约 49 ms)。度量:同工具 first_e2e,目标 0.0.62 为 670 以下、0.0.63 为 500 以下。
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
