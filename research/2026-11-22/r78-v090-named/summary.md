# R78 二期: 0.0.90 关键补丁集带 --expect-fail 具名杀重跑与 release-chain 存活变异补强汇总

## (1) --expect-fail 与 --expect-rc 匹配机制核查

`tools/mutation-gate.cx` 第 85..99 行 (`expect_failure`) 与第 473..497 行 (`quality_check`) 的匹配契约:
(1) `--expect-fail NAME`: 逐行扫描 check 输出中以 `FAIL ` 或 `FAIL\t` 开头的记录, 要求 `FAIL ` 或 `FAIL\t` 之后至行尾的内容与 `NAME` 精确全行匹配 (`strlen(name) + 5 == n && !memcmp(s + 5, name, n - 5)`), 且 `NAME` 内不可含 `\t`/`\r`/`\n` (上限 64 条)。
(2) `--expect-rc N`: 当 `rc < 124` 且输出无行首 `REFUSE ` / `ERR ` 基础设施前缀时, 若 `rc == N` (`1..123`) 则升级为 `KILL-named`。
(3) 优先级: 若命中任一 `--expect-fail NAME` 或 `--expect-rc N` (且无基础设施退出), 即使 `rc == 2` 也先于 `if (rc > 1) return Q_UNKNOWN;` 判定为 `KILL-named`。

---

## (2) 四个关键补丁带 --expect-fail (+ --expect-rc 1) 重跑汇总表

证据落盘目录: `~/.claude/jobs/2798e440/tmp/r78-v090-named/`

| 补丁标识 | 提交号 (base = REV~1) | 总采样 qn | 编译通过 N | KILL-named | KILL-weak | UNKNOWN (编译拒/超时) | INVALID | SURVIVED | MUTATION_QUALITY [lower, upper] | MUTATION_GATE score | verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| A: r80-bg-candidate | 188e45e558 | 5 | 4 | 2 | 0 | 3 (1/2) | 0 | 0 | [2/5, 5/5] (40.00% -> 100.00%) | 4/4 (100.00%) | PASS |
| B: r71-release-chain (原 check) | 73beca08c4 | 30 | 26 | 9 | 0 | 4 (4/0) | 0 | 17 | [9/30, 13/30] (30.00% -> 43.33%) | 9/26 (34.62%) | FAIL |
| C: r74-prerun-drive | b9f29ec898 | 30 | 26 | 17 | 0 | 4 (4/0) | 0 | 9 | [17/30, 21/30] (56.67% -> 70.00%) | 17/26 (65.38%) | PASS |
| D: r78-mutation-gate | 21c6fac34c | 30 | 26 | 18 | 0 | 4 (4/0) | 0 | 8 | [18/30, 22/30] (60.00% -> 73.33%) | 18/26 (69.23%) | PASS |
| 合计 (原 check 口径) | - | 95 | 82 | 46 | 0 | 15 (13/2) | 0 | 34 | [46/95, 61/95] (48.42% -> 64.21%) | 48/82 (58.54%) | 3 PASS / 1 FAIL |
| B+: r71-release-chain (补强后) | 73beca08c4 | 30 | 26 | 16 | 0 | 4 (4/0) | 0 | 10 | [16/30, 20/30] (53.33% -> 66.67%) | 16/26 (61.54%) | PASS |
| 合计 (补强 check-release-chain 后) | - | 95 | 82 | 53 | 0 | 15 (13/2) | 0 | 27 | [53/95, 68/95] (55.79% -> 71.58%) | 55/82 (67.07%) | 4 PASS / 0 FAIL |

真实杀伤率区间对比:
(1) 原 check + `--expect-fail`:
    - 全采样分母 (`qn = 95`): `[KILL-named, KILL-named + UNKNOWN] = [46/95, 61/95] = [48.42%, 64.21%]` (`KILL-weak = 0`)
    - 可编译变异分母 (`N = 82`, 排除 13 项 `compile_fail`): `[46/82, 48/82] = [56.10%, 58.54%]`
(2) 补强 `tools/check-release-chain.cx` 4 条断言后 + `--expect-fail`:
    - 全采样分母 (`qn = 95`): `[KILL-named, KILL-named + UNKNOWN] = [53/95, 68/95] = [55.79%, 71.58%]` (`KILL-weak = 0`)
    - 可编译变异分母 (`N = 82`, 排除 13 项 `compile_fail`): `[53/82, 55/82] = [64.63%, 67.07%]` (全部 4 个补丁均达 `>= 0.60` `verdict=PASS`)

---

## (3) 各补丁实跑原行

### 补丁 A: r80-bg-candidate (`~/.claude/jobs/2798e440/tmp/r78-v090-named/r80-bg-candidate/`)
```
MG_PREP files=1 added_lines=2 candidates=5 selected=5 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=34474
MUTATION_QUALITY named=2 weak=0 unknown=3 invalid=0 survived=0 adjudicated=0 lower=2/5 upper=5/5 min=0.60 status=INCONCLUSIVE
MUTATION_GATE mutants=4 killed=4 survived=0 compile_fail=1 missing=0 score=4/4(=1.00) adjudicated=0 score_adj=4/4(=1.00) min=0.60 verdict=PASS
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r80-bg-candidate/survivors.tsv
MG_RUN ms=101997
```

### 补丁 B (原 check): r71-release-chain (`~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain/`)
```
MG_PREP files=1 added_lines=31 candidates=55 selected=30 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=23901
MUTATION_QUALITY named=9 weak=0 unknown=4 invalid=0 survived=17 adjudicated=0 lower=9/30 upper=13/30 min=0.60 status=BELOW
MUTATION_GATE mutants=26 killed=9 survived=17 compile_fail=4 missing=0 score=9/26(=0.35) adjudicated=0 score_adj=9/26(=0.35) min=0.60 verdict=FAIL
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain/survivors.tsv
MG_RUN ms=160931
```

### 补丁 B+ (补强 check-release-chain.cx 后): r71-release-chain-strengthened (`~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain-strengthened/`)
```
MG_PREP files=1 added_lines=31 candidates=55 selected=30 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=33571
MUTATION_QUALITY named=16 weak=0 unknown=4 invalid=0 survived=10 adjudicated=0 lower=16/30 upper=20/30 min=0.60 status=INCONCLUSIVE
MUTATION_GATE mutants=26 killed=16 survived=10 compile_fail=4 missing=0 score=16/26(=0.62) adjudicated=0 score_adj=16/26(=0.62) min=0.60 verdict=PASS
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain-strengthened/survivors.tsv
MG_RUN ms=168543
```

### 补丁 C: r74-prerun-drive (`~/.claude/jobs/2798e440/tmp/r78-v090-named/r74-prerun-drive/`)
```
MG_PREP files=1 added_lines=25 candidates=71 selected=30 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=19223
MUTATION_QUALITY named=17 weak=0 unknown=4 invalid=0 survived=9 adjudicated=0 lower=17/30 upper=21/30 min=0.60 status=INCONCLUSIVE
MUTATION_GATE mutants=26 killed=17 survived=9 compile_fail=4 missing=0 score=17/26(=0.65) adjudicated=0 score_adj=17/26(=0.65) min=0.60 verdict=PASS
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r74-prerun-drive/survivors.tsv
MG_RUN ms=103098
```

### 补丁 D: r78-mutation-gate (`~/.claude/jobs/2798e440/tmp/r78-v090-named/r78-mutation-gate/`)
```
MG_PREP files=1 added_lines=161 candidates=233 selected=30 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=12933
MUTATION_QUALITY named=18 weak=0 unknown=4 invalid=0 survived=8 adjudicated=0 lower=18/30 upper=22/30 min=0.60 status=ABOVE
MUTATION_GATE mutants=26 killed=18 survived=8 compile_fail=4 missing=0 score=18/26(=0.69) adjudicated=0 score_adj=18/26(=0.69) min=0.60 verdict=PASS
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r78-mutation-gate/survivors.tsv
MG_RUN ms=68202
```

---

## (4) tools/release-chain.cx (73beca08c4) 17 个存活变异归类与补强验证

逐项分类表文件: `~/.claude/jobs/2798e440/tmp/r78-v090-named/release-chain-17-survivors.tsv`
补强补丁文件: `~/.claude/jobs/2798e440/tmp/r78-v090-named/check-release-chain-strengthen.patch`

三类统计 (共 17 项):
(1) `EQUIV_OR_DEGENERATE_FIXTURE` (4 项: id=2, 4, 23, 29):
    - `id=2` (`1197 行 if (!cand) return 1;`): 调用方恒传非 NULL 缓冲区 (`dst[4400]` 或 `bdir`), 空指针防御分支不可触发。
    - `id=4` (`1207 行 delete return 0;`): x86_64 ABI 下前一条 `sy_realpath(...) == 0` 比较为假时 `%eax` 已为 `0`, 删去末尾 `return 0;` 返回值不变。
    - `id=23` (`1201 行 char cabs[4097];`): `sy_realpath` 内部上限为 4096 字节 (`PATH_MAX`), 调用方栈缓冲 `4096 -> 4097` 完全等价。
    - `id=29` (`1202 行 sy_realpath(...) == 1`): `sy_realpath` 返回长度 `1` 仅当真实路径为 `"/"`, 而 `"/"` 不含 `"/.cache/"` 也不以 `"/.cache"` 结尾, 分支内外均返回 `0`。
(2) `UNREACHABLE_BRANCH` (9 项: id=7, 10, 16 及受 1202 行 `sy_realpath == 0` 死条件阻塞的 id=13, 17, 21, 24, 27, 28):
    - `id=7, 10, 16` (`1200 行 if (n >= 7 && !strcmp(cand + n - 7, "/.cache"))`): 原 check 只测了含 `"/.cache/"` 中间段的路径, 未测以 `"/.cache"` 结尾且长度恰好为 7 的路径。在 `check-release-chain.cx` 中补 `backup-evidence-exact-cache-suffix-refused` (`--backup-dir "/.cache"`) 后, id=7, 10, 16 全部被具名杀死 (`KILL-named`)。
    - `id=13, 17, 21, 24, 27, 28` (`1203..1205 行 sy_realpath 分支体`): 发现 `tools/release-chain.cx:1202` (及 `1185` 行) 存在真实生产代码缺陷——`cx/sys/fs.cx:130` 中 `sy_realpath` 成功返回路径长度 `n >= 1`、失败返回 `-errno < 0`, 永不返回 `0`; 因此 `if (sy_realpath(cand, cabs, sizeof cabs) == 0)` 在未变异代码中恒为假, 导致 `1203..1205` 行在 `73beca08c4` 上成为不可达死代码 (若将 `tools/release-chain.cx:1202` 修为 `>= 0` 并配符号链接夹具即可全部覆盖)。
(3) `WEAK_CHECK` (4 项: id=11, 14, 20, 22):
    - `id=11` (`1264 行 verify_backup_manifest 内 path_inside_cache 分支 delete return 2;`): 原 check 未测试 `--verify` 遇到清单中 `dst` 位于 `/.cache/` 时的拒绝码与原因。补 `backup-verify-inside-cache-manifest-refused` 后 id=11 被具名杀死 (`KILL-named`)。
    - `id=20` (`1317 行 if (!cdir || !pdir) usage(...)` 变异为 `&&`): 原 check 未测单侧漏传 `--prerun-dir` 时的 `REFUSE CHAIN_USAGE`。补 `backup-evidence-missing-prerun-dir-usage-refused` 后 id=20 被具名杀死 (`KILL-named`)。
    - `id=14, 22` (`1202 行 sy_realpath(...) != 0` / `!(... == 0)`): 反转条件后在 `cand` 尚不存在 (`sy_realpath` 返回 `-ENOENT < 0`) 时误入分支并读取未初始化的栈缓冲 `cabs[4096]` (复用前一调用 `path_inside_repo` 的 `rabs[4096]`)。补 `backup-evidence-nonexistent-dir-ignores-stale-cache-stack` (当 `--repo` 位于 `.cache` 下而 `--backup-dir` 为仓外非 `.cache` 新目录时应成功 `rc=0`) 后, id=14, 22 全部被具名杀死 (`KILL-named`)。

补强效果: `check-release-chain.cx` 新增 4 条断言 (共杀死 id=7, 10, 11, 14, 16, 20, 22 共 7 个原先存活的变异), 基线 `MG_BASELINE check=0 rc=0 failed=0`, `r71-release-chain` 的 `MUTATION_GATE` 从 `9/26 (=0.35, FAIL)` 提升至 `16/26 (=0.62, PASS)`, `KILL-named` 从 `9` 提升至 `16`。
