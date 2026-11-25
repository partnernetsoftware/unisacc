# 0.0.107 发布日:证据与说明落盘 runbook(ccogweco)

分工:cdx 的 car2-runbook.md 管起链到发布的命令序列;本页只管证据与说明的落盘点、核对命令与期望值。T0 = 0.0.106 已公开且读回通过。所有命令在版本工作树或私有目录执行,不写共享检出(冻结期规则)。时间均为顺序点,不绑定日历。

## 0 前置(第一车 0.0.106 公开当天,早于 T0)

| 步 | 动作 | 校验命令 | 期望 | 依赖页 |
|---|---|---|---|---|
| 0.1 | 106 证据四项归档:ape-receipt 脱敏拷入、fixedpoint-6targets.tsv、INDEX 两行、notes 语句 | `grep -c "" release/evidence/0.0.106/ape-receipt.txt`;`grep -c SAME release/evidence/0.0.106/fixedpoint-6targets.tsv` | 12 行;6 行 SAME | archive-106-steps.md |
| 0.2 | 106 证据链核:chain_rev=2e8a19a342、sources_sid=c6eeca9f10、ape_sha256=5f83ac93 | `grep -E "chain_rev|ape_sha256" release/evidence/0.0.106/fixedpoint-6targets.tsv` | 三值同上 | fp-106-extraction.md |
| 0.3 | E01 正控+负控(106 预跑完成后低载窗口,agy-h 执行) | 见 e01-exec-steps.md 的两条命令 | 正控 rc0;负控非零且含 `release/evidence.tsv stale: path does not reference candidate version` | e01-exec-steps.md |
| 0.4 | U08 关闭(agyneon 走 A:加 source-missing 负控) | `grep -n source-missing tools/check-slice-migration-gen.cx tools/check-slice-migration-gen.cx` 与批次实跑 | 生成器:90;检查:新增一条负控 PASS | u08-evidence.md |

## 1 T0:107 起链前,证据准备(与 cdx runbook 的 (1)(2) 并行)

| 步 | 动作 | 校验命令 | 期望 |
|---|---|---|---|
| 1.1 | 合并/派生/冻结按 cdx runbook 执行 | 以 cdx car2-runbook.md 为准 | — |
| 1.2 | 缺口节与说明草稿随 107 分支落库 | `git -C "$W" status --short` | notes-gaps-draft.md 与 plans 的改动在私有分支,不在共享 main |
| 1.3 | 冻结能用清单:把 usable-list-draft 的 30 项写成清单表(表头 `item category batch`,编号 1 到 30,分类 8/8/6/4/4) | `grep -c "" checklist.tsv`;`cut -f3 checklist.tsv | sort -u | wc -l` | 31 行(含表头);批次键 30 个唯一 |
| 1.4 | P0 账本七列化(五列加 state、closure_evidence) | `sh ./unisacc.com tools/seal-precheck.cx -- <EVIDENCE_DIR>` 的 P0 行 | 五列 P0 记未关闭;P0-chain-step 只有 CLOSED 且有已落盘证据才计关闭 |

## 2 107 发布动作中:U30 捕获(与 cdx runbook 的 (4)(5) 同步)

| 步 | 动作 | 校验命令 | 期望 |
|---|---|---|---|
| 2.1 | 真实五步 release-chain 跑完到 READY_TO_EXECUTE;完整 stdout 存 `release/evidence/0.0.107/release-chain-run.out` | `grep -c "CHAIN	PASS" release/evidence/0.0.107/release-chain-run.out`;`grep "CHAIN	RESULT" ...` | 5 行 PASS;一行 `CHAIN	RESULT	READY_TO_EXECUTE	version=0.0.107` |
| 2.2 | INDEX 追加两行(release-chain-run-0a 与 release-chain-run-1-5) | `grep -c "release-chain-run" release/evidence/0.0.107/INDEX.tsv` | 2 行 |
| 2.3 | notes-0.0.107.md 已知缺口节写入 U30 关闭句与其余缺口 | `grep -n "U30" release/notes-0.0.107.md` | 一句,指向 release-chain-run.out |
| 2.4 | 发布后关闭 U30 状态行 | state 表中 U30 的 evidence 列 | `release/evidence/0.0.107/release-chain-run.out;CHAIN RESULT READY_TO_EXECUTE` |

## 3 107 公开前:seal-precheck 证据目录组装(0.1.0 准入机械核对)

运行点与格式来自 plans/seal-precheck.md(cdx 的 seal-precheck.patch 内含该页与 tools/seal-precheck.cx;该补丁未落库前,本步等它落)。

| 步 | 动作 | 校验命令 | 期望 |
|---|---|---|---|
| 3.1 | 证据目录放齐:strict.tsv、expected-fail.tsv、expected-unmeasured.tsv、state-unverified.tsv、p0-ledger.tsv、checklist.tsv、seal-evidence.tsv 与引用文件 | `ls <EVIDENCE_DIR>` | 七类文件齐;seal-evidence.tsv 表头 `kind key value ape_sha256 ref` |
| 3.2 | release 三行:106、107 为 PUBLISHED,末行为 CANDIDATE 或 PUBLISHED;引用 fixedpoint-6targets.tsv | `sh ./unisacc.com tools/seal-precheck.cx -- <EVIDENCE_DIR>` 的 release 行 | 六目标;g2/g3 相等且 SAME;末行 APE 绑定 strict |
| 3.3 | ci 六行:六个目标值 QUALIFIED,摘要为当前 APE | 同上 ci 行 | 六行 QUALIFIED |
| 3.4 | frontend 三行:A、B1 为 PASS;B2 按 R37 顺延记未满足(本条允许豁免,但说明里必须写) | 同上 frontend 行 | A/B1 PASS;B2 记录未满足 |
| 3.5 | timing 三行:版本序三行 START_EPOCH/END_EPOCH,每趟上限 7200 秒;107 的 END 在公开前待补 | 同上 timing 行 | 106 有值;107 未公开时该门拒绝,公开后再跑 |
| 3.6 | 严格表重计与 FAIL/UNKNOWN/INVALID 为 0;UNKNOWN 仅已登记无生产者项且不超 2 | 同上 strict 行 | FAIL=0、INVALID=0;UNKNOWN <=2 且已登记 |
| 3.7 | 未验证计数:声称类按 R37、含 R38 的 U21,上限 3 | 同上 unverified 行 | 若 4 行则如实记未满足(预期 3 到 4) |
| 3.8 | 清单 30 项实得 PASS >= 25 | 同上 checklist 行 | 30 项批次键须在严格结果里;PASS >=25(当前预计 29 就绪) |

## 4 0.1.0 封版当天:最终核对

| 步 | 动作 | 校验命令 | 期望 |
|---|---|---|---|
| 4.1 | 六目标不动点连续三版归档(106、107、0.1.0 各一份 fixedpoint-6targets.tsv 与 ape-receipt) | 每份 `grep -c SAME` | 每份 6 行 SAME;107 与 0.1.0 各自链的 slices=same count=6 |
| 4.2 | CI 六格与 strict 汇总入 evidence | `sh ./unisacc.com tools/seal-precheck.cx -- <EVIDENCE_DIR>` | 全七行 `SEAL_GATE` 通过,末行 `SEAL_PRECHECK` 通过;任一拒绝 rc2 |
| 4.3 | E02 已知缺口写入 0.1.0 发布说明 | `grep -n "E02" release/notes-0.1.0.md` | 一句,按已知缺口,不改 kind |
| 4.4 | U33 与 chain-step 的处置写入说明 | `grep -n "U33\|chain-step" release/notes-0.1.0.md` | U33 P1 随 0.0.125;chain-step P0 已由 107 修复并在候选实跑 |
| 4.5 | seal-precheck 最后一次全绿后,才进 cdx runbook 的发布步骤 | 末行 `SEAL_PRECHECK` | 全绿 |

## 5 依赖与风险

- plans/seal-precheck.md 与 tools/seal-precheck.cx 目前只在 cdx 的 seal-precheck.patch 里,未落库;第 3、4 节依赖该补丁先落。若封版当天仍未落,按 R37 第 4 条如实记未满足,不能口头当通过。
- 条 4(未验证)按三值预期 3 到 4;若 4,seal-precheck 的 unverified 行会拒绝,这是设计如此,不调规则。
- 条 6 timing 的第三样本要等 0.1.0 到 READY_TO_EXECUTE 的 END 时刻,公开前该门必拒,顺序不能颠倒。
- 本页所有命令只读或写私有证据目录;落 release/ 的动作由集成者按 cdx runbook 执行。
