# Phase timing: eight finite gates on 79fb84e6

The installed product already emits package, model and inference stamps. The old
matrix still described these as absent. No product rebuild was needed.

`tests/phase-timing-stamps.cx` now runs its C probe with `-O2` so E4 actually runs.
It requires all seven stream identities in order and matching model/inference
pairs. Four negative controls reject missing E4, substitution by tapecheck,
mismatched pairs and an unpaired final model. The eleven malformed-report
controls remain intact. An auxiliary stage count cannot satisfy seven streams.

Executed command (outer watchdog 55 seconds, each child 12 seconds):

```
tools/bound 55 sh ./unisacc.com tests/phase-timing-stamps.cx -- /home/devali/repos/unisacc /home/devali/repos/unisacc/unisacc.com /home/devali/repos/unisacc/out/c43-phase-79fb-02
```

Result: 62 assertions, zero failures, exit 0; stdout/stderr are
`out/c43-phase-79fb-02.{stdout,stderr}`, with raw child outputs, statuses and
report in `out/c43-phase-79fb-02/`. Product identity is
`79fb84e65a10b55ce40be5f68746748acbd38f441944046c0a5dfa5718bc0c32`.
Product, probe and test source hashes were checked unchanged during execution.
The original default-optimization run remains recorded as 57/0 in `-01`; it
does not establish seven-stream coverage.

Only eight cells gain this evidence: isolated startup cold/hot, reuse lookup
miss, model inflate cold, seven-stream inference cold, validation cold/hot,
and report schema. Six were MISSING; two were NEEDS_PRODUCT_CHANGE.
The timings are observations, not speed thresholds or OS-cold measurements.

Remaining boundaries: lookup hit=2 is the executed host route, not the required
hit=1 memory route. Hot inference/model phases are explicitly skipped and remain
UNMEASURED. Package parsing alone does not establish the entire construction
contract. Exec image preparation is not program stdout flush duration. External
stream processes, memory binding and foreign hosts remain open; the parent
contract stays NOT_READY. No old matrix rows are migrated to this product.
