# Model-certified native ABI carriers

This prototype extends R10's NativePlan mechanism. It is not yet connected to
public union imports or `us_sym`; those actual source-compilation tests remain
red. The full union, bitfield and wide floating-point scope remains required.

The `exec/nativeabi` model consumes a single complete USLSIG2 declaration and
the exact `\0cli/target` resource. Its initial rule maps the eight-byte union
of `double` and unsigned64 to one unsigned64 ABI carrier. It checks both member
descriptors and layout facts, independent of order. A double-only union is not
accepted by this rule. The six profile rows describe classification rules,
not six-platform native execution evidence.

The internal output is `USLNCAR1\n`, followed by three little-endian u64
length-framed byte sequences: target, original USLSIG2, carrier USLSIG2.
The original wire is unchanged. Name, logical argument count, fixed calling
mode, and object extent/alignment agree. Changed root objects use one-to-one
byte copies; they are not expanded into independent scalar arguments.

`librarycarrierplan.h` decodes the complete model output transactionally,
checks the target and binds the original declaration. It does not classify
union members or prove carrier equivalence. Only a result produced by the
context's trusted model route may enter this internal interface; no public
API accepts an arbitrary caller-selected carrier as evidence.

`us_callable_make_carrier` owns both graphs. Registry identity and SCRIPT
slots/frames use the original graph; libffi layouts, CIFs and closures use
the carrier graph. Creation verifies storage and caches bounded copy flags.
Invocation never repeats graph classification. Opaque mappings containing
callback values are rejected because their active member is unknown. Legacy
callables keep their original path. Automatic native-pointer conversion still
needs certified-plan lookup integration.

Actual model, native mechanism, original-frame, register-exhaustion, failure,
canary and sanitizer evidence is recorded in
[the prototype receipt](../../research/r10-nativecarrier-prototype-evidence.json).
Mechanical SCRIPT hooks in these probes are not compiled script bodies.
Carrier classes that libffi cannot represent faithfully require the planned
BANK backend; wide source values already narrowed to F64 need source changes.
