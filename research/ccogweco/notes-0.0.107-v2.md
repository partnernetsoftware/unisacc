# unisacc 0.0.107 发布说明(v2 合成稿;2026-11-25 ccogweco)

状态:同一候选通过六格原生 CI,结论为 QUALIFIED(run 45512387135,2026-11-25 07:52:48 +0800,ms=543801);本版未执行全套 s9,未取得全套 s9 资格;同候选六格原生 CI 与全套 s9 是两件事,不得混称。
同候选六格原生 CI 与全套 s9 是两件事,不得混称。

## 真实身份

| 对象 | 身份 |
|---|---|
| 冻结提交(F4=起链提交) | 63354b4a2cb707ecc923a89f0df6030149fa697f |
| 发布 APE SHA256 | b773c7e2…(待 pack 后核全 64 位;回填命令见下) |
| 源快照 SID | 37ff46cd…(待 pack 输出核全 64 位) |
| CI run | 45512387135(QUALIFIED,07:52:48 +0800,ms=543801,六格原生 42/0) |
| pkg.bin / slice-win-arm64 / 两个 build.json / SHA256SUMS | <待 pack 后按候选表与 layout 核> |

回填命令:`git -C .claude/worktrees/v0.0.107 rev-parse HEAD`;`sha256sum <pack>/layout/unisacc.com`;
`grep -m1 'RELEASE_RUN	SOURCES_SID	VERIFIED' <pack-run.out>`;
`grep -m1 'RELEASE_RUN	FIXEDPOINT	VERIFIED' <pack-run.out>`;
`gh run list -R partnernetsoftware/unisacc --json databaseId,headSha -L 5`。

## CI 证据与失败类别

- CI run 的成功、任务失败与同候选重派发记录见本版证据 INDEX 与对应 `ci-<RUN>.json/.log`;失败分类以运行与作业日志所载阶段、实际首败和候选摘要为准,按真实 CI 裁决回填,不预写结果。
- 若本车出现瞬态资产下载失败,须记旧、新 run ID 与替代关系,区分资产缺失与原生格实际拒绝;没有该事件就保持实测记录,不搬用 106 的 run。内部对照:research/ccogweco/incident-playbook-107.md:12、loss-guards-107.md:23,64。
- 现稿首行的 QUALIFIED 为拟回填文字,须等实际 `GATE 3 CI_VERDICT PASS verdict=QUALIFIED fail=0 invalid=0` 后才落正式说明。

- 实际回执(2026-11-25 07:52:48 +0800,run 45512387135):`GATE 3 CI_SUCCESS PASS run success, six candidate targets success`;`GATE 3 CI_VERDICT PASS verdict=QUALIFIED fail=0 invalid=0`;INDEX `exec-3-ci ... PASS 45512387135 six targets success, CANDIDATE_QUALIFY no fail/invalid`;`STEP 3 ci DONE`,ms=543801。


- 第二轮:T1 05:08:30 -> T2 05:34:55,rc=0,26.4 分钟;REV=F4。链目录 bg-b1/63354b4a2c…。
- 第一轮:T1 02:49:02 -> T2 03:15:16,26.2 分钟,预跑在 W0201 产品红停;旧链与旧预跑目录原样保留作证据。
- W0201 定因:107 候选 APE 对 `typedef int row[3]; void f(row r[2]);` 输出 `error: expected ']'` 却 rc=0 并写出对象;
  宿主 cc 返回 1。判产品缺陷,走路径 C:修产品、产品输入变更、重起链、全量重预跑。
- 第二链固定点:objects=same count=102、kernels=same count=2、slices=same count=6、product=same,
  六目标 g2/g3 逐对 sha 相同;ape-receipt 与 fixedpoint-6targets 链止后归档。

## 发布结果(2026-11-25)

- Step 5 publish 07:57:29 +0800(isDraft=false, Latest,ms=5548);Step 6 readback 07:57:44 PASS public-readback.txt(ms=14664);Step 7 Y2 07:57:53 sha=6f55d3c1b5…(ms=20192);RELEASE_EXEC RESULT EXECUTED steps=7 deferred=0。
- 条件 6 原文口径:T1 05:08:30 到 T4 07:57:29 = 168.98 分钟,超 120,如实记未满足;R6 重述口径待 T3_R6 补。

## 产品改动

(1) W0201 修复:数组形参 typedef 的形参声明符错误路径返回非零且不写对象;N2 变 rc 非零、无 .o,其它 9 例与
check-array-param-typedef 全绿(先红后绿原行随证据)。
(2) 入车十三份:#1 precommit list-cap v7、#2 chain-step 回收后不杀、#3 dvqualify selector reuse v3 + 0755、
#4 release-exec 长路径 sha 缓冲、#5 cx-booter-ape 生产者、#6 tools-default-regression 生产者、#7 seal-precheck v2、
#9 wp4 install-ape v4、#10 wp4 material v2、#11 u08 source-missing v3、#12 gen-all-drift fixture、
#14 readback prewarm gate v2、tools.cxlib bump 0.4.11->0.4.12。
(3) 路径 B 三笔(链后 tools/tests-only):
   - 309e2e1ab8:B-t,check-ratchets 19 道门夹具合同(ratchets-tool-full);
   - a63bc7bece:B-t,把 check-precommit-list-cap 加入 daily-compile-smoke 表(W0213);
   - 5d04a80a47:B,retcheck 基线补两个夹具键(W0231);
   - 第四笔:<COMMIT_40HEX>(待定;含 precommit-list-cap 贴限登记与相关披露,落定后回填)。
   三笔均在 F4 之后,冻结路径零差异,SOURCES_SID 与 APE 不变(以第四笔后的 path-b 证明页为准)。

## 预跑证据

- 第二轮预跑:241 窗,80.9 分钟;起点 T_prerun2 05:39:02(PID/PGID 3163300);lane=2;
  priority-file 把本车新增/改动批置顶,W0000 为轻载窗(W0000 曾两次 rc124 后 PASS)。
- W0213(第二批 resume 窗):`SUMMARY batches=8 PASS=8 FAIL=0`,check-daily-compile-smoke PASS rc=0 5073 ms assertions=8;
  随后推进 W0213 到 W0219 全绿,再进入 W0220 恢复(jobs 8 -> 1)。
- 四补跑:W0000、W9916、W9931、W9993b 已 PASS;W9993b slow-split PASS 51144 ms(assertions=5,仍贴 55 秒)。
- strict(终态):519 批、518 PASS、0 FAIL、1 UNKNOWN、0 INVALID;UNKNOWN 为 `precommit-list-cap` 的窗口超时(多次尝试 55486/55492/55530/55557 ms,transport-1),属设计性贴限,
  登记 expected-unmeasured(非 expected-fail),reason 写 55 秒子进程限(run-batch.cx:264 的 53 秒),0.1.1 拆批。
  第四笔落定后按最终 strict 汇总写实,本节数字重核。

## 已知缺口

- (1) U05(provenance):本车 workflow 无取证步骤,记未测(UNMEASURED);关闭条件为 CI osx 格一次带 sha 对照的实跑行。
- (2) 全套 s9 本版未跑;严格证据所列批次之外 UNMEASURED。
- (3) slow-split:W9993b PASS 51144 ms,余量不足;披露为"PASS 但贴近 55 秒限"。
- (4) precommit-list-cap:设计性贴限,低负载(load1=1.82)未改变结论;不放宽 55 秒、不缩覆盖,0.1.1 拆批。
- (5) 准入条件 4(声称类 <=3):未满足预期 3 到 4,按 R38 口径如实记。
- (6) 准入条件 5(前端三期):未满足,按 R37 豁免;B2 顺延 0.1.x。
- (7) 准入条件 6(起链到公开 <=120 分钟):第二轮 T1 05:08:30 到 T4 07:57:29 = 168.98 分钟,超 120,如实记未满足;第一轮因路径 C 不成立已作废;R6 重述口径待 T3_R6 补,两口径并列,不选单一粉饰。
  R6 重述口径并列记录,不选单一粉饰。

- (8) 三批 PASS 但余量不足:premerge-selfcheck-main 50251 ms(距 53 秒限余 2749 ms)、premerge-selfcheck-rd 49898 ms(余 3102 ms)、slow-split 51144 ms(余 1856 ms),三批均不足 4 秒;与 list-cap 设计性贴限同类,不记失败但如实披露;0.1.1 按每批预算上限 27.5 秒拆批。证据入口:research/ompgpt/margin-disclosure-107.md。
- 披露句(可直接引用):"本版有三批 PASS 但距子进程上限不足 4 秒:premerge-selfcheck-main 50251 毫秒、premerge-selfcheck-rd 49898 毫秒、slow-split 51144 毫秒;按 PASS 记录,同时在 0.1.1 按 27.5 秒预算拆批,不把余量不足写成失败,也不隐藏贴限。"
- 证据入口:research/ompgpt/margin-disclosure-107.md(ompgpt 页已落地;首段可直接引用,逐批 0.1.1 拆分与三次冷测 <=27.5 秒;算术余量 main 2749 ms、rd 3102 ms、slow-split 1856 ms)。

## 顺延与 0.1.x 路线

- 107 未入车:#8 v4 材料宏检查、#13 r39e 与宿主遮蔽三批、#15 release body。
- 顺延:#16 CI attempt、#17 tlib bump 硬阻断、prerun-watch 登记、预跑提速 S1-S4 与 W0000 轻载锁。
- 0.1.x 产品车:0.1.1 诊断(108 原范围 + 103 判据 + list-cap 拆批)、0.1.2 前端 B2(109 原范围)、
  0.1.3 进程生命周期(125 原范围 + U33)、0.1.4 转发桩余项(126)。

## 链后占位(逐项回填)

| 占位 | 回填来源 |
|---|---|
| <COMMIT_40HEX>(第四笔) | 车树 git log 与 path-b 证明页 |
| CI run(已回填) | 45512387135,07:52:48 +0800,QUALIFIED;失败/重派记录无 |
| APE2 / SID2 全值 | pack 输出与 `sha256sum <layout>/unisacc.com` |
| T3_R6 / T4 | T4=2026-11-25 07:57:29 +0800(exec-5-publish);T3_R6 待 INDEX/CHAIN RESULT 时间补 |
| Y1 / Y2 | Y1=72187d1e48(07:56:07);Y2=6f55d3c1b583332322078c072b8efe3c6c273937(07:57:53) |
| strict 最终计数 | 合并 strict 的 SUMMARY 与 rows 行 |

## 落盘规则

- 正式 release/notes-0.0.107.md 只保留成文章节;本页回填命令不进正式件。
- 任一占位未回填前不得过 Y1;不把占位写成通过;不夸大余量与资格。
