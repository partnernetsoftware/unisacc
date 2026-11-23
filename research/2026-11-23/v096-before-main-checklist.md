# v0.0.96 入 main 前必改清单(2026-11-23,omp-sonnet,只读预演,未动 v0.0.96 工作树)

依据:tools/merge-version.cx 预演 main(a5169cbf21) <- v0.0.96,规则表 23 行,conflicts_left=0;在带 .git 的私有检出树里跑 derive-check。

## 必改(textual 合并看不出,合并后 derive --check 红)
1. plans/v0.0.96.md 首行主题与 main 的 build/version-seq.tsv 0.0.96 行不一致:verseq-gen R2 FAIL,首行"进程生命周期统一"不是槽表 class("执行核每步成本 19.8 档...")的子串。槽冲突(cc 已标),在重排点定:改计划首行或改槽表。
2. build/tlib-packages.tsv 三包 deps 缺单元(layer-api FAIL,derive --write 不会补):ext.textx(ext/textx/json.cx 用 cx/text/num.cx)、ext.sysx(ext/sysx/process.cx 用 cx/sys/signalnums.cx)、tools.cxlib(同类 outside-package-and-deps)。补 deps 后 layer-api.cx --check 再核。

## 规则表已处置(不需人工,但合并时知道)
3. exec/facts/derive-weights.tsv:两侧各自 cherry-pick 同一改动两次(47f1869765 与 0afffd585d),另 v0.0.96 有额外信号目标等权重行;规则 union,取 take-ours 会缺行(derive REFUSE missing-rows)。
4. 5 个陷阱文件(ext/sysx/exec.cx、processcore.cx、reaper.cx、tools/check-reaper.cx、build/layer-api/ext.sysx.tsv、tools/cxlib/releaseexec.cx、releaseprepare.cx、tests/manifests/l0-capture.tsv)按"always 取分支侧"强制;naive 合并后 merge-traps 8 项 FAIL(7 项 differs-from-branch、1 项 missing-in-tree)。
5. build 派生表(iface-hash、links、tlib-packages、std-surface 等)合并后须 derive --write 重算,不取任何一侧。

## 其它发现(非阻塞,需知)
6. batch-packages-gen 在 gate 冷跑 30s 超时(rc=142),热后过;derive-check 冷跑 51s 报 refused=2,热跑全 OK:正式跑前先热一遍。
7. product-map 偶发 REFUSE readset-changed-before-output(树内并发改读集),重跑可过。
8. ratchets gate 在树内 55s 超时(UNMEASURED),需分片。
9. check-proc-lifecycle 在 v0.0.96 树 25 PASS、3 GAP(live.noproc-liveness-honest、live.subreaper-missing-must-refuse、anchor.needs-pid-namespace),check-reaper CX_REAPER ok=27 failed=0;GAP 为已知,不是合并造成。

## 验收
合并后树:derive-write drift=0 refused=0;derive-check ok=17;traps ok=8(或 NA 项明示);tests metadata_valid=1;ratchets 18/0。
