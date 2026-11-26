# devsvc 提交互斥 v5.x 推迟至 v0.1.4 交接与整改单

- 性质: 私有交接文档(不提交进任何 git 工作树)
- 裁定日期: `2026-11-26` (依据 `1:cc` 冻结前调度与 `cdx`、`gatekeeper` 全轮复核原行)
- 适用范围: `v0.1.4` 互斥补丁续推(`v5.3` 收口落地)

---

## (1) 当前落地状态与资产基线

| 资产 | 状态 | 位置 / 提交 | 摘要 (`sha256` / 说明) |
|---|---|---|---|
| 提交门互斥安全子集 (`commit-gate-safe-subset.patch`) | **已落 `origin/main`** | `772b6e9eb` (已合入 `v0.1.3` `e3df6cdeb`) | `eb47dbf004bc3ed890608771a07c29fac3ef494d67bb2c2747595d4fc1321ec5` (零 `cxtrunc`、非互斥部分全生效) |
| 提交互斥完整补丁 v5.2 (`commit-gate-mutex.patch` / `commit-gate-mutex-v5-only.patch`) | **推迟至 `v0.1.4`** | `~/.local/share/unisacc/research/agy-flow/commit-gate-mutex.patch` (同步 `~/repos/unisacc/.claude/worktrees/v0.1.3/research/agy-flow/`) | `dede3ec00659f220927e9a4ce3b8521b4233a3ab2ae713e27b68aaaae573a034` (基于 `origin/main` `70756430eb` `git apply --check` 零冲突; `v0.1.4` 入库前需剥离 5 个登记表文件只留 3 个源码/文档文件, 见第 (4) 节) |

---

## (2) `a1`/`a2` 冷跑实测数据与根因剖析

### 2.1 `cdx` 独立新树冷跑复现 vs 共享缓存热跑对比

| 测试场景 | 树基线与环境 | `--cases a1` | `--cases a2` | `--cases b1` / `--cases b2` | 结论 |
|---|---|---|---|---|---|
| `cdx` 新私有索引树冷跑 #1 | `70756430e`, 不设 `DVGIT_EXE` / `DEVSVC_NATIVE_DIR` | **`52.907s`** (17 项 PASS, 超 30s 贴 55s 限) | 独立首跑 **`~52.9s`**; 随 `a1` 后 **`8.4s`** | 全绿 | **阻断落 `main`**: 距 55s 窗仅剩 2s, 且超 `register.cx` 新分片 33000ms 冷跑硬门 |
| `cdx` 另一新树独立冷跑 #2 | `c500c0db8`, 不设 `DVGIT_EXE` / `DEVSVC_NATIVE_DIR` | **`53.093s`** (17 项 PASS) | - | 全绿 | 跨树稳定复现, 确认非偶发负载抖动 |
| `neon` 设 `DVGIT_EXE` + 共享 `DEVSVC_NATIVE_DIR` 热跑 | `70756430e` 私有树 | **`10.182s`** (17 项 PASS) | **`9.205s`** (7 项 PASS; 首次冷编 `22.622s`) | `b1=8.028s` / `b2=8.426s` / `b=8.536s` | 证明全部用例逻辑本身仅耗时 `8-10s`, 43s 全为首次冷编开销 |

### 2.2 43 秒冷编开销的精确代码根因

在全新 `git archive + git init` 树、不设任何外部环境变量下跑 `sh ./unisacc.com tools/check-devsvc-git.cx -- --cases a1`, 发生三重重复冷编:
1. **`devsvc-core.cx` (~20s, 路径绑死新 `.git` 哈希)**:
   - `tools/check-devsvc-git.cx` 直接跑单分片时调 `ensure_dvgit_exe(0)` (`rdir = 0`), 当 `tools/devsvc-core.cx` 在工作区有补丁改动 (`git diff --quiet HEAD` 非零) 时跳过预编, 回退每次经 `tools/devsvc.cx` 启动。
   - `check-devsvc-git.cx` 将 `DV_PIN_DIR` 设为 `$HOME/.cache/unisacc/devsvc/pinned/repos/<sha256(root/.git)>`, 新私有树 `.git` 路径全新导致 `pinned` 必然 miss, 现场冷编 `tools/devsvc-core.cx` 约 20s。
2. **`tools/commit-own.cx` + `tools/gitsync-run.cx` 双冷编 (~22s, 缓存未默认化到独立内容身份目录)**:
   - `a1` 首条断言 `commit-push-rc0` 带 `--push`, 经 `git_push_owned_ex` (`tools/devsvc-core.cx:4137-4138`) 调 `native_get(work, "tools/commit-own.cx")` 与 `native_get(work, "tools/gitsync-run.cx")`。
   - `native_get` 本身已采用不含 `root` 的内容身份键 `v3|-|tool|phex|dhex` (`tools/devsvc-core.cx:533`), 但 `native_dirs` (`tools/devsvc-core.cx:424-428`) 在存在 `UNISACC_DEVSVC_DIR` 且无 `DEVSVC_NATIVE_DIR` 时落回 `$UNISACC_DEVSVC_DIR/native`, 而夹具每次启动先 `rm -rf inst` 把它清空。
   - v5.2 虽在 `check-devsvc-git.cx` 补了 `setenv("DEVSVC_NATIVE_DIR", "$HOME/.cache/unisacc/devsvc/native", 1)`, 但仍未解决全新空缓存/新树上与 `devsvc-core.cx` 串行三重冷编 (`20s + 11s + 11s + 10s = 52s`) 的问题。
3. **`register.cx` 硬门槛**: `tools/register.cx:7` 要求新分片在全新私有 Git 布局下冷跑必须在 `33000 ms` 内通过。

---

## (3) `v0.1.4` 缓存默认化与批外预热整改方案 (`v5.3` 要点)

原则: **只缓存编译产物(按完整源码闭包 + 产品二进制 SHA-256 寻址), 绝不缓存任何测试结论; 不依赖调用方传任何环境变量。**

1. **夹具内默认内容身份缓存目录(零外部环境变量依赖)**:
   - 在 `tools/check-devsvc-git.cx` `main()` 剥离继承 `DEVSVC_*` 后、调 `ensure_dvgit_exe` 前, 自动建立稳定目录 `CDIR="${TMPDIR:-/tmp}/unisacc-dvgit-native-cache"` (以及子目录 `$CDIR/native`, 权限 `0700`)。
   - 只要非负控 (`!getenv("DVGIT_CORE_FROM") && !getenv("DVGIT_PUSH_FROM")`), 夹具默认调用 `setenv("DEVSVC_NATIVE_DIR", "$CDIR/native", 0)` 并在 `envv[]` 显式透传, 使所有子进程(含 `devsvc` 服务端与 `run_flow_shards` 子分片)共享同一份按内容身份寻址的原生构建缓存。
2. **`devsvc-<src_sha256>.bin` 按内容身份缓存(跨新树零重编)**:
   - 改造 `ensure_dvgit_exe()`: 当未设 `DVGIT_EXE` 与 `DVGIT_CORE_FROM` 时, 对 `sh ./unisacc.com -cx-plan tools/devsvc-core.cx` 吐出的完整闭包文件(含 `tools/devsvc-core.cx`, `tools/cxlib/*.cx`) + `realpath(unisacc.com)` 内容哈希 + `tools/devsvc.cx` + `tools/devsvc-repo.tsv` 计算总 SHA-256 (前 24 hex 为 `key`), 缓存路径固定为 `$CDIR/devsvc-<key>.bin`。
   - 命中则 0ms 直接 `setenv("DVGIT_EXE", bin, 1)`; 未命中则编译到 `$CDIR/devsvc-<key>.tmp.<pid>` 后 `rename()` 原子落盘, 源码一改哈希即变(变异实验绝对隔离)。
3. **空缓存首跑三路并行预热(或批外材料注入)**:
   - 方案 3A (夹具预备相并行): 当 `$CDIR/devsvc-<key>.bin` 或 `$CDIR/native` 尚未就绪时, 用 3 路并行子进程同时编译 `devsvc-core.cx`、预热 `commit-own.cx` 与 `gitsync-run.cx` (`max(15s, 9s, 8s) ~ 16s`, 再加 `a1` 用例 `6s` = **`~22s < 30s`**)。
   - 方案 3B (批外预备材料, 最彻底): 在 `tests/batches.tsv` 为 `devsvc-git-a1`/`a2`/`b1`/`b2` 接入 `devsvc-core-tool:path`、`commit-own-tool:path` 等批外预构材料(参考 `devsvc-git-unstage` 第 8 列 `devsvc-core-tool:path` 与 `v0.1.2` `C8` 为 `real-commit` 接入 `devsvc-tool` + `precommit-tool` 使耗时降至 `9.4s` 的做法), 把全部冷编译移出 55s 测试窗。
4. **`v0.1.4` 验收硬门(三连全新树空环境冷跑)**:
   ```bash
   # 在全新 git archive + git init 树里、清空缓存目录且不设任何环境变量连跑 3 次:
   rm -rf /tmp/unisacc-dvgit-native-cache
   for i in 1 2 3; do
     env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp \
       sh ./unisacc.com tools/check-devsvc-git.cx -- --cases a1
     env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp \
       sh ./unisacc.com tools/check-devsvc-git.cx -- --cases a2
   done
   # 判据: 首跑(空缓存)与后两次(热缓存)均 < 30000 ms, CHECK_DEVSVC_GIT failed=0
   ```

---

## (4) 补丁剥离登记表 hunk 与可重放登记命令序列

### 4.1 补丁边界(与 `S2` 及其他并发登记 100% 零冲突)

`v0.1.4` 重新导出 `commit-gate-mutex.patch` 时, **仅包含以下 3 个文件**, 不含 `tests/batches.tsv`、`tests/coverage.tsv`、`build/links.tsv` 与 `tests/manifests/devsvc-git-a*.tsv`:
- `plans/devsvc.md`
- `tools/devsvc-core.cx`
- `tools/check-devsvc-git.cx`

(若从现有 v5.2 补丁 `dede3ec0...` 仅取这 3 个文件起手, 可直接执行:
`git apply --include='plans/devsvc.md' --include='tools/devsvc-core.cx' --include='tools/check-devsvc-git.cx' ~/.local/share/unisacc/research/agy-flow/commit-gate-mutex.patch`)

### 4.2 落库后可重放登记命令序列

在 `3` 个源码/文档文件应用并通过单跑验证后, 按当前 `origin/main` 实测过的两种等价正规路径之一完成 `devsvc-git-a1` 与 `devsvc-git-a2` 登记(推荐路径 A 走 `register.cx` 一次完成写入 + 冷跑验收 + 校验, 或路径 B 走 `manifest -> reg-manifest --write -> derive --write`):

#### 路径 A: `tools/register.cx` (推荐, 自动持 `.register.lock`、追加三表并执行 33s 新布局冷跑验收)
```bash
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/register.cx -- \
  devsvc-git-a1 tools/check-devsvc-git.cx CHECK_DEVSVC_GIT \
  "devsvc git 分组 a1:commit --whole --push、git rm 删除提交、失败提交无残留锁、--cwd 版本工作树提交与推送(check-devsvc-git --cases a1)" \
  "tools/devsvc-core.cx tools/check-devsvc-git.cx" interfaces product --args "--cases;a1"

CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/register.cx -- \
  devsvc-git-a2 tools/check-devsvc-git.cx CHECK_DEVSVC_GIT \
  "devsvc git 分组 a2:--derive 两段身份校验、超时 reason=timeout 映射、COMMIT_LANDED 后超时自动对齐索引恢复回执(check-devsvc-git --cases a2)" \
  "tools/devsvc-core.cx tools/check-devsvc-git.cx" interfaces product --args "--cases;a2"

CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/derive.cx -- --write --only links,table-index
```

#### 路径 B: manifest + `reg-manifest.cx` + `derive.cx` (已在 `70756430eb` 私有树实跑通过: `REG_MANIFEST manifests=819 bad=0 hard=0`, `AFFECTED_REGISTRY_CHECK batches=2029`)
```bash
cat > tests/manifests/devsvc-git-a1.tsv <<'EOF'
batch	devsvc-git-a1	IMPLEMENTED	interfaces	tools/check-devsvc-git.cx	SOURCE	lnx/x86_64	55	-	tools/check-devsvc-git.cx	--cases;a1	interfaces.devsvc-git-a1	rc0:CHECK_DEVSVC_GIT	product	commit
coverage	interfaces.devsvc-git-a1	interfaces	IMPLEMENTED_SEPARATE_BATCH	devsvc git 分组 a1:commit --whole --push、git rm 删除提交、失败提交无残留锁、--cwd 版本工作树提交与推送(check-devsvc-git --cases a1)	tools/devsvc-core.cx tools/check-devsvc-git.cx
packages	batch	devsvc-git-a1	all	-	-	-	-	-	-
EOF

cat > tests/manifests/devsvc-git-a2.tsv <<'EOF'
batch	devsvc-git-a2	IMPLEMENTED	interfaces	tools/check-devsvc-git.cx	SOURCE	lnx/x86_64	55	-	tools/check-devsvc-git.cx	--cases;a2	interfaces.devsvc-git-a2	rc0:CHECK_DEVSVC_GIT	product	commit
coverage	interfaces.devsvc-git-a2	interfaces	IMPLEMENTED_SEPARATE_BATCH	devsvc git 分组 a2:--derive 两段身份校验、超时 reason=timeout 映射、COMMIT_LANDED 后超时自动对齐索引恢复回执(check-devsvc-git --cases a2)	tools/devsvc-core.cx tools/check-devsvc-git.cx
packages	batch	devsvc-git-a2	all	-	-	-	-	-	-
EOF

CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/reg-manifest.cx -- --write
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/derive.cx -- --write --only links,table-index
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/reg-manifest.cx -- --check
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/affected.cx -- --registry-check
CC=/usr/bin/cc PATH="/usr/bin:/bin:$HOME/.local/bin" sh ./unisacc.com tools/ratchets.cx --
```

---

## (5) `cdx` 与 `gatekeeper` 历轮全部阻断清单对账表

| 编号 | 提出方 / 轮次 | 阻断项描述 | 当前状态 | 闭合实现与测试断言 / 变异实测原行 |
|---|---|---|---|---|
| `B1` / `B2` | gatekeeper v4 | 反序死锁环(`commit-wt -> need` 与手持写域锁 / `flow lock=registry` 互锁)与等写域锁期间阻断无关路径提交 | **已闭合 (v5)** | 重构为单向全序 `写域锁(need) -> commit-wt-<16hex>`, 释放严格逆序; `N1`, `C4`, `C5` 全绿 |
| `C1` | gatekeeper v4 | 非主工作树锁前缀不可污染含工作树哈希的 `commit-wt-` | **已闭合 (v5)** | `request_until` 放行 `commit-wt-` 不加 `lkpfx` |
| `C2` | gatekeeper v4 | `--derive` 两段提交共用初始锁截止导致第二段假超时 | **已闭合 (v5)** | `tool_locked_cwd_wt` 按段重算 `stage_dl`; `c2-derive-stage2-recomputes-lock-deadline` PASS + 变异必红 |
| `C3` | gatekeeper v4 | `DEVSVC_COMMIT_LOCK_WAIT_MS` 缺省与上限钳位 50000ms 及超时具名 `lock-held` | **已闭合 (v5)** | `commit_lock_wait_ms()` 缺省/非正/超限钳位 50000; `c3-*` 5 条断言 PASS + 变异必红 |
| `C4` | gatekeeper v4 | 手持 `registry` 遇他属主短持 `commit-wt` 排队 `rc=0` 且不代放 `registry` | **已闭合 (v5/v5.1)** | `c4-held-registry-queues-for-commit-wt-rc0` + `c4-manual-registry-preserved-and-wt-released` PASS |
| `C5` | gatekeeper v4 | `flow` 节点带 `lock=registry` 内部调 `git commit` 无死锁无泄漏 | **已闭合 (v5/v5.1)** | `c5-flow-lock-node-commit-rc0` + `c5-flow-commit-no-leaked-locks` PASS |
| `C6` | gatekeeper v4 | 释放 `commit-wt` 失败须报 `lock-not-released`, 且生产代码禁测试后门环境变量 | **已闭合 (v5)** | 删净全部 `DEVSVC_TEST_*`, 夹具内 `mock_deny_release_srv` 验 `c6-mock-deny-release-reports-lock-not-released` PASS + 变异必红 |
| `C7` | gatekeeper v4 | 变异守护不可假绿 | **已闭合 (v5/v5.1/v5.2)** | `mC2`/`mC3`/`mC6`/`M2`/`M2-RPC`/`M4`/`m1` 全部单点具名 `failed=1` 必红 |
| `C8` / `cxtrunc` | `neon` 自查 v5 | 裸 `snprintf` 触发 `RATCHET cxtrunc FAIL` | **已闭合 (v5)** | 全部改 `F()` / `cx_fmt()`, `RATCHETS ok=19 fail=0` (`cxtrunc 0 above baseline`) |
| `C9` / `b` 分片 | gatekeeper v4 | `--cases b` 超时风险 | **已闭合 (v5)** | 拆为 `b1` (`8.0s`) + `b2` (`8.4s`), `--cases b` 并行 `8.5s` |
| `R1` | gatekeeper v5 | 属主比较语义偏移(`DEVSVC_OWNER=W1` 未按基线 `auto_owner()` 归一化小写) | **已闭合 (v5.1)** | `tool_locked_cwd_wt` 恢复 `auto_owner()`, 服务端保持 `strcmp`; `r1-owner-lower-lock-upper-env-rc0` + `r1-owner-upper-lock-upper-env-held` 双向 PASS |
| `R2` | gatekeeper v5 | 等锁超时 `resp` 被 `ERR deadline-exceeded owner=deadline` 覆写丢失真实持锁人 | **已闭合 (v5.1)** | `auto_lock_until` 局部 `held_resp[256]` 保留最近 `ERR held`; 断言 `owner=wt-blocker` 且 `!has("owner=deadline")` PASS |
| `M2` | gatekeeper v5 | 取 `commit-wt` 失败须主动回滚释放已新取的 `need`(不能靠死进程 `lock.reap`) | **已闭合 (v5.1)** | `events` 验 `lock.acquire` + `lock.release` 且无 `lock.reap`; 变异 `MUT_M2` 单点 `FAIL devsvc-git.m2-wt-fail-rolls-back-acquired-need-lock` (`M2_RC=1`) |
| `M2-RPC` | `cdx` v5.1 | `M2` 回滚释放 RPC 返回错误未检查 | **已闭合 (v5.2)** | 新增 `lock_release_checked()`, 失败具名 `REFUSE rollback release ...` 与 `reason=rollback-release-failed lock= wt_lock=`; `mock_rollback_deny_srv` 验 `m2-wt-fail-rollback-rpc-error-named` PASS, 变异 `MUT_M2_RPC` 单点 `failed=1` (`M2RPC_RC=1`) |
| `M4` | gatekeeper v5 | `commit_wt_lock_name` 去掉 `realpath` 须必红 | **已闭合 (v5.1)** | `cmd_git` 传未规范 `raw_cwd` 经 `commit_wt_lock_name` 做 `realpath`; 符号链接 `--cwd sym_fx` 用例 `m4-symlink-cwd-shares-canonical-wt-lock` PASS, 变异 `MUT_M4` 单点 `failed=1` (`M4_RC=1`) |
| `m1` | gatekeeper v5 | 回退 v4 锁序在短截止下须具名 FAIL 不靠 55s 超时 | **已闭合 (v5.1)** | `N1` 设 `winA=3500ms` / `winC=1200ms`; 变异 `MUT_m1` 在 `1203ms` 立即具名 `FAIL devsvc-git.n1-winc-unrelated-commit-not-blocked-by-registry-waiter` (`m1_RC=1`) |
| `N1-sync` | `cdx` v5.1 | `N1` 固定 `400ms` 延时未观测 `winA` 已进入等待 | **已闭合 (v5.2)** | `auto_lock_until` 首次 `ERR held` 打 `LOCK_WAIT` + `JOB` 行补 `fflush(stdout)`, 夹具 20ms 轮询 `<run_dir>/out` 观测 `LOCK_WAIT lock=registry` (`n1-wina-observed-waiting-for-registry` 20ms 实测 PASS) |
| `a-split` | `cdx` v5.1 | `--cases a` 越 55s 窗须拆为 `a1`/`a2`, 断言零遗漏 | **已闭合 (v5.2)** | 拆为 `a1` (17 断言) + `a2` (7 断言) = 24 断言, 分界 `wt-fixture-remove` / `derive-phase-fixture`, `diff_count=0`, 预热后 `a1=10.2s` / `a2=9.2s` / `a=10.9s` |
| **`a1`/`a2` 无变量新树冷跑** | **`cdx` v5.2** | **全新树不设环境变量首跑 `a1`/`a2` 仍 ~52.9s (三重冷编)** | **唯一未闭项 -> 交 `v0.1.4`** | 按第 (3) 节落缓存默认化 (`$TMPDIR/unisacc-dvgit-native-cache` + `devsvc-<sha>.bin`) + 并行预热或批外预构材料, 三连空环境冷跑 `<30s` 即完全闭环 |
