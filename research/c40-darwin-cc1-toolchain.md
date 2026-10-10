# Darwin k2b_toolchain cc1 real path (0.0.40 gate hygiene)

**Root cause (m4pro.local, Darwin/arm64, Apple clang):** `cc -print-prog-name=cc1` and `cc -print-file-name=cc1` both return the bare name `cc1` (Apple clang has no separate `cc1` binary). `k2b_toolchain()` then stored `['missing','cc1']`, so any narrow attempt failed closed with `toolchain identity incomplete`, and `tests/k2bcheck.py` exited rc=1 on Darwin even with an empty whitelist (same root cause as the 20:12 / 20:28 message entries).

**Fix (`tests/gatequeue.py` `k2b_toolchain`):** when `-print-prog-name` yields a bare name that is not a file, try `-print-file-name`; if still missing and `prog=='cc1'` on `sys.platform=='darwin'`, resolve via `xcrun --find clang` (or fall back to the resolved `cc`). Absolute missing paths (k2bcheck's `/nonexistent/cc1` mock) stay missing — no whitelist bypass.

**Measured (2026-10-10 ~20:33 SGT, m4pro.local):**

| Probe | Before | After |
|---|---|---|
| `cc -print-prog-name=cc1` | bare `cc1`, not a file | (unchanged host) |
| resolved `cc1` identity | missing → incomplete | `/Applications/Xcode.app/.../usr/bin/clang` + sha |
| `tests/k2bcheck.py` | rc=1 (`toolchain identity incomplete` / real-channel refuse) | rc=0 (two consecutive runs) |
| `tests/queuecheck.py` | blocked earlier by same incomplete when fingerprinting with FFI set | rc=0 with `UNISACC_FFI_X86_PROVIDER` pointing at an existing provider dir (Darwin/arm64 host requirement; unrelated to cc1) |

Linux (box) unchanged: `cc1` still `/usr/libexec/gcc/.../cc1`; k2bcheck+queuecheck rc=0.

**Host scope:** the Darwin bare-`cc1` path is required for K2b toolchain identity on Apple clang hosts (verified m4pro). Linux already had a real `cc1` path. Gate is host-conditioned only in the sense that the bug reproduced on Darwin; the fix is in-tree and fail-closed elsewhere.

**Not claimed:** no suite added to the K2b whitelist; v0.0.40 pipeline/product gates not opened.
