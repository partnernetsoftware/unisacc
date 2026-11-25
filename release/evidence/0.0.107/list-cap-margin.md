# W0015 / W9915：precommit-list-cap 余量

最终判定：W9915c/d 固定布局 `tree-q7bjblqd`、同候选和源码、低负载连跑仍贴限。登记没有外部材料，tree 模式不走 kit 预链接，且夹具每轮新建最小仓串行跑四次真实提交门；按设计性贴限处置，内层各相仍无独立计时。106 无同批基准。

## 只读证据

目录 `~/.cache/unisacc/prerun/v0.0.107-63354b4a2c`，候选 `b773c7e2…`。

- `supervise.log`：W0015 有证据的两次 `jobs=1 rc=124`，`wall_ms=58032 sum_batch_ms=55501 outer_overhead_ms=2531` 与 `wall_ms=58037 sum_batch_ms=55488 outer_overhead_ms=2549`。`strict-W0015.tsv`：`UNKNOWN rc=0 ms=55488 note=transport-failed-raw-neg1`。
- W9915 首次补跑 kit 979 ms 后仍 UNKNOWN 55557 ms，load1=7.49。低负载热态对照 `strict-W9915b-precommit-list-cap.tsv` 原行：
  ```text
  precommit-list-cap	product-qualification	UNKNOWN	0	b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	55530	1.82	0	transport-failed-raw-neg1
  RAW	BATCH_RUN	id=precommit-list-cap	status=UNKNOWN	rc=0	transport=-1	marker=CHECK_PRECOMMIT_LIST_CAP	candidate=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	headers=tree	scope=actual-one-batch-no-cached-result	assertions=UNMEASURED
  ```
  stderr 已见布局复用、kit 942 ms；降负载未解除该次贴限，但内层四次提交门的材料命中未核实。`rc=0` 不是检查器正常退出的证据。
- 原始证据入口：`strict-W9915-precommit-list-cap.tsv` 55557 ms、`strict-W9915b-precommit-list-cap.tsv` 55530 ms、`strict-W9915c-precommit-list-cap.tsv` 55486 ms（load1=1.93）、`strict-W9915d-precommit-list-cap.tsv` 55492 ms（load1=2.21）。后两次同布局，均为 UNKNOWN。W9915d 是 M1 折叠采用的终态，以下 RAW 完整原行：

RAW	BATCH_RUN	id=precommit-list-cap	status=UNKNOWN	rc=0	transport=-1	marker=CHECK_PRECOMMIT_LIST_CAP	candidate=b773c7e221ab059e935276f41c0ac4070737f2ae6c548cd56ea8b6b0acc53c63	headers=tree	scope=actual-one-batch-no-cached-result	assertions=UNMEASURED

- `tests/batches.tsv:2031`：正式入口 `tests/check-precommit-list-cap.cx`，tree/daily，55 秒。`tests/run-batch.cx:264` 的约 53 秒是 checker 子进程预算，55 秒是批登记／资格窗口；约 58 秒是 W0015 外窗实测。它们不是四个子用例的相位计时。
- `tests/check-precommit-list-cap.cx:82–127`：私有仓准备与五条 Git 命令后，在同一夹具串行运行四次真实 `sh ./unisacc.com tools/precommit.cx --`。前两次使用同一大输出入口，分别验证 1MiB 后的具名拒绝与收紧到 1MiB 后的具名拒绝；后两次各重写、重暂存 `tests/run.cx`，分别验证有效元数据放行与无效元数据具名拒绝。最后才清理并打印 checker 汇总。现存日志无法分离 checker 启动、Git、四次门内编译与执行、清理；本轮未重跑。

## 107 处置

保留各窗原始 strict、RAW 与 stderr。M1 保留 W9915d UNKNOWN，不以 expected-fail 隐去、不放宽时限、不删断言。

旧 G1 无匹配。G1 v2 若经审落车，将本页原样复制到 `release/evidence/0.0.107/list-cap-margin.md`，登记一行（制表符分列）：

```text
precommit-list-cap	window-timeout	list-cap-margin.md
```

## 0.1.1：余量队列四加一

与 `slow-split-margin-design.md` 的四个组级证明批一起排队；第五项是 list-cap 家族，不把它加入 slow-split 证据表。先拆大输出组与小元数据组，两组各起独立最小仓，保留四个用例、原具名断言和原组内顺序；按 manifest 登记两批并重生 batches/coverage，覆盖键映射到两组。若仍贴限，再拆单用例，或把真实 precommit 执行体按候选、容器、闭包与配方准备为可校验材料；_toggle、Git 暂存和 selector 调用仍走真实路径，不缓存结论。阶段计时分别记录材料准备、夹具 Git、各次提交门、清理和批总壁钟。

每片正式 jobs=1，固定候选与源码身份，默认短 TMPDIR，材料独立预备后关闭运行缓存，冷跑三次均不超 27.5 秒；55 秒登记不变。核旧断言 ID 多重集（公共初始化断言单列比较），撤掉上限拒绝须让大输出组转红，小组正反合同均实际执行。触 tests 登记与检查源码，0.1.0 封版不纳入。
