# 0.0.40 误 restore 门闩：受控真实首窗观察（cc，2026-10-10 21:03；机房主任 20:55 授权方案 A）

**这是观察，不是验收。** 只跑具名的 contract 层轻量套件，单窗；正式 K2b 白名单为空，没有任何真实准入。

## 身份
- 树：0489b3cd（QUEUE_SUITES/RELEASE_SUITES 观察模式），status 0 行；
- 候选 unisacc-next.com：2b20f4b2fd9a496d2e36d47577741420f9e4aba47be0d377e3866f5f91aa5ed9（0.0.39 候选；自 82290c17 起产品闭包无变化，release.sh 报 product freshness: verified）；
- UA：fd8f3877e8b7b3bef517b276b16038cb5b54bf3a9fe76ed39ddc046cada971c2；
- 选集核对：`gatequeue.py --com --list-selection --suite checkrun --suite queuestart-strict --suite docedit` 恰好输出这三个名字；
- 正式白名单 tests/k2b-whitelist.tsv：0 条有效行。

## 命令
`QUEUE_SUITES="checkrun queuestart-strict docedit" QUEUE_WINDOWS=1 QUEUE_STATE=<新空目录>/state QUEUE_BACKUP=<新空目录>/backup STAGELOG_RUN=fwprobe2 release/tools/queue.sh /tmp/cc39-b5b/cand /tmp/cc39-b5b/ua /tmp/cc39-b5b/cand/seed`（未设 QUEUE_START，即默认 fresh）

## 读数（证据目录 /tmp/cc40-prep/firstwindow3）
- 启动回执 start-receipt.txt：mode=fresh、restored=no、source=none、artifact_sha256=2b20f4b2…；输出 “backup … left untouched (backup not read)”；
- release.sh：“OBSERVATION ONLY (checkrun docedit queuestart-strict); warm-ups skipped (contract-layer suites build no cache); not acceptance”；
- gatequeue 逐项（行首核，不采信汇总）：START checkrun / docedit / queuestart-strict；DONE checkrun rc=0（0.10 s）、docedit rc=0（0.25 s）、queuestart-strict rc=0（1.16 s）；
- 汇总：queue: 3/3 completed, 0 failed, 0 unverified, 0 pending；
- 结尾：“OBSERVATION ONLY: named suites … completed; not acceptance; release NOT ready (rc 65)”，release-queue.log 记 window 1 rc=65、final rc=65；
- state/observation.json：{"observation": true, "suites": [...], "acceptance": false}；
- stage log（~/.unisacc/stagelog/events.jsonl，run fwprobe2，boot_id 9122943e…）：8 个 begin 全部有对应 end，end 的单调时钟均不早于 begin。分别是：queue（rc 65）、setup、window-1（rc 65）、release-checks、prologue、jobs、epilogue、backup。另有一对 wait-begin/wait-end（gatequeue-alive），用时 0.06 s。

## 失败与未解释项（如实保留）
- 第一次尝试（/tmp/cc40-prep/firstwindow，STAGELOG_RUN=fw40-210214（原写 210213，按 stage log 实际 run id 更正））在 21:02:13 启动：回执 fresh/restored=no 已写出，但 release-queue.log 一直为空，被外层 timeout 58 杀掉（exit 124）。随后同一命令再跑两次（STAGELOG_RUN=0 一次，stage log 打开一次）都在约 2 s 内完成，rc 65。首次的阻塞点后经 stage log 定位（21:1x 补记）：run fw40-210214 只有 queue begin、setup begin 和一条 `wait-begin reason=gatequeue-alive`（13:02:14Z），**没有 wait-end**，即 queue.sh 停在“宿主上另有 gatequeue.py 存活”的等待循环里（queue.sh 最多等 90×2 s），直到被外层 58 s 杀掉。当时是哪个 gatequeue 进程仍为 UNKNOWN（事后 ps 已无）。observationcheck 的一次间歇挂起（exit 142、零启动）症状相同，两次重跑均绿。
- 本次只观察了接线。这不是产品验收，也不证明全量 queue 的行为。真实计时器下的超时首窗仍然缺。

## 新 helper 的真实首窗（firstwindow4，2026-10-10 21:26；机房主任 21:25）

- 树 87eb16ee（含 gqalive.py 进程闸），status 0；候选 2b20f4b2…、UA fd8f3877…、正式白名单 0 行；`--list-selection` 恰好输出三名；运行前 `gqalive.py` rc=1（无在跑 gatequeue）。
- 这次发起运行的 shell 自身命令文本含 `gatequeue.py`（旧逻辑下会自等），新逻辑未被它阻塞。
- 回执：mode=fresh、restored=no、source=none、artifact 2b20f4b2…；“backup … left untouched (backup not read)”。
- release.sh：“OBSERVATION ONLY (checkrun docedit queuestart-strict); warm-ups skipped …; not acceptance”。
- 逐项：START×3；DONE checkrun rc=0（0.10 s）、docedit rc=0（0.25 s）、queuestart-strict rc=0（1.11 s）；queue: 3/3 completed；window 1 rc=65、final rc=65，“OBSERVATION ONLY … not acceptance; release NOT ready (rc 65)”；observation.json acceptance=false。
- stage log（run fw4-212610，按 id 配对，同 boot_id）：共 9 对全部配齐（8 个阶段 begin/end + 1 对 wait-begin/wait-end）——queue（4.328 s，rc 65）、setup（0.55 s）、**wait-begin/wait-end reason=gatequeue-alive（13:26:11Z→13:26:11Z，0.066 s）**、window-1（3.492 s，rc 65）、release-checks、prologue、jobs（1.113 s）、epilogue、backup。
- 证据：/tmp/cc40-prep/firstwindow4（run.log、state/、stage-pairs.txt、run-id）。
- 仍单列：真实计时器触发的超时首窗；BSD ps 按空白切分 argv 非无损；21:02 首窗卡住的进程 UNKNOWN。firstwindow3 不作为本节证据。

## 真实计时器超时首窗（timerfirstwin，2026-10-10 21:39；机房主任 21:38）

- 树 f988066a（tests/queuetimercheck.sh），status 0。真实 release/tools/queue.sh、真实 tests/term.sh 与 tests/bound.py；只有窗口主体 tests/release.sh 是夹具：先记启动、记父进程，再 sleep 16 s（4×alarm）。
- 限时：TERM_SH_ALARM=4（Linux 上 term.sh 直接 exec `bound.py 4 …`）；外层 native bound 50 s；生产窗口预算 55 s 未改。
- 夹具记下的父进程：`python3 ./tests/bound.py 4 env … ./tests/release.sh --com`，说明窗口主体确由 4 s 内层看门狗托管。
- 结果：check rc=0；queue rc=142（内层看门狗，不是外层 124：总耗时约 4–5 s，远小于外层 50 s）；窗口主体启动恰 1 次（追加式计数 1 行），从未写出“完成”标记；release-queue.log 恰一行 `window 1 rc=142`、`final rc=142`，无 failed 0/OBSERVATION ONLY/completed 等通过字样；回执 mode=fresh、restored=no、source=none；无 start-refusals.log、输出无 REFUSED。
- stage log（run tfw-213955，按 id 配对，同 boot）：5 对齐——queue 4.584 s rc 142、setup、wait-begin/end reason=gatequeue-alive 0.065 s、window-1 4.105 s rc 142、backup。
- 证据：/tmp/cc40-prep/timerfirstwin（run.log、fixture/、stage-pairs.txt、run-id）。
- 边界：窗口主体是夹具（sleep），不是真实 gate 套件；首次 f31e76b3 的证据 run 在启动计数改为追加式之前，以本次 f988066a 为准。21:02 首窗卡住的进程仍 UNKNOWN；BSD ps argv 非无损照列。
