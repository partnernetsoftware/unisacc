# Development and construction support

`sourceflat.cx` is the native complete-source exporter:

```sh
sh ./unisacc.com tools/sourceflat.cx -- out/unisacc-flat.c
sh ./unisacc.com tools/sourceflat.cx -- --verify out/unisacc-flat.c
```

The export and provenance verification use the product directly. `export_ref.sh`
is only a compatibility loader for existing construction callers.

`stream-pack.cx` prepares seven isolated stream artifacts from a candidate's
existing model bytes. Its shared runtime and the new partial contracts are
documented in [streams.md](streams.md). Preparation never records a test PASS.

[nets.md](nets.md) describes the native full-gold integer-kernel batch and its
identity/anti-vacuity controls. [assembly.md](assembly.md) tracks the native .S
construction path, its real encoding checks and the remaining gaps.
[kernels.md](kernels.md) specifies the new bounded construction steps and native
ELF-to-UNIKERN1 linker contracts; it does not claim a completed product rebuild.
[packages.md](packages.md) describes the pinned, source-checked migration of
unchanged P3 models to freshly built native kernels and a rebuilt driver.

The following support was separated from the archived test suite so that
construction is not broken by its removal: `bound`, `bound.c`, `cxbin`,
`safedel.cx`, `lib.sh`, `build_ref.sh`, `refshim.h` and `reffoot.h`.
These are transitional construction adapters, not the new test framework.

Remaining dependencies are explicit: the native watchdog cold bootstrap and
independent reference compiler still use a host C compiler; `cxbin` has an
existing C-oracle bootstrap for the deletion guard. The old watchdog's Python
cold path has been removed. A direct self-hosted run of the old watchdog source
currently fails on the uncarried `sys/prctl.h`; this is an active migration gap.
Existing model coordinators still containing Python are not described as native.

The construction source identity includes the ten support inputs bound, bound.c,
cxbin, safedel.cx, lib.sh, build_ref.sh, export_ref.sh, refshim.h, reffoot.h and
sourceflat.cx under tools/. The native seed/ident.c and compatibility coordinator
declare the same ordered list. A native isolated-tree regression independently
assembles path-NUL plus binary SHA256 payloads and checks each input mutation,
restoration, and missing/directory refusal (15 checks). Run it after constructing
the native identity tool:

```
tools/bound 40 sh ./unisacc.com -O2 -fno-trim-libc seed/ident.c -o out/ident
make identity IDENT_BINARY=out/ident IDENT_OUT=out/identity-fresh
```

This regression checks support-source binding, not the complete build pipeline or
Python runtime parity. Changed source identities invalidate old build manifests.

`cxlib/` retains five native support modules still imported by exec/ callers.
Eight exec/ module symlinks now point here instead of the archived tests tree.
The host watchdog and reference-export paths inside the modules use tools/.
These compatibility callers do not become the new test gate, and their historical
PASS records are not imported. Four caller-family dependency plans and a six-check
default-run support probe passed after relocation; complete historical suites
were not run. Existing external-reference dependencies remain explicit debt.

New `.cx` tools use module interfaces and the implicit runtime without manual
`#include`. Compiled-artifact caching is reserved for measured setup hotspots;
it must not be confused with caching test results.
