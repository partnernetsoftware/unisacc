# M6: v0.0.43 plan item 4, baseline recheck (draft, 2026-10-23)

Source table: `archive/plans/v0.0.43-baseline-20261014.md` (the item-4 table plus its "后续" list).
Scope: roadmap M6 (`research/c43-roadmap-0043-and-refactor.md` A.1) under ruling R10 (`plans/v0.0.43.md` line 9).
This is a draft only. Nothing in the repository was edited.

## Subject under test

- **Tree:** HEAD `b7fea757`. Another workflow was committing during the run (tests/gate.sh and several tests/*.cx changed mid-run), so job counts are a single time point.
- **Product:** root `./unisacc.com`, sha256 `3b17e191a8c6d2e3…`, 2526333 B.
  - Provenance check, 实际跑过: `scratchpad/m9/seedbin/ident . check unisacc.com` printed `product freshness: verified 3b17e191…`, rc=0.
- **Classic reference:** `/tmp/ua_ref`, sha256 `94a09cf0eecbffc7…`.
- **Host:** 16-core Linux x86_64. This is the H1 baseline per R10.
  - Load average was 10–17 during the run because of the concurrent workflow.
- **Workspace:** all probes and logs are in `…/scratchpad/closing/item4-m6/w/`.

### Label mapping

The roadmap uses the labels P4a, P4c and P4e. Their mapping is 只读推断, taken from the order of plan item 4 ("数组指针成员、Windows库模式合同、十六进制/超时债…"):

| Label | Item |
|---|---|
| P4a | Array-pointer member |
| P4b | Windows library-mode contract (L-4) |
| P4c | Hex |
| P4d | Timeout debt |
| P4e | `-include` position |

The remaining rows are numbered by their order in the baseline table (B6 and up).

## Updated baseline table

### P4a: array-pointer member store miscompile (`unsigned char (*dl)[8]` member)

**Status: FIXED on product and classic.** The probe has not been moved into tests/c yet.

Evidence, 实际跑过: `tests/c/candidates/array-pointer-member-store.c` was run four ways.
- Product `-run`: rc0, "pass".
- Product `-o` then executing the binary: rc0, "pass".
- `/tmp/ua_ref`: rc0, "pass".
- `gcc`: rc0, "pass".
- The product and classic `-S` tapes are byte-equal.
- The sibling probe `array-pointer-arith.c` gives the same result, and its tapes are also byte-equal.

Fix commits (git log on the research note): 3f952fcb (case no longer reproduces), e02d924a, 726dfec2.

Remaining work is M6's own: move both probes from `tests/c/candidates/` into `tests/c/`. Once there, they join exec-chain, ccrun and e4 automatically through the glob at `gate.sh:85/263/354`.
- Today neither probe is referenced by any gate or check. 实际跑过: grep finds them only in plan, research and archive files.

### P4b: Windows `.hostcall`/`.hostaddr` library-mode contract (L-4)

**Status: STILL OPEN, deferred by R10** (A.2: L-4 moves to 0.0.44). M8 must say that Windows acceptance is not counted green.

Evidence, 实际跑过. Two tapes were used: `.hostaddr r0, 2` and `imm; imm; .hostcall r1, r0`.

| Target | Product | Python reference | Classic `/tmp/ua_ref` |
|---|---|---|---|
| win/x86_64 | rc0, image written | rc1, `ValueError: foreign host ABI is only supported on osx or lnx` | rc0, image byte-equal to the product's |
| lnx/x86_64 | rc0 | rc0 | not run |
| osx/x86_64 | rc0 | rc0 | not run |

- On lnx and osx, the product and Python reference images are byte-equal.
- No explicit mode, target or capability input exists yet. The two references disagree on the Windows case, so this is exactly the "false parity" that L-4 forbids (`spec.md:831`).
- No Windows native execution is possible on this host.

### P4c: hex immediates (`imm r2, 0x1f`)

**Status: STILL OPEN, deferred by R10** (A.2: hex-A′). This is a named refusal, not wrong code.

Evidence, 实际跑过:

| Route | Result |
|---|---|
| Product `-S` | rc0; the tape is echoed unchanged, so the Tape domain accepts it |
| Product `-c` | rc0 |
| Product `-run` | rc1, empty stdout, no file; `reject: not covered: an operand that is not a register or an integer` |
| Product `-b lnx/x86_64 -o` | same refusal as `-run` |
| Classic | rc0 |
| Python reference | rc0; image byte-equal to the classic image |

The baseline text still holds: the refusal comes from the back end, and the references accept.

### P4d: structural timeout debt (`exec-driver-core-modes` cold, the selfcheck family, and others)

**Status: STILL OPEN, deferred by R10** (A.2 "超时债余项"). It blocks only through M7, if a gate job goes red.

Evidence, 实际跑过: `timeout 58 tests/bound 58 python3 ./tests/tapebin.py structure-1/2` returned rc=124 at 58 s, with load 17.3. The private reference build alone took 18.1 s.

Sibling evidence, read-only, from `closing/timeout-debt/m/tapebin.log`: structure-1/2 rc0 took 213.4 s. Splitting jobs and per-job timings are owned by the timeout-debt role.

`com-tapebin` was already split by 6420cfb3 (that is the baseline's own record; it was not re-run here).

### P4e: wrong position attribution inside `-include` headers

**Status: FIXED and gated.** M6 should record it as fixed.

Evidence, 实际跑过:
- `-include inc.h` with `__FILE__`/`__LINE__`: product and classic both print `inc.h 1 g.c` (gcc prints `./inc.h 1 g.c`).
- A syntax error inside `-include bad.h`: product and classic both report `bad.h:1:9: error: …`.
- `com-forceinclude` (`forceincludecheck.cx`) on the product: rc0, 11 checks PASS, 10 s.

Fix commits: 9ed67c03 and e7c05741. Gated by b1ae536e.

### B6: library mode `library/bindings` and `exec/lower/codeprivate-manifest.tsv` still carry host addresses

**Status: PARTLY FIXED; the rest is STILL OPEN and deferred by R10** (A.2: 运行桥迁移 and the 2c2 debts move to 0.0.44).

Ordinary fixed-signature functions, 实际跑过 with the scratch probe `w/lib/addrprobe.c`:
- The probe uses the public `us_add_symbol` API on two contexts whose host addresses differ.
- The E3 tapes are byte-equal (510 B), with no address present (checked in both LE64 and decimal form).
- `binding_blob` is empty (0 B). Ordinary functions therefore no longer use `library/bindings`.

Still embedding host addresses, 只读推断:
- `libunisacc.c:1143-1145` passes `library_exit`, mmap and munmap, the four `process/dl` slots and the callable make/call dispatch to lower as scalar resources.
- `libunisacc.c:709` freezes typed, variadic and callback bindings with the bridge dispatch addresses.
- The `codeprivate-manifest.tsv` `LM.private` rows still emit `setreg r1, imm <address>`.
- `spec.md:835` (LB-1) still says "library/bindings/codeprivate嵌地址尚未迁移".

Gate leaves:
- `com-latebind` rc0, `com-bindings-neg` 12/0, `com-hostaddr-neg` 10/0, `com-extent-neg` 11/0 (all 实际跑过).
- `lib-latebind` and `lib-latebind-W` initially failed on this host because of the host only. Both were then re-run, see below.

Host finding, 实际跑过: two separate host problems stopped the library checks from building on stock glibc.
1. `ffi.h` is missing (already listed in B1).
2. With the header present, `exec/c/libraryresolver.h:183` uses `RTLD_DEFAULT`, and glibc only defines it under `_GNU_SOURCE`. The checks build with `cc -std=c11` and no `-D_GNU_SOURCE` (`liblatebindcheck.cx:18`).

The second problem is a portability gap in the test build line on Linux, not a product defect. It should be added to the B1 host recipe, or the build line should gain `-D_GNU_SOURCE` (that change would touch tests/ only).

With the extracted libffi on `CPATH`/`LIBRARY_PATH` and a cc wrapper adding `-D_GNU_SOURCE`:
- `lib-latebind`: rc0 ("library contract holds", 2c1 0 failures).
- `lib-latebind-W`: rc0 (67 assertions, 0 failures).

### B7: underscore integers (`1_000`): the Tape domain refused them while the reference accepted them

**Status: FIXED** (the references were aligned to C). Two pieces of documentation are still stale.

Evidence, 实际跑过, using `imm r2, 1_000`:
- Product `-S`: rc1, no file, `tape domain line 2: invalid integer literal`.
- Classic: rc1 (`back end: malformed tape`).
- Python reference: rc1, `line 2: invalid integer literal`.
- The four cases in `tests/tapecheck-cases.tsv` are now `invalid-underscore*` (lines 420 and 444–446).
- `com-tapedomain`: rc0, 220 case-modes, 0 failures.

Fix commit: c08a1c00.

Stale text:
- `exec/tapecheck/README.md:73-77` still describes "registered-underscore… reference acceptance, model `not covered`". This contradicts line 21 of the same file.
- `spec.md` TP-9 (line 829) has no underscore sentence. grep for "下划线|underscore" finds nothing in spec.md. This is the B2/M2 documentation item.

### B8: `-iquote` not migrated

**Status: FIXED** (926a8925).
- `com-iquote` (`iquotecheck.cx`) on the product: rc0, 14 checks PASS (实际跑过).

### B9: gate job count basis (baseline 748/749)

**Status: INFORMATIONAL, re-measured.**
- `tests/gate.sh --list --com | wc -l` returns 1051 at `b7fea757` plus the concurrent in-flight edits; there are no duplicate names (实际跑过).
- `--plan` output is NUL-separated, so it was not counted.
- The rule stands: state the `--plan` total at the time of closing (M7).

### B10: `seedmemory` red because evidence keys go stale

**Status: still RESOLVED.**
- `python3 tests/seedmemorycheck.py`: rc0, "OK", 7 s (实际跑过).

### B11: `pp-leaf-seal` and its negative

**Status: GREEN now.**
- `ppleafseal.cx --check-core`: rc0 (93 dependencies), 8 s.
- `--negative`: rc0 (PASS), 13 s.
- Both 实际跑过.

### B12: contract-layer suites were not in the commit gate

**Status: still RESOLVED.**
- `release/derived-map.tsv` has 401 lines and is referenced by `tests/commitgatecheck.sh`, `gateselect.py` and `refresh_gatedeps.py` (只读推断 from grep).
- `abi-manifest --check`: rc0, 4 s (实际跑过).
- `facts-export --check`: rc0, 1 s (实际跑过).

### Follow-up: illegal `.hostaddr` index negative

**Status: DONE and gated.** `com-hostaddr-neg`: rc0, 10 checks, 0 bad (实际跑过).

### Follow-up: tighten the `libraryimportscheck.py` refusal assertion

**Status: DONE per the plan** (2026-10-22). It is still not gated.
- Not re-run here, because it needs a candidate `lower.json` (只读推断 from `plans/v0.0.43.md` "后续").

### Tape-domain acceptance conditions 1–6 (plan item 3, kept as contract)

**Status: HOLD.** `com-tapedomain`: rc0, 220 case-modes, 0 failures, 13 s (实际跑过).

## Proposed M6 edits (for the main session; not applied)

1. **P4a:** `git mv tests/c/candidates/array-pointer-member-store.c` and `array-pointer-arith.c` to `tests/c/`, using an `a_`/`b_` prefix that follows local naming.
   - Then rerun exec-chain and ccrun on those two files.
   - Mark the baseline row "已修（3f952fcb/e02d924a/726dfec2），探针入 tests/c".
   - The driver workaround ecdbd98d can stay; it is not a correctness issue.
2. **P4e:** record it as fixed: 9ed67c03 and e7c05741, gate leaf `com-forceinclude`.
3. **P4c, P4b and the B6 remainder:** mark them "推迟 0.0.44 (R10 A.2)".
   - The M8 release notes must carry: "win accept ≠ green"; hex-A′ is refused with a name; the bridge, typed, variadic and callback paths still feed host addresses to lower.
4. **P4d:** "推迟 (R10)", with the cross-reference to the timeout-debt role's numbers.
5. **B7:**
   - Add the TP-9 underscore sentence to `spec.md` (M2).
   - Fix the stale `exec/tapecheck/README.md:73-77` paragraph.
6. **New host-recipe line for B1:** library-mode checks on glibc need `-D_GNU_SOURCE` (for `RTLD_DEFAULT`) in addition to libffi-dev.
   - Alternatively, add `-D_GNU_SOURCE` to the cc line in `tests/liblatebind*check.cx` (tests/ only, no closure change).

## Not verified

- Windows native execution of any `.hostcall` image.
- exec-chain and ccrun for the P4a probes inside the gate harness; only the direct four-way run and the tape comparison were done.
- `libraryimportscheck.py`.
- The `--plan` job total.
- `exec-driver-core-modes` and the selfcheck family in a cold run (left to the timeout-debt role).
- Why the variadic `us_add_symbol` probe was refused with `not covered: library import binding or signature`. The probe's signature may simply be malformed; no claim is made.
