# R10 recursive callback signature graphs: next design

Status: **design only, not implementation or runtime proof**. Read-only audit of current source on 2026-09-29. No build, shared artifact, executable or existing file was changed. The parent owns PRD integration before implementation. Typed variadic integration is in flight; this report describes the next domain, not its completion.

## 1. What actually exists

- `exec/c/librarysignature-v2.md`: USLSIG2 carries full top-level parameter arrays, explicit REGISTER/ALL_STACK and length-bounded recursive aggregate descriptors. kind4 is a function pointer; tag4 is reserved for its signature. Existing limits are recursion32/nodes16384/record16MiB/arguments1024.
- `exec/c/libraryexports.h:129`: tag4 accepts only empty payload and does not introduce a signature. `us_export_native` supports tag0 primitives and tag1/tag3 representable structures/arrays; neither kind4 nor tag4 produces an ffi pointer. `us_export_slot`/`us_export_result` also lack kind4 handling. Merely returning `ffi_type_pointer` for kind4 would pass native code addresses to the incompatible script ABI and is incorrect.
- `exec/parse2/librarytypes.py:34`: the model detects FPB/FPV/typed signature ranges, emits kind4/width8/alignment8, and sets support0. It does not emit the signature's return/parameters. All FP forms currently get the same coarse classification, including extra pointer depth; pointer-to-function-pointer needs explicit treatment.
- `exec/c/libraryexports.h:213` and `:235`: closure/frame machinery already converts complete GP/FP/plain aggregates, owns copies per invocation and invokes the declared script mode. Closure caching is by named top-level `us_export`; it has no `(signature, callable value)` cache, nested callable conversion, or dynamic target binding.
- `exec/c/librarynative.h:20`, `:52`: plans own signature graphs/cif; arenas own native argument/result objects. They consume explicit fixed or concrete variadic ABI, not format strings or inferred C prototypes. Currently pointer bytes are passed through, and aggregate objects are copied without translating embedded callback fields.
- `exec/c/librarycallplans.h:31`: structural equality compares aggregates/arrays and excludes local base/shape IDs; it has no signature edge or graph cycle handling. Callsite plans must preserve callback graph equality in fixed prefixes and tails.
- `exec/c/libunisacc.c:459`, `:718`: nested calls already obtain separate guarded script stacks, use per-frame exit/setjmp, restore active/TLS, and clean native arenas. Native arenas are registered before ffi_call. Context-owned closures are freed with exports; compilation invalidates image before native callsite/template/plan graphs.
- `tests/libraryabi/nine_mixed_bidirectional.c:13` and `:62`: the proven nested callback uses a host global `callback9`, separately populated by `us_sym`. No function-pointer parameter/result crosses the FFI boundary. This proves reentry of exported closures, not recursive callback argument ABI.
- Hidden full-pool gap: `exec/parse2/functiontypes-result.tsv` still stores typed signature parameters at `signature*16+index`; `FS.eq.limit` rejects distinct signature comparison above8; `FS.query` reads that pool. `libraryexports.py:41` captures into the new full pool using the named `FPS_FN`, while `FS.params.begin` sets `sig_fp` for anonymous/nested declarators. Reuse of the top-level exporter without separating this ownership will capture nested parameters into the wrong signature. Full nested signatures must use an actual signature key, with their own variadic/mode facts, and full structural equality.

## 2. Smallest faithful shared wire extension

Use the already reserved **nonempty tag4** under USLSIG2, and version its payload explicitly. Old readers already reject it. Keep V1 and all tag0/1/2/3 records byte-compatible. Update the written protocol, shared model canonicalizer, host decoder, bindings validators and prune reader together.

Proposed tag4 payload (design, must be frozen in PRD before coding):

```
u8 schema=1; u8 form; LE64 signature_id
form=0: definition
  u8 variadic; u8 script_mode; LE64 parameter_count
  result_descriptor; LE64 stored_count (=parameter_count)
  parameter_descriptors[parameter_count]; u8 supported
form=1: reference; no further bytes
```

- IDs are local to a record, allocated in first-definition traversal order. Reject duplicate definitions, dangling/forward references and illegal field values atomically. References to an already introduced definition still being decoded permit finite cycles through function-pointer edges. This supports graphs without exponentially repeating shared signatures. Canonical model equality remaps IDs by deterministic traversal and compares visited signature pairs; local parser IDs never become ABI identity.
- The descriptor denoting a directly callable function pointer is kind4/depth1/width8/alignment8/tag4. Additional indirection is an opaque data pointer to function-pointer storage; it cannot be automatically dereferenced or rewritten. Old unresolved FPB/FPV forms remain support0 until exact facts exist.
- Function signatures have a separate fixed count/variadic/mode from the enclosing function. Only declared fixed callable signatures are closure-capable in the first implementation. A variadic function-pointer type may remain represented but support0 until concrete indirect-callsite plans and an explicit typed native export contract exist. That remaining item must be named, not silently claimed complete.
- Keep independent limits for signature definitions, type nodes, argument count, recursion of serialized definitions and bytes. Reference edges do not recursively consume the same type forever. Validate by-value aggregate recursion as illegal; only pointer/signature edges may close a cycle.
- Host owns nodes in one graph arena, not recursively freed child ownership. Parse all definitions/references, resolve edges, then prepare each declared cif. A callback's ffi type is mechanically `ffi_type_pointer`; its child signature cif is a separate object. An outer cif does not recursively embed a child cif, so mutual pointer edges do not require infinitely recursive ffi layouts. Layout checking and unsupported union/bitfield/wide-FP behavior stay intact.
- Host equality for defensive callsite checks must follow the same graph representation with a visited pair set. Model canonicalization remains the authority for source/declared compatibility and candidate selection.

An inline-only signature tree is a possible first experiment, but cannot be called complete recursive graph support: shared/cyclic signatures would still be rejected. Def/ref framing avoids a subsequent format rewrite.

## 3. Runtime callable representation and ownership boundary

A pointer-shaped native ABI slot does not carry its provenance. Do not guess by address range, symbol spelling, host ABI, or whichever registry happens to contain it.

Recommended library-mode representation: model-defined callable handles for typed indirect calls, backed by a context-generation table. The model emits the operations that introduce a **script target** (address-taking of an actual script function plus its signature/mode) or a **native target** (native boundary value plus its declared nested signature). A handle records origin, exact graph signature, generation and target. Host verifies and executes these operations mechanically. It does not choose source types or infer whether an arbitrary numeric address is callable.

- Library-mode typed indirect calls lower to an existing six-field dispatcher plus declared ValueFrame/plan, rather than directly calling an imported native pointer using script registers. Script-handle dispatch uses the existing script invocation bridge; native-handle dispatch uses ffi_call and the exact nested signature plan. Default nonlibrary C behavior remains unchanged.
- Script-to-native callback value: null stays null; a native handle unwraps its borrowed target; a script handle produces/caches a closure with userdata `(context,generation,signature,target,mode)`. Closure entry converts incoming callback pointers to native handles and reenters `library_invoke_frame` on a private nested stack.
- Native-to-script callback value: null stays null; otherwise a declared native handle is created. Calling it from script goes through the typed indirect dispatcher. Passing it back native preserves the original native target, not an extra closure layer.
- Callback return values use the same conversion in reverse. For graph-typed aggregate values, traverse declared member/array offsets on independent copied objects and translate callback slots in the copy; never overwrite borrowed caller objects. Ordinary data-pointer pointees are not traversed implicitly. Native callback fields and returns need dedicated tests, beyond top-level scalar pointer tests.
- Cache by generation, signature identity, origin and target so repeated round trips preserve usable function-pointer identity. Source type compatibility is checked by the model before a callable is introduced at a typed position. Model-defined conversion actions, not C's address heuristics, decide bridge direction.
- All callback closures/handles and nested fixed plans live until the context generation is invalidated. This permits a native callee to retain a script callback under the existing `us_sym` lifetime contract. Invocation temporary aggregate/argument objects remain per-ScriptFrame. Borrowed native callbacks/modules must remain alive under the existing caller lifetime contract.
- Clear order: prohibit mutation while a context call is active; quiesce callers; destroy image/exports/callback code, then callable registry/plans and graph arena, then owned modules. Successful compile/relocate/binding mutation invalidates previous pointers. Failure registration preserves prior generation. Cross-context handles require an explicit ownership contract; reject implicit import rather than accidentally borrowing another context's image.

Alternative model-generated native-pointer wrappers would require a way to bind a dynamic target to each wrapper (or bounded static stub pools), which the present plain script function pointer lacks. Declared callable handles and a generic dispatcher add less ABI-specific machinery and avoid teaching C to generate script code.

## 4. Nested error and exit propagation

The existing frame reentry is reusable, but nested failure currently need not make the outer call fail: `us_export_callback_frame` returns a zero sentinel, while `library_native_invoke` subsequently treats ffi_call as success, and an outer successful `library_invoke_frame` can clear context error/status. Per-export `last_status` and context-global status are not adequate for recursive invocations.

Add a native-call boundary token/sticky outcome owned by its arena and connected to the active ScriptFrame chain. Closure invocation catches its own script error/exit on its own setjmp, stores first failure/exit status/message in the nearest waiting native boundary, returns the ABI zero/NULL/void sentinel, and restores TLS normally. Native target gets to return normally and release its own C resources. After ffi_call returns, dispatcher checks the sticky outcome **before copying a native result**; it cleans its arena and propagates to the outer script frame at the script boundary. Never longjmp across the native target's C stack from a callback. Cascading boundaries each preserve the first failure; nested success cannot clear it. Outermost `us_call_status/us_error` report the failure/exit. A standalone retained callback call has no enclosing native boundary, so reports through its own context call status.

Frames need context ownership and an explicit invocation outcome snapshot; a callback from another context must not poison the wrong active frame. Initial support remains one caller per context and synchronous reentry. Foreign-thread retained callback execution requires initialized TLS and the same one-caller/quiescence contract; six-platform qualification is separate and must not be inferred from macOS.

## 5. First real public-API acceptance test

Add `tests/libraryabi/callback_graph_bidirectional.c` with an actual host C target and actual model-compiled script. No ctypes substitute and no host-global `us_sym` callback injection.

Shared conceptual declarations:

```c
struct Pair { double d; int n; };
typedef struct Pair (*Leaf)(int,double,float,int*,struct Pair,int,double,int,int);
typedef struct Pair (*Relay)(Leaf);
struct Pair host_drive(Relay relay, Leaf leaf) { return relay(leaf); }
```

Script defines `script_relay(Leaf leaf)` which really calls `leaf(2,1.5,2.5,&local,{4.0,5},6,9.0,7,8)` with `local=12`; define host `native_leaf` to produce `{17.0,40}` from the nine mixed inputs. Script `entry(Leaf leaf)` returns `host_drive(script_relay,leaf)`. Host registers **typed graph** `host_drive`, compiles/relocates, obtains `entry` through public `us_sym`, and calls `entry(native_leaf)`.

This path is native→script entry→injected native host_drive→script callback relay→native leaf, and its callback descriptor recursively contains another callback signature. It covers callback ingress, script function address egress, closure reentry, typed native indirect call and Pair return, with nine arguments forcing the full nested pool. Run O0/O1/O2, ARM native/Rosetta,100 iterations each, exact17.0/40 and byte/canary checks. The model-emitted graph must contain all9 Leaf descriptors, not a truncated8 or synthetic host fixture.

Additional minimal controls: different native leaf addresses with same type; repeated null and roundtrip identity; incompatible nested result/parameter9 rejects before execution; callback returning a callback; struct/array callback fields preserving caller objects; shared graph edges and bounded cycles decode or explicitly reject execution where source facts are unresolved; malformed/truncated/dangling/duplicate graph atomic rejection; outer native target records normal completion after callback `exit(23)` and outer call reports failure23; subsequent call/context succeeds; failure mutation preserves old callback; generation teardown releases all closures/graphs without exercising invalidated pointers. Native callback retention across later calls must work until generation teardown. Full callback-varargs remains an explicit next acceptance, not covered by this fixed test.

## 6. Nonoverlapping implementation domains

1. **Model facts/semantics:** `exec/parse2/functiontypes-*.tsv`, `functiontypes.py`, `librarytypes.py`, `libraryexports.py`, `libraryimports.py`, `libraryvariadic.py`, relevant generation bindings and a new model-only callback module. Full signature capture keyed by `sig_fp` or named signature; complete typed query/equality; signature graph export; model callable origin/introduction/indirect-dispatch actions. Do not let nested capture overwrite named pools. Audit classic/reference function-type facts if used as differential oracle.
2. **Shared wire model/prune:** `exec/modelsignature.py`, `exec/prune/libraryroots.py`/schema/checks and protocol documentation. Canonical graph equality and bounded skip/validation; public roots retained independent support. Frozen protocol agreement first, then independent implementation.
3. **Host decoder/mechanical ABI:** `exec/c/libraryexports.h`, `librarynative.h`, `librarycallplans.h`, `librarybindings.h` and resolver typed validation, plus decoder/native host unit probes. Owned graph arena, child cifs, closures/callable table and conversion of declared copied fields. No model compatibility or symbol winner logic here.
4. **Parent integration:** `exec/c/libunisacc.c/.h`, ScriptFrame/boundary outcomes, public lifetime, resources/dispatcher addresses, public native test, gate registration, PRD and generated/model artifacts/candidate evidence. Coordinate with the in-flight variadic owner rather than edit its files concurrently. Freeze tree before rebuilding or integration runs.

Recommended order: agree protocol/runtime handle contract → graph facts + decoder red probes → actual public fixed callback chain → sticky errors/lifetime/aggregate pointer fields → indirect variadic callback callsites and remaining platforms. Every successful stage must be labelled with actual proof scope; this report proves none of them.
