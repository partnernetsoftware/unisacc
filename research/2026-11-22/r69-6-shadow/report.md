# R69.6 零断言守卫影子阶段盘点与评审落点报告 (2026-11-22)

## 一、六条评审要点核查与原型设计

- (a) 可核查豁免: 豁免表行须绑定可核查条件(如目标批已产出 assertions>0 或前置模块已落地),每次运行自动求值;条件一旦满足即翻转为 stale 并判红,禁止只写版本号或到期日。
- (b) 单调棘轮不设绝对下限: 存量未报断言数批次纳入只降不升基线(cur_unreported <= baseline_unreported),单批不设硬编码 assertions>=K 下限,防止随合法用例增减退化为机械改数字。
- (c) 测量机制自测与变异: 共享断言入口自增计数并在总结行输出 assertions=N,守卫同时核对总结行 assertions=N 字段与流式实测 PASS<TAB> 行数;自测覆盖删总结行字段必红、删断言点自增必红、手写常量无断言行必红三个变异。
- (d) 全局汇总覆盖可见: 每次汇总显式打印 R69_6_SHADOW_COVERAGE reported=N unreported=M total=K,不静默略过未报批次。
- (e) tests/run-batch.cx 可行性核查: np_run 已将子进程完整 stdout 读入内存 result.out(result.outn 字节),第 207 行 fwrite 转出、第 208 行 marker 扫描;在此处复用流式行累加器扫描 PASS<TAB> 行与 assertions=N 属纯内存 O(result.outn) 操作,零额外进程与磁盘开销。
- (f) 跨块流式行累加器任意切分点不变性: pass_line_acc_feed 逐字节维护行首 PASS<TAB> 状态机与行缓冲,pass_line_acc_end 收尾无换行末行;在 3 行 24 字节含无换行末行样本上遍历全部 25 个单切分点、325 个双切分点与逐字节喂入,错误数均为 0;按当前块测前缀的变异实现单切分错误 12 处、双切分错误 234 处,必红被杀。

## 二、影子盘点实测数据

- 严格预跑证据表 (release/evidence/v0.0.90-strict-2026-11-22.tsv):
  - 去重总批数 total=459 (PASS=457, UNKNOWN=2)
  - 证据表 note 列已报 count=N/M 批数: reported=123, 未报批数: unreported=336
  - 若启用流式 PASS<TAB> 行累加器与正向计数字段扫描: 可覆盖 reported=434, 仍未报 unreported=25
- 全量注册表 (tests/batches.tsv IMPLEMENTED 批次):
  - IMPLEMENTED 总批数 total=1883 (涉及唯一源文件 unique_sources=978)
  - 具备正向断言计数信号(PASS<TAB> 行或 pass=/ok=/checks=/cases=/assertions=): reported=1664 (其中含 PASS<TAB> 批=1539, 含正向计数字段批=1092; 唯一源文件=802)
  - 未报正向断言计数信号: unreported=219 (其中仅输出 failed=0 无正向计数批=148, 完全无计数关键字批=71; 对应唯一源文件: 仅 failed=133, 完全无计数关键字=43)

## 三、严格预跑证据表中已报 count=N/M 的前 30 个批次样本

- properties-cases-det: status=PASS note=count=16/0 source=tools/properties-cases-gen.cx
- properties-cases-meta: status=PASS note=count=28/0 source=tools/properties-cases-gen.cx
- properties-cases-bound: status=PASS note=count=18/0 source=tools/properties-cases-gen.cx
- properties-cases-tu: status=PASS note=count=5/0 source=tools/properties-cases-gen.cx
- properties-cases-lower: status=PASS note=count=26/0 source=tools/properties-cases-gen.cx
- optimization-corpus-0-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-0-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-0-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-1-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-1-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-1-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-2-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-2-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-2-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-3-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-3-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-3-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-4-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-4-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-4-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-5-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-5-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-5-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-6-o0-o1: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-6-o-o2: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-corpus-6-o3-os: status=PASS note=count=28/0 source=tools/opt-corpus-gen.cx
- optimization-closure-0-4: status=PASS note=count=66/0 source=tools/opt-corpus-gen.cx
- optimization-closure-5-9: status=PASS note=count=65/0 source=tools/opt-corpus-gen.cx
- optimization-cli: status=PASS note=count=6/0 source=tools/opt-corpus-gen.cx
- properties-cases-random-0-7: status=PASS note=count=53/0 source=tools/properties-cases-gen.cx

## 四、严格预跑证据表中未报正向断言计数的批次清单(前 40 项)

- facts-current: status=PASS layer=interfaces only_fail_kw=0 source=tests/facts-current.cx
- speed-ledger: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-speed-ledger.cx
- snapshots-cx-core: status=PASS layer=interfaces only_fail_kw=0 source=tools/check-snapshots-cx-part.cx
- snapshots-cx-alloc: status=PASS layer=interfaces only_fail_kw=0 source=tools/check-snapshots-cx-part.cx
- check-asm-attr-reject: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-asm-attr-reject.cx
- check-posix-fix: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-posix-fix.cx
- check-netregen-expected-fail: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-netregen-expected-fail.cx
- check-gen-all-check: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-hang: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-inc-bare-relative: status=PASS layer=contract only_fail_kw=1 source=tools/check-inc-bare-relative.cx
- check-gen-all-pass: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-drift: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-ready: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-split: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-synth: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-unstaged: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-gen-all-part2: status=PASS layer=contract only_fail_kw=1 source=tools/check-gen-all-check.cx
- check-e3-located-reject: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-e3-located-reject.cx
- check-c-neg-f1: status=PASS layer=interfaces only_fail_kw=1 source=tools/check-c-neg-f1.cx
- check-register-layout-gate: status=PASS layer=contract only_fail_kw=1 source=tools/check-register-layout-gate.cx
- check-register-layout-env: status=PASS layer=contract only_fail_kw=1 source=tools/check-register-layout-gate.cx
- check-register-layout-clean: status=PASS layer=contract only_fail_kw=1 source=tools/check-register-layout-gate.cx
- check-ln-noderef: status=PASS layer=contract only_fail_kw=1 source=tools/check-ln-noderef.cx
- check-prerun-drive: status=PASS layer=contract only_fail_kw=1 source=tools/check-prerun-drive.cx
- check-release-state: status=PASS layer=contract only_fail_kw=1 source=tools/check-release-state.cx
