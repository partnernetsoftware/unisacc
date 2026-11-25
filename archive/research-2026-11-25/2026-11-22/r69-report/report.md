# R69 普查综合报告 (第 2-6 项 + SKIP/零断言自查, 2026-11-21)

## 一、R69 第 2-3 项: tool-inventory 实跑与 check-cx-* / *-gen 交叉核对
- 实跑 `sh ./unisacc.com tools/tool-inventory.cx --`:
  `TOOL_INVENTORY_COMPLETE files=1432 lines=280703 bytes=17381685 skipped_nonregular_or_missing=145 recent_requested=10 candidates=REPORT_ONLY`
  候选计数: `no-lexical-caller=61`, `no-implemented-batch-entry=289`, `missing-paired-check=278`。
- 61 个 `no-lexical-caller` 拆解:
  - 54 个实为经 `#pragma unisa use` 引用的夹具/模块(`tools/install-fixes/uses48/u1..u49` 等 51 个 + `symindex/fixture` 3 个 + `tools/refimage.cx`); 加上通用 module/use 边后真零入度仅 8 个。
  - 有效待接线: `tools/units-gen.cx` (实跑 `UNITS_GEN SAME bytes=2900 RC=0`)。
  - 疑似历史/他车道草稿进候选清理(不自动删): `tools/uxe-points-proj.cx`, `tools/check-network-real.cx`, `tools/nn-inputs-compare.cx`, `tools/p1-baseline.cx`, `tools/lab-loader/decode-once.cx`。
- 未接线 `check-cx-*`: 全仓 70 个中 28 个未登记(与 `no-lexical-caller` 交集 0), 抽跑 5 个(`code/build/deflate/word/axis`)全 `RC=0 PASS` -> 有效待 `register`。
- 未接线 `*-gen`: 全仓 103 个中 45 个未登记(与 `no-lexical-caller` 交集 1 仅 `units-gen.cx`), 其中 24 个执行根不可达(23 个枚举/表生成器 + `units-gen.cx`)待核并入 `derive`/`gen-all-check`。

## 二、R69 第 4 项: 传递可达性与二级死候选 (`KIND=dead-importer-only`)
- 根集合: R1(`tests/batches.tsv` 1838 条 IMPLEMENTED source+entry)、R2(`.claude/settings*.json` 钩子命令)、R3(`tools/flows/{chain-preflight,daily-ready,verify-tree}.tsv`)、R4(`precommit/ratchets/derive/derive-flow/devsvc*`)、R5 扩展根(`prd.md`)。
- 实测(含通用 module/use 边):
  - `REACH_EXEC_ONLY(R1..R4)`: `reachable=1343 unreachable=89 no_caller=8 dead_importer_only=81`
  - `REACH_WITH_PRD(R1..R5)`: `reachable=1347 unreachable=85 no_caller=8 dead_importer_only=77`
- 77 个 `dead-importer-only` 归类:
  1. 成对未接线 `*-gen` 与 `check-cx-*`: 42 个(仅被 `exec/facts/limits.tsv` 提及);
  2. 仅被 `build/posix-census-baseline.tsv`(漏列入 `tool-inventory.cx:13` 排除表)或历史文档提及的孤儿工具(约 22 个, 进候选清理): `archive-entry-probe`, `carried-header-export`, `core-transition-{profile,shapes}`, `gen-model-decode-material`, `generator-json-compare`, `link-container-cases`, `runtime-budget-cases`, `tape-fallthrough-cases`, `run-watchdog-bench`, `source-job-verify`, `kernel-emit`, `net-built-private`, `lab-loader/template`, `ucn-table`, `cx-{driver,pipeline,runtime,workspace}` 等;
  3. 仅被文档或不可达 `practice-*` 互引: 约 11 个;
  4. `reg=1` 但配对 check 未写字面路径: 2 个(`gen-s0-look.cx`, `gen-s0-profile.cx`)。

## 三、R69 第 5 项: 字面量与长度参数不符检查
- 详单见 `research/2026-11-22/litlen/real-bugs-41.tsv` 与 `summary.md`(4477 处调用, 179 处不符, 41 条真缺陷含 7 越界多读 + 4 多写尾 NUL + 30 偏小截断; 已转交 cdx 做检查器并优先修 `release-exec.cx:1605,1723`)。

## 四、R69 第 6 项: 零断言守卫与 SKIP/DEFERRED 自查 (最高优先, 0.0.89 后先经守门人评审)
- `tests/run.cx -- --list`: `batches=1865 (IMPLEMENTED=1838, PLANNED=27)`, `coverage=1509 (unresolved=17, 仅 coverage.tsv:426 标 DEFERRED, 425 标 NOT RUN)`。
- 关键框架缺口: `tests/run-batch.cx:208` 与 `tools/cxlib/allsession.cx:425` 判 PASS 仅看 `rc==0` + `product_digest` 不变 + 1 行 MARKER, 全链路无 `assertions>0` 或不含 `SKIP/NOT_RUN` 校验。
- 948 个去重 IMPLEMENTED batch 源文件实测:
  - `no_count_kw_files=348 (36.7%)` 完全无断言计数关键字;
  - `failed_without_zero_guard=5`;
  - `tools/check-win-argv.cx:90` 在非 win 宿主打印 `native=NOT_RUN_FOREIGN` 仍 `return 0` 判 PASS。
- 已定方案(先文字+守门人评审, 0.0.90 前落): 批结果行带 `assertions=N`, 新批 `N=0` 判红, 存量 348 进只降不升基线, `NOT_RUN`/外来平台记 `NOT_APPLICABLE` 不得记 PASS。
