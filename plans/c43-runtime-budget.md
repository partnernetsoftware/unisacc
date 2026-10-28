# Standalone runtime budget refusal

`exec/c/run.c` used `strtol` without an end pointer or overflow check. The
unmodified generic executor failed nine of twelve checks: malformed values
became step exhaustion, while overflowing positive numbers ran successfully.
Raw baseline: `out/c43-budget-before-01.{stdout,stderr}` and child transcripts
in `out/c43-budget-before-01/`. The unchanged maximum positive value and default
were successful; budget 1 correctly exhausted with status 3.

The standalone executor now accepts only nonempty positive decimal values up
to signed 64-bit maximum. Overflow is rejected before multiplication. Parameter
errors produce empty output, status 2, and exactly:

```
run: UNISA_MAXSTEPS must be a positive decimal integer <= 9223372036854775807
```

The parser is excluded from `UNISA_PRODUCT_BUILD`, which retains its constant
budget and does not read this environment variable. Leading decimal zeroes
remain valid when the resulting value is positive. The shared core and model
data are unchanged.

Validation uses `tools/runtime-budget-cases.cx ROOT RUNNER MODEL NEW_ABS_OUT`.
The model is `out/c43-fallguard-mixed-pc-01/models/shared-prune.net`, SHA
`8590223fd6e5d3ed12437ad505052355266690947b4f867961593ac0272318bb`.
Each child has a five-second watchdog; the outer call has 55 seconds.

- Host C build (`cc -O2 exec/c/run.c`): fixed runtime 12/0,
  `out/c43-budget-after-01.stdout`; baseline runner SHA
  `87fedf44013aae66b2e351e34ccdb6e79f815a16821afd07405b4eaeea35a3eb`.
- Product build (`sh ./unisacc.com -O2 -I include -iquote exec/c exec/c/run.c -o out/c43-budget-selfbuilt.elf`):
  15/0, `out/c43-budget-selfbuilt-01.stdout`. This includes the same twelve
  executor checks and three installed-product compilations: absent, `1`, and
  `abc` values produce byte-identical nonempty tape, with clean output/diagnostics.

Only the installed 79fb84e6 product-env-ignored matrix cell gains local evidence.
The malformed-budget cell records the real defect and pending stream-container
adoption; it is not PASS. No model generation, installation, old evidence
migration, or foreign execution was performed. This fix is ready for the next
batched source epoch.
