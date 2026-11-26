# unisacc v0.1.3 起链与发布起跑卡(定稿版)

- 性质: 私有操作起跑卡(不提交进任何 git 工作树)
- 对齐基线(`2026-11-26` 最新状态):
  - `origin/main`: `70756430eb4789fa0b4be3cdf71fda95aeef0c7c` (已含 `batch-cap-4096-v4` `fd3994e9a`、`commit-gate-safe-subset` `772b6e9eb`、`tapepack` `49592cc39`、`tape-speed` `51023e8a4`、`tape M1` 证据 `70756430e`)
  - `v0.1.3` 工作树(`~/repos/unisacc/.claude/worktrees/v0.1.3`): `e3df6cdeb` (已合并 `origin/main` `70756430e`, 含 `C3` 与 `S2 v3` `8553aa45a`)
  - 范围裁定: `devsvc` 提交互斥 v5.x 整体推迟至 `v0.1.4`(交接见 `research/agy-flow/mutex-v014-handoff.md`, 安全子集 `772b6e9eb` 已在 `main`); `tape` 15 倍正式速度门未达, 仅带入 M1 载体与轻量合同并如实写入发布说明
- 对照依据: `skills/unisacc-orch-release-train/SKILL.md`、`tools/{chain-ready,bg-candidate,prerun-inputs,prerun-drive,dvqualify,release-run,release-exec,release-chain,release-post}.cx` 现行源码合同、`v0.1.1`/`v0.1.2` 两趟完整实跑原行

---

## (1) 冻结后 `main` 前进禁令执行办法

目的: 守住 `chain-ready` 门 `22-premerge-union` / `23-freeze-armed` 与 `release-exec.cx:249-277` 的 `N3 MAIN_ANCESTOR` 门(`origin/main` 必须是 `HEAD` 的祖先, 否则 `Step 1 X` / `Step 4 Y1` / `Step 7 Y2` 均在推送前报 `REFUSE RELEASE_EXEC_GATE_MAIN_ADVANCED` 停机), 避免 `v0.1.2` 期间 `main` 推进引发的链后合并、`/home/` 污染与额外返工。

### 1.1 四步执行规程(冻结前清空 -> 全员禁推 -> 冻结落锁 -> 解冻恢复)

1. **冻结前统一清空 `main` 积压(`1:cc` 唯一执行)**:
   - 在切 `C0a` 编号提交之前, 由 `1:cc` 统一收口所有允许进 `main` 的最后积压并推送 `origin/main`;
   - 确认 `v0.1.3` 分支已完成对最终 `origin/main` 的合入(`git -C "$WT" merge-base --is-ancestor origin/main HEAD` 退出 0), 且工作区零未提交文件。
2. **全员发布 `main` 前进禁令通知**:
   - 由 `1:cc` 向全部活跃窗口(含 `devsvc` 渠道与所有协作代理)发布明确通告:
     - **禁令窗口**: 自 `v0.1.3` 编号冻结(`C0a`/`C0b`)起, 直至 `v0.1.3` 公开读回完成、`Y2` 推送双远端、`release-post` 与解冻提交推上 `origin/main` 为止;
     - **禁令范围**: 禁止向 `main` / `origin/main` 做任何提交或推送(**包括纯文档、`plans/`、研究页、工具线补丁**); 所有窗口的文档与后续改进保留在私有目录或各自暂存分支/补丁文件, 待解冻通知后再统一合入;
     - **唯一写手**: 冻结窗口内只有发布集成者(`cc`)对 `v0.1.3` 版本工作树和远端 `origin/v0.1.3`、`origin/main` 拥有写权限。
3. **落锁校验(`C0a` + `C0b` 后即刻核对)**:
   - 核对 `origin/main` 与 `v0.1.3` 共同祖先即当前 `origin/main`:
     ```bash
     WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
     git -C "$WT" fetch --quiet origin
     git -C "$WT" merge-base --is-ancestor origin/main HEAD && echo "MAIN_ANCESTOR=OK"
     git -C "$WT" rev-parse origin/main HEAD
     ```
4. **破禁例外处置(仅当 `origin/main` 被意外推进时)**:
   - 严禁强推或回退 `origin/main`。
   - 先核查多出的 `origin/main` 提交是否触及产品源; 合并 `origin/main` 入 `v0.1.3` 后必须:
     (a) 用 `"$CHAIN/ctx/identity-helper" "$WT"` 实算 `SOURCES_SID`, 确认与已构建 `APE` 的 `SOURCES_SID` 逐字节一致(若改变则必须重跑整链, 严禁复用旧 `APE`);
     (b) 立即重跑 `sh ./unisacc.com tools/ratchets.cx --` 确认 `RATCHETS ok=19 fail=0`(拦截上游研究页夹带 `/home/` 路径导致 `no-home-path` 失败);
     (c) 在后续 `release-exec.cx` 传 `--post-chain-merge "merge origin/main <SHA>; SOURCES_SID unchanged"` 写入 `INDEX.tsv` 留痕(`exec-1-post-chain-merge WAIVED`)。

---

## (2) 编号与冻结提交(`C0a` + `C0b` 手工两笔隔离提交)

结论: **禁用 `0003-freeze-cut.patch`**, 改用手工两笔隔离提交 `C0a` + `C0b`。
原因(一手代码实证): `0003-freeze-cut.patch` 在 `origin/main` 与 `v0.1.3` 上均 `apply --check FAIL`, 且 `fc_update_candidate_version()` (`tools/freeze-cut.cx:757-797`) 只改 `release/candidate.tsv` 第 4 列 `version`, 漏改第 2 列 `tag`、第 6 列 `workflow_ref` 与第 7 列 `ci_run_id`——与 `v0.1.2` `254c8a347a` 触发 CI 误跳过的事故完全同源。

### 2.1 `C0a`: 编号、候选表四列、版本计划与派生同步(一笔做全)

在 `C0b` 之前, 必须先完成全部产品/测试/派生物同步(见 `SKILL.md` 第 (7) 节与 2.3 节防呆命令), 然后在 `C0a` 一次改齐并提交:
1. `build/version-seq.tsv`: 追加第 `68` 槽 `0.1.3` 行(`进行中`, 指向 `plans/v0.1.3.md`);
2. `plans/v0.1.3.md`: 写齐 `planned_units`、`a_class`、`a_files_cap` 与 R5 范围声明(明示 `C3`、`S2`、`tape M1` 载体与 15x 未达门、互斥 v5.x 推 `v0.1.4`);
3. `release/candidate.tsv`: **必须同时更新 4 列**(核对第 2/4/6/7 列):
   - 第 2 列 `tag` = `v0.1.3`
   - 第 4 列 `version` = `0.1.3`
   - 第 6 列 `workflow_ref` = `v0.1.3`
   - 第 7 列 `ci_run_id` = `0` (长度 `< 4`, 保证 Step 3 新触发 CI 而非跳过)
4. 同步派生表 `build/table-index.tsv` (`tools/derive.cx -- --write --only table-index`) 并核 `tools/verseq-gen.cx -- --check` `rc=0`。

### 2.2 `C0b`: 独立冻结提交(仅改 `build/freeze.tsv` 一个文件)

在 `C0a` 之后单独提交 `C0b`, 追加 `0.1.3-FREEZE` 行, `base` 列填 `C0a` 的 40 位完整 SHA, 范围覆盖 `src cx ext exec include seed kernel tests release`。`C0b` 之后不得再有 `tlib`/编号提交(否则门 `23-freeze-armed` 拒)。

### 2.3 冻结前秒级防呆总表(清零类别 A 红停)

在 `C0b` 之前与起链之前各跑一遍, 任一非零即停:
```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
(cd "$WT" && env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp DEVSVC_OWNER=cc sh -c '
  sh ./unisacc.com tools/gen-all-check.cx -- --check &&
  sh ./unisacc.com tools/check-gen-all-check.cx -- &&
  sh ./unisacc.com tests/daily-compile-smoke.cx -- --check &&
  sh ./unisacc.com tools/lane-inputs-gen.cx -- --check &&
  sh ./unisacc.com tools/verseq-gen.cx -- --check &&
  sh ./unisacc.com tools/reg-manifest.cx -- --check &&
  sh ./unisacc.com tools/affected.cx -- --registry-check &&
  sh ./unisacc.com tools/derive.cx -- --check &&
  sh ./unisacc.com tools/ratchets.cx --
')
```

---

## (3) 起链起跑卡(`env -i`、常规文件副本校验、`chain-ready` 全门、`bg-candidate`)

### 3.1 起链硬前置核对(对照 `SKILL.md` 第 (2) 节)

```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
MAIN="$HOME/repos/unisacc"
IH="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"

# 1. 根产品与构建元数据必须是常规文件副本, 严禁符号链接! (门 12/12p)
if [ -L "$WT/unisacc.com" ]; then rm -f "$WT/unisacc.com"; cp -p "$MAIN/unisacc.com" "$WT/unisacc.com"; fi
if [ -L "$WT/unisacc.com.build.json" ] || [ ! -f "$WT/unisacc.com.build.json" ]; then
  rm -f "$WT/unisacc.com.build.json"; cp -p "$MAIN/unisacc.com.build.json" "$WT/unisacc.com.build.json"
fi
test -f "$WT/unisacc.com" && test ! -L "$WT/unisacc.com" && echo "ROOT_PRODUCT_REGULAR=OK"
test -f "$WT/unisacc.com.build.json" && test ! -L "$WT/unisacc.com.build.json" && echo "ROOT_BUILDJSON_REGULAR=OK"

# 2. 核工作区干净、磁盘余量 (>=20G 建议)、无在途链或预跑进程
git -C "$WT" status --short
df -h / /tmp
pgrep -fa "bg-candidate|candidate-chain|prerun-drive|dvqualify" || echo "NO_INFLIGHT=OK"

# 3. 确认身份帮手存在(不存在时用 chain-ready --prepare 生成到 ~/.cache/unisacc/ident-helper/identity-helper-real5)
test -x "$IH" && echo "IDENT_HELPER=OK"
```

### 3.2 `chain-ready` 全 23 门实跑与 `bg-candidate` 唯一写手起链

全程强制 `env -i`, 固定 `PATH="/usr/bin:/bin:$HOME/.local/bin"`(彻底隔绝 `~/repos/moltbaby/bin/cc` 劫持及一切残留 `UNISACC_*` 变量):

```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
REV="$(git -C "$WT" rev-parse HEAD)"     # 必须是冻结提交 C0b 的 40 位 SHA
IH="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"

# (a) 全 23 门(不加 --fast), 末行必须 READY=yes 且 rc=0
(cd "$WT" && env -i HOME="$HOME" USER="${USER:-devali}" LOGNAME="${USER:-devali}" TMPDIR=/tmp \
  PATH="/usr/bin:/bin:$HOME/.local/bin" DEVSVC_OWNER=cc UNISACC_IDENT_HELPER="$IH" \
  sh ./unisacc.com tools/chain-ready.cx -- --repo "$WT" --root "$WT" --version 0.1.3)

# (b) 唯一写手后台起链(起后即把 job_id / PID / PGID / REV / T1 记入 prd_mempalace.md)
(cd "$WT" && env -i HOME="$HOME" USER="${USER:-devali}" LOGNAME="${USER:-devali}" TMPDIR=/tmp \
  PATH="/usr/bin:/bin:$HOME/.local/bin" DEVSVC_OWNER=cc UNISACC_IDENT_HELPER="$IH" \
  sh ./unisacc.com tools/bg-candidate.cx -- start --rev "$REV" --root "$WT" --deadline-s 9000)
```

### 3.3 整链终态机核(预计 ~27-35 min; 零手抄)

```bash
CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$REV"
cat "$CHAIN/owner.tsv"                                              # 核 T1, PID, rev==C0b
cat "$CHAIN/exit.tsv"                                               # 判据: rc=0 why=exited
cat "$CHAIN/ape-final/ape-receipt.txt"                              # 核 ape_sha256 与 version=unisacc 0.1.3
"$CHAIN/ctx/identity-helper" "$WT"                                  # 核实算 SOURCES_SID (64 hex)
(cd "$WT" && env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp \
  sh ./unisacc.com tools/fixedpoint-evidence.cx -- --chain "$CHAIN" --check)  # 判据: 6/6 SAME
```

---

## (4) 严格预跑起跑卡(绑定 `C0b` 头、~105 min 并发与窗口分配、红停决策表)

### 4.1 预跑输入冻结、布局预热与计划规划

说明: 预跑全程绑定 `C0b` 头 (`REV`), 目标目录 `$PDIR` 必为持久绝对路径(`~/.cache/unisacc/prerun/v0.1.3-<rev10>`, 严禁 `/tmp`)。并发配置推荐 `--window-lanes 2 --max-jobs 4` (双窗各 4 路 = 8 并发, 稳控负载防贴限批抖动, 整跑约 105 分钟; 若本机空闲核充裕可取 `--max-jobs 8`, 外层 `timeout` 留足 `7200s` = 120 分钟)。

```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
REV="$(git -C "$WT" rev-parse HEAD)"                                # 冻结头 C0b
REV10="${REV:0:10}"
PREV_REV="115e5720ded24809c6daadbc9aaa7c72afe08d90"                 # v0.1.2 冻结提交 C9b
CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$REV"
APE="$CHAIN/ape-final/unisacc-next.com"
PDIR="$HOME/.cache/unisacc/prerun/v0.1.3-$REV10"

# 1. 冻结五项预跑输入(--prev-rev 必传, 自动吸纳 0.1.2 -> 0.1.3 新增 IMPLEMENTED 批)
(cd "$WT" && env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp DEVSVC_OWNER=cc \
  sh ./unisacc.com tools/prerun-inputs.cx -- \
    --rev "$REV" \
    --version 0.1.3 \
    --prev 0.1.2 \
    --prev-rev "$PREV_REV" \
    --ape "$APE" \
    --dir "$PDIR" \
    --repo "$WT" \
    --prev-windows "$WT/release/evidence/0.1.2/prerun-windows.tsv" \
    --priority-src "$WT/release/evidence/0.1.2/strict.tsv" \
    --lanes 2 --jobs 4)

# 2. 规划窗口(原窗口号 W0000..W0NNN; 补跑保留 W9901..W9999)
(cd "$WT" && env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp DEVSVC_OWNER=cc \
  sh ./unisacc.com tools/prerun-drive.cx -- plan \
    --ids-file "$PDIR/ids.txt" \
    --out "$PDIR" \
    --times-file "$PDIR/times.tsv" \
    --priority-file "$PDIR/priority.tsv" \
    --normal-jobs 4 \
    --window-lanes 2)

# 3. 预跑前绝对路径预热 run-batch.cx 与 check-tsv-lint.cx(防首窗冷编顶 58s 上限)
sh "$WT/unisacc.com" "$WT/tests/run-batch.cx" -- --help >/dev/null 2>&1 || true
sh "$WT/unisacc.com" "-I$WT/include" "$WT/tools/check-tsv-lint.cx" -- --help >/dev/null 2>&1 || true
```

### 4.2 正式起跑命令(带外层 `timeout -k 10s 7200s`, 整跑约 105 分钟)

起跑前登记唯一写手、`PID/PGID`、`$PDIR`、`REV`、`APE_SHA256`、`T0 + 7200s` 截止; 预跑期间全员零重编译。

```bash
# 首跑不带 --resume; 同 REV 授权续跑才追加 --resume; --min-free 单位为字节 (2 GiB)
(cd "$WT" && timeout -k 10s 7200s env -i \
  HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp \
  DEVSVC_OWNER=cc UNISACC_BOOTER_CHAIN_OUT="$CHAIN" \
  sh ./unisacc.com tools/prerun-drive.cx -- run \
    --root "$WT" \
    --candidate "$APE" \
    --out "$PDIR" \
    --max-jobs 4 \
    --window-lanes 2 \
    --keep-going \
    --min-free 2147483648)
```

说明: `--rebase-rev` 当前不可用(`tools/prerun-drive.cx` 无实现且研究补丁为空文件); 严禁在修订 `REV` 后原位续跑旧 `$PDIR`, 换 `REV` 必须先 `mv` 备份旧目录再开新目录。

### 4.3 现用红停与批红分类决策表

遇任何红停或批红, 先读发码源码行并手跑复现, 再按表定性决策:

| 分类 | 典型症状与定因判据 | `SOURCES_SID` 变化 | 重起整链 | 唯一标准动作 |
|---|---|---|---|---|
| 类别 A: 陈旧派生表 / 分片表 / 常量计数漂移 | 仅测试或派生表滞后(`lane-inputs`, `daily-compile-smoke`, `gen-all-check`, `table-index`), 实算 `SOURCES_SID` 不变 | 不变 | 否 | 早期(<20% 窗): 停预跑 -> `mv` 备份 `$PDIR` -> 修表提交 -> 新 `$PDIR` 重跑; 后期单批: 仅获 `1:cc` 书面授权后写 `MANUAL_DISPATCH` 续跑, 链后单笔修表 + `W99NN` 补跑 + 发布带 `--post-chain-merge` |
| 类别 B: 首跑冷态 / 缺材料 / 并发贴限 (`rc=124` 或 `transport=-1`) | 单跑热态 `<25s` 通过; 或因缺 `*-tool` 预构材料/绝对路径 `run-cache` 未预热顶 58s 窗 | 不变 | 否 | 缺材料则补入材料依赖; 冷态贴限则严格按 4.4 节补跑 SOP: `cp -a` 备份 `$PDIR` -> 同布局绝对路径预热丢弃至 `/tmp` 核 `<25s` -> 新递增窗号 `W99NN` 正式取证(禁止 `rm` 任何 `strict-W*.tsv`) |
| 类别 C: 产品源码或运行库真回归 | 同布局对照复现编译错、断言失败或热态性能退化; 修复须改 `src/`/`cx/`/`ext/`/`kernel/` | **改变** | **必须重起** | 立即停预跑 -> `mv` 备份旧 `$PDIR` -> 修补或拆出该车(合车版红两次即拆回单车) + 同步派生物 + 刷 `FREEZE` -> 重起整链生成新 `APE` -> 新 `$PDIR` 全量重跑 |
| 类别 D: 负载性 / 磁盘或外部干扰 | `load1` 超门、他窗口并发重编译、或半成品材料目录(有目录无清单) | 不变 | 否 | 查清真实 PID 清理干扰; 半成品材料目录 `mv` 备份到 `~/.unisacc-worktree-backups/` 后重建, 再走类别 B 新窗 `W99NN` 补跑 |

### 4.4 补跑窗口四条铁律(防 `W9942` 误删重演)

1. **先备份**: 每次 `W99NN` 前执行 `cp -a "$PDIR" "$HOME/.unisacc-worktree-backups/prerun-v013-before-W99NN-$(date +%Y%m%dT%H%M%S)"`。
2. **窗号唯一禁 `rm`**: `dvqualify.cx:705-732` 对已存在 `--out` 直接 `sy_truncate`, 故每次尝试必递增全新窗号(`W9901 -> W9902 -> ...`), 启动前断言 `[ ! -e "$OUT_TSV" ] || exit 2`; 即使超时 `UNKNOWN` 也永久保留原文件, 严禁 `rm`/`mv`。
3. **同布局绝对路径预热丢弃至 `/tmp`**: 先按精确 `$LAYOUT_TREE` 绝对路径跑一次预热到 `/tmp/v013-prep-$$.tsv` 确认热态 `<25s`, 再正式写 `$PDIR/strict-W99NN-<batch>.tsv`。
4. **折叠双口径如实写进 `notes`**: `prerun-drive merge` 的 `PRERUN_MERGE` 中 `SUPERSEDED=k` 仅计 `FAIL->PASS`(不含 `INVALID`, `INVALID=0` 在 `SUMMARY` 行); `release-exec` 的 `GATE 1 STRICT` 中 `superseded=m` 计全部 `FAIL/UNKNOWN->PASS`(`m >= k`)。两行须原样分列写入 `release/notes-0.1.3.md`。

---

## (5) `release-exec` 七步与 `release-post` 原行判据与回滚总表

### 5.1 发布前打包、归档、预热与 `g1-dry` 演练

```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
REV="<C0b_REV_HEX40>"
CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$REV"
PDIR="$HOME/.cache/unisacc/prerun/v0.1.3-${REV:0:10}"
PACK="/tmp/v013-rel-pack/pack"
WARM="/tmp/v013-rel-pack/warm"
IH="$HOME/.cache/unisacc/prep-cache/identity-helper-b9ed85e8f3890fa8.bin"

# 1. 折叠与归档(注意: archive 真实参数为 --in-dir --repo --version, 无 --chain-dir)
(cd "$WT" && sh ./unisacc.com tools/prerun-drive.cx -- merge --dir "$PDIR" --version 0.1.3)
# 判据: PRERUN_MERGE ... FAIL=0 UNKNOWN=0; SUMMARY ... INVALID=0
(cd "$WT" && sh ./unisacc.com tools/prerun-drive.cx -- archive --in-dir "$PDIR" --repo "$WT" --version 0.1.3)
cp "$PDIR/strict-final.tsv" "$WT/release/evidence/0.1.3/strict.tsv"
(cd "$WT" && sh ./unisacc.com tools/fixedpoint-evidence.cx -- --chain "$CHAIN" --write "$WT/release/evidence/0.1.3/fixedpoint-6targets.tsv")

# 2. 打包 6 资产并核对 candidate.tsv 第 2/4/6/7 列
(cd "$WT" && env -i HOME="$HOME" PATH="/usr/bin:/bin:$HOME/.local/bin" TMPDIR=/tmp UNISACC_IDENT_HELPER="$IH" \
  sh ./unisacc.com tools/release-run.cx -- --pack "$WT" "$WT/release/candidate.tsv" "$CHAIN" "$PACK" --chain-out "$CHAIN")
# 判据: RELEASE_RUN RESULT PACK_OK; candidate.tsv 列2=v0.1.3 列4=0.1.3 列6=v0.1.3 列7=0

# 3. 备齐 release/evidence/0.1.3/{evidence-rows.tsv,known-gaps.md,INDEX.tsv}、release/{notes-0.1.3.md,rollback.txt}
#    并过脱敏与全量棘轮:
(cd "$WT" && sh ./unisacc.com tools/scrub-home-paths.cx -- --check release/evidence/0.1.3 release/notes-0.1.3.md release/rollback.txt)
(cd "$WT" && sh ./unisacc.com tools/ratchets.cx --)

# 4. --warm 预热读回 + --dry 七步演练(dry 输出写独立 /tmp/v013-dry-out 防污染 INDEX.tsv)
(cd "$WT" && sh ./unisacc.com tools/release-exec.cx -- --warm "$PACK/candidate.tsv" --layout "$PACK/layout" --out "$WARM")
# 判据: WARM RESULT READY key=<64hex> targets=5
cp "$PACK/candidate.tsv" "$WT/release/candidate.tsv"
DRY="/tmp/v013-dry-out" && rm -rf "$DRY" && mkdir -p "$DRY" && cp "$WARM"/warm-*.tsv "$DRY/"
(cd "$WT" && sh ./unisacc.com tools/release-exec.cx -- \
  --execute release/candidate.tsv --layout "$PACK/layout" --strict release/evidence/0.1.3/strict.tsv \
  --out "$DRY" --dry)
# 判据: rc=0; CMD 3 DRY 含 workflow run candidate-native.yml --ref v0.1.3 (非 SKIP);
#       末行 RELEASE_EXEC RESULT DRY_GATES_OK steps=7 deferred=6
# 提交 release/evidence/0.1.3 与 release/notes-0.1.3.md 前先还原 candidate.tsv(由 Step 1 X 自提交)
git -C "$WT" checkout HEAD -- release/candidate.tsv
```

### 5.2 `release-exec.cx` 七步原行判据清单与逐步骤回滚/撤回办法

源码依据: `tools/release-exec.cx:1-48,250-310,876-1700`。`release-exec` 在步骤 1..4 尚未公开时失败不触及公网状态; 一旦进入步骤 5 (`published=1`), 步骤 5..7 任一门失败即自动触发 `rollback()` (`release-exec.cx:295-310`), 退回草稿并将 `Latest` 切回 `v0.1.2`, **永不删除 tag 或 release, 永不强推 `main`**。

| 步骤 | 执行命令与受控停机 | 必须命中的原行判据 (`GATE` / `INDEX` / `RESULT`) | 失败时的精确回滚与撤回办法 |
|---|---|---|---|
| **Step 1 `X-commit`** | 拷 `$PACK/candidate.tsv` -> `release/candidate.tsv`; 跑 `--from-step 1` (`RELEASE_EXEC_POLL_MS=3000 RELEASE_EXEC_POLL_MAX=1 DEVSVC_OWNER=cc`); 在 `X_PUSHED` 停机后由 `cc` 快进推双远端 `git push origin HEAD:refs/heads/main HEAD:refs/heads/v0.1.3` + `git fetch origin` | `GATE 1 MAIN_ANCESTOR PASS`<br>`GATE 1 F_CHAIN_REV PASS <REV>` (或 `WAIVED` 若带 `--post-chain-merge`)<br>`GATE 1 STRICT PASS ...`<br>`GATE 1 CANDIDATE_SHA PASS`<br>`GATE 1 APE_VERSION PASS unisacc 0.1.3`<br>`GATE 1 RATCHETS PASS 19/0`<br>`INDEX exec-1-X-commit - <X_SHA>`<br>双远端 `ls-remote`: `main` 与 `v0.1.3` 均指 `<X_SHA>` | 尚无公网 release, 零公网影响。若 `X` 本身有误: 在 `v0.1.3` 与 `main` 执行 **`git revert <X_SHA>` 并快进推送(绝不 `git push --force`)**; 若 `MAIN_ADVANCED` 拒门: 核实 `origin/main` 增量与 `SOURCES_SID` 后合入, 重走 `--from-step 1` |
| **Step 2 `draft`** | 同命令再跑 `--from-step 1` (`RELEASE_EXEC_POLL_MS=3000 RELEASE_EXEC_POLL_MAX=1`): Step 1 自动 `SKIP`, Step 2 建私有草稿传 6 资产并下载校验 `SHA256SUMS` | `GATE 1 X_PUSHED PASS <X_SHA>`<br>`GATE 2 ASSETS PASS six assets equal candidate sha256`<br>`GATE 2 DRAFT_READBACK PASS five assets match downloaded SHA256SUMS`<br>`INDEX exec-2-draft-readback PASS draft-readback.txt` | 草稿私有未公开: 保持草稿私有不公开(`ROLLBACK 2 PLAN`); 若换候选走 `release-chain.cx -- retry-ci` 归档清理草稿, 对 `X` 做 `git revert`(不删已公开 tag) |
| **Step 3 `ci`** | (a) 上条命令触发 `candidate-native.yml --ref v0.1.3` 并在 3s 后有序停在 `CI_SUCCESS`; 人工用 `gh run view <run_id>` 盯 6 格 `attempt=1` 全绿;<br>(b) 填实 `notes-0.1.3.md` / `evidence-rows.tsv` 第 6/7 行 / `rollback.txt` 后跑 `--from-step 3` (不限 poll) | `INDEX exec-3-ci-dispatch - <run_id>`<br>`GATE 3 CI_SUCCESS PASS run success, six candidate targets success`<br>`GATE 3 CI_VERDICT PASS ...`<br>`INDEX exec-3-ci PASS <run_id>` | 草稿私有未公开: 若为 CI 瞬态故障(同候选), 执行 `--from-step 3 --redispatch`(工具自动写 `exec-3-ci-prior FAIL` 与 `supersedes`); 若为候选缺陷, 跑 `release-chain.cx -- retry-ci --version 0.1.3 --run-id <run_id> --evidence-dir release/evidence/0.1.3 --candidate release/candidate.tsv` 归档后重起候选 |
| **Step 4 `Y1`** | 紧接 Step 3 自动执行, 提交 `v0.1.3 Y1` 并记录冻结哈希对, 停在 `Y1_PUSHED`; 由 `cc` 快进推双远端 `main` + `v0.1.3` + `git fetch origin` | `GATE 4 MAIN_ANCESTOR PASS`<br>`INDEX exec-4-Y1 - <Y1_SHA>`<br>`INDEX exec-4-Y1-frozen - <sha_ev>:<sha_dec>`<br>快进后 `GATE 4 Y1_PUSHED PASS <Y1_SHA>` | 仍未公开: 保持草稿私有, 严禁越过进入 Step 5; 若需撤回 `Y1`, 按 `release/rollback.txt (3)` 执行 **`git revert <Y1_SHA>` (与 `X`) 并快进推送**, 绝不强推; 且切勿在 `Y1` 后手改 `release/evidence.tsv` 或 `release/decision.txt`(否则 Step 7 `Y1_FROZEN` 拒门) |
| **Step 5 `publish`** | 获 `cc` 公开令后: 先 `gh -R partnernetsoftware/unisacc release edit v0.1.3 --notes-file release/notes-0.1.3.md` 同步定稿正文并重跑 `--warm`, 再跑 `--from-step 5` (`DEVSVC_OWNER=cc`) | `GATE 5 READBACK_PREWARM PASS smoke passed; chain-step and release-ape hit`<br>`INDEX exec-5-publish - v0.1.3 isDraft=false, Latest` | 进入公网撤回边界(`published=1`): 若失败或正文/资产异常, 立即执行 `release/rollback.txt (1)`:<br>`gh -R partnernetsoftware/unisacc release edit v0.1.3 --draft=true`<br>`gh -R partnernetsoftware/unisacc release edit v0.1.2 --latest`<br>并 `git revert` 对应提交, **严禁删除 `v0.1.3` tag 或 release** |
| **Step 6 `readback`** | 紧接 Step 5 自动执行: 从公网重下 `unisacc.com`, 验 SHA-256 与候选一致, 跑 `c47-1-readback` 全 11 相位 `attempt=1 rc=0` | `GATE 6 PUBLIC_SHA PASS <ape_sha256>`<br>`GATE 6 READBACK PASS rc=0`<br>`INDEX exec-6-public-readback PASS public-readback.txt` | **自动触发 `rollback()` (`release-exec.cx:302-309`)**: 工具自动执行 `gh release edit v0.1.3 --draft=true` + `gh release edit v0.1.2 --latest` 并记 `INDEX exec-6-rollback PASS/FAIL`; 若任一 `draft_rc!=0` 或 `latest_rc!=0`, 立即手工补跑两条 `gh release edit` 并验证 `releases/latest` 确已指回 `v0.1.2`, 再 `git revert` 留痕(不删 tag) |
| **Step 7 `Y2`** | 紧接 Step 6 自动执行: 校验 `Y1_FROZEN` 与 `DECISION` 重算相等, 自提交 `v0.1.3 Y2: public readback`; `cc` 快进推双远端后再跑一次 `--from-step 7` 闭环, 并做 `jq -j .body` 原字节比对 | `GATE 7 MAIN_ANCESTOR PASS`<br>`GATE 7 Y1_FROZEN PASS`<br>`GATE 7 DECISION PASS recompute equals release/decision.txt`<br>`INDEX exec-7-Y2-done - <Y2_SHA>`<br>`RELEASE_EXEC RESULT EXECUTED steps=7 deferred=0` (`rc=0`)<br>`BODY_CMP=IDENTICAL` (`cmp -s` 对 `jq -j .body` 与 `release/notes-0.1.3.md`) | 若为 `Y1_FROZEN` 拒门(人工误改 `evidence.tsv`/`decision.txt`): 不回滚公网, 按 `NEXT` 行从 `Y1` 还原文件后重跑 `--from-step 7`; 若为 `BODY_CMP` 不一致: 立即 `gh release edit v0.1.3 --notes-file release/notes-0.1.3.md` 重比对; 若为公开物实质损坏: 按 Step 5/6 执行 `--draft=true` + `v0.1.2 --latest` + `git revert` |

### 5.3 `release-post` 与公开后解冻四步(全部先提交不推, 最后统一快进推 `main`)

```bash
WT="$HOME/repos/unisacc/.claude/worktrees/v0.1.3"
REV="<C0b_REV_HEX40>"
CHAIN="$HOME/.cache/unisacc/chains/bg-b1/$REV"
PDIR="$HOME/.cache/unisacc/prerun/v0.1.3-${REV:0:10}"
BDIR="$HOME/.unisacc-release-evidence-backups/0.1.3"

# (1) 拉远端 tag 并生成 R33 私有备份(删工作树含本机绝对路径的 backup-evidence.tsv)
git -C "$WT" fetch origin tag v0.1.3
rm -rf "$BDIR"
(cd "$WT" && sh ./unisacc.com tools/release-chain.cx -- backup-evidence \
  --version 0.1.3 --chain-dir "$CHAIN" --prerun-dir "$PDIR" --backup-dir "$BDIR")
# 判据: BACKUP_EVIDENCE OK version=0.1.3 files=7 dir=...
rm -f "$WT/release/evidence/0.1.3/backup-evidence.tsv"

# (2) release-post 先 --dry 后 --execute(默认只提交不推)
rm -f /tmp/v013-release-post-state.tsv
(cd "$WT" && DEVSVC_OWNER=cc sh ./unisacc.com tools/release-post.cx -- run \
  --version 0.1.3 --backup-dir "$BDIR" --state-file /tmp/v013-release-post-state.tsv --dry)
rm -f /tmp/v013-release-post-state.tsv
(cd "$WT" && DEVSVC_OWNER=cc sh ./unisacc.com tools/release-post.cx -- run \
  --version 0.1.3 --backup-dir "$BDIR" --state-file /tmp/v013-release-post-state.tsv --execute)
# 判据: POST_STEP 1 index-seal OK; POST_STEP 2 mark-version-seq OK;
#       POST_STEP 3 backup-evidence OK BACKUP_VERIFY-PASS; POST_STEP 4 metrics OK; RELEASE_POST status=DONE

# (3) 独立提交解冻 build/freeze.tsv(注释 0.1.3-FREEZE 行并注 published DATE Y2=<Y2_SHA>)
(cd "$WT" && DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git commit --cwd "$WT" \
  -m "v0.1.3 post-publish: thaw 0.1.3-FREEZE in build/freeze.tsv" --whole build/freeze.tsv)

# (4) 独立提交登记 tools/old-product-assets.tsv(追加 0.1.3 行)
(cd "$WT" && DEVSVC_OWNER=cc sh ./unisacc.com tools/devsvc.cx -- git commit --cwd "$WT" \
  -m "v0.1.3 post-publish: register 0.1.3 unisacc.com in tools/old-product-assets.tsv" --whole tools/old-product-assets.tsv)

# (5) 由 cc 快进推双远端并发布解冻通告(解除第 (1) 节 main 前进禁令, 随后可按需升级 Root 并合入 v0.1.4 积压)
DEVSVC_OWNER=cc UNISACC_INTEGRATOR=1 git -C "$WT" push origin HEAD:refs/heads/main HEAD:refs/heads/v0.1.3
```

---

## (6) 常见 CLI 参数陷阱速查(防止 `rc=2` 浪费窗口)

1. `tools/prerun-drive.cx archive` 真实参数为 `--in-dir` / `--dir`、`--repo`、`--version` (**无 `--chain-dir`**)。
2. `tools/prerun-inputs.cx` 必传 `--prev-rev`, **拒绝 `--times-file`**(自动只读拷贝 `~/.cache/unisacc/batch-times.tsv`)。
3. `tools/release-exec.cx --execute` 强制参数路径的 `realpath` 等于检出树内 `release/candidate.tsv`(不能直接传 `/tmp/.../pack/candidate.tsv`), 且自动读 `release/evidence/V/evidence-rows.tsv`(**无 `--evidence-rows`**)。
4. `tools/release-exec.cx` Step 2 建草稿正文为空, Step 5 公开前必须显式 `gh release edit v0.1.3 --notes-file release/notes-0.1.3.md` 并重跑 `--warm`, 公开后用 `jq -j .body`(禁止 `jq -r`)做 `cmp -s` 原字节比对。
5. `tools/release-post.cx` Step 3 只验证不生成备份, 跑前必须先 `git fetch origin tag v0.1.3` + `release-chain.cx -- backup-evidence` + 删除工作树里的 `release/evidence/0.1.3/backup-evidence.tsv`。
