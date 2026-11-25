# 交接要点:0.1.0 说明回填(ccogweco,2026-11-25)

上下文已 81%;本页只交接"0.1.0 说明还差什么、去哪儿取值、哪些已定稿不要重做"。写后空闲待命,不接重活。

## 1 当前文件与状态

- 目标页:`research/ccogweco/notes-0.1.0-final.md`(终版 sha 85a29f75;含公开后更正两处、Y1/Y2/T4 回填、条件 6 实测 175.4 分钟)。
- 已定稿部分(不要重做):
  - 零改动三硬断言与"重封"口径(产品闭包/六切片/compiler.p4.pkg/pkg.bin 字节相同;APE 由同一输入加版本槽重封,
    裸字节差异远大于 32 字节);该口径已扫全目录,旧"仅版本槽不同"表述已清零。
  - 准入七条、已知缺口(list-cap 设计贴限、三批余量 main 50251/余 2749、rd 49898/余 3102、
    slow-split 51144/余 1856)、起车缺陷三行、顺延 0.1.1-0.1.4。
- 已填值:F5=`a11d28fc0a…`;T1=08:59:43(1795568383);T2=09:26:30(1795569990,exit rc=0);用时 26.8 分钟;
  APE5=`b7cae17b1ae557a4907247e3168db56925504819f98d15f1b75a2da2de8fa97f`;
  fixedpoint 四同 102/2/6/same、package `c4120564…`;六目标 g2/g3 前 16 位清单在页内。

## 2 仍待回填的占位(与取值位置)

| 占位 | 取值页/命令 | 何时 |
|---|---|---|
| `<CI_RUN_ID>`(状态行与身份表两处) | research/ccogweco/v010-notes-backfill.md 第 1 节;`gh run list` 按 headSha | pack 后派 CI |
| `<SID5_64HEX>` | 同页;pack 输出 `RELEASE_RUN	SOURCES_SID	VERIFIED` | pack 后 |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | 候选表 sha 列与 layout | pack 后 |
| 公开时刻 | INDEX `exec-5-publish` | 公开后 |
| T3_R6 | `CHAIN RESULT READY_TO_EXECUTE` 输出时刻 | X 前 |
| strict 计数 | 合并 strict 的 SUMMARY | 预跑后 |
| Y1/Y2 | INDEX `exec-4-Y1`/`exec-7-Y2` | 公开后 |
| 条件 6 的 T1 到 T4 分钟 | T1 已填;T4 上表 | 公开后 |

- 链段取值命令:`v010-chain-fill-script.md`(已按 107 链目录演练修正,`chain-fill-dryrun.md`)。
- 回填顺序与验收:backfill 页第 2 节;占位残留 0、家路径 0。

## 3 预跑证据节

- 骨架在 `v010-prerun-evidence-section.md`(sha 390f69df):245 窗/521 批占位、W0000 首次
  module-units-limits 两次 rc124 58041 ms、重起 module-units-forward 5297 ms、损失约 8 分钟、四批贴限、
  与 107 对比表;取值命令已在页内。
- 预跑完成后把实际窗数/批数/用时/首窗原行填入,不引用 107 数字代替。

## 4 已定稿、不要重做的事

- 不要重开"APE 仅 32 字节版本槽"表述;不要重写零改动三断言。
- 不要把 107 的 strict/CI 数字搬进 0.1.0;不要提前用 107 的 SID。
- 不要改条件 4/5/6 的如实未满足措辞;不要为过线放宽 45/120/55 秒。
- 决定块(prd-decision-block-fable-flow.md,sha fccceb65)已按 fable 定稿重写,不再按旧稿改。
- 107 说明(notes-0.0.107-v2.md,sha dfec878b)已公开发布,研究页不再动。

## 5 当前队列与待命

- 0.1.0 预跑约 10:42 完成;之后 30 分钟时间轴见 agy-flow 的 v010-post-prerun-timeline.md(待落);
  B 提交落车、strict merge、pack/warm/dry/X 与公开由 agy-flow/cdx/ompgpt 执行。
- 我待命:只接说明回填(按第 2 节)、预跑证据节填数、或 cc 指定的只写页;不跑重命令,load1>3 不跑。
- 依赖页:backfill(468d1be1)、chain-fill-script(82868e77)、dryrun(887f6705)、risk-stop(f5edf32a)、
  final-docs-index(98c6201d)。
