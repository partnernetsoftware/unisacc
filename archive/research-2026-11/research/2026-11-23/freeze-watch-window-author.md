# freeze-watch 能否区分窗口作者(只读方案)

日期: 2026-11-23; 范围: 只读核对; 未改 devsvc / team-pulse / commit-own。

问题: 冻结 S0..S9 只允许集成者(cc)写域提交,但各窗 git 作者均为 wanjo。freeze-watch 现靠 owner 与 --lock-author 判提交是否放行,现实里能否区分「集成者的提交」与「别窗的提交」。

## 现状(代码事实)

(1) `tools/team-pulse.cx` `--freeze-watch` 提交扫描(`fw_scan_commits`):
`git log --pretty=format:'COMMIT\t%ct\t%an'` 只取 **作者名 %an**,对域内路径若 `fw_author_ok` 失败则 VIOLATION。
`fw_author_ok`: `%an` 大小写不敏感匹配 `build/freeze.tsv` 第 3 列 owner,或 CLI `--lock-author`。

(2) 脏树扫描(`fw_scan_dirty`):只报 path,`author=-`,**完全无作者信号**。

(3) 夹具(`tools/check-team-pulse.cx` fw-seed):故意把集成者 `git config user.name cc`,违规提交 `-c user.name=agyneon`。夹具假设 **git 作者名 = 窗口名**,与生产「全员 wanjo」不一致。

(4) `build/freeze.tsv` 合同 owner 是窗口名(例历史行/`s0-merge-checklist` 写 `owner=cc`),不是 git 作者 wanjo。

(5) 提交路径身份:
- `commit-own` / `devsvc git commit`: `git commit-tree -m MSG`,**不改** GIT_AUTHOR_*;继承本机 `user.name=wanjo`。
- `tools/cxlib/trailer.cx` **主动剔除** Co-Authored-By / Signed-off-by / Generated with [Claude Code];**没有**写入任何窗口 trailer。
- 近 15 笔 main 提交:作者全 wanjo,消息无 trailers。

(6) 运行时身份(不入 commit 对象):
- `$DEVSVC_OWNER`(推荐窗名) -> `auto_owner()` -> 锁与 **实例 events.tsv** 的 `git.commit owner=...`(落盘在 cache 实例目录,环形/可轮转,**不在 git 对象里**)。
- `$UNISACC_INTEGRATOR`:仅 `precommit` freeze_gate / GATE_WAIVE 放行用,等于 freeze owner 时放行;**不写入提交**。
- 回退:`$CLAUDE_JOB_ID` / `pane-$TMUX_PANE` / `auto-<pid>` —— 非稳定窗身份。
- `commit-own` 合并路径把 DEVSVC_OWNER 哈希进 `update-ref -m "devsvc merge v1 ..."` 的 **reflog 备注**,非常规提交消息,且仅 merge-continue。

## 可靠区分信号清单

| 信号 | 耐久(进 git?) | 能否分窗 | freeze-watch 是否读 | 现实可用性 |
|---|---|---|---|---|
| `%an` / `%ae` | 是 | 否(全 wanjo) | 是 | **不可靠**(与 owner=cc 错位:要么全红要么 `--lock-author wanjo` 全绿) |
| 提交消息 trailer | 可是 | 若强制写入则可 | 否(现不写不读) | **现无** |
| Co-Authored-By 等 | 被 tr_strip 删 | 故意禁用 | 否 | 不可用 |
| DEVSVC_OWNER 事件日志 | 否(实例目录) | 设了则可 | 否 | 审计旁路,非冻结证明;易丢/不全(直跑 commit-own 不经事件) |
| UNISACC_INTEGRATOR | 否(仅进程环境) | 仅门禁豁免 | 否 | 拦写入,不标历史提交 |
| 工作树目录名(window 列) | 否 | 标脏改所在树 | 脏/提交都标 window=目录名 | 只能定位「哪棵树脏」,不能证明提交作者是谁 |
| reflog merge mark | 本地 reflog | 弱 | 否 | 非通用 |

**结论:今天没有「写进提交对象、且 freeze-watch 可读」的可靠窗身份信号。** 现 freeze-watch 在生产全 wanjo 下**不能**区分集成者与别窗提交。

## 方案(先报,未改)

推荐 A(最小耐久、对齐夹具语义):
1. `devsvc git commit` / `commit-own` 在落 `commit-tree` 前,若 `$DEVSVC_OWNER` 为合法窗名(非 `auto-*`/`pane-*`),向消息追加一行 `X-Unisacc-Window: <owner>`(空行分隔的 trailer)。**不得**进入 `tr_strip` 剔除名单。
2. 冻结期集成者提交须 `DEVSVC_OWNER=cc`(与 freeze.tsv owner 同值);别窗保持各自 DEVSVC_OWNER。
3. `freeze-watch` 改读 trailer(或 `%B` 扫 `X-Unisacc-Window:`):有则用它替代 `%an` 做 `fw_author_ok`;无 trailer 的域内提交一律 VIOLATION(冻结期新提交必须带;历史可 `--since-min` 收窄或豁免表)。
4. 脏树:仍无作者;继续按 path 域报 VIOLATION(或另查锁 holder,非本方案必需)。
5. 检查:扩 `check-team-pulse` 夹具为「作者仍 wanjo + trailer=cc 放行 / trailer=agyneon 或缺失则红」;保留现有 user.name 夹具作回归。

备选 B:冻结期把 `GIT_AUTHOR_NAME=$DEVSVC_OWNER`(email 仍 wanjo)。改动面小但与「署名统一 wanjo」政策冲突,且混淆真人作者与窗身份。不推荐。

备选 C:devsvc 旁路账本 `commit_sha -> window`(实例或 `build/` 生成物)。freeze-watch join。缺点:不随 clone/push 走,S10 清理/换机易丢;直跑 git/commit-own 漏记。仅作 A 的补充审计。

与现门禁关系:`precommit`+`UNISACC_INTEGRATOR` 仍是写入时硬拦;trailer 是 **事后只读审计** 与跨窗可验证证据。两者互补,不互相替代。

## 风险与回滚

- 未实施前:freeze-watch 对生产要么误报集成者(owner=cc vs an=wanjo),要么 `--lock-author wanjo` 放行所有窗 —— **不宜单独作 S0..S9 作者门**。
- 实施 A 后回滚:停写 trailer + freeze-watch 回退只读 `%an`;已带 trailer 的提交无害。

未改代码。等裁决再动 devsvc / team-pulse。

## 实施状态(2026-11-23 csr)

方案 A 已批准并落地(本窗写域,未改 tools/devsvc-core.cx):
- `tools/cxlib/trailer.cx`: tr_window_ok/put/get; X-Unisacc-Window 不被 tr_strip 剔除。
- `tools/commit-own.cx`: DEVSVC_OWNER 合法时追加 trailer,缺失不写,已有不覆盖(不伪造)。
- `tools/team-pulse.cx` --freeze-watch: 读消息体 trailer 对齐 freeze owner/--lock-author; 域内无 trailer 或别窗 -> VIOLATION; 域外无 trailer 不报。
- 夹具: check-trailer 15/0; check-team-pulse 含 match CLEAN / other VIOLATION / 无 trailer 域内 VIOLATION / 域外安静 / dirty / INACTIVE。
- `devsvc git commit` 路径由 agyneon 另改。伪造 trailer 不在 git 层防,只防误操作与绕过(裸 git 无 trailer)。
