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

## 补:ratchets 17/1 的红项(2026-11-23 追查,只读)
10. v0.0.96 合并树 ratchets 唯一红项是 ref-job-paths:plans/v0.0.96.md baseline 1 exceeds 0 refs(STALE)。不是 ccogweco 提交的代码带来的 cxapis/cxtrunc 真红(那两项在该树 ok)。原因:v0.0.96 分支把 plans/v0.0.96.md 里的作业私有路径引用清零,而 build/ref-job-paths-baseline.tsv 合并取了 main 一侧,行 'plans/v0.0.96.md 1' 留成过期行。修法:合并后跑 tools/ref-job-paths.cx -- --write-baseline(只降不升);merge-version gate derive-write 现已自动带这一步,实测合并树 ratchets 18/0。

## 补:S0 当天负载前检
11. merge-version 每个 gate 入口先读 /proc/loadavg:load1 > 8(--max-load N 可调,默认 8)时输出 'MERGE_GATE NAME REFUSED load1=X max=8 ...' 并 rc 3,不开始跑。依据:derive-check 冷跑约 54 s 贴 55 s 线,负载高时会把负载造成的超时误报成 gate 失败。这是等待信号,不放宽任何时限。

## 补:陷阱清单表驱动
12. 陷阱文件清单与其适用版本线不再写在代码里:tests/merge-rules.tsv 的 `always` 行即陷阱(第 3 列路径,第 2 列版本线),第 2 列支持 `>=vA.B.C`(该线及更新,按数字比较,v0.0.100 新于 v0.0.96;非 vA.B.C 的分支名也受约束,畸形阈值不匹配任何分支),模块 tools/cxlib/mergebranch.cx 由 tools/merge-version.cx 与 tools/premerge-union.cx 共用。新增陷阱文件只改表。
