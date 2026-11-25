# unisacc 0.1.0 发布说明骨架 v2(零改动封存;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run <CI_RUN_ID>)。本版未执行全套 s9,未取得全套 s9 资格;
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F5=起链提交) | <COMMIT_40HEX> |
| 发布 APE SHA256 | <APE5_SHA256>(由同一正式输入按 0.1.0 版本槽重封;裸字节差异不等于版本槽长度) |
| 源快照 SID | <SID5_64HEX>(与 107 相同) |
| CI run | <CI_RUN_ID> |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | <按 pack 后核> |

回填:`git -C .claude/worktrees/v0.1.0 rev-parse HEAD`;`sha256sum <pack>/layout/unisacc.com`;pack 输出的
`RELEASE_RUN SOURCES_SID VERIFIED` 与 `FIXEDPOINT VERIFIED`;`gh run list` 按 headSha 过滤。

## 零改动封存口径(三条硬断言)

(1) 产品闭包零差异:`src cx include exec kernel seed weights`、`iterate/kernel/typekw.tsv` 与 13 个 tools 支撑文件
相对 107 冻结提交的 `git diff --stat` 无输出。
(2) SOURCES_SID 相等:107 与 0.1.0 两快照的 ident helper 结果逐字相同;链产物 `NEED source_sid` 与 pack 的
`SOURCES_SID VERIFIED` 同值。
(3) 未注版切片与 pkg.bin 相等:六目标 `fixedpoint/g2` 与 `p3.pkg` 逐字节相同;`pkg.bin` 与 107 相同;
候选 APE 由同一正式输入加不同版本槽经重封得到(版本槽 32 字节经压缩与 PE 资源与 Mach-O 重签传播,裸文件字节差异远大于 32 字节),重封两次私有复现一致;`--version` 输出 0.1.0。

三条在链止后立即跑(1)(2),pack 前后各跑(3)对应部分;任一失败取消封版车,按 R37 重排。

## 与 0.0.107 的差异清单(只允许两类)

- tools/ 下发布形态修复:release 版本形态相关修复与 tools.cxlib 包版本(0.4.12 之后按实际 bump);
- 版本槽:`release/candidate.tsv` 版本槽、APE 的版本槽重封(非裸 32 字节差异)、win-arm64 切片注入版本。
- 除上述外不应有 src/cx/include/exec/kernel/seed/weights 字节差异;出现即零改动断言失败。

## 准入与 107 复盘

- 条件 1(六目标不动点连续 3 版):106/107 已归档,0.1.0 自身链止后归档第三版。
- 条件 2(CI 六格 QUALIFIED + strict FAIL/UNKNOWN=0):107 为 519 批 518 PASS / 1 UNKNOWN(list-cap 设计贴限);
  0.1.0 按自身 strict 如实记。
- 条件 3(零已知 P0):P0 账本 5 行,F1 与 chain-step 已在 107 关闭;U33 为 P1 随 0.0.125(0.1.x)。
- 条件 4(声称类 <=3):未满足,预期 3 到 4,按 R38 口径如实记。
- 条件 5(前端三期):未满足,按 R37 豁免;B2 顺延 0.1.2(原 0.0.109)。
- 条件 6(起链到公开 <=120 分钟):107 实测 T1 05:08:30 到 T4 07:57:29 = 168.98 分钟,超限;0.1.0 自身
  按 T1/T3_R6/T4 两口径记录,不选单一粉饰。

## 已知缺口

- (1) list-cap 设计性贴限:107 strict 1 UNKNOWN(precommit-list-cap 多次 55486/55492/55530/55557 ms);
  不放宽 55 秒、不缩覆盖,0.1.1 拆批。
- (2) 三批 PASS 但余量不足:premerge-selfcheck-main 50251 ms(余 2749)、premerge-selfcheck-rd 49898 ms(余 3102)、
  slow-split 51144 ms(余 1856),均距 53 秒限不足 4 秒;披露沿用并引用 research/ompgpt/margin-disclosure-107.md。
- (3) U05(provenance)未测;全套 s9 未跑;严格证据范围外 UNMEASURED。
- (4) 条件 4/5/6 未满足如实写;B2 顺延 0.1.2。

## 顺延与 0.1.x

0.1.1 诊断(108 原范围 + 103 判据 + list-cap 拆批);0.1.2 前端 B2(109 原范围);0.1.3 进程生命周期(125 原范围 + U33);
0.1.4 转发桩余项(126);工具线 T1-T11(roadmap-0.1.x.md)。

## 占位回填表

| 占位 | 回填来源 |
|---|---|
| <COMMIT_40HEX> | 0.1.0 车树 git log(F5) |
| <APE5_SHA256> / <SID5_64HEX> | pack 输出与 sha256sum |
| <CI_RUN_ID> | gh run list 按 headSha;失败/重派各留一行 |
| T1/T2/T3_R6/T4 | 链 owner/exit、CHAIN RESULT、INDEX exec-5-publish |
| Y1/Y2 | INDEX exec-4-Y1 / exec-7-Y2 |
| strict 计数 | 合并 strict 的 SUMMARY 与 rows |
| 三条零改动断言原行 | git diff/SID/pack 回执 |

## 落盘规则

正式 release/notes-0.1.0.md 只保留成文章节;占位未回填不得过 Y1;不把未测写成通过。
