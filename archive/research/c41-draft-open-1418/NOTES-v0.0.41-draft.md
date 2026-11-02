## unisacc 0.0.41 (Draft)

**Status: Draft.** Unsigned product bytes (signing deferred; align 0.0.40 precedent). Authority: 政委授推到 42 公开.

### Materials
- `src/version.h` = **0.0.41**; H1=INHERIT (`archive/plans/v0.0.38.md`)
- Candidate `unisacc-next.com` sha256 `bcd2b9b75129cd511e5bb7fa7acad317e7a54d13bb6750ab4c540bbced07b2b9` @ commit `809871a4`
- env-pin GATE `/tmp/c41-0843-envpin/cand.queue` **721/721** PASS **627** (1146对照 +30; 禁 void 1146)
- Buckets: research/c41-envpin-closeout-1409/final-bucket-vs-1146.md
- env-pin false-red tip fix: `d4cd5f6f` (gate-infra/queuestart; local suite green)

### Gaps (do not block Draft open; block seal/Latest until filled or waived)
- GHCR `seal_candidate` / `release/candidate.json` still 0.0.40 (model-audit/models.json not in cand tree)
- `rc/v0.0.41` not yet cut
- Apple/Win signing deferred

### Non-pass host surface (listed, not PASS)
- rc77×6 Darwin/host N/A; rc142×20 timeouts; rc2×10 seedgen; ruled/unruled rc1 per closeout
