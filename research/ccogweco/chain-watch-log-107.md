# 0.0.107 整链值守记录(ccogweco;2026-11-25)

角色:证据收集模板预建(已完成:research/ccogweco/evidence-107-template.md,sha a71a991d)与值守记录。
只读记录;不动 ~/.cache/unisacc/chains/bg-b1 与 /tmp/cc-chain-107;不跑重命令;链内红只报 cc。

## 值守事实(cc 通告原样)

- 整链起:T1=02:49:02;REV=28b6a518fe;pid/pgid=1984486;deadline 5400 秒。
- **整链止:T2=03:15:16,rc=0,用时 26.2 分钟**。
- 链输出目录:~/.cache/unisacc/chains/bg-b1/28b6a518fe9fac6b117f31adeb25e1c8ab51701f
  (ape-final 有 ape-receipt.txt、p3-package.body、unisacc-next.com)。
- 链内固定点:objects=same count=102、kernels=same count=2、slices=same count=6、product=same、
  package=present sha=153b9f8f…。
- 候选 APE sha256:9a1e2630100deace2b6a8d82a1b6c47fdf57b872e82f3cd4e1ee55756c2be544。
- shared main = d71c225b7d(含账本与 24 小时裁定决定块);冻结行:main 07:30,车树 F 08:00。
- 分工:agy-flow 链进度与 P1-P6,起预跑并回 PRERUN 起始时刻与 PID/PGID;ompgpt 封包后候选 kit 独立预备;
  cdx 链产物对账与 CI 前置核对只读版;agy-h 记 T2 与 prerun-watch 每 10 分钟、第 10/30 分钟证伪点;
  agyneon 补笔待命盯 F1 fwd-cap 与 fwdstub-replay strict 行;ccogweco 本页。

## 计时点

| 点 | 时刻 | 含义 | 硬线 |
|---|---|---|---|
| T1 | 02:49:02 | 整链起 | 起链到公开 120 分钟线 = 04:49 |
| T2 | 03:15:16 | 整链止 rc=0 | 链止到 X 45 分钟线 = 04:00 |
| T3_X | 待 X 提交 | 候选表提交 | 相对 T2 <=45 分钟 |
| T3_R6 | 待 CHAIN RESULT READY_TO_EXECUTE | R6 重述口径终点 | (T3_R6-T1)/60<=120 |
| T4 | 待 exec-5-publish | 公开 | 原文口径 (T4-T1)/60<=120 |

- 备注:04:49 的 120 分钟线在本窗口字面不可能保证(预跑 106 实测 122.6 分钟),按准入第 6 条如实记未满足;
  链止到 X 的 45 分钟线仍须每轮报剩余分钟。

## 记录表

| 时点 | 预期/要求 | 实测/观察 | 备注 |
|---|---|---|---|
| T+0 02:49 | 链启动 | 已起,pid/pgid 1984486 | — |
| T2 03:15 | 链止 | rc=0,26.2 分钟 | fixedpoint 四同,package sha 153b9f8f… |
| T2+1 到 +3 | agy-flow P1-P3 | 待报 | 生成 fixedpoint-6targets 与 ape-receipt、src-dir、说明与 rollback |
| kit 后 | 预跑起 | 待 PRERUN 起始与 PID/PGID | unset DVQUALIFY、2 lane、--resume --keep-going、clean PATH、短 TMPDIR |
| 预跑 +10 分 | 完成窗数 >=110 | 待 agy-h | 小于 110 即保守档 |
| 预跑 +30 分 | 完成窗数 >=110 | 待 agy-h | 同上 |

## 预跑段(T_prerun 起)

- 起始:T_prerun=03:21:56;PID/PGID=2213169;目录 ~/.cache/unisacc/prerun/v0.0.107-28b6a518fe。
- 判据:第 10 分钟(03:32)核 lanes=2 且完成窗数 >=14;第 30 分钟(03:52)核 >=110;每 10 分钟由 agy-h 用 prerun-watch v2 报(done、rate_inc/cum、eta_min、rc124 窗名、lanes)。
- 分工:agyneon 盯 W0199(fwd-cap)与 W0226(fwdstub-replay 等 8 批)strict 行;ompgpt 首窗 rc124 两分钟内原位 --resume;cdx 红分诊先报 cc;ccogweco 记速率与事件。
- 红停处置:已审豁免红两分钟内 --resume;产品红停报 cc,不自行改。
- 108 候选新增(agy-h 记入 car2-108-candidates):prerun-inputs 打印的 PRERUN_CMD 带 --normal-jobs 8,而 prerun-drive.cx:3067 不接受该参数;属工具参数不一致,归 0.1.x 工具线。

## 转保守档(03:53,cc 周知)

- 依据:第 30 分钟完成 105 窗 <110(fable 证伪点);107 公开预计 06:10-06:30,0.1.0 起链 06:30-07:00,公开保守约 11:00 前。
- 纪律:不加速预跑、不放宽时限与覆盖、lane 保持 2;不因时间压力改口径。
- 硬线:T2+45(04:00:16)链止到 X 必超,如实记未满足。

## 预跑产品红停(W0201,待分诊)

- cc 周知:W0201 `c99-array-param-typedef` 产品红停,assertions=0;预跑 STOPPED。
- done=202/247;停机时刻与原行由 agy-h 记入账本(本页不探活、不补时间)。
- 处置:ompgpt 与 ccogweco 待命;等 cdx 分诊结论与 cc 决定(续跑或按路径 B/B-t/C)。
- 本页只记录,不改预跑、不动链与预跑目录。

## W0201 产品红停与路径 C 重起链(cc 裁定)

- 事件:W0201 `c99-array-param-typedef` 产品红停,assertions=0;done 202/247;预跑 STOPPED,原目录保留。
- 直证:107 APE 对 `typedef int row[3]; void f(row r[2]);` 打 `expected ']'` 却 rc=0 并写 N2.o 2144B;宿主 cc rc=1。
- 裁定:产品缺陷,路径 C——修产品、产品输入变更、重起链、全量重预跑;旧 202 窗不可复用,只作定因。
- 分工:ompgpt 修产品(30 分钟内,含 SID/bump 影响);cdx 对抗复核与邻近批摸底;agyneon 重起链序列;agy-flow 时钟重估;本页记录。
- 时间重估:修复 30 + 整链 27 + 预跑 70 + 补跑发布 60 分钟;0.1.0 13:40 最迟起链线重核。

## 第二轮整链(T1 05:08:30,REV 63354b4a2c)

- T1=05:08:30(1795554510);F4=63354b4a2cb707ecc923a89f0df6030149fa697f;guard 2968888,pid/pgid 2968889;deadline 5400。
- 链目录 bg-b1/63354b4a2cb707ecc923a89f0df6030149fa697f;预计链止 05:34-05:37。
- 第一轮链与预跑目录保留;第二轮计时点 T1/T2/T3/T4 全按本轮重记。
- 分工:agy-flow 链值守与 P1-P3(先封存旧证据,grep 零残留);agy-h chain-to-public-r2 每 10 分钟;cdx 红分诊与 CI 前置;ompgpt kit 双轮 17/0;agyneon 盯新增批置顶;ccogweco 记录。
- 纪律:不动 chains/bg-b1 与 /tmp/cc-chain-107,不抢重命令;链内红只报 cc。

- **T2=05:34:55(1795556095),rc=0,26.4 分钟**;链目录 bg-b1/63354b4a2c…。
- 硬线:T2+45=06:19:55;T1+120=07:08:30;产品红停不 resume。
- 接着:P1-P3、cdx 对账与 APE2/SID2 全值、ompgpt kit 双轮 17/0、agy-flow 起预跑(UNISACC_BOOTER_CHAIN_OUT、priority-file、21 个置顶 id 在前 32 窗、run 不带 --normal-jobs、timeout 5400)。

## 第二轮预跑(T_prerun2 05:39:02)

- T_prerun2=05:39:02;PID/PGID=3163300;目录 ~/.cache/unisacc/prerun/v0.0.107-63354b4a2c;241 窗。
- 采样:agy-h 按 round2-sampling.md 用 prerun-watch v2;05:49 核 lanes=2 与窗数,06:09 核保守判据;账本 prerun-rate-107-r2.tsv。
- 新增批验收(W0021 约 05:45):fwd-cap(FWD_CAP failed=0 skipped=0 assertions>=28)、fwdstub-replay、c99-array-param-typedef(10 例全 PASS)、c99 另两批、cxmod-tri-exports;不绿立刻报 cc,不 resume;产品红停按路径判定。
- ompgpt 值守首窗 rc124 自动重试;agy-flow 备 05:55 前后外层与 resume 预案。
- 红停:已审豁免两分钟内 resume;产品红停报 cc。

## 补跑终态与贴限裁定(静默解除 07:13)

- PASS:W0000、W9916、W9931、W9993b。
- 仍贴限:precommit-list-cap(低负载 load1=1.82,热态 55530 ms,UNKNOWN transport-1)、tsv-lint-root(load1=2.31,57633 ms,UNKNOWN 3,no-BATCH_RUN);低负载未改变结论,属设计性贴限(内层多个真实 precommit 或冷编译叠加),不是负载假象。
- cc 裁定:不放宽 55 秒、不缩覆盖;两批保留 strict 原 UNKNOWN 与 RAW 如实披露;登记 expected-unmeasured(非 expected-fail),reason 写 55 秒子进程限(run-batch.cx:264 的 53 秒)与证据入口;0.1.1 拆批。
- M1 分工:agy-flow 合并 strict(按文件名字典序折叠 W9915/W9915b、W9917/W9917b、W9993/W9993b,后者在后)并核 FAIL=0/UNKNOWN=2/INVALID=0、archive、scrub;ompgpt 更新 list-cap-margin 并写 tsv-lint-root 判定页;cdx 复核两行 G1 匹配与 W9993b(51144 ms,assertions=5)余量披露;agyneon 备 --post-chain-merge 与三 B 提交 REASON。

## 草稿与 CI 派发(agy-flow 通告)

- Step 2 draft 建立并通过回读:07:43:44 +0800。
- Step 3 CI 触发:07:43:53 +0800,run_id=45512387135,commit=8a0b2939730222c2c2581399bf962c59201e0be9;裁决待回执。
- notes v2 sha 9880fe21 经 agy-flow 核实(零 /home/ 路径),Y1 前用 gh release edit 更新草稿正文;本页更新 CI run 行后 sha 见下。

## Step 4 Y1(agy-flow 通告)

- 草稿正文与车内 release/notes-0.0.107.md 已用 notes-v2 sha d30a2fa1 更新。
- Y1 于 07:56:07 +0800 提交并推送 origin/v0.0.107:Y1_SHA=72187d1e489008617d80ef8e74ebf1e12180d647。
- 待 cc 推 origin/main 后进 Step 5 publish -> 6 readback -> 7 Y2。

## 公开发布终态(agy-flow 通告)

- Step 5 publish 07:57:29 +0800(isDraft=false, Latest,ms=5548)。
- Step 6 readback 07:57:44 PASS(ms=14664)。
- Step 7 Y2 07:57:53 sha=6f55d3c1b583332322078c072b8efe3c6c273937(ms=20192)。
- RELEASE_EXEC RESULT EXECUTED steps=7 deferred=0。
- 条件 6:T1 05:08:30 到 T4 07:57:29 = 168.98 分钟,超 120,如实记未满足。

## release-post 与 R33(agy-flow 通告)

- 68b15c4028(08:14:05):build/freeze.tsv + tools/old-product-assets.tsv。
- 521797c3a8(08:14:10):build/version-seq.tsv + release/evidence/0.0.107/INDEX.tsv。
- RELEASE_POST status=DONE;R33 OLD_PRODUCT PREPARED version=0.0.107。
- 0.0.107 收尾完成;后续按 0.1.0 封版车推进。

## 0.1.0 车预备(agy-flow 通告)

- 工作树 .claude/worktrees/v0.1.0,HEAD=a24762a6aeedcf613e92bbbacedf85da1497ff49。
- env -i 干净环境下 chain-ready --version 0.1.0:除 23-freeze-armed 外 22 项全 PASS(READY no failed=23-freeze-armed,131951ms);待 cc 落 F 起链。
- 冻结行与 F 由 cc 写;写后复跑 23 再起链。

## 0.1.0 F 落定(agy-flow 通告)

- F=a11d28fc0a61939ee71345d732a7dcf205b261f1;env -i 下 chain-ready --version 0.1.0 全 23 门 PASS(23-freeze-armed PASS),末行 READY yes failed=none 133666。
- 按暂停令待 cc 放行后起链。

## 0.1.0 整链起(T1 08:59:43)

- T1=2026-11-25 08:59:43 +0800(epoch 1795568383);REV=F5=a11d28fc0a61939ee71345d732a7dcf205b261f1。
- guard=570322,pid=570323,pgid=570323;输出 ~/.cache/unisacc/chains/bg-b1/a11d28fc0a61939ee71345d732a7dcf205b261f1。
- 预计链止约 09:26(按 107 的 26.4 分钟);链止后按 v010-chain-fill-script.md 取 T1/T2、四同、APE5 与回执。

## 0.1.0 链后核对与预跑起(agy-flow 通告)

- FIXEDPOINT_EVIDENCE PASS version=0.1.0 targets=6 compare_attempt=245;证据已写 release/evidence/0.1.0/{fixedpoint-6targets.tsv,ape-receipt.txt,INDEX.tsv}。
- 严格预跑 T_prerun3=2026-11-25 09:29:39 +0800(epoch 1795570179);pid/pgid=817099;PDIR=~/.cache/unisacc/prerun/v0.1.0-a11d28fc0a;total=521 windows=245。
- 后续:agy-h 用 prerun-watch v3 只读监视;agyneon 盯负载与四批贴限单跑;链后占位 SID5/pkg.bin 等 pack 后填。

## prerun 期间负载纪律(cc 通告)

- prerun 进入 long 相位(done=108/245),load1=6.8;W0100 sum_batch_ms=51871 已贴限。
- 私有实跑一律单批串行、不并发、不跑整套 check-*;每次实跑前先看 uptime,load1>4 就等。
- 违反导致贴限窗被推过 53 秒,该窗补跑,耽误公开。

## 证据模板状态

- evidence-107-template.md 已建:33 个证据文件、INDEX 行类型、空壳头、格式坑六条;链止后按表收 pack/ape-receipt/
  fixedpoint-6targets,预跑后收 strict/deferred/prerun 日志,公开后收 readback/post-state/Y2。
- 本页只记录通告事实,不写结论;不引用未实跑的 PASS。
