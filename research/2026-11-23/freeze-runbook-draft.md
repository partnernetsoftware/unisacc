# 冻结到公开运行手册(0.0.94 起用)草案

状态:草案,未执行,fable 2026-11-23 起草,待 agy-flow 核实"待核"项并演练。标 UNMEASURED 的数字未实测,标"推断"的是按 0.0.93 外推,标"待核"的是未读到源码依据。

T0 定义:版本分支合入 main 的合并提交推送完成的时刻。总预算:T0 到公开 120 分钟;本手册一次通过目标 105 分钟,余量 15 分钟。硬规则:余量只够吸收一次窗口级重跑,不够吸收一次重冻结。T0 之后出现需要改 main 的红灯,默认动作是中止并回到 T-0 重排,不是赶工。

## 一、时间线(K 为关键路径,P 为并行泳道)

| 编号 | 泳道 | 动作 | 负责 | 预算 |
|---|---|---|---|---|
| S0 | K | 合并 v0.0.94 入 main,切版本槽,推送 | 集成者(唯一写 main) | 5 分 |
| S1 | K | chain-ready 分批(`--only`) | 集成者 | 3 分(空闲 `load<8` 分批实测 2-3.4 分; 单命令全量必超 55s) |
| S2 | K | 整链 bg-candidate | 集成者起,Monitor 订终态 | 30 分(0.0.91..0.0.93 三版实测 27.1-29.3 分) |
| S3 | K | 预跑 P0 计划加 P1 运行(`--window-lanes 3 --normal-jobs 8`) | 预跑窗口 | 38 分(`load<8` 静默下预估 36-41 分; 0.0.93 `lanes=4,j=4` 去重实测 49.1 分) |
| S3b | P | ci-early 取 candidate-native 样本 | CI 窗口,与 S3 并行 | 10 分 |
| S3c | P | rollback.txt 与 notes-0.0.94.md 填身份 | 文档窗口,与 S3 并行 | 10 分 |
| S4 | K | P2 merge 加 P3 archive | 预跑窗口 | 3 分 |
| S5 | K | 0a pack 加 0b warm | 集成者 | 5 分(0.0.93 实测 pack 到 warm 同一分钟) |
| S6 | K | 1 prep 加 2 stage-a | 集成者 | 3 分 |
| S7 | K | 3 stage-commit(X 提交,含 rollback/decision/notes) | 集成者 | 3 分(0.0.93 实测 X 后等文档 11 分) |
| S8 | K | 4 stage-dry | 集成者 | 3 分 |
| S9 | K | 5 execute(X 核验、草稿、CI、Y1、公开、读回、Y2) | 集成者,devsvc x --bg | 12 分(0.0.93 实测 8 分,CI 7 分) |
| S10 | K | 公开后记账与 Root 升级 | 集成者 | 8 分(0.0.93 实测 6 分) |

合计 111 分(T0 到 S9 公开 103 分 <= 120 分红线,余量 17 分; S10 公开后 8 分)。各步命令、预期行、门、失败处置的细节见下;完整原文由 fable 输出,关键点如下。

- S0:git merge --no-ff v0.0.94;改 release/candidate.tsv 为 0.0.94 行;git commit 明确路径;推送。门:chain-ready --only 11-version-slot --version 0.0.94 为 PASS。失败:冲突说明 T-1 的冻结合并预演没做,中止回 T-1,不在 T0 之后解冲突。大合并(1000+ 文件)在冻结窗口内触及 genall(20s 上限)或 derive 的 GATE_TIMEOUT 时,集成者(须匹配 build/freeze.tsv 第 3 列 owner)显式带 `UNISACC_INTEGRATOR=<owner> UNISACC_GATE_WAIVE="derive:s0-merge-0.0.94,genall:s0-merge-0.0.94"` 放行一次并双写记账,落提交后立即在 S1 前分门执行核销(每门单次 <=55 秒):`sh ./unisacc.com tools/precommit.cx -- --verify-deferred --gate derive` 与 `sh ./unisacc.com tools/precommit.cx -- --verify-deferred --gate genall`,追加绑定合并后 `git write-tree` 输入身份的 `VERIFIED` 哈希链行,确保 S1 `chain-ready` 的 `1-rev` 通过。
- S1:sh ./unisacc.com tools/chain-ready.cx -- --version 0.0.94,预期末行 READY yes failed=none;除 7-recipe、9-ledger 两项手工 UNMEASURED 外全部 PASS;--fast 的 SKIPPED 不算。全量超过 55 秒就按 --only 分项并行跑。环境前置:PPEQ 变量用命令行 `--models-a`/`--models-b` 传,不要在持久 shell 里导出 `UNISACC_PPEQ_PKG_*`(`chain-ready` 的 `10-envhygiene` 会点名拒绝所有未登记 `UNISACC_*` 变量,见 `152f8f58e9`)。
- S2:tools/bg-candidate.cx start;Monitor 订终态事件不轮询;门:链 DONE,APE sha256 与 build.json product_digest 一致;失败先读发码行,同一失败两次即停手。
- S3:推荐配置 `--window-lanes 3 --normal-jobs 8`(已定稿,经 0.0.93 最重 7/13 个 `normal` 窗 58 批试点验证 `PARTIAL`: 0/7 次 `rc=124`、最长窗 48.7s 留 9.3s 余量、实测并发加速 5.10x,剩余 `W0025..W0030` 6 窗断点留于 `~/.cache/unisacc/prerun-pilot-j8`,不阻塞冻结; 全静默 `load < 4` 时可用 `--window-lanes 4 --normal-jobs 8`): 优先用 `sh ./unisacc.com tools/release-chain.cx -- prerun-inputs ...` 生成冻结输入与推荐命令,或直接 `sh ./unisacc.com tools/prerun-drive.cx -- plan --ids-file "${PRERUN_IDS:?}" --times-file "${PRERUN_TIMES:?}" --priority-file "${PRERUN_PRIORITY:?}" --out "${PRERUN_DIR:?}" --normal-jobs 8 --window-lanes 3` 后跑 `sh ./unisacc.com tools/prerun-drive.cx -- run --root . --candidate "${PRERUN_APE:?}" --out "${PRERUN_DIR:?}" --resume --max-jobs 8 --window-lanes 3 --keep-going`(全部路径绝对路径)。护栏: 起跑前实测 `uptime` 1 分钟负载 `< 8`; 运行中若负载超过 12,重跑/续跑立即降为 `--window-lanes 2 --max-jobs 6`(防把边缘 58s 窗口推成 `rc=124`)。门: strict 表 0 FAIL,UNKNOWN 只允许出现在 expected-unmeasured.tsv 已列项; W0000 再现 rc=124 按确定性缺陷停手。
- S3b:让 osx/win 的调用号与信号常量在冻结 CI 前拿到一次真实样本(surf-probe 四项),红灯在这里比在 S9 便宜约 40 分钟。
- S5:release-run --pack 与 release-exec --warm 分开跑(合在 prep 里有 55 秒超时风险)。
- S6 至 S8:release-chain run --inputs --state 绝对路径 --layout;X 由操作者提交推送,若 S7 提交曾用 `UNISACC_GATE_WAIVE`,须先在已提交树上跑 `sh ./unisacc.com tools/precommit.cx -- --verify-deferred --gate <gate>` 核销并确认 `chain-ready --only 1-rev` 为 PASS,再在已提交的树上重跑 S8 `stage-dry`。
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

禁写(T0 到 Y2,所有窗口):产品输入(src/、cx/、ext/、exec/、include/、seed/、kernel/);harness(tests/、被链/预跑/release-* 读取的 tools/ 及其 check、tests/batches.tsv、coverage.tsv、build/ 派生表);release/ 下除集成者外任何人不写。仅集成者写 main,只写 release/ 与 prd_mempalace.md;其他车道(ujs、apps/csih)照常提交自己路径,但每次推进 origin/main 都会让 release-exec 的 MAIN_ANCESTOR 门要求再合一次(0.0.93 链期间 main 上有二十余次他车道提交),建议 S7 到 S9 这 20 分钟全仓静默推送。写窗口锁与提交门分工(已落地): `devsvc lock acquire freeze --owner <owner> --ttl 7200` (定义于 `tools/devsvc-repo.tsv` `lock freeze`, 写域 `src/,cx/,ext/,include/,seed/,exec/,kernel/,weights/,tests/,release/,build/freeze.tsv`, 全仓及 worktree 共享同一锁, 缺省 `ttl=7200` 且上限 `7200`) 管 **`devsvc` 写入通道** (`devsvc git commit`/`add`/`merge`/`pull`/`push`/`f write`/`f edit`/`flow`/`register`/`derive --write`/`doc`/`run --write`, 优先直读 `locks.tsv` 并在锁不可读或冻结期服务不可达时 fail-closed, 非持锁属主立即拒绝 `LOCK_HELD freeze owner=<owner>`, 到期自动恢复或经 `devsvc lock release freeze --owner <owner>` 释放; 合法 `DEVSVC_OWNER` 或 `--owner` 经 `devsvc git commit` 提交时自动追加小写 `X-Unisacc-Window: <owner>` trailer); `build/freeze.tsv` (`tools/precommit.cx` `freeze_gate`) 管 **`git` 提交通道** (含直接 `git commit`)。两者不重复判据, 并存时以更严者为准(任一拒绝即阻断)。防误操作边界与冻结日使用约定: `DEVSVC_OWNER` 可由任意窗口设置, `devsvc freeze` 锁仅防误操作不防恶意冒充, 冻结期真正的强拦截以 `precommit` 的 `build/freeze.tsv` 为准; 集成者全程使用同一小写窗名(`DEVSVC_OWNER`、`lock acquire --owner`、`build/freeze.tsv` 第 3 列三处完全一致), 显式带 `--ttl 7200` 并在运行超过 90 分钟前由同属主重跑 `devsvc lock acquire freeze --owner <owner> --ttl 7200` 续期(7200s 相对 111 分钟预算余 9 分钟); `devsvc git merge` 已支持 `--no-ff` 且默认不 fast-forward(确保版本合入必留双父合并提交 `ff=0`)。

红灯:判瞬态之前先读发码行,rc 2 与 REFUSE 默认确定性;同一失败连续两次即停手;需要改产品输入才能修的红灯使 T0 作废回 T-1;只需改测试或 harness 的红灯(C 类)允许一次,记为 F 提交并写入 rollback.txt,第二次即作废重排;终止进程只按真实 PID/PGID,先在账本记一行。

负载静默(2026-11-23 负载实测新增):从 S1 到 S9 全程,除集成者外其余所有窗口暂停重检查(derive、ratchets、批量 tests、构造、全量 premerge-union),仅保留只读查阅与轻编辑;启动 S2(整链)与 S3(严格预跑)前须实测 `uptime` 1 分钟负载 `< 8`(load >= 8 时先等或请相关窗口收尾,禁止在高负载下硬起 S2/S3 撞 30s/55s 超时线)。静默开始(S1 前)与静默结束(S9 公开读回完成或本次冻结中止后)均由集成者统一经 `mux` 广播宣布,未收到结束广播前不得恢复重检查。

延迟门双锚点哈希链账本规范(C3/P2 合同):
- 双锚点路径:工作树可见锚点 A 为 `<top>/build/deferred-gates.tsv`,Git 内部锚点 B 为 `<git-common-dir>/unisacc-deferred-gates.tsv`;两处按工作树根 `top` 互锁,单删、截断、改为 0 字节、加 `#` 注释、插入空行、`chmod 000`、换成目录或软链/悬空链接均由 `chain-ready` (`1-rev`) 直接判 FAIL。
- 9 列固定格式(TAB 分隔,禁止注释与空行):
  `rec_type<TAB>timestamp_s<TAB>top<TAB>gate<TAB>reason<TAB>tree_sha<TAB>head_sha<TAB>detail<TAB>chain_sha256`
  其中 `rec_type` 仅限 `DEFERRED` 或 `VERIFIED`;按同一 `top` 独立串联哈希链,创世值 `DEFERRED_GENESIS_SHA` 为 64 个 `'0'` (`0000000000000000000000000000000000000000000000000000000000000000`),每行 `chain_sha256 = sha256_hex("<prev_chain_sha256_for_top>\t" + 前8列原文)`。
- 核销命令:`sh ./unisacc.com tools/precommit.cx -- --verify-deferred [--gate derive|genall|tlibfrozen|lint|ratchets]`(每门单次 `<= 55` 秒),实跑对应门通过后取当前 `git write-tree` 写入 `VERIFIED` 行(`detail` 绑定 `verified_deferred_tree=<orig_tree>`);`chain-ready` 仅按当前 `--repo` 的 `top` 校验全部 `DEFERRED` 均已被后续同 `top`、同 `gate` 且 `tree_sha` 匹配当前树的 `VERIFIED` 行核销。

## 四、公开后动作

1. INDEX 封尾 `exec-7-Y2-done` 行随下一个提交带上(0.0.93 已实测验证在 `release/evidence/0.0.93/INDEX.tsv`)。
2. `release-chain mark-version-seq --version 0.0.94 --dry`(先只读预演,核将改行与 `verseq-gen --check`),确认无误后再跑 `release-chain mark-version-seq --version 0.0.94 --commit --push`。
3. `release-chain backup-evidence --version 0.0.94 ...` 再 `--verify`(校验时展开行首 `~/` 为 `$HOME`,0.0.93 实测 `BACKUP_VERIFY PASS files=7`)。
4. 先 `tools/version-metrics-gen.cx -- --write-history`,再 `tools/version-metrics-gen.cx -- --write`,最后 `tools/day-metrics.cx -- --write`(不可漏 `--write-history`,见 `tools/release-chain.cx:1475`)。
5. **Root 升级(S10 强前置)**: 升级本机 Root 前必须先有合规的 0.0.93 备份槽(`.unisacc-root-backups/<sha256>/` 带 `manifest.tsv` 且登记在 `.unisacc-root-backups/index.tsv`),并通过 `sh ./unisacc.com tools/install-ape.cx -- . --rollback` 干跑输出 `INSTALL_APE ROLLBACK_DRYRUN <0.0.93-sha256>`(现状 0.0.93 缺合规槽,历史 `0.0.86-public..0.0.92-public` 仅为手工目录、不在 `index.tsv`,需先建合规槽再升 0.0.94); 确认后装已验 0.0.94 APE。
6. `release/state.tsv`: 已退役(仅 `tools/release-state.cx` 读写,停在 0.0.86,`release-chain`/`release-exec`/`chain-ready` 均不读取,不纳入公开后动作)。
7. 扫除: S10 用 `git worktree remove` 删 `.claude/worktrees/v0.0.94`(分支 `v0.0.94` 保留到 0.0.95 公开); 现存已公开旧工作树 `v0.0.93`、`v0.0.88`、`v0.0.89-old` 一并在 S10 盘点清理,其中 `v0.0.88` 与 `v0.0.89-old` 有未提交改动,清理前必须先问各自所有者。
8. `prd_mempalace.md` 原位更新(手工动作,写明已公开版本、F/APE/CI/Y1/Y2 与本机 Root 版本)。
9. 版本序顺延(csr 演练清单): 改 `build/version-seq.tsv` 后跑 `sh ./unisacc.com tools/verseq-gen.cx -- --check`,四处口径 `fail=0` 才算完成。
10. 合并顺序与顺延前置: 先合 0.0.94 入 `main`; 顺延前须先解决 `build/version-seq.tsv` 槽 31(0.0.96)主题("执行核每步成本")与 `v0.0.96` 分支(进程生命周期承接版)不一致的问题,定号后再把 `main` 合进 `v0.0.96`(重跑 `check-proc-lifecycle`)与 `v0.0.95`。
11. 解冻(在 Root 升级之后): 提交冻结闸由 `tools/precommit.cx` 读取 `build/freeze.tsv` 控制,无独立解冻 CLI,解冻即删除或提交移除 `build/freeze.tsv`(注意不要混淆: `release/freeze.tsv` 是 0.0.43 历史表,`tools/freeze-gate.cx` 是阶段门而非提交冻结开关)。

## 五、发布说明如实口径

不得声称:osx/win 调用号与信号常量经实机运行验证(没有 CI 样本的项一律写"仅读码"或 `UNMEASURED`, osx 四项探针直到 S3b `ci-early` 实跑前均为 `UNMEASURED`);全批资格 QUALIFIED(s9 UNKNOWN 时只能写 NOT_QUALIFIED);Z3/Z3b/Z5/Z6 与前端三项在本版交付(已撤出 3970fc6d5a, 分别归 `v0.0.96` 与前端类候选);pipe2 在 osx 上原子(是 `cx/sys/proc.cx:225-240` 的 pipe 加 fcntl 非原子实现, win 为具名拒绝 `-ENOSYS`)。须写明已知缺口:U31/U32 已迁 `v0.0.96:build/state-unverified.tsv:24-25`, U33 改回 `OPEN` 记于 `v0.0.94:build/state-unverified.tsv:24`; `l0-posix2` 批次在 `v0.0.94:tests/batches.tsv:1931` 为 `IMPLEMENTED`, 与 `plans/v0.0.94.md` 的 `REGISTERED_NORUN` 说明不一致, 由 S3 候选产品重建后首跑决定, 未实跑前保持 `REGISTERED_NORUN`; `win` 无原生停止信号投递(`exec/facts/signal-targets.tsv` 仅进程内兼容值, `ext/sysx/reaper.cx` 与 `sy_child_subreaper` 在 win/osx 为具名拒绝 `-ENOSYS`);严格预跑中的 UNKNOWN 与 expected-fail 逐条列出;0.0.95 若同期冻结, 其改动、证据、未判定项与回滚材料均不并入 0.0.94 发布说明, 两版各自独立绑定 F/APE/evidence 目录。自动门: `sh ./unisacc.com tools/check-release-prep.cx -- --part notes` 核对每条「已验证」声称必须带真实证据引用(且非 `TBD` 占位行), 否则 FAIL。

## 六、风险与本手册最可能错在哪里

风险:确定性红仍在 T0 之后出现(0.0.93 两次都是,22-premerge-union 是新门,尚无真实拦截记录);链期间他车道推进 main 触发 MAIN_ADVANCED 重合并;osx 探针在冻结 CI 才首次真跑并变红(代价约 40 分钟);提交门复核未过前门本身可能误放或误拦;chain-ready 全量耗时未知。

最可能错的地方:S2 整链 30 分钟是外推,没有读整链日志(证伪:读最近一次 bg-candidate 起止时间戳);0.0.93 的 125 分钟主要归因于 F、F2 是推断(证伪:对 prerun-supervise.log 按 PRERUN_WINDOW 的 wall_ms 求和);T0 口径可能与 fable 报告不同(若从 F2 起算 0.0.93 只用了 47 分钟);写窗口锁可能并不存在;余量 15 分钟的"一次窗口级重跑"假设单窗 58 秒。

最便宜的总体证伪:在并集树上按本手册 S1 至 S8 做一次不公开的全程计时演练(S9 用 --dry),拿到每步真实耗时后再定预算;演练超 105 分钟,预算表作废重排。

## 取证发现的三处不一致(需先定)

1. release-chain plan 不带 --inputs/--state/--layout 直接拒绝(REFUSE CHAIN_USAGE),无参只读形态不存在;相位取自源码头注释与 show_prerun。
2. tools/devsvc.cx 里没有具名的 lock/claim 子命令,build/*.tsv 里也搜不到 U31/U32/U33。
3. build/version-seq.tsv 的 0.0.96 行主题是"执行核每步成本",与"Z3/Z5/Z6 迁入 v0.0.96"不一致;0.0.94 行状态仍为"建议"。


## 附录:冻结日广播正文

来源:csr 2026-11-23 `--dry` 实跑 `tools/freeze-broadcast.cx`(补齐手册对齐项后;检查 `check-freeze-broadcast` 33/0)。白名单表 `skills/unisacc-orch-supervise/freeze-broadcast-windows.tsv`。

### 收件清单(--dry targets=7)

`agy-cage`/`cage`/`cagemon` 已标 `other-repo` 且 `include=0`(moltbaby 仓窗口,默认不发)。

`--dry` 原行(2026-11-23 重核):
```
FREEZE_BROADCAST	cmd=start	targets=7	dry=1
FREEZE_BROADCAST_TARGET	omp-go
FREEZE_BROADCAST_TARGET	csr
FREEZE_BROADCAST_TARGET	cdx
FREEZE_BROADCAST_TARGET	ccogweco
FREEZE_BROADCAST_TARGET	agyneon
FREEZE_BROADCAST_TARGET	omp-sonnet
FREEZE_BROADCAST_TARGET	agy-flow
```

恰七窗:`agyneon` `agy-flow` `cdx` `omp-sonnet` `csr` `ccogweco` `omp-go`。`cdx2`/`ccapi`/`agy-cage`/`cagemon`/`cage` 均不在清单。夹具断言默认 targets=7 且 other-repo 不在;把 other-repo 加回 include=1 时 targets=10(变异必红性质)。

### START(--dry --lock cc --until T0+120m --load-ceil 8)

```
冻结广播 START
静默开始:S1 前起至结束广播前,除集成者外暂停重检查(derive/ratchets/批量 tests/构造/全量 premerge-union);只读与轻编辑可保留。
禁写域:src/ cx/ ext/ exec/ include/ seed/ kernel/;harness(tests/ 与链/预跑/release-* 相关 tools);release/ 除集成者外不写。
负载阈值:起 S2/S3 前 uptime 1 分钟负载 < 8;过高先等或收尾,禁止硬起。运行中负载超 12 时 S3 重跑/续跑降为 --window-lanes 2 --max-jobs 6。
持锁/集成者:cc(devsvc lock acquire freeze --ttl 7200;同属主续期;强拦截以 precommit freeze.tsv 为准)。
提交 trailer:域内提交须带 X-Unisacc-Window 对齐 freeze owner;裸 git 无 trailer 一律 VIOLATION(freeze-watch)。
预计结束:T0+120m
结束条件:收到 END(公开读回完成)或 ABORT(中止)广播后才可恢复重检查;未收到前不得恢复。
```

### PHASE S2 / S3 / S9(--dry --lock cc)

```
冻结广播 PHASE S2
请按手册该阶段动作推进;禁写域与静默规则仍有效;持锁/集成者:cc。
```

(S3/S9 同形,仅相位名替换。)

### END(--dry)

```
冻结广播 END
静默结束:可恢复重检查与常规提交纪律;禁写域随 freeze.tsv 解除一并失效(以 precommit freeze_gate 为准)。
```

### ABORT(--dry --why dry-rehearsal)

```
冻结广播 ABORT
本次冻结中止。原因:dry-rehearsal
静默结束;勿继续按冻结期节奏推进,等待集成者下一指令。
```

### 与手册一致性核对

| 规则 | 手册位置 | 广播正文 | 结论 |
|---|---|---|---|
| 负载静默阈值 8 | 第三节 / S3 护栏 | START 负载 < 8 | 一致 |
| 负载超 12 降配 lanes2/j6 | S3 护栏 | START 已写 | 一致(本轮补进) |
| 禁写域 | 第三节 | START 列出 | 一致(harness 表述略压缩) |
| trailer 要求 | freeze-watch 方案A | START 已写 | 一致(本轮补进) |
| 持锁 owner + ttl 7200 | devsvc freeze 锁 | START `cc` + `ttl 7200` | 一致(本轮补进) |
| 结束条件 END/ABORT | 第三节静默结束 | START 结束条件 + END/ABORT 正文 | 一致(本轮补进) |

### 冻结日监视命令(Monitor 友好)

总上限 2 小时,每 120 秒一次,单次 `tools/bound 55`;只在 VIOLATION 或非 CLEAN 状态变化时出行,CLEAN 不刷屏:

```sh
cd ~/repos/unisacc
PREV=/tmp/unisacc-freeze-watch.prev; : > "$PREV"
DEADLINE=$(( $(date +%s) + 7200 ))
while [ "$(date +%s)" -lt "$DEADLINE" ]; do
  OUT=$(./tools/bound 55 sh ./unisacc.com tools/team-pulse.cx -- --freeze-watch --since-min 180 2>&1) || true
  LINE=$(printf '%s\n' "$OUT" | grep '^FREEZE_WATCH' | head -n 1)
  OLD=$(cat "$PREV" 2>/dev/null || true)
  printf '%s' "$LINE" > "$PREV"
  case "$LINE" in
    *VIOLATION*) printf '%s\t%s\n' "$(date -Iseconds)" "$LINE" ;;
    *CLEAN*|"") ;;
    *) [ "$LINE" != "$OLD" ] && printf '%s\t%s\n' "$(date -Iseconds)" "$LINE" ;;
  esac
  sleep 120
done
```

私有目录模拟(造无 trailer 域内提交):出行原行
`2026-11-23T05:23:41+08:00	FREEZE_WATCH	VIOLATION	window=shared	path=src/leak.c	kind=commit	...	author=-`
(复验命令:`sh ./unisacc.com tools/team-pulse.cx -- --freeze-watch --root /tmp/csr-freeze-day/fw-sim --since-min 120`。)

实质检查:`sh ./unisacc.com tools/check-freeze-broadcast.cx --` -> `FREEZE_BROADCAST_SUMMARY failed=0 passed=33` + `CHECK_FREEZE_BROADCAST`。
