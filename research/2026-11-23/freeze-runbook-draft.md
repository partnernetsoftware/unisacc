# 冻结到公开运行手册(0.0.94 起用)草案

状态:草案,未执行,fable 2026-11-23 起草,待 agy-flow 核实"待核"项并演练。标 UNMEASURED 的数字未实测,标"推断"的是按 0.0.93 外推,标"待核"的是未读到源码依据。

T0 定义:版本分支合入 main 的合并提交推送完成的时刻。总预算:T0 到公开 120 分钟;本手册一次通过目标 105 分钟,余量 15 分钟。硬规则:余量只够吸收一次窗口级重跑,不够吸收一次重冻结。T0 之后出现需要改 main 的红灯,默认动作是中止并回到 T-0 重排,不是赶工。

## 一、时间线(K 为关键路径,P 为并行泳道)

| 编号 | 泳道 | 动作 | 负责 | 预算 |
|---|---|---|---|---|
| S0 | K | 合并 v0.0.94 入 main,切版本槽,推送 | 集成者(唯一写 main) | 5 分 |
| S1 | K | chain-ready 全量 | 集成者 | 3 分(UNMEASURED) |
| S2 | K | 整链 bg-candidate | 集成者起,Monitor 订终态 | 30 分(推断) |
| S3 | K | 预跑 P0 计划加 P1 运行 | 预跑窗口 | 30 分(wall_ms 1689560 约 28 分) |
| S3b | P | ci-early 取 candidate-native 样本 | CI 窗口,与 S3 并行 | 10 分 |
| S3c | P | rollback.txt 与 notes-0.0.94.md 填身份 | 文档窗口,与 S3 并行 | 10 分 |
| S4 | K | P2 merge 加 P3 archive | 预跑窗口 | 3 分 |
| S5 | K | 0a pack 加 0b warm | 集成者 | 5 分(0.0.93 实测 pack 到 warm 同一分钟) |
| S6 | K | 1 prep 加 2 stage-a | 集成者 | 3 分 |
| S7 | K | 3 stage-commit(X 提交,含 rollback/decision/notes) | 集成者 | 3 分(0.0.93 实测 X 后等文档 11 分) |
| S8 | K | 4 stage-dry | 集成者 | 3 分 |
| S9 | K | 5 execute(X 核验、草稿、CI、Y1、公开、读回、Y2) | 集成者,devsvc x --bg | 12 分(0.0.93 实测 8 分,CI 7 分) |
| S10 | K | 公开后记账与 Root 升级 | 集成者 | 8 分(0.0.93 实测 6 分) |

合计 105 分。各步命令、预期行、门、失败处置的细节见下;完整原文由 fable 输出,关键点如下。

- S0:git merge --no-ff v0.0.94;改 release/candidate.tsv 为 0.0.94 行;git commit 明确路径;推送。门:chain-ready --only 11-version-slot --version 0.0.94 为 PASS。失败:冲突说明 T-1 的冻结合并预演没做,中止回 T-1,不在 T0 之后解冲突。
- S1:sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.94,预期末行 READY yes failed=none;除 7-recipe、9-ledger 两项手工 UNMEASURED 外全部 PASS;--fast 的 SKIPPED 不算。全量超过 55 秒就按 --only 分项并行跑。
- S2:tools/bg-candidate.cx start;Monitor 订终态事件不轮询;门:链 DONE,APE sha256 与 build.json product_digest 一致;失败先读发码行,同一失败两次即停手。
- S3:prerun-drive plan 与 run(PRERUN_APE、PRERUN_IDS、PRERUN_TIMES、PRERUN_PRIORITY、PRERUN_DIR 用绝对路径);门:strict 表 0 FAIL,UNKNOWN 只允许出现在 expected-unmeasured.tsv 已列项;W0000 再现 rc=124 按确定性缺陷停手。
- S3b:让 osx/win 的调用号与信号常量在冻结 CI 前拿到一次真实样本(surf-probe 四项),红灯在这里比在 S9 便宜约 40 分钟。
- S5:release-run --pack 与 release-exec --warm 分开跑(合在 prep 里有 55 秒超时风险)。
- S6 至 S8:release-chain run --inputs --state 绝对路径 --layout;X 由操作者提交推送,S8 必须在已提交的树上重跑。
- S9:devsvc x --bg 起 release-exec --execute;公开前失败草稿保持私有;CI 红重做要清 ci_run_id、改名旧证据、删陈旧草稿;MAIN_ADVANCED 自己合并 origin/main 再 --from-step N;公开后门失败自动执行 rollback.txt 第 (1) 条;Y1 到 Y2 之间不得改 evidence.tsv 与 decision.txt;CI 轮询上限 25 分钟。
- 超限规则:任一步超过预算的 1.5 倍,先核真实 PID/PGID,再决定拆分或中止;累计 100 分钟仍未进 S9,本次冻结作废。

## 二、冻结前置清单

T-1 天(并集树或私有副本上做,不动 main):
1. premerge-union 全绿(chain-ready 22 项)。UNMEASURED。
2. 冻结合并预演:私有副本合并,列冲突,跑 scrub-home-paths 与 no-home-path。UNMEASURED。
3. 并集树上跑一遍严格预跑(0.0.93 的 F、F2 都属这一步能拦的确定性红)。UNMEASURED。
4. 提交门缺陷清零:coreapi 漏拦已修;freeze_gate 自我解冻 d40ee7db5f 已修;全局上限棘轮 dac97db90e;隔离脚本 493f59eca8;提取去重 51efd3e7d7 已落,待 gatekeeper 复核通过。
5. ci-early 拿到 osx 四项探针真实样本(2f165f193d)。现状未拿到。
6. plans/v0.0.94.md 的 freeze_at 仍 TBD;l0-posix2 批次 REGISTERED_NORUN,需产品重建后补跑。
7. build/version-seq.tsv 0.0.94 行状态仍为"建议"、主题仍含 sy_proc_capture,需按收缩后范围改写。
8. release/notes-0.0.94.md 不存在;rollback.txt 与 decision.txt 仍是 0.0.93 内容,需备好 0.0.94 模板。
9. 共享检出有未提交改动(build/links.tsv、tools/check-ratchets.cx、tools/tier-census.cx、team-scoreboard.md 等),冻结前必须落定或移走。

T-0 小时 chain-ready 逐项:11-version-slot 实测 FAIL(candidate.tsv 仍是 0.0.93 行,S0 修),21-verseq-gen PASS(checks=164),其余 UNMEASURED;chain-ready --fast 全量在 55 秒内无输出,T-0 之前必须拿到一次全量耗时,否则 S1 的 3 分钟预算无依据。

## 三、链期间写入规则与红灯规则

禁写(T0 到 Y2,所有窗口):产品输入(src/、cx/、ext/、exec/、include/、seed/、kernel/);harness(tests/、被链/预跑/release-* 读取的 tools/ 及其 check、tests/batches.tsv、coverage.tsv、build/ 派生表);release/ 下除集成者外任何人不写。仅集成者写 main,只写 release/ 与 prd_mempalace.md;其他车道(ujs、apps/csih)照常提交自己路径,但每次推进 origin/main 都会让 release-exec 的 MAIN_ANCESTOR 门要求再合一次(0.0.93 链期间 main 上有二十余次他车道提交),建议 S7 到 S9 这 20 分钟全仓静默推送。写窗口锁:待核,本次未在 tools/devsvc.cx 中找到具名的写窗口锁子命令,若确无则提为 devsvc 缺口,链期间暂由提交门 freeze_gate 兜底。

红灯:判瞬态之前先读发码行,rc 2 与 REFUSE 默认确定性;同一失败连续两次即停手;需要改产品输入才能修的红灯使 T0 作废回 T-1;只需改测试或 harness 的红灯(C 类)允许一次,记为 F 提交并写入 rollback.txt,第二次即作废重排;终止进程只按真实 PID/PGID,先在账本记一行。

## 四、公开后动作

1. INDEX 封尾 exec-7-Y2-done 行随下一个提交带上。2. release-chain mark-version-seq --version 0.0.94 --commit --push。3. backup-evidence 再 --verify。4. version-metrics-gen --write、day-metrics --write。5. Root 升级:先确认 0.0.93 只读备份在 .unisacc-root-backups/<sha256>/ 带 manifest.tsv,再装已验 APE。6. release/state.tsv 现停在 0.0.86,是否现行待核。7. 扫除:删 .claude/worktrees/v0.0.94,分支保留到 0.0.95 公开。8. prd_mempalace.md 原位更新。9. 版本序顺延(csr 演练清单):改 version-seq.tsv 后跑 verseq-gen --check,四处口径 fail=0 才算完成。10. 合并顺序:先合 94 入 main;再把 main 合进 v0.0.96,重跑 check-proc-lifecycle;0.0.95 同理先合 main 再验(version-seq 的 0.0.96 行主题与"进程生命周期承接版"不一致,先定号再合)。11. 解冻在 Root 升级之后。

## 五、发布说明如实口径

不得声称:osx/win 调用号与信号常量经实机运行验证(没有 CI 样本的项写"仅读码");全批资格 QUALIFIED(s9 UNKNOWN 时只能写 NOT_QUALIFIED);Z3/Z3b/Z5/Z6 与前端三项在本版交付(已撤出 3970fc6d5a);pipe2 在 osx 上原子(是 pipe 加 fcntl 非原子实现)。须写明已知缺口:U31/U32 已迁 v0.0.96,U33 改回 OPEN(本次未在 build/*.tsv 搜到这三个编号,引用位置待核);l0-posix2 若到冻结仍未实跑写 REGISTERED_NORUN;win 停止信号表达法待裁,osx/win 的 sy_child_subreaper 为具名拒绝 -ENOSYS;严格预跑中的 UNKNOWN 与 expected-fail 逐条列出;0.0.95 若同期冻结,其未判定项不并入本版说明。

## 六、风险与本手册最可能错在哪里

风险:确定性红仍在 T0 之后出现(0.0.93 两次都是,22-premerge-union 是新门,尚无真实拦截记录);链期间他车道推进 main 触发 MAIN_ADVANCED 重合并;osx 探针在冻结 CI 才首次真跑并变红(代价约 40 分钟);提交门复核未过前门本身可能误放或误拦;chain-ready 全量耗时未知。

最可能错的地方:S2 整链 30 分钟是外推,没有读整链日志(证伪:读最近一次 bg-candidate 起止时间戳);0.0.93 的 125 分钟主要归因于 F、F2 是推断(证伪:对 prerun-supervise.log 按 PRERUN_WINDOW 的 wall_ms 求和);T0 口径可能与 fable 报告不同(若从 F2 起算 0.0.93 只用了 47 分钟);写窗口锁可能并不存在;余量 15 分钟的"一次窗口级重跑"假设单窗 58 秒。

最便宜的总体证伪:在并集树上按本手册 S1 至 S8 做一次不公开的全程计时演练(S9 用 --dry),拿到每步真实耗时后再定预算;演练超 105 分钟,预算表作废重排。

## 取证发现的三处不一致(需先定)

1. release-chain plan 不带 --inputs/--state/--layout 直接拒绝(REFUSE CHAIN_USAGE),无参只读形态不存在;相位取自源码头注释与 show_prerun。
2. tools/devsvc.cx 里没有具名的 lock/claim 子命令,build/*.tsv 里也搜不到 U31/U32/U33。
3. build/version-seq.tsv 的 0.0.96 行主题是"执行核每步成本",与"Z3/Z5/Z6 迁入 v0.0.96"不一致;0.0.94 行状态仍为"建议"。
