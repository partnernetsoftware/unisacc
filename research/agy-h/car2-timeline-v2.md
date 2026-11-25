# 第二车 (0.0.107) 起链时间账与风险页 (v2)

编写窗口: `agy-h` (只读分析写作, 未执行重命令)
当前时钟锚点: `2026-11-25 01:52 +0800` (`0.0.106` 已于 `01:51:18` 合入解冻提交 `5c6cbff0f2`)
24 小时总截止目标: `2026-11-25 17:50 +0800` (`prd_mempalace.md:44`, 距当前余 `15 小时 58 分钟 = 958 分钟`)

---

## 1. 0.0.106 实测时间账复盘 (原行出处与损耗定因)

所有时间均取自 `origin/main` 与仓内实证文件原行:

| 实测里程碑 | 实测时间戳 (+0800) | 分段耗时 | 累计 (自链起) | 原文出处 (文件与行号) | 备注与异常耗时定因 |
|---|---|---|---|---|---|
| 链起 (`chain_start`) | `2026-11-24 21:31:05` | - | `0.0m` | `research/agy-h/chain-to-public.tsv:6`; `owner.tsv:1` (`1795527065`) | `rev=2e8a19a342` (`mode=full`) |
| 链止 (`chain_stop`) | `2026-11-24 21:58:56` | `27.9m` | `27.9m` | `research/agy-h/chain-to-public.tsv:6`; `exit.tsv:1` (`1795528736`) | 纯整链 499 步干净通过 (`1671s`) |
| 预跑启动 (`ids.txt`) | `2026-11-24 22:05:40` | `6.7m` | `34.6m` | `research/agy-h/prerun-rate.tsv:2,11`; `chain-to-public.tsv:6` | 首波含 `opt-frame4` 材料 (`43.9s`) 与 `W0000` 冷重跑 |
| 预跑完成 (`PRERUN_DONE`) | `2026-11-25 00:08:15` | `122.6m` | `157.2m` | `research/agy-h/prerun-rate.tsv:11`; `release/evidence/0.0.106/prerun-supervise.log:470` | 链止->预跑完 `129.3m`; 含 `W0080` `rc=124 x2` (`:157-159`) 与 `W0087`/`W0191`/`W0206` 红停 (`:174-436`) |
| 链后补跑与 R40 B-t 证明写入 | `2026-11-25 00:27:00` | `18.8m` | `175.9m` | `release/evidence/0.0.106/INDEX.tsv:1-5` | 复跑 `W9997..W9999` 20 批 + 3 次 `W0150` + 录不动点证据 |
| `exec-1-post-chain-merge` | `2026-11-25 00:45:18` | `18.3m` | `194.2m` | `release/evidence/0.0.106/INDEX.tsv:6` | `8ca2f5e4b8` 六文件 R40 B-t 放行证明 |
| `exec-1-X-commit` (`07d767d1eb`) | `2026-11-25 00:50:55` | `5.6m` | `199.8m` | `release/evidence/0.0.106/INDEX.tsv:7` | 链止->X 实测 `172.0m` (超 45m 硬线) |
| 首轮草稿读回 + `ci-45442318936` 派发 | `2026-11-25 00:53:27` | `2.5m` | `202.4m` | `release/evidence/0.0.106/INDEX.tsv:8-9`; `ci-45442318936.json:2` (`16:53:19Z`) | `windows-11-arm` 于 `16:55:43Z` 因草稿 404 失败; `report` 至 `17:09:15Z` (`01:09:15`, 耗 `15.8m`) |
| 二轮草稿读回 + `ci-45446325698` 重派发 | `2026-11-25 01:14:30` | `21.1m` | `223.4m` | `release/evidence/0.0.106/INDEX.tsv:10-12`; `ci-45446325698.json:2` (`17:14:23Z`) | CI 重派发净间隔 `21.1m` (首轮运行 `15.8m` + 等待人工处置 `5.3m`) |
| `exec-3-ci` (`45446325698` PASS) | `2026-11-25 01:30:12` | `15.7m` | `239.1m` | `release/evidence/0.0.106/INDEX.tsv:13`; `ci-45446325698.json:587` (`17:29:51Z`) | 正常 6 格原生 CI 单轮墙钟实测 `15.5-15.7m` |
| `exec-4-Y1` 提交 (`32f26cd024`) | `2026-11-25 01:34:44` | `4.5m` | `243.7m` | `release/evidence/0.0.106/INDEX.tsv:14-17` | 含复验草稿+CI 与 `release-decide` |
| 首次 `exec-5-publish` | `2026-11-25 01:36:31` | `1.8m` | `245.4m` | `release/evidence/0.0.106/INDEX.tsv:18-20` | 首次转公开后公开读回异常触发回滚 |
| 回滚 (`exec-6-rollback`) 与二次 `exec-5-publish` | `2026-11-25 01:42:35` | `6.1m` | `251.5m` | `release/evidence/0.0.106/INDEX.tsv:21-24` | `01:37:39` 回滚退 draft; `01:42:26..35` 重跑步骤 `2/3/5`, 公开事故多耗 `6.1m` |
| 公开读回 + `post-publish` + `exec-7-Y2-done` | `2026-11-25 01:43:00` | `0.4m` | `251.9m` | `release/evidence/0.0.106/INDEX.tsv:25-28`; `public-readback.txt:1` | `c47-1-readback` `01:42:49` 完成(仅 `14s`); `Y2` `9d08f985fa` |
| 收尾 `release-post` + 解冻合入 (`5c6cbff0f2`) | `2026-11-25 01:51:18` | `8.3m` | `260.2m` | `git log -n 6` (`32d7503b67..5c6cbff0f2`) | `old-product` 登记 `0.0.106` + `version-seq` 标公开 + 解冻 |

小结 (`0.0.106` 对 `0.0.107` 的直接输入修正):
- 整链本体 (`chain`): `27.9m` (`owner.tsv:1` -> `exit.tsv:1`), 极稳。
- 正常六格 CI 单轮: `15.7m` (`INDEX.tsv:11-13`), 上轮因重派发损耗 `+21.1m` (`00:53:27 -> 01:14:30`)。
- 公开读回事故重来: 多耗 `6.1m` (`INDEX.tsv:20-24`)。
- 预跑实测总耗时 `122.6m` (`prerun-rate.tsv:11`), 其中仅 `W0080..W0206` 停机与冷热 `outer_overhead` (`24s` -> `2.1s`, `prerun-supervise.log:4,161`) 及 `Path B` 补跑 (`00:08:15 -> 00:50:55`, `42.7m`) 即额外吞噬了约 `85m`。

---

## 2. 0.0.107 从阶段 B 到公开的逐段耗时估计

注: 按 `car2-timeline.md:9`, 阶段 A (`git merge origin/main` + `tables-gen` + `derive` + `layer-api` + 复制公开 `unisacc.com`, 约 `13m`) 先行收口后进入阶段 B; 基准口径参考 `car2-timeline.md:10-25` 与 `car2-commit-sequence.md:41-103` (`precommit` 单笔约 `5-18s`, `tables-gen`/`derive` 约 `30-60s`)。

### (1) 阶段 B 14 份补丁逐份耗时估算 (对齐 `car2-review-queue.md` 与 `car2-commit-sequence.md`)

说明: `research/agyneon/car2-review-queue.md:5-19` 列 `#1..#13` 共 13 序号 (`#13` 并列 `r39e` 候选与 `cc` 遮蔽三批修复两件); 同目录 `car2-commit-sequence.md:58-93` 阶段 B 记 12 编号 (其中 `(3)` 含 `3a`+`3b`), 将 `#13` 拆出 `r39e` (`13a`) 与 `cc` 遮蔽三批 (`13b`) 即为阶段 B 的 14 份补丁 (阶段 C 另有 `v0107-criteria.patch`)。

| 序号 | 队列号 / 阶段 B 编号 | 补丁名 | `precommit` 分钟 | 冲突处置 / 表重算 (`tables-gen`/`derive`) / `-c` 分钟 | 单份合计分钟 | 依据文件与行 |
|---|---|---|---|---|---|---|
| 1 | `#10` / `(1)` | `wp4-material.patch` v2 (`staged_cx=2`) | `0.3m` (`~18s`) | `1.0m` (`tables-gen` + `derive batch-domains,table-index`) | `1.3m` | `car2-commit-sequence.md:58-60`; `car2-timeline.md:10` |
| 2 | `#9` / `(2)` | `wp4-install-ape.patch` v4 (`staged_cx=1`) | `0.2m` (`~12s`) | `1.0m` (`tables-gen` + `derive`) | `1.2m` | `car2-commit-sequence.md:61-63` |
| 3 | `#5` / `(3a)` | `producer-cx-booter-ape.patch` (3a) | `0.2m` (`~12s`) | `1.0m` (`tables-gen` + `derive`) | `1.2m` | `car2-commit-sequence.md:64-66`; `car2-rehearsal.md:12` |
| 4 | `#6` / `(3b)` | `producer-tools-default-regression.patch` (3b) | `0.3m` (`~18s`) | `2.5m` (`tools/affected.cx:876` 人工并集解 + `-c` 实编 + 表重算) | `2.8m` | `car2-commit-sequence.md:64-66`; `car2-rehearsal.md:13` |
| 5 | `#3` / `(4)` | `dvqualify-selector-reuse-v3.patch` | `0.3m` (`~18s`) | `1.0m` (`tables-gen` + `derive`) | `1.3m` | `car2-commit-sequence.md:67-69`; `car2-review-queue.md:9` |
| 6 | `#1` / `(5)` | `precommit-list-cap.patch` v7 (`staged_cx=2`) | `0.3m` (`~18s`) | `1.0m` (`tables-gen` + `derive`) | `1.3m` | `car2-commit-sequence.md:70-72`; `car2-review-queue.md:7` |
| 7 | `#11` / `(6)` | `u08-source-missing.patch` v3 (`staged_cx=1`) | `0.2m` (`~12s`) | `1.0m` (`tables-gen` + `derive`) | `1.2m` | `car2-commit-sequence.md:73-75` |
| 8 | `#12` / `(7)` | `gen-all-drift-fixture-tests.patch` | `0.2m` (`~12s`) | `0.4m` (`apply` + `git show` 核) | `0.6m` | `car2-commit-sequence.md:76-78` |
| 9 | `#7` / `(8)` | `seal-precheck-v2.patch` | `0.3m` (`~18s`) | `0.8m` (`apply --exclude=build/links.tsv` + `derive --only links` + 核编译清单) | `1.1m` | `car2-commit-sequence.md:79-81`; `car2-rehearsal.md:18` |
| 10 | `#2` / `(9)` | `chain-step-reaped-kill.patch` | `0.2m` (`~12s`) | `0.7m` (`tables-gen --write`) | `0.9m` | `car2-commit-sequence.md:82-84`; `car2-review-queue.md:8` |
| 11 | `#8` / `(10)` | `check-tool-material-macro` v3 | `0.3m` (`~18s`) | `2.0m` (落盘核 sha + `tools-compile-sources.tsv` + `tables-gen`/`derive`) | `2.3m` | `car2-commit-sequence.md:85-87`; `car2-review-queue.md:14` |
| 12 | `#4` / `(11)` | `r39c-release-exec-sha.patch` (`staged_cx=2`) | `0.3m` (`~18s`) | `0.5m` (`-c` 实编 `release-exec.cx` 与 `check-release-exec.cx`) | `0.8m` | `car2-commit-sequence.md:88-90` |
| 13 | `#13a` / `(12a)` | `r39e` 候选修复 (待定稿落盘) | `0.3m` (`~18s`) | `1.8m` (核 sha + `-c` 实编 + 表校验) | `2.1m` | `car2-commit-sequence.md:91-93`; `car2-review-queue.md:19` |
| 14 | `#13b` / `(12b)` | `cc` 遮蔽三批修复 (待定稿落盘) | `0.3m` (`~18s`) | `1.7m` (核 sha + `-c` 实编 + `derive`) | `2.0m` | `car2-commit-sequence.md:91-93`; `car2-review-queue.md:19` |
| 合计 | `14 份` | 阶段 B 串行执行合计 | `3.7m` | `16.4m` | `20.1m` (约 `18-22m`) | 注: 若 `precommit` 与 `tables-gen`/`derive` 在阶段 B 末批合并重算, 可压至 `~15-17m` (`car2-timeline.md:10`) |

### (2) 从阶段 B 到公开的端到端逐段耗时表 (基准情景 vs 保守情景)

| 阶段 | 范围与内容 | 基准耗时 (分钟) | 保守耗时 (分钟) | 数据依据 (文件与行) | 是否在关键路径 |
|---|---|---|---|---|---|
| 前导: 阶段 A | 合 `origin/main` + `layer-api` + `tables-gen`/`derive` + 副本复制 | `13m` | `16m` | `car2-timeline.md:9`; `car2-commit-sequence.md:10-37` | 是 (起于 `01:52`) |
| 阶段 B | 14 份补丁落车 (`precommit` `3.7m` + 冲突/表重算 `16.4m`) | `20m` | `32m` | 上表 (14 份汇总); `car2-review-queue.md:21-33` | 是 (部分未审项与关键落车可并行准备) |
| 阶段 C | 合入 `v0107-criteria.patch` (`7d3dc50c...`) + `freeze.tsv` + `chain-ready` (`READY=yes`) | `5m` | `8m` | `car2-commit-sequence.md:97-103`; `car2-timeline.md:11` | 是 |
| 整链 (`chain`) | `499/499` 完整构建 (`owner.tsv -> exit.tsv`) | `28m` | `31m` | `chain-to-public.tsv:2-6` (`27.6/29.7/27.6/27.9m`); `car2-timeline.md:12` | 是 (`T1 -> T2`) |
| 预跑 (`prerun`, 2 道 lane) | `238` 窗 / 2 lane = `119` 窗/lane (`20s-27s/窗`, 已含 `r39c`/`dvqualify v3`) | `50m` (`40-54m`) | `78m` | `car2-timeline.md:13,19-25`; `prerun-rate.tsv:4-11`; `prerun-margin-106.md:10-24` | 是 (`T2` 后启动) |
| 证据、打包与 CI 公开 | `fixedpoint` + `pack`/`prepare` + `release-chain` (`15m`) -> `draft` + 六格 CI (`16m`) -> `Y1/publish/readback/Y2` (`5m`) -> `release-post` (`5m`) | `41m` (`36-45m`) | `68m` (含 CI 重派 `+21m` + 回滚 `+6m`) | `0.0.106` `INDEX.tsv:7-28`; `0.0.105` `INDEX.tsv:1-13`; `car2-timeline.md:14` (`40m`) | 是 (除 `release-post` `5m` 在 `T4` 公开之后) |
| 自阶段 B 到公开累计 | 阶段 B + 阶段 C + 链 + 2 lane 预跑 + 证据与 CI 到 `exec-5-publish` | `139m` (`2h19m`) | `212m` (`3h32m`) | 扣除 `release-post` `5m` 后按基准线估 | 关键路径总长 |
| 自当前 (`01:52`) 含阶段 A 到公开 | 阶段 A (`13m`) + 上项 (`139m`) | `152m` (`2h32m`) | `228m` (`3h48m`) | 对比 `car2-timeline.md:16` (`148-158m`) 一致 | 预计公开 `04:24` (基准) / `05:40` (保守) |
| 其中第 6 条口径 (`T1` 起链 -> `T4` 公开) | 链 (`28m`) + 2 lane 预跑 (`50m`) + 打包至公开 (`36m`) | `114m` (压线 `<=120m`) | `172m` (超 `120m`) | 对标 `clock-rule-draft.md:72-85` 情景 B/C | 决定第 6 条 `0.0.107` 能否拿 `1/3` 样本 |

---

## 3. 与 24 小时目标 (2026-11-25 17:50) 比对

- 总预算: 自当前 `01:52` 至 `17:50` 剩余 `958` 分钟 (`15 小时 58 分钟`, `prd_mempalace.md:44`)。
- `0.0.107` 消耗与剩余空间:
  - 基准情境: `0.0.107` 于 `04:24 - 04:35` 公开 (`release-post` 至 `04:40` 解冻), 剩余 `13 小时 10 分钟` (`790m`)。
  - 保守情境 (含 CI 重派 + 单轮贴限补跑): `0.0.107` 于 `05:40 - 05:50` 公开, 剩余 `12 小时 00 分钟` (`720m`)。
  - 最差翻倍情境 (触发后文 3 项翻倍风险之一): `0.0.107` 自阶段 B 起约 `4.5-5.5` 小时 (`~07:00-07:30` 公开), 仍余 `10 小时 20 分钟` (`620m`), 足以为 `0.1.0` 封版车 (预计 `114-140m`) 及多出一趟兜底版 (`~150m`, 如第 6 条补样本或 `0.0.127`) 留足窗口。
- 但第 6 条时钟红线 (`<= 120m`): 基准 `114m` 仅余 `6m` 安全垫, 任何 `>6m` 的波动(哪怕仅多 1 次 `readback` 回滚 `6.1m` 或 `precommit` 重跑)即使 `0.0.107` `T1->T4` 跨过 `120m`。

---

## 4. 关键路径与可并行清单

### (1) 关键路径 (不可跳过、串行锁死 `152m`)

`阶段 A (13m) -> 阶段 B (关键未冲突/已审补丁 + 阶段 B 末 10-14 笔, 20m) -> 阶段 C (5m) -> 整链 499/499 (28m) -> 2-lane 预跑 238 窗 (50m) -> pack/prepare/release-chain/X (15m) -> draft + 六格原生 CI (16m) -> Y1/publish (2.5m) -> public-readback/Y2/release-post (5.5m)`

### (2) 可并行项 (不占关键路径时间)

1. `阶段 A (13m) || Gatekeeper 波次 1-2` (`car2-review-queue.md:23-28`): 在合 `origin/main` 与重算 4 张派生表期间, 并行完成 `#1/#2/#3/#5/#6/#7/#8` 的负控实跑与定因复核, 避免阶段 B 干等审查。
2. `阶段 B 前 9 笔落车 || #10/#13a/#13b 定稿落盘`: 前半段补丁无需等 `#8`/`#13` 定稿即可推进 (`car2-commit-sequence.md:58-84`), 同时并行执行下一笔私有镜像 `-c` 实编预热。
3. `整链 (28m) || 预跑环境准备 + 0.0.107 说明定稿骨架 + 0.1.0 前置材料`: 并行检查 `frozen-root` 路径短名/干净 `PATH`、回填 [notes-0.0.107-skeleton.md](notes-0.0.107-skeleton.md) 产品改动节、冻结 `0.1.0` 能用清单 30 项 ([seal-0.1.0-evidence-map.md](../ccogweco/seal-0.1.0-evidence-map.md))。
4. `2-lane 预跑 (50m) || fixedpoint-evidence 验证准备 + 0.0.106/0.0.107 声称类剩余关闭验证`: 按 `R38` 口径在非冲突 scratch 并行关闭 `U08`/`E01`/`U09` (`R38-ruling.md:96-101`, 预跑高载阶段避让重负载、在 `normal` 尾段或前段低载穿插)。
5. `六格 CI (16m) || release-run --prepare / expected-fail / notes-fill 核验`: `Y1` 前除 `ci_run_id` 外的所有表格与说明段在 `CI` 运行的 `16m` 内全部写齐待填。

---

## 5. 最可能让时间翻倍的三个风险与预置处置

### 风险 1: 预跑冷态开销与 `55s` 贴限批次引发多次停机与 `Path B` 补跑 (`+70` 至 `+100` 分钟)

- 历史证据 (`0.0.106`): `prerun-supervise.log:4,7` 显示冷态每窗额外耗 `24.0s` (`outer_overhead_ms=24008 / 24089`), 全 `238` 窗总跨时 `122.6m` (`prerun-rate.tsv:11`); 其中 `W0080` (`check-split-parents`, `54947ms`) 前两次 `58.0s rc=124` 停机 (`prerun-supervise.log:157-159`), 且 `notcovered` (`55865ms`)、`module-units` (`55557ms`)、`devsvc-git-c-operations` (`55555ms` 超时) 全部贴近 `55s` 硬限 (`prerun-margin-106.md:13-18`), 叠加 `Path B` 六文件补跑共使 `chain_stop -> X` 膨胀到 `172.0m`。
- 预置处置:
  (1) 阶段 B 必落 `#4 r39c-release-exec-sha` (消除 `W0150`)、`#3 dvqualify-selector-reuse-v3` (消除误冷编) 与 `#12 gen-all-drift-fixture-tests`;
  (2) 预跑启动即带热态暖机与已审豁免继续语义, 且两道 lane 起跑严格清理 `PATH` (`export PATH="/usr/bin:/bin:$HOME/.local/bin"`, `path-b-proof.md:38`) 杜绝 `W9998` `moltbaby/bin/cc` 遮蔽错误;
  (3) 对已知 5 个 `>50s` 贴限批在空闲窗单批先行观测或隔离并发, 避免反复 `rc=124` 中断全部窗口。

### 风险 2: CI 首轮瞬态失败重派发与公开读回回滚事故 (`+27` 至 `+45` 分钟)

- 历史证据 (`0.0.106`):
  - `INDEX.tsv:9-13` 与 `ci-45442318936.json:2,187-236,587`: `windows-11-arm` 因 `fetch` 撞 GitHub 草稿最终一致性 404 被吞后失败, 触发重新 `exec-2` 与重派发 `45446325698`, 净多耗 `21.1` 分钟 (`00:53:27 -> 01:14:30`);
  - `INDEX.tsv:20-24`: 首次 `exec-5-publish` (`01:36:31`) 后读回失败回滚 (`01:37:39`) 再重新执行步骤 `2/3/5` 至最终 `exec-5-publish` (`01:42:35`), 再多耗 `6.1` 分钟; 两项合计多耗 `27.2m`, 若遇 `osx` 格连续超时重派或二次失败更会触停重起链。
- 预置处置:
  (1) 在 `exec-2` 上传草稿后、`exec-3-ci` 派发前先核 `gh release view` 六件资产全部列表可见, 降低草稿最终一致性 404 概率;
  (2) 若 CI 首次失败, 按 `prd_mempalace.md:146` 立即先读发码行与单格日志再只重派 `gh run rerun --failed` 一次, 第二次失败直接停手定因不空转;
  (3) 保持干净 `PATH` 与本地工具暖缓存后再跑 `exec-5` 与 `c47-1-readback`, 消除环境造成的 `exec-6-rollback`。

### 风险 3: 阶段 B 14 份补丁中未审/未定稿项阻塞或引发合入后 `affected` 大面积转红 (`+40` 至 `+90` 分钟)

- 历史证据: `car2-review-queue.md:5-19` 中 13 序号共有 8 项尚处未独立复核/部分复核状态 (`#2, #3, #5, #6, #7, #8, #12, #13`), 其中 `#8` (`check-tool-material-macro v3`) 待落盘核 sha、`#13` (`r39e` + `cc` 遮蔽三批修复) 待定稿落盘, 且 `#6` (`3b`) 与 `#5` (`3a`) 在 `tools/affected.cx:876` 存在真冲突 (`car2-rehearsal.md:13`), 仅 Gatekeeper 预计就需 `37m` (`car2-review-queue.md:33`), 一旦某笔导致 `chain-ready` 或 `precommit` 返工甚至重起链, 阶段 B+C+返工链总耗时可直接翻倍。
- 预置处置:
  (1) 严格执行 `car2-commit-sequence.md:58-93` 既定顺序, 第 `1..10` 批不依赖 `#8`/`#13` 先行合入提交与预验证;
  (2) Gatekeeper 审查分波提前至阶段 A 并行, 每波上限 2 路;
  (3) 启动条件搭车裁决线: 到阶段 B 第 12 笔完成节点, 凡 `#8` 或 `#13` 中尚未定稿且未过 Gatekeeper 绿灯的条目, 立即剥离转排 `0.1.x` (不阻塞核心 `B1 + F1 + P0 + r39c + dvqualify-v3 + list-cap` 发车, 按 `prd_mempalace.md:47` 保 `0.1.0` 主节奏)。
