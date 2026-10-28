# Explicit native runtime compatibility module

`tools/cx-runtime.cx` projects the carried C runtime into a declaration tree
and one ordinary C implementation unit. Consumers use the declaration tree as
their `HeaderRoot`; `RuntimeProvider` adds the implementation to the same CX0
bundle. Every consumer still has its own preprocessing state. Runtime storage
such as errno, allocator lists and stdio buffers has one provider definition.
No undefined function is silently forwarded to host libc.

From the repository root, with a fresh absolute output directory:

```
tools/bound 55 sh ./unisacc.com tools/cx-runtime.cx -- "$PWD/include" "$PWD/out/runtime-new" "$PWD/unisacc.com" --abi cx/runtime/lnx-x86_64.c tools/cx-driver.cx cx/cx0/bundle.cx cx/cx0/plan.cx tools/cxlib/netpack.cx
```

The explicitly named bootstrap product only preprocesses an identifier-demand
probe under `-b lnx/x86_64 -E -I ROOT -include ROOT/cx.h`. Macro replacement
identifiers contribute to demand without executing consumer directives. The
provider is self contained preprocessed C; final consumer/provider compilation
uses the actual CX model chain. Preparation is reported as `PREPARED` and does
not certify a consumer run. All consumer modules must be named as demand inputs.

`source-identity.tsv` records original header hashes, bootstrap identity and
flags, optional ABI source and implementation hash. Headers and bootstrap are
rechecked after preprocessing; `declaration-identity.tsv` records generated
header hashes. The final driver's immutable bundle binds actual consumed bytes.
This is a before/after identity check, not an atomic snapshot against hostile
concurrent writers.

The projection preserves declarations, types, macros and conditional ownership,
replaces top-level static definitions with extern declarations, and removes
their bodies and initializers. The implementation exports the corresponding
symbols once. This is an unsafe C compatibility ABI: internal helper/state names
remain visible, and arbitrary callers can declare or mutate them. It does not
yet implement protected module privacy, compile-time ownership or arbitrary C99
header projection. Conditional body branches with inconsistent brace shapes,
unbalanced lexical constructs and oversized output are explicitly refused.

`lnx-x86_64.c` supplies the native Linux process-group syscall previously left
as an undefined host-forwarded declaration. It uses the same negative-syscall
return/errno convention as the carried runtime. Other platforms and general
host-library replacement remain unimplemented.

Independent projection gold is 14/0 in
`out/c43-cx-runtime-projection-gold-04.stdout`, including initialized multiple
declarators, wide literals, conditional function bodies and macro-selected
runtime demand. Imported constructor/destructor attributes are removed from
declarations and retained only on the provider definition; the independently
executed three-case provider batch is in `out/c43-runtime-provider-03/`.

`process.cx` is the small process capture module used by the source driver.
Its private buffer/error helpers and public `np_run`/`np_free` retain the old
netpack process contract, including environment replacement, independent output
channels and bounded descendant termination. Construction/packing algorithms
remain in the old netpack module and are no longer source-driver dependencies.
