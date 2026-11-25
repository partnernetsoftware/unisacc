来源:会话临时目录 ~/.claude/jobs/7f46e032/tmp/R27/design.md 拷贝于 2026-11-24
# R27 设计裁定:把"整链止到 F"从 146 分钟压到 15 分钟

日期 2026-11-24。只读取证,未改仓内文件,未动在途进程。标"推断""未核"的句子没有实测支撑。

## 0. 最重要判断

146 分钟里只有约 35 分钟是"main 被别人推动"造成的;83 分钟耗在整链之后才串行起、且被 11 次已知豁免红停住的预跑,26 分钟耗在集成者自己把本地 main 合进候选引出的 SID 修补。所以"链前写推送冻结行"是必要的,但单靠它只能把 146 压到约 110,到不了 15。要到 15,必须同时做三件:F 在起链前就定死(F 等于起链提交,链后不再合 main)、预跑不再因已审豁免行停机、冻结前先把本地积压推空。

R25 把 146 分钟整体归因于"main 前移、四次合 main",这个归因偏了,本报告修正它。

## 1. 问一:0.0.105 链止到 F 的实际时间线

| 区间 | 时长 | 实际在做什么 | 证据 |
|---|---|---|---|
| 12:43:15 -> 12:46:30 | 3 分 | 链止,交接到预跑 | ~/.cache/unisacc/chains/v0.0.105/0bca*/bg.log mtime;~/.cache/unisacc/prerun/0.0.105/ids.txt mtime |
| 12:46:30 -> 12:58:04 | 12 分 | 预跑窗口表已出,等冻结根 frozen-root 建好 | 同目录 windows.tsv 12:46:31、frozen-source.rev 12:58:04(中间做了什么未核) |
| 12:58 -> 13:01 | 3 分 | 材料准备 53 秒,热身首次 rc=124 超 58 秒后重试 32 秒 | release/evidence/0.0.105/prerun-supervise.log 头四行 |
| 13:01 -> 14:06 | 65 分 | 预跑 190 窗口;窗口墙钟合计只有 997 秒(16.6 分),其余约 48 分是停机与重起 | supervise.log:PRERUN_PRODUCT_RED_STOP 11 次、PRERUN_FROZEN_SOURCE 13 次;done.W 文件按 10 分钟分布 33/43/14/30/33/20/17 |
| 14:06 -> 14:11 | 5 分 | 第一次合 main:合的是未推送的本地 main(7cf95861ee,tools.cxlib 提到 0.4.3) | git log e777557aad;会话时间线 06:07 "freeze pending neon's final step; agent to merge" |
| 14:11 -> 14:32 | 21 分 | 合入后 exec/facts/env-semantics.tsv 变了,SID 门红;裁定零重编,把表迁到 tests/,重取包号 | fa21f218cd(改 9 个文件含 build/tlib-packages.tsv);时间线 06:16 |
| 14:31:18 -> 14:35 | 4 分 | 第一次不明推送把 19 个本地提交推上 origin/main,被迫第二次合 | 8aff677899、8e7f71d8c6;记忆宫殿第 35 行 |
| 14:35 -> 14:46 | 11 分 | 等提交门回归修复 7bb814f5f9(E7),第三次合 | 21c6f64441;时间线 06:36 到 06:43 |
| 14:46 -> 15:09 | 23 分 | 严格证据分段重跑(14:56);15:02:08 第二次不明推送,第四次合得 F | strict-W0901 到 W0906 的 mtime;414a024ea5 |

归类:预跑相关 83 分(57%);自愿合本地 main 加 SID 修补 26 分(18%);不明推送与其返工约 37 分(25%)。重取包号只在 fa21f218cd 出现一次,不是独立瓶颈;推送审计与 gatekeeper 不在这 146 分钟的关键路径上(审计钩子是事后加的)。

11 次停机全部是 netregen-module 与 netregen-opt-module 两族的指纹红,正是 expected-fail.tsv 里已审的那 12 行。prerun-drive.cx:1227 与 :2101 对 product-qualification 的 FAIL 无条件停,expected-fail 只在 g1-dry 阶段才读(:10)。同一个已知红停 11 次,每次等代理发现并重起,这是本段最大的单项浪费。

## 2. 问二:链前冻结行加"链期只收集成者提交"能否压到 15 分钟

不能单独做到。逐项对照:

(1) 冻结行能消掉的:两次不明推送及返工,约 37 分钟。前提是 tools/prepush.cx 已装且过期行不当解冻(D7 已落)。git push --no-verify 仍可绕,服务端无保护,这是残余风险,所以 X 之前仍须 git fetch 核 origin/main 等于冻结点。

(2) 冻结行消不掉的:第一次合 main 是集成者主动合"未推送的本地 main",冻结门只管推送,管不到。要另立规则:起链后候选分支不得再合任何 main(本地或远端),F 定义为起链提交本身;只有阻塞修复例外,且例外即重起链。0.0.105 的 F 是合并提交 414a024ea5 而不是链跑的 0bca20928b,靠"SID 相等"才成立,这条路径本身就是返工源。

(3) 冻结行消不掉的:预跑 83 分钟。

立场:目标应改写。F 定为起链提交后,"链止到 F"恒为 0,这个指标会失去意义;真正要量的是"链止到 X 提交"。0.0.105 是 12:43 -> 15:24,161 分钟。可达下限(推断):预跑窗口墙钟 17 分加 pack、warm、dry 实测 12 分(15:12 -> 15:24),串行约 30 分钟;预跑里不依赖候选产品的窗口提到链期并行后可到 20 分钟上下。建议验收线:链止到 X 不超过 30 分钟,硬线 45 分钟。

代价:main 推送被挡的时长等于起链到 release-post,按新目标约 30(链)加 30(链止到 X)加 24(X 到公开)加 10(解冻)约 95 分钟每版。三版一天就是近 5 小时。会话里 fable 路线图已量过:main 提交率从每小时 18 笔掉到 5.1 笔。

补偿办法,按优先序:
(1) 冻结前推空积压:写冻结行之前,本地 main 必须等于 origin/main(未推提交数为 0)。0.0.105 冻结时本地压着 13 到 19 个提交,这堆东西既引诱合并又被误推。
(2) 冻结期开发窗口照常在共享检出本地提交,不推;发布从版本工作目录做,HEAD 只含冻结点的 origin/main。解冻口由集成者一次合入(0.0.105 的 9e7c326f16、323b70985c 已是这个做法)。
(3) 冻结期本地提交不得动 build/tlib-packages.tsv、layer-api、iface-hash:带取号的件排到解冻口现取。7cf95861ee 就是冻结期取了 0.4.3,随后被候选合入又重取。
(4) 长期解法仍是分支直发(改 gate_main 语义,服务端开保护),它被 gatekeeper 两轮 BLOCK。本方案不依赖它,也不替代它。

## 3. 问三:起链前一次性合 main 的判据与取号预演

起链前置顺序(全部为真才起,顺序不可换):
(1) 本地 main 等于 origin/main:`git rev-list --count origin/main..main` 为 0。
(2) 写冻结行并推送(build/freeze.tsv 是被跟踪文件,写行本身是一次 main 提交;截止取起链后 180 分钟,越线先续期)。
(3) 版本分支合 origin/main,`git rev-list --count v0.0.NN..origin/main` 为 0。
(4) 在分支上重取 tools.cxlib 号、layer-api、iface-hash。因为 main 已冻,此刻取的号就是落 main 时的号,链后 F 快进上 main 无需再取。这就是"预先模拟":不是模拟,而是把取号时点移到冻结之后、起链之前。
(5) `sh ./unisacc.com tools/premerge-union.cx -- --base origin/main --branch v0.0.NN` 末行 verdict=PASS conflict=0 derive_drift=0(chain-ready 第 22 项已含,合完后它退化为恒等检查,仍保留作证)。
(6) chain-ready READY=yes,起链;同时起预跑。

落后提交数上限:起链时必须为 0,没有容差。开发期的同步判据另设:落后超过 10 个提交,或 main 上出现任何动候选键路径(src、cx、exec、unisa、ext)或共享表(tests/batches.tsv、tests/coverage.tsv、build/tlib-packages.tsv)的提交,当天合一次。0.0.103 在 R25 取证时落后 12,已越线。10 这个数是经验值(推断),可证伪条件见第 6 节。

链后冲突的剩余来源只有两个:有人绕门推送(靠第 2 节的 fetch 核对抓),以及阻塞修复(按规则重起链,不修补)。

## 4. 问四:切片

| 刀 | 谁 | 改哪些文件 | 验收命令与预期 | 收益估计 |
|---|---|---|---|---|
| S1 预跑不为已审豁免行停机 | agy-flow 写,gatekeeper 审 | tools/prerun-drive.cx(run 与 supervise 接 --expected-fail,命中已审且未过期的行记 PRERUN_EXPECTED_RED 后继续,strict 行仍写 FAIL)、tools/check-prerun-drive.cx | check-prerun-drive 新增三例:已审行不停且 strict 仍为 FAIL;未审红照停;过期行照停。变异:去掉过期判断须转红 | 约 48 分钟每版 |
| S2 冻结前置与 F 等于起链提交 | cc(集成者) | tools/chain-ready.cx 加一项 23-freeze-armed(冻结行在且未过期、本地 main 等于 origin/main、分支落后 0);tools/release-exec.cx 在 step_x 前比对 HEAD 与 chain-run.identity 的 rev,不等则 REFUSE RELEASE_EXEC_F_NOT_CHAIN_REV,显式 --post-chain-merge REASON 才放行并写进 INDEX | check-chain-ready、check-release-exec 各加红绿夹具;0.0.103 实发时 INDEX 里无 post-chain-merge 行 | 约 26 加 37 分钟每版 |
| S3 冻结期取号拒绝 | cdx | tools/commit-own.cx(冻结生效且非冻结主时,拒绝提交 build/tlib-packages.tsv、build/layer-api、iface-hash,具名 COMMIT_OWN_FREEZE_TLIB) | check-commit-own 新夹具红绿;解冻后同一提交通过 | 防复发,不直接省时 |
| S4 预跑前移 | agy-flow | tools/prerun-drive.cx、tools/bg-candidate.cx:不依赖候选产品的窗口在链期起 | 链止时 done.W 数不少于非产品窗口数;链 KILL 仍为 0 | 约 10 分钟每版(推断) |

最小的一刀是 S1:只动一个工具加它的检查,不碰冻结门与发布门,收益最大,且用 0.0.105 的现成目录可离线重放验证(~/.cache/unisacc/prerun/0.0.105 的 strict 文件加 release/evidence/0.0.105/expected-fail.tsv)。S2 的规则部分今天就能人工执行(写进记忆宫殿),工具部分随后。

S1 不违反铁律的理由:不删断言、不改判定,红仍记 FAIL 进 strict 并在发布说明披露;改的只是"遇到已经人工审过并登记到期的红,是否要停下来等人"。未核:run 模式下 expected-fail 的版本键与到期比较语义(R25 已指出按数值比较在乱序版本号下会误判),S1 必须先读这段源码再动手。

砍与延后:S4 延后到 S1、S2 经一次实发验证后再做,链期加载已三次触发 KILL,不宜同时加变量。分支直发不在本轮。不新造"发布车道专用检出"之类的隔离设施,现有版本工作目录已够。

## 5. 风险

(1) S1 被滥用成"先登记豁免再绿":防线是豁免行必须带审阅理由与到期,过期照停,且 g1-dry 阶段的对账不变。
(2) F 等于起链提交后,链后发现发布工具缺陷就没法靠合 main 带入修复:这是有意的,列车工具链期本就冻结;真要修,走 --post-chain-merge 留痕并核 SID,次数计入版本复盘。
(3) 冻结前推空积压会把未审的工具提交赶在链前上 main:应反过来,未过审的不推、留本地,但第 3 节 (1) 就不成立。取舍:前置 (1) 改为"本地未推提交不含候选键路径与取号文件",允许纯工具积压留本地。这一条我倾向宽松版,理由是 0.0.105 的 19 个积压里只有 7cf95861ee 一个真正引发返工(推断,未逐个核)。
(4) 冻结行是被跟踪文件,过期即全员拒推:发布越线忘续期会把集成者自己也挡住(prepush.cx:18 写明过期行对冻结主也不放行)。截止要留足,并把续期写进发布步骤。

## 6. 我最可能错在哪里,以及最便宜的证伪

(1) 预跑停机的 48 分钟也许不是"等人重起",而是每次重起后重建冻结根或静默窗让路。证伪:读 supervise.log 里每个 PRERUN_STOP 与下一个 PRERUN_FROZEN_SOURCE 之间的间隔(日志无时间戳,须对 strict-W 文件 mtime),5 分钟可做完。若间隔短而总时长仍长,S1 收益要下调,S4 上调。
(2) 预跑可能大部分依赖候选产品,前移空间小。证伪:windows.tsv 里按域统计 product-qualification 占比,一条 .cx 命令。
(3) 集成者第一次合本地 main 也许有我没看到的硬理由(例如发布工具修复必须进 F)。证伪:问 agy-flow 当时为何合 7cf95861ee,或看 e777557aad 之后 release-exec 用到的工具是否在该合并里改过。若确有硬理由,S2 的 --post-chain-merge 会变成常态,规则要改成"链前先落发布工具修复"。
(4) "链止到 X 30 分钟"的下限按 0.0.105 的 pack 到 X 12 分钟估,那一段当时无争用;三版连发叠加推测链时可能翻倍。证伪:0.0.103 实发时在 INDEX 里记链止、预跑止、pack、X 四个时间点,一版就见分晓。
(5) 落后上限 10 是拍的。证伪:用 git log 回算最近五版每次合 main 的落后数与合并后返工提交数,看有无阈值效应。
