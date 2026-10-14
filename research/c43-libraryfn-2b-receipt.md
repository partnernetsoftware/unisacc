# 0.0.43 late binding 2b receipt

Actual candidate: `/tmp/c43-libraryfn-2b/cand-r2/unisacc-next.com`, sha256 `eb1da51a681938f17e5319d0b7df0cfd224693fa5c7c40768e00492292b3b1e8`; verified source record `62dd5834071cc9624c5ce2d5094fb4b6d434a521d2dcd799b5311c47901a8527`. Build/test snapshot `tree-r2` is main c08a1c00 plus this adapter/package patch. All shared/six-target/three-prep/package/driver construction steps rc0; five non-native compiler.c builds with its same-source reference also rc0 (compile only).

cc independently ran 28/28 rc0: com-tapedomain, com-latebind, lib-latebind (2a eight + 2b six), com-hostenv, com-forward-multi, com-forceinclude, com-iquote, all nine com-tapebin checks, abi-manifest, staticimage, facts-export, tapebin-shape, five library/host model checks, and three tapecheck shards. A/B actual addresses leave E3 and unbound lower fragments equal/clean; bound calls differ as expected; injected-only missing provider refuses at relocate with name and no callable image.

Parent actual seven suites rc0: lib-bindings, lib-bindings-registry, lib-resolver, lib-native-exports, lib-module-exports, lib-lifecycle, lib-callable-catalog. exec-driver-core-modes rc0/22s after bounded preparation, 27 mode/level plus six in-memory controls unchanged. forward-multi rc0/1s, three baseline controls. Private O0/O1/O2 compatibility diagnostic all rc0. Span helper dynamic-C selftest 19 cases rc0, first/middle/last spans, mixed digit lengths, exact other bytes and malformed/ownership/missing-provider atomic refusal.

Cold-cache record: original core outer55s rc142; separate exact compilerpack preparation outer58s first rc142, second original command rc0 after partial content-keyed prep finished. No larger watchdog, READY flag, fewer controls or result reuse. The known preparation/execution debt is not included as a fix here.

First 0fb55c55 sample regressed because adapter requested elf on run/O1, whose route ends at lower. Correct binding boundary is before the separate target/memory encoder route. Corrected r1 re-built byte-identical compiler package but different source record (private library runtime changed); old source receipt discarded. r2 includes subsequent Tape integer and derived-shape changes. Compile snapshots roll back together before commit, not mixing a new contract with old callable code. Getter observes only already-produced lower receipt: query after relocate, including a missing-provider failure.

Commit ABI generated from clean HEAD 17906286 + only the named adapter/package/doc paths, 420 entries; isolated check rc0. The final extra commit changes only per-suite derived-check scheduling. This differs from the r2 c08a1c00 test base only by later reference/seed-port/derived-check changes; those are checked by main commitgate/derived validation. The r2 product evidence is never relabelled as those later source bytes.

Boundary: only ordinary kind=0/callkind=0 POSIX functions migrate. Typed/variadic/callback/data and runtime bridges retain old address-bearing paths, Windows/L-4 not qualified. No full-lower address-free or ABI-signature-mismatch claim; no historical gate/stamp/env-pin or release mutation.

## Bound implementation and data identities

- `exec/c/libunisacc.c`: 80419 bytes; sha256 `d9e0ba44eb3db11c3690524aa8e978d04802b19b359d5292f01548b2ebfa6eff`
- `exec/c/libunisacc.h`: 6151 bytes; sha256 `5c2a95d54a254e783e767d3c2a4433d9b4845dafe750d6ad426476c38ab641a8`
- `exec/c/librarylatebind.h`: 6772 bytes; sha256 `28726a9ec811a27cc3f312dfed1627855a869e49be084aa9b28cf3b5380956ff`
- `exec/c/libraryrelocate.h`: 3635 bytes; sha256 `84b18a8aab3984262e6fc1efb492c7da750dcad534b1a22306f92aac7ec8d6dc`
- `exec/c/compilerpack.py`: 22414 bytes; sha256 `2291945190102cd8e1a9475d2ade555dc3df51e14d2240b6697a98f62e0f74e2`
- `seed/compilerpack.c`: 18656 bytes; sha256 `b8788a8e4819b30b6ff243755f11927cda036a62d7027c8ee20b5c24e45788eb`
- `cand-r2/compiler.pkg`: 1451167 bytes; sha256 `7c9a9ea86bee07a6f14f8ee0f2141d6f50e4e66f22b89458226e9ace6697932f`
- `cand-r2/unisacc-next.com.build.json`: 318 bytes; sha256 `fe119ae839e1c94eca31b82fc4ea07dfd28bd0df0611ad0d35fac9bb69e2e27b`
- `r2-selfhost-five.tsv`: 417 bytes; sha256 `4d85eaa10962cba08a3d66f738e700a3cebc47b79468f5bea60a2fafd0ac4965`

Native lib-bindings probe: package SHA above; native library SHA `374f61c89c2f567039e5af9b65ef5c796c35bf42eaa6aef878dd1436f6bba44c`, source probe SHA `6ba7fd5efb220814f84a689738ea3b9d116c3c1d30a2180f8d06c7a993df8233`.
