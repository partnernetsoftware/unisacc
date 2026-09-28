# R10 typed variadic callsites — implementation design

This is a design, not an implemented or qualified public variadic API.
Existing fixed USBIND3 and USLTAPE1 remain compatible.

## One E3 pass, model-owned declarations

USBIND3 format2 describes a variadic candidate template, with the format1
payload shape: dispatcher, owned template handle, USLSIG2 prototype, support.
The prototype records only fixed arguments and has variadic1/ALL_STACK.
The host stores target and explicit graph; it does not prepare a call cif yet.

E3 compares its actual source prototype to the selected template through the
existing canonicalizer. Fixed arguments use the complete declaration pool and
existing conversions. Tail arguments undergo model-owned C promotions:
float becomes double; narrow integer/Bool becomes int; other supported scalar,
data-pointer and ordinary aggregate descriptors remain. Value and descriptor
changes must agree. No host format-string scanning or source/type guessing.

At CL.a2 after conversion, retain effective depth/base/shape under an allocated
site id and epoch. Capture id/epoch/active state must participate in existing
nested-call saves and restores; na/fid alone is insufficient. Store all <=1024
arguments. Validate fixed_count<=total, and variadic source mode stays ALL_STACK
even when the fixed prefix has <=6 arguments.

Emit USCPLAN1 with count and length-framed records: site_id, selected_template
handle, fixed_count, concrete_signature_length, concrete USLSIG2 signature.
The concrete graph has variadic0/count=total and already-promoted tail types.
It is a call descriptor, distinct from the variadic function prototype.

If requests exist, USLTAPE2 carries tape/export/call lengths and those three
byte streams. Ordinary output without requests stays USLTAPE1. Host splits
framing, then runs only tape through E4/prune/lower. No request is a public root.

## Source priority and callsite wrappers

Confirm selected native calls only after all source definitions are known.
Retain call-output spans so delta can rebuild tape: a later source definition
keeps the original call; otherwise use a unique site wrapper. Native requests
are emitted only for actual selected candidates. No host winner selection.

Each ALL_STACK wrapper copies total argument slots and uses six words:
(template_handle, slots, result, total_count, site_id, reserved_zero).
The variadic dispatcher is distinct from the current fixed dispatcher.
Scalar and stable aggregate result handling reuse the existing wrapper path.

## Host mechanical preparation and lifetime

After E3, decode all length-framed requests transactionally. Check unique site
ids, live template membership, fixed_count consistency and declared bounds.
Prepare each concrete call with us_native_plan_add_variadic. Publish all plans
only after complete success; failures release the temporary set. Runtime looks
up the template/site pair, verifies count/reserved fields and executes ffi_call.
No second E3 run or host patch of opaque pointers in tape is required.

Plans and ffi objects stay owned until context/binding/image invalidation.
Per-invocation arena registration precedes ffi_call, preserving explicit exit
and nested callback cleanup. Reuse existing frozen generation semantics.

## First real end-to-end test

Public typed registration → actual source compile/relocate/us_sym → native
va_arg function returning Pair. Fixed prefix double1.5/int7; tail signed char
-3, ushort24, Bool1, float2.5, int-pointer9, Pair4/11 and six doubles. Validate
exact results, unchanged caller objects, O0/O1/O2 on ARM64/Rosetta100 times,
nested expressions and two same-name sites with different call graphs.
Controls: missing fixed args, bad prefix graph, unpromoted descriptors, unknown
site, truncated request, and later source definition preventing native calls.

This does not complete typed function-pointer signatures/callbacks, unions,
bitfields, wide floating formats or the remaining six-platform obligations.
