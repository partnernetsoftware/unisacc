# 2a address-free library declaration contract

This finite stage validates a complete canonical declaration stream. It does not
resolve providers, compare ABI signatures, or change any existing consumer.
The shared integer executor runs the generated network; no native parser is the
model implementation. The reference is the frozen expected-bytes column of
tests/librarycontract-cases.tsv, captured once from the Python seed (W1-F, 2026-10-23).

Input: `USLDEC1\n`, target byte 1..4, version byte 1, two zero reserved bytes,
then count u32 little endian (maximum 4096). Output: identical bytes except magic
is `USLCON1\n`. Rejection has empty stdout and a named `library contract`
diagnostic. No bytes are emitted before the entire input is validated.

Each record: length u32 (not including its own four bytes), id u32 (256+ordinal),
kind byte (0 function/1 data), callkind byte (0 legacy/1 typed/2 variadic/3 callback),
signature-version byte (0/2/3), flags byte (bit0 writable/bit1 injected-only),
name-length u32, signature-length u32, extent u64, argument-count u32,
variadic byte, three zero reserved bytes, name and signature payloads.
Length is 32+name-length+signature-length. Names are ASCII C identifiers,
1..1024 bytes, strictly increasing and unique. The stage independently checks
sorting; adapter-side sorting alone is not evidence of this property.
Arguments are at most 1024; signature payload at most 1048576 bytes.
Functions have extent 0 and writable flag 0. Data has callkind 0, argument-count
0 and variadic flag 0. Counts, lengths, IDs, reservations and the exact final EOF
are checked; truncated or trailing bytes reject.

Version 0 contains 48*(1+argument-count) bytes of flattened type descriptors.
Each descriptor has six u64 fields (depth, base, shape, kind, width, unsigned).
Base and shape must be zero: parser-internal identifiers are not core identity.
Versions 2/3 retain existing validated typed API blobs, checking exact
`USLSIG2\n`/`USLSIG3\n` magic and minimum 16-byte length. Their interior is opaque
in 2a. This is not ABI qualification or signature-mismatch evidence; a later
cut must establish and compare contract/provider signatures explicitly.

Actual local construction: seed/gen C and exec/build/gen.py produced identical
JSON; the network has 153 states, 154 banks, 157 hidden units, 6099 bytes.
The independently executed network is compared byte-for-byte to the reference
on minimal/ordinary/data/typed controls, truncations, lexical/order faults and
seeded byte mutations. Detailed execution receipt is in the parent 2a selftest.
