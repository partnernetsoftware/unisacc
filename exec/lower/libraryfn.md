# Ordinary library function lower fragment contract (2b)

This model consumes tapecheck-accepted Tape, validated address-free USLCON1,
model-produced USLSEL1, and the explicit target rules. Enabled by the scalar
library/latebind=1; an absent flag preserves the old output. Windows is not migrated.
Only owner=1 ordinary flat ABI declarations are covered (legacy ABI description,
not signature-mismatch proof). Unsupported declarations refuse with a named reason.

`.libraryfn rN, ID` materializes `setreg REG, imm 0`. The only authorized consumable
call template is `.libraryfn`, `mov r0, r7`, `.librarycall rN, r0`. Clobbers and
control transfers poison this token, including across multiple labels; an old
USBIND cache cannot reauthorize that register. A successfully consumed token may
end at a label before a distinct old typed wrapper. No general dataflow claim.

Enabled main output is USLOWP1\n, two little-endian u64 lengths, complete and
fragment UNILOW1 frames. UNILOW1\n has textN:u64, count:u32, text, then 20-byte
records: offset:u64, length:u32=1, ID:u32, role:u8=0, reserved[3]=0. Each span
points to ASCII `0`. The fragment contains only new materialization instructions,
not the complete wrapper or old runtime bridges. Banks 900..903 times 2^40 hold
complete offsets, fragment offsets, stable IDs, and consumable authorization.

Actual model selftest: Linux x86_64 --full, 3530 states, 2070 hidden units,
393181-byte net. C/Python constructors are identical. Dynamic C checks 13 fixed
fixtures on tbl and net, exact reference fragments, canonical span extents,
four provenance refusals with empty stdout/named diagnostic, two unchanged typed
baselines, and alternate cwd/environment replay. Native executor and Python are
observation tools; old MODEL_COM only runs the checker. This is not fresh product,
other-target execution, API A/B provider, or whole-lower address-free evidence.

The fixed two-label, one-label, and jump-target cases previously accepted via an
old typed binding cache; the model now refuses. Do not remove these NEG controls.
