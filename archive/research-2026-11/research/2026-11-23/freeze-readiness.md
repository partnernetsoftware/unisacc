# 0.0.94 冻结就绪看板

更新时间: 2026-11-23 05:48 (+08:00)
实测基点: `main` HEAD `e64cadfdd4b833aef7aaab91d507fc4a4a34c871` (`origin/main..HEAD` 领先 49 提交), `v0.0.94` 远端备份已同步 (`2735df3efe`), 本机 Root `0.0.93` (`c561e7e0...`)

## 一句话结论

**当前不可立即进 S0 冻结(接近就绪, 仅余 3 项硬阻断)**: 须先 (1) `gatekeeper` 完成提交门与 `devsvc freeze` 锁二轮甲 `(A)(B)(D)` 窄复核并收口工作区中 `agy-flow` 的未提交尾项, (2) 清理共享检出 24 个未提交/未跟踪文件并把 `origin/main` 领先的 49 笔提交推送归零, (3) 查明 05:00:07 276 个 `cx/` 权限位变更来源以防冻结期复发; 上述 3 项清零且集成者填齐 `plans/v0.0.94.md` `freeze_at` 后即可按 S0 清单冻结(osx 四项探针归 S3b、`l0-posix2` 首跑归 S3, 均不阻断 S0)。

## 阻塞项排序 (按冻结放行优先级)

1. **P0-1 门禁复核与尾项收口**: `gatekeeper` 正在窄复核 `devsvc freeze` 锁二轮提交甲 (`a9cc0de6d8`) 及提交门复核收口; 同时共享工作区中 `tools/{precommit,chain-ready,check-precommit-derive,check-precommit-freeze}.cx` 仍有 `agy-flow` 未提交改动。
2. **P0-2 冻结前清仓与 `origin/main` 推送**: 共享检出当前仍有 **24** 个未提交/未跟踪路径(`tools/release-post.cx` + `check-release-post.cx` 暂存中、`tools/cap-probe.cx` + `check-cap-probe.cx` 未跟踪、`plans/slice-audit.md` 暂存、`build/links.tsv` 等), 且本地 `main` 领先 `origin/main` **49** 提交待审核推送。
3. **P0-3 `cx/` 权限位突变来源未查明**: 05:00:07 276 个 `cx/` `.cx` 文件 100644 -> 100755 变更虽已还原(`cx_mode_dirty_count=0`), 但触发源尚未查明, 存在冻结窗口期再次污染产品输入目录的风险。
4. **P1 (不阻断 S0, 归冻结后既定阶段)**: `plans/v0.0.94.md` `freeze_at=TBD` (S0 填)、`build/version-seq.tsv` 0.0.94 状态 `建议` (S10 `mark-version-seq` 转已公开)、`l0-posix2` 首跑 (S3)、`osx` 四项探针样本 (S3b `ci-early`)、`release-post` 工具 (S10 公开后动作)。

## 冻结前置与新增项就绪总表

| 编号 | 项 | 现状 | 证据提交号与命令原行 | 负责窗口 | 剩余动作与预计 |
|---|---|---|---|---|---|
| 1 | `premerge-union` 与 `merge-version` S0 预演 | 已完成 | `7d3d24de98` / `baa5877d07` / `5be7e2dbdf` (`merge-version` 规则表合并预演 + 私有仓库 gate 检出 + `merge-traps` 缺项记 `NA`); S0 预演 READY (树 `3888236d`) | `omp-sonnet` | S0 合并前按最新 `main` 复核一次 (~2 分) |
| 2 | 冻结合并预演与 `no-home-path`/`scrub-home-paths` | 已完成 | `c0da7baf14` (私有并集副本 `no-home-path ok over=0`, `scrub-home-paths` v0.0.94 新增 0 越线); `a5169cbf21` (清 `v095` 路径) | `agyneon` | 无 (已闭环) |
| 3 | 提交门系列 (C1-C7 / P2 / P3) | 进行中(复核收尾) | `4c9406d48e` (C1/C2/C5/C6/P2/P3) + `69afbc3371` (C3/C4) + `411c12824b` (C7) + `c3c9373bd3` (`ratchets fail=0`/55s) + `59fac7ee0b` (清洗控制字符/夹具 11l); 工作区仍有 `M tools/{precommit,chain-ready,check-precommit-derive,check-precommit-freeze}.cx` | `agy-flow` + `gatekeeper` | 收口工作区未提交尾项并获 `gatekeeper` 复核通过 (~10-15 分) |
| 4 | `devsvc freeze` 写窗口锁 + `X-Unisacc-Window` trailer + `git merge --no-ff` | 进行中(待窄复核) | `91f5280021` -> `c4ab186f33` -> `d0c95fb523` -> 二轮甲 `a9cc0de6d8` (`(A)(B)(D)` 22/0) -> 二轮乙 `e64cadfdd4` (`(C)`+M3 必红+PLAUSIBLE, `CHECK_DEVSVC_FREEZE pass=23 fail=0 elapsed_s=17`, `CX_LINT findings=0`) | `agyneon` + `gatekeeper` | 等待 `gatekeeper` 完成二轮甲 `(A)(B)(D)` 窄复核 (~5-10 分) |
| 5 | `ci-early` 前置自检与 `osx` 四项探针样本 | 进行中(归 S3b) | `55206270c2` (`tools/ci-early.cx --preflight`, 18/0 6 变异全杀); `v0.0.94` 已推 `origin/v0.0.94` (`2735df3efe`); 路径 A 批复归 S3b | `agyneon` / CI 窗口 | S2 整链 DONE 后于 S3b 触发 `ci-early` 取 `osx` 四项探针实机样本 (~10 分, 不阻断 S0) |
| 6 | `plans/v0.0.94.md` `freeze_at` 与 `done_units` + `l0-posix2` 首跑 | 进行中(按阶段) | `git show v0.0.94:plans/v0.0.94.md`: `planned_units=4`, `done_units=4`, `unplanned_units=0`, `a_files_cap=20`, `freeze_at=TBD`; `l0-posix2` 在 `v0.0.94:tests/batches.tsv:1931` 为 `IMPLEMENTED`、说明为 `REGISTERED_NORUN` | `cc` (S0) / 预跑窗口 (S3) | S0 填 `freeze_at`; S3 候选重建后首跑 `l0-posix2` 决定状态 (不阻断 S0) |
| 7 | `build/version-seq.tsv` 0.0.94 行 | 已完成 | 第 29 行: `29 0.0.94 核心库面二期(...Z1a/Z1b/Z2/Z4/std-surface...; Z3/Z3b/Z5/Z6 由 0.0.96 承接...) 建议` (`7903267e1a`, `verseq-gen --check` 164/0) | `agyneon` / `cc` (S10) | 冻结前无需再改; S10 公开后由 `release-chain mark-version-seq` 转已公开 |
| 8 | `release/` 下 0.0.94 模板与 `evidence` 目录 | 已完成 | `7903267e1a` + `07ee4ff320` + `152f8f58e9`: `release/notes-0.0.94.md`、`release/rollback.txt`、`release/decision.txt`、`release/evidence/0.0.94/prerun-add-ids.tsv` 均在位; 实测 `check-release-prep --part notes`: `CHECK_RELEASE_PREP pass=5 fail=0`, `--part rollback`: `CHECK_RELEASE_PREP pass=23 fail=0` | `agyneon` / `cc` | S3c/S6-S7 填实跑身份与回执 (冻结前已齐) |
| 9 | 冻结前清仓 (共享检出未提交文件 + `cx/` 权限位) | **阻塞** | `git status --short`: 共 **24** 个未提交/未跟踪文件 (含 `A tools/{release-post,check-release-post}.cx`, `M tools/{precommit,chain-ready,check-precommit-*}.cx`, `M plans/slice-audit.md`, `M build/links.tsv`, `?? tools/{cap-probe,check-cap-probe}.cx`, `?? research/2026-11-23/*.md` 等); `cx_mode_dirty_count=0` (05:00:07 276 文件已还原, 但来源未查明) | 各属主窗口 + `cc` | 各窗口提交或移走未提交文件至 0 脏树; 查明 05:00:07 `cx/` 权限位变更来源防复发 (~15-20 分) |
| 10 | 广播与监视工具 (`freeze-broadcast` + `team-pulse --freeze-watch`) | 已完成 | `2e2473c7c0` + `30138a8bd7` + `1a975b654f` (`freeze-broadcast` 15/0, START 正文对齐手册, 移出 moltbaby 三窗); `70221df268` + `67acbe321b` (`team-pulse --freeze-watch` 39/0 变异必红 + `commit-own` `X-Unisacc-Window` trailer) | `csr` | 无 (冻结日直接启用) |
| 11 | `release-post` (S10 公开后自动化) | 进行中 | `git status --short`: `A tools/release-post.cx`, `A tools/check-release-post.cx`, `?? tests/manifests/release-post.tsv` (暂存中待落库) | `csr` | 完成验收并落库 (不阻断 S0, 须在 S10 前落库或从工作区清仓) |
| 12 | 预跑输入 (`prerun-inputs` + `add-ids` 5 批) | 已完成 | `b60dc8b230` + `2d55a45236` + `152f8f58e9` + `418d2370d1` (`prerun-inputs` 冻结 5 项输入、新增批次自动并入、`--normal-jobs 8 --window-lanes 3`、0.0.94 `release/evidence/0.0.94/prerun-add-ids.tsv` 纳入 5 个 0.0.92-0.0.93 漏进批次) | `omp-sonnet` / `cc` | S3 开跑时由 `release-chain prerun-inputs` 一键生成冻结输入 |
| 13 | `origin/main` 推送状态 | **阻塞** | `git rev-list --count origin/main..HEAD` = **49** (远端停在 `6abd4a3847`, 本地领先 49 笔提交) | `cc` | 审核并批量推送 `origin/main` (S0 合并推送前必先对齐, ~5 分) |
| 14 | `tools` 目录与全仓 18 项棘轮 | 已完成 | `sh ./unisacc.com tools/ratchets.cx`: `RATCHETS ok=18 fail=0 elapsed_ms=16692` (`f1f53e2d07` / `998dc44aa6` / `5b7f44388a` / `76a656176c` 已清 `dup-census` 重复) | 全员 | 清仓提交后再复跑一次确认 `18/0` (~17 秒) |
| 15 | 本机 Root 0.0.93 合规备份槽与 S0-S10 运行手册 | 已完成 | `7bb64c2bf8` (`install-ape -- . --rollback` -> `INSTALL_APE ROLLBACK_DRYRUN c561e7e0...`) + `7093632d70` (`POST_ROOT_BACKUP_GATE`); `02cf40e95b` (`s0-merge-checklist.md` + S3 推荐配置 `lanes=3 jobs=8`, 总预算 111 分 / T0..S9 103 分 <= 120 分红线) | `agyneon` | 无 (按手册与 S0 清单执行) |
