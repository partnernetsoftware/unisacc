# Managed byte memory

`memory.cx` owns byte buffers behind value handles. Import it through the normal
CX module mechanism, for example `#pragma unisa use core.memory` from `cx/`.
The implementation uses the existing implicit `calloc`, `free` and `memmove`
runtime APIs and contains no source include. All storage, accounting, tokens
and access helpers are module private. Public results and handles contain no
pointer. The frozen `cx/owned.cx` i64 example remains a legacy prototype; new
owner and borrow capabilities evolve here rather than in both modules.
The 64 MiB cap counts live buffer payload. Fixed slot metadata and the host
allocator's own overhead are additional; this is not a process RSS limit.

Every operation returns `cm_result`. Its `status` is a `CM_*` value. On failure
all other fields are zero and existing resources and bytes remain unchanged.
Successful creation fills `instance`, allocation/move fills `owner`, borrowing
fills `lease`, length fills `length`, and byte reading fills `byte`. Other
successes carry no value. Instance validity is checked first, then instance
membership and owner/lease token validity, then borrow permissions and ranges.

| Operations | Contract |
|---|---|
| `cm_create(byte_limit, buffer_limit, lease_limit)` | At most 16 live instances; 1–64 buffer slots and 0–128 lease slots per instance. Byte quota is 0–64 MiB. Invalid limits return `CM_SIZE`; full tables return `CM_CAPACITY`. |
| `cm_new(instance, length)` | Zero initialized, fixed length. Zero length consumes an owner slot and token but no byte storage. Requests over 64 MiB return `CM_SIZE`; quota or the module-wide 64 MiB live-byte cap returns `CM_BUDGET`. Actual `calloc` failure returns `CM_ALLOC`, without installing an owner. |
| `cm_length`, `cm_read_byte`, `cm_write_byte` | Length is metadata. Content reads refuse an active exclusive lease. Writes refuse every active lease, including an empty lease. Bytes outside 0–255 are refused rather than truncated. |
| `cm_copy_range(instance, dst, to, src, from, count)` | Both owners must belong to the same supplied instance. Destination must permit owner writes and source must permit owner reads. Overlap has `memmove` semantics. |
| `cm_move` | Requires no leases. Rotates the token without copying the allocation. Every old owner copy becomes invalid. |
| `cm_drop` | Requires no leases; frees storage and returns quota and slot. Every old owner copy stays invalid after slot reuse. |
| `cm_borrow_read`, `cm_borrow_write` | A bounded view of an owner. Multiple shared readers coexist; a writable lease excludes all other leases on that buffer. A lease on a different buffer remains independent. |
| `cm_lease_length`, `cm_lease_read_byte`, `cm_lease_write_byte` | View offsets are relative. Writing requires an exclusive lease; shared writes return `CM_READ_ONLY`. An exclusive lease permits reading its own view. |
| `cm_lease_copy_range` | Destination must be a writable lease; source must be a live lease in the same instance. Both view ranges are checked; overlap has `memmove` semantics. |
| `cm_release` | Invalidates that lease and all copies. Shared siblings remain active. Reusing the lease slot never revives an old token. |
| `cm_destroy` | Refuses live owners or leases with `CM_BUSY`. On success the instance and all copies remain invalid even after its table slot is reused. |

Ranges use `offset <= length && count <= length - offset`; they never validate
by an overflowing addition. A zero count permits the one-past offset and does
not perform pointer arithmetic or touch storage. Zero-count writes still obey
borrow permissions. Scalar access to a zero-length buffer/view is refused.
Cross-instance handles return `CM_CROSS_INSTANCE`. A stale owner/lease returns
`CM_INVALID_OWNER`/`CM_INVALID_LEASE`; a destroyed instance returns
`CM_INVALID_INSTANCE`. Tokens share one monotonic 64-bit counter and never wrap;
exhaustion returns `CM_TOKEN_EXHAUSTED` with the current resource intact.

`cm_unsafe_import` is the explicit foreign-pointer boundary. The destination,
borrow permissions and range are checked first; a null pointer is allowed only
for zero bytes. The caller must supply `count` readable bytes for the duration
of the call. Arbitrary C pointers cannot be validated by this API. No operation
returns an internal pointer, and there is no raw export or force-destroy API.

This is a single-threaded runtime facility without concurrent or signal-handler
reentry. Handle structs are ordinary
copyable and forgeable values: owner copies alias while the token is current.
The checks prevent stale access and conflicting managed operations through
these APIs. They do not establish linear types, handle secrecy, Rust compile-
time borrowing, safety for arbitrary C/unsafe pointers or races, or memory
safety throughout the compiler. Existing compiler consumers still require
separate migration and validation.

Run `tools/bound 55 sh ./unisacc.com cx/check_memory.cx`. The independent tests
exercise lifecycle recovery, both copy directions, view bounds, cross-instance
refusals, shared/exclusive borrowing, exact global byte limits and accounting.
Quota rejection is not a simulated native allocation failure. Native OOM and
64-bit token exhaustion are explicitly unrun, rather than counted as PASS.

The installed Linux x86_64 product `da4e7ef1` ran the final default-run batch
with 138 assertions and zero failures (`out/c43-core-memory-03.stdout`, empty
stderr). The actual `-cx-interface` output contains no pointer in result or
handle types; the only pointer parameter is the explicitly unsafe input bridge
(`out/c43-core-memory-interface-01.stdout`, empty stderr). These are finite
managed API observations, not compiler-wide memory safety qualification.

The first test compilation exposed the known member-struct return restriction
(`out/c43-core-memory-01.stderr`). Test helpers now copy the member into a local
struct before returning it, with all assertions retained. The direct positive
case remains `NEEDS_PRODUCT_CHANGE` in
`tests/pending/struct-return-member.cx`; that tracked case also ran with rc1
and the named refusal in `out/c43-struct-return-member-pending-01.stderr`.
Its original minimal red remains in `out/c43-core-memory-struct-return-01.stderr`.
