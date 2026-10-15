# 2c2 typed E3 reader: measured boundary

This is the reader commit only. The shared change is line 5 of
`exec/parse2/libraryimports-manifest.tsv`; there is no terminal wrapper overlay,
no W/runtime edit, no public API edit and no product fresh/seal/release claim.
The opt-in constructor is `parse2/librarytyped --librarytyped`; the default E3
constructor remains byte-identical. The old wrapper dispatch literal is zero:
this model-only witness must not be executed as a typed program.

## Construction and narrow witness (actually run)

Source: clean export of main `24bde40213437bcde300a57c92f5b5069792cb01` plus
only this reader change. Submission base is 011108d7 (the intervening change
only edits prd.md, outside constructor/ABI/readset inputs). `tests/seals/librarytypedreader/receipt.json` records
both base/off artifact paths, lengths and SHA256 separately, exact commands,
constructor/observer identity and source-file SHA256. The base copy restores the
original reader row in its own temporary exec directory; the repository is not
mutated during construction. JSON (10131 states), table and network are equal.
Enabled locations construction is C/P byte-identical (10371 states).

The named dynamic-C test passed: 5 accepts, 10 named refusals, 60 table/network/A/B
observations. Complete E1 USLFACT1+UNITOK frames and explicit attributes are frozen
in the cases. Used, unused and source-defined controls are separate; source
prototype conflict and invalid count/version/variadic/owner/mode/name cases refuse.
Static review caught owner2 V2 being validated as the new class before ownership.
The new typed table now dispatches by owner first; legacy V2/variadic controls pass,
new-owner old-category and legacy-owner fixed-V3 claims refuse without downgrade.
The fixed-two-int Python oracle is observation only; it does not parse arbitrary C
or qualify arbitrary ABI graphs. The C constructor's parse2base check-hook negative
returns rc1 and `defined twice` in both C and Python. Timeout/signal/tool failure
is classified separately from domain refusal.

Preparation originally hit 58 seconds while the dynamic-C helper repeatedly
hashed two 61MB JSONs. The helper now compares complete bytes; sha256sum records
both sides independently. The same 58-second limits are retained; preparation is
run as separately bounded tools/base/typed steps before the named job.

## Seed read set, RSS and job results (actually run)

`tests/seals/librarytypedreader/seed-memory.json` binds 30 existing gen routes plus
six formerly unmeasured parse2 flag routes, direct wait4 C RSS, C/P output hashes,
compiler flags/binary identity, source hashes and raw trace hashes. All 36 pairs
are byte-identical. Default/errors plus six additional flags cover all eight
parse2 combinations. Python source traces were repeated with pyc removed and -B;
no old read dependency was removed because of a cache hit. The gen set is 779
files, parse2 is 380; the latter now includes the warning tables actually read by
the newly measured flags. C memory values retain the one-MiB guard and admission
retains four-worker, environment identity and warm-reference requirements. Cold
Python references, unknown flags and COM construction remain UNKNOWN.

- seedgen: rc0, SAME 15 / DIFF 0 (includes ident/blob).
- seedgen-2: rc0, SAME 10 / DIFF 0.
- seedgen-3: rc0, SAME 3 / DIFF 0.
- seedgen-4: rc0, SAME 4 / DIFF 0 (initially memory rc77; after available memory recovered, the original job passed unchanged).
- seedparse2-1: suite rc77 UNVERIFIED; required 8011120640 bytes, observed 6547447808.
- seedparse2-2: suite rc77 UNVERIFIED; required 10218373120 bytes, observed 6536024064.

The last two jobs were NOT verified in this commit; before release they must be
rerun on a host with enough memory. Their gate aggregate rc is 4. Serial route
receipts are substitute evidence, NOT a claim that these jobs passed. No memory
margin, parallelism or timeout was lowered. seedmemorycheck passed 17 tests,
including measured flags requiring actual warm files and an unknown flag refusing
admission.

## Not proved here

Tail overlay, actual E3/lower ownership-consumption linkage, libraryfn closure,
provider signature comparison, compile-time-empty native plans and P1–P6 remain
for subsequent commits. P6 warehouse data is not registered or copied into this
commit. Full arbitrary-input reference equivalence is not claimed. The S probe,
old queue/env-pin and release surfaces are untouched.

## Cold preparation follow-up (after c9f4d792)

Review found the former combined `--prepare-base` exceeded its 58-second cold
window. It now refuses with rc2 and names seven explicit preparation steps;
limits and verdicts are unchanged. New script content selects a new cache key.
After the first successful run, all seven base artifacts and all base completion
SHA files were explicitly removed; the following cold replay used external
`time.monotonic()` measurements (each command also bound to 58 seconds):

| Step | rc | Wall seconds |
|---|---:|---:|
| c | 0 | 4.23 |
| off | 0 | 19.65 |
| reference | 0 | 20.27 |
| tables-base | 0 | 2.37 |
| tables-off | 0 | 3.72 |
| net-base | 0 | 0.31 |
| net-off | 0 | 1.97 |

Tools: 6.58s; typed preparation: 26.29s, both rc0 on the new content key.
Each step verifies its prerequisite artifact hashes. Only final network equality
publishes the complete base receipt; absent/corrupt preparation remains rc2.
The original seedparse2-1/-2 UNVERIFIED publication obligations are unchanged.
ABI regenerated on clean HEAD plus these files: 447 entries, unchanged bytes.
Raw per-step logs and external timings are observation-layer warehouse evidence
(`reader-prep-split/cold-results.json`); physical paths are not core identity.
