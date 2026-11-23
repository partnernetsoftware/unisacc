# R81 盘点报告: 永不失败的阈值断言与文本存在性自测 vs 行为探针

## (1) 盘点口径与全仓规模真值

- 扫描工具: `~/.claude/jobs/2798e440/tmp/r81-census/r81-scan.cx`
- 产出表 1 (阈值断言): `~/.claude/jobs/2798e440/tmp/r81-census/r81-thresholds.tsv`
- 产出表 2 (自测分类): `~/.claude/jobs/2798e440/tmp/r81-census/r81-check-modes.tsv`
- 扫描范围: `tools/check--.cx` 共 764 个文件, `tests/--.cx` 共 232 个文件 (`tools/` 顶层 `.cx` 共 1259 个)
- 全仓关键表实测规模: `tests/batches.tsv` 数据行 1909 (`IMPLEMENTED` 1883)、`tests/coverage.tsv` 数据行 1539、`build/chain-phases.tsv` 499 相、`build/chain-phase-fallthrough.tsv` 499 行、`build/std-surface.tsv` 545 行、`build/core-closure.tsv` 627 行、`build/lane-inputs.tsv` 2120 行、`build/pkg-gaps.tsv` 231 行、`build/owner-reqs.tsv` 72 行、`exec/facts/derive.tsv` 14 行

## (2) 自测模式盘点 (`ccogw` 第 103 节: `TEXT_EXISTENCE` vs `BEHAVIORAL_PROBE` vs `IN_PROCESS_UNIT`)

| 模式分类 | 子类 (`submode`) | 文件数 | 占比 | 定义与特征 |
|---|---|---|---|---|
| `TEXT_EXISTENCE` | `SOURCE_SUBSTR_GREP_NO_EXEC` / `STATIC_TOKEN_SCAN_NO_EXEC` / `TABLE_OR_DOC_SCAN_NO_EXEC` | 19 (其中关键门相关 8 个) | 2.4% | 不调用任何子进程运行 API (`ck_run`/`np_run`/`px_spawn_spool`/`sy_run` 等), 仅读仓内 `.cx`/`.tsv`/`.md` 源码查找字符串或数 token |
| `BEHAVIORAL_PROBE` | `PURE_BEHAVIORAL_PROBE` / `PROBE_WITH_MUTATION_COPY` (10 个) / `PROBE_PLUS_SOURCE_GREP` (5 个) | 560 | 73.2% | 实际起子进程运行被测程序并校验输出与退出码 |
| `IN_PROCESS_UNIT` | `DIRECT_LIBRARY_CALL` | 185 | 24.2% | 直接 `#pragma unisa use` 被测库并在同一进程内调用被测函数 |

### (2.1) 纯 `TEXT_EXISTENCE` 检查完整清单 (19 个, 含关键门相关 8 个)

| 检查文件 | 子类 | 关键门 | 读取 API | 被检视的仓内目标文件 | 行数 |
|---|---|---|---|---|---|
| `tools/check-batch-split.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | NO | `cx_read` | `tests/stdio-crossunit.cx,tests/check-stream-pipe.cx,tests/stdio-crossunit-split.tsv,tests/stream-pipe-split.tsv` | 73 |
| `tools/check-chain-argv-check.cx` | `STATIC_TOKEN_SCAN_NO_EXEC` | YES | `cx_read` | `tools/candidate-chain.cx,tools/chain-step.cx,tools/whole-front.cx` | 137 |
| `tools/check-chain-argv-timeout.cx` | `STATIC_TOKEN_SCAN_NO_EXEC` | YES | `cx_read` | `tools/source-job-wave.cx,tools/chain-step.cx` | 37 |
| `tools/check-cx-no-ext-deps.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `sy_read_file` | `build/tlib-packages.tsv` | 98 |
| `tools/check-ext-dock.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | NO | `cx_read` | `build/ext-dock-exceptions.tsv,ext/a/x.cx,ext/a/pure.cx,ext/a/new.cx` | 518 |
| `tools/check-fake-gh-contract.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | YES | `cx_read` | `tools/cxlib/releaseexec.cx,tools/release-body-diff.cx,tools/ci-verdict.cx,tools/post-publish-check.cx,tests/cases/fake-gh.cx` | 101 |
| `tools/check-mirror-pairs.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `sy_read_file` | `tests/mirror-pairs.tsv` | 239 |
| `tools/check-objectplan-pair.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `sy_read_file` | `exec/enc/objectplan-result.tsv,tests/objectplan-pair-allow.tsv,exec/enc/objectplan-manifest.tsv` | 245 |
| `tools/check-out-writers.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | YES | `cx_read` | `tools/release-build.cx,tools/release-run.cx,tools/cxpack-candidate-check.cx,tools/driverdiag-candidate-check.cx,tools/chain-run.cx,tools/chain-step.cx,tools/candidate-chain.cx,tools/s9-root.cx,tools/s9-daily-flow.cx,tools/check-chain-batchkeys.cx,tools/ot` | 254 |
| `tools/check-own-adoption.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `cx_read` | `build/own-adoption.tsv,build/own-escapes.tsv` | 206 |
| `tools/check-own-escapes.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `cx_read` | `build/own-escapes.tsv` | 126 |
| `tools/check-pins.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | YES | `cx_read` | `tools/source-job-wave.cx,tools/pins.tsv,tools/pins-line-exceptions.tsv` | 411 |
| `tools/check-prune-nametable.cx` | `TABLE_OR_DOC_SCAN_NO_EXEC` | NO | `cx_read` | `exec/facts/tape-shape.tsv,exec/facts/prune-nametable.tsv,exec/prune/prune-result.tsv` | 190 |
| `tools/check-run-uxe-regress.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | NO | `fopen` | `tools/run-uxe-regress.cx` | 29 |
| `tools/check-sixtarget-roots.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | NO | `sy_read_file` | `build/chain-facts.tsv,src/fake_win_only.cx` | 87 |
| `tools/check-source-job-count.cx` | `STATIC_TOKEN_SCAN_NO_EXEC` | YES | `cx_read` | `tools/source-package.cx,tools/source-job-wave.cx` | 143 |
| `tools/check-sourcepackage-pins.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | YES | `fopen` | `tools/cxlib/sourcepackage.cx,tools/source-package.cx,tools/source-package-pack.cx` | 225 |
| `tools/check-sy-stat-struct.cx` | `SOURCE_SUBSTR_GREP_NO_EXEC` | NO | `sy_read_file` | `cx/cx0/bundle.h,cx/cx0/bundle.cx,cx/sys/fs.cx,tests/ok.cx,cx/runtime/runexec.cx,src/other.cx,src/dup.cx` | 565 |
| `tools/check-whole-front-expand.cx` | `STATIC_TOKEN_SCAN_NO_EXEC` | YES | `cx_read` | `tools/candidate-chain.cx,cx/runtime/modelcache.cx` | 190 |

### (2.2) 混合式行为探针 (`PROBE_PLUS_SOURCE_GREP`: 虽有子进程调用, 但同时直接 `cx_read`/`fopen` 仓内 `.cx` 源码并做 `strstr`/`tx_find` 字符串扫描, 共 5 个)

| 检查文件 | 关键门 | 运行 API | 读取 API | 被检视的仓内目标文件 | 行数 |
|---|---|---|---|---|---|
| `tools/check-candidate-chain.cx` | YES | `cx_runv_timeout,sy_spawn` | `cx_read` | `tools/candidate-chain.cx,build/g2-budget.tsv,tools/entry-build.cx,tools/chain-phases-gen.cx,tools/cache-retain.cx,build/chain-phases.tsv,build/chain-phase-fallthrough.tsv` | 1601 |
| `tools/check-devsvc-remain.cx` | YES | `np_run` | `sy_read_file` | `tools/devsvc-core.cx,tools/devsvc.cx` | 212 |
| `tools/check-net-digest.cx` | NO | `np_run` | `cx_read` | `tools/net-deps.cx,exec/pp/location-byte.tsv,tools/cxlib/sourcepackage.cx` | 22 |
| `tools/check-s9sched.cx` | NO | `np_run` | `cx_read` | `tools/s9-nightly.cx,tools/check-s9-nightly.cx,tools/cxlib/s9sched.cx` | 53 |
| `tools/check-stage-lower.cx` | NO | `np_run` | `cx_read` | `tests/stream-lower-domain.cx` | 179 |

## (3) 永不失败的阈值断言盘点 (`ccogw` 第 100 节: `>= 常量` / `> 常量`, 常量 `>= 5`)

- 扫描共提取断言调用内常量下限阈值条件 (`>= CONST`、`> CONST` 或 `if (... < CONST) fail`, `CONST >= 5`): 88 处
- 已关联全仓/被测表真实规模 (`actual_val > 0`): 35 处
- 疑似永不失败 (`实际/常量 > 3` 倍, `SUSPECT_GT_3X`): 27 处

### (3.1) `实际/常量 > 3` 倍 (`SUSPECT_GT_3X`) 的疑似永不失败阈值断言清单 (27 处)

| 位置 (`file:line`) | 比较表达式 | 阈值常量 | 实测真实规模 | 余量比 (`实际/常量`) | 断言上下文与描述 |
|---|---|---|---|---|---|
| `tools/check-affected.cx:721` | `full >= 100` | 100 | 1883 (`tests/batches.tsv(IMPLEMENTED=1883批)`) | 18.8x |  ck(rc == 0 && full >= 100, "iface-baseline-full"); ck(!has(o, "IFACE_NARROW\t") && !has(o, "DEFERRED_BY_IFACE"), "iface-default-prints-no-narrow-line"); |
| `tools/check-affected.cx:790` | `fullt >= 100` | 100 | 1883 (`tests/batches.tsv(IMPLEMENTED=1883批)`) | 18.8x |  ck(rc == 0 && fullt >= 100, "iface-pkg-baseline-text"); } |
| `tools/check-affected.cx:794` | `fulls >= 100` | 100 | 1883 (`tests/batches.tsv(IMPLEMENTED=1883批)`) | 18.8x |  ck(rc == 0 && fulls >= 100, "iface-pkg-baseline-sys"); } |
| `tools/check-affected.cx:836` | `fullt >= 100` | 100 | 1883 (`tests/batches.tsv(IMPLEMENTED=1883批)`) | 18.8x |  ck(rc == 0 && fullt >= 100 && !has(o, "DEFERRED_BY_SYMBOL") && !has(o, "SYMBOL_NARROW"), "sym-default-prints-nothing"); } |
| `tools/check-boundenv.cx:103` | `scanned > 100` | 100 | 1491 (`tools+tests/*.cx(1491文件)`) | 14.9x |  ck(hits == 0 && scanned > 100, "no-bare-read-outside-helper"); } |
| `tools/check-candidate-chain.cx:122` | `rows >= 100` | 100 | 499 (`build/chain-phases.tsv(499相)`) | 4.9x |  ck(had_tb && rows >= 100 && !over && cp_kinds >= 5 && has_group && has_override && has_lane && has_serial, "call-plan-kinds-cover-the-real-phase-table"); |
| `tools/check-candidate-chain.cx:1484` | `nf > 30` | 30 | 499 (`build/chain-phase-fallthrough.tsv(499行)`) | 16.6x |  ck(r1 == 0 && nf > 30 && has(o1, "\tuntil=-\tstop=end\n") && r2 == 0 && np > 20 && has(o2, "\tuntil=pack\tstop=step\n") && r3 == 0 && nc > 30 && has(o3, "\tuntil=compare\tstop=step\n") && r4 == 0 && has(o4, "\tuntil=jobs- |
| `tools/check-candidate-chain.cx:1484` | `np > 20` | 20 | 499 (`build/chain-phase-fallthrough.tsv(499行)`) | 24.9x |  ck(r1 == 0 && nf > 30 && has(o1, "\tuntil=-\tstop=end\n") && r2 == 0 && np > 20 && has(o2, "\tuntil=pack\tstop=step\n") && r3 == 0 && nc > 30 && has(o3, "\tuntil=compare\tstop=step\n") && r4 == 0 && has(o4, "\tuntil=jobs- |
| `tools/check-candidate-chain.cx:1485` | `nc > 30` | 30 | 499 (`build/chain-phase-fallthrough.tsv(499行)`) | 16.6x |  && r3 == 0 && nc > 30 && has(o3, "\tuntil=compare\tstop=step\n") && r4 == 0 && has(o4, "\tuntil=jobs-tool-0-start\tstop=inner\n"), "main-steps-printed-read-only"); |
| `tools/check-chain-phases-gen.cx:472` | `outers > 5` | 5 | 499 (`build/chain-phase-fallthrough.tsv(499行)`) | 99.8x |  ck(ok && outers > 5 && lanes > 5, "fallthrough-contains-deps-closure"); } |
| `tools/check-chain-phases-gen.cx:472` | `lanes > 5` | 5 | 499 (`build/chain-phase-fallthrough.tsv(499行)`) | 99.8x |  ck(ok && outers > 5 && lanes > 5, "fallthrough-contains-deps-closure"); } |
| `tools/check-core-libc-map.cx:49` | `tn > 100` | 100 | 5686 (`build/core-libc-map.tsv(5686字节)`) | 56.8x |  ck(tb != 0 && tn > 100, "repo-table-readable"); if (tb) { |
| `tools/check-cscan.cx:26` | `n > 100` | 100 | 1820 (`tools/check-cscan.cx(1820字节)`) | 18.2x |  ck(e && n > 100 && e[n] == 0 && e[0] == '/', "slurp-reads-file"); ck(!cs_slurp("tools/no-such-file.cx", &n), "slurp-missing-is-null"); |
| `tools/check-cx-cond-use.cx:123` | `cu_files > 100` | 100 | 1491 (`tools+tests/*.cx(1491文件)`) | 14.9x |  cu_ck(cu_files > 100 && cu_repo_hits == 0, "repo-cx-src-zero-cond-use"); { |
| `tools/check-dcheck.cx:117` | `num("DCHECK_SELECT\tchanged=") >= 10` | 10 | 1909 (`tests/batches.tsv(1909行)`) | 190.9x |  ck(rc == 0 && num("DCHECK_SELECT\tchanged=") >= 10 && has("CHECK\tfx-daily\tPASS\t") && has("DCHECK\ttotal=1\tpass=1\t"), "git-worktree-changes-are-the-default"); |
| `tools/check-devsvc-core.cx:176` | `n > 8` | 8 | 1432 (`tools/devsvc-repo.tsv(1432字节)`) | 179.0x |  ck(okw && !sy_read_file(hp, &b, &n, 1 << 20) && b && n > 8 && !memcmp(b, "dvmemo1\t", 8), "bad-memo-rewritten"); if (b) free(b); } |
| `tools/check-devsvc.cx:424` | `strlen(out) > 8` | 8 | 1432 (`tools/devsvc-repo.tsv(1432字节)`) | 179.0x |  ck(dv1("git", "log", "-1", "--oneline", 0, 0) == 0 && strlen(out) > 8 && !has("REFUSE"), "git-log-readonly"); ck(dv1("git", "diff", "--stat", "HEAD", 0, 0) == 0 && !has("REFUSE"), "git-diff-stat-readonly"); |
| `tools/check-docslice.cx:162` | `minov >= 50` | 50 | 834 (`plans/devsvc.md(834行)`) | 16.6x |  ck(okw && minov >= 50 && maxov <= 80, "window_overlap"); ck(rc == 0 && tiles(&o, n) && all_at_units(&o, big, n), "sentence_units"); |
| `tools/check-gen-header-idents.cx:48` | `n > 1000` | 1000 | 180160 (`build/header-idents.tsv(180160字节)`) | 180.1x |  ck(t && n > 1000, "repo-table-nonempty"); ck(t && mh_rowcount(t, n, "stdio.h\tprintf\tident") == 1, "repo-printf-ident"); |
| `tools/check-lane-inputs-gen.cx:41` | `r.outn >= 64` | 64 | 2120 (`build/lane-inputs.tsv(2120行)`) | 33.1x |  int ok = !tr && r.rc == 0 && r.out && r.outn >= 64; if (ok) { |
| `tools/check-lane-inputs-gen.cx:355` | `hit >= 7` | 7 | 2120 (`build/lane-inputs.tsv(2120行)`) | 302.8x |  int ok = (ok_rc == 8 && built >= 1 && hit >= 7); printf("%s\tlane-prep.cold-8way-conc\tok_rc=%d/8 built=%d hit=%d\n", ok ? "PASS" : "FAIL", ok_rc, built, hit); |
| `tools/check-phase-times-calc.cx:124` | `rows >= 100` | 100 | 499 (`build/chain-phases.tsv(499相)`) | 4.9x |  ck(tb && rows >= 100 && dep_bad == 0, "c58-sim-keeps-every-dependency-edge"); ck(ni >= 400 && lane_bad == 0, "c58-sim-same-lane-phases-never-overlap"); |
| `tools/check-refusals-gen.cx:73` | `m > 30` | 30 | 1743 (`exec/facts/refusals.tsv(1743字节)`) | 58.1x |  ck(m > 30 && mm_same(t, "#pragma unisa module refusals\n#pragma unisa dock\n", 49), "module-header-first"); ck(hasl(t, "Generated by tools/refusals-gen.cx --write from exec/facts/refusals.tsv; do not edit by hand."), "generated-bann |
| `tools/check-register-cold-runner.cx:132` | `rows >= 9` | 9 | 1909 (`tests/batches.tsv(1909行)`) | 212.1x |  ck(rows >= 9, "anchor fact table has at least 9 rows"); if (regpath[0]) fixture(regpath); else ck(0, "register source named by the fact table"); |
| `tools/check-release-prep.cx:203` | `gn >= 15` | 15 | 69 (`release/evidence/v0.0.74-strict-2026-11-18.tsv(69行)`) | 4.6x |  check("ratchets-same-rows-as-committed", g && w && gn >= 15 && has(g, want_ok) && has(w, "RATCHETS\tok=15\tfail=0") && (!strcmp(fc, T_COMMIT) ? gn == wn : gn >= wn)); |
| `tools/check-textx-json-emit.cx:371` | `cnt >= 500` | 500 | 3338 (`tests/fixtures/textx-json-emit/numbers.tsv(3338行)`) | 6.6x |  tk_ok(src && cnt >= 500 && bad == 0, "jsdiff-nested"); } |
| `tests/tools-compile.cx:498` | `rn <(fail) 5` | 5 | 1909 (`tests/batches.tsv(1909行)`) | 381.8x |  size_t rn;char -r=cx_read(p,&rn);if(!r--rn<5--memcmp(r,"FAIL\t",5))die("deferred result"); FILE -f=fopen(p,"wb");if(!f)die("deferred result open"); |

### (3.2) 其余常量阈值断言清单 (`actual/const <= 3x` 或特定夹具内局部计数)

| 位置 (`file:line`) | 比较表达式 | 阈值常量 | 实测真实规模 | 余量比 | 断言上下文与描述 |
|---|---|---|---|---|---|
| `tools/check-candidate-chain.cx:122` | `cp_kinds >= 5` | 5 | chain-phases种类(7种) | 1.4x |  ck(had_tb && rows >= 100 && !over && cp_kinds >= 5 && has_group && has_override && has_lane && has_serial, "call-plan-kinds-cover-the-real-phase-table"); |
| `tools/check-chain-argv-timeout.cx:28` | `calls(tw,"wave_bs") >= 5` | 5 | - | - |  ck(calls(tw,"wave_secs")>=6&&calls(tw,"wave_bs")>=5,"wave-all-sites-derived"); ck(fcnt(tw,"wave_secs","UNISACC_CHAIN_PHASE_BOUND_S",1)==1&&fcnt(tw,"wave_secs","6",0)==1&&fcnt(tw,"wave_secs","90",0)==1&&fcnt(tw,"wave_secs","5",0)==2,"wave- |
| `tools/check-chain-run.cx:427` | `nb >= 8` | 8 | - | - |  ck(nb>=8&&nb<=20,"need-measured-call-bound-is-segment-left-not-above"); { char tc[4400];file_of(tc,"needfits","tail-calls");ck(count(tc,"REFUSE")==0&&!has(p,"BLOCKER"),"need-measured-call-accepted-by-validator"); } |
| `tools/check-chain-run.cx:828` | `en > 8` | 8 | - | - |  { char e[4400];size_t en=0;join(e,base,"yield/yield-env");char -eb=cx_read(e,&en);ck(eb&&en>8&&eb[0]>='1'&&eb[0]<='9',"attempt-end-handed-to-the-phase");free(eb); } |
| `tools/check-chaincall.cx:126` | `cases > 100000` | 100000 | - | - |  ck(cases > 100000 && !bad_shape, "answer-is-run-run-group-or-yield"); ck(!bad_accept, "every-issued-call-passes-the-validator"); |
| `tools/check-ctor-debt.cx:191` | `min >= 19` | 19 | - | - |  ck(inv && min >= 19 && rows >= min, "inventory-rows-kept"); } |
| `tools/check-ctor-debt.cx:150` | `r.outn >= 40` | 40 | - | - |  ok = !r.rc && r.outn >= 40 && tx_same(r.out, 40, want, 40); if (!ok) pr_echo(&r); |
| `tools/check-cx-phase.cx:30` | `n > 100` | 100 | - | - |  tk_ok(tf != 0 && n > 100, "tsv-written"); return tk_end("CHECK_CX_PHASE", 0); |
| `tools/check-derive-tracked-only.cx:187` | `rows >= 14` | 14 | exec/facts/derive.tsv(14行) | 1.0x |  ck(rows >= 14 && bad == 0, "all-14-derive-audited: every derive.tsv generator guards dir scans with tracked set"); } |
| `tools/check-dev-product.cx:87` | `n > 8` | 8 | - | - |  ck((n > 8 && b[0] == 'M' && b[1] == 'Z' && b[2] == 'q' && b[3] == 'F' && b[4] == 'p' && b[5] == 'D' && b[6] == '=' && b[7] == 39) -- |
| `tools/check-funit-procslot.cx:227` | `built <(fail) 16` | 16 | - | - |  if (built < 16) { printf("FAIL\tcorpus-slot\tonly %d of 17 sources compiled, want at least 16\n", built); fails++; } if (named == 0) { printf("FAIL\tcorpus-slot\tno named _unisa_proc_- relocation in the corpus (vacuous scan)\n"); fa |
| `tools/check-iface-hash.cx:131` | `lastn > 6` | 6 | - | - |  ck(last && lastn > 6 && tx_prefix(last, (long)lastn, "REFUSE", 6) && cell(tb, 0, 0, num, sizeof num), "duplicate-rows-REFUSE-no-table"); |
| `tools/check-iface-hash.cx:165` | `lastn > 10` | 10 | - | - |  ck(rc == 0 && last && lastn > 10 && tx_prefix(last, (long)lastn, "IFACE_HASH\tok", 13), "checked-in-table-current"); |
| `tools/check-interface-digest.cx:54` | `n > 100` | 100 | - | - |  ck(run(dir, "--table", 0, &o, &n) == 0 && n > 100, "a.table-rc0"); { char -o2; size_t n2; run(dir, "--table", 0, &o2, &n2); ck(o && o2 && n == n2 && mm_cmp(o, o2, n) == 0, "a.deterministic"); } |
| `tools/check-own-flow.cx:157` | `n_rej >= 5` | 5 | - | - |  ck(n_rej >= 5 + 3 && n_ok >= 4 && n_unsafe == 1, "table keeps five reject, four OK and one UNSAFE row plus the real rows"); ck(n_real >= 3, "table keeps three real-defect rows r1..r3"); |
| `tools/check-perf-hot.cx:63` | `rows >= 15` | 15 | tests/cases/perf-hot/baseline-sample.tsv(20行) | 1.3x |  ck(rows >= 15 && found == rows, "fixture-rows-are-real-evidence-rows"); char in[4200]; |
| `tools/check-phase-times-calc.cx:88` | `sum1 - sim4 >= 34312` | 34312 | - | - |  ck(sim4 > 0 && sum1 - sim4 >= 34312, "c58-parallel-lanes-save-at-least-3431.2-of-8689.6"); sy_printf("NOTE\tc58\tserial_s=%ld.%ld\tcritical_s=%ld.%ld\tsim_width4_s=%ld.%ld\tsaved_s=%ld.%ld\n", sum1 / 10, sum1 % 10, cp / 10, cp % 10, |
| `tools/check-phase-times-calc.cx:125` | `ni >= 400` | 400 | build/chain-phases.tsv(499相) | 1.2x |  ck(ni >= 400 && lane_bad == 0, "c58-sim-same-lane-phases-never-overlap"); } |
| `tools/check-precommit-gates.cx:196` | `al > 10` | 10 | - | - |  ck(al > 10 && al == bl && tx_same(a, al, b, bl), "reuse.manifest-line-as-base"); } if (failed) sy_printf("%s\n", out); |
| `tools/check-qsort-intro.cx:78` | `total <(fail) 20` | 20 | - | - |  if (total < 20) nfail = nfail + 1; printf("QSORT_INTRO\t%s assertions=%ld failed=%ld prog_total=%ld prog_rc=%d\n", nfail ? "FAIL" : "PASS", npass + nfail, nfail, total, r.rc); |
| `tools/check-rechunk.cx:35` | `v >= 12` | 12 | - | - |  ck(v < 16 && v >= 12, "above-band-shrinks", v, 12); v = rc_next(131, 4, 4, 6000, 60, 1, 16); /- 10 % -> grow toward 60 % -/ |
| `tools/check-refusals-gen.cx:119` | `sy_refusal_count() >= 10` | 10 | exec/facts/refusals.tsv(10行) | 1.0x |  ck(sy_refusal_count() == SY_REFUSAL_COUNT - 1 && sy_refusal_count() >= 10, "linked-count"); ck(SY_REFUSAL_NONE == 0 && SY_REFUSAL_FS_FDATASYNC_WIN == 1 && SY_REFUSAL_FS_MKFIFO_OSX == 4 && SY_REFUSAL_MEM_PROT_WIN == 10, "linked-first |
| `tools/check-release-run.cx:259` | `oks >= 6` | 6 | - | - |  ck(oks >= 6, "R2b all layout assets OK (SHA256SUMS has 6 members)"); } |
| `tools/check-s9-p3-pins.cx:21` | `n > 16` | 16 | - | - |  ck(b&&n>16,"bad-copy-src"); if(b&&n>16){ b[n-1]^=1; fd=sy_open_new(badpkg,0600); |
| `tools/check-safedel-chains.cx:77` | `total <(fail) 20` | 20 | - | - |  if (total < 20) nfail = nfail + 1; printf("SAFEDEL_CHAINS\t%s assertions=%ld failed=%ld prog_total=%ld prog_rc=%d\n", nfail ? "FAIL" : "PASS", npass + nfail, nfail, total, r.rc); |
| `tools/check-status.cx:109` | `rows > 10` | 10 | tools/status.cx输出行数(约26行) | 2.6x |  ck(rows > 10 && rowsok, "every row has five tab-separated fields"); ck(lastis, "STATUS is the last line"); |
| `tools/check-stdio-buffer.cx:125` | `nw <=(fail) 20` | 20 | - | - |  else if (nw >= 1 && nw <= 20) { npass = npass + 1; printf("PASS\tmulti-object-writes %ld <= 20\n", nw); } else { nfail = nfail + 1; printf("FAIL\tmulti-object-writes %ld want 1..20\n", nw); } |
| `tools/check-tapebin.cx:21` | `oldn >= 136` | 136 | - | - |  check(old && oldn >= 136 && old[10] == 0 && old[11] == 0, "historical-minor0-fixture"); if (old && oldn >= 136) { |
| `tools/check-test-loader.cx:43` | `a.outn >= 31` | 31 | - | - |  char -direct[]={(char -)slice,"stream","e2","--models",(char -)models,"--level","2","--target","lnx/x86_64","--source",(char -)input,"--res",res,0};char -adapt[]={(char -)loader,"--slice",(char -)slice,"--models",(char -)models,"--e2-adapt |
| `tools/check-textx-isotime.cx:219` | `cnt >= 1000` | 1000 | - | - |  tk_ok(src && cnt >= 1000 && bad == 0, "jsdiff-format"); src = cx_read(pp, &n); cnt = 0; bad = 0; |
| `tools/check-textx-isotime.cx:235` | `cnt >= 200` | 200 | - | - |  tk_ok(src && cnt >= 200 && bad == 0, "jsdiff-parse"); } |
| `tools/check-textx-json-emit.cx:350` | `cnt >= 2000` | 2000 | tests/fixtures/textx-json-emit/numbers.tsv(3338行) | 1.6x |  tk_ok(src && cnt >= 2000 && bad == 0, "jsdiff-numbers"); src = cx_read(sp, &n); cnt = 0; bad = 0; |
| `tools/check-tlib-base.cx:105` | `ra >= 10` | 10 | build/tlib-packages.tsv(17包) | 1.7x |  int ok = ra == rb && ra >= 10 && ra < 60 && na == nb && na > 0 && oa && ob && mm_same(oa, ob, (long)na); if (!ok) sy_printf("NOTE\tprobe\tsource rc=%d bytes=%ld\ttlib rc=%d bytes=%ld\n", ra, (long)na, rb, (long)nb); |
| `tools/check-tlib-prep.cx:469` | `ln > 64` | 64 | - | - |  ck(lb && ln > 64 && !cx_write(path, kit, kn) && !cx_write(dst, lb, ln) && prep(0, k2, tool, other) == 3 && sy_exists(other) != 1, |
| `tools/check-tlib-prep.cx:367` | `bn > 64` | 64 | - | - |  ok = lb && bn > 64 && !cx_write(victim, lb, bn); mm_free(lb); |
| `tools/check-ujs-ape.cx:49` | `n > 1000000` | 1000000 | - | - |  ck(n > 1000000, "size-above-single-elf"); ck(n >= 2 && b[0] == 'M' && b[1] == 'Z', "mz-head"); |
| `tools/check-unit-frontcache.cx:613` | `total > 300000` | 300000 | - | - |  ck(total > 300000 && nnames(d, "uc-") == 3, "budget.unlimited-has-entries"); /- 预算 300000:小于总量,淘汰最旧的,留存不超预算 -/ |
| `tools/check-uxe-s0-exist.cx:438` | `gots >= 29` | 29 | - | - |  ck_record(&failed, gots >= 29, "nobow-sky"); printf("EL\tnobow-bow\tgot=%d\n", gotb); |
| `tools/check-uxe-s0-exist.cx:461` | `gots >= 29` | 29 | - | - |  ck_record(&failed, gots >= 29, "nocloud-sky"); printf("EL\tnocloud-cloud\tstd=%d\tdelta=%d\n", gstd, gotc); |
| `tools/check-uxe-s0-exist.cx:483` | `gots >= 29` | 29 | - | - |  ck_record(&failed, gots >= 29, "noreed-sky"); printf("EL\tnoreed-reed\tgot=%d\n", gotr); |
| `tools/check-uxe-s0-exist.cx:505` | `gots >= 29` | 29 | - | - |  ck_record(&failed, gots >= 29, "nowater-sky"); printf("EL\tnowater-water\tstd=%d\tdelta=%d\n", gstd, gotw); |
| `tools/check-uxe-s0-look.cx:201` | `nm >= 12` | 12 | - | - |  ck_record(&failed, nu == 3600 && nm >= 12, "red-fixture-unlike"); } |
| `tools/check-uxe-t1b.cx:35` | `lr > 80` | 80 | - | - |  ck_record(&failed, lr > 80 && lr < 180, "lin-mid"); if (out) free(out); |
| `tests/adopted-package.cx:26` | `r.own_physical_reads >= 8` | 8 | - | - |  ck(r.own_physical_reads>=8&&r.controller_endpoint_reads>0&&r.controller_post_endpoint_reads>0&&r.own_elapsed_ns>=r.snapshot_body_ns&&r.snapshot_body_ns>=r.controller_endpoint_ns&&r.controller_endpoint_ns>=r.ni_ns&&r.controller_endpoint_ns> |
| `tests/arm-assembler.cx:5` | `am_object_n-sh >= 320` | 320 | - | - |  if(!rc){unsigned long long sh=am_object_n>=64?at_rd(am_object+40,8):am_object_n;int ok=sh<=am_object_n&&am_object_n-sh>=320;unsigned long long off=ok?at_rd(am_object+sh+256+24,8):0,n=ok?at_rd(am_object+sh+256+32,8):0; |
| `tests/asm-layout.cx:4` | `am_object_n-sh >= 320` | 320 | - | - |  if(!rc){us_u64 sh=am_object_n>=64?at_rd(am_object+40,8):am_object_n;int ok=sh<=am_object_n&&am_object_n-sh>=320;us_u64 off=ok?at_rd(am_object+sh+256+24,8):0,n=ok?at_rd(am_object+sh+256+32,8):0; |
| `tests/check-stream-cells.cx:100` | `n >= 16` | 16 | - | - |  ok = b && n >= 16 && !memcmp(b, want, 16); free(b); |
| `tests/elf-arm64-image.cx:210` | `n >= 64` | 64 | - | - |  ck(n >= 64 && n <= (long)sizeof img, "image-size"); if (n >= 64 && n <= (long)sizeof img) { |
| `tests/elf-winargs.cx:156` | `sz > 64` | 64 | - | - |  ck(sz > 64 && sz <= 5120 + 512, "image-budget"); if (sz < 64 -- sz > (long)sizeof img) { sy_close(fd); die("size"); } |
| `tests/elf-x86-image.cx:191` | `n >= 64` | 64 | - | - |  ck(n >= 64 && n <= (long)sizeof img, "image-size"); if (n >= 64 && n <= (long)sizeof img) { |
| `tests/modelcache-budget.cx:87` | `st.mtime > 2000` | 2000 | - | - |  ck(sy_stat(pa, &st) == 0 && st.mtime > 2000, "hot-lookup-touches-mtime"); /- A real open reader owns the oldest victim while trim unlinks its name. -/ |
| `tests/modelcache-budget.cx:159` | `tot >= 398` | 398 | - | - |  ck(tot >= 398 && tot <= 597, "acct-grows-with-publish"); /- Over the limit of a small budget: full pass again, stale removed. -/ |
| `tests/objects.cx:89` | `n > 60` | 60 | - | - |  ck(n > 60+pn && get(obj,2) == (i == 2 ? 0x8664 : 0xaa64) && get(obj+2,2) == 1 && get(obj+16,2) == 0 && !memcmp(obj+20,".unisatp",8) && |
| `tests/release-version-pack.cx:27` | `n >= 1536` | 1536 | - | - |  ck(n>=1536&&image[0]=='M'&&image[1]=='Z',"PE magic");unsigned long p=get32(image+60);ck(p+24<=n&&!memcmp(image+p,"PE\0\0",4),"PE signature"); |
| `tests/release-version-pack.cx:55` | `n >= 18` | 18 | - | - |  ck(n>=18&&b[0]==31&&b[1]==139&&b[2]==8&&b[3]==0,"gzip header");long at=10,total=0;int last=0; while(!last){ck(at+5<=n,"stored header bounds");int h=b[at++];last=h&1;ck((h&254)==0,"stored block");long z=b[at]-b[at+1]<<8;long nz=b[at+2]-b[a |
| `tests/resources.cx:74` | `n > 5` | 5 | - | - |  size_t n;path(p,sizeof p,capdir,"resources.tsv");char -trace=load(p,&n);ck(n>5&&!memcmp(trace,"#SC2\t",5),"multi-SC2-identity");np_buf map={0};ck(!canonical(capdir,trace,n,&map),"multi-changed-reply-cannot-be-SC1-map");free(map.p);free(tra |
| `tests/resources.cx:86` | `tn > 5` | 5 | - | - |  size_t sn,tn;char -sym=load(symbols,&sn);/- Output contract: research/c43-late-binding-step0.md, empty Linux program requirements. -/static const unsigned char gold[12]={'U','N','I','S','Y','M','1','\n',1,0,0,0};ck(sn==sizeof gold&&!memcmp |
| `tests/resources.cx:284` | `n > 5` | 5 | - | - |  r=key_run(d,0,c,"m.c","cap-legacy");{size_t n;char -k=load(q,&n);ck(r.rc==0&&!r.errn&&r.outn>0&&n>5&&tx_prefix(k,(long)n,"#SC2\t",5),"capture-legacy-empty-ledger-accepted");mm_free(k);}np_free(&r); |
| `tests/resources.cx:343` | `ledger_n > 5` | 5 | - | - |  if(unsetenv("UNISACC_RF_CAPTURE"))die("clear inherited capture");if(setenv("UNISA_JOBS","1",1))die("serial resource jobs");if(output_mode){trace_output_batch();return finish(before,container_before,"SC2-output-fail-closed-SC1-metadata-and- |
| `tests/ri-image-multi.cx:60` | `n > 10` | 10 | - | - |  {size_t n;char -s=cx_read(ledger,&n);check(s&&n>10&&!memcmp(s,"#SC2\tstage=source:",18),"multi.header-is-source-not-e3");free(s);} check(hex_key_row(ledger,k_kern,sizeof k_kern-1),"multi.frontend-kernel-bound"); |
| `tests/ri-image-pem.cx:103` | `n > 5` | 5 | - | - |  {size_t n;char -s=cx_read(ledger,&n);ck(!!s&&n>5&&!memcmp(s,"#SC2\t",5),"pem.capture-SC2");free(s);} ck(!missing(e3id),"pem.capture-e3-file"); |

## (4) 高价值改造候选与治理建议

- (1) 关键门与构造链 `TEXT_EXISTENCE` 检查改造为行为探针:
  - `tools/check-chain-argv-timeout.cx` (37 行): 当前仅 `cx_read` 读取 `tools/source-job-wave.cx` 与 `tools/chain-step.cx` 数 `wave_secs`/`wave_bs`/`timed_out` token 出现次数(头注明写只读源码不编译不运行)。改造建议: 构造假超时子作业夹具实跑 `source-job-wave` 与 `chain-step`, 断言超时退出码 `rc=124` 且进程组无残留。
  - `tools/check-source-job-count.cx` (143 行) 与 `tools/check-sourcepackage-pins.cx` (225 行): 当前仅用 `ca_tokens` 或 `strstr(src, "static const SpModel recipes[]")` 扫 `tools/source-package.cx`、`tools/source-job-wave.cx`、`tools/cxlib/sourcepackage.cx` 源码文本。改造建议: 增加调用 `source-package --list` / `source-job-wave --dry-run` 或进程内调用 `sourcepackage` 校验入口的真实行为探针, 并保留 1 个篡改 pin 必红的负控夹具。
  - `tools/check-fake-gh-contract.cx` (101 行): 当前通过在 `tools/cxlib/releaseexec.cx`、`tools/ci-verdict.cx`、`tools/post-publish-check.cx` 源码中 `strstr` 查找硬编码 C 代码片段 (如 `v[n++] = "--draft=false"`) 核对 `gh` 调用点。一旦代码格式化或改用辅助函数拼参数即假红, 而参数顺序写错却可能漏检。改造建议: 在私有夹具下以录制模式驱动 `release-exec --dry` / `ci-verdict` 走过 `fake-gh.cx`, 断言实际发出的 `argv` 序列与 tape 精确匹配。
  - `tools/check-batch-split.cx` (73 行) 与 `tools/check-run-uxe-regress.cx` (29 行): 当前仅 `cx_read`/`fopen` 读源码 `strstr` 查找测试名字符串。改造建议: 调用被测拆分入口 (`--list` 或 `--cases`) 核对实际注册与可执行的子用例集合等于分片表。
- (2) 疑似永不失败阈值断言 (`实际/常量 > 3x`) 改造为精确相等或单调棘轮:
  - 全仓批次数断言 (`tools/check-affected.cx:704,721,765,790,794,836` 的 `>= 100` vs 实际 `1883-1909` 行, 余量比 `19.0x`; `tools/check-dcheck.cx:117` 的 `>= 10`): 当 90% 批次因解析错误被静默丢弃时 `>= 100` 依然绿。应改为对照 `tests/batches.tsv` 实测 `IMPLEMENTED` 行数或与未过滤全集做精确差集核对 (`selected + excluded == total_implemented`)。
  - 整链相位表断言 (`tools/check-candidate-chain.cx:122` 的 `rows >= 100` vs `build/chain-phases.tsv` 实际 `499` 行, 余量比 `4.9x`; `tools/check-phase-times-calc.cx:124` 的 `rows >= 100`): 应改为读取 `build/chain-phases.tsv` 的实际数据行数断言 `rows == expected_table_rows` (499)。
  - 全仓文件扫描覆盖数断言 (`tools/check-boundenv.cx:103` 的 `scanned > 100`、`tools/check-cx-cond-use.cx:123` 的 `cu_files > 100` vs 全仓 `tools+tests` 实际 `1491` 个 `.cx` 文件, 余量比 `14.9x`): 目录遍历若漏扫 `tools/` 甚至只扫了 `cx/` 也会大于 100。应改为断言每个目标子目录 (`src/`、`cx/`、`ext/`、`tools/`、`tests/`) 均非空且总扫描数不低于已跟踪 `.cx` 文件数。
  - 表非空粗略字节阈值 (`tools/check-gen-header-idents.cx:48` 的 `n > 1000` vs 实际 `180160` 字节, 余量比 `180.1x`): 应改为解析表头并断言已解析有效行数与关键必含符号存在。
