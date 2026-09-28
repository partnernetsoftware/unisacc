# Optional raw library symbol map

`library/symbols` is an optional resource named by the exact byte key
`\0library/symbols`. Its only valid value is eight little-endian bytes encoding
1. It requires a complete memory binding; malformed values and a request on a
file-image route reject before writing the image. Absence retains UNIMEM1 bytes.

When enabled, the model writes:

- `UNILIB1\n`, then the same four LE64 fields as UNIMEM1: text byte count,
  data extent, stored data byte count, entry text offset.
- Exactly the same text bytes and stored data bytes as UNIMEM1.
- `SYMS1\n`, then an LE64 record count.
- Each record: kind u8 (0 code label, 1 data), name byte count LE64,
  absolute address LE64, then that many name bytes with no terminator.

The map is computed inside the delta from validated TIns name spans and final
layout tables. x86 labels resolve through the relaxed instruction-offset table;
ARM labels contain offset+1. Data symbols use the final data shift. All code
labels retain their exact names, including `main` and `__init` when present.
Each data name is retained; `g_` additionally declares a stripped-prefix alias.
An empty alias or a collision between any exported names rejects. Shared code
labels at one offset are allowed when their names differ.

These are **raw addresses**, not public symbols or system-ABI function pointers.
The map does not infer linkage, signatures, visibility, or calling convention.
The compiler's x86 argument registers differ from native ABIs; ARM returns via
its x7 software stack. A host must decode the bounded format and separately
adapt an explicitly supported signature through the declared ABI bridge before
calling code. The checker never calls a raw map address as a native function.

`librarysymbolscheck.py` checks x86 and ARM with both the generic C network
executor and Python simulator, full finite network/table equality, default byte
identity, map addresses, reference code bytes, malformed resource rejection,
non-memory rejection, and data/code alias collisions. Run it on a frozen private
snapshot with an outer watchdog no longer than 60 seconds; each subprocess has
its own 25-second bound.
