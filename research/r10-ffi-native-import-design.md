# Typed native imports after USLSIG2 — source audit only

This is a design report, not an execution receipt. Main tree was not written or rebuilt.
Audited current files: exec/c/{libunisacc.h,libunisacc.c,librarybindings.h,libraryresolver.h,libraryexports.h}, exec/parse2/{libraryimports.py,librarytypes.py,libraryexports.py}, exec/{modelbindings.py,modelcandidates.py}, exec/lower/libraryimports.py, exec/enc/hostbridge.py, unisa/hostabi.py.

## 1. Important version correction

USBIND2 is ALREADY allocated. libraryresolver.h emits USBIND2 candidate records by adding ordinal after origin; modelcandidates.py chooses origin/ordinal in delta and converts the winner to USBIND1. Replacing USBIND2 with recursive descriptors would ambiguously reinterpret existing bytes. New typed binding wire should be USBIND3, with explicit version dispatch and V1/V2 compatibility retained. Candidate priority remains in delta.

## 2. Actual current restrictions

- libunisacc.h us_type_descriptor has only six words (depth/base/shape/kind/width/uns), no alignment/ordered members; us_signature adds kind/result/args/count/variadic/extent/writable. us_add_symbol/us_declare_import mechanically flatten those six words into librarybindings.h.
- librarybindings.h permits count <=1024 but sets supported only for nonvariadic <=6 GP/data pointers/void. Registry copying is shallow because current descriptors have no children. Wire is record-length-framed USBIND1 with fixed 48-byte descriptors.
- libraryresolver.h declaration equality compares depth/kind/width/uns only; neither base/shape nor member layout is compared. New recursive graph equality must compare ABI facts recursively, never opaque parser IDs.
- parse2/libraryimports.py re-decodes USBIND1, matches source prototype via FPS_PARAM stride16 (not the new 1024-slot library parameter capture), rejects argc >6, FP and aggregates. It emits fixed .frame 48, stores r0..r5, invokes the borrowed native address directly via .librarycall.
- lower/libraryimports.py validates that library capability is present and rewrites .librarycall to hostcall. Encoder/unisa.hostabi implements a FIXED SIX GP native ABI. It cannot directly call arbitrary mixed FP/aggregate native functions. Keeping this bridge as a GP dispatcher entry is valid; extending its six slot list alone is not sufficient.
- New librarytypes.py already emits the USLSIG2 recursive graph. libraryexports.py captures all parameters before the old eight-slot limit. Reuse those captured facts and the SAME descriptor serializer for imports. Do not implement another source type classifier in libraryimports.py.

## 3. Minimal executable fixed-type slice

Introduce a single context-owned typed native dispatcher. Keep the existing six-GP machine bridge to call THIS dispatcher, never the arbitrary typed target.

1. New explicit typed declaration API (preserve old public struct ABI/API): us_add_symbol_typed/us_declare_import_typed receive a bounded USLSIG2-compatible signature/TypeGraph blob. Deep-own it once, validate the same recursive layout using the shared decoder/libffi builder, prepare fixed ffi_cif, retain target address separately. A normal struct is supported only if native ffi size/alignment/each field offset equals declaration; union/bitfield/packed shapes remain represented pending NativePlan, not byte-array hacks.
2. USBIND3 keeps record_length/name/kind/origin/ordinal/nativeABI/address plus full recursive result and all parameters, variadic/count, data extent/writable, dispatcher plan handle. The ABI field is an explicit host-target ABI identifier, not implicit zero. Opaque base/shape remain diagnostics. Preserve V1/V2 semantics; old GP declarations can be converted mechanically from the same primitive ffi type facts, without source parsing. Do not mutate old symbol/invoke interfaces.
3. All candidates remain available. modelcandidates chooses source/injection/process/owned as before and retains the selected typed payload and plan handle. No host winner selection. Wrong typed graph on a referenced name fails with a named signature mismatch; unused unsupported candidates remain legal.
4. E3 compares the source canonical TypeGraph against selected native declaration recursively (depth/kind/width/unsigned/alignment/layout tag/member offset/bit facts/children). Use librarytypes serializer or its fact reader, ignoring parser-local base/shape identity. Capture params from new PARAMDEPTH/PARAMBASE/PARAMSHAPE pools, not FPS_PARAM*16. Struct-vs-union tag is persistent model data. REGISTER/ALL_STACK ingress comes from the source prototype, not C guessing count.
5. E3 emits a wrapper with owned script stack slots and result storage. For >6 fixed arguments, input is ALL_STACK: read ALL arguments at wrapper FP+16+8*k. Aggregate input slots are pointers to source-owned by-value objects; dispatcher copies the objects into per-invocation native storage. FP is raw bits. Result aggregate goes into wrapper return storage (the source __rv_<function> ABI), then returns its pointer. Scalar FP returns rawbits; void has no result object.
6. Add a dedicated typed tape capability (.librarycallframe or explicit plan operand), not an untyped raw target escape hatch. lower checks model binding/plan capability and emits a GP call to the context dispatcher. The fixed GP argument record can hold plan_handle, frame_pointer, generation and reserved zeros. No encoder language ABI classification is needed.
7. C dispatcher validates owned context/generation/plan/count/byte extents, constructs pointers to typed scalar objects, calls ffi_call using the prepared declaration, and copies scalar rawbits/aggregate bytes back before script frame teardown. It must register arena cleanup on the context ScriptFrame: native code can recursively call a script closure, and explicit script exit must not longjmp past an unregistered malloc arena. Invalidating context clears ffi plans after closures/images are quiescent.

Shared ValueFrame: reuse the semantic contract of us_export_frame (slots,count,mode,result_kind,result_bytes,result). Do not serialize compiler-dependent C struct padding. If the wrapper writes a memory POD, freeze explicit field offsets/widths in a checked layout declaration; the host validates those offsets against sizeof/offsetof. On current 64-bit ABIs the C fields are pointer0/count8/mode16(kind20)/result_bytes24/result32 (size40), but this is a measured assertion to add, not an undocumented assumption. Prefer one shared libraryframe.h with the existing typedef moved without an ABI change.

## 4. First real bidirectional nine-parameter Pair test

Use the original host ABI type:
struct Pair { double d; int n; };
struct Pair host_exchange(int,double,float,int*,struct Pair,int,double,int,int);

Host defines host_exchange and injects its real address with the explicit recursive graph. Script declares that prototype and defines script_exchange with the identical nine-parameter prototype; script_exchange CALLS host_exchange and returns the Pair. Host obtains us_sym(script_exchange) and invokes that native closure with 1,2.0,3.0f,&four,{5.0,6},7,7.0,8,14. Expected Pair is 17.0/40. This is native -> actual compiled script -> actual injected native -> script return -> native, not only a synthetic closure.

Host target must assert all input values, set a counter exactly once, mutate only its local Pair copy, and return a result containing poison-free exact fields. After call, caller Pair still 5/6 and pointed integer unchanged unless intentionally changed. Repeat 100 calls, interleave two contexts, and add one native target that calls a different script closure on the same context to exercise nested arena/soft-stack restoration. Check count/mode, return bytes, status, stack canaries, and live closure after a failed registration. Seed/reference ABI computation independently expects 17/40; run ASan/UBSan host builds. Next run the identical source and declared graph on macOS x86, Linux and Windows native ffi—not only mock bytes.

## 5. Parallel file domains and sequence

A: shared typed storage/public declaration API/ffi native dispatcher: exec/c/librarybindings.h, libraryresolver.h, shared libraryframe/type helpers, libunisacc.h/.c, own host unit tests. Root owns final shared integration.
B: candidate V3 framing/selection and lower capability: exec/modelcandidates.py + its finite_rules, exec/modelbindings.py + rules, exec/lower/libraryimports.py + own tests. Must use exact frozen protocol from A; preserve legacy framing controls.
C: E3 graph comparison/wrapper: exec/parse2/libraryimports.py/rules + own probes; reuse librarytypes/libraryexports capture helpers without racing another writer. No new host C parser and no source grammar changes.
Root: protocol/PRD first, integration and frozen tests/builds. Native closure outbound path already added should be reused, not forked.

## 6. Full R10 remaining mechanisms, not scoped away

- Script -> native variadic: cannot be one symbol-level wrapper only. E3 must emit CallSite plans containing fixed_count, actual_count and fully converted tail types after default promotions (float -> double, small ints -> int/unsigned int as applicable). ffi_prep_cif_var uses that callsite shape; cache by generation + exact type vector, not function name. Apple ARM64 and Windows ARM64 varargs ABI differ and need actual native tests.
- Native -> script variadic: a plain variadic pointer cannot discover types/count. Add explicit us_call_typed or us_sym_typed specialised fixed closure contract. Do not cast a fixed cif closure to an arbitrary variadic function pointer and call that support.
- Function pointer parameters/callbacks: nested signature tag4 needs a schema with full signature and origin/native-vs-script/generation lifetime. Raw script labels are not native addresses. Native callback entry can reuse us_exports_symbol_frame with the nested graph; script wrappers must request/retain the typed closure, and native function pointers returned into script need typed native dispatch, not direct callr. Recursive source-defined nested types must use bounded graph references/cycle validation.
- Union/bitfield/packed/overaligned: libffi plain struct is insufficient. Either prove a matching libffi representation independently or implement model-produced NativePlan (GP/FP banks, stack, sret, byte-copy/extend operations) with generic mechanical ABI bridges. Do not fake union bytes as uint8 struct; that changes register classification. The graph must retain layout facts now so later plans do not lose information.
- Wider long double: source currently folds to double; distinguish source/native floating format before offering native long-double ABI equality.
- Windows SEH across guest frames remains separate from ordinary fixed-type returns. Existing native-pass evidence must not claim exception unwind safety.

Recommended next action: fixed nonvariadic graph + dispatcher + nine-Pair end-to-end first, immediately followed by two distinct variadic callsites and a typed nested callback. This slice advances the full objective but does not certify complete FFI or final 0.0.10.
