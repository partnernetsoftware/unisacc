# Batch A level=product Receipt Prerequisites (m25.o, modelnumber.o, driveargs.o)

To promote a Batch A unit from `level=table` (`PENDING`) to `level=product` (`ADMIT`) with a valid `build/admission-receipts/<unit>.rcpt`, all of the following are required:

1. **Common Prerequisites (all 3 units)**:
   - Private relinked product binary (or `priv-relink` candidate APE) executing the migrated unit through the shared TapeRuntime / thin host call table (`TapeUnitSpec`).
   - Counting-stub measurement on the real product binary recording `calls_per_run` and `steps_per_call` (plus `elapsed_ms`), deriving `cost_us_per_run = calls * (10.68 + steps * 0.00177)` from `plans/migration-cost-model.tsv`.
   - `shape-scan` output confirming `blockers_n = 0`.
   - Receipt v2 (`rc_write2` / `rc_fresh`-valid `binding_sha`, 64-hex `candidate_sha` of the real product binary, `unit`, `level=product`, `golden_pass`, `calls_per_run`, `steps_per_call`, `blockers_n`).

2. **`m25.o` (`cx/compiler/rtsharedname.cx`)**:
   - `level=table` already green (`tools/check-golden-m25.cx`, 177/177 `enum-full` vs `exec/facts/rt-shared.tsv`).
   - Remaining: link the migrated `m25` table/tape into a private product binary, run `check-golden-m25` at `level=product` against that binary, and record counting-stub `calls_per_run` / `steps_per_call`.

3. **`modelnumber.o` (`cx/runtime/model_number.cx`)**:
   - `level=table` already green (`tools/check-golden-modelnumber.cx`, 5,256/5,256 `byte-diff`).
   - Remaining: migrate `cxmn_hex` / `cxmn_tag` / `cxmn_budget` / `cxmn_number` to tape/table in a private product binary, run all 5,256 cases at `level=product` against the native oracle, and record counting-stub metrics.

4. **`driveargs.o` (`src/driveargs.cx`)**:
   - Remaining: land `tools/check-golden-driveargs.cx` (`48` `DA_TAPE_GOLDEN` cases + seeded corpus) in `tools/`, migrate `dv_word` / `dv_parse_args` in a private product binary, run `level=product` byte-diff comparison, and record counting-stub metrics.
