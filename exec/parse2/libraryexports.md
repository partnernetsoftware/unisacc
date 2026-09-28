# Optional E3 library signature envelope

Resource key `\0library/symbols` accepts only LE64 value 1. Absent, E3 emits its
ordinary tape exactly. Present, it emits a binary envelope rather than adding
new tape directives. A host separates byte ranges before prune/lower; it does
not parse C or infer signatures.

Envelope: `USLTAPE1\n` (9 bytes), tape length LE64, metadata length LE64,
exact tape bytes, exact metadata bytes. No suffix is permitted.
Metadata: `USLSIG1\n` (8 bytes), definition-record count LE64. Each record:

1. Name length LE64, name bytes (no NUL).
2. Linkage u8 (0 external, 1 internal), defined u8 (always 1 in this version),
   syntactic variadic u8.
3. Declared parameter count LE64.
4. Return descriptor: six LE64 fields described below.
5. Stored parameter count LE64, then one descriptor per stored parameter.
6. Supported u8, placed at the end after every descriptor is classified.

Descriptor: pointer depth, raw E3 base code, raw shape ID, type class,
width in bytes, unsigned flag. Classes: void=0, integer=1, ordinary data
pointer=2, floating=3, function pointer=4, aggregate=5, unknown=6.
Raw composite identifiers are E3-local descriptors, not public layout or nested
signature specifications. Width=0 marks absent/unsupported concrete width.

Source facts are E3's existing FPS_FN/RD/RB/RSH/COUNT/PARAM/PSH tables. Named
function-pointer parameters bypass SIG.store in the legacy allocation path; a
metadata-only hook records their already parsed td/tb/type_shape at
FN.pfpdecl1, without changing parser type facts. Primitive
integer widths and signedness use tyinfo. The syntactic variadic flag is saved
at FN.body before FN.many also selects the stacked ABI for more than six
arguments. TOP.st preserves the actual static/extern token; linkage is not
inferred from mangled names. Records snapshot definitions before parsing the
body; prototypes do not invent definitions. Duplicate definition names reject.

The supported first slice is external defined functions with at most six fixed
integer parameters of any existing width, or ordinary data pointers, and an
integer/data-pointer/void result. Function-pointer, floating and aggregate value
ABIs and variadic calls are unsupported, explicitly marked. Internal functions
are recorded but not public exports. The existing E3 signature table stores
only the first eight parameter descriptors; stored count is min(total,8), and
more than six parameters is unsupported. This is not a claim that missing
parameter types or composite layouts have been reconstructed.

Library mode still requires main because the existing E3 entry-generation
contract does. A no-main module, public ABI wrappers and host symbol lookup are
separate integration work. The host must decode bounds, uniqueness and enum
fields and refuse unsupported signatures. A raw code address remains unusable
as a native function pointer until the declared ABI bridge/wrapper is applied.

`libraryexportscheck.py` independently asserts nine signature shapes with the
C network executor and Python simulator, rejects malformed resources, duplicate
definitions and damaged framing, and compares only ordinary tape/payload with
the classic reference. Metadata is never claimed correct merely because tape
matches. `--keep N/M` partitions the unchanged fixed keep-e3 list to check
resource-absent default outputs against a private classic reference.
