# 0.1.1：precommit-list-cap 余量

本车候选 `d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb`，冻结树 `b3e9731b6ada2e5ffb34f5a74061e5e8cb1dfa64`。G1 凭据来自原预跑单批窗 `strict-W0014.tsv`（`load1=5.68`，保留 UNKNOWN `55531` ms 与完整 RAW）：

```text
precommit-list-cap	product-qualification	UNKNOWN	0	d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb	55531	5.68	0	transport-failed-raw-neg1
RAW	BATCH_RUN	id=precommit-list-cap	status=UNKNOWN	rc=0	transport=-1	marker=CHECK_PRECOMMIT_LIST_CAP	candidate=d83537aef92eb55ed94d81081b0cff27d146a0ef0d83b2a544b989c15701c3eb	headers=tree	scope=actual-one-batch-no-cached-result	assertions=UNMEASURED
```

夹具串行四次跑真实提交门，贴 `tests/run-batch.cx:264` 的约 53 秒子进程限；正式批登记 55 秒。结论为设计性余量不足；严格表保留 UNKNOWN 和完整 RAW；不登记 expected-fail，不放宽时限。正式目录 `release/evidence/0.1.1/` 下本页保存为 `list-cap-margin.md`，`expected-unmeasured.tsv` 精确登记：

```text
precommit-list-cap	window-timeout	list-cap-margin.md
```
