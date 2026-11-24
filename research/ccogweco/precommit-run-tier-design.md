# 提交钩子按层跑批:设计页(precommit-run-tier-design,2026-11-24 ccogweco)

定位:工具线后续件,不进 0.0.106/0.0.107;先设计,实现在 R34 B4 五件列车工具的基线上单独立项。
本页只回答:钩子在固定门之外,按 affected 选中的 commit 层批在预算内实跑、并把结果纳入 blocked 判定,怎么做才不违铁律。

## 1 缺口前提(取自 precommit-gap-affected.md,已实证)

- 选批不缺:`affected --files tools/precommit.cx` 在 3de1f09b87 前后都选中 check-gen-all-drift;但钩子只跑固定门,
  tier=commit 只被 `tests/run.cx --commit ID` 与 `all-waves --tier` 消费。
- 本设计把「选中的批在本提交实跑」接到钩子里,但必须限定集合与预算,否则 precommit.cx 一改选出 500+ 批。

## 2 数据:三个自指工具的登记源批与实测耗时

来源:`~/.cache/unisacc/batch-times.tsv`(run-batch 实测追加),工具 tools/batch-timing.cx;本次用临时只读脚本
/tmp/bisect/times.cx(未入库)按 coverage source 取「直接登记该工具为源」的批,取每批 max total_ms(含 kit/load)。
集合是登记源交集,不是 affected 全闭包(闭包对 precommit.cx 是 503 批,不可能在钩子里跑)。

| 工具 | 登记源批 | 已测 | <=10 s | 10-30 s | >30 s | 未测 | 实测 max 合计 |
|---|---|---|---|---|---|---|---|
| tools/precommit.cx | 43 | 40 | 14 | 5 | 21 | 3 | 1052.7 s |
| tools/affected.cx | 32 | 23 | 18 | 5 | 0 | 9 | 105.7 s |
| tools/check-gen-all-check.cx | 3 | 3 | 0 | 2 | 1 | 0 | 74.4 s |

<=10 s 子集细节(本设计的 MVP 实跑集):

- affected 18 个,合计 8.6 s:affected-iface2 44、affected-hosttcc 67、affected-iface 67、affected-run 107、
  gate-prepare 124、affected-product-inputs-real 129、affected-domain-run 131、affected-tiers 131、
  affected-product-inputs 150、affected-domain-refusal 153、affected-roots 155、affected-processcore 158、
  affected-domain-query 176、batch-packages 176、affected-list2 193、affected-list 223、dcheck-bg 1243、dcheck-manifest 5173。
- precommit 14 个,合计 3.2 s:precommit-gates-toolsreg-c 52、-b 65、precommit-gates-toolsreg 88、
  tables-gen 112、precommit-tlibapi 114、precommit-gates-reuse-commit 139、precommit-precheck-pass 139、
  check-gen-all-ready-precommit 149、precommit-tlibhint 157、precommit-gates-toolsreg-d 159、precommit-precheck-trunc 180、
  precommit-preview 184、precommit-lint-r2-sep 324、bpidecl 1333。
- gen-all-check 0 个(最小 13.1 s)。
- 合计 32 批、串行 11.8 s;2 路并发理论 6 s 量级,加启动与装载按 12-15 s 估。

重批(>30 s,不在 MVP):check-gen-all-* 四件(30.3-53.1 s)、precommit-coverage-* 五件(38.6-55.7 s)、
precommit-toolsloc 三件(46.1-46.3 s)、precommit-gates-na/-key/-toolsreg-a2(46.4-53.1 s)、lane-key-probes(51.5 s)、
real-commit(53.1 s)、precommit-lint-r3-die(37.0 s)、precommit-derive(39.0 s)等。
10-30 s 段:affected 5 个(gitnode 11.4、dcheck 14.4、tool 17.3、check-opt-frame4-material 24.0、oldproduct 30.0)、
precommit 5 个(flow-route 14.1、flow 14.5、gates-reuse 21.9、freeze 28.3、runner-identity 28.9)、gen-all 2 个。
未测 12 个(affected 9、precommit 3),按未测处理,不得当快批。

## 3 铁律约束(设计前提)

tools/batch-timing.cx 表头明写:只记观测,不记结论,任何门不得读它判通过或跳过执行。因此:

- 耗时表只用于两件事:立项时挑出 MVP 集合、给策略表填 bound。门在运行时不得读耗时表。
- 「哪些批在钩子里跑、哪些推迟」是**声明式策略表**,不是由耗时推断的跳过。
- 未实跑 = 未测,不得记 PASS;推迟项必须具名、带到期,并进账本计数。

## 4 新事实表:tests/precommit-runtier.tsv

列:`batch_id<TAB>mode<TAB>bound_ms<TAB>owner<TAB>expires<TAB>reason`

- mode=run:本提交选中的批必须实跑;bound_ms 是该批超时上限(取实测 max 的 1.5 倍向上取整,cap 10000)。
- mode=defer:推到 pre-push/daily,reason 必填,expires 必填(日期),到期即按 BLOCKED。
- mode=defer 的批在提交输出里打印 RUN_DEFERRED 行(批 id、reason、expires),不记 PASS。
- 新门 `precommit-run-tier` 读取策略表 + `affected --files <staged> --list-only` 的结果,在登记源交集内执行。

配套检查 tools/check-precommit-runtier.cx(登记为 commit 层批):策略表可读、列数、mode 枚举、bound 合法、
defer 行有 reason 与未过期 expires、run 行 bound 不小于 batch-times 最近实测 max 的 0.9 倍(只作策略表体检,
不在运行期读)、以及「三个自指工具的登记源批全部出现在表中」——缺行即红。改 precommit.cx/affected.cx/check-gen-all-check.cx
时必须同步改策略表,否则缺口页的登记检查(修复方向 4)红。

## 5 快照与执行

触发:暂存路径命中三个自指工具之一时(cx 门已有的路径判断),执行顺序:

1. `affected --files <staged> --list-only`(与 affected-canary 同读集预热;rc≠0 直接 BLOCKED)。
2. 取结果与策略表 run 行的交集;空交集则打印 RUN_TIER empty=1 并通过(选批为空的正常情况)。
3. 非空:经 devsvc flow `--par 2`(服务不可用时 precommit-flow)跑 run 行;每批 bound = min(bound_ms, 策略预算)。
   总预算 30 s:20 s 后不再启动新批,28 s 杀剩余进程组(TERM->等待->KILL->核实残留),与 0.0.125 的收尾合同同形。
4. 判定:每批只认 run-batch 的完成行(rc0:<token> 或明确的 FAIL 行);缺完成行=BLOCKED(incomplete),不按退出码猜。
5. 输出 `PRECOMMIT_RUN_TIER ran=N deferred=M blocked=K ms=T` 与逐批 RUN_BATCH/RUN_DEFERRED 行;deferred 行写进
   现有 deferred-gates 账本(同一账本,不同 reason)。

## 6 失败关闭规则(全部 fail-closed)

1. 策略表不可读/缺行/坏列 -> BLOCKED(不放行)。
2. affected 选批 rc≠0 或超时 -> BLOCKED(沿用 canary 的具名原因)。
3. run 行被选中但未跑(预算不足、启动失败) -> BLOCKED incomplete,不是 deferred。
4. run 行实跑 FAIL 或超时 -> BLOCKED,具名批 id 与末行原因。
5. defer 行被选中且 expires 已过 -> BLOCKED。
6. 三个自指工具的登记源批在策略表里缺行 -> BLOCKED(策略表与登记不同步)。
7. 任何未测批不得因「耗时表里没有」而跳过:缺行即第 6 条的 BLOCKED,不是默认 defer。

## 7 时延影响(估算,以实测为底)

- MVP(32 批、串行 11.8 s):2 路并发 + 启动/装载,典型 8-15 s,上限 30 s;只在暂存三个自指工具时触发,
  其余提交零增加。
- 现状固定门本身已有各自 55 s 的 slurp 上限;新门把总提交时延上限从「各门之和」推高最多 30 s,
  仍在单命令 60 s 铁律内。若日后扩到 10-30 s 段,必须同时把并发提到 4 或再裁集合,否则超时。
- 重批一律不进钩子:它们的价值在 daily/整链波次,钩子里重跑只会造成 50 s 级串行,违反分治时限。

## 8 与 R34 B4 工具线的关系

- R34 B4 采纳五件列车工具草案(pre-push、ready、release、devsvc、precommit)为工具线保全基线,工具线不随版本冻结;
  链跑期间列车工具仍冻结。
- 本设计改的正好是 precommit.cx(五件之一)与其新检查批:必须作为工具线单独提交落 main,带 check 批与登记,
  不进版本分支工作树,不随 0.0.106/0.0.107 走;落地前保留 B4 草案已有的夹具仓分支、gate 复用键与 freeze 行为不变。
- 与 pre-push 的分工:钩子只跑 <=10 s 批;10-30 s 与 >30 s 由 pre-push 或 daily 波次跑,pre-push 用同一策略表 mode=defer
  的批,输出同一 RUN_DEFERRED 格式,便于一处统计。

## 9 最小可行版本(MVP)

范围:只对 tools/precommit.cx、tools/affected.cx、tools/check-gen-all-check.cx 三个自指工具启用;只跑 32 个 <=10 s 批。

步骤:
1. 先立 tools/check-precommit-runtier.cx 与 tests/precommit-runtier.tsv(32 行 run + 46 行 defer;12 个未测批一律 defer
   并写 reason=unmeasured),登记批,单独提交;此时不动钩子。
2. 在 precommit.cx 的 cx 门之后加 run-tier 节点,默认开关 UNISACC_PRECOMMIT_RUN_TIER=off,先只打印选批与策略命中
   (report-only 一个周期),观察 3 个提交的时延与误报。
3. 打开开关:选批为空或全 defer 时打印并放行;有 run 行且全绿才放行。先红后绿证据:在 tree-b150aa37e2 上
   暂存一个改坏的 precommit.cx,run 行必须红;恢复后绿。
4. 回滚:删 run-tier 节点与策略表条目,恢复 precommit.cx 上一版;开关置 off 时行为与现在完全一致。

明确不做:不把 「affected 全闭包」 搬进钩子;不让门读 batch-times 判跳过;不为过线登记预期失败;
不把重批塞进提交期;不在冻结期或第一车链跑期间落地本设计。
