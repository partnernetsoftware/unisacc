# 0.1.0：precommit-list-cap 余量

本车候选 `b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`，冻结树 `a11d28fc0a61939ee71345d732a7dcf205b261f1`。旧 `strict-W0022.tsv` 只有表头，原样保留；`supervise.log` 记载 W0022 两次单批外窗分别 `rc=124 wall_ms=58037`、`rc=124 wall_ms=58034`，均无 RAW，不作 G1 凭据。G1 凭据来自同布局 `tree-6cve4rtr`、低负载 `load1=1.43` 单批补跑 `strict-W9922-precommit-list-cap.tsv`：

```text
precommit-list-cap	product-qualification	UNKNOWN	0	b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	55587	1.43	0	transport-failed-raw-neg1
RAW	BATCH_RUN	id=precommit-list-cap	status=UNKNOWN	rc=0	transport=-1	marker=CHECK_PRECOMMIT_LIST_CAP	candidate=b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f	headers=tree	scope=actual-one-batch-no-cached-result	assertions=UNMEASURED
```

107 对照：W9915、W9915b/c/d 四次均 UNKNOWN，分别 55557、55530、55486、55492 ms；后三次为独立低负载对照（load1 1.82、1.93、2.21），c/d 同布局。108 前的复现记录见 107 发布证据 `release/evidence/0.0.107/list-cap-margin.md`。夹具串行四次跑真实提交门，反复贴 `tests/run-batch.cx:264` 的约 53 秒子进程限；正式批登记 55 秒，58037/58034 ms 是外窗壁钟，不能混用。结论为设计性余量不足；单个提交门的编译与运行没有单列测时。`rc=0` 不能解释为检查正常退出，也没有断言通过结论。

严格表保留 UNKNOWN 和完整 RAW；不登记 expected-fail，不放宽时限。已在 main 的 G1 修复 `2d387e524d` 支持具名 `window-timeout`。正式目录 `release/evidence/0.1.0/` 下本页保存为 `list-cap-margin.md`，`expected-unmeasured.tsv` 精确登记：

```text
precommit-list-cap	window-timeout	list-cap-margin.md
```

拆批排期见 `research/ompgpt/p0-prep-window-plan.md` 的 `precommit-list-cap` 项：拆四次真实提交门，保留原断言，冷态每片三次最大不超 27.5 秒，55 秒上限不变；不改本封版车。
