# 0.0.42 single ruling request (FINAL, from the finished gate run /tmp/c42-7000)

Source: `python3 -I release/tools/exittable.py /tmp/c42-7000/cand.queue --h1 archive/plans/v0.0.38.md`. 722/722, final rc=1 (rc0 632, rc1 30, rc2 7, rc77 14, rc142 39). Exit table: PASS 632; BLOCKED 28; HOST_TIMEOUT 11; NEEDS_RULING 9; RESOURCE_UNKNOWN 7; RULED_BASELINE 21; UNVERIFIED_HOST 14. Candidate `unisacc.com` sha256 `ba11d636...`, sealed (`release/candidate.json`, GHCR `9da796385d67...`), gate worktree `f871dbaf`. This replaces the earlier draft built on run 3000. Bucket record: `research/c42-7000-final-bucket.md`; remote evidence: `research/c42-remote-evidence.md`.

## 1. What is asked, and what is not

Asked: rule the groups in section 2 once, and answer the release-criterion question in section 4.
Not asked, not done: no acceptance wording is changed, no limit or jobs count is raised, no test is deleted or skipped. A classification is never a pass; every class below stays outside PASS and its obligation stays open. Bypass solo runs (the gate's own worktree, same candidate, results not written to the gate state) are attribution evidence and do not replace the gate record. `release/preauth.tsv` and `release/rulings.tsv` were not edited (section 5 is a proposal only). No product red: `lib-lifecycle`, `com-multi`, `com-forward-multi` are rc 0.

Host property: the cloud host suspends when the driving session is idle (uptime stops); about 100 of the gate's roughly 293 wall minutes were such pauses, and one solo run reported 421 s for a suite that takes 10-11 s. This may also explain in-gate timeouts. It is not proven to be the cause of any specific one.

## 2. Groups needing a decision

| # | Suites | rc / count | Gate-log signature (limit, seconds in gate) | rulings.tsv row | Bypass solo (quoted) | Question |
|---|---|---|---|---|---|---|
| A | com-declmatrix-1..4 | 142 / 4 | last line `refused  char.zg3 ... printf("zg3 %d\n", (int)sizeof p3[0][0][0]);` (-3, -4: empty); limits 13-20 s, cut at 15-22 s | none | `com-declmatrix-1 rc=0 11s :: declmatrix ok 68 wrong 0 refused-by-name 5`; -2 and -3 `rc=0 10s ... ok 62 wrong 0 refused-by-name 1`; -4 `rc=0 421s ... ok 62 wrong 0 refused-by-name 1` | New timeouts, all passed in earlier rounds. Load/host pause, or a real slowdown? |
| B | seedgen-optconsumer-2, -3, seedgen-pruneconsumer-1 | 142 / 3 | `seedoptc  tree f871dbaf...` (pruneconsumer: `SAME cc shim refuses an unknown source`); limit 43 s, cut at 48.2 s | none | `optconsumer-2 rc=0 21s :: seedoptc slice 2 same 4 bad 0`; `-3 rc=0 25s ... slice 3 same 4 bad 0`; `pruneconsumer-1 rc=0 12s :: seedprunec slice 1 same 10 bad 0` | New timeouts, solo well inside limit. |
| C | seedgen-ppconsumer-1 | 142 / 1 | `SAME NEG-badsg prepare stops rc 2 before lower, no e2.json, no Python fallback`; solo retry also 142 at 43.08 s (limit 43) | none | `rc=0 49s :: seedppc shard 1 same 22 bad 0` (49 s is above its 43 s limit) | Timed out in a prior run too; over the limit even solo. Genuine size problem or load? |
| D | rowcov-lower-3 (root of 1 BLOCKED: rowcov-lower-union) | 142 / 1 (+1 blocked) | empty log; limit 42 s, cut at 47.25 s | none | `rowcov-lower-3 rc=0 34s :: rowcov probe set of this shard changed since its baseline: the union decides; new line: lower/3/8 399 f62138cfd9d1` (second run, `solo2-`; the first solo in the main checkout lacks the build cache and stopped without a result line); `rowcov-lower-union rc=0 3s :: rowcov lower above the baseline 450` | Timeout in a new family. The union ran only in the bypass. |
| E | seedgen, seedgen-2..4, seedparse2-1/-2, com-seedgen | 2 / 7 | `UNKNOWN: required=memory observed=unknown missing=memory-evidence reason=unmeasured cache branch: cold: e2` (seedgen-2 `cold: lower-lnx-x`, -3 `cold: tokenpp`, -4 `cold: warnparse`; seedparse2-1 `unmeasured C flags: locations`, -2 `locations,warnings`; com-seedgen `unmeasured COM construction/generator route`) | none (preauth class RESOURCE_UNKNOWN: rule once for the group) | n/a; memory was re-measured this cycle (`research/c39-a5-gen-memory.md`, dated section) with a documented protocol deviation | UNKNOWN by design, asserted by `tests/seedmemorycheck.py`. Record once as unexecuted? |
| F | ccinterop, syscall6, hosthdr, elfobj, com-elfobj, forward, com-forward, windows-resolver-host | 77 / 8 | `UNVERIFIED: required=... missing=...` (Lima guest, zig, macOS ld64/host) | pre-authorised class | n/a | Eight obligations still open. Acceptable for Draft? |

Already settled, listed for transparency only: HOST_TIMEOUT 11 (closure-c1..4, stages-1/2, exec-e5, exec-f1-attributes, lib-callable-variadic-model, shared-e2-plain, tools-2; `tools-2` solo `rc=0 45s ... pass 4 wrong 0`), RULED_BASELINE 21, BLOCKED 28 (counted under their roots), and UNVERIFIED_HOST 6 closed by darwin-gate run 38387935866 (exec-bootstrap-osxarm/osxx86, lib-stack-arm/x86/x86-hostabi, lib-windows-gp-native).

## 3. Options and recommendation

Allowed options only: record against an H1/H2 baseline; gather named evidence; fix in a named later version; block. None is a pass.

| Group | Options and consequence | Recommended (one line) |
|---|---|---|
| A, B | (1) H1 TIMEOUT baseline rows, limits unchanged, failure kept on record. (2) Re-run the gate on a quiet host: not possible in this window (cost: another full run). (3) Block. | (1): every solo run is rc 0 with wrong 0 and inside the limit, so the evidence points to load, but the gate record stays red. |
| C | (1) H1 baseline row. (2) Split the NEG cases into a second shard in 0.0.43. (3) Block. | (1) plus (2): solo is 49 s against a 43 s limit, so this one is genuinely over the limit and needs the split. |
| D | (1) H1 baseline row for rowcov-lower-3; lower-union stays BLOCKED/unexecuted in the gate. (2) Fix with the 0.0.43 rowcov work. (3) Block. | (1) plus (2): consistent with the existing rowcov H1 rows; the coverage obligation stays open and named. |
| E | (1) Record once as unexecuted, evidence missing. (2) Measure the cold branches and flag sets with a new commit of constants. (3) Block. | (1) plus (2) in 0.0.43: the suite refuses to start rather than guess; nothing wrong is hidden. |
| F | (1) Accept as UNVERIFIED with a named closing source, Draft only. (2) Close first: Lima arm64 guest, zig, macOS runner/host. (3) Block Draft. | (1) for Draft; (2) required before Latest, which needs separate authorisation. |

## 4. Question for the owner: the release criterion

Section 10 requires the local queue to end rc=0. This Linux cloud host cannot reach that: rc 77 host cells (14, of which 8 remain) and H1 timeouts. The only defensible basis for a Draft is: (a) exit table fully classified (632 PASS, 0 unclassified), no product red; (b) release-check run 38374765471, all six candidate cells green on the sealed bytes; (c) darwin-gate run 38387935866, six Darwin-host suites rc=0.
Still unverified if a Draft opens on that basis: the 8 group F obligations; the 7 memory-evidence suites; rowcov (parse2 24 + union, enc union, lower-union) and com-luatests-build dependants unexecuted; the 9 NEEDS_RULING suites have no in-gate pass; no local rc=0 exists. Owner decision requested: may a Draft be opened on this basis, with Latest still under separate authorisation?

## 5. PROPOSED rows for release/rulings.tsv (not applied; format: suite, kind, signature, baseline, ruling)

```
# PROPOSED, NOT APPLIED -- c42-7000 single ruling (pending owner decision)
com-declmatrix-1	TIMEOUT		H1	PROPOSED c42-7000 group A (solo rc0; gate cut under host load; limit unchanged)
com-declmatrix-2	TIMEOUT		H1	PROPOSED c42-7000 group A
com-declmatrix-3	TIMEOUT		H1	PROPOSED c42-7000 group A
com-declmatrix-4	TIMEOUT		H1	PROPOSED c42-7000 group A
seedgen-optconsumer-2	TIMEOUT		H1	PROPOSED c42-7000 group B
seedgen-optconsumer-3	TIMEOUT		H1	PROPOSED c42-7000 group B
seedgen-pruneconsumer-1	TIMEOUT		H1	PROPOSED c42-7000 group B
seedgen-ppconsumer-1	TIMEOUT		H1	PROPOSED c42-7000 group C (solo 49 s > 43 s limit; split in 0.0.43)
rowcov-lower-3	TIMEOUT		H1	PROPOSED c42-7000 group D (union stays BLOCKED)
```

## Mismatches against the stated facts

declmatrix-4 solo reports 421 s, not 10-11 s (the suspend-affected run; declmatrix-1/-2/-3 are 11/10/10 s). The first `solo-rowcov-lower-3.log` has no result line; only `solo2-rowcov-lower-3.log` (34 s, rc 0) counts. All other facts match.
