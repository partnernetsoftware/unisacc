# R40 发布门范围评审:tests/ 下夹具修复(W0191 fwdstub-replay)走哪条路径

取证 23:50 至 00:00,只读。未实跑任何批次。

## 裁定(一句)

走路径 B 的受限变体(下称 B-t):tests/check-fwdstub-replay.cx 单文件、只改候选启动方式、不动任何 tests/*.tsv 与 harness 时,不属路径 C;但放行前必须先闭合一个 R39 没有的前置证明:解释"同一文件自 661de08935 起未改、0.0.105 严格表与今晚整链为何是绿、现在为何红"。这一条 00:20 前闭不上,就在 00:37 前起路径 C。

## (1) tests/ 是否真属路径 C

不是身份原因,是证据与纪律原因。
- SID:seed/ident.c:101 目录集为 exec src kernel include weights seed cx,L110 加 iterate/kernel/typekw.tsv,L113-115 加 13 个 tools 支撑文件。tests 不在内(本次重读已核)。tests 改动不改候选字节。
- F 门:tools/release-exec.cx:934-968,--post-chain-merge 分支只写 INDEX 豁免行,不核任何路径(本次只 grep 复核行号,逻辑引 R36 证据 (1))。对 tests 没有额外核验,所以也没有额外保护,全靠人工自证。
- release-exec 对 tests 的唯一硬读取是 tests/split-parents.tsv(L529-617)。本修复不碰。
- tests/batch-product-inputs.tsv:59 把 fwdstub-replay 绑定到 src/drive.cx 等 12 个产品输入前缀;该表是"产品输入 -> 批"的选择表,只改 check 源文件不改这行,选择关系与候选绑定不变。
- R39 把 tests/ 列进路径 C 的真实理由(推断,依据 R36 四问 (1)):tests/ 里有三类东西,(a) 表(batches.tsv 时限与期望、coverage、split-parents),(b) harness(run.cx、run-batch.cx),(c) 单批 check 源。(a)(b) 一改,整链 499 批的证据对全部批失效,只能重链;(c) 只让本批证据失效。R39 当时没有细分,一刀切是保守写法。再加 build/freeze.tsv 路径列含 tests,属冻结纪律。
- 但本案比 R39 的两笔更敏感:fwdstub-replay 是 product-qualification 批(tests/batch-domains.tsv:131),直接运行候选;前两笔是工具夹具。所以证明标准要高一档。

## (2) B-t 所需额外证明(全部要留证到 $CLAUDE_JOB_DIR/tmp)

(P0,前置,未核,最重要)红因闭合。已核事实:该文件最后一次改动是 661de08935,早于 0.0.105;运行库无 ENOEXEC 回退(车树 cx、src、tools/cxlib 内只有 cx/sys/sysinfo.cx:50 一处名字表);车对 f23c7b56fd 在 cx/runtime 与 cx/core 只动 rcident.cx;本机 /proc/sys/fs/binfmt_misc 只有 register 与 status,没有 APE 注册;已装 unisacc.com 头为 MZqFpD,同为 APE 壳形态。推断三种可能:(i) 本机 binfmt 注册今晚丢失(环境);(ii) 此前传给 {candidate} 的是别的形态或包装;(iii) 候选 0.0.106 头形态变了(产品行为变化)。证伪办法,2 分钟:用未打补丁的 check 加已装 0.0.105 产品当候选跑同批。同红 -> 与候选无关,B-t 成立;0.0.105 绿而 5f83ac93 红 -> 是产品行为差异,不是夹具缺陷,B-t 作废,且不是重链能解决的,转"发布说明写明加撤冻结改期"由 fable 另裁。
(P1) git diff --name-only 2e8a19a342 HEAD 只含已有三个 tools 文件、release-exec.cx 与 tests/check-fwdstub-replay.cx;逐项不在 SID 清单;tests/*.tsv 零差异。
(P2) 差异逐行:只在 argv 数组前插 sh;断言文本、期望 rc、10 秒与 8 秒时限、residue 判定逐字节不变。
(P3) 同一候选 sha 5f83ac93 先红后绿;预计耗时在批上限 55 秒内。
(P4) 变异自证两条:把收据破坏拒绝路径的期望改掉必须转红;把候选换成恒 rc0 的假程序必须转红(证明套 sh 后没把断言变成恒真)。
(P5) 套 sh 的副作用:parallel 段 sy_spawn 拿到的是 sh 的 pid,须确认 APE 壳是 exec 接力而非 fork,否则超时杀组与 residue 判定被削弱;实测超时变异(候选挂起)仍能清干净。
(P6) 同类扫描:车树 tests/ 与 tools/check-*.cx 中把 {candidate} 直接放 argv[0] 的其余批,对照 W0192 至 W0238 是否还会撞;先让 --keep-going 跑完拿全红单再一次提交。

风险:豁免无自动核对;product 批的启动方式被改,若 P5 不过等于弱化了并发回放合同;严格表会出现主体在原 frozen rev、补跑窗在新 rev 的混合来源,release-exec --strict 是否接受未核。

## (3) 路径 C 的时间账

整链 28 加预跑 60 加发布 40 加余量 15 等于 143 分钟;冻结 03:00,最晚起链 00:37,只容一次,任何一次新红即出窗。SID 不变推断候选字节相同、构造相位可命中缓存,但批次不缓存,不应按"更快"排程(未核相位拆分)。
预跑复用:tools/prerun-drive.cx:1329-1350 首次运行把 HEAD 写进 out 下 frozen-source.rev,之后每窗 source= 与之比对(L1396-1447),不等即 PRERUN_SOURCE_DRIFT 加 PRERUN_STOP。新提交必然换 rev,同一 out 目录不能 --resume。不许手改 frozen-source.rev 或 done.W*,那是改证据。可行的只有:原 out 目录保持原 frozen 源不动跑完全部窗,红窗另开 out 目录在新 rev 上用同一候选 sha 单批补跑,两份证据并列入发布记录。这正是 B-t,不是路径 C;路径 C 下预跑必须全量重做。

## (4) 推荐与执行步骤

推荐 B-t,带硬截止。
(1) 立即:P0 对照实验(已装 0.0.105 当候选跑未补丁批)。同时原预跑 --keep-going 跑到 W0238。
(2) 00:10 前:cdx 出补丁,范围仅该文件全部候选调用点(:57-66、:113-114、:221),不改表。
(3) 00:20 前:P1 至 P5 留证;红窗 W0087、W0148、W0150、W0191 在新 rev 新 out 目录用候选 5f83ac93 补跑全绿。
(4) gatekeeper 复核通过后才执行发布步;--post-chain-merge 理由写明四个文件名、SID 不变、补跑窗号。
(5) 发布说明如实写:标签 2e8a19a342 内该批夹具直接 execve APE,在无 binfmt 注册的主机上红;修复提交在链后;产品字节与 SID 未变;严格表四窗为链后补跑。
(6) 截止:00:20 P0 未闭合或新红项碰 SID 集、tests/*.tsv、harness、release/ -> 00:37 前起路径 C;过 00:37 -> 不发,撤冻结改期,不登记预期失败。
(7) 发布后落 main:R36 的 F 门自证补丁增加分类,tests/check-*.cx 单文件且 tests/*.tsv 零差异归 B-t,其余 tests/ 归 C;prerun-drive 增加"候选 sha 相同的单批补跑并入"的正式入口,免去混合证据靠人工并列。

## gatekeeper 复核点

(1) P0 对照结果与结论一致;(2) 差异不含 batches.tsv、coverage.tsv、batch-product-inputs.tsv、split-parents.tsv、run*.cx;(3) 断言与时限逐字节不变;(4) P4 两条变异转红;(5) P5 超时清理实测;(6) 补跑证据的候选 sha 等于 5f83ac93、source rev 等于车 HEAD;(7) release-exec --strict 对混合来源表的实际行为;(8) INDEX 豁免文案与发布说明一致。

## 我最可能错在哪里,怎样便宜证伪

- 错点一:红因是候选行为变化而非夹具或环境。证伪即 P0,2 分钟。这是本裁定的单点。
- 错点二:release-exec --strict 或 dvqualify 拒绝混合 rev 的严格表,B-t 在最后一步才撞墙。证伪:--dry 加拼好的表先跑一遍,约 1 分钟;应在 00:20 前做。
- 错点三:冻结守卫(check-chain-ready-freeze、prepush)拒绝车上 tests/ 路径提交。证伪:本地提交时即见;被拒不得绕过,转路径 C。
- 错点四:预跑实际跑在 out 下 frozen-root 快照上,前两笔 tools 提交如何进入预跑我未核(L1298-1375 未读全);若补跑入口与我描述不同,以 prerun-drive 实际合同为准。
- 错点五:最晚起链 00:37 用的是简报给的 28、60、40 分钟,未自行核日志。
