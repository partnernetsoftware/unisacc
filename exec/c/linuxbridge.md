# Linux host/script ABI bridge

`librarycall_arm64.S` and `librarycall_x86_64.S` now declare Mach-O symbols on
Apple and ELF `us_library_bridge_raw` GLOBAL FUNC symbols on Linux, with a
non-executable GNU-stack note. Instruction bytes are unchanged. `_WIN32`
preprocessing rejects both files: these are AAPCS64 / SysV host bridges, not
Windows native ABI implementations.

`linuxbridgecheck.py` cross-assembles both Linux ELF objects, compares exact
text bytes against current and HEAD-baseline Mach-O objects, verifies function
symbols and GNU-stack declarations, and checks Windows targets reject. This
checker proves representation and instruction preservation, not native
execution on Linux x86_64.

Private evidence for this slice is pointed to by
`/tmp/unisacc-r10-bridge-path` (not a stable repository fixture). A frozen copy
of the full runtime, prepared six-target package and native export C harness
was used on macOS arm64, Rosetta x86_64 and an already-running Lima Linux
arm64 guest. Each harness compiles source through E3, maps actual library code,
and calls typed exports through libffi: O0/O1/O2; six arguments; signed and
unsigned widths; void and pointer returns; state persistence; exit37/exit0
recovery; and main preserving saved function pointers. Linux required privately
downloading/extracting libffi-dev headers and linking the guest's installed
libffi.so.8; no system package was installed and no VM was started/stopped.
Linux GCC emits existing runtime style/unused warnings; compilation and the
harness return zero. This is three host execution cells, not all six targets.

The fixed bridge still requires a correct supplied signature and a private
writable soft stack. Function symbol discovery, binding policy, dynamic
imports, callbacks, recursive/thread-safe context entry and Windows bridges
are separate work. Exact source/package hashes and command stdout/stderr live
in the private evidence directory; newer model/module changes cannot inherit
these old-package checks without rerunning them.
