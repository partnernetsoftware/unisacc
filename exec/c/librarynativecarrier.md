# Model-certified native ABI carriers

## Current product seam

R10 public imports and `us_sym` use model-certified carriers. This is no longer
an unconnected union8 prototype. The `exec/nativeabi` model consumes an original
USLSIG2 graph plus the exact `\0cli/target` resource; the finite target policies
and carrier recipes live in `rules.tsv`. Classification runs in the model,
not in the host adapter. Six profile rows are not six-platform execution proof.

Current domains include natural scalar unions, recursively laid-out natural
structs/arrays, recursive callback signatures, and fully occupied natural16B
unions. Each domain has its own rejects and executable qualification. Padding,
packing, over-alignment, complete bitfields and wide FP are not implied by
those domains. See [the model specification](../nativeabi/README.md).

## Identity and storage

The internal output is `USLNCAR1\n` followed by three little-endian u64
length-framed sequences: target, original USLSIG2, carrier USLSIG2. Name,
logical argument count, calling mode, and object extent/alignment remain bound
to the original graph. A composite remains one logical argument; its carrier
members are not independent parameters.

`librarycarrierplan.h` transactionally decodes the complete model output and
checks its target and original declaration. Only the context's trusted model
route may produce this certificate; no public API accepts a caller-selected
carrier as proof. Host checks enforce storage, graph pairing and ownership;
they do not choose an ABI class.

`us_callable_make_carrier` owns both graphs. Registry identity and SCRIPT
slots/frames use the original graph; libffi layouts, CIFs and closures use the
carrier graph. Conversion plans are cached at creation. Recursive callable
fields require conversion, while a data-only union can use bounded raw COPY.
An opaque union containing callback objects is rejected because its active
member is unknown. Native-pointer adaptation uses the context's certified-plan
registry and retains callback signature topology.

## Evidence and dependency qualification

[Current model/candidate evidence](../../research/r10-union16-candidate-evidence.json)
records all24 deployed networks' equality to their retained tables and28
specific affected formal gates. This is not a final release gate receipt.
[Actual public union16 qualification](../../research/r10-union16-qualified-public-evidence.json)
records144 macOS ARM/Rosetta variants,43200 native calls and21600 SCRIPT callbacks.
The earlier [failed system-libffi matrix](../../research/r10-union16-public-evidence.json)
is retained. An independently reproduced Darwin x86 mixed-return dependency
failure is handled by a qualified official libffi3.5.2 provider, not by changing
the model's class to hide the failure.

`buildlibrary.py --ffi-provider DIR` validates source/artifact identity and runs
an actual mixed-class pressure probe before publishing. Static consumers link
both delivered archives; dynamic consumers set the documented rpath. Details
and byte receipts are in [BUILDING.md](BUILDING.md) and
[provider delivery evidence](../../research/r10-ffi-provider-delivery-evidence.json).
The [early prototype receipt](../../research/r10-nativecarrier-prototype-evidence.json)
remains historical: its mechanical SCRIPT hooks were not compiled script bodies.

## Remaining ABI obligations

Bitfield source facts must retain anonymous entries, zero-width barriers and
layout modifiers before complete source certification. Shared storage units
need model-generated transport projections; callable objects inside such
records need explicit conversion edges. [The design](../../research/r10-bitfield-model-plan.md)
and [independent native controls](../../research/r10-bitfield-native-controls.json)
are separate from product support. BANK, wide FP, six actual platform aggregate/
callback qualification and Windows SEH/lifecycle remain R10 obligations.
