# L1b production slice: POSIX sys6 private frame

Actual runs on 2026-10-09. Code landed in shared main as `819793f1` (author wanjochan, message `;`); this session did not amend that commit.

## Scope

Reference C, Python lowering, and lower delta snapshot all six tape operands into an 80-byte private frame before ABI argument writes. Slots 0–40 hold sources, 48 holds FP, 56 holds the original SP. An r7 source uses its original value. Snapshot scratch is x12 / r11. Generic syscall takes its number from slot 0 and five args from 8–40; named operations take six args from 0–40. FP restores from 48 and the frame is released after the gate.

Windows follows its previous route. Signal header, universal host entry, atomic SP updates, HOSTCALL, and .sys/.write private frames are outside this slice. This slice does not claim complete sigaction support.

## Evidence

- `tests/build_ref.sh /tmp/cdx-l1b-production-ref.c /tmp/cdx-l1b-production-ref` built the changed C reference.
- `sh unisacc.com -run research/l1b-sys6-acceptance/accept.c -- /tmp/cdx-l1b-production-ref /tmp/cdx-l1b-production-accept RUN`: accept ok, four macOS runs (C + handwritten tape on arm64 and x86_64). Four Windows images exactly match expect-ref.tsv.
- Native C delta fullcheck: all six targets match Python instruction args, metadata, labels and data. Both native executor and Python simulator check the fixture; raw A–D tapes additionally match on four POSIX targets. For this text entry only semicolon comment lines are removed, instructions are preserved.
- Persisted fullcheck fixture adds number/source permutations, direct r7, and r6/r7 mmap sources. `exec/lower/fullcheck.sh` and `armcheck.sh` pass with the private reference, including hello/fib, both architectures and >1,000,000 ARM argument slots.
- Delta-lowered raw tape assembled with the Python image writer matches production C reference images byte-for-byte: Linux images 4096 bytes each; macOS images 33250 bytes each. Tins omits zero data tails; comparison restores Padded(data, data_len) before writing the image.
- `sh unisacc.com -run seed/gen.c lower /tmp/cdx-l1b-seed-lower.json --full`: generated JSON equals Python lower --full output byte-for-byte.
- facts export --check: 30 tables, 0 differences. lower graphhash: 9 modes recorded, Windows two full hashes and default data hash unchanged. All nine fresh-order modes re-recorded, no baseline change.

## Remaining formal checks

New private product .com must run accept.c RUN; current actual execution above used the changed reference, not a rebuilt product package. Linux runtime and formal candidate lower/E5/E6/full queue are delegated to cc. Gate wrapper was not claimed green: invocation without UNISACC_FFI_X86_PROVIDER refuses before suite execution; direct lower suites above completed.
