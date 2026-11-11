# Driver preparation/execution split (design only)

Actual diagnosis reported by cc: cold compilerpack preparation takes about 47s;
with other preparation and 27 comparisons the suite exceeds its 60s window.
An interrupted construction does not publish a completed cache. One bounded
standalone preparation warmed the existing content cache; the original suite
then passed in 24s without reducing its 27 checks.

Proposed split: bounded preparation shards construct stage networks, optimization,
package and three drivers; publish a complete manifest only after success.
Execution must recompute source-closure, target and construction-tool identities
and verify every artifact digest before invoking the existing 27 comparisons.
Missing, partial or stale preparation fails closed. Preparation results are not
test passes, and execution results are never reused merely because preparation
is cached. Keep all outer and inner limits at most 60s.

Use the existing checks in exec/pipeline/models.py:49-70 and
exec/c/compilerpack.py:93-121 as the identity/verification basis. Do not merely
set STAGE_MODELS_READY=1: exec/pipeline/elf.sh skips preparation under that flag
without independently verifying its directory.

No implementation in this proposal; no suite, queue fingerprint or layer changes.
