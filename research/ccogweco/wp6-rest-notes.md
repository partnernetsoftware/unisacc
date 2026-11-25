# WP6 剩余:已交付部分与未做项(ccogweco)

补丁: research/ccogweco/wp6-rest.patch(基线 v0.0.107 头 d750ad123c;1 文件 14+/3-)。
私有树编译: `sh ./unisacc.com -c -funit tools/check-npfault.cx` rc0。
元数据: `sh ./unisacc.com tests/run.cx -- --list` -> metadata_valid=1。
precommit 镜像树: git archive d750ad123c 解到私有目录、git init 提交基线、应用补丁并 `git add tools/check-npfault.cx`,在该树跑 `sh ./unisacc.com tools/precommit.cx`,原行:
    PRECOMMIT_GATES	not_affected=7	na=manifest,tables,toolsreg,runlist,affected-canary,facts,ledger	source=table
    precommit: lint(report-only):LINT	R1-product	tools/check-npfault.cx:51	product path literal "./unisacc.com" placed in an argv slot
    precommit: lint(report-only):LINT	R1-product	tools/check-npfault.cx:75	product path literal "./unisacc.com" placed in an argv slot
    PRECOMMIT	staged_cx=1	blocked=0
(两条 R1 为该文件既有写法,报告级,不阻塞。)

## 内容:cx-npfault 材料复用

四个用例原先各自 `sh ./unisacc.com tests/run-batch.cx`,每次让产品冷编一遍 run-batch。改为:检查开始时在私有目录一次 `sh ./unisacc.com tests/run-batch.cx -o <hold>/run-batch`,之后四个用例直接调原生执行体(与 all-waves 的 SESSION/executor/run-batch 同形);失败即 die。断言、env 注入、时限均不变。

## 未做:s8c-bind 与 dev-product(附理由)

- s8c-bind "用法拒绝/绑定两批":绑定用例必须真的 init 会话并 `all-waves material`,而 all-waves 原生件的构建同 wt-gate 一样受 tests/run-batch 材料规则限制(备选 A 未落);照现状再拆一个绑定批只会把同样的原生构建成本再付一次,而且当前 46.46s(KILLED,峰 55.78)的归因没有实测——本入口只有一个子进程编译,解释不了 46 秒,先量再拆更稳。故本包不动它。
- dev-product "拆 seedgen 两批":入口 main 是一条顺序体,seed-gen 两套构造与其余检查共用 work 目录;要拆须加 `--cases seed|no-seed` 并把其余所有检查块逐段设门(或另起第二入口),改动面远大于其余九批;且 39.09s 是 commit-tier 39 秒截断窗的 UNKNOWN,不是真实单跑数。故本包不动它,建议链后先各跑一次冷态,再按实测决定是拆批还是只加材料。
