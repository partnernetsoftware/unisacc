# v0.1.3 已知缺口汇总(供发布说明引用)

口径:每条给「缺口 / 范围 / 来源文件:行 / 证据路径 / 发布说明建议」。标记「来源待补」的是本树 grep 未命中、需对应线补一行出处的项,不臆造。
本文件为私有草稿(cc 合并进 research/ccogweco/)。

| 缺口 | 线/范围 | 来源文件:行 | 证据路径 | 发布说明建议措辞 |
|---|---|---|---|---|
| C3 I9 未验:进程树收集上限/上限+1/-ENOSPC 的假清单用例仍归 S4 | C3 进程生命周期 | research/cdx/c3-invariants.md:23 | 同上(私有 S4 计划) | 「C3:I9 进程树收集上限场景未验证,登记到 S4」 |
| C3 macOS/Windows `sy_proc_finished` 路径 UNCOVERED | C3 平台面 | research/cdx/notes-0.1.3-skeleton.md:51 | 同上(需 `<C3_PLATFORM_EVIDENCE>` 补) | 「C3 的 macOS/Windows finished 路径 UNCOVERED,仅 Linux 实测」 |
| 裸 fork/waitpid:存量 12+ 文件 137 处未受棘轮保护(棘轮补丁 v3 尚未入库) | 进程原语卫生 | research/ccogweco/bare-proc-calls.tsv;私有补丁 s5-bare-proc-ratchet-v3.patch | /tmp/ccogweco-docs/research/ccogweco/s5-bare-proc-ratchet-v3.patch(私有树夹具 13/13,ratchets 20/0) | 「S5 裸 fork/waitpid 棘轮未入库前,表外裸调用无可依赖的门」 |
| 27 格 UNOBSERVED | v0.1.3 验收矩阵 | 来源待补(本树 grep UNOBSERVED 未命中该批;候选 agyneon/S1 侧文档) | 待补 strict/matrix 回执路径 | 「27 格 UNOBSERVED,未计 PASS」 |
| nn:Darwin/Windows UNCOVERED | nn 平台 | research/agy-h/slice-nn-progress.md:113(四格 osx/win 的精确分量标 UNMEASURED) | 同上 | 「nn 在 Darwin/Windows 未覆盖;四格分量 UNMEASURED」 |
| nn:同尺寸原地写 UNSUPPORTED-DOCUMENTED | nn 写路径 | 来源待补(本树未命中「同尺寸原地写/UNSUPPORTED-DOCUMENTED」原文;archive 中同尺寸统计为历史项) | 待补 | 「nn 同尺寸原地写属 UNSUPPORTED(文档化),非缺陷」 |
| tape:速度门 UNMEASURED | tape 性能 | research/agy-flow/tape-t14c-speed-baseline.md:80(15 倍门量化,~16.9-18.5 亿步上限) | 同文件 | 「tape 速度门 UNMEASURED,15 倍门未达」 |
| tape:T1.6 与索引不做 | tape 范围 | research/agy-flow/tape-t14c-speed-baseline.md:80(T1.6 预解码紧凑记录为候选);索引项来源待补 | research/agy-flow/tape-t14-containers.md:13(容量表) | 「T1.6 与索引不在本版范围」 |

备注:
- 现有已按实:BLOCKED 与 FAIL 不记 PASS、UNMEASURED/UNOBSERVED 均如实保留;上表不含 archive/ 历史项的重复登记。
- 补源方式:对应线在自身研究页加一行「缺口-来源-证据路径」,我来合并,不改各线正文。
