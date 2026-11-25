# WP6 + WP8 拆批方案(ccogweco,只读核对,未跑冷态)

依据 research/agy-h/margin-packages.md 第 2 节工作包定义;每条都重读了入口源码,计时取自 research/agy-h/margin-audit.tsv 与 tests/commit-tier-timings.tsv。命令原行见文末。

## WP6(5 批)

| 批 | 层级/预算 | 现测 | 根因判定 | 证据(源码行) | 处置 | 预计冷态 | 写域 |
|---|---|---|---|---|---|---|---|
| cx-booter-ape | release/55 | 45.69s KILLED(另挂 no-BATCH_RUN) | C 第二工具链 + A:两次 gen 各跑 release-ape.cx,再调 kernel-route.cx 现编 x86_64.kernel | tools/check-booter-ape.cx:54,60(两次 np_run 跑 release-ape),:111(kernel-route.cx 现编) | 材料相:预建 {release-ape-tool};kernel 用独立 {x86_64-kernel} 材料(需补 booter-raw-elf 生产者,prd_mempalace:151 已记) | 15-24s(推断) | tools/check-booter-ape.cx、tests/batches.tsv、tests/batch-product-inputs.tsv |
| cx-npfault | daily/55 | 44.65s KILLED,峰 53.95 | C 嵌套第二运行器 | tools/check-npfault.cx:47 起 `sh ./unisacc.com tests/run-batch.cx` | 复用 {run-batch-tool} 或预建 run-batch 原生件;绝不能把内部 run-batch 再当批跑 | 10-20s(推断) | tools/check-npfault.cx、tests/batches.tsv |
| s8c-bind | daily/55 | 46.46s KILLED,峰 55.78 | A 现场编译 | tools/check-s8c-bind.cx:7 `sh ./unisacc.com tools/s8c-bind.cx`(子进程冷编 s8c-bind) | 机械改:材料 NAME-tool:path 规则(tools/affected.cx:684 把 NAME-tool 映射到 tools/NAME.cx 并预编)可用 `s8c-bind-tool:path`;但只省一次子编译(约 4-8s),46s 仍超,须同时把用法拒绝与实际绑定拆两批 | 拆后 12-20s(推断) | tools/check-s8c-bind.cx、tests/batches.tsv、tests/manifests/s8c-bind*.tsv |
| wt-gate-clean | daily/55 | 44.66s KILLED,峰 54.02 | A 并行现编两份原生执行器 | tests/check-wt-gate.cx:100 `sh ./unisacc.com tests/run-batch.cx -o ... & sh ./unisacc.com tools/all-waves.cx -o ...` | {all-waves-tool} 可走 NAME-tool;tests/run-batch.cx 不属 tools/,NAME-tool 回退只到 tests/cases/,故 run-batch 需先加材料规则或预建副本;建议拆 clean 与 native 两片 | 两片各 12-20s(推断) | tests/check-wt-gate.cx、tests/batches.tsv、tests/batch-product-inputs.tsv |
| dev-product | daily/55 | 39.09s UNKNOWN(commit-tier 39s 窗) | C 第二工具链 | tools/check-dev-product.cx:213-219 连跑 `--seed-gen` 与 `--seed-gen=gcc-O2` 两套构造 | 拆 dev-product-seedgen(两种 seed 生成各一批);无近期单跑,先冷测一次再定 | 15-24s(推断) | tools/check-dev-product.cx、tests/batches.tsv、tests/manifests/dev-product*.tsv |

WP6 材料 vs 分片:cx-booter-ape、dev-product 先拆;C 类分批收益大于材料;cx-npfault 只做材料复用;s8c-bind、wt-gate-clean 材料+分片都要。

## WP8(4 批)

| 批 | 层级/预算 | 现测 | 根因判定 | 证据 | 处置 | 预计冷态 | 写域 |
|---|---|---|---|---|---|---|---|
| slice-inventory | daily/55 | 39.08s UNKNOWN | A 现编两工具 + B 真实 11 源编译 | tools/check-slice-inventory.cx:75 `rn_prepare(...tools/symindex.cx...)` 与 `...slice-inventory.cx...`;同函数内 11 个源逐个 `-c` | 已有 `--prepare` 相(批次 materials=regprep-dir):把两工具也纳入材料(NAME-tool:path 两条),prepare 相只产出、consume 相复用;11 源编译属真实工作,留主相 | 12-20s(推断;prepare 相与主相分开计) | tools/check-slice-inventory.cx、tests/batches.tsv、tests/manifests/slice-inventory* |
| flow-tool | daily/55 | 39.06s UNKNOWN | B 真实工作量大(全库 flow --graph 与 check) | tools/check-flow.cx 全库扫描 | 先冷测一次;若 >27.5s,按目录拆 flow-tool-src 与 flow-tool-docs | 复测可能已 <25s | tools/check-flow.cx、tests/batches.tsv |
| skill-cx | daily/55 | 39.06s UNKNOWN | C 多段 shell 链 | tests/check-skill-cx.cx:63-70 每个代码块 `sh -c`(SK_SECS 上限) | 按文档节拆两批(命令块/示例块),不改判据 | 12-20s(推断) | tests/check-skill-cx.cx、tests/batches.tsv、tests/manifests/skill-cx* |
| signal-targets | daily/55 | 39.14s UNKNOWN | B,已拆父批 | tests/split-parents.tsv:3 指向 gen/posix/win 三片 | 父批仍在 daily 表;确认默认排除策略对父批生效(排除条件核对 tests/split-parents.tsv 与 run-batch 选择),不新建片 | 无需改(父批不跑即不超) | tests/split-parents.tsv、tests/batches.tsv(核对,未必须改) |

WP8 无在飞补丁冲突(第一/二车与 0.0.125 均未触这些文件);slice-inventory 与 skill-cx 是 WP8 里唯二需要改源码的。

## 是否写私有补丁

看了 ompgpt module-units 补丁形态:材料规则是批次行 `materials` 列写 `NAME-tool:path`、`args` 里放 `{NAME-tool}`;生产者由 tools/affected.cx:684 的 NAME-tool 规则自动预编 tools/NAME.cx,不需要入口自己实现 dump。据此最机械的一件是 s8c-bind(`s8c-bind-tool:path` 一条材料 + 入口接受 argv 路径),但它只省一次约 4-8s 的子编译,46s 仍超预算,价值不足,我没有写这件补丁。真正能进预算的是 slice-inventory(材料两条 + 已有 prepare 相拆分)与 wt-gate-clean(需先给 tests/run-batch.cx 找材料路径),两件都要求入口改参数与批次行,建议解冻并做一次冷测后再写,避免无测量瞎改。

## 命令原行

    sed -n '50,65p;105,115p' tools/check-booter-ape.cx
    sed -n '42,52p' tools/check-npfault.cx
    sed -n '1,12p' tools/check-s8c-bind.cx
    sed -n '95,105p' tests/check-wt-gate.cx
    sed -n '208,222p' tools/check-dev-product.cx
    sed -n '70,80p' tools/check-slice-inventory.cx
    sed -n '60,75p' tests/check-skill-cx.cx
    grep -F '<批名>' research/agy-h/margin-audit.tsv          # 计时/峰值/KILLED 来源
    grep -P '^<批名>\t' tests/batches.tsv                     # materials/args/budget 登记
    grep -n 'signal-targets' tests/split-parents.tsv
    sed -n '680,690p' tools/affected.cx                        # NAME-tool:path 规则

## 附:wt-gate-clean 的 run-batch 材料路径两个备选(链后由 cc 定)

现状:tests/check-wt-gate.cx:100 在并行步里同时 `sh ./unisacc.com tests/run-batch.cx -o ...` 与 `sh ./unisacc.com tools/all-waves.cx -o ...`,两份都现场冷编;tools/affected.cx:684 的 NAME-tool 只认 tools/NAME.cx(回退 tests/cases/NAME.cx),tests/run-batch.cx 不在这两个位置。

备选 A(改材料规则,改一处通用代码):
- 在 tools/affected.cx 的 prep_source 回退链里追加 `tests/NAME.cx`(在 tests/cases/NAME.cx 之前或之后),批次行即可声明 `run-batch-tool:path`(解析到 tests/run-batch.cx)与 `all-waves-tool:path`;入口 tests/check-wt-gate.cx 接受 `--run-batch PATH --all-waves PATH`,并行步直接用材料路径。
- 影响面:改的是全仓材料解析(所有批共用),要有负控(名字冲突、非 tools/ 名、sha 一致)与 tools/affected.cx 自身批次回归;风险中,收益一次解决同类"runner 在 tests/"的批。
备选 B(不改通用规则,拆批+入口自预编):
- 把 wt-gate-clean 拆成 wt-gate-clean(clean 路径)与 wt-gate-native(native 路径)两批;native 批在入口里先顺序 `-o` 编一次 run-batch 与 all-waves(复用 all-waves 的 NAME-tool 材料,run-batch 仍现场编一次),再执行,不再并行两路各编一次;clean 批只留无需原生件的路径。
- 影响面:只改 tests/check-wt-gate.cx 与 tests/batches.tsv(tests/run-batch.cx 不动),风险低,但 run-batch 仍有一次现场编译,冷态收益小于 A。
