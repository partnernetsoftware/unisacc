# v0.0.96 合并预演更新(只读,2026-11-23,omp-sonnet)

输入身份:main 头 a5169cbf21,v0.0.96 头 f7e9069324(已提交头,ccogweco 的 (d) 整改未含),v0.0.94 头 2735df3efe。merge-base(main,v0.0.96)=a206d09fa1(v0.0.96 比 main 少 203 提交,多 36 提交);merge-base(v0.0.94,v0.0.96)=d5ed6f6869(v0.0.96 含 v0.0.94 的 Z3/Z3b/Z4/Z5/Z6 早期提交)。
方法:git 2.34 无 `merge-tree --write-tree`,改用私有索引 `git read-tree -i -m --aggressive BASE OURS THEIRS` 找未决路径,再逐路径 `git merge-file -p` 判冲突;不写共享检出、不动 refs(仅写了一个悬空 commit 对象 d22f7d455b 作场景二的父)。私有目录 ~/.claude/jobs/omp1/tmp/mp。

## (1) 直接 main <- v0.0.96:13 个路径两侧都改,10 个冲突

| 文件 | 结果 | 原因 |
|---|---|---|
| tools/derive.cx | 冲突 3 处 | main 加 140 行、v0.0.96 加 64 行。47f1869765(v0.0.96 的 4 路池)已被移植到 main 为 0afffd585d,同一改动两个提交号,加上 main 后续 ee38149ecc、c5846283cc 又改同区 |
| exec/facts/derive-weights.tsv | 冲突 1 | 同上:权重表 47f1869765 与 0afffd585d 各加一份 |
| build/links.tsv | 冲突 2 | 两侧各追加批次行(main 14,v0.0.96 15),尾部追加冲突 |
| tests/batches.tsv | 冲突 2 | 同上,两侧各加批次行 |
| tests/coverage.tsv | 冲突 1 | 同上 |
| tests/tools-compile-sources.tsv | 冲突 1 | 同上 |
| build/tlib-packages.tsv | 冲突 1 | v0.0.96 改 6 行,main 改 1 行(tools.cxlib 版本与摘要列) |
| build/ref-job-paths-baseline.tsv | 冲突 1 | main 改 1/4,v0.0.96 改 1/2 |
| plans/v0.0.94.md | 冲突 2 | v0.0.96 改 132 行(范围收缩与版本计划),main 改 2 行 |
| plans/v0.0.96.md | 冲突 3 | 两侧都新建或改写该文件 |
| build/iface-hash.tsv、build/lane-inputs.tsv、tools/prerun-drive.cx | 自动合并 | 无冲突,但 iface-hash 是派生表,合并后须重算 |

处理规则:表类(links、batches、coverage、tools-compile-sources)取并集(两侧新增行都留);derive.cx 与 derive-weights.tsv 以 main 为准(丢 v0.0.96 的 47f1869765 重复部分,保 main 的 0afffd585d 与后续);plans 文件以版本分支为准;派生表一律重算不手并。

## (2) 先把 v0.0.94 合入 main,再合 v0.0.96

场景一步:main <- v0.0.94 只有 5 个路径两侧都改(build/iface-hash.tsv、build/tlib-packages.tsv、tests/batches.tsv、tests/coverage.tsv、tests/tools-compile-sources.tsv),全部自动合并,0 冲突(与 S0-3 清单预期一致)。

场景二步:再把 v0.0.96 合入。21 个路径未决,20 个冲突(含修改/删除 1 个),std-surface 与 prerun-drive 自动:
- 冲突:build/iface-hash、build/layer-api/cx.os、build/layer-api/ext.sysx、build/links、build/ref-job-paths-baseline、build/state-unverified、build/tlib-packages、cx/sys/proc.cx(3 处)、ext/sysx/processcore.cx、ext/sysx/reaper.cx(4 处)、plans/v0.0.96.md(3)、tests/batches、tests/coverage、tests/tools-compile-sources、tools/check-cx-sys-spawn.cx、tools/check-reaper.cx、tools/cxlib/releaseexec.cx、tools/cxlib/releaseprepare.cx、tools/check-l0-capture.cx(v0.0.94 删除而 v0.0.96 改,修改/删除冲突)。
- 成因:3970fc6d5a(0.0.94 范围收缩,撤出 Z3/Z3b/Z5/Z6)把 v0.0.96 承接的进程生命周期代码从 v0.0.94 里撤掉,v0.0.96 则保留并扩展。

3970fc6d5a 的 8 个高危静默陷阱文件,在「naive 合并」树(未决路径取 merge-file 输出含冲突标记,可 trivial 合并的取合并结果)上跑 `premerge-union --shard merge-traps --branch v0.0.96 --tree TREE` 的原行:

```
MERGE_TRAP	FAIL	tests/manifests/l0-capture.tsv	reason=missing-in-tree
MERGE_TRAP	FAIL	ext/sysx/exec.cx	reason=differs-from-branch
MERGE_TRAP	FAIL	ext/sysx/processcore.cx	reason=differs-from-branch
MERGE_TRAP	FAIL	ext/sysx/reaper.cx	reason=differs-from-branch
MERGE_TRAP	FAIL	tools/check-reaper.cx	reason=differs-from-branch
MERGE_TRAP	FAIL	build/layer-api/ext.sysx.tsv	reason=differs-from-branch
MERGE_TRAP	FAIL	tools/cxlib/releaseexec.cx	reason=differs-from-branch
MERGE_TRAP	FAIL	tools/cxlib/releaseprepare.cx	reason=differs-from-branch
MERGE_TRAPS_SUMMARY	verdict=FAIL	branch=v0.0.96	ok=0	waived=0	fail=8	total=8
```

要点:ext/sysx/exec.cx 与 layer-api/ext.sysx.tsv 不在冲突列表里,却不等于 v0.0.96:它们只被 0.0.94 一侧改过,git 静默取 0.0.94 的范围收缩版本,这就是静默陷阱。l0-capture.tsv 被 0.0.94 删除,树里直接缺失。
把这 8 个文件整体取 v0.0.96 版(`git checkout v0.0.96 -- 8 个路径`)后同命令原行:`MERGE_TRAPS_SUMMARY	verdict=PASS	branch=v0.0.96	ok=8	waived=0	fail=0	total=8`。结论:合并后必须对这 8 个文件显式取 v0.0.96 侧,并把 merge-traps 作为合并提交前的硬门;tools/check-l0-capture.cx 与 tests/manifests/l0-capture.tsv 也要取回(v0.0.96 里有)。

## (3) v0.0.96 合并后必须重算的派生表

两侧在这些表上都有改动,合并结果不可信,须用生成器重算:build/iface-hash.tsv、build/layer-api/cx.os.tsv、build/layer-api/ext.sysx.tsv(取回 v0.0.96 后与 cx.sys、cx.os 的接口同步)、build/std-surface.tsv、build/links.tsv、build/tlib-packages.tsv、build/table-index.tsv(main 侧已变)。命令:`sh ./unisacc.com tools/derive.cx -- --write`,之后 `-- --check` 全 OK;build/state-unverified.tsv 与 build/ref-job-paths-baseline.tsv 不是派生表,按行并集手并(state-unverified 用 v0.0.96 的 U31/U32/U33 承接行)。

v0.0.96 已提交头 f7e9069324 的干净树(git archive,unisacc.com 链到共享检出)实跑原行:
- check-proc-lifecycle:25 PASS、0 FAIL、3 GAP(live.noproc-liveness-honest、live.subreaper-missing-must-refuse、anchor.needs-pid-namespace)。注意:是 3 个 GAP,不是 2 个;(d) 整改提交后再重演并核 GAP 数。
- check-reaper:`CX_REAPER	ok=27	failed=0`,另有两条 NOTE 原行(nproc 上限 2512;np_run 抽样)。

整改提交(ccogweco 的 (d))后须再演一次:check-proc-lifecycle 的 PASS 与 GAP 数、check-reaper 总数、merge-traps 8/8、derive --check。
