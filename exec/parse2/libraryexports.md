# Optional E3 library signature envelope

Resource `\0library/symbols` accepts only LE64 value 1. When absent the model
emits ordinary tape byte-for-byte. Present it emits `USLTAPE1` carrying the tape
and model-generated `USLSIG2` declarations. Exact wire specification is
[`../c/librarysignature-v2.md`](../c/librarysignature-v2.md). No host C parser or
signature guessing is involved. The host validates recursive framing/layout;
prune uses the declaration to retain public definitions independent of support.

## Full parameter and layout facts

`SIG.store` is intercepted **before** its legacy eight-descriptor check. The
metadata pool uses `signature*1024+parameter` and a per-definition epoch mark;
no 1024-slot reset is emitted. Named function-pointer parameters have their
own `FN.pfpdecl1` capture because legacy parser allocation skips SIG.store.
The model rejects any capture beyond 1024 or any uncaptured declared parameter.
True ellipsis is saved before the legacy >6 stacked convention is selected.
V2 records REGISTER vs ALL_STACK explicitly, then all declared descriptors.

Primitive widths/signedness come from tyinfo. Struct/union sizes and alignment
come from SSZ/SAL; SMEM uses `sid*64+i` ordered member keys. Member offset,
pointer depth/base, array count and bitfield facts come from MOF/MPT/MBS/MAR
and BFO/BFW/BFS; `SB.go` captures union identity before enclosing state restores.
Recursive layout serialization uses a private frame bank, 32-level depth bound
and 16384 nodes per record. Arrays carry count, stride and recursive element
layout. Payload lengths are produced from actual model output blobs.

Fixed GP, data-pointer, FP and known plain struct/array descriptors may be
supported. Union, bitfield, flexible/unresolved members and function pointers
remain represented but unsupported. True variadic exports remain unsupported
pending a typed invocation interface. Static linkage records remain nonpublic.
The existing parser collapses source long double to its double descriptor;
**this format does not establish native wider-long-double support**. That
source type distinction must be added before the native ABI can claim it.

## Actual checks

`libraryexportscheck.py` runs C network and simulator, performs all-observation
`--check-net`, and independently decodes V2. Its 13 definitions include the
nine-mixed GP/FP/pointer/Pair function, Pair return size16/align8 and ordered
members, recursive Box/short[3], unsupported union, named function-pointer,
true varargs, seven parameters and seventeen parameters. It asserts full
counts, mode, widths, offsets, alignment, layouts and support flags.
Malformed resources, duplicate definitions and damaged framing are rejected.
Ordinary tape and framed tape payload are each checked against a private
classic reference. `--keep N/M` partitions existing keep-e3 inputs to check
resource-absent outputs independently; a passed fixture is not the whole list.
Native us_sym closures, actual foreign ABI execution and remaining R10 ABI
shapes need their separate integration evidence.
