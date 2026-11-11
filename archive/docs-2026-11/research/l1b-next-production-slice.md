# L1b next production slice: sys/write private snapshots

Status: source review only; no product files changed. Input window remains with cc after 7646bb81.

## Current contracts

`src/back_lower.c` bk_syscall, `.sys` and `.write` route globals SCR0/SCR1/PLEN through per-op ABI shapes. `exec/lower/code-abi-sources.tsv` is the ordered schema for mode 0 (.sys) and 2 (.write). Mode 1 (.sys6) is already private. Python and C/delta must preserve each shape's argument count and immediate roles.

| operation shape | ABI argument sources after snapshot |
|---|---|
| plain | slot0, slot8, slot16 |
| zero4 | slot0, slot8, slot16, imm0 |
| atfd_1 | imm-100, slot0, slot8, slot16 |
| atfd_1_zero | imm-100, slot0, imm0 |
| atfd_2_zero5 | imm-100, slot0, imm-100, slot8, imm0 |
| .write | imm1, slot0, slot8 |

Old research/l1b-stack-prototype.patch bk_proto3 emits all six ABI registers regardless of shape. That is a diagnostic prototype, not the sequence to land.

## Production implementation plan

1. Snapshot original SP into x12/r11. Allocate 80B; store exactly the source operands before ABI changes (three for .sys, two for .write), remapping source r7 to the original-SP snapshot. Save FP at 48 and original SP at 56.
2. Emit syscall number and exact ordered argument map. Memory reads become stack-relative load64; immediate sources retain existing setreg/role metadata. No extra zero-filled ABI args.
3. Use current gate metadata/carry handling. .sys writes the return to r0; .write retains its existing result behavior (review the physical return register contract before adding a mov). Restore FP and release 80B.
4. Extend exported private_modes to POSIX modes 0/1/2 with explicit per-mode stackoffsets. Dispatch identifies both op and mode; never overwrite shared legacy mode/gate state names. Windows uses its current graph unchanged.
5. Keep .exit/.print outside this change. Shared .print formatting is not promised signal safe. Do not activate the signal header while ARM real-SP invariant is still incomplete.

## Verification before handoff

- Compare C reference, Python lower and C delta for four POSIX targets; Windows graph and four fixed images unchanged.
- Cover every shape in the table, duplicated/permuted source regs, and r6/r7 operands.
- Actual raw tape on macOS two architectures: sys6/sys/write with original r7 buffer, output ABCABCABC; run candidate .com both -run/-o, plus Linux arm64 when ready.
- Deterministic nested invocation at snapshot/load boundaries checks outer args, FP/SP and return value; actual nested signals remain a later combined acceptance.
- Seed C lower full JSON == Python; facts check, graphhash and fresh-order follow existing gates. Each command and inner step <=60s.

## Subsequent ARM encoder work

.frame/call/ret, explicit destination r7 writes, SPINIT, HENTRY/HLEAVE and HOSTCALL need one common real-SP protection rule. Allocation protects real SP before committing x7; deallocation commits x7 before releasing real SP. HLEAVE uses an entry anchor independent of moving current SP. Existing HOSTCALL already owns a real-stack save frame, but every signature path must be checked against the invariant. Windows must not absorb the POSIX ARM prototype.
