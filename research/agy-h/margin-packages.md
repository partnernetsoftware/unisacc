# S1 超限归因工作包与 S5 零记录低成本冷态测量方案 (2026-11-24)

依据 [margin-audit.tsv](margin-audit.tsv) 与只读源码核对(未改仓、未碰 `.claude/worktrees/v0.0.106`、遵守 `REV 2e8a19a342` 压载纪律),对 S1 全部 38 批与已合拆仍超线的 3 组共 43 个去重批次完成归因分组(按改动域合并为 8 个工作包),并为 S5 的 459 个零记录已实现 `commit` 批提供低成本冷态测量方案。

## 1. S1(38 批) + 已合拆仍超线 3 组(共 43 批) 归因总览

三类超时主因判定定义:
- (A) **每片重编闭包/材料**: 批内现场 `-o` 编译大工具(`devsvc-core.cx`、`precommit.cx`、`commit-own.cx`、`layer-api.cx` 等)或无材料声明反复冷启 `sh ./unisacc.com`,可抽出 `{*-tool}` 材料准备相(同 `module-units` 做法)或复用预编译原生执行体。
- (B) **真实工作量大**: 检查自身用例矩阵庞大、全仓 `git archive` / 扫表 / 大文件 SHA 过多,须拆 `--cases`/`--part` 分片或轻量化夹具目录。
- (C) **依赖第二个工具链启动**: 批内调用宿主 `gcc`/`clang`/`kernel-route.cx` 汇编器、生成后再启动新产品、或嵌套 `run-batch.cx` / `SKILL.md` 多段子进程链。

## 2. 八个拆批/优化工作包 (WP1..WP8)

### WP1: `devsvc` 核心与冻结/Git/并发检查预编译材料包 (9 批)

- **批次清单**: `devsvc-core-b` (`50.60s`)、`devsvc-core-c` (`48.60s`)、`devsvc-remain` (`51.18s`)、`devsvc-compile-context` (`48.42s`)、`devsvc-run-b2` (`43.64s`),及已合拆仍超线组 `devsvc-freeze` (`43.85s` / pre `54.01s`)、`devsvc-freeze-owner` (`24.17s` / 冷 `53.15s`)、`devsvc-freeze-scope` (`25.02s` / 冷 `41.81s`)、`devsvc-git-c` (`c1a` 冷 `27.385s` 贴 27.5s 线)。(同域顺带 `S2` `devsvc-git-unstage` 冷 `48.58s`)
- **归因**: 主因 **(A) 每片重编闭包/材料**。[check-devsvc-remain.cx:112-113](../../tools/check-devsvc-remain.cx#L112-L113) 批内 `-o` 编译 `devsvc-core.cx` 与 `devsvc.cx`;[check-devsvc-compile-context.cx:93](../../tools/check-devsvc-compile-context.cx#L93) 反复 `sh ./unisacc.com tools/devsvc.cx`;[check-devsvc-core.cx:84-99](../../tools/check-devsvc-core.cx#L84-L99) 单跑 `b`/`c` 时 `pin` 未热需冷编 `devsvc-core`;[check-devsvc-freeze.cx:195-224](../../tools/check-devsvc-freeze.cx#L195-L224) 虽接 `{devsvc-core-tool}` 但三片各自 `git archive HEAD` + 多次真实 `git commit` 在临时仓内冷编译运行 `commit-own.cx`;[check-devsvc-git.cx:271](../../tools/check-devsvc-git.cx#L271) (`c1a`) 批内 `-o` 编译 `commit-own.cx` 单步即占 `13.16-13.68s` (`g35-native-c1a-cold-1..3.log`)。
- **建议做法**: (1) 为 `devsvc-remain`、`devsvc-compile-context`、`devsvc-core-b/c`、`devsvc-run-b2` 接入 `{devsvc-core-tool}` 与 `{devsvc-launcher-tool}` 材料相;(2) `check-devsvc-freeze` 与 `check-devsvc-git` (`c1a`) 增接 `{commit-own-tool}` 并在 `freeze` 三片间改用轻量只读基础树替代各片全仓 `git archive`。
- **预计冷态(推断)**: 剥离 13-22s 工具冷编后,各批预计降至 **12-22s** (`devsvc-git-c` c1a `27.3s - 13.5s -> 约 14s`)。
- **写域路径**: `tools/check-devsvc-remain.cx`、`tools/check-devsvc-compile-context.cx`、`tools/check-devsvc-core.cx`、`tools/check-devsvc.cx`、`tools/check-devsvc-freeze.cx`、`tools/check-devsvc-git.cx`、`tests/batches.tsv`、`tests/batch-product-inputs.tsv`、`tests/manifests/devsvc-*.tsv`。
- **主刀窗口**: `ompgpt`
- **在飞补丁冲突**: **高** — 与 `0.0.106` 第一车、`0.0.107` 第二车、`0.0.125` 均在 `tests/batches.tsv` / `tests/batch-product-inputs.tsv` / `plans/devsvc.md` 重叠,并受 `cdx` 未合 `g35-followup-v2.patch` 影响,须第一车解冻后串行 `--keyed` 落库。

### WP2: `precommit-gates` 与 `commit-own` 提交门材料化及 `toolsloc` 分离 (9 批)

- **批次清单**: `commit` 层 `precommit-gates-na` (`53.08s`)、`precommit-gates-toolsreg-a2` (`53.13s`)、`precommit-gates-toolsreg-d` (`52.12s`)、`precommit-gates-reuse-commit` (`50.96s`)、`precommit-gates-reuse-key` (`46.39s` KILLED)、`commit-own-reg-a` (`47.25s` 首冷); `daily` 层(`45s` 限) `precommit-toolsloc` (`46.15s` KILLED)、`precommit-toolsloc-source` (`46.27s` KILLED)、`precommit-toolsloc-symlink` (`46.23s` KILLED)。
- **归因**: 主因 **(A) 每片重编闭包/材料**,次因 **(B) 真实工作量大**。[check-precommit-gates.cx:127-140,160-195,353](../../tools/check-precommit-gates.cx#L127-L140) 每片先调 `prebuild-self`,再全仓 `git archive HEAD` + `tool-index --tables` + `git commit`,且 `na` 直跑 `sh ./unisacc.com precommit.cx` / `precommit-base.cx` / `affected.cx`,`toolsreg-d`/`reuse-*` 批内 `-o` 编译 `precommit.cx` / `commit-own.cx`;[check-commit-own.cx:523,578-617](../../tools/check-commit-own.cx#L523) 在临时克隆冷跑 `devsvc.cx -- run --local commit-own.cx`;[check-precommit-toolsloc.cx:158](../../tools/check-precommit-toolsloc.cx#L158) 在 45s 限内调用 `rn_prepare` 编译 `tools/precommit.cx` 并运行约 18 个夹具提交。
- **建议做法**: 抽出 `{precommit-tool}`、`{commit-own-tool}`、`{precommit-base-tool}` 预编译材料并在 `na` / `reuse-*` / `toolsreg-*` / `commit-own-reg-a` / `precommit-toolsloc*` 全量消费;把 `precommit-gates` 每次全仓 `git archive` + `tool-index` 替换为最小清单夹具根。
- **预计冷态(推断)**: 剥离约 18-22s 工具冷编与 5-8s 归档建仓后,预计降至 **15-24s** (`precommit-toolsloc*` 在 45s 限下须控 `<= 22s`)。
- **写域路径**: `tools/check-precommit-gates.cx`、`tools/check-commit-own.cx`、`tools/check-precommit-toolsloc.cx`、`tools/fixture-gate-tool.cx`、`tests/batches.tsv`、`tests/batch-product-inputs.tsv`、`tests/manifests/precommit-gates*.tsv`。
- **主刀窗口**: `cdx`
- **在飞补丁冲突**: **中** — 与 `0.0.106`/`0.0.107` 共享登记表邻接;与 `R28` (`omp-sonnet` 改 `tools/affected.cx`) 及 `affected-canary` 门修复(`check-precommit-gates.cx:179-196`)有行为耦合。

### WP3: Git 重放/落库/提交前自检分片与二进制复用 (4 批)

- **批次清单**: `push-replay-commit-push` (`53.13s` FAIL)、`land-patch` (`46.35s` KILLED),及已合拆仍超线组 `premerge-selfcheck-main` (`53.23s` FAIL)、`premerge-selfcheck-rd` (`40.42s` PASS)。
- **归因**: `push-replay-commit-push` 与 `land-patch` 主因 **(A) 每片重编闭包/材料** ([check-push-replay.cx:939-947](../../tools/check-push-replay.cx#L939-L947) 批内构建 `cli_image`/`push_image`/`commit_image`/`sync_image` 多枚原生二进制;[check-land-patch.cx:35,47](../../tools/check-land-patch.cx#L35) 全仓 `git ls-files | tar` 后冷跑 `land-patch.cx` + `precommit` 预演); `premerge-selfcheck-main` 与 `-rd` 主因 **(B) 真实工作量大 + 多次重启动** ([check-premerge-selfcheck.cx:91-96](../../tools/check-premerge-selfcheck.cx#L91-L96) 队列内每个子用例均调用 `sh ./unisacc.com tools/premerge-selfcheck.cx -- ...` 并反复建仓打补丁)。
- **建议做法**: (1) `check-push-replay.cx` 接入 `{push-replay-tools}` 预制材料相;(2) `check-land-patch.cx` 改为最小文件夹具 + 预编 `land-patch.cx`;(3) `check-premerge-selfcheck.cx` 批首用预编好的 `{premerge-selfcheck-tool}` 替代每次 `sh ./unisacc.com`,必要时把 `main` 再拆成 `main-core` 与 `main-guards`。
- **预计冷态(推断)**: `push-replay-commit-push` 与 `land-patch` 预计 **16-24s**;`premerge-selfcheck` 复用预编二进制并再分片后预计单片 **15-22s**。
- **写域路径**: `tools/check-push-replay.cx`、`tools/check-land-patch.cx`、`tools/check-premerge-selfcheck.cx`、`tests/batches.tsv`、`tests/split-parents.tsv`、`tests/coverage.tsv`。
- **主刀窗口**: `ompgpt`
- **在飞补丁冲突**: **低** — 第一/二车未改这三个检查源码,仅需注意 `premerge-selfcheck` 同步更新 `tests/split-parents.tsv` 并通过 `check-slow-split`。

### WP4: 核心表/棘轮/计时/安装校验工具预编译与分片 (5 批)

- **批次清单**: `commit` 层 `layer-api` (`53.117s` 冷超时 / 热 `24.7s`)、`batch-timing` (`55.453s` 首冷);`daily` 层 `tsv-lint` (`55.83s` KILLED)、`ratchets-tool` (`46.19s` KILLED / 峰 `53.12s`)、`install-ape` (`46.42s` KILLED / 峰 `55.61s`)。
- **归因**: `layer-api` ([check-layer-api.cx:279-280](../../tools/check-layer-api.cx#L279-L280) 批内 `rn_prepare` 现编 `layer-api.cx` + `iface-hash.cx` 两工具,热跑 24.7s 说明冷编占 ~28s) 与 `batch-timing` ([check-batch-timing.cx:78](../../tools/check-batch-timing.cx#L78) 批内 `tq_native` 编 `batch-timing.cx`) 主因 **(A) 每片重编闭包/材料**; `tsv-lint` ([check-tsv-lint.cx:200,223-224](../../tools/check-tsv-lint.cx#L200) 现编工具+全扫真库 `exec/`)、`ratchets-tool` ([check-ratchets.cx:35,103](../../tools/check-ratchets.cx#L35) 18 条棘轮计划与子命令起多次 `sh ./unisacc.com`)、`install-ape` ([check-install-ape.cx:445-612](../../tools/check-install-ape.cx#L445-L612) 反复写算 ~7MB APE 摘要及 split2 回滚) 主因 **(B) 真实工作量大**。
- **建议做法**: `layer-api` 抽 `{layer-api-tool}` / `{iface-hash-tool}` 材料相并将 `real` 校验与夹具变异拆两片;`batch-timing` 抽 `{batch-timing-tool}` 材料相;`tsv-lint` 拆出实库扫描 `tsv-lint-live` 并预编 `tsv-lint`;`ratchets-tool` 拆 `ratchets-plan` 与 `ratchets-run`;`install-ape` 拆 `install-ape-basic` 与 `install-ape-split2`。
- **预计冷态(推断)**: `batch-timing` `< 10s`;其余拆材料+分片后各 **12-23s**。
- **写域路径**: `tools/check-layer-api.cx`、`tools/check-batch-timing.cx`、`tools/check-tsv-lint.cx`、`tools/check-ratchets.cx`、`tools/check-install-ape.cx`、`tests/batches.tsv`、`tests/coverage.tsv`。
- **主刀窗口**: `cdx`
- **在飞补丁冲突**: **低** — 与第一/二车源码不冲突;须确认第二车合并后的 `build/layer-api/*.tsv` 与 `layer-api` 分片基线一致。

### WP5: `module-units` 落地既有拆分补丁 (1 批 -> 3 片)

- **批次清单**: `module-units` (`daily`, `44.72s` KILLED / 峰 `54.09s` / `baseline-resume` `40.51s`)。
- **归因**: 主因 **(A) 每片重编闭包/材料 + (B) 真实工作量大** — 单批同时覆盖单元展开、转发边界与 1..257 上限检测。
- **建议做法**: 直接复核并合入 `research/` 已有拆批补丁 `~/.local/share/unisacc/research/ompgpt/r35-module-units.patch` (及 `r35-module-units-ac48.patch`),拆为 `module-units` / `module-units-forward` / `module-units-limits` 并接 `{module-units-tool}` / `{check-unit-limit-tool}`。
- **预计冷态(已有实测)**: `~/.local/share/unisacc/research/ompgpt/r35-module-units-evidence.md:16-24` 三次实测冷态分别为 **7.46-7.70s**、**6.87-7.08s**、**7.12-7.29s**。
- **写域路径**: `tests/module/check.cx`、`tests/batches.tsv`、`tests/coverage.tsv`、`tests/batch-domains.tsv`、`tests/batch-product-inputs.tsv`。
- **主刀窗口**: `agyneon` (补丁落库;需核对 `CXM_MAXU=256`)
- **在飞补丁冲突**: **中** — `research/ccogweco/car2-next.md:70` 已列为 `0.0.107` 第二车预热对象,需与第二车合并序及 `CXM_MAXU=256` 限制改动对账。

### WP6: 发布引导器 / 内核路由 / 产物自检链与嵌套包装 (5 批)

- **批次清单**: `cx-booter-ape` (`release`, `45.69s` KILLED, 且挂账 `no-BATCH_RUN`)、`cx-npfault` (`daily`, `44.65s` KILLED / 峰 `53.95s`)、`s8c-bind` (`daily`, `46.46s` KILLED / 峰 `55.78s`)、`wt-gate-clean` (`daily`, `44.66s` KILLED / 峰 `54.02s`)、`dev-product` (`daily`, `39.09s` UNKNOWN `commit-tier-timings.tsv`)。
- **归因**: `cx-booter-ape` ([check-booter-ape.cx:54,60,111](../../tools/check-booter-ape.cx#L54) 现跑两次 `release-ape.cx` + 调 `kernel-route.cx` 现编 `x86_64.kernel`) 与 `dev-product` ([check-dev-product.cx:213-219](../../tools/check-dev-product.cx#L213-L219) 调 `--seed-gen`/`--seed-gen=gcc-O2`) 主因 **(C) 依赖第二个工具链启动**; `cx-npfault` ([check-npfault.cx:47](../../tools/check-npfault.cx#L47) 嵌套起 `tests/run-batch.cx`)、`s8c-bind` ([check-s8c-bind.cx:7](../../tools/check-s8c-bind.cx#L7) 批内再冷编 `tools/s8c-bind.cx`)、`wt-gate-clean` ([tests/check-wt-gate.cx:100](../../tests/check-wt-gate.cx#L100) 并行 `-o` 编译 `run-batch.cx` 与 `all-waves.cx`) 主因 **(A) 每片重编闭包/材料**。
- **建议做法**: `cx-booter-ape` 改用独立 `{x86_64-kernel}` 与 `{release-ape-tool}` 材料相(并按 `prd_mempalace.md:151` 补全 `booter-raw-elf` 生产者);`cx-npfault` / `wt-gate-clean` 复用 `{run-batch-tool}` / `{all-waves-tool}`;`s8c-bind` 改测预编 `s8c-bind` 或并入 `s8c` 工具自检;`dev-product` 拆出 `dev-product-seedgen`。
- **预计冷态(推断)**: `s8c-bind` / `cx-npfault` / `wt-gate-clean` 去重编后 **10-20s**; `cx-booter-ape` / `dev-product` 拆分后 **15-24s**。
- **写域路径**: `tools/check-booter-ape.cx`、`tools/check-npfault.cx`、`tools/check-s8c-bind.cx`、`tests/check-wt-gate.cx`、`tools/check-dev-product.cx`、`tests/batches.tsv`、`tests/batch-product-inputs.tsv`。
- **主刀窗口**: `ccogweco`
- **在飞补丁冲突**: **中** — `v0.0.106` 改 `src/drive.cx` / `src/host.cx`;`v0.0.125` 改 `cx/sys/proc.cx` 进程组语义,与 `cx-npfault` 直接交集。

### WP7: 编译器/运行时跨宿主工具链与多轮构造批 (6 批)

- **批次清单**: `cx-procguard` (`daily`, `40.08s` UNKNOWN)、`unit-boundary` (`daily`, `39.22s` UNKNOWN)、`source-bom` (`daily`, `39.09s` UNKNOWN)、`clock-targets` (`daily`, `39.09s` UNKNOWN)、`p3-swap` (`daily`, `39.06s` UNKNOWN)、`cx-host-run` (`daily`, `38.08s` UNKNOWN)。
- **归因**: 6 批均只有历史 `tests/commit-tier-timings.tsv` (39s 窗截断),缺近期单跑实测。源码归因: `cx-host-run` ([tests/cx-host-run.cx:3,565](../../tests/cx-host-run.cx#L3) 编两宿主驱动 + 调 `gcc`/`clang` 跑矩阵)、`source-bom` ([tests/check-source-bom.cx:9-10,168-202](../../tests/check-source-bom.cx#L9-L10) `cc`/`gcc` 编 `src/cxmod.c` 再跑 14+ 用例)、`clock-targets` ([tools/check-clock-targets.cx:33](../../tools/check-clock-targets.cx#L33)) 主因 **(C) 依赖第二个工具链启动**; `unit-boundary` ([tests/unit-boundary.cx:79](../../tests/unit-boundary.cx#L79) 编跑 1..129 多组规模单元)、`p3-swap` ([tools/lab-loader/check-p3-swap.cx:253-488](../../tools/lab-loader/check-p3-swap.cx#L253) 6 次冷启 `p3-swap.cx`)、`cx-procguard` ([tools/check-procguard.cx:1-14](../../tools/check-procguard.cx#L1-L14) 起多组 `sleep` 子进程及最多 5 轮 `/proc` 重试) 主因 **(B) 真实工作量大**。
- **建议做法**: 解冻后先对这 6 批在 55s 限下单跑 1 次冷态摸底;对复测仍 `> 27.5s` 者:`cx-host-run` 按 `gcc` / `clang` 拆两批,`unit-boundary` 按 1..72 与 127..129 拆两批,`p3-swap` 批首一次 `-o` 编出 `p3-swap` 再跑 6 用例。
- **预计冷态(推断)**: 复测后多批预计已 `< 27.5s`,须拆分批拆分后 **12-22s**。
- **写域路径**: `tests/unit-boundary.cx`、`tests/check-source-bom.cx`、`tests/cx-host-run.cx`、`tools/check-clock-targets.cx`、`tools/lab-loader/check-p3-swap.cx`、`tools/check-procguard.cx`、`tests/batches.tsv`。
- **主刀窗口**: `agy-h` (解冻后先做 1 次 55s 限冷态复测,再按实测决定拆批)
- **在飞补丁冲突**: **中** — `0.0.107` 第二车的 `cxm-maxu-256.patch` 修改模块上限为 256,直接冲击 `tests/unit-boundary.cx` (`128/129` 边界断言);`0.0.125` 改 `proc`/`reaper` 关联 `check-procguard.cx`。

### WP8: 切片清单 / 全图检查 / 文档自检与已拆父批清理 (4 批)

- **批次清单**: `slice-inventory` (`daily`, `39.08s` UNKNOWN)、`flow-tool` (`daily`, `39.06s` UNKNOWN)、`skill-cx` (`daily`, `39.06s` UNKNOWN)、`signal-targets` (`daily`, `39.14s` UNKNOWN, 已合拆父批)。
- **归因**: `slice-inventory` ([tools/check-slice-inventory.cx:75](../../tools/check-slice-inventory.cx#L75) 批内调 `rn_prepare` 编译 `symindex.cx` + `slice-inventory.cx`,虽已传 `--part 1`) 主因 **(A) 每片重编闭包/材料**; `flow-tool` ([tools/check-flow.cx](../../tools/check-flow.cx) 扫全库 `flow -- graph` 与 `check`) 主因 **(B) 真实工作量大**; `skill-cx` ([tests/check-skill-cx.cx:63-70](../../tests/check-skill-cx.cx#L63-L70) 逐段 `sh -c` 跑文档全部代码块) 主因 **(C) 依赖多段工具链启动**; `signal-targets` 为 `tests/split-parents.tsv` 已拆分历史父批(主因 B, 现已有 `-gen`/`-posix`/`-win` 三片)。
- **建议做法**: 先对前 3 批做 1 次现行冷态复测;若仍 `> 27.5s`,将 `slice-inventory` 抽 `{symindex-tool}` 材料相、`skill-cx` 按节拆两批;同时核对 `tests/split-parents.tsv` 四个已拆父批(`signal-targets`、`win-argv`、`snapshots-cx`、`premerge-selfcheck`)的默认排除策略,避免父批继续被 `daily` 选中超时。
- **预计冷态(推断)**: `flow-tool` 复测预计 `< 25s`,`slice-inventory` / `skill-cx` 优化后 **12-20s**。
- **写域路径**: `tools/check-slice-inventory.cx`、`tools/check-flow.cx`、`tests/check-skill-cx.cx`、`tests/split-parents.tsv`、`tests/batches.tsv`。
- **主刀窗口**: `ccogweco`
- **在飞补丁冲突**: **无** — 第一/二车及 `0.0.125` 均未触及上述文件。

---

## 3. S5 零记录 `commit` 459 批低成本冷态测量方案 (只写方案不执行)

### (1) 范围与结构特征

`S5` 共 **459** 批(`commit` 层 `IMPLEMENTED`),覆盖 **381** 个唯一入口脚本(`entry`):
- **无材料批 439 批**: 大部分 1-3 批/入口;前 10 个多批入口占 63 批(如 `tools/check-textx-json-build.cx` 13 批、`tools/check-lower-prefix.cx` 10 批、`check-osx-sentinel.cx` 6 批、`check-textx-json-emit.cx` 6 批、`check-textx-regex.cx` 6 批、`check-ext-sqlite.cx` 5 批等)。
- **带材料批 20 批**: 集中在 `check-chain-run.cx` (3)、`check-s9-nightly.cx` (3)、`check-libneed-native-material.cx` (3)、`release-exec-scenario.cx` (3)、`check-source-ident-cli.cx` (2)、`check-candidate-chain.cx` (2)、`check-release-prep.cx` (1) 等。

### (2) 隔离冷态与 `batch-times.tsv` 落表规范

- **执行时环境**: 依据 [tests/accept-profiles.tsv](../../tests/accept-profiles.tsv) `cold-fresh-layout-33s` 口径,单批执行设置 `UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0`(保留默认 `E3CACHE`,不设 `UNISACC_E3CACHE=0`),并发严格 `<= 2`,外层 `timeout 60` (`run-batch` 自身按 `budget_s` 截断,最多 55s)。
- **独立运行落表机制**: 依据 [tests/run-batch.cx:301-306](../../tests/run-batch.cx#L301-L306),显式设置环境变量 `UNISACC_BATCH_TIMES=/tmp/unisacc-s5-cold/batch-times.tsv` (不设 `UNISACC_TABLES`),`tests/run-batch.cx` 在每批结束会自动追加标准 8 列(`ts_s id status total_ms run_ms kit load1 src=run-batch`),不与在途车共享同一工作流或结论文件。
- **归档回写**: 每波完成后,先跑 `sh ./unisacc.com tools/batch-timing.cx -- report --table /tmp/unisacc-s5-cold/batch-times.tsv` 核对无误,再将记录前加 `# S5-cold rev=<HEAD> UNISACC_RUN_CACHE=0` 注释头后追加到 `~/.cache/unisacc/batch-times.tsv` ([tools/batch-timing.cx:69](../../tools/batch-timing.cx#L69) 原生忽略 `#` 注释行并立即计入下一轮 `report`/`split`)。

### (3) 分波安排与何时执行 (`哪些链后跑`)

- **纪律前提**: `0.0.106` 冻结+全量整链期(`REV 2e8a19a342`)严禁开跑。须在第一车整链完成并解冻后,于机器低载窗口(`并发 <= 2`)分三段推进,每批 `<= 60s`:
  - **第 A 波(代表入口首测,约 65 批,链后立即可跑)**: 对无材料 439 批中多批同 `entry` 只取第 1 批 + 近期新增高复杂度模块(`layer-api-real-b/c`、`layer-api-fixture-bump-a`、`dup-census`、`flowadmit`、`entry-plan-memo`、`precommit-flow`、`textx-*`、`ext-*`、`lower-prefix` 首片),`-j 2` 执行。预期耗墙钟 **10-15 分钟**;若某多批 `entry` 首片冷态 `> 27.5s`,立即标记该 `entry` 全组优先进拆批队列而不盲测余片。
  - **第 B 波(20 批带材料 `commit` 批,须在解冻且无任何后台链时 `-j 1` 串行跑)**: 先单跑一次 `affected --ids <20批> --prepare-materials` 把材料准备完(只记 `PREPARED`),再逐批以 `UNISACC_RUN_CACHE=0 UNISACC_UNIT_CACHE=0` 直接调 `tests/run-batch.cx` 传入已准备材料路径。因涉及 `chain-run`/`s9-nightly`/`release-exec-scenario`/`candidate-chain`/`release-prep` 内部可能拉起较重夹具,`-j 1` 防互扰,预期 **10-12 分钟**。
  - **第 C 波(其余约 374 批无材料批,按每片 45 批分 9 片)**: 可在解冻后日常空闲窗口随时逐片执行(`-j 2`,每片约 **6-9 分钟**,可断点续接),逐片检验并追加到 `~/.cache/unisacc/batch-times.tsv`。
- **退出判据**: 全部完成后跑一次 `sh ./unisacc.com tools/batch-timing.cx -- report --near-ms 27500`,将新暴露出 `max_ms > 27500` 的 `commit` 批并入下一车(第三车 `0.0.108` 或 `0.1.0` 封版前)拆批工作包。


## 4. 0.0.106 预跑新增贴限余量缺口 (放 0.1.x 余量包)

依据 `0.0.106` 第一车预跑 `~/.cache/unisacc/prerun/v0.0.106-2e8a19a342/strict-W0080.tsv` (限 `55s`, 续跑于 `/tmp` layout `tree-hgxir4ip`, `load1=2.50`, 入口 `tests/batches.tsv:1674`, 无材料声明 `-`):

| 批名 | 本次实跑 (`0.0.106`) | `0.0.105` 实跑 | 冷/热状态 | 建议归属 | 预跑完成后待查项 |
|---|---|---|---|---|---|
| `check-split-parents` | `54947ms` (`PASS`, `rc=0`, 紧贴 `55000ms` 限; 前两轮均 `58.0s` `rc=124` 超窗) | `38490ms` | 冷/热状态未知 | 拆片或材料相, 放 `0.1.x` 余量包 (写域: `tools/check-split-parents.cx`、`tests/batches.tsv`) | 待 `0.0.106` 预跑完成后排查比 `0.0.105` 慢约 `16.5s` (`54947` 减 `38490 = 16457ms`) 的根因 (`run-cache` 命中状态 / 负载 / `W0080` 内核编译工作量) |
