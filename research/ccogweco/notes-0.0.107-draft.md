# unisacc 0.0.107 发布说明(v1 草稿;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run <CI_RUN_ID>)。本版未执行全套 s9,未取得全套 s9 资格;
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F=起链提交) | 28b6a518fe(已推 origin/v0.0.107) |
| 发布 APE SHA256 | 9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544 |
| 源快照 SID | 9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6(与链 ctx/context.tsv 的 source 一致,cdx 核) |
| CI run | <CI_RUN_ID>(待回填) |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | <待 pack 后按候选表与 layout 核> |

回填命令:`git -C .claude/worktrees/v0.0.107 rev-parse HEAD`;`sha256sum <pack>/layout/unisacc.com`;
`grep -m1 'RELEASE_RUN	SOURCES_SID	VERIFIED' <pack-run.out>`;
`gh run list -R partnernetsoftware/unisacc --json databaseId,headSha -L 5`。

## 整链与固定点

- 链:T1 02:49:02 -> T2 03:15:16,rc=0,用时 26.2 分钟;REV=28b6a518fe。
- fixedpoint 四同:objects=same count=102、kernels=same count=2、slices=same count=6、product=same、
  package=present sha=153b9f8f…。
- 候选 APE sha256=9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544;SOURCES_SID=9b83432e63a84a7fb00a9bb5fcb33533230342c61499903ab13e29b37d4677b6。
- 证据入口:链目录 ape-final(ape-receipt.txt、p3-package.body、unisacc-next.com);
  release/evidence/0.0.107/fixedpoint-6targets.tsv 与 ape-receipt.txt(链止后归档)。

## 产品改动(入车 13 份)

(1) #1 precommit list-cap v7:提交门大输出按上限 fail-closed(`5db7d34071`)。
(2) #2 chain-step 回收后不再杀:失败分支不按已回收 pid 发信号(`37f0aef541`,重基版)。
(3) #3 dvqualify selector reuse v3 + 0755 修订(`f0f07a315f`)。
(4) #4 release-exec 长路径 sha 缓冲修复,解除 106 W0150 的 200 字节截断(`5b4f77f33e`)。
(5) #5 cx-booter-ape 的 booter-raw-elf 生产者(`1d3e80a4c2`)。
(6) #6 tools-default-regression 的 old-checker 生产者(`b32a48e979`)。
(7) #7 seal-precheck v2:七条封版机械判据(`586db8dbcf`)。
(8) #9 wp4 install-ape v4:缺 APE fail-closed 与拆片(`04eb6d0d74`)。
(9) #10 wp4 material v2:batch-timing/tsv-lint/ratchets-tool 材料化(`5df7c35286`)。
(10) #11 u08 source-missing v3:缺源具名拒绝负控(`62cf5bc4e6`)。
(11) #12 gen-all-drift fixture:漂移夹具负控(`1122ace564`)。
(12) #14 readback prewarm gate v2:公开前读回预热硬门(`48f9e054ad`)。
(13) tools.cxlib bump 0.4.11->0.4.12,覆盖 #3 函数体改动(`e2f0539e2a`)。

未入车:#8 v4 材料宏检查(3394b077,余 36.8 秒,需先消 affected.cx:1203 与 devsvc-core.cx:538 字面)、
#13 r39e 与宿主遮蔽三批、#15 release body。顺延:#16 CI attempt、#17 tlib bump 硬阻断、prerun-watch 登记、
预跑提速 S1-S4。

## 严格证据(占位)

    证据 release/evidence/v0.0.107-strict-<DATE>.tsv:rows=<ROWS>,expected_fail=<N>,superseded=<SUPERSEDED>;
    预跑原 merge 为 <PRE_BATCHES> 批 PASS=<P> FAIL=<F> UNKNOWN=<U>;
    合并后 SUMMARY <FINAL_BATCHES>/<PASS>/<FAIL>/<UNKNOWN>/<INVALID>。

回填:`grep -E 'SUMMARY|rows=' release/evidence/v0.0.107-strict-*.tsv`;原 merge 计数取 prerun-supervise.log 的 SUMMARY 行。

## 预跑中暴露并已处置的问题(待补跑)

- kit 17/0 补热轮:候选 kit 就绪后补一轮预热,避免首窗冷编;原行待补跑记录。
- old-product 缓存目录权限 0775(手工预热)导致 60 批中 16 窗红;已 chmod 0700 修复,相关窗待补跑并保留原 FAIL 行。
- W0014 缺 `UNISACC_BOOTER_CHAIN_OUT`:env 未设导致该窗红;待补跑并记录环境差异。
- W0088 slow-split 贴 53 秒:106 同形(贴限),非本版回归;待补跑并对照 106。

以上四条在补跑完成前一律 UNMEASURED,不写 PASS。

## 预跑实测速率与分相(数据取 agy-h prerun-rate-107.tsv 原行)

- T_prerun 03:21:56(lanes=2,pid 2213169),总窗 247(107 实际窗数,非 106 的 238)。
- T+10.4 分:done=29/247,rate=2.85 窗/分,eta_prerun_rem=76.4 分,est_prerun_end=04:48;note 原行:rc124=W0000;keep_going=11(W0014,W0019..W0028)。
- T+20.2 分:done=87/247,rate=5.92 窗/分,eta_prerun_rem=27.0 分,est_prerun_end=04:09;note 原行:rc124=W0000,W0080;keep_going=17(stopped_at_W0034)。
- T+28 分:done=99/247(agy-h 报),速率与 ETA 由该时刻账本行决定,本页不重算。
- 保守档依据:总窗数 247 高于 106 的 238,且前段出现 rc124(W0000)与多次 keep_going;T+10 的 est_start_to_public 若按 04:48 已超 120 线,按保守档排。证伪点:第 30 分钟完成窗数小于 110 即保保守档;若后续速率持续 >5 窗/分且 rc124 不再增加,可按基准档复评(以账本新行为准)。
- 以上除标注外均为实测账本行;未出现的时刻标 UNMEASURED,不插值。

## 转保守档(03:53)

- 触发:按 fable 证伪点,第 30 分钟完成 105 窗,小于 110;03:53 转保守档。
- 预计:107 公开 06:10-06:30;0.1.0 起链 06:30-07:00,公开保守约 11:00 前;最迟起链 13:40 不触。
- 纪律:不加速预跑、不放宽时限与覆盖、lane 保持 2;不因时间压力改任何口径。
- 硬线:链止到 X 的 45 分钟线(T2+45=04:00:16)必超,按准入第 6 条与 R37 如实记未满足。

## W0201 产品红停与路径 C 重起链(2026-11-25)

- 事件:预跑 W0201 `c99-array-param-typedef` 产品红停(done 202/247),预跑 STOPPED;原目录保留作证据,不清。
- 直证(cdx 私有):107 候选 APE 对 `typedef int row[3]; void f(row r[2]);` 输出 `error: expected ']'` 却 **rc=0** 并写出 N2.o(2144B);宿主 cc 返回 1。
- 裁定:候选产品缺陷,不改检查迁就;走路径 C——修产品、产品输入变更后重起链、新候选全量重预跑(证据按候选字节绑定,旧 202 窗不可复用)。
- 分工:ompgpt 定位诊断发出点与退出码路径并出最小修法(基线 F=28b6a518fe,含 SID 路径与 bump 影响);cdx 对抗复核并摸底邻近 C99 批(W0198/W0200 等);agyneon 写重起链精确序列;agy-flow 重估时钟;ccogweco/agy-h 记录。
- 时间重估(agy-flow 页为准):修复约 30 分钟 + 整链约 27 分钟 + 预跑约 70 分钟 + 补跑与发布约 60 分钟;0.1.0 最迟起链线 13:40 需重核。
- 本版说明的 strict/公开时刻全部顺延为"重起链后按新候选填";旧 202 窗只作定因证据,不记 PASS。

## 第二轮整链(路径 C 重起链;2026-11-25 05:08:30)

- T1=05:08:30(epoch 1795554510);F4=63354b4a2cb707ecc923a89f0df6030149fa697f;guard=2968888,pid/pgid=2968889;deadline 5400 秒。
- 链输出:~/.cache/unisacc/chains/bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f;预计链止 05:34-05:37。
- 第一轮旧链与旧预跑目录原样保留作证据,不清;本轮 strict/公开时刻按 F4 重算。
- 第二轮口径页:round2-diffs.md、round2-sampling.md、f2-seal-checks.md(由 cc/agy-flow 维护);预跑命令不带 --normal-jobs,外层 timeout 5400,unset DVQUALIFY_NATIVE_RUN_BATCH,old-product 条目 700,新增批置顶。
- 本说明的身份、链段与固定点占位在第二轮链止后按 F4 的值重填。

- **T2=05:34:55(epoch 1795556095),rc=0,用时 26.4 分钟**;链目录同前。
- 本轮硬线:T2+45=06:19:55;T1+120=07:08:30(按准入第 6 条如实记)。
- 红停按 S1 与事故速查;产品红停立即报 cc,不 resume(第一轮 W0201 教训)。

## 第二轮预跑起点(2026-11-25 05:39:02)

- T_prerun2=05:39:02,PID/PGID 3163300,241 窗;新增批置顶 priority-file 已用。
- 新增批验收窗 W0021(约 05:45):fwd-cap、fwdstub-replay、c99-array-param-typedef(10 例全 PASS)与另两批、cxmod-tri-exports;结果入当轮 strict。
- 保守判据采样点:05:49(10 分钟)、06:09(30 分钟);账本 prerun-rate-107-r2.tsv。

## 贴限两批的披露口径(cc 裁定)

- `precommit-list-cap`:热态 55530 ms、UNKNOWN transport-1,低负载 load1=1.82 仍贴限;`tsv-lint-root`:57633 ms、UNKNOWN 3(no-BATCH_RUN),load1=2.31。两批均为设计性贴限,非负载假象。
- 处置:不放宽 55 秒、不缩覆盖;strict 保留原 UNKNOWN 与 RAW;登记 expected-unmeasured(非 expected-fail),reason 写 55 秒子进程限(run-batch.cx:264 的 53 秒),0.1.1 拆批。
- 说明的 SUMMARY 预期:FAIL=0、UNKNOWN=2(上述两批)、INVALID=0;不以 PASS 掩盖 UNKNOWN。

## 已知缺口

- (1) U05(provenance):本车 workflow 无取证步骤,记未测(UNMEASURED);关闭条件为 CI osx 格一次带 sha 对照的实跑行。
- (2) 全套 s9 本版未跑;严格证据所列批次之外 UNMEASURED。
- (3) 准入条件 4(声称类 <=3):未满足预期 3 到 4,按 R38 口径如实记。
- (4) 准入条件 5(前端三期):未满足,按 R37 豁免;B2 顺延 0.1.x。
- (5) 准入条件 6(起链到公开 <=120 分钟):本车 T1 到公开尚未发生;历史仅 98 达标;按 R6 两口径并列记录,
  不选单一粉饰。
- (6) #8/#13/#15 未入车;顺延项见 0.1.x 路线。

## 链后才有的占位(逐项回填)

| 占位 | 回填来源 |
|---|---|
| <CI_RUN_ID> | `gh run list` 按 headSha 过滤;失败/重派各留一行 |
| X 提交 | release-exec INDEX `exec-1-X-commit` |
| Y1 / Y2 | INDEX `exec-4-Y1` / `exec-7-Y2` |
| 公开时刻 T4 | INDEX `exec-5-publish` |
| T3_R6 | `CHAIN	RESULT	READY_TO_EXECUTE` 输出时刻 |
| strict 计数 | strict 表 SUMMARY 与 rows 行 |
| ape-receipt / fixedpoint-6targets | 链止后归档文件的 sha 与行数 |

## 落盘规则

- 正式 release/notes-0.0.107.md 只保留成文章节;本页回填命令不进正式件。
- 任一占位未回填前不得过 Y1;不把占位写成通过。链后若发生 post-chain-merge,按 106 第 (192) 条格式补写。
