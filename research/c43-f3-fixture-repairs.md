# F3 acceptance fixture repairs

Actual initial CONTRACT queue on 735f4c743/root 4f0b493c: 187 completed, 162 green, 25 failed, zero unverified/pending. This is a failure inventory, not completed F3. The old queue and first-red logs remain under out/f3-contract-735f4c74.

The Linux cleanup test now executes the real guarded guest wrapper in a shell mock with the real guard closure. It checks removal of owned unisa-scratch linux.* and tmp.* paths, lock and current log; retains the latest failure log; preserves unrelated files and legacy guest home directories. Full boundcheck actual run retained all native exit/signal/descendant/stopped-cleanup assertions and ended 77 solely because this Linux host cannot verify Darwin Terminal ownership. This is UNVERIFIED, not PASS. No waiver is added.

The pipeline cache fixture now copies existing stamp.sh, cxbin, safedel.cx and three oracle headers using fixturecopy.sh. Production run.py and stamp.sh are unchanged. The rejected HERE-only candidate passed the first cleanup but failed the real stamp cleanup; both failure logs are retained. The complete original cache assertions then passed: mutation/restoration rebuild, unchanged hit, parse/lex addition/removal invalidation, generated-output exclusion and reference identity. No assertion removed and no interpreter logic/call site added.

queuecheck38 changes nine existing numeric constants. The admission/deadline reasoning is retained in out/queue38-budget-review.md; full/solo settlement, fractional rounding, four SIGTERM-ignoring leftovers, memory admission and booking serialization assertions remain. The combo limit and tail wall-time assertions tighten with the shorter deadlines. Actual full original test passed twice under unchanged bound44; timed run 35.121s. Original 44s timeouts remain red evidence, not waivers.

The vsnprintf regression retains the fd0 trap and all result checks, leaves scratch cleanup to the caller's guard, and removes raw unlink. The fileno regression likewise leaves its owned scratch file to caller cleanup instead of raw remove. Actual host and product binaries both exit0 and print exactly `vprintf 7 ok` / `vsnprintf bad=0`.

Progress now enumerates the current full --com plan. Missing jobs stay pending even with stray results/waivers; invalid, empty or duplicate plans cannot complete F3/F4. The new and existing selftests passed with zero failures. A CONTRACT-only result must never count as F4 completion. No host-exclusion policy is added here.

The bound fixture's new guarded cleanup has an explicit deletion ledger row and mutation row. The dependency generator declares the two fixtures' real guard closure. Derived consistency, inventory and zero-Python ratchets must pass in the checked index before landing. F3 edge seals, fresh-order traces and `stage-contract-e1e3` recapture remain separate unresolved evidence work.

Pre-landing measurements: the new cleanup row passed three refusal injections and its positive case (4 passed, 0 failed). The complete deletion check passed with 178 existing raw exemptions, 783 guarded rows and 783 mutation rows. Fileno host/product both exited0 with `fileno bad=0`. Progress over the original CONTRACT inventory reported 162/187 for F3 and 162/1118 for F4, with 931 missing full-plan jobs pending; zero-Python ratcheted to 1788/92/411. These measurements do not close F3 or start F4 acceptance.
