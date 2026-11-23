# v0.0.94 S0 合并日逐步检查清单

状态: 就绪检查清单(2026-11-23 整理,对应 `research/2026-11-23/freeze-runbook-draft.md` S0 与 `4c9406d48e` 提交门定稿)。
冻结锁现状核查: `tools/devsvc.cx` 仍无具名 `freeze`/`lock`/`claim` 写窗口锁子命令(仅有构建产物 POSIX 文件锁 `bin.lock`); 冻结期写保护由 `tools/precommit.cx` 读取 `build/freeze.tsv`(现行仅含已过期历史行 `L0-CORE-FREEZE-049`)加 `4c9406d48e`(C1/C2/C5/C6/P2/P3 落库, `deferred-gates` 30 行副作用已清零)兜底。

## S0 逐步执行清单(总预算 5 分钟)

| 步 | 谁执行 | 精确命令 / 动作 | 预期输出行 | 失败动作 |
|---|---|---|---|---|
| S0-0 | 集成者 (`cc`) | `mux` 全仓广播「负载静默开始(S1..S9 暂停重检查)」并查 `uptime` 与 `git status --short` | 1 分钟负载 `< 8`; 共享检出无阻塞性未提交改动 | 负载 >= 8 先等或请占核窗口收尾; 共享检出有未提交他人改动先催清仓,不得用 `stash/reset/clean/checkout` 动他人路径 |
| S0-1 | 集成者 (`cc`) | 更新 `build/freeze.tsv`(写明 `0.0.94`、`base_commit`、`owner=cc`、`deadline_epoch_s` = T0+7200、冻结前缀 `@lane:A` 与 `release/`)并核对 5 列格式 | 5 列无超长行, `tools/precommit.cx` 识别冻结窗口生效 | 列数不对或超长行会被 C1/C2 拒绝,修正后继续 |
| S0-2 | 集成者 (`cc`) | 核对 `v0.0.94` 与 `origin/v0.0.94` 一致且最近一次 `premerge-union` 全绿: `git rev-parse v0.0.94 origin/v0.0.94` | 两行均等于 `2735df3efe...`(或确认的最新头) | 不一致先核对未推提交归属,未核清不合并 |
| S0-3 | 集成者 (`cc`) | 合并 `v0.0.94` 入 `main`: `git merge --no-ff v0.0.94` | `Merge made by the 'ort' strategy.` (0 冲突) | 出现任何合并冲突立即 `git merge --abort` 中止并回 T-1 重做并集预演,不在 T0 后现场解冲突 |
| S0-4 | 集成者 (`cc`) | (仅当 S0-3 大合并触及 `genall`/`derive` 20s `GATE_TIMEOUT` 时) 带 `UNISACC_INTEGRATOR=cc UNISACC_GATE_WAIVE="derive:s0-merge-0.0.94,genall:s0-merge-0.0.94"` 落合并提交,随后立即分门核销: `sh ./unisacc.com tools/precommit.cx -- --verify-deferred --gate derive` 与 `... --gate genall` | `VERIFIED` 哈希链行写入且 `sh ./unisacc.com tools/chain-ready.cx -- --only 1-rev` 输出 `1-rev PASS` | 核销 FAIL 说明真实漂移或门失败,立即中止冻结并定位,禁止跳过核销 |
| S0-5 | 集成者 (`cc`) | 切 `release/candidate.tsv` 为 `0.0.94` 槽(同步更新 `plans/v0.0.94.md` 的 `freeze_at` 为当前 UTC/CST 时间戳),用 `devsvc git commit --whole release/candidate.tsv plans/v0.0.94.md` 提交并 `git push origin main` | `GIT_COMMIT rc=0`, `origin/main` 推进至 `T` | 推送拒或 `MAIN_ADVANCED`: 仅合并 `origin/main` 后重核,严禁 force push |
| S0-6 | 集成者 (`cc`) | 验收版本槽与静态门: `sh ./unisacc.com tools/chain-ready.cx -- --only 11-version-slot --version 0.0.94` 与 `sh ./unisacc.com tools/install-ape.cx -- . --rollback` | `11-version-slot PASS` / `INSTALL_APE ROLLBACK_DRYRUN c561e7e0...` | `11-version-slot` FAIL 立即核 `release/candidate.tsv` 第 4 列; `ROLLBACK_DRYRUN` 缺失先 `install-ape -- . --backup --write` 补 0.0.93 槽后再进 S1 |
| S0-7 | 集成者 (`cc`) | 记 `T0` 时刻(合并+版本槽推送完成时刻)进入 S1(`chain-ready` 按 `--only` 分 2-3 批,`5-candidate-chain` 独占一批) | T0 记账完成,进入 S1 | S0 总耗时 > 7.5 分钟(预算 1.5 倍)按超限规则核因后决定续行或中止 |
