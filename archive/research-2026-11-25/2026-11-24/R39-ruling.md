# R39 发布门缺陷评审:第一车 0.0.106 严格预跑 W0087 check-gen-all-drift 确定性红

取证 23:13 至 23:25,只读加私有树实跑。私有树与日志均在 ~/.claude/jobs/7f46e032/tmp/R39/。

## 裁定(一句)

这不是候选产品缺陷,也不是 lane-inputs 过期,而是第一车内的工具提交 3de1f09b87(affected-canary 读集补入 ext 与 cx)与本批夹具迷你仓(不含 tests/)的交互:夹具缺陷,修复只有一行,落在 tools/check-gen-all-check.cx,不在 SOURCES_SID 路径集、不在冻结路径;不需要重走整链,走 R36 路径 B(车上一个 tools-only 提交加 --post-chain-merge 豁免加受影响批用候选 APE 重跑);不准登记预期失败。

## 证据(全部实跑)

(1) 红断言原文:FAIL precommit-lane-inputs-add-ext-blocked-and-synced: staging new ext/ file blocks on lane-inputs-gen DIFF until build/lane-inputs.tsv is staged。输出 CHECK_GEN_ALL_DRIFT ok=2 failed=1。
(2) 手工拆步(候选 APE 5f83ac93,第一车树):
- 前半(暂存 ext/net/cgac_dummy.cx):rc=1,blocked=2,两条 BLOCKED:product-table-drift lane-inputs-gen DIFF(预期)与 affected-canary tools/affected.cx rc=2(非预期)。blocked_ok 为真。
- 后半(重算并暂存 lane-inputs.tsv):lane-inputs DIFF 消失,但仍 rc=1,blocked=1,PRECOMMIT_GATE_FAIL gate=affected-canary reason=affected-rc,原因行:affected: run from the repo root (tests/batches.tsv not found)。pass_ok 为假,合取转红。
- 日志:step3a.out、step3b.out。
(3) 机制:tools/precommit.cx:184 的 affected-canary 读集含 ext cx;暂存任一 ext 文件即触发 affected --registry-check(precommit.cx:3760-3796);夹具 cgac_prep_minirepo(check-gen-all-check.cx:61-73)只归档 include src cx ext seed exec build weights iterate kernel tools cxroot.tsv,不含 tests/,而 tools/affected.cx:1889 要求 tests/batches.tsv 与 tests/run-batch.cx,拒绝 rc=2。
(4) 与候选无关:同一第一车树换已装 0.0.105 产品(unisacc.com sha 934c7c0b)跑同命令,同样 ok=2 failed=1(inst.out)。
(5) 二分(已装产品):01d73ad35b^ -> ok=3 failed=0 绿;3de1f09b87 -> ok=2 failed=1 红(首跑并行负载下 45 秒超时,单独重跑 20.0 秒红)。01d73ad35b 本身未跑;按其提交说明读集只有三张表与 affected.cx,推断仍绿,引入点为 3de1f09b87(E8 第二轮,读集补全)。
(6) 0.0.105 的 strict 表 PASS 与此一致:该门在 0.0.105 之后才进树。

## 分类

批次夹具与新提交门的交互,属"第一车树内工具缺陷",不是产品输入缺陷。候选 APE 字节、SID、整链 499 证据均不受影响。agy-flow 的线索方向对(新增 ext 文件后分叉),但分叉点不是 derive 门或生成器,是 affected-canary;cdx 定位的合取行正确。

## 修复(已私有验证核心路径)

在 cgac_prep_minirepo 的 rm 清单里加 tools/affected.cx(与现有 rm tools/ratchets.cx 同处)。依据:precommit.cx:3762-3768 为夹具仓设计了显式分支,仓内从未有 affected.cx 则打印 AFFECTED_CANARY skipped=1 reason=no-tools/affected.cx (fixture repo),不阻断。
私有实跑(候选 APE,mini-fix2):前半 rc=1 blocked=1 仅 lane-inputs DIFF;后半 rc=0 blocked=0。先红后绿成立。
否决的两种修法:
- 往迷你仓补 tests 三张表:实跑无效,registry-check 还要 tests/run-batch.cx 及每个 IMPLEMENTED 批的源文件(REFUSE metadata missing-implemented-source id=modelcache-budget-lru),等于归档整个 tests/。
- 改 precommit 让缺 tests/ 时放行:放宽真实门,不做。
不违反铁律的理由:未删断言、未改断言文本、未改时限;canary 门自身有独立批次(58c7026b2e 的 canary.red 夹具)覆盖;本批的合同是 lane-inputs 门,修复后该合同被更干净地单独检验。

## 路径比较

- SID:tools/check-gen-all-check.cx 不在 seed/ident.c:101-115 清单,也不在 sourcepackage.cx 钉住文件内(引 R36 证据 (5),本次未重读);不碰 tests/,不在冻结路径。
- 路径 B(推荐):车上加一个 tools-only 提交;跑 git merge-base --is-ancestor 与 git diff --name-only 2e8a19a342 HEAD(应只有 1 个文件);用候选 APE 重跑共用该源文件的 9 个 check-gen-all 分片批加 affected.cx 给出的波及批;预跑从 W0087 续跑;release-exec 带 --post-chain-merge。耗时约 10 分钟加剩余预跑。
- 路径 A(车不动、修复只落 main):不成立。严格预跑表在车树上该批确定性红,238 窗无法全绿。
- 路径 C(重链):不需要,产品字节未变。仅作退路:28 加 60 加发布约 30 加余量 15,最晚起链约 00:45(按冻结 03:00 推算),只容一次。
- 预期失败登记:否决。修复一行且已验证,登记等于为过线放宽;且指纹(批号加断言文本加 affected-canary rc=2 tests/batches.tsv not found)会把同批下任何真实的 lane-inputs 门退化一并遮住,因为红的是合取。

## 执行顺序

(1) 先让 --keep-going 的预跑跑完剩余窗,拿全红项清单再定,不要修一个撞一个。
(2) 红项全是 tools-only 夹具类 -> 合成一个 tools-only 提交走路径 B;任一红项碰 SID 集、tests/ 或 release/ -> 路径 C,00:45 前起链。
(3) 修复同时落 main 工具线(冻结后推)。
(4) 发布说明如实写:标签 2e8a19a342 内该批夹具有缺陷,修复提交在链后,产品字节与 SID 未变。

## 风险

- 豁免分支无自动核对(R36 证据 (1)),靠人工两条 git 命令自证。
- 我只验证了第三步两半,未用打过补丁的 check 文件整批跑 --drift,也未跑其余 8 个模式;--unstaged 与 --pass 暂存的是 tools/win-argv-gen.cx 与 include/stdlib.h,不在 canary 读集内,推断不受影响,未核。
- 迷你仓删 affected.cx 后 build/lane-inputs.tsv 或其它派生表是否把 tools/affected.cx 记为输入:第三步实跑未见额外 DIFF;第一、二步未在补丁态下跑,未核。

## gatekeeper 复核点

(1) 补丁态下 9 个 check-gen-all 分片用候选 APE 5f83ac93 全绿,且 --drift 为 ok=3 failed=0;(2) 变异自证:把补丁态夹具里重算 lane-inputs 那步去掉,断言必须转红;把 precommit 的 lane-inputs 门去掉,断言必须转红(证明删 affected.cx 没把断言变成恒真);(3) git diff --name-only 2e8a19a342 HEAD 只含 tools/check-gen-all-check.cx,与 SID 清单逐项不相交;(4) INDEX 豁免行文案与发布说明一致;(5) affected-canary 自身批次在车树上用候选 APE 为绿。

## 结构性原因(一条)

3de1f09b87 改了提交门读集,却没有触发"复制 precommit.cx 进迷你仓"的夹具批;整链 499 不含 commit 级批,所以链全绿而预跑才红。推断:该批的输入声明没把 tools/precommit.cx 记成输入,或落地时没跑 affected,未核。发布后在 main 上核 tests/batch-product-inputs.tsv 与 affected 闭包,二选一补上。

## 我最可能错在哪里,怎样便宜证伪

- 错点一:补丁态整批另有红(第一、二步受删 affected.cx 影响)。证伪:私有树改一行跑 --drift,约 25 秒。
- 错点二:01d73ad35b 已经红,引入点早一个提交。不影响处置;证伪 25 秒。
- 错点三:剩余 151 窗里还有同类夹具红(其它建迷你仓并暂存 ext 或 cx 文件的 check)。证伪:grep tools/check-*.cx 里同时出现 precommit.cx 与 git init 的文件,逐个看是否保留 affected.cx,约 3 分钟。
- 错点四:release-exec 之外另有门要求 HEAD 等于链提交且无豁免。证伪:grep cand.commit 的全部消费者。
