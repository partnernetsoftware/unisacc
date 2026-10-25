# Archived test system

The owner retired the 1333-unit queue on 2026-10-25 and requested a new native
contract-based suite for 0.0.43. This tree preserves the historical test source
and Makefile; it is not a live gate and is not counted as new acceptance.

The original layout is recoverable from commit `2500337ef4`. Scripts here assume
that historical layout; do not redirect current commands into this archive.
Product construction support moved separately into `tools/`.

The independent F4 checkout and raw state remain at
`/tmp/unisacc-c43-final-e4-20261025`, stopped after window082:
327 settled, 314 rc0, 6 resource unknown, 7 failures, 1006 pending.
Failures and unknowns have not been relabelled as passes.

The new entry is `sh ./unisacc.com tests/run.cx -- --smoke`.
`--all` must refuse completion while coverage gaps remain. Valuable old fixtures
and assertions should be migrated with their contract, expected result and
independent evidence; archiving them supplies no new test result.
