# Context-owned native imports (R10 development)

`us_add_symbol` borrows an address. `us_declare_import` supplies a trusted
function ABI or data-object description without an address. `us_load_library`
opens a native library with `RTLD_NOW | RTLD_LOCAL` and owns its handle until
`us_free`. The public declarations are in `libunisacc.h`.

For declared or injected names, compile freezes **all** lookup candidates:
explicit injection, the loaded process (`RTLD_DEFAULT`), and owned libraries
in load order. E3 and lower use the same model rules to choose the smallest
`(origin, ordinal)`, then check the selected declaration against the source.
A real source definition takes precedence. An incompatible selected candidate
is rejected; a lower-priority address is not substituted to hide that error.

The frozen `USBIND2` resource is reused unchanged during relocation. Native C
only enumerates and serializes candidates. It does not parse the source,
choose a function, resolve source types, or select a preferred candidate.
`USBIND1` remains accepted. V2 validates all candidate structures before
selection; legal unsupported losers do not invalidate a supported winner.

## Trust and lifetime

`dlsym` proves neither ABI nor object type, extent, alignment, or writability.
The caller supplies accurate declarations and keeps borrowed addresses alive.
Object extent is storage size, not pointee size. Descriptor `base` and `shape`
are opaque local IDs; depth, class, width and unsignedness determine the
supported ABI comparison.

Successful registration, declaration or loading invalidates compiled code and
previous exports. Recompile and relocate before use; never call a stale
pointer. Failed mutations preserve the previous compiled generation. Owned
handles close only after images and callable closures are destroyed.
Each context has one caller at a time; separate contexts may run concurrently.

## Verified scope and remaining work

The native macOS ARM checker exercises O0/O1/O2, source/injected/process/owned
priority, first-owned-library precedence, writable integer objects, missing
symbols, declaration conflicts and mutation recovery. Host candidate IO also
has ordinary and ASan/UBSan checks. These are not six-platform qualification.
Current callable signatures are fixed integer/data-pointer arguments (at most
six), with integer/data-pointer/void return; floating point, aggregates,
variadics and automatic function-pointer adaptation remain unfinished.
Data array/aggregate/read-only access is not implemented. Windows native
resolver and library OS adaptation remain unfinished.

Use `lib-resolver` and `lib-resolver-host` in `tests/gate.sh`; they consume the
explicit `MODEL_COM` candidate. `tests/modelcandidatescheck.py RUN` separately
constructs the selection network, enumerates its entire finite observation
domain, and checks malformed wire and truncated input against both executors.
