---
name: unisacc-orch-review
description: 对执行者交付做对抗复核并裁决能否落库时调用;覆盖交付要求(先红后绿、变异自证、mutation-gate 口径)、检查真实性、高负载判读、两轮封顶与安全子集、复核者对抗清单与落库前补跑项。
---

# 对抗复核与落库裁决

复核者独立于作者,不信自报,自己核。关键路径补丁(提交门、运行器、dvqualify、release-*、register、install-ape)必须经复核者,可用代理定义 `unisacc-gatekeeper`。

## (1) 人工评审前先检验证据新鲜度

- 对以执行结果为验收证据的交付，先运行 `sh ./unisacc.com tools/mutation-gate.cx -- execution --produce '生产命令' --judge '判分命令' --assertion NAME`。两个命令都收到本轮 ID (`$1`) 和证据路径 (`$2`)。生产命令走实际入口并写证据，判分命令走真实验收判据、只读证据，并核本轮 ID。基线应以 rc 0 输出 `PASS<TAB>NAME`，拒绝应以 rc 1 输出 `FAIL<TAB>NAME`，标记写到 stdout 且单独成行。
- `delete-run-step` 省略本轮生产并移除证据。`feed-previous-output` 省略本轮生产，把此前通过的证据字节放到同一路径。两项 `MUTATION_EXECUTION` 都须为 `KILLED` 再进入人工评审。`SURVIVED` 退回修判据，`UNMEASURED` 补跑。不能用退出失败、超时或错断言折算为击杀。
- 命令接线必须保持真实生产与判分职责。评审者核对判分不会在内部重新生产证据，且 ID 绑定到实际完整输入和执行。纯静态交付注明此步不适用。

## (2) 交付要求

- 先红后绿:贴改前失败原行与改后通过原行。原因:只有绿行无法证明检查能检出缺陷。
- 至少 3 个变异,各自转红并贴检出原行。复核者再做至少 1 个与自报不同的变异。
- 关键路径补丁跑 `sh ./unisacc.com tools/mutation-gate.cx`:
  - 口径 `min-score 0.6`;`uncovered>0` 默认失败。
  - 确有不可覆盖项时用 `--allow-uncovered` 并配 `--adjudicated` 写明裁定理由。原因:豁免要留痕,不能静默。
  - 贴 `MUTATION_GATE` 与 `MUTATION_COVERAGE` 行。
- 断言只迁移不删;无新开关时行为逐字节对拍;冷环境耗时测三次。
- 新增行家目录绝对路径命中 0(`tools/no-home-path.cx --staged`)。
- 回报由 `tools/handoff-check.cx` 生成,编排者只收其输出,执行者自述的"完成""无回归""既有失败"不作证据:
  - 末行 `HANDOFF passed= failed= unmeasured= deferred= review=`;rc 0 才算干净,rc 1 有失败,rc 3 有未测、延后或待复核项,逐项处理不得略过。
  - 声称"既有失败"时用 `--vs REV --cmd '命令'`:同命令在工作树与 REV 的 git archive 各跑一次并贴两侧原行;当前失败而 REV 通过判 FAIL,两侧同一行失败判 REVIEW_REQUIRED 由复核者裁定。
  - 没做完的事用 `--defer 名称=原因` 声明,或让 tests/batches.tsv 新增行带非 IMPLEMENTED 状态,都计入 `deferred=`。原因:自述会漏报延后项,机械输出不会。

## (3) 检查必须真实

- 检查调真实入口(仓内工具本身),不测文件内重写的副本。原因:副本与真实入口会漂移,检查绿而产品坏。
- 不空壳、不桩化:检查体里必须有能失败的断言。
- 不缓存测试结论冒充执行。

## (4) 高负载判读

高负载下新旧结果一致只能说没变坏,不能记绿灯。超时与未跑写 UNMEASURED。原因:负载掩盖的超时与竞态在低负载时才现形,反之亦然。

## (5) 两轮封顶与安全子集

- 复核至多两轮;第二轮仍阻塞则退回作者拆小,不开第三轮。原因:第三轮通常是在同一错误设计上打补丁。
- 阻塞时给可立即落地的安全子集(例如只落检查不落行为改动),让流水线不断档。

## (6) 复核者对抗清单

| 维度 | 要问的问题 |
|---|---|
| 完备性 | 缓存键是否漏项(输入文件、工具版本、环境);读取方与依赖是否全列 |
| PID 复用 | 判活是否只看 PID;应同时核启动时间或 PGID |
| 符号链接 | Root、快照、锁文件是否可能是符号链接;是否拒绝或解析 |
| 并发 | 两个实例同时跑会否互写;共享表写是否持锁 |
| 崩溃中途 | 写到一半被杀,残留是否被下次误认为完成;是否原子改名 |
| 叠加 | 与已落或待落补丁是否冲突 |

## (7) 落库前补跑项

- 在你自己的 w- 树里 `git checkout --detach` 到最新 origin/main(或交付提交的基点),`git cherry-pick` 交付提交,列重叠冲突;不再用 `git archive` 解整仓到 /tmp(在 w- 树里,devsvc 拦 checkout 时仅限该树用 `DEVSVC_ALLOW_DANGEROUS=1` 前缀)。
- 跑 `tools/affected.cx --files ... --run` 选出的批次。
- 改过的每个 .cx 用已装产品实编一遍。
- 涉及 register 的跑冷环境 33 秒门。

## (8) 裁决格式

回复先一句结论;每项标 通过 / 须改 / 阻塞;每段不超过 400 字;证据路径写 `~/` 不写真实家目录。
