# WF1 quick-test closure receipt (2026-10-10 21:50 SGT, cc)
wt /tmp/cc40-prep/wf1wt detached @0a6f334e9bf45f94fe703ebdfc5d770234f89070, status-lines=0 before/after.

## PRECHECK (precheck.json / precheck.log): GREEN, cold 11.68 s, warm 0.6 s, rc=0
identity: source_digest 0bd83174…f214 · ua_sha256 fd8f3877…71c2 · cc "Debian 14.2.0-19" · oracle cc 5c036d42…, backend gcc-14 a23ecab8… · difftest_o.sh e5d8c6be… · build_ref.sh 061f6446… · c99precheck.py b6b5dc74… · group ac5a9da2…
formal_plan: 4 shards, 257 members, ordered_list_sha256 c889acd7…
mapping: fb12-31→difftest_o-1 · fb12-29→difftest_o-4 · fb12-32→difftest_o-2 (each 3/3 agree)

## Formal shards (formal-difftest_o-{1,4,2}.log): gate.sh --suite, UA=same file, env -u WF1_PRECHECK -u PROBES
| shard | leading suite rc | child rc | probes | agree | wrong/refuse/known/revived/named | gate time |
|---|---|---|---|---|---|---|
| difftest_o-1 | 0 | 0 | 65 | 195 = 65×3 | 0/0/0/0/0 | 3s |
| difftest_o-4 | 0 | 0 | 64 | 192 = 64×3 | 0/0/0/0/0 | 3s |
| difftest_o-2 | 0 | 0 | 64 | 192 = 64×3 | 0/0/0/0/0 | 3s |
Wall-clock of my outer wrapper: UNKNOWN (bc not installed, wall field empty). Not reran, not estimated; only the gate's own 3s is cited.
Shard sizes 65+64+64(+64 for shard 3, not run)=257 match formal_plan.

## Identity after (21:50:15): HEAD, clean tree, sha256 of difftest_o.sh, build_ref.sh, c99precheck.py, oracle cc, gcc-14 backend, UA all equal the PRECHECK values; UA sha same before and after every shard.

## Reading
Same identity PRECHECK → mapped shards 1/4/2 → actual: GREEN, wrong 0. Per-probe result in the formal run comes from conservation (agree = probes×3, other counts 0), because the formal log does not name probes individually.

## Boundaries (not claimed)
- Dynamic UA ≠ unisacc.com: com-difftest_o-1/4/2 not run; PRECHECK does not discharge product duty.
- The UA validity key does not include the launcher backend's current sha, so the UA is not auto-invalidated by a backend change. Here the backend sha was checked equal by hand.
- Shard 3 not run (not affected). Real outer timeout is a separate item.
