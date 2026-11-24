# U30 关闭清单页 v2:107 发布时真实五步 release-chain 原行

校验规则(本页自 v2 起):所有「要贴的原行」必须逐字来自源码打印字面量,并随行附源码行号(以下行号均取
`git show d750ad123c:tools/<file> | grep -nF '<片段>'` 的结果)。源码变动后先重跑 grep,不得凭记忆写文案。

目标(R38):U30 声称"release-chain 端到端 --dry 未用真实证据复刻",在 107 发布时用真实 pack 跑 release-chain
五个步骤到 READY_TO_EXECUTE,把原行贴进 107 证据与说明即关闭。

## 材料与目录由谁创建

- 冻结提交上的现成文件只有 `release/candidate.tsv`(d750ad123c 的 blob 2138ce99)与 `release/evidence.tsv`;
  `release/evidence/0.0.107/` 现在**不存在**(`git ls-tree d750ad123c -- release/evidence/0.0.107/` 为空),它由发布流程创建:
  - prep 把 `pack-run.out`、`cache-manager.tsv` 写进 `STATE/prep/release/evidence/0.0.107/`
    (release-flow.cx:204-211 的 emit 以 --out 为根;:278 写 pack-run.out);
  - stage-a 把 `ratchets.txt`、`rollback.txt`、`evidence-rows.tsv` 写进 `STATE/stage/release/evidence/0.0.107/`
    (release-flow.cx:383-397 install_prepared,其中 :386-395 写三份);
  - 第 3 步由人把 `STATE/stage/release` 提交进 origin/main,HEAD 上才出现该目录;
  - release-exec 实跑时写 `release/evidence/0.0.107/INDEX.tsv`(release-exec.cx:1755-1770 的 evdir)。
- 0.0.85 的真实 pack 目录已不在:`release/evidence/0.0.85/pack-run.out` 指向的作业目录早已删除;
  `~/.unisacc-release-evidence-backups/` 最早为 0.0.89(已核),无 0.0.85。彩排可用
  `~/.unisacc-release-evidence-backups/0.0.105/src-dir/`(含 p3.pkg、sources.tsv 与六目标切片,已核)。

## 五步:命令与要贴的源码原行

| 步 | 实际命令 | 要贴的原行(源码字面量,行号) |
|---|---|---|
| P0/P1/P2 | `release-chain plan --inputs F --state S --layout L`(release-chain.cx:4-6 usage) | `CHAIN\tCMD\tP0\tprerun-plan\t...`(:88)、`CHAIN\tCMD\tP1\tprerun-start\t...`(:91)、`CHAIN\tCMD\tP1b\tprerun-parallel-docs\t...`(:98)、`CHAIN\tCMD\tP2\tprerun-merge\t...`(:100)、`CHAIN\tCMD\tP3\tprerun-archive\t...`(:103) |
| 0a pack | `release-run --pack ROOT CANDIDATE_TSV SRC_DIR OUT_DIR --chain-out CHAIN_DIR [--snap SNAP]`(release-run.cx:22-23 usage) | `RELEASE_RUN\tMODE\tPACK\tv0.0.107`(release-run.cx:804 emit("MODE","PACK",cand.tag));`RELEASE_RUN\tSOURCES_SID\tVERIFIED\t...`(:548);`RELEASE_RUN\tFIXEDPOINT\tVERIFIED\t...`(:242);`RELEASE_RUN\tRESULT\tPACK_OK\t...`(:815) |
| 1 prep | `release-flow prep --inputs RESOLVED --out STATE/prep [--decisions D]`(release-flow.cx:3-4 usage) | `CHAIN\tRUN\t1\tprep\trc=0\tms=...`(release-chain.cx:124);续跑时 `CHAIN\tSKIP\t1\tprep\tdone`(:120)。**没有 CHAIN PASS 1**(全文 CHAIN PASS 只有 :216 与 :1534) |
| 2 stage-a | `release-flow stage-a --inputs F --prep STATE/prep --out STATE/stage [--decisions D]`(release-flow.cx:3-4、:472-476) | `CHAIN\tRUN\t2\tstage-a\trc=0\tms=...`(:124),或 `CHAIN\tSKIP\t2\tstage-a\tdone`(:120) |
| 3 stage-commit | 人工 commit 并 push 到 origin/main(本工具不做,release-chain.cx:204) | `CHAIN\tPASS\t3\tstage-commit\tHEAD carries candidate.tsv, ratchets.txt, evidence-rows.tsv (X_PUSHED ok)`(:216,逐字) |
| 4 stage-dry | `release-exec --execute release/candidate.tsv --layout L --strict S --dry`(release-chain.cx:1523 的实际 argv) | `CHAIN\tRUN\t4\tstage-dry\trc=0\tms=...`(:124)或 SKIP(:120)。**没有 CHAIN PASS 4** |
| 5 execute | 不在此工具跑;`release-exec --execute ...` 由人/后台起(release-chain.cx:1536-1538) | 先决+回执:`CHAIN\tPASS\t5\tbackup-evidence\tversion=0.0.107\tfiles=7`(:1534;前置 verify_backup_manifest 失败则 `CHAIN\tFAIL\t5\tbackup-evidence\t...` :1530);命令: `CHAIN\tSTOP\t5\texecute\tstart it as a long job: ...`(:1538)。**没有 CHAIN PASS 5 execute,也没有 STATE/execute.done**(marker 只在 step() 写,:112-113;execute 不是 step()) |
| 收尾 | — | `CHAIN\tRESULT\tREADY_TO_EXECUTE\tversion=0.0.107`(:1540) |

## prep 必须的输入(照页执行前先备齐)

- prep 的输入表必须带 `pack=` 与 `pack-log=`;缺 `pack-log` 时 release-flow 直接 `MISSING_INPUT` 拒绝
  (release-flow.cx:496、:504)。release-chain 自己在 0a 写 pack 时把日志留在 `STATE/pack/stage-a-pack.out`
  (release-flow.cx:500),inputs 走 pack= 时要显式指向它或 release-run 原输出。
- `pack-run.out` 的内容就是 release-flow prep 读到的 pack-log 原文(release-flow.cx:278),不是另一次运行。

## 贴进 107 证据与说明的格式

- 完整 stdout 原样存 `release/evidence/0.0.107/release-chain-run.out`;家路径由 release-flow 的 scrub_home 脱敏
  (release-flow.cx:214),pack-run.out 里的路径按 0.0.105 做法用 `~` 前缀或 srcdir 角色表示。
- INDEX.tsv 是**五列** `item<TAB>time<TAB>verdict<TAB>path_or_ref<TAB>note`
  (release-exec.cx:1763 表头),不是四列。追加示例(五列):
  `release-chain-run-0a	<时间>	PASS	pack-run.out	release-run --pack 真实 0.0.107 输入;SOURCES_SID 与 FIXEDPOINT VERIFIED;六目标与 p3 逐行 sha`
  `release-chain-run-1-5	<时间>	PASS	release-chain-run.out	CHAIN RUN 1..4 rc=0;CHAIN PASS 3/5;RESULT READY_TO_EXECUTE version=0.0.107`
- notes-0.0.107.md 已知缺口节加一句:`- U30 已关闭:107 发布时以真实 pack 跑 release-chain 五步到 READY_TO_EXECUTE,原行见 release/evidence/0.0.107/release-chain-run.out;0.0.85 旧 pack 材料已不存在,不再引用。`
- 关闭 U30 时 evidence 列写 `release/evidence/0.0.107/release-chain-run.out;CHAIN RESULT READY_TO_EXECUTE`。

## 注意

- 每步最多 55 秒(各子命令 np_run(v, cwd, 0, 55, ...),release-chain.cx:123):超界的步骤要拆,不延长时限。
- 第 3 步人工提交必须真实到 origin/main,否则 X_PUSHED 复核不过(release-chain.cx:216 之前的 :207-212)。
- 第 5 步之前必须有 backup manifest,否则停在 `CHAIN FAIL 5 backup-evidence`,拿不到 READY_TO_EXECUTE。
- 这条同时关闭 U30,并把 release-chain 批本身的冷态 KILLED 68 秒留在余量缺口(0.1.x),两件事分开记。
