# precommit 流程缺口取证:改了 precommit.cx,受影响批为什么没跑(2026-11-24,ccogweco)

结论:选批没有漏,执行没有接通。`tools/affected.cx --files tools/precommit.cx` 在 3de1f09b87 前后都选中
`check-gen-all-drift`(via=literal,该批登记的 coverage source 明确含 tools/precommit.cx);但提交门
(`.git/hooks/pre-commit` -> `tools/precommit.cx`)从不按 batches.tsv 的 tier 跑任何批次,暂存 tools/precommit.cx
只触发 `cx` 门(shebang/new_tools/tlibfrozen/cx-lint/ratchets),affected-canary 门的读集里没有 tools/precommit.cx,
记 NOT_AFFECTED。于是这次红只能靠有人手工 `tests/run.cx --commit <id>` 或 `affected --run` 才会暴露,提交路径上没有这一步。

## 1 选批结果(只读,via 原行)

命令:

    cd /tmp/bisect/tree-55bc125935 && sh ./unisacc.com tools/affected.cx -- --files tools/precommit.cx --list-only
    cd /tmp/bisect/tree-3de1f09b87  && sh ./unisacc.com tools/affected.cx -- --files tools/precommit.cx --list-only
    cd /tmp/bisect/tree-414a024ea5 && sh ./unisacc.com tools/affected.cx -- --files tools/precommit.cx --list-only
    cd /tmp/bisect/tree-b150aa37e2 && sh ./unisacc.com tools/affected.cx -- --files tools/precommit.cx --list-only

输出(四棵树一致,只列该批与汇总行):

    AFFECTED	check-gen-all-drift	why=tools/precommit.cx	via=literal
    AFFECTED_SUMMARY	changed=1	batches=503	scanned=1965	not_applicable=4
    0.0.105 树:batches=498	scanned=1959;b150 树:batches=518	scanned=1992。

即:选批在 3de1f09b87 提交前后都对,且该提交自己改的 affected.cx 没有改变结论。

## 2 check-gen-all-drift 的层与提交门实际跑什么

    tests/batches.tsv:1689  check-gen-all-drift  IMPLEMENTED  contract  tools/check-gen-all-check.cx  SOURCE  lnx/x86_64  55  -  tools/check-gen-all-check.cx  --drift  contract.check-gen-all-drift  rc0:CHECK_GEN_ALL_DRIFT  product  commit

- 末列(tier)= commit;tests/coverage.tsv:1325 的 source 列含 `tools/precommit.cx`。
- 消费该列的只有 `tests/run.cx --commit ID`(内部 `affected --tier commit --ids ID --run`)与 `tools/all-waves.cx --tier`。
  两者都是人工/CI 入口,不在钩子里。
- `.git/hooks/pre-commit` 只有一行:`exec sh ./unisacc.com tools/precommit.cx`。
- `tools/precommit.cx` 全文没有 run-batch,也没有 `tests/run.cx --commit`;它跑固定门集(manifest/tables/toolsreg/runlist/facts/ledger/
  affected-canary/cx、derive、genall、tlibfrozen、lint、ratchets 等),tier 不参与门的触发。
- 暂存 tools/precommit.cx 命中哪些门:只有 `cx`(ACMR `*.cx`)与工具注册类;affected-canary 读集是
  `tests/batches.tsv tests/batch-product-inputs.tsv tests/coverage.tsv cxroot.tsv tools/affected.cx tools/cxlib ext cx`
  (tests/precommit-gates.tsv:11 与 precommit.cx:184 gt_builtin 同文),不含 tools/precommit.cx,故记 NOT_AFFECTED。
- 即便跑,affected-canary 只执行 `affected --registry-check`(注册表与 product-input 声明校验),不做选批,也不跑批。

缺口定义:coverage 登记把 tools/precommit.cx 写成该批的源,选批器忠实选中;但「选中的 commit 层批必须在本提交实际跑过」
这条没有任何执行点。3de1f09b87 的首轮红(以及后续 b150/2e8a19a342 上仍红)就是这样漏过去的。

复现红(只读复跑,已装产品,b150 树):

    cd /tmp/bisect/tree-b150aa37e2 && sh ./unisacc.com tools/check-gen-all-check.cx -- --drift
    PASS synthetic-header-drift-refused
    PASS precommit-derive-gate-blocks-drift
    FAIL precommit-lane-inputs-add-ext-blocked-and-synced
    CHECK_GEN_ALL_DRIFT ok=2 failed=1 ms=18532
    同一命令在 tree-414a024ea5(0.0.105):ok=3 failed=0 ms=18184。

## 3 同类缺口面(coverage source x batches tier 交叉)

方法:临时只读工具 /tmp/bisect/covcross.cx(未入库),按 batches.tsv 第 11 列 coverage-id 与 coverage.tsv 第 1 列连接,
取 source 列含 tools/{precommit,commit-own,derive,affected,ratchets,devsvc}.cx 的行;在 b150 树上运行。

    n=141 行;tier=commit 125,tier=daily 15,tier=release 0。
    source 含 tools/precommit.cx:coverage 43 行,其中 41 个能在 batches.tsv 找到行(38 commit、3 daily)。
    source 含 tools/commit-own.cx:36;tools/affected.cx:32;tools/derive.cx:6;tools/ratchets.cx:7。

这些批全部是「登记了门工具的源、但钩子不跑批」的同一结构缺口。按风险(是否声称 commit 层强制、是否直接断言阻止/放行语义、
门工具改动频率)列前 10:

| # | 批 | tier | 为什么高风险 |
|---|---|---|---|
| 1 | contract.check-gen-all-drift | commit | 已实证红;source 含 precommit.cx;测试的正是「该拦不拦」本身 |
| 2 | contract.check-gen-all-pass | commit | 同一门的放行侧;桩过期会掩盖过度拦截或错误放行 |
| 3 | interfaces.precommit-gates-na | commit | 断言 NOT_AFFECTED 读集语义;读集错=门静默跳过 |
| 4 | interfaces.precommit-gates-reuse-key | commit | 门结果复用键;键错=旧绿冒充本次绿 |
| 5 | interfaces.precommit-gates-reuse-commit | commit | 同上,提交侧复用路径 |
| 6 | interfaces.precommit-derive | commit | derive 门拦截语义;派生表陈旧可静默提交 |
| 7 | interfaces.commit-own-reg-refuse | commit | 注册/拒收路径;错则坏行入库 |
| 8 | interfaces.precommit-coverage-src2 | commit | 软链源 REFUSE;错则环境类源静默通过 |
| 9 | interfaces.real-commit | commit | 真实 git commit 过钩子端到端;唯一的端到端断言 |
| 10 | interfaces.precommit-freeze | daily | 冻结门只在 daily 波次跑,tier=commit 的提交门不覆盖 |

次高风险(同结构,未列入 10):interfaces.affected-tool 与 contract.derive-readset(daily,选批器/读集自身),
interfaces.precommit-flow(-route)、interfaces.tables-gen、interfaces.precommit-tlibapi、contract.lane-key-probes(-part2)。

## 4 修复方向(建议,未实施)

1. 钩子侧:对暂存的门工具类路径,增加一道「选批必须为空或必须已跑」的门——例如 `affected --files <staged> --list-only`
   输出 commit 层交集,非空时要求提交内带本次 tree 的执行收据(可复用 UNISACC_GATE_REUSE 键),否则 BLOCKED 具名列出批 id。
   precommit.cx 一改会选出 500+ 批,必须按 coverage 源精确收窄到直接登记该工具的批,或放到 pre-push/CI 层。
2. 读集侧:把 tools/precommit.cx、tools/commit-own.cx、tools/derive.cx 纳入 affected-canary 读集,至少让改动触发一次活选批自检。
3. 夹具侧:minirepo 的 archive 清单不含 tests/(check-gen-all-check.cx:65),新 affected-canary 门在夹具里读不到
   tests/batches.tsv 就拒绝(affected.cx:534-535 的 die rc2),这与 check-gen-all-drift 第三断言的失败面一致;
   夹具要么带 tests/ 两张表,要么在夹具仓显式标记跳过该门(只能具名跳过,不得静默绿)。
4. 登记侧:新增检查,遍历「coverage source 含门工具 且 tier=commit」的批,断言其出现在某个提交期执行清单里;
   现在没有任何清单满足,检查会直接红,作为该缺口的常驻看门。
