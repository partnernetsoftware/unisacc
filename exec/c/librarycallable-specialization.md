# Explicit variadic script export specialization

`us_sym_typed(ctx, name, declaration, bytes)` returns a **fixed native ABI**
entry for one declared argument shape of a variadic script export. The caller
supplies a single complete USLSIG2 record with the export name, variadic=0,
mode=ALL_STACK, its result and all promoted actual argument types.

The model-produced export remains the authority for the real variadic
prototype. Its result and every fixed-prefix type must match; actual count
must be at least the prefix count. Tail float32 and integers narrower than
int are rejected: declarations describe values after default promotion.
The existing graph decoder, concrete-signature validator, registry and ABI
bridge are reused. No C source parsing or new executor instruction is added.

For `Pair scriptvar(double, int, ...)`, a declaration of `(double, int,
double, int) -> Pair` returns code callable as
`Pair (*)(double, int, double, int)`. A separate `(double, int) -> Pair`
declaration gives a zero-tail entry. Neither is a general `Pair (*)(double,
int, ...)`: such a pointer cannot discover an arbitrary caller's tail.
Ordinary `us_sym` continues to refuse the unspecialized variadic export.

Same-generation identical complete signatures reuse an entry; distinct shapes
have distinct closures. The registry owns copied graphs, so declaration bytes
may be released immediately. Invalid or truncated declarations return NULL,
set us_error and leave existing entries intact. Recompile, relocation and
free invalidate all returned pointers; callbacks must be quiescent first.
Error propagation uses the existing script/native owner boundary and ABI-zero
failure return. It does not perform a nonlocal jump across a native caller.

Current native evidence: macOS ARM64 and Rosetta x86_64, ASan/UBSan,
O0/O1/O2 with 100 calls each, promoted double/int tail reads, zero-tail,
eight invalid declarations and preservation of old entries. Other native
platforms and union/bitfield/wide-FP layouts still require qualification.
