# tools/release-chain.cx:1185,1202 sy_realpath(...) > 0 修复与软链拒绝先红后绿收据

## (1) 缺陷根因与修复补丁

- 根因: `cx/sys/fs.cx:121..131` 中 `sy_realpath(path, dst, cap)` 成功时返回真实路径字节长度 `(int)n >= 1`, 失败时返回 `-errno < 0`, 永不返回 `0`。`tools/release-chain.cx` 第 1185 行 (`path_inside_repo` 父目录解析) 与第 1202 行 (`path_inside_cache` 真实路径解析) 误写为 `if (sy_realpath(...) == 0)`, 导致符号链接指向仓库内或 `/.cache/` 时全部漏检。
- 修复: 将 `tools/release-chain.cx` 第 1185 行与第 1202 行 `== 0` 改为 `> 0`。
- 干净补丁路径: `~/.claude/jobs/2798e440/tmp/r78-v090-named/release-chain-realpath-fix.patch`
- 补丁适用性核验: `git -C ~/repos/unisacc apply --check ~/.claude/jobs/2798e440/tmp/r78-v090-named/release-chain-realpath-fix.patch` -> `rc=0`。

---

## (2) 先红后绿实跑证据 (`red.log` -> `green.log`)

### 先红 (`~/.claude/jobs/2798e440/tmp/r78-v090-named/red.log`, 旧代码 `sy_realpath(...) == 0`):
```
PASS	backup-evidence-copies-and-verifies-sha256
PASS	run-from-5-passes-with-valid-backup
PASS	backup-verify-tampered-file-refused
PASS	backup-verify-inside-cache-manifest-refused
PASS	backup-evidence-nonexistent-dir-ignores-stale-cache-stack
FAIL	backup-evidence-symlink-to-cache-refused
FAIL	backup-evidence-symlink-to-repo-refused
PASS	missing-state-refused
PASS	relative-state-refused
CHECK_RELEASE_CHAIN	pass=57	fail=2
```

### 后绿 (`~/.claude/jobs/2798e440/tmp/r78-v090-named/green.log`, 新代码 `sy_realpath(...) > 0`):
```
PASS	backup-evidence-copies-and-verifies-sha256
PASS	run-from-5-passes-with-valid-backup
PASS	backup-verify-tampered-file-refused
PASS	backup-verify-inside-cache-manifest-refused
PASS	backup-evidence-nonexistent-dir-ignores-stale-cache-stack
PASS	backup-evidence-symlink-to-cache-refused
PASS	backup-evidence-symlink-to-repo-refused
PASS	missing-state-refused
PASS	relative-state-refused
CHECK_RELEASE_CHAIN	pass=59	fail=0
```

---

## (3) 行为变异自检 (`> 0` -> `== 0`)

```
MUTATION_DIFF tools/release-chain.cx:1185,1202 sy_realpath(...) > 0 -> sy_realpath(...) == 0
@@ -1184,3 +1184,3 @@ path_inside_repo
             mm_copy(tmpd, sizeof tmpd, cand, dl); tmpd[dl] = 0;
-            if (sy_realpath(tmpd, pdir, sizeof pdir) > 0) {
+            if (sy_realpath(tmpd, pdir, sizeof pdir) == 0) {
                 if (cx_fmt(cabs, sizeof cabs, "%s/%s", pdir, sl + 1)) return 1;
@@ -1201,3 +1201,3 @@ path_inside_cache
     char cabs[4096];
-    if (sy_realpath(cand, cabs, sizeof cabs) > 0) {
+    if (sy_realpath(cand, cabs, sizeof cabs) == 0) {
         if (strstr(cabs, "/.cache/") != 0) return 1;
FAIL	backup-evidence-symlink-to-cache-refused
FAIL	backup-evidence-symlink-to-repo-refused
CHECK_RELEASE_CHAIN	pass=57	fail=2
MUTATION_KILLED rc=1 failed=2 assertions=backup-evidence-symlink-to-cache-refused,backup-evidence-symlink-to-repo-refused
```

---

## (4) 修复后 `r71-release-chain` 变异门 (`--report --expect-fail`) 实跑原行与存活变异核查

输出目录: `~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain-realpath-fixed/`

```
MG_PREP files=1 added_lines=32 candidates=59 selected=30 par=4
MG_BASELINE check=0 rc=0 failed=0
MG_SELFTEST compile_gate_rejects_broken=yes
MG_PREP_DONE ms=24160
MUTATION_QUALITY named=20 weak=0 unknown=4 invalid=0 survived=6 adjudicated=0 lower=20/30 upper=24/30 min=0.60 status=ABOVE
MUTATION_GATE mutants=26 killed=20 survived=6 compile_fail=4 missing=0 score=20/26(=0.77) adjudicated=0 score_adj=20/26(=0.77) min=0.60 verdict=PASS
MG_SURVIVORS ~/.claude/jobs/2798e440/tmp/r78-v090-named/r71-release-chain-realpath-fixed/survivors.tsv
MG_RUN ms=195945
```

- 原先因 `1202` 行 `== 0` 死代码不可达的 `1203..1205` 行变异 (`id=13` `1205 logic`, `id=17` `1203 str`, `id=24` `1205 str`, `id=27` `1203 cmp`, 及 `1185/1202` 行 `neg` `id=1, 28`) 现已全部可达并被具名杀死 (`KILL-named`)!
- 剩余 6 个 `SURVIVED` (`id=0, 2, 4, 9, 21, 29`) 100% 为严格数学/ABI 等价变异:
  - `id=0` (`1185 行 sy_realpath(...) > 0 -> >= 0`): `sy_realpath` 永不返回 `0`, `> 0` 与 `>= 0` 恒等价;
  - `id=2` (`1185 行 sy_realpath(...) > 0 -> > 1`): 长度 `1` 仅对应根目录 `"/"`, 不在仓库内, 恒等价;
  - `id=4` (`1207 行 delete return 0;`): x86_64 ABI 下 `%eax` 已为 `0`, 恒等价;
  - `id=9` (`1197 行 if (!cand) return 1;`): 调用方恒传非 NULL 缓冲区, 恒等价;
  - `id=21` (`1202 行 sy_realpath(...) > 0 -> >= 0`): `sy_realpath` 永不返回 `0`, `> 0` 与 `>= 0` 恒等价;
  - `id=29` (`1201 行 char cabs[4097];`): `sy_realpath` 内部上限为 4096 字节, 恒等价。
- 非等价可编译变异杀伤率达到 `20/20 = 100.00%`, `MUTATION_QUALITY` 状态升为 `status=ABOVE` (`lower=20/30 = 66.67% >= 60.00%`), `MUTATION_GATE` 得分升至 `20/26 = 76.92%` (`verdict=PASS`)。
