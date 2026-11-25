# 重起链预跑 priority-file 草案(107 新增/改动批置顶;2026-11-25 ccogweco)

目的:重起链后的第一次预跑,把 107 车相对 106 新增或改动的批排到最前,让 W0201 型红在头几分钟暴露。
只写文件内容与说明,不执行预跑。

## 0 取数命令原行

    cd ~/repos/unisacc/.claude/worktrees/v0.0.107
    git diff 2e8a19a342 28b6a518fe -- tests/batches.tsv | grep -E '^[+-][a-z0-9]' | cut -c2- | cut -f1 | sort -u
    # 20 个 id
    D=~/.cache/unisacc/prerun/v0.0.106-2e8a19a342
    for id in <ids>; do grep -P "\t${id}(,|\t|$)" "$D/windows.tsv" | head -1 | cut -f1; done
    # 106 原窗号(无输出=106 时不存在,即 107 新增)

## 1 差异批清单(20)与 106 原窗

| 批 | 106 原窗 | 类别 |
|---|---|---|
| c99-array-param-typedef | 无(107 新增) | 产品 C99 |
| c99-global-redecl-conflict | 无 | 产品 C99 |
| c99-tentative-array-incomplete | 无 | 产品 C99 |
| fwd-cap | 无 | 产品 F1 |
| fwdstub-replay | W0191 | 产品 F1(106 已有) |
| module-units | W0085 | 检查拆片 |
| cxmod-tri | 无 | 检查拆片 |
| release-exec-g | 无 | 发布工具 |
| install-ape / install-ape-split2 | 无 | 发布工具材料 |
| precommit-list-cap | 无 | 提交门 |
| seal-precheck | 无 | 封版工具 |
| tsv-lint / tsv-lint-root | 无 | 材料/检查 |
| ratchets-tool / ratchets-tool-full | 无 | 材料/检查 |
| batch-timing | 无 | 材料/检查 |
| fixedpoint-evidence | 无 | 证据工具 |
| cxmod-tri-exports | 无 | 检查拆片 |
| module-units-forward / module-units-limits | 无 | 检查拆片 |

说明:fwdstub-replay 在 106 已存在(故不在 diff),但它是 F1 的核心 watchdog,仍置顶;U08 的批行未变
(改的是检查源),不在本清单。

## 2 priority-file 内容(照 prerun-drive 的格式)

格式(prerun-drive.cx:446-518):每行 `id<TAB>2` 为最高优先(tier 2,显式风险/非 PASS 最先),
`id<TAB>1` 为次级;不带第二列默认 2;`#` 注释;空行跳过。排序时 tier 2 先于 tier 1(源码 :726-757)。

    # 107 重起链:本车新增/改动批置顶(tier 2),产品与检查优先
    c99-array-param-typedef	2
    c99-global-redecl-conflict	2
    c99-tentative-array-incomplete	2
    fwd-cap	2
    fwdstub-replay	2
    module-units	2
    cxmod-tri	2
    cxmod-tri-exports	2
    module-units-forward	2
    module-units-limits	2
    fixedpoint-evidence	2
    seal-precheck	2
    precommit-list-cap	2
    release-exec-g	2
    install-ape	2
    install-ape-split2	2
    tsv-lint	2
    tsv-lint-root	2
    ratchets-tool	2
    ratchets-tool-full	2
    batch-timing	2

## 3 排序后的预计位置

- 20 个 tier 2 批在 warmup 与早期 canary 之后先排;非 heavy 的优先级批按窗打包,heavy 的优先级批在 heavy
  段最前(:757)。按 106 的窗打包密度(jobs=1 的 heavy 单窗、long 可多窗),预计占用**前 12 到 20 个窗**。
- W0201 若仍在,应在预跑前 10 分钟内暴露,而不是 202 窗后。
- 之后才轮到普通 heavy/long/normal;总的窗数仍是 247 上下,不因排序减少覆盖。

## 4 使用方式与注意

    # 重起链后的预跑(agy-flow 执行)
    sh ./unisacc.com tools/prerun-drive.cx -- run --root <FROZEN_ROOT> --candidate <APE> --out <PRERUN_DIR> \
        --resume --keep-going --window-lanes 2 --priority-file research/ccogweco/priority-newbatches-107.txt

- 文件落点建议:`research/ccogweco/priority-newbatches-107.txt`(本页正文即内容;由 agy-flow 复制到车树或直接引用)。
- priority 只改顺序,不改窗数、上限与并发;lane 仍 2。
- 若某批 ID 在 107 登记里拼写不同(如带 -root 后缀),以 tests/batches.tsv 实际 id 为准,预跑前用
  `grep -c` 逐条核存在,不存在则从文件删除并记一行。

## 5 未定

- 107 的窗表尚未生成(重起链后才有),本页的"前 12-20 窗"是推断,不是实测。
- fwd-cap 与 C99 批的 heavy/long 归属以 107 windows.tsv 生成为准。
