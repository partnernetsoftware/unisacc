# 0.0.37 g5 stack precheck (2026-10-09, before the reference window)

A private reference from main + the seven parked reference patches, applied in this order:
typedef-reset, unsized-rows, str3d-refuse, bkscr-bound, header-splice-line, err-atu-counts and L1′
macpool/MAXTD (/tmp/cc-l1prime-multiunit.patch).  All apply cleanly in that order.
Each suite ran under bound 58, with UA set to that private reference through term.sh:

| suite | rc | note |
|---|---|---|
| difftest 1-4/4 | 0 | |
| c99 | 0 | |
| declmatrix 1-5/5 | 0 | wrong 0 |
| declshape | 0 | |
| diag | 0 | ok 31 wrong 0 |
| sqlspeed ref | 0 | same as cc |
| luatests | 0 | ok 50 wrong 0 |
| realprog | 0 | ok 6 wrong 0 |

This is a precheck, not acceptance.  The window still runs export --check and the re-exports, then the gates.
