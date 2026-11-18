# Q-11 合并预演与 v0.0.77 冻结流水线(草稿,不入仓,2026-11-18)

## 结论

- v0.0.76 的 66 批补差不是批慢,是顺序错:预跑在 T 布局上跑(22:26 起),合并在预跑之后(22:46 到 22:52)才把 main 的 tools/affected.cx 带进来;def_sha 含 affected.cx 与 run-batch.cx 的 sha(tools/dvledger.cx 第 54、66 行),一处变则全部批失效。
- 根治 = 合并前移到链运行期间得 T2,预跑只在 T2 布局上跑,从预演开始对"运行器与登记表"路径静默,冻结 = 把 main 快进到 T2(或只差白名单的 T3)。
- 冻结门改成"补差预判为 0 才冻结":补差在冻结前用 dvledger delta 对最新合并点干跑,needs 大于 0 就在冻结前补跑,不带进冻结窗。冻结后只剩哨兵 4 批实跑。
- 预测冻结到公开 10 到 13 分钟(硬项:prepare 加证据提交约 3 分,CI 4 分 48 秒实测,读回与 Y2 约 1 分);上限 15 分钟,超出即按回退规则处理。
- 运行器(affected.cx、run-batch.cx)保留在复用判据内,不排除、不做"等价豁免";靠静默协议保证它不变,变了就全部重跑。

## A v0.0.76 事实基线

| 事实 | 数值 | 证据 |
|---|---|---|
| 冻结到最终公开 | 22:52:58 -> 23:20:12,27 分 14 秒 | release/evidence/0.0.76/time-ledger.tsv 末行 |
| 预跑(T 布局) | 22 批 914 秒加 39 批 511 秒,复用 0 | 同表 prefreeze 两行与 reuse 行;strict-prefreeze.tsv 首行"不复用" |
| 预跑首窗 | sample 前 7 批全 UNKNOWN(60 秒外层或 57.5 秒 rc 3),load1 4.2 到 7.5 | strict-prefreeze.tsv 第 5 到 13 行 |
| 冻结后补差 | 66 批 515 秒,65 PASS 1 FAIL(c-negative 53/9 预期) | time-ledger postfreeze-done;v0.0.76-strict 表 |
| 冻结树相对 T | 50 个路径,其中 tools 11、tests 26(含 affected.cx、batches.tsv、tools-compile-sources.tsv、pp-header-locations.cx) | git diff --name-only d9903e5874 debb8f43f9 |
| 合并形态 | devsvc 提交丢第二父,原生 git commit 收尾;main 又前进,再合一次 | time-ledger merge 行;提交 497bc0fd68、a986b20bca、a2deed1d12 |
| 冻结后修工具 | check-release-exec N0 夹具在 qualify 布局无历史,约 4 分 | 提交 debb8f43f9 |
| 回滚一 | 读回内 chain-step smoke 45 秒超时(冷编,load1 9.8),手动复跑 5 秒 | time-ledger readback-fail |
| 回滚二 | Y2 推送非快进,main 被 d82b26e3e3 与 27654c7b62(Q-01,tools)前进 | time-ledger Y2-fail;git log 23:17:20、23:17:30 |
| 冻结窗内他人推 main | 23:17:20 Q-01(tools)、23:19:26 ujs 合入(docs/uxe) | git log d9903e5874..0c8f512330 |

## B 合并预演流程(链 25 分钟内并行完成)

前提:链输入是 T 的快照(tools/chain-step.cx 有 git archive 调用 3 处;链是否完全不读版本工作目录 UNVERIFIED,起链时核一次,不成立则合并改到链结束后做,预演其余不变)。本机 git 2.34.1 无 merge-tree --write-tree,不能免工作树合并。

| 步 | 动作 | 机器可判通过条件 | 失败 |
|---|---|---|---|
| M1 | 版本工作目录内 git fetch;devsvc git merge origin/main | rc 0 或冲突清单只含 M2 表内文件 | 冲突落在表外文件即停,上报 |
| M2 | 冲突表按"取并集 -> 按事实重算"处理(见下表) | 每张表对应 check 的 rc 0 | 任一 check 非零即停 |
| M3 | 原生 git commit --no-edit 收合并 | git rev-list --parents -n 1 HEAD 输出 3 个字段 | 2 个字段即丢父,git reset 不许用;重做 M1(分支本地未推) |
| M4 | 产品输入零改动 | freeze-plan head-delta ROOT T 只报非产品路径;git diff --name-only T T2 在 src、cx、ext、kernel 下为空 | 非空即停:候选作废,不是预演问题 |
| M5 | 候选键相同、版本槽 | release-packcheck slot rc 0;check-releaseversion rc 0 | 停 |
| M6 | 棘轮与派生物 | ratchets 15/0;derive --check --changed rc 0 | 停,修后重跑本步 |
| M7 | tools-compile 一致 | tests/tools-compile-sources.tsv 分片数等于 batches.tsv 中 tools-compile-NNN 行数,且等于 batch-domain-index 对应行数 | 补行后回 M2 |
| M8 | 推版本分支,记 T2 | git merge-base --is-ancestor origin/main T2 rc 0 | - |

冲突表标准流程(执行者:发布单子代理一人,持 devsvc 写窗口锁;失败即停):

| 表 | 取并集 | 按事实重算 | 核对 |
|---|---|---|---|
| tests/batches.tsv、coverage.tsv | 两侧行都留,按 id 去重 | 无 | tests/run.cx --list rc 0 |
| tests/batch-product-inputs.tsv | 两侧 id 声明行都留 | 无(v0.0.76 缺声明七条属登记缺口 REGISTER-NO-PRODUCT-INPUT-DECL) | affected --ids 抽新增批 rc 0 |
| tests/batch-domain-index.tsv | 不手合 | 生成器重算 | derive --check |
| tests/tools-compile-sources.tsv 与分片行 | 不手合 | 按 git 清单重算,补缺的分片行(v0.0.76 补了 157) | M7 |
| build/table-index.tsv | 不手合 | tool-index.cx --write | derive --check |
| doc-links、links | 不手合 | 各自生成器 --write | 各自 --check |

三方合并已有库 tools/cxlib/appendmerge.cx 与 check-tsvx-merge3.cx(能否直接当 git 合并驱动 UNVERIFIED),脚本化见 E 的 N1。

## C 预跑位置与复用判据

预跑放 T2 布局:链完成当刻 T2 已就绪,qualify 以 T2 为布局提交、链 APE 为候选。release-run-exec-gates 这类"布局内无历史"的失败在冻结前暴露并修,修完算 T3。

复用判据(五条不降低,在根治方案 (a) 到 (e) 上只补机器规则):

| 键 | 规则 | 不满足时 |
|---|---|---|
| cand | 账行 cand 等于发布 APE sha256(packcheck ape 保证链 APE 即发布 APE) | 全部重跑 |
| runner | tools/affected.cx、tests/run-batch.cx、tools/dvqualify.cx 及其 use 闭包的 sha 与预跑时相同 | 全部重跑 |
| def | 批行文本加 source、entry 文件 sha 相同 | 该批重跑 |
| tree | 预跑布局树与冻结树的差异路径全在白名单(docs、plans、prd*.md、release/evidence、release/candidate.tsv、archive) | 走下一行 |
| tree 出白名单 | 差异路径交给 affected --files 选批;tools/cxlib 下任一文件变 -> 全部 tool-selfcheck 与 tools-compile;tools 清单增删 -> tools-compile 整族;affected 判不出归属的路径 -> 全部重跑 | 选中批重跑 |
| 行有效 | PASS 或 FAIL 实值、ms_max 不超 55000、RAW 行在、origin 与树 sha 落表 | 该批重跑 |

affected 运行器的推荐做法:留在判据内,且从 def_sha 里拆成独立的 runner 键。依据:(1) 运行器决定 BATCH_RUN 行、candidate 绑定与超时裁决怎么产生,它变了旧结论就不是同一合同下的执行;(2) 把它排除在外等于"读值相同就算过",是缓存结论;(3) 拆键只为让 delta 报出"全部失效的原因是运行器",不改变结论;(4) 真正的办法是让它不变:v0.0.76 的失效来自 22:51 的 1c58acd2ad 在预跑之后才合入,合并前移加静默即可消掉。Q-01 原生 run-batch 补丁必须在预演开始前合入或推迟到公开后。

main 在冻结前又前进:再合得 T3 -> dvledger delta 以 T3 为根干跑 -> needs 为 0 直接冻结到 T3;needs 不超 20 在冻结前补跑;超 20 说明静默被破,按 D 的回退处理。

## D 冻结窗静默协议

| 项 | 规定 |
|---|---|
| 起止 | 预演 M1 开始 -> Y2 推送成功 |
| 禁推 main | tools/affected.cx、tests/run-batch.cx、tools/dvqualify.cx、tools/dvledger.cx、tools/release-*.cx、tools/chain-step.cx、tools/cxlib/**、tests/batches.tsv 等登记表、全部产品输入(src、cx、ext) |
| 冻结前允许 | 其余 tools、tests 的新增(会进 T3,只触发被触及批);docs、plans、prd*.md |
| 冻结后到 Y2 | main 只收发布单子代理的提交;ujs 经 ccapi、cdx2 的合入与其它窗口的推送一律排到 Y2 之后(v0.0.76 的 23:17:20 与 23:19:26 两次即反例) |
| 通知 | 信封三行:静默开始时刻与预计结束、禁推路径清单、解除信号(Y2 提交号);预演开始与 Y2 后各发一次 |
| 机器核对 | 冻结前一刻、X 前、Y1 前、Y2 前各跑一次 git fetch 加 git merge-base --is-ancestor origin/main HEAD;rc 非 0 先合并再继续,不靠推送失败发现 |
| 突破时 | git diff --name-only 旧点 新点 落禁推路径 -> 冻结前:重合并得 T3 并按 C 重判;冻结后:见 G |
| 与 B3 槽位、Q-06 | 链运行期间不起 qualify;预跑与补差期间其它窗口的批次实测静默,非发布作业 load1 高于 4 让路;jobs 保持 2;布局建好后先串行跑一个预热(记 PREPARED,不记结论)再放开并行 |

## E 读回两次回滚的预防

| 回滚 | 预防 | 通过条件 |
|---|---|---|
| 冷编 smoke 超 45 秒 | 冻结前空闲时在发布工作树内把读回要用的工具各实跑一次(chain-step smoke、release-exec --dry、c47-1-readback 的入口),预热记 PREPARED;身份 = 工具闭包 sha,预热后闭包变则重预热;读回前 load1 高于 6 先等(有界 60 秒) | 预热后同命令二次耗时低于 15 秒;读回 smoke 实测低于 45 秒 |
| Y2 非快进 | D 的祖先核对放进 release-exec 的 X、Y1、Y2 三步前;不符先 devsvc git merge 再推 | 三次核对 rc 0 |

注意编译缓存按路径取键(build/pkg-gaps.tsv DEVSVC-DERIVE-FLOW-RUNCACHE),预热必须在读回实际使用的同一工作树路径做,换目录无效。

## F 时间账预测

| 段 | 目标 | 依据 |
|---|---|---|
| 链 | 25 分 | v0.0.76 实测 22:00:31 -> 22:25:12 |
| 预演(并行于链) | 10 到 15 分 | v0.0.76 合并 6 分;棘轮、derive 耗时 UNMEASURED |
| 预跑(T2,冻结前) | 9 到 15 分 | 66 批热态 515 秒实测;加预热 60 秒;首窗 UNKNOWN 重试余量 |
| 冻结快进与推送 | 0.5 分 | 快进无合并 |
| 冻结后哨兵 4 批 | 1 到 2 分;补差上限 20 批约 3 到 5 分 | 热态每批 2 到 15 秒,jobs 2 |
| pack、prepare、证据提交 | 3 到 4 分 | v0.0.76 约 5 分(23:06 -> 23:11),notes 提前起草 |
| X、草稿、CI、Y1、公开 | 5.5 分 | v0.0.76 实测 5 分 15 秒 |
| 读回、Y2 | 1 分 | v0.0.76 单次读回 17 到 34 秒 |
| 冻结到公开合计 | 10 到 13 分,上限 15 分 | 以上相加;预热与静默效果 UNMEASURED |

## G 零件清单

| 零件 | 状态 | 文件与接口 | 验收 | 规模 | 负责 |
|---|---|---|---|---|---|
| qualify、--resume、分段 | 已有 | tools/dvqualify.cx | - | - | - |
| 账三键、delta、ingest | 已有 | tools/dvledger.cx | - | - | - |
| packcheck、prepare、execute | 已有 | release-packcheck、release-run、release-exec | - | - | - |
| head-delta 判非产品路径 | 已有 | tools/freeze-plan.cx head-delta | - | - | - |
| N1 合并预演工具 | 新 | tools/merge-rehearsal.cx:run、check 两个子命令,串 M1 到 M8,打印 REHEARSAL_OK 加 T2 | 夹具:单父合并具名拒绝;产品输入有差具名拒绝;分片数不符具名拒绝 | 中 | cc |
| N2 账加 runner 与 tree 键、origin 列 | 新(原 Q2) | tools/dvledger.cx delta --root T3 --prerun-tree SHA | 改 docs 零 needs;改一个 tools 文件只出对应批;改 affected.cx 全 needs 且原因写 runner | 中 | opus |
| N3 祖先核对进执行门 | 新 | tools/release-exec.cx 的 X、Y1、Y2 前 | 夹具:远端前进时具名拒绝且不回滚公开态 | 小 | cdx |
| N4 读回预热 | 新 | release-run --prewarm 或 N1 的子命令 | 预热后 smoke 低于 15 秒;闭包变则报需重预热 | 小 | cc |
| N5 qualify 预热步与 load 门修正 | 新(Q-06 建议 2、3) | tools/dvqualify.cx | 首窗 UNKNOWN 为 0 | 小 | cdx |
| N6 devsvc git commit 保留 MERGE_HEAD | 新 | tools/devsvc.cx | 合并提交双父 | 小 | 主调度 |

N1 到 N6 都未做完时,v0.0.77 用下面的手工版;N2 缺时 tree 判据人工按 C 表执行并把差异路径清单写进证据。

## H v0.0.77 手工版剧本(版本工作目录内执行,每条命令经 devsvc x,单条不超 55 秒)

现状:main 已领先 v0.0.77 分支 18 个提交(git rev-list --count v0.0.77..main),预演必做。

| 阶段 | 命令 | 通过条件 |
|---|---|---|
| P0 起链前 | release-packcheck slot ROOT;chain-ready;发静默预告信封;确认 Q-01 补丁已合入或已推迟 | VERSION_SLOT OK;READY=yes |
| P1 链运行中 | M1 到 M8;写 notes 草稿;发静默开始信封 | M3 三字段;M4 为空;ratchets 15/0;derive rc 0 |
| P2 链完成 | --version;promote --check;packcheck;记 APE sha | 各 rc 0 |
| P3 预跑 | 等 load1 低于 4(有界 5 分);devsvc qualify --candidate APE --set product-qualification --jobs 2 --out F(布局提交 T2);UNKNOWN 用 --resume F 续 | SUMMARY 行 UNKNOWN=0 INVALID=0;FAIL 只有已登记预期项 |
| P4 修与再合 | 有工具修复或 main 前进:提交 -> 再合得 T3 -> 重核 M3 到 M7 -> 按 C 表选批,qualify --ids 补跑 | 补跑后同 P3 条件 |
| P5 预热与干跑 | 读回工具预热;release-run --prepare;--execute --dry | DRY_GATES_OK;smoke 二次低于 15 秒 |
| P6 冻结前一刻 | git fetch;merge-base --is-ancestor origin/main T3;git diff --name-only 预跑树 T3 | rc 0;差异全在白名单或已补跑 |
| P7 冻结 | main 快进到 T3 并推送;记冻结时刻 | git rev-parse main 等于 T3;推送 rc 0 |
| P8 冻结后 | 哨兵 4 批 qualify --ids c-negative,pp-negative,run-warm,tools-compile-001(冻结提交布局);pack;packcheck ape;证据提交 | 4 批有实值;PACK_APE SAME |
| P9 执行 | 每步前祖先核对;release-run --execute | 七步 PASS;读回 rc 0 |
| P10 收尾 | Y2 推送后发静默解除信封;写 time-ledger | Y2 提交在 origin/main |

## I 风险

| 风险 | 对策 |
|---|---|
| 复用被滥用成缓存结论 | 五条不降;runner 不豁免;账只收带 RAW 的实跑;证据逐行标 origin 与树 sha,发布说明写冻结前 N 批、冻结后 M 批;哨兵 4 批冻结后必实跑 |
| 人工判 tree 白名单出错(N2 未做) | 差异路径清单原样入证据;拿不准的路径一律按"全部重跑"处理 |
| affected --files 选批漏批 | 判不出归属即全跑;batch-product-inputs 声明不全是已知缺口,N2 验收含负控 |
| 冻结后静默被破 | 禁推路径被动:解冻,退回候选点,合并得 T3,补跑被触及批后重新冻结,APE 不变则链不重跑;只动白名单:合并后继续 |
| 冻结后出 FAIL 或 UNKNOWN | 不在冻结后修批;解冻退回 P4 |
| 新工具扩大事故面 | N1、N4 只读加打印,不推送不提交;N3 只加拒绝不加动作;每件带 check 与负控;v0.0.77 先走手工版,N1 用手工版实录做夹具 |
| 预测未实跑 | 预热、jobs、静默效果均 UNMEASURED;v0.0.77 实战后按 time-ledger 改写本表 |
