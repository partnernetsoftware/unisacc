# U30 与 E01 页命令静态核对(107 树 d750ad123c,2026-11-24 ccogweco)

只读:git show/ls-tree 取 107 树源码与表,不跑任何批次。核对对象:research/ccogweco/u30-capture-checklist.md、
e01-exec-steps.md、e01-closure.md。源码行号均指 d750ad123c 的 tools/ 文件。

## 总判

命令与参数拼写基本对(五个入口与全部 flag 都能在源码参数解析里找到);主要问题是**要贴的期望原行有三处不存在**、
**E01 负控改错了文件**、**E01 前置在纯 archive 上不成立**、以及两处材料/路径事实不成立。逐条如下。

## A U30 捕获清单(u30-capture-checklist.md)

### A1 不存在:CHAIN PASS 1/2/4(步骤 1、2、4 的期望行)

- 源码:release-chain.cx:124 每个执行的步骤打印 `CHAIN\tRUN\t%d\t%s\trc=%d\tms=%ld`;:120 已完成的步骤打印
  `CHAIN\tSKIP\t%d\t%s\tdone`。全文 `CHAIN\tPASS` 字面量只有两处:216(步骤 3)与 1534(步骤 5 backup-evidence)。
- 页里把步骤 1、2、4 写成 `CHAIN PASS 1 prep`、`CHAIN PASS 2 stage-a`、`CHAIN PASS 4 stage-dry` 均不成立。
  应改为 `CHAIN RUN 1 prep rc=0 ms=...`(首跑)或 `CHAIN SKIP 1 prep done`(续跑);步骤 2、4 同形。

### A2 不存在:CHAIN PASS 5 execute;且步骤 5 的 done 回执也不存在

- 源码:release-chain.cx:1534 打印的是 `CHAIN\tPASS\t5\tbackup-evidence\tversion=%s\tfiles=7`,不是 execute;
  :1538 打印 `CHAIN\tSTOP\t5\texecute\tstart it as a long job: ...`;execute 只 show(5, s5) 打印命令,不由本工具跑。
- 页里步骤 5 的「日志 STATE/execute.done」不成立:marker 只在 step() 里写(:112-127),execute 不是 step(),
  所以 STATE/execute.done 永不出现;会出现在 STATE 的是 prep.done、stage-a.done、stage-dry.done。
- `CHAIN RESULT READY_TO_EXECUTE version=0.0.107` 成立(:1540),但它的前置是 backup manifest 校验通过
  (:1528-1534,失败是 `CHAIN FAIL 5 backup-evidence`)。页里应写明这条前置。

### A3 pack-run.out 的来源与输入前置没写

- 源码:release-chain 不写 pack-run.out;它把 release-run 输出留在 STATE/pack。release-flow prep 用输入 `pack-log`
  的原文写 `release/evidence/<V>/pack-run.out`(release-flow.cx:278),且 `pack-log` 缺失时 prep 直接
  `MISSING_INPUT` 拒绝(release-flow.cx:504)。页里 0a 行说「inputs 已给 pack= 时跳过」正确,但没写 prep 必须有
  `pack-log=` 指向 release-run 原输出;照页执行会在步骤 1 就 REFUSE。

### A4 INDEX 列数写错

- 页写「四列风格同 0.0.101」,但 release-exec.cx:1763 的表头是五列
  `item<TAB>time<TAB>verdict<TAB>path_or_ref<TAB>note`;0.0.101/INDEX.tsv 与 0.0.105/INDEX.tsv 的实际行也都是
  五个字段(页里给的两行示例本身也是五个)。改为「五列」。

### A5 「现成材料 release/evidence/0.0.107/」当前不存在

- `git ls-tree d750ad123c -- release/evidence/0.0.107/` 为空;107 树里只有 release/candidate.tsv(blob 2138ce99)
  与 release/evidence.tsv。该目录是发布时由 prep/stage 产生,不能列为「现成材料」。

### A6 已核对的正确项

- `release-chain plan --inputs F --state S --layout L` 与 usage 一致(release-chain.cx:4-6);
  plan 下 `CHAIN CMD P0/P1/P2/P3` 字面量存在(:88、:91、:100、:103)。
- `release-flow prep --inputs RESOLVED --out STATE/prep [--decisions D]`、`stage-a --inputs F --prep STATE/prep --out
  STATE/stage [--decisions D]` 的 flag 与 release-flow.cx:472-476 一致;prep 产出 ratchets.txt、rollback.txt、
  evidence-rows.tsv(release-flow.cx:384-407)与页一致。
- 步骤 3 期望行与 release-chain.cx:216 逐字相同。
- 步骤 4 命令与 release-chain.cx:1523 一致(内部还带 `--out STATE/dry`,页不必写)。
- `RELEASE_RUN MODE PACK v0.0.107`、`SOURCES_SID VERIFIED`、`FIXEDPOINT VERIFIED` 分别由
  release-run.cx:804(emit MODE PACK)、:548、:242 打印,成立。
- 「0.0.85 pack 材料已不存在、备份最早 0.0.89」属实:~/.unisacc-release-evidence-backups/ 下有 0.0.89,
  无更早;0.0.105/src-dir 含 p3.pkg、sources.tsv 与六目标切片,可作彩排输入。

## B E01 执行步骤页(e01-exec-steps.md)

### B1 阻断:纯 git archive 上第 4 步过不了 MAIN_ANCESTOR

- 源码:release-exec.cx:254-269 gate_main 在第 4 步执行(step_y1 首行调用);它要求
  `git rev-parse --verify refs/remotes/origin/main` 与 `HEAD` 都是 40 位,并 `git merge-base --is-ancestor
  origin/main HEAD`;缺任一即 `MAIN_UNKNOWN` 拒绝(:263-264)。
- 页写「私有工作树用冻结提交的 git archive」:git archive 没有 .git,三条都失败,正控在第 4 步前就 REFUSE。
  必须在真实 git 仓里跑(worktree 或本地 clone),或 git init + 提交 + 设置 refs/remotes/origin/main 并让 HEAD 含它。

### B2 负控改错文件:改 layout 副本无效

- 源码:第 4 步先 sync_evidence_rows (release-exec.cx:1297-1311):若 cwd 有
  `release/evidence/<ver>/evidence-rows.tsv`,把它写到 `../../release/evidence.tsv`(--dry);否则退回 cwd 的
  `release/evidence.tsv`(:1527-1528)。检查只读这两个之一。
- 页把 `$T/layout-neg/release/evidence.tsv` 改一行再跑,--layout 的这份副本从不被 release-exec 读取,负控不会红。
  应改 cwd 的 `release/evidence/<ver>/evidence-rows.tsv`(或先造 `--out DIR` 下的 evidence.tsv 并传 --out);
  拒绝句本身逐字成立:release-exec.cx:1332 `release/evidence.tsv stale: path does not reference candidate version`。

### B3 前置缺失:冻结提交里没有 0.0.106 证据目录

- `git ls-tree 2e8a19a342|b150aa37e2|d750ad123c -- release/evidence/0.0.106/` 全为空;106 树只有
  release/candidate.tsv(blob fc4048b2)。页写「取 106 证据目录的 evidence-rows.tsv」在 106 预跑完成时并不存在
  (证据目录要到 106 发布/prep 才生成)。改用当前 HEAD 的 release/evidence.tsv,或等 106 证据目录真实出现后再取。

### B4 判据文案太松

- 正控判据「含第 4 步通过行(以当版输出为准)」应收紧为源码实打印的两行:
  `STEP\t4\t<step_names[4]>\tDONE`(release-exec.cx:1763 附近主循环)与
  `RELEASE_EXEC\tRESULT\tDRY_GATES_OK\tsteps=7\tdeferred=..\tindex=..`(:1793)。否则「通过行」无法核对。
- flag 本身成立:`--execute`、`--layout`、`--strict`、`--dry`、`--from-step 1..7` 都在
  release-exec.cx:1712-1737 的解析里;`--from-step 4` 合法。

### B5 未证风险:layout 只有 release/ 时第 4 步可能被 release-decide 拒绝

- 页的 layout 是 `cp -a release $T/layout/`,不含六件资产与 zip。第 4 步 step_y1 调
  `release-decide --candidate release/candidate.tsv --evidence ... --assets <layout>`(release-exec.cx:1348),
  release-decide 对 --assets 的检查本页未核(静态只到这一步)。要么把 layout 换成真实 pack/prep 布局,要么先核
  release-decide 在缺资产时的行为,别把「正常布局」当已验证。

## C e01-closure.md

- 第 18 行「chain 目录已在 ~/.cache/chains」路径错:实际是 `~/.cache/unisacc/chains`(下有 bg-b1、snap),
  `~/.cache/chains` 不存在。其余与 e01-exec-steps 同步修正(B1/B2/B3)。

## D 需要改的最小集

1. U30 步骤 1/2/4 期望行改 `CHAIN RUN ... rc=0 ms=...`;步骤 5 删除 execute.done 与 CHAIN PASS 5 execute,
   写 `CHAIN PASS 5 backup-evidence` + `CHAIN STOP 5 execute`,并注明 backup manifest 前置。
2. U30 0a 行补 inputs 必须带 pack= 与 pack-log=;INDEX 改五列;删「release/evidence/0.0.107 现成」。
3. E01 改为在有 .git 与 origin/main 引用的仓里跑(B1);负控改 cwd 的 evidence 源(B2);删 106 证据目录依赖(B3);
   收紧判据(B4)。
4. e01-closure 改 chain 路径。
