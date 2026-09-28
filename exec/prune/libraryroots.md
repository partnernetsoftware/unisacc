# Model library export roots

The host supplies complete E3 `USLSIG1` bytes as resource `\0library/signatures`.
It does not filter linkage or build a root-name list. The prune delta validates
all records before processing tape, interns each name, and keeps every external
(linkage 0), defined (1) function as an extra root regardless of supported ABI.
Internal records do not become export roots. Existing main/init, address-taken
and data handling remain unchanged. A missing public code label rejects.

The fixed wire declarations are libraryroots-schema.tsv. Complete fields follow
parse2/libraryexports.md: header/count, bounded nonempty ASCII name, linkage,
defined, variadic, total count, six-field return descriptor, stored count,
stored parameter descriptors, supported. Count is at most existing NAMEMAX;
stored count must equal min(total,8). Boolean fields are validated, defined must
be 1, type class is 0..6, width is 0/1/2/4/8, unsigned is 0/1. Depth/base/shape
are opaque u64 facts rather than independently reclassified C semantics. Name
uniqueness and exact resource exhaustion are checked. Every LE64 read checks
remaining bytes using unsigned comparison; no truncated read or length addition
overflow can succeed. Bad input rejects, never invokes conservative FALLBACK.

Without the resource, original prune output is unchanged. libraryrootscheck.py
requires private prepared JSON/TBL/NET/core inputs and compares C network with
Python simulator, checks the complete finite net domain, keeps unsupported
public functions, removes an internal dead function, preserves init/address
roots, and checks all resource prefix truncations plus malformed fields.
This is export retention, not a claim that raw addresses support native calls.
