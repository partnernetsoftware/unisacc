# Generic C execution kernel

`core.c` and `core.h` are the complete C machine, independently compilable.
They contain threshold-network inference, the retained table-control path,
all actions, fixed-width arithmetic, stack/input frames, byte buffers and
attributes, sparse indexed memory, blobs, interning, resource-cache identities,
and stage-state cleanup. None interprets a compiler token, type, ABI or image.
The action numbers/arity are unchanged; `tbl.py` checks them against `core.h`.

`run.c` owns model/package decoding, routes, file IO, resource-name adaptation,
step-budget configuration and printing diagnostics. `compiler.c` owns the
command line and byte framing; the models still implement compiler decisions.
The default build includes core.c for the compiler's one-source build route.
`-DUNISA_CORE_EXTERNAL` instead links a separately compiled core.c; corecheck.sh
executes that form against the same independent expected results.

## Interface and ownership

- `CoreModel` borrows decoded immutable arrays from the loader. The loader
  retains them until `core_run` returns. No parsed source or compiler state is
  supplied through this interface.
- `core_run` takes source bytes, a source-name byte string and an explicit
  step limit. All machine state starts empty and is freed before return.
  The current implementation permits one active invocation at a time.
- `CoreResult` owns accepted output and diagnostic bytes, which the caller
  frees. A rejection reason can borrow a model string, so it is printed before
  model unload. Failed output is discarded. Status remains 0/1/2/3 for
  accept/reject/bad transition/step limit.
- `core_host_fetch` takes an uninterpreted byte key. It returns absent (0), a
  borrowed span (1), or a malloc-owned span (2). The core copies a present span
  into a blob and frees only the latter. The host handles packaged resources,
  process inputs and filesystem paths; language-dependent header-search order
  is still computed by E2.
- `core_host_panic` reports an unrecoverable allocation/invariant failure and
  must not return. The fallback abort is not an execution action.

The memory allocation/copy/compare/string-length functions are ordinary linked
C-library dependencies. They must be included in whole-artifact accounting;
calling them does not make their bytes part of the standalone core object.
The source-name length and OFILL decimal rendering are generic byte operations.
The signed decimal renderer covers INT64_MIN without signed negation overflow.

## Measured C core (macOS host, `cc -Os`, uncompressed)

| Object section | arm64 | x86_64 |
|---|---:|---:|
| machine code `__text` | 6,396 B | 7,106 B |
| constants `__const` | 380 B | 224 B |
| diagnostic strings `__cstring` | 372 B | 372 B |
| static zero storage | 0 B | 0 B |
| compact unwind | 800 B | 768 B |
| eh_frame | 0 B | 1,024 B |

`corecheck.sh` rebuilds and prints these sections, checks the external-linkage
runtime, and audits undefined symbols. Its allowed imports are the two host
entries, libc allocation/memory primitives, abort and stack-protector support.
The object includes every generic helper, not just the dispatch loop. Heap
storage, OS/loader/driver, C-library implementation and model bytes are not in
this table. These are C-compiler results, not a handwritten assembly kernel or
sizes produced by unisacc; the latter remains a separate task.

The network-built runtime still reconstructs itself through all six networks
in nativecheck.sh. Its isolated input directory now explicitly includes core.c
and core.h alongside runtime.c: those are real source inputs, not copied model
answers. Product unisacc.com still uses the retained reference compiler route.

The following compression/whole-tool measurements are the initial 8b3be84
baseline, before the assembly-interface changes; they are not current sizes.
For compression reference only, gzip-9 of the object __text bytes is 4,004 B
(arm64) and 4,232 B (x86_64); runtime does not decompress these objects and
those numbers are not the kernel-size criterion. On the same macOS arm64
host, the complete run tool built with cc -Os has __text 16,184 B and file
55,288 B (gzip-9 14,187 B; dynamic libSystem excluded). The unisacc -O2 build
has __text 98,288 B and file 132,258 B (gzip-9 22,613 B; carried library
included). This is not a same-optimization speed/size comparison. Models and
templates are external to these tools; this slice changes no model payload.
The loader/IO portions are still linked with their respective libraries; a
complete per-component release ledger is still required at final switching.

The former 603-line run.c is now 418 lines, plus 254 lines of core.c and 32
of core.h (704 combined). This is an explicit API/ownership split, not a
source-code reduction. It establishes a measurable C baseline for assembly.

The next migration slice is documented in [asm/README.md](asm/README.md):
handwritten inference, word arithmetic, buffer append, sparse indexed
memory, binary string interning, blob copies, resource caching decimal field rendering and control/input stacks on arm64 and x86-64, with remaining C actions retained and an explicit
build selector. The C-only measurement above includes the current capacity
guards and explicit CoreMachine invocation state. Product routing is unchanged.


CoreMachine now makes the complete mutable action state explicit. core_run
allocates it locally, invokes the selected action engine and transfers/frees
its owned buffers at exit. Both ISA action engines are assembly. All decoded actions are low-level machine operations. See asm/README.md for
per-action state comparisons and the remaining lifecycle migration boundary.
