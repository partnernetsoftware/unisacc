# Explicit native runtime compatibility module

`tools/cx-runtime.cx` projects the carried C runtime into a declaration tree
and one ordinary C implementation unit. Consumers use the declaration tree as
their `HeaderRoot`; `RuntimeProvider` adds the implementation to the same CX0
bundle. Every consumer still has its own preprocessing state. Runtime storage
such as errno, allocator lists and stdio buffers has one provider definition.
No undefined function is silently forwarded to host libc.

From the repository root, with a fresh absolute output directory:

```
tools/bound 55 sh ./unisacc.com tools/cx-runtime.cx -- "$PWD/include" "$PWD/out/runtime-new" "$PWD/unisacc.com" --abi cx/runtime/lnx-x86_64.c tools/cx-driver.cx cx/cx0/bundle.cx cx/cx0/plan.cx cx/runtime/process.cx tests/preprocess/runtime-native.cx tests/preprocess/runtime_native_state.cx
```

The explicitly named bootstrap product only preprocesses an identifier-demand
probe under `-b lnx/x86_64 -E -I ROOT -include ROOT/cx.h`. Macro replacement
identifiers contribute to demand without executing consumer directives. The
provider is self contained preprocessed C; final consumer/provider compilation
uses the actual CX model chain. Preparation is reported as `PREPARED` and does
not certify a consumer run. All consumer modules must be named as demand inputs.

`source-identity.tsv` records original header hashes, complete consumer source
hashes, generator source hash, bootstrap identity and flags, optional ABI source
and implementation hash. Headers, consumers, generator source and bootstrap are
rechecked after preprocessing; `G` identifies source bytes, not the executing
generator ELF. `declaration-identity.tsv` records generated
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

`RuntimeProvider` currently selects a provider during assembly through C symbol
names and generated declarations. This is the first slice of late binding. The
next interface must let CX0 declare stable import IDs, type/ABI contracts and
ownership effects, then let workspace validate target/version-compatible
providers before wiring the graph. Each provider must uniquely own private
state and initialization. Structured import validation and protected privacy
are still missing from the present C symbol bridge; unchecked name lookup does
not supply those guarantees.

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

The final local five-unit source-driver compilation completed the real model
chain under its unchanged 45-second total and 12-second stage limits:
`out/c43-cx-runtime-self-driver-05/`. The generated 217,031-byte statically
linked ELF has SHA256
`771741a7d1ed15c3b559ca5341398924716f096662af5fa4315cf10206b99d75`.
Its preparation receipt remains `PREPARED-no-test-result`. That actual ELF then
compiled and ran `tests/preprocess/runtime-native.cx` through the new chain,
passing all five allocation/errno/stdio/process-group gold assertions in
`out/c43-cx-runtime-native-gold-05.stdout`.
The same self-compiled ELF also passes the original 14 driver contract
assertions in `out/c43-cx-runtime-driver-contract-05.stdout`.

Earlier failures remain separate: the original E2 budget exhaustion, missing
native `setpgid` at E3, and the larger netpack consumer's E4 12-second timeout
were not relabelled as passes. This local result does not replace complete
suite or same-candidate cross-platform qualification.
