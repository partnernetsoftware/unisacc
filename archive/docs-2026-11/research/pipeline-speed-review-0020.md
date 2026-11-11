# Release pipeline speed review (0.0.20)

**Status: design only.** This analyses the measurements in the brief. Nothing was run and the repository was not read.

## 1. Critical path and avoidable cost

The freeze-to-publish run took about 115 min. The critical path, in order:

| Segment | Minutes | Kind |
|---|---|---|
| Freeze, ref build, candidate build | 3 | inherent (2 min of retry is avoidable) |
| Bootstrap stage2/3 (2-3 bounded attempts each) | 7 | about half avoidable (lost to restart overhead) |
| Seal + manifest + push | 1 | inherent |
| Queue #1 | 26 | inherent cost about 15 at 4 jobs; about 11 lost to packing/window overhead |
| Rebuild + bootstrap + reseal after 1 red | 20 | **rework** (the check takes 0.5 s and could run before freeze) |
| Queue #2 + stale-metadata continuation | 27 | **rework** (most suites had unchanged inputs) |
| Windows VM (boot 4 + skipped + manual rerun + 8 work) | about 15, partly serial | about 5 rework |
| Linux VM (564 s false reds + 672 s rerun + 3 min flake reruns) | about 23, partly serial | about 12 rework (concurrency with the Windows VM) |
| Apple notarize x2 | 8 | 4 avoidable |
| Remote CI, signing (2 + 3), publish | about 8 | inherent, serial because of approval |

**Rework** (the brief counts about 50 min): queue #2 26, rebuild 20, VM reruns and timeouts about 15 (some of this overlapped other work, so wall-clock is lower), second notarization 4, pack-models retry 2.
**Inefficiency inside the inherent work:** queue packing about 9-11, bootstrap retries about 3, lib-* rebuilding the same artefacts about 8-10 min of suite time (about 2-3 wall-clock at 4 jobs).

**Estimated avoidable total: about 60-65 min**, which brings the 115-min run to about 50-55 min. Around 50 min of that is rework and the rest is scheduling and caching.

## 2. Speed-ups

### S1. A pre-freeze precheck containing every cheap, input-static check (saves about 46 min when it applies)
Before freezing, run every suite whose declared inputs exclude the candidate binary and that takes under 3 s. That covers the doc-reference check, lint and manifest checks, and the 127 suites under 3 s. It also runs a check for untracked or stale metadata (`git status --porcelain` limited to the declared-input paths) and fails if anything shows up.
- Saves the 20 min rebuild and the 26 min queue #2 whenever a check of this kind would have been the only red. The untracked-file check removes the invalidation entirely.
- Risk: low. The queue still reruns these suites, so they are only moved earlier, never dropped.
- Experiment: tag suites by whether their inputs are candidate-independent, then run that set at HEAD before 0.0.20's freeze commit. It should catch the doc-reference red within 60 s.

### S2. Reuse results when the product bytes are unchanged (saves about 20-25 min on any rerun)
Byte-identical builds mean that a rebuild after a doc-only fix produces the same candidate hash. Key each suite's result on the hash of its declared inputs, with the candidate's content hash as one of those inputs and never the commit id. Queue #2 would then rerun only the suites whose inputs actually changed (the doc check plus the 6 metadata suites, about 1 min). It could also skip the rebuild itself: if the fix touches no product input, compare the product-input hash before rebuilding and reseal only.
- Risk: medium. It is only as safe as the declared inputs are complete. Mitigation: keep running a declared-input audit (a suite reading an undeclared file fails) in the queue.
- Experiment: rebuild the 0.0.20 candidate from freeze+1 (the doc fix) and check that the candidate sha256 is identical and the queue reports about 411 reused and 7 rerun.

### S3. Longest-first scheduling with short suites as filler (saves about 8-10 min per queue)
3557 s of suite time at 4 jobs is about 15 min, against 26 min actual. The gap comes from 56 window boundaries, idle slots at the end of each window, and warm-up. Schedule by recorded duration, longest first (exec-*, lib-*, self-build, warnings, tools, stages). Fill the remaining slots in each 55 s window with the under-3 s suites (bin packing, first-fit decreasing). Start a suite only if its recorded duration fits in what is left of the window.
- Risk: low. Order does not affect the results, given deterministic inputs.
- Experiment: replay the 0.0.20 per-suite timings through an offline simulator (pure Python, no builds) to get the projected window count. A target of 30-32 windows (about 17 min) passes.

### S4. Share lib-* and self-build artefacts through a content-addressed cache (saves about 8-10 min of suite time, about 3 wall-clock)
The lib-* suites rebuild the model artefacts on every run (790 s total). Build them once in the warm-up window, keyed by input hash, and have each suite hash-check and read them. Self-build suites (about 10 x 36 s) that share a stage can share the host-side build and keep only the per-target step.
- Risk: medium. A suite that tested building the artefact would now test reading it. Keep one suite that builds from scratch and compares against the cache hash.
- Experiment: time one lib-* suite with a cold cache and then a warm one. Expect 17-24 s to drop to about 5 s.

### S5. Serialise the VMs and start them early (saves about 15 min of rework)
Never run the Linux VM suite while the Windows VM is busy, because the 4 GiB guest and the host are oversubscribed and that produced the 19 false timeouts. Boot the Windows VM in the background at freeze time: boot takes 250 s and needs no candidate. Give the `up` helper a 300 s readiness poll, split into bounded 55 s polls that resume. Run Windows while the local queue runs at jobs=3 (heat budget), then Linux at jobs=2 after the queue finishes. Treat a timeout as needing a solo rerun before it counts as red, and record both results.
- Risk: low to medium for heat. Running the queue and the Windows VM together needs a temperature check; drop the queue to jobs=3 while it overlaps.
- Experiment: run `vms.sh up` with the extended poll from a cold start 5 times and confirm it reaches the agent every time.

### S6. Notarise once, after the payload is final (saves 4 min)
The notarised payload changed because the candidate was rebuilt. With S1 and S2 there is a single candidate. Submit for notarisation only after queue #1 is green, and key the notarisation ticket on the payload sha256 so an unchanged payload reuses it.
- Experiment: check that the 0.0.20 first and second notarised payload hashes differ only because of the rebuild.

### S7. Checkpoint the bootstrap inside a stage (saves about 3 min)
Stage2 and stage3 take about 60-90 s each and get re-entered through 58 s attempts. Make each stage resume per translation unit or per object, so each attempt does useful work only and the stage needs about 2 bounded calls at most. The fixed-point comparison is unchanged.
- Experiment: record the work completed per attempt on stage2. If attempts 2 and 3 redo work, checkpointing is the fix.

### S8. Fix the flaky benchmark (saves about 3 min and false reds)
A benchmark that measures a negative duration is using a clock that is not monotonic, or subtracting in the wrong order. Use a monotonic clock and clamp at 0, and make the benchmark assert on ratios rather than absolute times.
- Experiment: loop the benchmark 50 times inside the VM, bounded in 55 s chunks, and expect no negative values.

### S9. Give pack-models headroom (saves 2 min)
pack-models takes 48 s against a 58 s bound, and one run hit the bound. Split it into two resumable halves (per model group).
- Experiment: time both halves 3 times and expect each under 30 s.

### S10. Overlap CI and the signing qualification with the VM checks (saves about 2 min)
Push once, after queue #1 is green. CI (2.5 min) and the qualification signing (2 min) then run remotely while the VMs run locally. Only the company signing waits for approval.

## 3. Recommendation and order of work

1. **S1, the pre-freeze precheck plus the untracked-file check.** It removes the largest rework (about 46 min) at the lowest risk and in about a day of work.
2. **S2, result reuse keyed on the candidate content hash.** It turns any remaining late fix from about 46 min into about 2 min, and it builds on the declared-input hashing that already exists. Ship it together with the undeclared-read audit.
3. **S3, longest-first packing**, about 9 min on every queue run. It can be validated offline against recorded timings before touching the real queue.

Then S5 (VM ordering and early boot) and S4 (artefact cache). With S1-S3 and S5, a clean run is about 3 + 4 (bootstrap) + 1 + 17 (queue, VM Windows overlapped) + 11 (Linux) + 4 + 8 = **about 50 min**.

## 4. Tempting ideas that conflict with the constraints

- **Raising jobs to 8, or running both VMs alongside the queue.** This conflicts with the heat constraint, and the measurements show oversubscription creates false reds that cost more than they save.
- **Raising or relaxing the 60 s ceiling for bootstrap or pack-models.** This is forbidden. Use checkpointing and splitting (S7, S9) instead.
- **Reusing results across commits by commit id, or skipping suites "unlikely to be affected".** This conflicts with attribution to exact inputs. Reuse is allowed only on equal declared-input hashes (S2).
- **Using remote CI as the test loop, or pushing early to start signing.** This conflicts with the no-push-to-test rule. Push only after the local green.
- **Notarising before the queue to save time.** It risks a third notarisation if the queue goes red, so it is only acceptable once S1 and S2 make reds rare.
- **Dropping the solo rerun on a timeout red and auto-retrying until green.** This hides real hangs. A rerun is a diagnostic and both results must be recorded.
- **Caching artefacts without hash verification.** This breaks byte-identical attribution. Every cache read must check the hash.

## 5. First experiment

Replay the S3 scheduling offline (zero load on the machine, under 60 s), and in parallel run the S1 test: run the candidate-independent suite set at the 0.0.20 freeze commit. If the precheck catches the doc-reference red in under 60 s, that justifies S1 immediately.
