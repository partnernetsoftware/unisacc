# 拆分抽函数：实时字段→值快照 检查清单

来源：C44-PACKAGE-01 cut 3（3b83968f6a）把参数走查从 dv_main 抽成
src/driveargs.cx 的 dv_parse_args 时，把「实时字段 `opts.runit`/`opts.defrun`」
改成「按值局部参数」，而 `cxc_step` 在同一循环内更新 `opts->runit`，局部快照
不变，导致 `-run FILE -- ARGS` 的 `--` 后参数被当源重解析（程序 argc=1）。
这是 release-stopping 回归（四自重启 check 失控递归、load 151）。

## 检查清单（每刀拆分前逐条核对）

1. 被抽出的函数，若「参数是某个状态结构体的字段值（int runit）」，先确认该字段
   是否会被「本函数体内的调用（cxc_step/cxp_step/子函数）以指针更新」。
   - 会更新 → 该参数必须改传指针（`int *runit`），函数体读 `*runit` 实时值。
   - 不会更新 → 值快照安全，保留。
2. 「实时读」的判定不看字段名，看「谁写」：凡「结构体字段」被「本函数体内
   的调用通过 `st->field = 1` 更新」，且「更新点在被读点之前」→ 就是实时字段，
   必须指针/访问器，不得复制进局部快照。
3. 拆分时对照「拆分前原代码」：原代码若写 `opts.runit`（结构体字段实时读），
   拆分后改成局部 `runit`（快照）就是回归；反之 `runit2 = opts.runit` 在调用点
   一次性赋值、之后循环内不再更新 opts 字段，则快照安全。
4. 回归断言要覆盖「拆分前后行为等价的最小可观察面」：`-run FILE -- ARGS`
   的 argc/argv 透传（tests/p1-baseline-argc.cx + p1-baseline 的
   run_argc/run_argc_default 两用例），不是只测「有无源码」。
5. 提交前跑七棘轮（tools/ratchets.cx），srcshape 只降不升：注释也会占行，
   加注释导致 lines/maxfn 超基线时要删或压缩。

## 已修复

- 47e4fc5467 driveargs: runit/defrun 改指针实时读（唯一同类；cut 1-7 其它
  dv_finish/dv_run_route/dv_read_input/dv_link_objects_route 无循环内实时字段快照）。
