# S-17 迁移日志（历史原文，2026-09-26—2026-09-28）

从 `68cdd50:prd.md` 的非结构化尾部逐字保留。当前规格见 [主 PRD](../prd.md)。这里记录的是各日期/提交的状态，并不自动成为当前限制或最新通过记录；原文内相对路径仍按原仓库根目录解释。生成字节账的当前维护副本在主文。

原文 SHA-256：`b2c4a45ffd2ef260b4ee573aae7350a0d6c858cf7a41eb4f6633d024e10ffaf3`。

<!-- original-log:begin -->
- 2026-09-26 (S-17, gen2 54cbaee): multi-dimensional arrays measured and done. `b[i][j][k]`: each subscript that is not the last multiplies by (element size × the remaining dimensions) and adds, with no load. A DIM table per variable holds the dimensions. Also: printf with any conversion other than %d/%% is a real variadic call. mkdump now runs autoinc on the dump path. The old E3 has 1 DIFF (p67) on the corrected input. Structured E3: 2,371 states, 96 equal, 0 differ.
- 2026-09-26 (S-17, gen2 up to 3404ebc): new measured rules, all done in the structured E3.
  - **sizeof** is a constant `imm` and its operand emits nothing.
  - **Pointer difference** is sub64 then `.div` by the element size, giving a long. **n + p** scales the pushed n in place.
  - **Local initialisers** emit: the value; `imm r2, slot; sub64 r1, r6, r2`; then a store at the variable's width. **Comma declarators** add their stars on the base depth.
  - **A statement `*p;` alone** computes the address only.
  - **++, -- and op=** work on every width and on pointers (the step is the element size). Postfix loads raw. Prefix and op= load masked and mask an unsigned narrow result again.
  - **unsigned int** loads and casts are masked to 32 bits. Operators mask both operands, use the unsigned form, and mask the result. Hex constants above INT_MAX are unsigned int.
  - **Tooling:** a name defined twice is now a build error (it had silently merged DSCALE with DIMSAVE).
  - **Status:** 2,828 states; 127 equal, 0 differ. The old E3 matches 110. What remains is where the old one covered and the new does not yet: p43/p58/p59 (the pairing of long and unsigned int), p47 (parameter), p53 and p7, p65 (double), p76/p81/p83/p84 (width).
- 2026-09-26 (S-17): **E3 switched to the structured delta** (97fd265, exec/pipeline/stages.tsv).
  - **Switch gate:** `E3KEEP=exec/parse2/keep-old-e3.txt` passes with exit 0. All 110 files the old E3 matched are kept. In total 141 files are equal and 0 differ.
  - **Sizes, old vs new:** states 5,233 → 3,210; entries 1,345,252 → 825,517; json 29.9 → 17.3 MB.
  - **Product fix (9052993):** a constant now resets curuns, curflt and curstruct. Before, a unit's first expression saw curstruct 0 ("a struct"), so `1 + 10u` lost its unsignedness.
  - **compare.py (cdx review):** tool failures, an empty file list and an empty keep list now all fail. reject-both is an observation only.
  - **Status:** still a structured state machine / lookup-table prototype. E3 is not yet a net.
- 2026-09-26 (S-17, cdx review of 97fd265): **exact byte count.** Both deltas are serialized the same way (`json.dumps(separators=(",", ":"))`) and built from gen.py at acb6f58 and gen2.py at a8ae6fb.

  | | raw bytes | gzip -9 |
  |---|---|---|
  | old | 29,938,744 | 3,147,912 |
  | new | 17,262,831 | 1,903,000 |

  The "23.9 → 13.3 MB" in the first version of the 97fd265 message was wrong. The message was amended before it was pushed.
  - **Speed:** not yet compared per file on the same file set.
  - **Dependency:** only the old *generator entry* is retired. gen2.py still imports exec/parse/gen.py as its token reader, table assembler and constants, so that file stays live code.
  - **Product regression:** added tests/c/b_constkind.c, for the constant-kind fix 9052993. It checks `-1 < 1u` as a unit's first expression and again right after a struct operand, plus a hex constant above INT_MAX.
    - cc prints `0 0 0 1`.
    - The fixed compiler prints the same at -O0, -O1 and -O2.
    - The compiler before the fix (9052993^) prints `1 1 0 1`.
- 2026-09-26 (S-17 / reviews by cdx and bdy-ds4flash), measured:
  - **E3 now reads the type table.** OPX's integer tail asks type.tsv the way the product's binary() does (549e88d): `ck = type(t1 + t2)` gives the masks and the spelling, `res = type(t1 op t2)` gives the result. The fixed set exec/parse2/keep-e3.txt (164 files) stays all equal. States went 3,212 → 3,014, but gen2.py grew by 37 lines: the rules moved into the table, the code did not shrink.
  - **printf is always a real call** in both front ends: the C one in 991d337, the Python one in 18c8f22.
  - **main falls off its `}` with 0** in both front ends (C99 5.1.2.2.3). This was a bug exposed by the real printf: corpus 00206/11/12 exited 12.
  - **%p prints 0x** followed by the hex digits.
  - **The referee ledger is now data**: research/referee.tsv, ratcheted to gold ALL by tests/docs.sh.
  - **New external referees:**
    - `tests/prec_audit.py`: 291 of 324 operator pairs agree, 33 are undetermined, and the level domain 1..10 is asserted.
    - `tests/tyinfo_audit.py`: 27 facts agree.

    External referees now cover 4 of 18 stages (type partly, abi, prec, tyinfo).
  - **ablate** is in release.sh as 8 shards. prec is in ablate's list; peep and opinfo are registered as not covered on the Python path.
  - **pfconv:** since printf became a real call, pfconv is asked only by the fallback for a unit with no printf declared.
- 2026-09-26 (S-17), **the C executor**, measured (4b37a77):
  - **What it is.** `exec/c/run.c` is the generic executor in C; it copies `exec/pp/sim.py` action for action (55 actions). `exec/c/tbl.py` renumbers a JSON delta into an integer table, and nothing in either file is specific to a language.
  - **E3 check.** `exec/c/check.sh` compares its verdict with sim.py's over all 225 files. All 225 match: 164 identical tapes and 61 identical reject reasons.
  - **E2 check.** 32 files (examples/ and tests/c/a_*) give byte-identical output, with headers read through the file dictionary.
  - **Speed**, one file (tests/c/a_for.c), wall time including the table load:

    | stage | C | Python |
    |---|---|---|
    | E3 | 0.07 s | 0.44 s |
    | E2 | 0.01 s | 0.06 s |
  - **Remaining steps toward the slice:**
    - E1 on the same executor (its nets have their own simulator today);
    - a binary table format;
    - the table as nets.
  - **Product fix.** `-nostdinc` no longer adds `<stdio.h>`. A printf there takes the compile-time lowering (`do_printf`); `tests/cli.sh` now runs this path.
- 2026-09-26 (S-17), **one executor, three tables, source to tape** (exec/c/chain.sh). One generic C executor, `exec/c/run.c`, runs all three stages:
  - E2, the preprocessor delta. It now follows the product's rule that any `printf(` pulls in stdio.h.
  - E1, the typed lexer.
  - E3, the parser.

  **Result:** on examples/ and tests/c (113 files), 54 compile from source to a tape identical to the reference's, 59 are not covered by some stage, and 0 differ. Checked alone, each stage agrees with its Python executor: E3 on 225 files, E2 and E1 on 32 each.

  **Still missing:**
  - chain.sh in the gate;
  - a binary table format;
  - E4 and later (lowering and encoding), which have no delta yet;
  - the tables as nets.
- 2026-09-26 (S-17), **table sizes after row defaults** (acc894f). Each byte-keyed or r-keyed row now lists only the keys that differ from its most common (next, seq) pair; that pair answers every key not listed. The tables are still text, not yet a binary format.

  | table | size |
  |---|---|
  | E3 | 9,772,470 → 368,976 B |
  | E1 | 155,080 B |
  | E2 | 72,226 B |
  | executor binary (cc -O2) | 54,120 B |

  So the whole front end, from source to tape, is one 54 KB executor plus about 0.6 MB of text tables.
  - **Speed:** one E3 file takes 0.02 s warm.
  - **Checks:** check.sh gives 225/225 the same verdict as the Python executor. The chain of three stages keeps its 54 files equal and is in the gate (exec-chain, gate 30/30).
  - **Still missing:** a binary format, the stages after E3 (lowering and encoding), and the tables as nets.
- 2026-09-26 (S-17), **E4 met on the tested inputs** (522f746, 1d4a953, 9a705d4). `exec/opt/gen.py` turns the tape optimiser into a delta that runs on the generic executor. The reference's -O0 tape run through it equals its -O1 and -O2 tapes, byte for byte:
  - on examples/ and tests/c (113 files at each level);
  - on unisacc.c itself: a 3.9 MB tape, 11 s on the C executor.

  **The delta:** the -O1 table has 191 states; the -O2 table has 904 states.

  **What -O2 contains**, each computed by the delta itself, not by the executor:
  - the per-line facts of `ol_prep`;
  - blocks, labels and targets;
  - the liveness solver;
  - the push/pop rule with the carry through r3..r5, and `ol_local`;
  - `stfuse`;
  - the peep relations. The peep table (1,632 keys) and opinfo's `simple`/`acls`/`bcls` columns are loaded into memory at START.

  **Gate:** exec-e4 (28 s) and exec-e4self (12 s); the gate is 33/33.

  **Still missing:**
  - E1–E3 coverage: E3 matches 164 of 225 files, and the source-to-tape chain 54 of 113;
  - E5 (lowering and encoding, six targets), E6 (image writing) and E7 (.com layout);
  - the tables as nets.
- 2026-09-26, **working rule** (the owner caught this): after every batch of product source changes, rebuild unisacc.com (`make com`) and run `tests/gate.sh --com`. Committing the sources alone is not enough; the .com had not been rebuilt for most of a day. It was rebuilt at 04a127b's tree (1,342,224 B), and gate --com passed 41/41. The exec/ migration is still a parallel path that unisacc.com does not use. S-17's switch rule, which moves the default path to the new implementation only after its gate passes, has not been applied yet.
- 2026-09-26 (S-17), **fixed product miscompile: `*"literal"`**. At product reference 4b37a77, `int f(void) { return *"z"; }` returned the literal's address truncated to int: `.lea` without the required load. Product fix 48a5c4b resets the string literal's complete type state before setting its `char *` descriptor. `tests/c/b_strderef.c` covers preceding struct and pointer-to-pointer expressions and ordinary string pointers. Independent takeover verification: system cc, rebuilt `.com` at -O0/-O1/-O2 and Python `--drive built` all print `122 67` / `284 Q pq`, exit 0. Python was already correct. E3's temporary rejection was removed; `exec/parse2/probes/s35d.c` now matches the fixed reference on both executors and joins the keep list after all old 191 entries passed in four bounded shards (192 kept, not a fresh full-corpus count). The broader product probe remains outside E3 coverage (`expected ;`), and no extra syntax was added.
- 2026-09-26, **takeover build verification**: product sources at 48a5c4b, `unisacc.c` regenerated by `make com`; artifact `unisacc.com` is 1,342,320 B, SHA256 `d8ac8275157951044dcd22de0677984e81473387eff969af751fdabd583f4dd3`. Independent `tests/gate.sh --com`: 41 suites, 0 failed, 92 s aggregate wall time, each suite bounded at 60 s (longest 45 s). Host macOS arm64; Rosetta fat execution 114/0. Linux/Windows native runs were not repeated. Existing known cases remain: corpus 4 known failures, ccparity 1 known case. E3 chain retained 68/68; E4 and E5 gates passed. Local working artifact rebuilt, no release or push; exec remains outside the product execution path. Detailed local logs: `/tmp/unisacc-takeover-48a5c4b/` (temporary, not archival).

- 2026-09-26, **continued refactoring after takeover**: E5 x86 integer division/remainder (`.div/.mod/.udiv/.umod`) share a delta encoding path and the existing ALU/MEM helpers; no new executor primitive. `exec/enc/check.sh`: 32 fixtures pass on both executors, including hand-worked signed-division bytes and scratch-register rejection. One-off comparison: all 1,372 combinations of four ops and the seven non-stack tape registers, 59,682 bytes, identical on C and Python executors. gen.py 533 lines (net +50 from this batch's baseline); 416 states, JSON 2,116,894 B, text table 20,033 B. This adds hand encoding rules; it does not reduce rule-source code or turn tables into nets. Product inputs are unchanged, so the previously validated `.com` is unchanged. Full S-17 remains active: E1–E3 completeness and failure contracts, remaining E5/ARM/lowering, E6 images, network execution, compact per-ISA executor and single-model E7 product adoption all still require implementation and verification.

### Takeover continuation: FP encoding and gate reliability (2026-09-26)

- E5: all 24 FP operations encoded by delta, with hand rules in `exec/enc/fp.py`;
  no executor primitives added and no network claim. Both executor byte checks
  plus independent host-C execution: `exec/enc/check.sh` 35 ok, 0 bad; FP execution
  550 cases, 0 bad. One-off exhaustive register comparison: 5,292 combinations.
- `tests/fat.sh`: build/sign/incomplete runs now fail instead of silently skipping;
  every subprocess has a 60 s bound, output and exit code must match the reference.
  hello/fib normal run: 2 pass; a PATH substitute failing only fib's build leaves
  hello passing but makes the suite fail (1 pass, 1 mismatch, rc 1).
- This is not complete test coverage: E3 remains partial; E5 is not lowering or
  full six-target image generation. These changes have not rerun Linux/Windows.
  No product source changed, so the existing .com artifact is unchanged.

### E5 non-WinAPI gate continuation (2026-09-26)

- Migrated syscall and Darwin carry correction as delta transitions, no new
  executor primitive. Known non-WinAPI metadata accepted; winapi and malformed
  carry rejected. `exec/enc/check.sh`: 38 ok, 0 bad, including the 550 independent
  FP execution cases retained from the preceding batch. Syscall bytes compared,
  syscall execution not claimed.
- Real hello/fib lowering audit now identifies address-dependent forms as the
  next encoder boundary: .lea, setmem, setreg mem, argsave, argvget. Full lowering,
  address layout and image generation remain to be migrated; no product switch.

### Complete lowering payload transport (2026-09-26)

`exec/enc/tins.py` now has an explicit full form for target, data bytes,
DATA_BASE-relative symbols, src_os, data_len, BSS and relocation offsets.
The instruction-only form remains compatible. `roundtrip.py` compares every
TargetProgram attribute and typed instruction/meta field, not just code.
Three real programs across six targets are now fixed gate checks; E5 gate
43 ok, 0 bad. This is transport, not six-platform execution or header support
in the delta encoder. Relaxed layout/address encoding remains the next step.

### Address encoding reaches complete real code sections (2026-09-26)

- Delta now computes Linux/macOS x86 layout from final relaxed text size and
  emits .lea, setreg mem/addr, setmem, argsave and argvget at final positions.
  Format constants are read declarations; layout/RIP algorithms are hand rules
  in `address.py`. No new executor primitive and no network claim.
- `realcheck.py`: whole hello/fib lowering (including stdio, no filtering),
  10,009/10,072 instructions each; exact reference bytes on Linux and macOS x86.
  macOS x86 execution of delta code wrapped by the existing Python image writer
  gives expected outputs and rc 0. E5 gate: 48 ok, 0 bad.
- Header payload is now consumed for target and symbols; data-related headers
  are retained but image writing is not migrated. Lowering, remaining targets,
  image emission and product adoption remain unfinished.

### E6 first image route: Linux x86_64 ELF (2026-09-26)

- `gen.py --elf` compiles the ELF image writer into the delta, sharing the
  address/text encoder. Generic executor unchanged. Data decoding, pointer
  relocation, zero-tail trimming, entry point and two load segments are emitted
  by ordinary actions; no Python image writer in this execution path.
- Whole hello/fib ELF output equals reference (45,366 / 45,358 bytes). Two
  synthetic relocated-pointer cases match on C and Python executors, plus
  independent entry/segment/address/file-length assertions. Out-of-data
  relocation rejected on both. `exec-elf` added as a bounded gate batch.
- Input lowering is still Python; Linux native execution not run in this batch
  (the x86 Lima VM is stopped). Other formats/architectures and frontend
  completion remain required; current .com unchanged.

### E6 Linux execution evidence (2026-09-26, f506dc6)

The actual delta-produced hello.elf and fib.elf were copied unchanged into
`minicon-lnx-x86_64` (Lima/QEMU on the ARM host). Each ran under a 10 s guest
watchdog: hello stdout `hello from C99\n`, fib stdout `55\n`, both rc 0 and
empty stderr. This is Linux x86_64 emulation, not a native x86_64 machine.
The VM was initially stopped, started for this check and stopped afterwards
(shutdown confirmed). The separately running default ARM VM was untouched.

Lowering remains the next algorithmic dependency: raw tape data directives
must be decoded and reordered by zero_last before scratch cells are assigned;
register mapping, entry setup, syscall shapes and adjacent push/pop fusion
must then be executed by the delta. Serialising an already lowered program
does not complete that dependency. Its interface must eventually consume E4's
actual tape output, rather than moving the Python lowering behind an adapter.

### Lowering starts at raw E4 tape data (2026-09-26)

`exec/lower/gen.py` now generates a 153-state data-pass delta. It parses .str
(including byte escapes) and .bss, aligns data, performs zero-last reordering,
rewrites symbol addresses and allocates Linux scratch cells. Non-data code
and label meaning remain unchanged. It outputs a data header plus tape code,
not yet target instructions. No executor primitive was added.

The fixed `exec-lowdata` gate includes five edge fixtures on C and Python
executors (escapes, empty data, all-zero data, aliases and terminal alignment),
plus actual hello -S input and fib output produced by the E4 delta. Bytes and
symbols match the Python referee; code/labels survive. Python parsing and
zero_last only judge results, not supply intermediate answers to the delta.
This is a hand algorithm compiled into a state table, not a new network.
Instruction lowering and remaining architectures are still pending.

### Raw source through six deltas to Linux ELF (2026-09-26)

The 1,063-state full lowering delta now consumes raw tape and handles Linux
x86_64 register mapping, entry setup, argument/syscall rules and adjacent
push/pop fusion. It reads regmap/enc/abi/reloc facts; control algorithms remain
hand-written in the generator. `.print` and other targets remain unsupported.
The full typed TargetProgram comparison passes a 41-instruction fixture on
both executors, hello (9,791 instructions) and optimized fib (5,530) on C.
Data-only checks remain green after introducing the full-mode hook.

`exec/pipeline/elf.sh` generates six tables, then runs source through E2/E1/E3/
E4/lowering/ELF on one generic C executor. No Python stage processes source
once tables have been generated. `exec-srcelf` checks hello and fib: optimized
tapes match the product reference, and complete ELF files match the Python
image referee (28,982 and 28,970 bytes). `exec-lower` checks lowering separately.
These two examples establish an end-to-end route, not C99 completeness, six-
target support, network implementation or replacement of the product path.

The two new source-to-ELF outputs were executed on local Lima Linux x86_64
(emulated on the arm64 host): stdout `hello from C99\n` and `55\n`, exit 0,
empty stderr. An initial manual hello expectation was wrong; after checking
the source the rerun passed. The VM started for this check was stopped;
the already-running default VM was left alone. This is runtime evidence for
these two outputs, not a Linux suite pass.

### End-to-end fixed set expands to 68 (2026-09-26)

All 68 existing chain keep inputs now complete the six-stage source-to-ELF
route. Each optimized tape equals the product reference and each full ELF
image equals the reference lowering/assembler/image result, with encoded
instruction count checked. The set is retained independently in
`exec/pipeline/keep-elf.txt`; `exec-srcelf` now rebuilds tables and checks all
68, failing on a missing input, rejecting stage, nonzero reference or byte
mismatch. The complete check passed under one 60-second watchdog. Linux
execution evidence remains the two examples above, not all 68 programs.

### Self-source blocker: literal tokens and escape decoding (2026-09-26)

E2 and E1 accept unisacc.c. E3 formerly stopped at MODEL's multiline adjacent
string token: its reader stopped at the first newline. 6bf71f7 makes the token
span quote-aware and shares a byte decoder between pooling and character
array initialization. Ordinary adjacent literals, hex escapes and one-to-three
digit octal escapes are covered by s37/s38; the old 192 E3 keep inputs stayed
equal and the two new probes agree on both executors (194 retained). Wide
prefixes remain unsupported. This is added hand parser logic in a delta,
not network construction or removal of the original parser.

The new probes found a product defect, fixed independently in 8a87421:
C decode assumed exactly two hex digits and could read through a closing
quote; it also reinterpreted octal results 'r'/'x' as escape instructions.
Python limited hex to two digits and lacked four standard simple escapes.
The fixed paths decode each escape once and stop at the literal boundary.
`tests/c/b_stresc.c` agrees with system cc in the reference, Python and rebuilt
.com at all three optimization levels. Kernel provenance was regenerated.

Frozen-tree gate --com, JOBS=2: 45 suites passed, none failed, 183 seconds total,
each at most 60 seconds; exec-srcelf (68 programs) 46s, fat 115/0, nativeboot,
bigclosure 6/6, difftest_o 342/0, kernel 23/0. This run was macOS arm64 plus
Rosetta, not a fresh Linux/Windows product gate. The rebuilt .com is 1,343,904 B,
SHA256 736a6c86ff71b1b34e29de969ba6501572f622cb37a7a0455073b0d617378f3c,
from the product inputs in 8a87421. Nothing was pushed or released.

Self-source E3 now passes the large MODEL declaration and next stops on an
array bound constant expression (64 * 1024). Full frontend self-compilation,
other targets, constructed-network execution and product-path replacement
remain incomplete; this is not a self-bootstrap result for the new route.

### Self-source: constant bounds and postfix values (2026-09-26)

E3 now evaluates integer bound/enum expressions using the shared prec rows
and generic integer actions. `constexpr.py` adds 49 lines of hand parser
rules; it is not a newly constructed network. Current limits: signed 64-bit
arithmetic without full C conversion/overflow handling; sizeof and casts
are not yet covered; a zero divisor is refused even in an unselected arm.
These are prototype coverage limits, not C language restrictions.
Global numeric expressions reuse EXPR once, saving the resulting initializer
tape and source end position for later emission, preserving source-order
label allocation. String and function-call values now reuse POSTIX, clearing
stale array rank before subscript processing. No executor action was added.

All old 194 retained inputs stayed equal; s39 (bounds/enum), s40 (global
expressions), s41 (literal subscripts), s42 (call-result subscripts) and the
product b_strderef regression pass on both executors and join the fixed set:
199 equal, 0 lost/differ/tool-fail. gen2.py 1629 -> 1633 lines, plus the new
49-line evaluator; rules were added/reused, not removed. Delta: 3647 states,
938024 entries, JSON 19854065 B, text table 410815 B. Full current self-source
E2/E1 succeeds; E3 stops at optimizer `t < 0 ? 1 : bl_live[base+t]`, whose
integer arms differ in width. This remains an incomplete frontend self-run.

The walk found a product type-projection defect, fixed separately in fb5e167:
binary i64 & i64 asked the table's address-of sentinel row, so `(5L & 3L)+1`
scaled by eight and yielded 9. Binary & now asks the | arithmetic row, leaving
unary address-of unchanged; E3 uses the same contextual projection. System
cc, the rebuilt native reference at -O0/-O1/-O2 and Python built all give
`2 0 2 8 1` for b_longand. That regression is still outside E3 coverage because
of its sizeof-expression form, so it is not added to E3's retained set.
Full rebuilt-product gate results are recorded after the frozen run below.

Frozen b06170a/fb5e167 product run: gate --com 45/45, JOBS=2, 191s total,
with each suite bounded by 60s. This is macOS arm64/Rosetta evidence only.
Rebuilt unisacc.com: 1343904 B, SHA256
58df0c97b398a600b344b967be3a2b3033422f057a5b753d5d78d2385ac1d7b5.
The .com b_longand check at -O0/-O1/-O2 also agrees with host cc.
The next self-source blocker exposed another reference defect: for
`sizeof(k ? 1 : c)`, `sizeof(k ? c : 1)`, `sizeof(k ? c : c)` with char c,
host cc gives 4/4/4, native gives 1/4/1, Python gives 4/1/1. Mixed signed and
unsigned arms also inherit only one arm's type. This must be corrected
against the common-type rule before it is used as a migration reference.

### Conditional integer types follow the shared table (2026-09-26)

The product's two frontends now ask type(t1 + t2) for two integer arms of
?:, retain that result's width and signedness, and mask a narrow unsigned
selected value at the merge. Neither the first nor the second arm alone
supplies the type. b_condkind agrees with host cc: `4 4 4 0 0`, in native
-O0/-O1/-O2 and Python built; the pre-fix outputs are recorded above.
E3 reuses its existing CKT and RESD procedures for the same decision,
replacing the earlier hand check limited to signed int/long. The old 199
retained inputs pass; s43 agrees on both executors, bringing the fixed set
to 200. Delta 3659 states, JSON 19929833 B, text table 411066 B.
Current self-source E2/E1 succeeds; E3 next stops at main.c's block-scope
`static char dname[520]`. No self-source success is claimed yet.

Frozen 47ef8c3 / product 1f3ba5b validation: gate --com 45/45, JOBS=2,
187s total, each suite <=60s. macOS arm64/Rosetta only; no new Linux/Windows
product-suite claim. Rebuilt .com 1345056 B, SHA256
44e0fe5ca0194ef567a92181b7e9984619ebf2e2a7963fecb5b46f33865bf25d.
The .com conditional regression also matches cc at -O0/-O1/-O2.

### S-17 current self-source milestone (2026-09-26, bb4b4b8)

The generic C executor now runs E2 -> E1 -> E3 on the complete current
unisacc.c. Its 3,918,249-byte tape equals the native reference exactly;
exec/parse2/selfcheck.sh enforces this in the gate. This is a frontend
self-source result, not bootstrap of the replacement compiler or net execution.
E3's fixed set is 207 equal (old 200 retained, plus s44-s49 and b_static).
The seven additions also agree through the C executor. The added rules cover
block static storage and scalar initialisation, saved array dimensions and
enum shadowing, int-return function-pointer casts, repeated tentative global
definitions, and the product's declared syscall list. gen2.py is 1,678 lines
(+42 from the preceding 1,636), plus statics.py 45 lines; rules and source grew.
3918 states, JSON 21,449,911 B; this is still a generated transition table.

Product 7d50875 rejects automatic objects referenced from static initialisers;
sizeof non-VLA objects remains legal and discards both output buffers. Python's
constant sizeof now also accepts expression operands. staticinit checks nine
compile-only rejections, including addresses and array decay; b_staticsize
checks the legal path. Calls and ordinary global-value reads in C static
initialisers remain known nonconformances; this is not a full constant-expression
validator. E3 isolates automatic-storage initialisers with a named not-covered
rejection. Static aggregate initialisers and other function-pointer cast return
types remain outside this slice's coverage.

Frozen validation: gate --com 49/49, JOBS=2, 186s total, every suite <=60s;
exec-chain 68/68, source-to-ELF's fixed 68 retained, fat 118/118. macOS arm64
and Rosetta only. Product .com: 1,345,808 B, SHA256
60b396834a71d96e93a942c1c33d75e8b4e955709b6dbaffad069f51ca150908.
No push or release. The shipped compile path is still the handwritten compiler.

The preceding self-source tape also passed E4 -O2 equality. Full lowering hit
alarm 60: its data pass expands 609,909,154 bytes of declared zero storage,
scans and copies them byte by byte, and emits hex. Next is sparse zero-region
layout with unchanged symbol addresses and image bytes, not a higher watchdog.
The full six-target route, constructed-network runtime, compact executor and
E7 product adoption remain outstanding.

### S-17 sparse self-source ELF milestone (2026-09-26, 1775315)

The zero-storage blocker above is resolved. Lowering keeps virtual storage
separate from literal bytes; ELF records the omitted zero tail in p_memsz.
No executor action was added. The complete six-delta route now processes
unisacc.c (product source 7d50875) into a 671,404-byte Linux x86-64 ELF,
byte-identical to the reference. Generator/runtime source: 1775315.
SHA256: d8f7586e1d0250064c248cfbb1bfe8ad8dc8021abf6f9f6e86da5d50b58d1979.

Actual provenance: exec/pipeline/elf.sh produced
/tmp/unisacc-self-route/final-pipeline/unisacc.elf. That image was copied as
n1 into /tmp/unisacc-delta-self-7ba6f76 in Lima minicon-lnx-x86_64. There,
`n1 -O2 -b lnx/x86_64 unisacc.c -o n2` and the same command using n2 to
produce n3 gave N1=N2=N3 with the hash above. Each command was bounded at
50 s in the guest and 60 s on the host. All six examples/apps programs ran
via n1 -run, exited 0 and matched host cc stdout. This VM is emulated on
arm64, not native x86 hardware; it was stopped afterwards.

Frozen gate --com: 51/51, JOBS=2, 191 s total; exec-selfelf took 16 s.
Standalone exec/pipeline/selfcheck.sh (table generation, six stages, reference
tape and ELF comparisons): 15.68 s wall, 14.56 s user, 0.34 s system.
The 60 s watchdog is unchanged. Supplemental alias/duplicate and sparse
relocation boundary checks passed on both executors after the full gate;
the sparse-input substitution now asserts exactly one matching data line.

This output is the existing C compiler compiled through transition tables.
The tables are not constructed networks, and the shipped .com has not switched
to the executor. Other targets, broader coverage, network execution, compact
executor and E7 adoption remain open. No push or release.

### S-17 ARM64 encoder started (2026-09-26)

The remaining-target work now includes exec/enc/arm.py: integer constants,
register ALU/shifts/comparisons, multiply, nop and tape callr/ret. ENCSPEC
supplies ALU3 and condition values; packing and tape ABI sequences remain
hand-written generator rules. Same generic executor, no new action. 220 states,
10,059 B text table. This is not a network or a product-path change.

Targeted armcheck passed: both executors match 60 reference instructions / 384
bytes; eleven out-of-domain cases reject. On macOS arm64, 540 operations agree
with system-assembler functions, ten full-width immediate values execute as
expected, and the tape call/return bytes match independent expectations.
Added exec-arm to the gate; the previous 51-suite run predates this entry, so
no new full-gate total is claimed. Memory, labels, metadata, FP, image formats
and integration with ARM lowering remain to be implemented.

ARM64 memory follow-up: load64/store64 and .ld/.st support widths 1/2/4/8,
scaled imm12, signed imm9 and MOVIMM/ADD-or-SUB long-address fallback. Opcode
constants come from emit_arm's LDS/STS/LDU/STU declarations; selection and
packing are still hand generator rules. Fallback rejects a clobbered x16 base
or store source; x16 as load destination is allowed. callr now also rejects
x7, which its software push changes. regmap maps tape r7 to x7 (SP), not an
ordinary value register; x17 has no regmap row.

Frozen gate --com: 52/52, JOBS=2, 191 s, max suite 40 s, macOS arm64/Rosetta.
ARM memory initially checked 92 instructions; afterwards a test-only addition
covers -2^63 offsets for all widths. Targeted rerun: 100 instructions / 768 B
match on both executors, four memory-domain rejects, 64 native load/store
cases match C memcpy and signed values. The earlier 550 native arithmetic and
constant checks still pass. 313 states, 13,213 B table; no new executor action,
no product source change, no .com rebuild or publication needed for this slice.

ARM64 branch follow-up: section-local jump/jumpz/call and shared/end labels now
run on the same executor. Two passes measure actual lengths, discard provisional
bytes, then resolve offsets. Calls use the BL position after three setup words.
RELFIELD supplies bit width/shift; layout/range logic remains hand-written in
the generator. Duplicate/undefined labels reject. 367 states, 16,297 B table.
Targeted armcheck passes all earlier checks plus ten branch fixtures on both
runtimes, six hand-worked byte expectations, seven rejects, eight synthetic
helper boundary contexts for imm19/imm26, and native loop/direct-call results
5/42. No new full-gate run is claimed after the previous 52/52. Metadata,
remaining ops, ARM lowering and images are still outstanding; .com unchanged.

ARM64 integer lowering forms: .div/.udiv/.mod/.umod, sext, addi/subi/lsli and
.frame now encode through the same executor. Remainder rejects x17 source
clobbers; frame changes software SP x7. These are hand-written encoding rules,
not table-derived semantics. Immediate field checks use full width before
narrow selectors; only imm accepts unsigned positive 64-bit bit patterns.
Targeted armcheck retains all prior checks and adds 28 instructions / 152 B
on both runtimes, ten domain/scratch rejections and 24 native functions checked
against defined C arithmetic/frame results. 524 states / 22,355 B. No new full
gate claim; product sources and .com unchanged. Metadata, setreg/address forms,
FP, syscall setup and image/ARM-lowering integration remain outstanding.

ARM64 FP follow-up: all 24 FP operations emit on the generic integer executor,
with v16/v17 used by generated machine code. Opcode declarations are read from
FARITH/FCMP_INV/FP_OPS; packing/conversion sequences remain hand rules. 72
operand/alias fixtures / 960 bytes agree on both runtimes; the shared native
C referee passes 550 cases on macOS arm64. x86's 48 fixtures, real hello/fib
encoding/execution, and its 550 numerical cases were rerun successfully after
the harness gained an explicit arm64 mode. 744 states / 37,347 B table. No full
gate rerun or .com change is claimed. Reference equality and finite numerical
cases are not full FP semantic proof.

Cross-target known UB behaviour: ARM SDIV/UDIV return zero for divide by zero,
and SDIV returns MIN for MIN/-1; x86 IDIV traps. C-defined-input tests exclude
these cases, so a difference on them is not evidence of a delta regression.

ARM64 TIns interface: setreg imm/reg, host-sp spinit and POSIX gate forms now
encode. Metadata keys follow tins.META; duplicates, unknown/empty fields and
incompatible reloc/gate/carry values reject. Tags retain argument types.
Windows annotations on POSIX gate remain informational. Five whole fixtures
match both executors/reference, three worked byte strings and seventeen rejects
pass; all prior ARM checks remain green. 822 states / 41,672 B. Inspection of
real hello lowering found remaining address/setup forms (.lea, setmem, setreg
mem, argsave, argvget); no whole real ARM program claim yet. Product unchanged,
no new full-gate run claimed. Next is actual payload/address layout integration.

### S-17 ARM64 real code encoding (2026-09-27)

armlayout now derives text/data addresses from measured text size and format
constants, resolves data/code symbols and emits ADRP/ADD and POSIX setup forms.
Whole hello/fib lowering outputs encode identically: lnx/arm64 54,352/54,696 B;
osx/arm64 54,476/54,820 B. macOS images execute with expected output/exit 0.
No instructions were filtered. Python remains the lowering producer and image
wrapper; ARM delta lowering/image output and product adoption are still open.
Payload headers are retained, not certified as valid image data by this encoder.

Address fixtures cover both layouts, page crossings and data-over-code name
precedence (same as reference); actual ADRP/ADD bytes are decoded to assert
addresses in fixtures and all four real encodings. Thirteen malformed header,
unknown symbol and scratch alias inputs reject on both executors. 981 states /
48,726 B table. Gate --com: 52/52, JOBS=2, 196 s, ARM suite 13 s and Linux
self-source suite 16 s. Test-only strengthening after the gate passed the full
ARM suite again. Product source/.com unchanged; no push or release.

### S-17 Linux ARM64 ELF writer (2026-09-27)

ARM --elf reuses elfimage.py; e_machine and label representation are explicit
parameters, with no architecture-specific image logic in the executor. Shared
ELF byte output is self-contained and relocation-index digits are bounded
before overflow. ARM requires a data declaration and Linux target. ELF mode
1,079 states / 55,788 B; no new runtime primitive.

hello/fib complete ELF images match reference (57,654/57,646 B) and execute in
existing Lima default aarch64: expected stdout, no stderr, exit 0, guest timeout
10 / host 60 s. Hashes: hello 1f227471a8d2cb471fcc2eb8b7a2e90319266b982663be152138d3575937e2b2;
fib d4efd5598e6d77f9c342189b350fad35287ad63d5a1af8171c86ac7a71e9181f.
No VM was started; the existing VM was left running. Python still does lowering.
ARM text checks, both image suites, large sparse extent and x86 self-source
671,404-byte ELF equality all reran green. exec-armelf added to gate, but this
batch does not claim a new full-gate count. Product .com unchanged. Next: ARM
lowering on the executor, then remaining formats/targets and network adoption.


### S-17 ARM lowering and source route (2026-09-27)

Shared data/register/ABI lowering now selects Linux ARM64. armfuse.py encodes
sext and immediate peepholes, including the 32-instruction dead-after scan,
as ordinary actions. These are migrated hand rules, not removal of algorithms
or a constructed network. No executor primitive added. ARM lowering table:
1,482 states, 134,562 B compressed text.

Both executors match setup/sext fixture (96 instructions) and immediate fixture
(131), including 4095/4096, signed constants, power-of-two multiply through
2^63, labels, aliases and 31/32 scan boundary. C executor matches hello/fib
full typed lowering and current self-source: 103,254 target instructions plus
labels/data/metadata. x86 self-source route rerun: 671,404 B equal.

TARGET=lnx/arm64 elf.sh now runs all six deltas from source. Optimized hello
28,982 B sha256 54008849528a13e1d1168e7c3e706d55d9cdac08e8f9f8f56f261257b6d50e34;
fib 28,970 B sha256 ab4f92611388643a4a21805ae089d7f0f26798458ab3de52a5fb8c4bdbe738fd.
Both match ua_ref -O2 ARM ELF and run in existing Lima default (aarch64), exact
stdout, rc 0, empty stderr; guest 10 s, host 60 s. VM was already running.
Self-source reaches encoder but rejects; .zero is still unmigrated. This is
not an ARM self-bootstrap or E7 adoption. exec-armlower added to gate; no new
full-gate result claimed. Product source/.com unchanged, no push/release.


### S-17 Linux ARM64 self-source closure (2026-09-27)

ARM .zero now stores XZR using existing memory transitions, 8/4/2/1 pieces.
Both-executor byte checks plus seven native zero-fill cases passed (memory
suite total 136 instructions / 1,128 B, 71 native cases). Negative lengths,
fallback scratch aliases and signed offset wrap reject before output.

First full self-source image differed by two data bytes: E2 still declared
__x86_64__. E2 now takes the explicit Linux target, preserving the default x86
path. elf.sh passes TARGET through E2, lowering and encoder. Complete ARM ELF
now equals the reference: 716,458 B, SHA256
5e95f0dc9efbcbfacf3552d5e1b505a5fe9010b4cf57ecec8f828002c8bc07ef.
Input product source remains 7d50875. Host outputs /tmp/unisacc-arm-self-target;
Lima default guest /tmp/unisacc-arm-self-target/n1,n2,n3. n1 is the six-delta
output; n1 -O2 -b lnx/arm64 unisacc.c -> n2; n2 similarly -> n3; all three
cmp and SHA256 identical. Guest compiles timeout30, host each command<=60.
VM was already running and remains running. Tables are still lookup tables,
generated by Python; this is not E7's executor-based product switch.

ARM selfcheck is now a gate item with target-specific -S and ELF reference.
Both ARM (716,458 B) and x86 (671,404 B) selfchecks reran green. ARM ELF table
1,112 states / 56,816 B; text table 1,014 / 50,134 B. Full ARM suite green.
No product-source or .com change; no push/release. Full-gate status separately.


Post-70f7c9a full local gate --com: 55/55, 115 s, slowest suite 43 s;
ARM and x86 source-to-ELF selfchecks each 16 s. Actual concurrency was the
wrapper default 4: JOBS=2 outside term.sh was not forwarded. Do not report 2.
Use explicit `tests/term.sh env JOBS=2 ...` until wrapper propagation is fixed.
After this gate, only selfcheck's target-macro fixture was strengthened: the
six Linux names must each expand to 1 and the other architecture/Apple/Mach/
Windows names must be absent. Generated E2 and product -E both checked; ARM
and x86 selfchecks reran green with this fixture. No new full-gate run claimed.


### S-17 Darwin lowering preparation (2026-09-27)

`gen.py --full --osx [--arm64]` reuses the POSIX lowering transitions. ABI
facts select Darwin syscall numbers and svc80/syscall; gates carry=true and
argsave takes loader registers (false), with @src_os osx. Tests compare all
TargetProgram fields, not only code bytes. Darwin ARM fixture93/immediate131
on both executors, hello/fib12395/5824 instructions; Darwin x86 fixture41 on
both, hello/fib9791/5530. Both Linux suites reran green. Mach-O image writing
is not yet a delta; no complete Darwin source-route claim.

Test wrapper repair 81294d2 forwards explicit JOBS, TARGET and E2/E3/E4/chain
settings through Terminal, quoting apostrophes/newlines and preserving empty
versus unset. Actual Terminal probe checked those values and exit7 propagation.
No compiler/product-source change, no .com rebuild or push.


### S-17 Mach-O prerequisite: SHA-256 transitions (2026-09-27)

The existing Mach-O writer requires ad-hoc page signatures. sha256delta.py
compiles SHA-256 padding, schedule, rounds and digest emission into ordinary
A64/LDX/STX/input/output actions; no runtime hash/image primitive added. The
64 round constants and eight initial words are read as declarations from
src/back_image.c, with counts/indices/ranges checked. SHA256 takes a blob,
appends 32 bytes, restores input and returns. This is an explicitly migrated
algorithm, not a constructed network and not yet a complete Mach-O writer.

shacheck: 17 inputs, both C and Python executors, each hashes twice in one run.
Three fixed known digests (empty, abc, long standard text) plus deterministic
binary inputs checked against hashlib: 55/56,63/64/65,119/120,127/128 and
4095/4096/4097/8192 boundaries. All passed. Standalone harness has 30 states,
5,786 B table. exec-sha registered in gate; only targeted suite run this batch.
Next integrate header/layout and CodeDirectory/SuperBlob, then full Mach-O
byte comparison and native execution. Product .com unchanged; no push.


### S-17 shared Mach-O writer (2026-09-27)

arm.py/gen.py --macho select one machodelta.py for both architectures. Common
payload decoding, sparse extent, relocation and trimming stay in elfimage.py;
Mach-O emits its load commands, aligned segments, LC_MAIN and ad-hoc signature.
CodeDirectory/SuperBlob and per-4KB SHA hashes use ordinary actions. No native
signer or hash/image-specific runtime primitive; the Python writer is only a
test oracle. Static format constants/templates come from unisa.image.macho.

Read-only review found DATA at 93e6 could overlap SHA W/K at 95/96e6. DATA,
W and K now occupy distinct 1<<40 regions (byte extent <2^31). A 2,000,100-byte
stored data fixture with relocation at 2,000,000 matches the complete reference
image (2,047,650 B) and independently validates all 497 page hashes. Small
empty/16/16385-byte data fixtures compare both executors. A Linux-target
payload in Mach-O mode is rejected by both runtimes, with no output.

Complete hello/fib images: ARM 82,722 B each, x86 66,210 B each, match reference.
Both run on macOS arm64 host (x86 via Rosetta), expected stdout/rc0/no stderr.
Every signature page is separately checked with hashlib. ARM states1332, x86
states1189. Both ELF suites reran green after shared changes. Registered
exec-macharm/exec-machx86; only targeted suites run this batch, not full gate.
Input producer is still Python lowering for these tests; source-to-Mach-O,
self-bootstrap and E7 product adoption are not claimed. Product .com unchanged.


### S-17 both macOS source routes/self-bootstrap (2026-09-27)

elf.sh now selects all four POSIX OS/arch combinations, preserving Linux's
.elf path and writing .macho for Darwin. E2 supplies __APPLE__, __MACH__,
__unix__, the architecture macro, __LP64__ and __UNISA__; selfcheck explicitly
checks all six values and absence of other target macros using both E2 and
product -E. It compares the target-specific -O2 tape and full image.

osx/arm64: 743,202 B, SHA256
d2bf5cc5f485f3b7ad16f01dbd9b459d4a130d84206c9dea1e8fa1f00425cc55.
osx/x86_64: 693,666 B, SHA256
f971def83f4cb8e6cb085c84c62ce28b8c36637b3a5eb8188bd7b41474f6409f.
Both six-delta images equal the reference; N1 compiles unisacc.c to N2, N2 to
N3 with -O2 -b TARGET, all bytes identical. ARM native, x86 via Rosetta on
macOS arm64. Artifacts /tmp/unisacc-osx-{arm,x86}-source. Product source is
still 7d50875. Added native bootstrap to supported-host selfcheck, otherwise
explicitly reports not executed. Individual commands remain <=60 s.

Mach-O additionally rejects a header/text size mismatch, padding overrun and
file/signature extent exceeding 32-bit fields. Both Darwin selfchecks passed
after these guards; Linux x86 source selfcheck still 671,404 B equal. Registered
exec-macself and exec-macxself; no new full-gate result claimed for this batch.
Windows/PE and executor/model product adoption remain outstanding; tables are
not neural networks. No product-source/.com changes, no push/release.

### Product follow-up: target predefinitions and `-U` (2026-09-27)

Moved native `predef()`'s `-U` loop after all target predefinitions. Previously
Darwin, Windows, architecture, `__LP64__` and `__UNISA__` names were defined
after the removal loop. `tests/cli.sh` now checks every such name across all
six targets, including command exit status: 64 passed, 0 wrong. This does not
claim full ordered `-D`/`-U` compatibility or add `-U` to the delta preprocessor.
Regenerated `unisacc.c`, rebuilt `unisacc.com`: 1,345,792 B,
SHA256 `d1583b832d7bb4d0020a8448af3c008dd597cfcc4bcf1ba3d063d0a9051ffe82`.
Frozen-tree local `gate --com`: 60 suites, 0 failed, 138 s total, each suite
bounded at 60 s; macOS host evidence, not a new Linux/Windows native run.
Log: `/tmp/unisacc-u-gate.log`. No release or push.

### Windows typed lowering (2026-09-27)

Windows data/extra-stack layout and full typed lowering now share the existing
transition generator with POSIX. `wincheck.sh` covers x86_64 and arm64, all
catalog WINAPI calls, mmap's argument truncation, setup/save/restore and
hello/fib; compares complete typed fields and layout, not selected bytes.
Both executor fixtures and C-executor real tapes passed. Existing four POSIX
lowering comparisons passed unchanged. Added `exec-winlower` to gate; the last
full gate remains the preceding 60/60 run, not a claim for the new 61-suite list.
No new executor primitive. This is lookup-table migration, not network runtime;
Windows encoding/PE/native execution and E7 product adoption remain unfinished.

### Windows ARM64 text setup (2026-09-27)

Raw encoder now computes PE text/import/data addresses and emits addressed
spinit, winsave/winrest and winstdh, including indirect GetStdHandle calls.
`armcheck.sh` passed within its 60 s bound: three OS address layouts and page
crossings on both executors; prior native arithmetic, memory, FP and Darwin
program execution retained. Table: 1,073 states, 54,766 B text. Encoding rules
remain explicit in generator; declarations supply imports and standard-handle
numbers. No new runtime primitive. PE files/native Windows and the remaining
WinAPI sequences are not yet covered. Log: /tmp/unisacc-win-armsetup-final.log.

### Windows ARM64 gate text (2026-09-27)

ARM delta now encodes reference-supported WinAPI bodies and return conversions;
winargs retains the explicitly declared machine-code parser template. Tested
fixture 132 instructions / 2,772 B on both executors; Windows hello/fib complete
text 56,304 / 56,648 B equals reference. Seven bad contracts reject, including
winrest with result other than x0 (review22). Existing ARM native arithmetic,
FP/memory and Darwin execution suite passed. Logs /tmp/unisacc-arm-winapi.log
and /tmp/unisacc-arm-winapi-regression.log. Added exec-armwin gate entry;
no new full-gate result claimed. PE writer and native Windows execution remain.

### ARM64 PE writer and first native Windows evidence (2026-09-27)

Complete ARM PE emitted by the delta, not a Python image wrapper. Whole-file
reference comparison plus independent structure checks passed for sparse/BSS,
relocation duplicates/order/page boundaries and 2 MB data; small fixtures on
both executors. ELF/Mach-O regressions passed. Initial duplicate relocation
sort corruption was caught and fixed; fixture retained.
Windows 11 ARM64 UTM actual runs, separate process timeout 30 s: hello/fib
exit 0, stdout equals host cc. hello 59,392 B SHA256
`3de62f3522f36f6831d70a99c8be451231bc3246323fe36f9b17786ab5fc1153`;
fib 59,904 B SHA256
`a59d240caa6f4ab8d861224b6ea0761b210782c2ad0c8b1f65a7f76604774ffa`.
Artifacts /tmp/unisacc-delta-pe; logs /tmp/unisacc-pe-final.log and
/tmp/unisacc-pe-native.log. VM was stopped after use. Reference lowering is
still the input producer in this test, not the full source pipeline. Windows
x86 encoding/PE route, self-source, E7 and network runtime remain unfinished.

### Windows ARM64 full source route and bootstrap (2026-09-27)

E2 now selects the five Windows predefines; source pipeline accepts win/arm64
and writes .exe. Source is unisacc.c at f360ba0, SHA256
`11ef59801d8c18f637e67b2a5b0e988ad31baec1859fabdd06dc879935574307`.
Six-delta output, reference image, Windows-native N2 and N3 are identical:
741,888 B, SHA256
`a831d9b1000f7661e6db4346cd9f17a1bd0c0c568a12c40b78a4208430ffa08f`.
Actual Windows 11 ARM64 execution through UTM, independent exit receipts,
30 s per guest compile; started VM stopped after use. Artifacts in
/tmp/unisacc-win-arm-source and log /tmp/unisacc-pe-bootstrap.log.
The self-source input has zero data relocations (@relocs -); quadratic sorting
is a general scaling limitation, not a measured self-source timeout.

Code-frozen local gate --com: 64 suites, 0 failed, 147 s total; exec-winself
17 s, exec-armwin 3 s, exec-pearm 4 s, every suite bounded at 60 s. Log
/tmp/unisacc-win-source-gate.log. The peer's prd-only 1435ed3 was retained during
this run; product/code inputs were unchanged. Windows native bootstrap was a
separate run, not implied by the macOS gate. No push/release. This closes the
fifth source-to-image target, not Windows x86_64, full frontend coverage,
network inference or product adoption. WINARGS_BODY remains a named template.

### Windows x86 setup after relaxation (2026-09-27)

The encoder retains per-instruction setup operands, measures fixed sizes, and
regenerates winsave/winrest/winstdh/winargs after branch relaxation and PE
address layout. A 359-byte fixture with a shortened preceding jump equals the
reference on C and Python executors; wrong return register, target and arity
are rejected. WINARGS_BODY is a declared template, not a migrated algorithm.
No executor primitive added. WinAPI gate bodies and full x86 PE/source closure
remain pending. Existing x86 fixture regression passes; product unchanged.

### Windows x86 full source route and bootstrap (2026-09-27)

Eleven WinAPI bodies and ABI-selected return conversions now run as deferred
x86 encoding transitions. The final RIP bytes follow relaxation, with fixed
length rechecked. No executor primitive was added. The reference's short
zero-extending immediate form is preserved (the first gate fixture caught
an overlong MOVABS in the new implementation, corrected before acceptance).
132 real lowered instructions / 2,381 B agree on both executors. Shared PE
fixtures include unsorted/duplicate DIR64 entries, a page crossing, and
2,000,100 B data with a relocation at 2,000,000; the machine field is checked.
Real hello/fib PE files are 46,080 / 46,592 B, identical to the reference.

Six-stage source pipeline and selfcheck pass for win/x86_64 (target macros,
optimized tape and complete PE). Source is f360ba0's unisacc.c, SHA256
`11ef59801d8c18f637e67b2a5b0e988ad31baec1859fabdd06dc879935574307`.
N1/N2/N3 are all 694,272 B, SHA256
`19bfcf3d809ac42e6120a4cbc033b9002de3d556c1634c6f59ad23647b312fc8`.
Actual execution: Windows 11 ARM64 UTM, **x86_64 emulation**, not x86 hardware;
each guest compile has a 30 s timeout, command has a 60 s outer watchdog.
hello/fib each exit 0 with host-cc expected output. VM started here was stopped
and is not left running. Artifacts: /tmp/unisacc-win-x86-source,
/tmp/unisacc-winx86-pe; logs: /tmp/unisacc-winx86-bootstrap.log and
/tmp/unisacc-winx86-exec.log. WINARGS_BODY remains a template. All six measured
self-source routes exist; frontend completeness, networks, compact executor
and E7 product adoption remain open. Product sources/.com unchanged.

Frozen-tree gate --com: 67 suites, 0 failed, 264 s aggregate with JOBS=2;
each suite <=60 s. exec-x86win 2 s, exec-pex86 2 s, exec-winx86self 16 s;
ARM PE/self-source regression also green. Log /tmp/unisacc-winx86-gate.log.
Windows x86 PE transition table: 1,401 states; JSON 7,548,924 B, text table
94,862 B. x86win.py 125 lines (setup and gate rules); no code-shrink claim.
No push/release. The reviewer is paused at the owner's request and did not
review this batch; the test and Windows execution evidence above is mine.

### E3 sizeof expressions and reference type correction (2026-09-27)

General scalar sizeof operands reuse UNARY/expression parsing, then discard
emitted instructions using a stack-saved output mark. Static-initializer
frame checks are suppressed only during that unevaluated walk and restored.
The existing named-array/dimension path is retained; general non-scalar
sizeof expressions remain explicitly not covered. No executor primitive or
new hand type ladder: ELSZ derives the size from the existing descriptor.

Independent cc execution of the new probe found a product defect that tape
comparison alone had hidden: sizeof(sizeof x) was 4 in C, and sizeof returned
a signed type in Python. Both frontends now return the compiler ABI's unsigned
64-bit size_t; C resets the result kind instead of inheriting operand state.
Regression tests/c/b_sizeoftype.c and exec/parse2/probes/s50.c: host cc,
rebuilt .com at O0/O1/O2 and Python agree, exit 0. Nested sizeof is 8, unsigned
comparisons agree, ++ and a function call inside sizeof have no side effect.

Before growing lists, old keep-e3 207 all pass; full exploration: 258 files,
221 equal, 37 not-covered, 0 DIFF/tool-fail/LOST. Six additions are fixed:
s50, b_sizeoftype, b_notint, b_shift, b_condkind, b_longand. Source-to-tape:
old 68 plus these six = 74/74 equal, no rejection or loss. Five new cases
(excluding b_longand) ran through all six deltas to macOS ARM64 images:
whole image equals reference, native output/exit equals host cc. Logs:
/tmp/unisacc-e3-sizeof-fixed.log and /tmp/unisacc-sizeof-chain.log.
Rebuilt .com: 1,345,824 B, SHA256
`dfb14d6bb8f0df5a774feec3fd8a78fb9dc6b255e473026172244c782da58f5c`.
Earlier route/bootstrap hashes remain historical evidence for their recorded
source commits; no claim that those old hashes describe this new product.

Product correction committed as c4ba1c4; the same frozen source tree
passed gate --com: 67 suites, 0 failed, 264 s aggregate, JOBS=2, each suite
<=60 s. exec-chain 74/74, exec-srcelf all 74 complete images equal, all six
self-source routes pass; macOS native/Rosetta self-bootstrap remains in those
checks. No new Linux/Windows native run is claimed for the sizeof batch.
Log /tmp/unisacc-sizeof-gate.log. No push/release.


### S-17：实际阈值网络执行首片（2026-09-27）

- `exec/c/net.py` 按有限行的相邻差分构造整数阈值网络；`run.c` 实际计算隐藏阈值与整数输出，不展开成查表。状态选择网络 bank；下一状态、动作序列号是两个线性整数输出。证明与边界见 `exec/c/NETWORK.md`，不借用旧 18 阶段的验证替代本次证明。
- `NETWORK=1 exec/pipeline/elf.sh ...` 为可选开发路线，默认表路线仍保留。六段的动作和字符串声明不变；Python 仍构造模型，生成器的手写编译规则未消失。
- osx/arm64 六段全域：E2 159,188 / E1 86,625 / E3 1,013,168 / E4 232,976 / lowering 366,362 / image 399,644，共 2,257,963 个观测（含缺项），网络与表完全相同。E3 的同一检查在 UBSan 下通过。
- 六份 `.net` 文本含动作与字符串的字节数依次为 59,840 / 42,873 / 453,241 / 105,323 / 137,869 / 111,969；这是完整声明文件口径，不是仅网络权重或执行内核大小。
- s50 的六段输出逐字节等于表路线；完整 `unisacc.c` 经六段网络生成 osx/arm64 镜像 743,202 B，与参考相同，实跑 N1=N2=N3。生成的仍是现有 C 编译器，不是 E7 发布切换。
- `netcheck.py`：三种观测、非状态栈符号、缺项、接受路径以及改坏 bias 必须失败。`gate --com` 新增 net/netself：69/69，JOBS=2 总 277 s，每套件 ≤60 s；netself 19 s。产品 `.com` 本轮未改变。其他五目标本轮通过的是原表路线，网络路线尚不借此声称已验收。
- 仍未完成：前端全部覆盖、失败诊断等价、几 KB 内核与单份模型 `.com`；T2/T3 仍未证。cc-unisacc 保持暂停，本片无其独立复核。


### S-17：六目标网络路线与默认切换（2026-09-27）

- 在 `004d28e` 上显式 `NETWORK=1`，其余五目标 selfcheck 全通过：lnx/x86_64 671,404 B（17.6 s）；lnx/arm64 716,458 B（17.3 s）；osx/x86_64 693,666 B（21.2 s）；win/arm64 741,888 B（18.4 s）；win/x86_64 694,272 B（17.5 s）。每项重新构造六段网络并全域核对，完整 tape/镜像等于参考；macOS x86_64 经 Rosetta 实跑 N1=N2=N3。Linux/Windows 本轮只验证生成字节，未重启 VM 实跑。
- `elf.sh` 与 `chain.sh` 默认网络推理，`NETWORK=0` 为显式查表对照；回执标明 net/tbl，Terminal 环境传递 NETWORK。门禁的六个 selfcheck 因默认切换而测网络，另保留 osx/arm64 查表 selfcheck。
- 切换后：默认网络 source-to-tape 74/74、source-to-ELF 固定 74 项全等；显式 NETWORK=0 的同一 74 项 tape 全等。产品代码及 `.com` 未改，最近完整 gate --com 仍是上一片 69/69；本片没有把定向复跑写成新一轮全门禁。
- 下一处实际依赖：`unisacc.com -O2 exec/c/run.c` 目前在三个 typedef 形式的 for 初始化处报未知标识符。执行器尚不能称为自托管或产品路径；继续沿真实构建错误处理。


### S-17：执行器由 unisacc 构建（2026-09-27）

- 真实构建发现并修复两处产品问题：for 初始化只看内置 type token，现改用已有 `is_typeat`，Python 同步允许 enum/struct/union 类型；词法器把内部类名 `str`/`num` 当成字面量，现将前五个 TOKV 类名都视为普通标识符。E1 生成器同步修正，未把参考误编译继续抄入网络。回归 `b_fortype.c`、`b_toknames.c`：cc、`.com` 的 O0/O1/O2、Python 全部一致；旧产物对两例以 1 失败。隔离的 `char **str` 复现原先还会把变量读成字符串地址。
- `run.c` 移除 fscanf/ferror/atoll 依赖，用显式整数读取器与分块 IO。动作语义不变；64 位边界以实际 OFILL 输出核对，越界拒绝；输出 fwrite/fclose 失败以 IO 错误 2 退出。native POSIX read/open 保留负 errno，原有 read-dir / ENOTDIR 等五个负例在自建执行器上通过。
- `nativecheck.sh`：先由 unisacc 生成编译器，再用它构建执行器；六段全域核对通过；hello/fib/b_toknames 的 18 份阶段输出等于 cc 构建执行器；两种执行器在真实 1,024 B 文件限制下都报告写失败。入门禁 `exec-native`，11 s。
- 完整负载：自建执行器对先前冻结的 3.9 MB self tape 跑 E4，26.81 s，2,231,188 B 等于 cc 执行器。另在当前源码上 `.com` → native compiler → native executor → 六段网络 → osx/arm64 编译器，完整 selfcheck 在 alarm 60 内通过；743,202 B 等于参考，N1=N2=N3。这里没有把旧负载的计时当作新源码或跨平台性能。
- 规模：cc -O2 runtime __text 16,620 B / 文件 56,520 B（动态 libSystem 不计）；unisacc -O2 __text 85,220 B / 文件 115,746 B（含随带库）。均为加载、IO、校验、执行合计，不是单独内核，也未达几 KB 目标。
- `.com` 已随产品修复重建：1,345,792 B，SHA-256 `236aee387020e2f5d04f8d4a18bff05cb44121eb11f0ae553332d2480c8e1c13`；源 `unisacc.c` SHA-256 `caa8a5ba4b8685403505f299fd8ce2a90d50af999339b136f48164fb70aeea2b`。本地 gate --com 70/70，JOBS=2 总 287 s，每套件 ≤60 s；fat 121/0，两架构均执行；六目标网络 self-source 全部相同。未推送、未发布。
- 仍有实质缺口：b_fortype 在 E3 为 `not covered: identifier is not a local`（产品修复不代表 prototype 已覆盖）；b_toknames 已端到端 tape 相同。native Windows IO 错误分类未闭合；Linux/Windows 本轮未实跑自建 runtime；发布 `.com` 仍为手写 C 编译器路径。cc-unisacc 继续暂停。


### S-17：for 声明与作用域（2026-09-27）

- cc-unisacc 保持暂停，由 cdx-unisacc 继续。E3 的 for 初始化接入已有 TSPEC/S.decl，识别内置类型、typedef、struct/union 与已声明 enum tag；标签在初始化之后分配，避免条件表达式先分配标签时错位。循环用现有 BIND/UNWIND 保存和恢复局部作用域，没有新增执行器原语。enum tag 使用独立命名空间，不混用 typedef 或值名。
- 独立执行暴露产品缺陷：外层 i=7，`for (int i=0;i<2;i++) {}` 后返回 i，旧 `.com` 返回 2。C 前端补循环作用域的符号、帧偏移、类型及 VLA 栈恢复；Python 已有循环作用域，不改其算法。回归 b_forscope 覆盖同名遮蔽、嵌套、continue/break、条件初始化及表达式初始化；cc、C O0/O1/O2、Python 输出 `7 2 9`，rc 0。
- 固定清单扩大之前，原 E3 213 项全部 equal；b_fortype 与 b_forscope 再加入，现 215 项。两例通过六段实际网络生成 macOS ARM64 镜像，完整字节等于参考，实际运行等于 cc。chain/ELF 清单 74→76；门禁 source-to-tape 76/76。未声称前端全覆盖：exec/c/run.c 仍停在 `sizeof non-scalar expression`。
- 规模：gen2.py +18/-10（净 +8 行），状态 3928→3942，JSON 21,506,653→21,584,161 B。属于行为补齐，不是代码缩减；既有类型/声明机制被复用，未把语言规则放进执行器。
- 已重建 `.com`：1,347,184 B，SHA256 `0475e2c62a3835c6a5104479dda948a4c11456be5cb87af8d658fcc9e0af9fa9`；unisacc.c SHA256 `ce491daefe821f8eacda919fc94acfbd697b05e0873d246ced89115efd894157`。
- 冻结树 gate --com 70/70，JOBS=2，总 285 s，每套件 ≤60 s；fat 122/0（两架构实跑），六目标网络 self-source 与参考相同，macOS 自举沿用门禁实跑。Linux/Windows 本轮仅字节核对，未启动 VM。日志 /tmp/unisacc-for-gate.log；旧 E3 清单证据 /tmp/unisacc-for-keep.log。未推送、未发布；完整重构与 E7 产品切换仍未完成。


### S-17：执行器源码的尺寸与成员声明（2026-09-27）

- E3 sizeof 识别 typedef 与 enum 类型；`sizeof *name`/多重解引用复用命名对象的 DIM/ELSZ 计算，保留数组剩余维度，不把数组退化为 8 字节指针。复杂操作数仍走既有表达式解析，未覆盖者继续明确拒绝。结构体逗号声明复用 DSTARS 与同一成员布局，支持每个声明符独立的指针深度与数组长度。无新执行器原语。
- 独立 cc 对照发现产品错误：sizeof 解引用结构体指针原为 8、实际应为 16；二维数组解引用原为 4、应为一行的 12。C 前端解引用现在记录聚合尺寸与剩余维度，指针结果明确为 8。b_sizeofderef 在 cc、C O0/O1/O2、Python 的输出相同：`8 1 8 16 8 16 4 / 24 12 4 / 96 48 16 4`。Python 原本正确，未改。s51 核对同一声明的 char 指针/char/指针、int、数组成员，布局与 cc 相同。
- 新直接分支一度抢先 LOOKUP，令旧有 `sizeof *f()` 从 equal 退为 not-covered。补测发现后，改为先分派复杂操作数，再查直接对象；s52 固定此回归，cc、.com、Python 与六段网络均为 `4 4 0`、rc 0（函数未被调用）。没有把旧覆盖损失当作可以接受的扩展代价。
- 扩清单前旧 215 E3 项全部通过；最终 218/218 equal。chain 从 76 增至 79，最终网络路径 79/79，无拒绝/丢项。三例均通过六网络生成 macOS ARM64 镜像，原始字节等于参考，实跑等于 cc。E3 网络全域 1,023,230 观测相等，3967 banks / 7445 hidden units / 含动作字符串的文本 455,303 B。
- gen2.py +21/-8（净 +13），状态 3942→3967，JSON 21,584,161→21,725,618 B：覆盖进展而非代码缩减。执行器源码已越过 sizeof 和逗号成员声明，当前真实阻挡是 `o->n++`，未称整个 runtime 源码已经由网络编译。
- 产品已重建：.com 1,347,504 B，SHA256 `cd19cca573d7a9fc44eccb4ffb4d88d72617c2f096e452cec2cec9aa64aa04b7`；unisacc.c SHA256 `c20e182aa65723f4315673c461d2929d8ada0d2081e90e7bcd0825a137c9f912`。冻结产品/前端初版 gate --com 70/70、287 s、JOBS=2，各套件 ≤60 s；fat 123/0，六目标网络镜像等于参考（Linux/Windows 本轮不宣称 VM 实跑）。日志 /tmp/unisacc-szd-gate.log。
- gate 之后只有上述 E3 分派回退及 s52/固定清单改变，产品未变；最终再跑 E3 218、chain 79、s52 六段完整镜像，以及 osx/arm64 网络 selfcheck（743,202 B、N1=N2=N3），均通过。没有把前一轮 70 项门禁冒称为最后两行解析调整后的全量复跑。日志 /tmp/unisacc-szd-final-{keep,chain,self}.log。未推送、未发布；cc-unisacc 保持暂停。


### S-17：网络路线重建通用执行器（2026-09-27）

- `exec/c/run.c` 现在由六段实际阈值网络生成 osx/arm64 可执行文件，字节等于原编译器构建结果；该执行器再加载同一组网络，重编自身源码，六个中间结果与最终镜像逐字节相同。`nativecheck.sh` 固定这条检查，并保留六段全域核对、原有 18 份输出比较、三种执行器的真实写失败。Python 仍构造模型，未参与六段的源码处理；`.com` 尚未切换路线，前端完整覆盖、几 KB 内核和单份模型打包仍未完成。
- 实际源码驱动的 E3 修改：命名/成员/下标左值共享后缀增减与赋值，结构体复制统一到同一例程；指针成员初始化复用既有 8 字节存储；地址/成员遍历复用 MEMB；case 复用 CE；全局初始化根据首次声明记录的名字处理，不再被数组界限中的 enum 名覆盖。没有增加执行器原语。s53/s54/s55/s57 分别固定左值、聚合复制、指针初始化、含 enum 界限的全局声明与 64 位常量。
- 独立 cc 比较发现产品宽 case 标签被 int 截断：旧产物输出 `1 2 3 5 4`，应为 `1 2 3 4 5`。catom/cexpr 与 case 存储保持 long；case 按控制表达式的提升类型转换，32 位有符号/无符号及嵌套状态分别处理。C/Python/E3 同步；b_casewide 在 cc、最终 .com O0/O1/O2、Python 输出 `1 2 3 4 5 / 6 7 8 9 10`。这是已测整数路径，不声称完整 C 常量表达式语义已证明。
- 扩清单前旧 218 E3 全通过；最终 224/224 equal（含真实 runtime）。chain/ELF 79→85，全部一致。新探针六网络 macOS 镜像实际运行与 cc 相同。gen2.py +59/-49（净 +10 行），状态 3967→3991，JSON 21,725,618→21,869,510 B；复用减少重复路径，但本批源码没有净缩减。
- 最终冻结代码 gate --com：70/70、294 s aggregate、JOBS=2，每套件 ≤60 s；exec-native 19 s，fat 124/0 两架构实跑，六目标完整源码镜像相同。Linux/Windows 本轮仅核对生成字节，未启动 VM；没有新的跨平台运行主张。日志 /tmp/unisacc-runtime-model-gate.log、/tmp/unisacc-runtime-model-keep.log。
- `.com` 1,348,688 B，SHA256 `c018f8275910808d8e7e750b437faa298ab815a93ef1699080fbb923b6ef7967`；unisacc.c SHA256 `aab9b9378dd5d81a0df3066cf514e81742d737be0fe609886a5880812f464edc`。未推送、未发布；cc-unisacc 保持暂停，重构目标继续。


### S-17：产品语料中的三处接受差异（2026-09-27）

- 在 `0d3358b` 的 70 项门禁后，额外检查 examples、examples/apps、tests/c 共 130 个输入：95 equal、32 not-covered、3 DIFF。固定清单通过不能替代这一探索结果。差异为 queens 的 unsigned 按位取反缺少截断，以及 b_fuzzfound/b_structarg 的按值结构体参数没有局部副本；未把接受差异改名为 not-covered。
- unsigned int 的 `~` 复用 NARU。参数全部 spill 后，才为结构体逐个分配局部副本；调用端同样建立临时副本，避免后一次结构体返回覆盖前一实参。赋值、参数、实参临时值、返回共用 COPYSTRUCT，删除原来返回只支持 8 字节倍数的重复循环。没有执行器原语或产品源码改动。
- s58 固定 3 字节结构体、多结构体实参、连续结构体返回和超过六参数的栈约定：cc、现有 .com 与网络产物输出 `924 44 3 4 5`，调用后原对象仍是 `3 4 5`。另外三例的网络完整镜像等于参考，实际输出等于 cc。旧 224 项先通过再扩至 228；source-to-ELF 固定清单 85→89，89 项全部通过。nativecheck 再次通过网络执行器自身重建及真实写失败。
- 同一 130 输入重新检查：99 equal、31 not-covered、0 DIFF（通用复制顺带使 b_struct3 一致）。保留未覆盖清单，不称全部 C99 产品测试已迁移。日志 /tmp/unisacc-structargs-{keep,frontier,srcelf,native,network}.log。
- gen2.py +26/-13（净 +13 行），4007 状态，JSON 21,971,103 B。仅定向回归，没有把前一提交的 70/70 称为此提交的完整门禁；产品 .com、源码哈希沿用上一记录。下一重点仍是这些真实语料的拒绝缺口与最终产品接入，不以 equal 数量代替收尾。


### S-17：共享聚合初始化与静态数组声明（2026-09-27）

- 删除 E3 全局数字列表的独立 walker；全局、自动、静态聚合初始化共用 INITLIST，只由 INITADDR 区分地址来源。全局/静态初始化在声明处生成并缓存，随后回放到 __init，保留字符串编号和声明顺序。静态路径复用同一 walker，未增加执行器原语。无括号 `sizeof lines[0]` 复用已有维度求积，修正先消费名字后留下下标的缺口。
- 新探针 s59 同时覆盖三种存储期的二维短行、零填充、指针和结构体成员；calc 与 b_ptrdepth 也由共享路径通过。旧 228 E3 项先保持，再加入三例，231/231 equal；chain/ELF 固定清单 89→92。三个完整网络 macOS ARM64 镜像逐字节等于参考，实跑等于 cc；b_ptrdepth 给宿主 cc 显式包含 stdio.h（其源码依赖产品自动包含），没有把宿主缺声明的首次编译失败写成编译器差异。
- 探针发现产品静态局部数组只解析一维，合法的二维声明被拒。C 前端抽出 dimtail，自动、静态、全局声明共用第二/第三维处理，并给静态聚合初始化传入行尺寸。Python 已支持，不改其实现。产品 b_staticarray 覆盖二维/三维、逗号声明、sizeof、跨调用持久性，cc、Python、最终 .com O0/O1/O2 均输出 `24 48 6 2 0 9 1 / 24 48 6 4 0 9 3`。不据此称网络 frontend 已支持三维嵌套初始化。
- gen2.py 净 -33 行，statics.py 净 +4 行，合计 -29；状态 4007→3969，JSON 21,971,103→21,768,127 B。减少的是重复初始化逻辑；未把规则搬到执行器或另一个生成脚本。任意嵌套聚合、designator、完整 C99 初始化语义仍未覆盖。
- 冻结代码 gate --com 70/70，JOBS=2，合计 298 s，每套件 ≤60 s；fat 125/0 两架构实跑，chain 92/92，source-to-ELF 固定清单通过，六目标网络自源码镜像相同，nativecheck 18 s。Linux/Windows 本轮未启动 VM，仅验证生成字节。日志 `/tmp/unisacc-init-gate.log`、`/tmp/unisacc-init-final-keep.log`、`/tmp/unisacc-init-network.log`。
- 重建 .com：1,348,432 B，SHA256 `d1d29056f7d8fb32e2173d8c6fd71ac8872580ab575d0834fc8251a903b22e83`；unisacc.c SHA256 `1442c8ee298439f6adf3ea934ea012dd1292f21a26790d5abe9c30dafd7ed3b9`。未推送、未发布。产品入口仍未切换为网络运行时，重构未完成；cc-unisacc 保持暂停。


### S-17：递归聚合上下文与成员数组（2026-09-27）

- 全局、自动、静态初始化继续共用一个 INITLIST；用嵌套上下文记录当前聚合的标量槽范围，右括号跳过省略成员，designator 在当前层解析。成员偏移/槽数来自已解析布局，宽度复用 ELSZ/STOREV；默认 union 只遍历第一成员。原二维专用循环被替换，新增 initializers.py 仅组织生成器过程，没有执行器原语。省略维度按聚合槽计数；全局标量表达式也共用 EXPR/STOREV，删除独立地址/字符串指针初始化路径。PWIDTH 复用 ELSZ，修复 double 指针步长。
- s60 固定三种存储期的嵌套结构体数组、三维数组、designator、零填充、跨调用持久性、默认 union 与 double 元素。b_init6/b_init7 由同一机制通过。期间发现并修复逗号声明类型状态未保存、&function 被新公共表达式路径拒绝的回归；旧 231 项先全部通过，再扩至 234/234。chain/ELF 92→95，全通过。实际产品语料 132 项：104 equal、28 not-covered、0 DIFF，不称前端完整覆盖。
- 独立 cc 对照发现 C 产品把结构体数组成员当成结构体值，取下标步长错误。新增显式 mbarr（包括长度 1），随匿名/复制成员传播；成员数组衰变时设置指针深度与元素宽度。Python 原本正确。b_structarrmember 的 cc、最终 .com O0/O1/O2、Python 结果为 `3 4 20 9 10 30 16 16`；该产品回归在 E3 仍因 sizeof 复杂成员表达式未覆盖，不虚列入 equal。s60 覆盖不含该 sizeof 的成员数组路径。
- 三个新增 E3 输入的六网络 osx/arm64 完整镜像等于参考，实际输出等于 cc；b_init6/b_init7 为宿主显式包含 stdio.h。gen2.py 净 -60 行，initializers.py +81，合计 +21；状态 3969→4033，JSON 21,768,127→22,187,240 B。这是统一规则路径与补齐已测形状，不是源码净缩减；任意初始化语义、非首 union designator 等不在完成主张内。
- 最终冻结 gate --com 70/70，JOBS=2，297 s aggregate，每套件 ≤60 s；fat 126/0，两架构实际执行；六目标网络 self-source 等于参考，nativecheck 18 s。Linux/Windows 本轮未启动 VM，仅验证生成字节。日志 /tmp/unisacc-aggr-{finalkeep,frontier,network,gate}.log。
- .com 重建为 1,348,784 B，SHA256 `66d466a7d2a48796b62a30a71167d7befd4a08b6e301317a73806b11ce28041a`；unisacc.c SHA256 `577e9261925d3c3c2becbd3247b2c12ab6f85e650e7dd5180ed14507dc835d6e`。未推送、未发布。cc-unisacc 保持暂停；完整前端、失败契约、紧凑内核与单份模型 .com 切换仍需继续。


### S-17：单进程网络字节流（2026-09-27）

- `run.c` 抽出 execute，新增开发入口 `--chain INPUT SRCPATH INCLUDE_DIR MODEL...`。列表显式、有序、长度不限于六段；驱动不认识 C 阶段或目标架构。每次只把上一段接受的字节交给下一段，与原文件边界相同，不传递输出属性。每段释放模型与临时状态，只有接受的输出存活；stdout 在全部接受后才写出。旧单模型与全域核对接口保留，没有新增模型动作。
- 普通运行的寄存器、索引内存、栈、reader frame、blob、intern、文件缓存逐段重置；SWAP 释放已替换的输入。netcheck 对同一带状态模型连续运行 20 次，检查空输出、拒绝/步数超限后不打开后续不存在的模型、无半成品 stdout。cc 与 unisacc 构建均通过；nativecheck 的三种构建（cc、unisacc、六网络生成）对 hello/b_toknames/runtime 源码跑内存链，均与原六进程镜像相同，含网络执行器重建自身。ASan+UBSan 在 hello/b_printf3/runtime 三个真实六段输入上通过，无报告；未把它称为全平台内存证明。
- 同批删除 E3 对已定义 printf 的专用格式预扫描，直接 CALL；只有未定义 fallback 才要求字面量。b_printf3 动态格式通过且 macOS 网络产物实跑等于 cc。旧 234 项先保持，再扩至 235；chain/ELF 95→96。gen2.py 净 -11 行，4029 状态，JSON 22,169,415 B。不是新增 printf 规则，也未把规则放进执行器。
- macOS ARM64 六网络文本合计 917,921 B；cc -O2 runtime __text 16,688 B / 文件 56,496 B（动态 libSystem 不计），网络/unisacc 构建 runtime __text 87,808 B / 文件 115,746 B（含库）。这是加载/IO/校验/执行合计，不是独立几 KB 内核。run.c 净 +43 行，用于重入、清理与字节流驱动。模型仍是显式六文件，尚未合成 E7 单份 payload，也没有对外切换 .com CLI。
- 冻结 gate --com 70/70，JOBS=2，312 s aggregate；各套件 ≤60 s，最大 40 s，新增 nativecheck 39 s。fat 126/0，六目标 self-source 字节对齐。Linux/Windows 未启动 VM，单进程本轮实际运行仅 macOS ARM64。日志 /tmp/unisacc-stream-{native,network,gate}.log 与 /tmp/unisacc-call-keep.log。产品源码/.com 未改变，沿用 dec6553 的产物哈希；未推送、未发布。下一步仍须完成产品驱动/单份模型接入、紧凑内核与前端/错误路径缺口，目标未完成。


### S-17：共享模型包与声明路线（2026-09-27）

- 新增构造期 pack.py：读显式 TSV 清单（路线、阶段、输入格式、输出格式、网络路径），按完整网络字节去重。runtime `--bundle PACKAGE ROUTE INPUT [SRCPATH] [INCLUDE_DIR]` 只读一个包，先核对目录、邻接格式、重复阶段、索引与范围，再复用既有 loadbytes/execute；不含 C 阶段或架构决策，不启动 Python。包格式 P1 及边界写入 exec/c/PACKAGE.md。发布 CLI 未改变。
- image-stages.tsv 声明六段顺序和格式；elf.sh 用其顺序，构造与全域核对后写 route.tsv/models.pkg。原逐段文件路线仍作为比较对象。旧 pipeline/stages.tsv/run.py 仍是早期三段参考工具，没有冒称所有历史工具已共用唯一清单；独立格式检查与产品参数接入仍需收敛。
- 六目标 36 个条目合为 21 份唯一网络：原分散网络合计 5,595,439 B，单包 2,555,823 B，gzip-9 参考值 359,340 B（运行器不解压）；SHA256 `5b69ca54701e191f7b7f59aee9537532f514f8d889a3337539d49823618ae379`。合包的 hello 六目标镜像均等于各自散文件路线。另在门禁中，各目标完整 unisacc.c 从各自包生成的镜像均与逐段及产品参考相同；没有把 hello 合包测试写成六目标合包的完整自源码实跑。
- cc/unisacc 的 netcheck 通过共享体、路线选择、格式失配、重复阶段、越界索引、截断包；nativecheck 在 cc、unisacc、网络构建执行器上从包重建 runtime，输出相同。ASan+UBSan 对包内 hello 和 runtime 全六段通过、无报告；E3 对新增包加载器源码仍 equal。单目标 macOS ARM64 包 918,211 B。
- 规模：cc -O2 runtime __text 18,636 B，文件 57,648 B，gzip-9 15,780 B；网络/unisacc runtime __text 92,452 B，文件 115,746 B，gzip-9 21,090 B。动态 libSystem 不计入 cc 文件，unisacc 包含随带库；这些都是整个工具而非独立内核。运行器净 +66 行用于目录与加载，未增加模型动作。几 KB 内核和 .com 内嵌单份模型仍未达成，包也未包含产品内嵌头文件资源。
- 冻结 gate --com 70/70，JOBS=2，360 s aggregate；各套件 ≤60 s，nativecheck 53 s（余量仅 7 s），六目标包自源码各 29–33 s。fat 126/0、chain 96/96。macOS 实际执行，Linux/Windows 本轮未启动 VM，仅生成字节对齐。日志 /tmp/unisacc-package-{native,network,gate}.log；六目标合包产物 /tmp/unisacc-package-six/all.pkg。产品源码/.com 未改，沿用 dec6553 的哈希。未推送、未发布；cc-unisacc 继续暂停。


### S-17：包内命名字节资源（2026-09-27）

- 包格式增加兼容 v1 的 v2 资源目录：非空字节名、任意内容（可为空），长度在索引前检查。构造期 `--mount PREFIX_HEX DIRECTORY` 显式读取目录；相同键/相同内容合并，冲突拒绝。runtime 通过现有 SBFIND 先读精确字节键资源，再走原文件系统适配；缓存仍逐阶段清理，资源本身保持不可变。不新增模型动作、不包含语言相关解码规则。
- elf.sh 将 19 份 include/*.h 以 E2 既有的 NUL-hdr/ 名称挂入包。它们是 103,636 B 的源文本资源，不是网络权重；单目标 osx/arm64 包 1,022,275 B，gzip-9 参考 157,046 B（运行时不解压），SHA256 `a91b49311e348a91b3e4fd91b1a4280e4c98e0077e13ee7162fd776d636d44c2`。这不是已嵌入 .com 的发布产物。
- nativecheck：隔离目录只有 runtime.c 和 models.pkg，没有 include/，也不给 include-directory 参数；cc、unisacc、网络构建的三种 runtime 都生成相同自身镜像。去掉资源的同源包在该目录以 2 拒绝、无 stdout，避免隐式读回仓库的假通过。ASan+UBSan 对同一隔离资源路线生成自身通过，无报告。netcheck 覆盖含 NUL/高字节的内容、空资源、重复读取缓存、两段间重置、冲突挂载、缺目录及截断/越界资源。
- 运行器净 +17 行 C；cc -O2 整体 __text 19,140 B，文件 57,904 B，gzip-9 16,240 B；unisacc runtime __text 94,448 B，文件 115,746 B，gzip-9 21,516 B。包含加载/IO/校验/执行及各自库口径，不以这些数字冒称独立几 KB 内核。产品程序源码与用户自备头文件仍是文件输入。
- 冻结 gate --com 70/70，JOBS=2，364 s aggregate；各套件 ≤60 s，nativecheck 54 s、余量 6 s，六目标包自源码字节一致；fat 126/0，两架构实际执行。Linux/Windows 本轮未启动 VM。日志 /tmp/unisacc-resource-{native,gate}.log。E3 对修改后的 runtime 源码单独 equal；固定清单仍 235/96。产品源码/.com 未改，仍沿用 dec6553 的哈希；未推送、未发布。单份 payload 嵌入、产品驱动/CLI、紧凑内核与剩余前端/错误契约继续推进，cc-unisacc 保持暂停。

### S-17：单份模型包嵌入开发 .com（2026-09-27）

- APE 打包增加可选 `--payload`，包在所有切片之后只存一次；16 B 尾部由 `UNIPKG1\n` 与 LE64 长度组成。runtime 先验证范围，再复用已有包/模型加载器。Unix 启动脚本把原容器路径放入 UNISA_CONTAINER，避免缓存的架构切片丢失包来源；`--embedded ROUTE INPUT` 为开发入口。无环境变量时尝试 argv[0]，Windows PE 路径尚未实跑，不称已通过。
- 六目标合包含 21 个共享模型体、36 个阶段目录项和 19 个头文件资源，2,659,887 B，SHA256 `f3edc4e9ff9faf172f4f39105e01678623f7aaf5c23a2adb37b663704ce0e23a`。独立开发容器 `/tmp/model-runtime.com` 为 2,845,967 B，SHA256 `d342972ce88e278d77ba1f4db4a88fc4d67db16f920a644efc0ed4479fb345cb`。使用当前 runtime 源码与此前已验收六路线模型构建；不覆盖产品 unisacc.com。
- 隔离目录中只有开发容器和 hello.c：六目标输出均与外置同包路线逐字节相同，osx/arm64 镜像实际打印 hello from C99；其余为字节检查，本轮未开 Linux/Windows VM。通用 embeddedcheck 检查两容器共用同一执行器切片但携带不同模型、路径含空格、删除外置包后仍输出各自结果。netcheck 在 cc/unisacc 构建上拒绝尾部截断、错误 magic、零长度与超界长度，无 stdout。E3 对 runtime 源码保持 equal。
- 默认无 payload 的打包结果 SHA256 仍为 `66d466a7d2a48796b62a30a71167d7befd4a08b6e301317a73806b11ce28041a`，与现有产品逐字节相同。冻结 gate --com 71/71，JOBS=2，362 s aggregate；nativecheck 55 s、新增 embeddedcheck 3 s，单套件均 ≤60 s。日志 `/tmp/unisacc-embedded-gate.log`。
- Python 用于离线模型构造与 APE 包装，开发容器执行阶段不调用它。运行器仍包含加载/IO/校验等，独立几 KB 内核尚未完成；产品 CLI/默认路线尚未切换，前端与失败契约仍有缺口。单份模型嵌入已落地不等于完整重构完成。未推送、未发布；cc-unisacc 保持暂停。

### S-17：普通编译命令接入网络包（2026-09-27）

- 新 `exec/c/compiler.c` 复用 run.c 的字节流执行器，独立入口只做参数/文件/路线选择；没有源码解析、tape 生成、优化、lowering 或编码代码，也不调用参考编译器。run.c 的工具入口与 --check-net 在库构建中排除；runroute 共用于开发执行器与新驱动，readstream 支持 stdin。产品 unisacc.com 未切换。
- `compiler-routes.tsv` 声明 pp、tape/O0/O1/O2、image/O0/O1/O2；compilerpack.py 将已有 image 清单展开为路线目录，O0 跳过优化器，O1 使用显式构造的 O1 网络，O2 复用原网络。六目标包 42 条路线、174 阶段项、22 个去重模型体、19 资源；编译的动作仍由网络执行，没有复制各目标的模型体来实现 CLI。
- 已接 `-E`、`-S/-c`、`-b/-t`、`-O/-O0/-O1/-O2`、`-o`、一个 `-I`、单文件及 stdin。默认目标/输出模式与现产品对应。成功接受后才打开输出文件；短写循环推进、写入停止与关闭失败均拒绝。`-run`、多单元、宏/强制 include、依赖输出、告警/仪器等尚未迁移，显式拒绝，不默默退回旧实现。完整 CLI/失败诊断一致性仍未达成。
- 新 compilercheck 每次构造模型；O1 的 49,538 个观测全域等价检查通过。cc/unisacc 驱动 18 组模式×优化级输出与当前产品逐字节相同，另核对 stdin、无效输入不截断既有文件、未迁移选项、未知路线、打不开输出与真实 RLIMIT 短写。六网络构建新驱动自身，镜像与 unisacc 构建相同；该网络构建驱动再编译 hello 输出相同。隔离目录仅有嵌入式开发容器及 hello.c，普通 hello.c -O2 生成 a.out 并实际打印 hello from C99。E3 对 compiler.c 与 run.c 均 equal。
- 产物：六目标包 2,675,518 B（gzip-9 388,434 B），SHA256 `4c04fbddbcdbcfc4e051d89f676ad5bec92f41588e433e4daa3057046c27a98d`；独立 `/tmp/compiler-model.com` 2,861,502 B（整个文件 gzip-9 499,295 B），SHA256 `6e46ec4168d6da8169be3b5b446015f37b85326ac85b0954e23d2c475869f3f5`。运行时包不解压，这里 gzip 仅为账目参考。cc 驱动整体 __text 20,176 B/文件 57,808 B，unisacc 驱动整体 __text 95,368 B/文件 115,746 B（含随带库），仍不是独立几 KB 内核。新驱动 105 行，包展开 43 行；功能尚不等于旧 main，不把行数差说成整个产品缩减。
- 冻结 gate --com 72/72，JOBS=2，370 s aggregate，最长 54 s；新 exec-driver 23 s。日志 `/tmp/unisacc-driver-gate.log`。当前产品源码与 .com 均未改；本轮实际执行 macOS，Linux/Windows 未开 VM。后续继续其余 CLI、前端/失败契约、紧凑内核与最终默认切换。未推送、未发布；cc-unisacc 继续暂停。

### S-17 continuation: command-line macro inputs interpreted by E2 (2026-09-27)

- `compiler.c` now passes raw `-D`, `-U`, `-include`, and one `-I` value as four NUL-named, immutable process resources. The generic runtime only serves exact byte keys through existing SBFIND; no new action or macro parser is in C. E2 performs name/body parsing, forced-source prefixing, define/predefine/undefine ordering, and source-relative / -I / include/ / carried-header search. Runtime action semantics and network inference are unchanged. Macro expansion of an empty body now emits the reference's single separating space.
- The integration exposed two product defects, fixed in `1e1a6b3`: string literals had pointer size under sizeof (the embedded-NUL resource name was truncated to seven bytes); `'U'` and `'u'` incorrectly acquired unsigned integer type from their contents. C/Python now preserve literal array extent and decay it for pointer arithmetic, conditional and comma results. Python also accepts the parenthesised comma operand used by the regression. New b_strsizeof and b_charkind agree with host cc in C O0/O1/O2, Python, and the rebuilt product .com O0/O1/O2. The character rule is unchanged in Python, which was already correct.
- E3 recognises complete literal operands of sizeof, including redundant parentheses and decoded escapes, without changing string-pool numbering; other operands return to the existing expression path. Old keep-e3 235/235 passed before adding s61 and b_charkind: 237/237 equal, 107,047,479 Python action steps, no refusal/difference/tool failure. Driver source itself is equal. This does not claim all pointer/wide sizeof forms are now covered by E3.
- Fresh compilercheck: cc/unisacc native drivers and Python E2 action oracle agree on 13 macro/include cases (empty/default/expression/negative body, replacement, -U before/after -D, target predefines, forced-header ordering, quoted local header priority, -I override of carried stdio). A -D-controlled complete image also equals the product. Existing 18 mode/level comparisons, stdin, failures before opening output, real write failure, network-built driver, and isolated embedded container still pass. Exact failure diagnostic rendering and the remaining CLI are not claimed complete.
- Frozen gate --com: **72/72**, JOBS=2, 370 s aggregate; longest nativecheck 54 s, driver 23 s, each suite <=60 s. Log `/tmp/unisacc-cli-gate.log`; E3 fixed-list log `/tmp/cli-e3-keep.log`. Real host execution macOS arm64/Rosetta; Linux/Windows VMs not started this round. Cross-target byte equality is not a VM run.
- Product .com rebuilt: 1,349,584 B, SHA256 `e46a8baaf0c7c4e49e2b2cf9eba0b6509e4150acd56a2afff777665783e03dd0`; generated unisacc.c SHA256 `a0bdb5014bb4f481fb11e76ee9b02295da6299d0fd4d8e0af2bb979f87322afd`. The product includes the literal fixes, but its default compiler route has not switched to the new network driver. -run, multiple units, remaining options/diagnostics, frontend gaps, and the compact executor remain. Local commits only, no push or release; cc-unisacc stays paused.

### S-17 continuation: model-generated native-memory execution (2026-09-27)

- The development driver now connects POSIX `-run`. Lowering and encoding networks receive raw argc/argv and mapped-base resources; they compute argument cells, addresses, relocations, code/data and entry. The C adapter only allocates adjacent memory, validates the UNIMEM1 byte image, copies it, makes text RX and enters it. No temporary executable, reference compilation, new executor action or runtime Python is used. A size-plan pass and a bound pass reuse the same encoder network; ordinary ELF/Mach-O/PE routes remain available.
- `memorycheck.sh`: for hello and function-pointer input, code/data/entry match the retained bk_run backend at its actual mapped addresses, independently through the Python action oracle and C network runtime. cc/unisacc drivers match 24 native runs (six programs, O0/O2), plus explicit argv/environment/O1/exit-7 checks. The unisacc main argv copy omits the environment; the adapter therefore reads the existing process-vector intrinsic. Both builds reject malformed memory lengths/entry/extent. Both executors reject half-specified mapped bases and argument headers without process context.
- Fresh driver integration remains green: 18 mode/level comparisons, CLI resource cases, network-built driver, write failures and isolated embedded container. The embedded development container also runs hello in memory without creating an executable file. No product source or shipped .com changed in this slice.
- Frozen `gate --com`: **73/73**, JOBS=2, 387 s aggregate, each suite <=60 s; new exec-memory 31 s, exec-driver 25 s. Log `/tmp/unisacc-memory-gate.log`. New memory execution was measured on macOS arm64; existing gate also exercises Rosetta image paths, which is not evidence of x86 memory execution. Linux/Windows VMs were not started.
- Windows memory imports remain explicitly unsupported. Multiple units, remaining CLI and failure compatibility, frontend coverage, isolated compact executor and final product switch remain unfinished. This is a development-driver slice, not completion of S-17. Local commit only; cc-unisacc remains paused, no push/release.

### S-17 continuation: Windows memory imports and Rosetta execution (2026-09-27)

- The development driver now runs model-generated Windows code in memory. A generic OS adapter enumerates its loaded PE's named imports into immutable process resources; the encoder network chooses the declared imports, lays out IAT slots, binds code/data addresses and emits UNIMEM1. Allocation/protection remain OS operations. No compiler import-name list, instruction encoding or new executor action was added to C. Ordinary image routes remain unchanged.
- Windows optional header lookup follows the existing product's try-next behaviour: its native open wrapper does not provide POSIX errno discrimination. Explicit missing input still fails; this is not a claim of POSIX error fidelity. The adapter uses unisacc's 64-bit long, not Windows host-cc LLP64.
- Fresh simulated binding checks on both Windows architectures compare lower/encoder network output with the action oracle and independent assembly at declared addresses, including import slots, argv cells, entry and extent. Generic PE import enumeration gives 14 exact names/values in cc and unisacc builds and rejects an out-of-image name. These simulated checks are separately labelled.
- Actual Windows 11 arm64 VM: a network-built driver, byte-identical to the reference-built driver, passed eight memory runs per architecture (hello/function pointers at O0/O2, argv/O1/exit 7, file IO, malloc, explicit missing input). arm64 was native; x86_64 used Windows emulation, not an x86 physical host. Driver SHA256: arm64 `1201c5610c24a4dd036270835f6a805648e93993539ff09daa3569bbef95d678`; x86_64 `7b8fbf28a0a36968055d83fbe6674cab8e4f025b1103eb44e2c48e7e6bacfa5b`. Evidence under `/tmp/unisacc-memory-win-arm/` and `/tmp/unisacc-memory-win-x86/`; VM stopped afterwards. Windows embedded APE runtime was not tested by these standalone PE runs.
- Rosetta x86_64 now runs the same native-memory checks as macOS arm64, including binding to the retained backend, 24 executions, argv/environment and loader bounds. Frozen gate --com **76/76**, JOBS=2, 415 s aggregate; each suite <=60 s. Memory arm/x86 suites 31/34 s, simulated Windows arm/x86 4/3 s. Log `/tmp/unisacc-winmemory-full-gate.log`. No product source or shipped .com changed.
- Linux native execution of this new memory route remains unmeasured. Multiple units, remaining CLI/diagnostic compatibility, frontend coverage, compact isolated executor and final default switch remain unfinished. Local commit only; no push/release; cc-unisacc remains paused.

### S-17 continuation: Linux ARM64 memory execution (2026-09-27)

- Fresh six-network Linux ARM64 route built `exec/c/compiler.c`; its ELF is byte-identical to the reference-built driver, SHA256 `b7618ad8fb3265186062b13c90a489e129920cff3e38f89ecde61675245d24bc`. Package 1,045,263 B, built from the current route models and the unchanged O1 network. No runtime Python or retained compiler path is used by the driver.
- Actual native ARM64 Lima run: nine programs (hello, fib, struct, argv, printf, static, function pointers, file IO, malloc) at O0/O1/O2 give **27/27** output/stderr/status matches against guest system cc, plus explicit argv/environment/exit-7 and missing-input rejection. The test Python script only runs the independent compiler and compares results. Per-process bound 15 s, outer run 60 s, exit 0. Evidence `/tmp/unisacc-memory-linux-arm/native-result.log`; reusable opt-in harness `exec/c/posixmemoryrun.py`.
- Dedicated minicon-lnx-aarch64 initially lacked cc: that attempt failed before comparisons and is not counted. It was stopped. Tests instead ran in the already-running default ARM64 Lima VM with system gcc; that VM was left running. Windows VM remains stopped. Linux x86_64 memory execution is still unmeasured.
- This adds only a test harness and evidence/documentation after the frozen 76/76 gate, not compiler changes; the full gate was not redundantly rerun. Product .com remains unchanged. Local commit only; no push/release; remaining S-17 work continues.

### S-17 continuation: multiple translation units through model inference (2026-09-27)

- The development driver now accepts multiple C files for tape, image and native-memory output. It runs each file through fresh E2/E1 state, preserving that file's path/macros/header guards, then passes LE32-length-framed typed streams to a shared network. C only frames bytes and dispatches routes; declaration scanning, file-static renaming, unit markers and shared-program state are handled by models. Runtime Python and reference fallback are absent. Multiple -E outputs and exact diagnostic compatibility remain unfinished.
- New units.py is an 89-line handwritten constructor compiled into a threshold network, not a gold-derived rule or code reduction: 417 states, 1,400 hidden units, 31,558 B; all 107,330 observations equal its table, with identical actions/strings. It validates up to 64 units and uses the reference's later-unit __uN names. Inline aggregate static specifiers remain explicitly unsupported. The package reuses one network body across routes.
- Integration exposed a product defect fixed in acb7e84: per-file token ordinals gave two functions' block statics the same ls8 storage. Independent system cc returned 19; the old product returned 100. Labels are now unit*MAXTOK+token, with original token identity retained for symbol lookup and unit-zero labels unchanged. E3 uses the same source-declared MAXTOK at construction. Python already separated units. tests/staticunits.sh checks both orders against cc in product C O0/O1/O2, Python and rebuilt .com, including static scalar, character array, aggregate and string-pointer initializers.
- E3 keeps one program symbol/string/label space, tracks the declaring unit and resets only per-file token ordinals. The static-character-array case exposed a missing prototype path; global, automatic and static string initialization now share one emitter. E3 states fell from 4,072 to 4,058 in that refactor. The fixed list remains 237/237 equal (107,047,638 action steps), with no refusal, difference or tool failure; no acceptance list was weakened.
- New exec-multi checks 24 tape comparisons (two existing pairs, both orders, all three optimization levels, cc/unisacc drivers), native runs, independent macro/guard/static-function-pointer behavior (cc exit 27), block-static isolation (exit 19), and a missing later input without truncating output. Framing acceptance and seven malformed frames are checked on the action oracle and native inference. Host package 1,092,647 B. The new driver remains a development route, not the product default.
- First frozen run was **77/79**: exec-multi exposed the missing static-array path, and com-staticunits used direct exec on an APE file (ENOEXEC). Both were fixed without dropping tests. Final frozen gate --com: **79/79**, JOBS=2, 428 s aggregate, each suite <=60 s; exec-multi 28 s, driver 25 s, nativecheck 54 s, fat 128/0 with both slices executed. Logs /tmp/unisacc-multi-final-gate.log, /tmp/unisacc-multi-keep-final.log and /tmp/unisacc-multi-check3.log. macOS arm64/Rosetta were exercised; no fresh Linux/Windows VM run this round.
- Rebuilt product unisacc.com: 1,349,776 B, SHA256 `1ff98759433f58acc6e74e37b19e17d2c0a322934a54d958a0a69870207e190d`; generated unisacc.c SHA256 `b0c86ced0b40ee73def60b5cfffd517ad5c6b2509c632c27e9ba1d5cd2a57c8e`. It includes the storage fix, not the new default route. Remaining CLI/failure compatibility, frontend gaps, isolated compact executor and final switch remain active. Local commits only; no push/release; cc-unisacc stays paused.

- Post-gate lifecycle cleanup: the multi-input framing buffer's position array is freed before route inference (single-input NULL remains safe). This removes retained scratch storage without changing bytes or model semantics. Fresh multicheck and compilercheck both pass, including cc/unisacc drivers and the network-built driver; logs /tmp/unisacc-multi-buffer-{check,driver}.log. This one-line follow-up is covered by those targeted runs, not by the preceding frozen 79-suite run. Product source and .com are unchanged.

### S-17 continuation: independently compiled generic C kernel (2026-09-27)

- core.c/core.h now contain inference, all unchanged actions, arithmetic, input/control stacks, byte/attribute buffers, sparse memory, blobs, interning, resource-cache identities and reset/free. run.c retains decoded model/package loading, route dispatch, step-limit configuration, exact resource/file adaptation and diagnostic printing. Two fixed host symbols fetch opaque byte keys and report fatal failures; no compiler-specific primitive or model rule was added to C. Model/action bytes are unchanged.
- The first interface used function-pointer members in a host structure and exposed an E3 struct-member-call refusal. A single-active-execution kernel needs no per-instance callback table: the final interface uses ordinary fixed linkage. The independent object and default included-C build share the exact same implementation. This is an interface simplification, not a claim that E3 now supports that rejected syntax.
- Direct `cc -Os` object measurement: **arm64 __text 6,468 B, x86_64 __text 7,496 B**, uncompressed, including every generic helper. Constants/strings/static storage/unwind and imports are separately recorded in exec/c/CORE.md; gzip-9 of object text is 4,004/4,232 B only as a reference. libc memory primitives and host adapter implementations are external dependencies, not silently counted as zero whole-product cost. These are C compiler measurements, not unisacc-generated core sizes or handwritten assembly. The few-KB C baseline is now measurable; assembly and final slicing remain unfinished.
- Whole run tool on macOS arm64: cc -Os __text 16,184 B/file 55,288 B/gzip-9 14,187 B (dynamic libSystem excluded); unisacc -O2 __text 98,288 B/file 132,258 B/gzip-9 22,613 B (carried library included). Different build modes, no performance comparison claimed. The former 603-line run.c becomes 418 lines +254 core.c +32 core.h =704; this is isolation and explicit ownership, not source reduction.
- corecheck links the object separately and repeats inference/stream reset/failure/resource/container tests; its undefined-symbol audit allows only generic libc/stack-protector support and the two host entries. The opcode/arity crosscheck now reads core.h. Nativecheck again passes cc, unisacc and network-built runtime, six full-domain checks, 18 outputs, in-process chains, self-reconstruction and real short writes. Isolated self-build inputs explicitly include core.c/core.h; nothing is silently fetched from the repo.
- Frozen gate --com **80/80**, JOBS=2, 430 s aggregate, each suite <=60 s; nativecheck 51 s, fat 128/0 on both slices, chain 96/96. Log /tmp/unisacc-core-full-gate.log. Afterward the separate-linkage network checks also passed ASan+UBSan with halt-on-error, no report (/tmp/unisacc-core-sanitizer.log); leak detection was disabled because deliberate fatal-loader cases terminate without cleanup, so no leak-freedom claim. This specifically checks the new borrowed/owned resource and result boundary.
- No product source, .com or model payload changed; .com hash stays `1ff98759433f58acc6e74e37b19e17d2c0a322934a54d958a0a69870207e190d`. macOS execution this round; no new Linux/Windows VM claim. CLI/diagnostic compatibility, frontend gaps, assembly core and final default switch remain. Local commit only, no push/release; cc-unisacc stays paused.

### S-17 continuation: handwritten inference on both ISAs (2026-09-27)

- Added handwritten core_transition assembly for AArch64 and x86-64 System V. Both evaluate the actual threshold network with signed 64-bit accumulators, including negative weights, and retain the table-control path for comparison. No compiler rule or new action was added. CoreModel offsets are build-asserted against the real C struct; no external function is called from either routine. The selected development build omits the C transition body rather than calling it behind the assembly entry.
- Each ISA passes **537,620** checks against the retained C function plus independently specified missing/domain/output/64-bit-overflow expectations. The ordinary network/stream/reset/error/resource/container checks also pass with the assembly symbol linked. Four fresh six-stage images (hello, fib, b_strderef, run.c) per ISA equal the product reference; three programs per ISA run with output/status equal to system cc. The generated run.c image is the C runtime, not an all-assembly self-rebuild. Mac arm64 native and x86-64 Rosetta actually ran; Linux ELF spelling and Windows bindings are not claimed validated. Windows selection is explicitly rejected.
- Text: assembly 332/334 B (arm64/x86-64), remaining cc -Os C core 6,104/7,148 B, total 6,436/7,482 B. Each assembly object additionally has 58 B error strings; gzip-9 text 278 B each is only a reference. The original C-only baseline is 6,468/7,496 B. No significant optimization claim: actions and storage remain C, and whole-driver/libc/model costs still apply.
- Logs /tmp/unisacc-asm-arm.log and /tmp/unisacc-asm-x86.log. First host-cc comparison failed because hello omits stdio; the comparison now explicitly supplies stdio.h for host cc (product auto-includes it), and both complete scripts pass. The C-only external-linkage baseline was rechecked in /tmp/unisacc-asm-c-baseline.log. New exec-asm / exec-asmx86 jobs join the next gate. This slice used targeted checks; the previous **80/80** remains the preceding commit's full gate, not a freshly claimed 82-suite result.
- core.c only gains conditional implementation selection; default runtime, product source, shipped .com and model payload are unchanged. Full assembly action/storage migration, remaining frontend/CLI/diagnostics and final default switch remain unfinished. Local commit only; no push/release; cc-unisacc remains paused.

### S-17 continuation: handwritten fixed-width arithmetic (2026-09-27)

- The explicit assembly build now replaces both alu32 and alu64 as well as transition inference on arm64 and x86-64 System V. The original C helper bodies remain the default/reference and are excluded in this selected build; the C object has unresolved core_alu32/core_alu64/core_transition symbols, satisfied by assembly. No compiler rule or new machine primitive was introduced.
- Contract kept: low-32-bit operands with sign-extended results; modulo add/sub/mul; 5/6-bit-masked shift counts; 32-bit div/rem by zero return 0; 64-bit signed/unsigned division by zero returns 0 and sets the error flag; other 64-bit ops clear it. MIN/-1 returns MIN or zero, with explicit guards against x86 IDIV traps. Unknown operations tail-call the existing non-returning panic hook. These are execution-machine semantics, not C-language UB promises.
- Each ISA passes **108,919** value/flag comparisons against C compiled directly from core.c, including independent expected edges, plus 16 invalid-operation C/ASM invocations. The existing 537,620 inference comparisons remain green. Fresh network checks and four six-stage images per ISA equal the reference; three native program runs per ISA equal host cc. Actual execution macOS arm64 and Rosetta x86-64 only; no new Linux/Windows execution claim. Logs /tmp/unisacc-arith-{arm,x86}-route.log; C-only baseline rechecked in /tmp/unisacc-arith-c-baseline.log.
- Object __text: arithmetic 436/450 B (arm64/x86-64), transition 332/334 B, remaining cc -Os C 5,360/6,655 B; sums **6,128/7,439 B**. Arithmetic error strings 24 B each; gzip-9 arithmetic text 283/330 B for reference only. Generic storage and action dispatch are still C; this is not a full assembly core. Full runtime/OS/libc/model costs remain separately counted.
- Default runtime/product source/.com/model payload unchanged. Existing asm gate jobs now include arithmetic; no new gate category. This was a targeted recheck, not a fresh full gate (last full result remains the earlier 80/80). Next work remains storage/dispatch assembly plus frontend/CLI/diagnostic compatibility and final default switching. Local commit only, no push/release; cc-unisacc stays paused.


### S-17 continuation: assembly byte-buffer append (2026-09-27)

- The explicit development build now selects assembly append on arm64 and x86-64 System V, alongside inference/arithmetic. Bytes and their 64-bit attributes grow together via libc realloc; state layout is compile-time checked. Other storage and action dispatch remain C. Added the same pre-doubling signed-capacity overflow guard to C and assembly; default C behavior for valid sizes is unchanged.
- Each ISA passes 70,000 moving-allocation append checks, truncation/reset reuse, and six explicitly simulated C/ASM allocation/capacity failure checks. Existing 537,620 inference and 108,919 arithmetic comparisons, network checks, four six-stage images and three native comparisons remain green. C-only external linkage also passed. Logs /tmp/unisacc-buffer-{arm,x86}-route.log and /tmp/unisacc-buffer-c-baseline.log. Actual macOS arm64/Rosetta only.
- Buffer __text 208/170 B, strings 39 B each; remaining C 5,232/6,562 B. Mixed uncompressed text sums are **6,208/7,516 B**, slightly larger than the previous 6,128/7,439 B, so no size improvement claimed. Current C-only baseline including the new guard: 6,508/7,530 B. Host/libc/model costs remain outside these object sums.
- This was targeted verification, not a fresh full gate; preceding full gate remains 80/80. Existing ASM gate jobs include the new checks. Product sources, .com and model payload unchanged; local commit only, no push/release. cc-unisacc remains paused; full assembly core and product-route completion continue.


### S-17 continuation: assembly sparse indexed memory (2026-09-27)

- Migrated complete generic map lookup/insertion/growth/rehash to arm64 and x86-64 System V assembly. CoreMemory makes the former five globals one explicitly laid-out state; all fields are build-asserted. The hash, half-load growth, collision probing, overwrite and absent-zero behavior are unchanged. Stage cleanup/reset stays in C; calloc/free are explicit library dependencies. No compiler rule or action was added.
- Each ISA passed 140,000 keys through growth, zero overwrites, 64 engineered end-slot collisions, signed/extreme keys, absent reads and cleanup/reuse; backing arrays also equal the actual retained C implementation. Eight simulated fault invocations per ISA check three allocation failures and the 64-bit extent guard. The selected C object has unresolved core_memory_get/set, resolved by assembly.
- Fresh four-image six-network routes and three native program comparisons passed on Mac arm64 and Rosetta x86-64; inference/arithmetic/buffer checks remain green. First route failed because carried stdint has no SIZE_MAX; the guard now uses the explicit 64-bit storage bound and the rerun passed. C-only external linkage and ASan+UBSan network/resource/reset/failure tests passed; leak detection disabled for intentional fatal exits. Logs /tmp/unisacc-map-arm-route2.log, /tmp/unisacc-map-x86-route.log, /tmp/unisacc-map-c-baseline.log, /tmp/unisacc-map-sanitizer.log.
- Sparse memory __text 500/473 B plus 39 B strings each; remaining C 4,636/6,016 B. All assembly plus remaining C text sums **6,112/7,443 B**. C-only current baseline **6,480/7,548 B**. This is not yet an all-assembly kernel; blobs/interning, other state and action dispatch still remain in C. No new platform execution claim, product source/model/.com unchanged, no push/release; cc-unisacc remains paused.
- Frozen-tree `JOBS=2 tests/gate.sh --com`: **82/82**, failed 0, 430 s aggregate; each suite <=60 s. exec-native 52 s includes network-built runtime self-reconstruction, six full-domain checks and real write failures; ASM arm64/x86 jobs 8/10 s; fat 128/0 with both slices executed. Log /tmp/unisacc-map-full-gate.log. This supersedes the older 80-suite full-run record for this candidate. No code was edited during the run.


### S-17 continuation: binary string interning and prefix member updates (2026-09-27)

- d9b0332 extends E3 prefix ++/-- through the existing member/subscript address walk, then shared type/step/load/store logic. It was exposed by the new core's `(I)++t->n`; the initial complete route rejected it with expected-semicolon. No language-specific executor primitive was added and the source was not rewritten to evade the missing syntax. Constructor +9 net lines, states unchanged at 4,058; no code-reduction claim.
- Before growing keep-e3, all previous **237/237** passed in four disjoint <=60 s batches, 107,170,884 actions total. New prefix_members probe equals the reference and is now the 238th kept file. Its nested/narrow/unsigned/pointer members and side-effecting subscript produce independent exit **40**, actually checked on the five-image ARM64 and x86-64 routes against system cc. The two ASM route jobs now retain four old images plus this one. Evidence /tmp/unisacc-intern-debug/keep-{0,1,2,3}.log and /tmp/unisacc-intern-{arm,x86}-final.log.
- Handwritten intern/hash assembly on both ISAs preserves the existing nonstandard hash seed, binary lengths, copied ownership, insertion IDs, collision probing, growth-before-lookup and pointer-preserving rehash. CoreIntern's entry/state offsets are compile-asserted; the selected C object imports core_string_intern rather than a C fallback. Blob/resource/frame storage and dispatch remain C; allocation/copy/compare are explicit libc dependencies.
- Per ISA: 20,000 distinct strings, five fixed hash vectors, empty/NUL/prefix distinctions, 16 engineered wrapping collisions, growth on duplicate, reverse lookup, mutated input-buffer ownership and reset pass. Eight explicitly simulated C/ASM fault runs cover table/byte allocation, rehash allocation and 64-bit extent overflow. Fresh network/stream/error/package tests, five complete images and four actual native/Rosetta executions also pass.
- C-only external linkage and ASan+UBSan network/resource/reset/failure checks passed (leak detection disabled for fatal-exit cases). nativecheck passes six full-domain checks, 18 outputs, network-built runtime self-reconstruction, isolated packages and three real short writes. Logs /tmp/unisacc-intern-c-baseline.log, /tmp/unisacc-intern-sanitizer.log and /tmp/unisacc-intern-native.log.
- Intern/hash __text **508/479 B** (arm64/x86-64), strings 39 B each. Remaining C **4,144/5,533 B**; assembly plus remaining C text **6,128/7,439 B**. Current C-only baseline 6,528/7,641 B. This slice used targeted checks; last complete **82/82** gate belongs to de81ff3, not this new source/model state. Model schema/actions unchanged, E3's constructed model changes with its new path. Product sources and .com unchanged; no push/release; cc-unisacc remains paused. Full core and final product-route completion remain active.


### Assembly core: blob copies/resource ownership and required E3 addressing fix

- cc-unisacc remains paused. Product source and .com unchanged; no push/release.
- Explicit CoreBlobs/CoreResources state is shared by C and both assembly
  helpers. Blob IDs, cached absence, copied borrowed buffers and exactly-once
  freeing of host-owned buffers are preserved. Cleanup still belongs to C.
- Both macOS arm64 and Rosetta x86-64 pass 300 moving-allocation blocks,
  256 resource keys, cache/reset checks and ten simulated failure invocations.
  Resource count INT32_MAX guard is not exercised. Network tests, six full
  images per ISA and five native behavior comparisons per ISA pass.
- The new C source exposed E3's early return on &s.pointer[index]. The model
  now loads the pointer before resolving the final element address. Old 238
  fixed files passed in four bounded batches before adding the independently
  checked member_index_address probe (exit 25), making 239. No executor
  language-specific primitive was introduced.
- Blob/resource __text is 548/513 B (arm64/x86-64), strings 70 B each;
  remaining C 3,848/5,153 B; mixed text sums 6,380/7,572 B. C-only baseline
  6,664/7,707 B. This is migration, not size reduction. Host/libc/model costs
  remain separate. Native C network self-reconstruction and ASan/UBSan
  network checks pass. Logs: /tmp/unisacc-bytes-{arm-final,x86-final,native,core,sanitize}.log
  and /tmp/unisacc-bytes-keep-{0,1,2,3}.log.
- Only targeted checks were run for this slice; the last full 82/82 gate
  remains de81ff3. Complete assembly dispatch, platform bindings and the final
  product switch remain unfinished.


### Assembly core: signed decimal and reserved output fields

- Both ISA helpers now own signed decimal conversion and OFILL byte writes.
  They retain no-terminator output, INT64_MIN, right alignment, untouched
  attributes and field-overflow precedence. Bounds are checked by subtraction
  after the offset is validated, avoiding signed overflow in at + width.
- Each actual macOS arm64/Rosetta run passes 10,013 numbers and 250,325 fields
  against independent snprintf output, plus ten C/ASM bounds failures. Existing
  network checks and six images/five program executions stay equal. Native C
  network self-reconstruction and ASan/UBSan pass. Logs:
  /tmp/unisacc-format-{arm,x86,native,sanitize}.log.
- Format text 272/225 B, strings 26 B per ISA; remaining C 3,504/4,612 B;
  combined text 6,308/7,256 B. C-only 6,664/7,715 B. Allocation/host/model
  bytes remain outside these object sums. No speed claim. Dispatch, frames,
  initialization and cleanup are still C, so full assembly migration remains
  incomplete. Product unchanged, no push. Targeted checks only; last full gate
  still de81ff3 82/82.


### Assembly core: control symbols and input-frame stacks

- The two stacks now have explicit ABI-checked state and real assembly push/pop
  implementations. Symbols retain 32-bit storage; frame records retain borrowed
  byte/attribute pointers and 64-bit cursor/end. The bottom input frame is never
  popped. Signed capacity overflow is rejected before doubling; no new model
  limit. Initialization, action dispatch and final cleanup are still C.
- Per actual macOS arm64/Rosetta ISA: 20,000 symbols and frames checked through
  moving allocation, all values, reverse pops and reuse; fourteen simulated
  allocation/overflow/empty-pop failures; existing six images/five native
  comparisons pass. Network-built C self-reconstruction and ASan/UBSan pass.
  Logs /tmp/unisacc-stack-{arm,x86,native,sanitize}.log.
- Stack text 288/275 B, strings 84 B each. Remaining C 3,240/4,215 B;
  full assembly-helper-plus-C text 6,332/7,134 B. C-only baseline 6,532/7,758 B.
  These object sums exclude linked host/libc/model data, and do not mean the
  full assembly kernel or product-route switch is complete. Product unchanged;
  no push. Last complete gate remains de81ff3 82/82; this slice uses targeted
  checks and actual self-reconstruction.


### Assembly core: explicit machine state and all ARM action handlers

- All mutable action state is now one invocation-local CoreMachine (280 B,
  layout asserted), replacing C globals. C reference network checks and actual
  C network self-reconstruction passed before enabling the assembly dispatcher.
- arm64 now selects hand-written dispatch plus all 56 action implementations,
  calling only the migrated assembly primitives and generic host/libc entries.
  Its build rejects a mixed C primitive fallback. x86-64 still selects the C
  action engine explicitly; its existing assembly helpers continue to pass.
- The action test compares 1,050 full states across all 56 actions and two
  bad-action failures; includes both output selections, absent attributes,
  clipped/reversed spans, signed/unsigned values, cache reuse and repeated SWAP.
  A missing address-add caused by an assembly semicolon comment was caught and
  fixed. SWAP allocator failures are not injected by this suite.
- Both ISA jobs retain six image/five native comparisons. ASan/UBSan and C
  network self-reconstruction pass. Logs /tmp/unisacc-action-{context,
  context-native,arm-route,x86-route,sanitize}.log. Targeted checks only.
- ARM action __text 1,968 B, strings 41 B. Remaining C 980 B (outer inference
  loop, initialization, cleanup), full mixed text 6,040 B. x86 remaining C
  3,855 B, mixed text 6,774 B. C-only baseline 6,396/7,106 B, static C state
  zero. Host/libc/model bytes still excluded from object sums. No product
  switch or all-assembly self-rebuild claimed. No push/release.


### Frozen gate and x86-64 action dispatch

- Frozen 8743d4f: JOBS=2 gate --com **82/82**, 428 s total, all per-suite
  bounds <=60 s. exec-native 56 s, exec-asm 10 s, exec-asmx86 12 s; fat
  executed both slices, 128/0. Log /tmp/unisacc-action-full-gate.log. No edits
  overlapped that run. ARM action state comparisons additionally pass ASan/
  UBSan; this macOS sanitizer does not support leak detection, which was off.
- After that gate, x86-64 assembly dispatch was integrated. It passes the
  same 1,050 full-state comparisons over all 56 actions, bad-action rejection,
  all primitive checks, network checks and six-image/five-native route under
  Rosetta. Log /tmp/unisacc-action-x86-final.log. This is targeted evidence
  for the later x86 change, not a new full-gate result.
- x86 action text 2,116 B, diagnostic strings 41 B. Remaining C 1,135 B;
  total mixed text 6,170 B (ARM remains 980 B C / 6,040 B mixed). Both selected
  action engines now execute all actions in assembly; outer loop, initialization
  and cleanup remain C. Host/libc/model costs are still separate. Product
  .com unchanged, no push/release, cc-unisacc remains paused.


### Complete assembly execution lifecycle

- Both ISA adapters now use assembly for the run loop, initialization and
  ownership transfer/cleanup as well as all actions/primitives; core.c is no
  longer linked by asm/cc.sh. Shared action arities remain checked against OPS.
- Lifecycle differential checks: 62 exits with an allocation ledger, plus four
  simulated initial-allocation failures. Both macOS ISA jobs retain six image
  comparisons, five native runs and the generated C runtime comparison. Native
  C network self-rebuild and C ASan/UBSan pass. Logs:
  /tmp/unisacc-lifecycle-{arm,x86,native,sanitize}.log. No edits overlapped tests.
- Uncompressed assembly __text totals 5,976/6,195 B (arm64/x86-64), including
  lifecycle 916/1,160 B. Current C-only -Os baseline 6,436/7,146 B. Host, libc,
  data/strings, loader and model costs remain outside these sums.
- Targeted verification only; the latest full 82/82 gate remains 8743d4f.
  Product .com unchanged. Actual product ABI/OS bindings and the default
  switch remain unfinished; no assembly self-rebuild is claimed. Local work
  continues; cc-unisacc remains paused, no push/release.


### Assembly kernel through the compiler CLI

- Existing compiler, multi-unit and memory suites now require a third driver,
  linked against the full assembly kernel (no core.c). Native macOS checks:
  27 mode/level matches, 36 multi-unit tape comparisons, isolated package/file
  compilation and memory execution, macro/include handling, failure-before-
  output and real short writes. Memory suite: 36 runs per ISA across all three
  drivers, plus argv/env/O1/exit-status checks, on arm64 and Rosetta x86-64.
- Logs /tmp/unisacc-asm-{driver,multi,memory-arm,memory-x86}.log; all four suites
  exited 0, each bounded by 60 s. These are new required paths in existing
  gates, not optional tests. The package remains unchanged (host route:
  1,092,744 B arm64, 1,072,876 B x86-64). No fresh full-gate claim.
- The host-linked assembly driver is not the shipped .com. Product-internal
  ABI/carried-library binding remains necessary; x7 is the ARM tape stack and
  the first x86 tape argument is rax rather than System V rdi. Product sources
  and .com unchanged. Local commit only, cc-unisacc paused, no push/release.


### Product-ABI assembly binding and network driver self-rebuild

- Added a small tape-ABI/assembly bridge on both ISAs and nine generic carried-
  library/host services. ARM separates hardware stack use from the live x7
  tape stack; x86 aligns rsp and preserves the tape r9 frame. The entry takes
  a tagged argument record, avoiding the reference's six-register indirect-
  call limit. No language-specific action was added, no C core fallback.
- Offline macOS assembly/static link yields a self-contained, position-
  independent blob with no unresolved imports or rebasing/binding. One service
  pointer is filled before RX protection. Complete blob: **7,664 B each ISA**,
  including envelope, header/padding, strings/constants, core and bridge.
  UNISA_KERNEL is still an explicit development input, not embedded yet.
- Fresh native arm64/Rosetta x86 binding checks both pass: network/domain/
  resource/error tests; four complete images; nine CLI modes; nine memory runs;
  two multi-unit runs; missing/corrupt blob rejection; **N1=N2=N3** for the
  assembly-bound compiler driver, rebuilt through its networks using the same
  explicit fixed assembly blob. Logs /tmp/unisacc-binding-self-{arm2,x862}.log.
- Self-rebuild exposed E3's int-only function-pointer cast guard. Added signed
  long casts and made indirect results the reference's full machine word.
  New long_fptr probe equals the reference tape and executes with exit 0 under
  host cc and product. Old fixed 239 all pass in eight bounded Python-oracle
  shards before adding the probe (keep now 240). Generator +1 net line; this
  matches the measured word-result convention, not complete function types.
- Binding suites are required macOS gate entries. Linux/Windows binding,
  single-package integration and default switching remain unfinished. Product
  source/.com unchanged, cc-unisacc paused, local commits only; no push/release.

### Assembly core carried with the compiler models

- Frozen 8c3ab88 full `JOBS=2 tests/gate.sh --com`: **84/84**, all exit 0,
  467 s aggregate; each suite bounded at 60 s. Evidence:
  `/tmp/unisacc-binding-full-gate.log`. This covers that commit, not later edits.
- New development `exec/c/buildcompiler.sh OUTPUT_DIR` builds one container
  using `asmcompiler.c`, six model routes, headers and both fixed ISA core
  resources. Kernel lookup uses the package first; an external path is retained
  only for standalone tests. No C core fallback. No product/CLI source parsing
  was moved into the assembly bridge.
- Local targeted checks: carried ARM binding N1=N2=N3; isolated container with
  loose model/kernel files removed, all six hello images equal the reference,
  native/memory execution on macOS arm64 and Rosetta x86-64. Package has 23
  unique models, two cores and one container payload. Logs:
  `/tmp/unisacc-carried-bindarm.log`, `/tmp/unisacc-carried-container.log`.
- Actual Windows ARM execution caught the bridge's POSIX shared-stack assumption.
  Windows now keeps AAPCS/WinAPI on the hardware stack and tape callbacks on x7;
  x28 carries that callback stack and is reserved (the run loop's sequence count
  moved into its frame). The first fix's register conflict failed macOS and was
  corrected before acceptance. Full ARM helper/action/lifecycle checks and
  carried binding N1=N2=N3 pass again. ARM blob now 7,704 B, x86 remains 7,664 B.
- Windows then rejected the embedded .com before entry: PE offset 781 was
  unaligned. APE construction now aligns the PE signature at an 8-byte boundary
  (784 here), guarded in the constructor and embedded/container checks. The
  actual full container subsequently passed five x86-64-emulated Windows runs
  (hello/pointer at O0/O2 and argv at O1, including exit 7). ARM native passed
  those five with a carried-core package, then file/malloc/missing-input runs
  with one native image carrying its own package. This VM was stopped afterward.
  Logs `/tmp/unisacc-carried-winarm3.log`, `-wincom2.log`, `-winarmio.log`.
- Actual Lima Linux arm64: four programs at O0/O1/O2 in memory and as native
  images, six-target cross-output comparison, driver N1=N2=N3; no external
  kernel. `containerlinux.sh BUILD_DIR` preserves the opt-in reproduction.
  Linux driver bare-image SHA-256:
  `0eba58a2e360df6bd10a328351cd2e80910a0a931e41f4eda5f65c79abb155d0`.
- Development container: **3,064,442 B**, SHA-256
  `e31284381653f87e208ec963fc4459e1a3fd2724d08746e9b3fee2da370315d8`.
  Package 2,892,922 B = 2,749,364 B network/action/string models + 103,636 B
  carried headers + 15,368 B two kernel blobs + 24,554 B directory/framing.
  Other executable/launcher/compressed-slice bytes 171,504; footer 16 B. These
  are different byte categories, not an assertion that the whole compiler is
  a few KB. Final isolated macOS checks pass in `-container-final.log`.
- Still a development route: default switch, complete CLI/source parity,
  final same-input speed comparison and remaining platform scope are not
  complete. No Linux x86-64 or Windows x86-64 hardware claim from this batch.
  The reference .com will be rebuilt for the APE fix without changing its
  handwritten compiler path. No push/release; cc-unisacc remains paused.

- Acceptance at **95d4f17**: full `JOBS=2 tests/gate.sh --com` **85/85**,
  exit 0, 465 s aggregate; all suites individually <=60 s. New container suite
  18 s, ARM/x86 binding suites 15/18 s. Log `/tmp/unisacc-carried-full-gate.log`.
  Final aligned container also passed the Linux reproduction, with the same
  e3128438... hash as the actual Windows container; log `-linux-aligned.log`.
  Rebuilt reference product remains 1,349,776 B, SHA-256
  `ee67c33f323ddb4bafa4d1bc1925fe7a04aa009c906ecb86e78610b0fe5fa055`.
  Follow-up builder chmod and documentation updates do not change image bytes;
  their targeted check is recorded separately. No default-route switch.

- Post-gate targeted follow-up: builder now marks its container executable;
  `containercheck.sh` asserts that and passes (`/tmp/unisacc-carried-executable.log`);
  docs check passes. `tests/cli.sh`'s six legacy 120 s child limits were reduced
  to 60 s. Reference CLI remains **64/64**. Running the same suite directly on
  the assembly/model container gives **53 pass / 11 fail**, not product parity:
  math-header coverage, ignored build flags, -l/-L/-x, -nostdinc's two cases,
  dependency target/header output, undefined-function wording, --version,
  -dump-tokens, missing-file wording. Logs `/tmp/unisacc-carried-cli-{reference,next}.log`.
  This is the concrete next acceptance list; the 85-suite green result does
  not mean this new container passed the old product's complete CLI suite.


### Model CLI follow-up: sqrt builtins (local acceptance)

- The carried math-header rejection was not evidence of a forward-declaration
  defect: E3 lacked `__builtin_sqrt` and `__builtin_sqrtf`. Their parser-delta
  paths now select the product's signed/unsigned/double/float conversions and
  square-root operation. FPU spellings are read from irsel; f32 access width
  comes from tyinfo. No executor operation or host source parser was added.
- Before extending keep-e3, all old **240/240** accepting C-executor tapes were
  byte-identical to the stamped reference. The new `builtin_sqrt.c` also agrees
  on the Python and C executors; keep is now 241. It covers nested builtins,
  float loads, signed integers, u64, and both result precisions. Host cc and the
  rebuilt assembly/model container at O0/O1/O2, in-memory and native execution,
  all return the independently expected 3 on macOS arm64.
- Logs: `/tmp/unisacc-sqrt-keep.log`, `/tmp/unisacc-sqrt-cli.log`; scratch
  reproduction scripts `/tmp/unisacc-sqrt-{keep,run}.py`. The candidate is
  `/tmp/unisacc-sqrt-candidate/unisacc-next.com`, not the shipped product.
- CLI remains **53/64**: full math.h now reaches `isnan`'s double `!=`, which
  E3 still refuses. This batch is a local capability repair, not closure of
  the complete header test. Other ten CLI failures are unchanged. No full
  gate or cross-platform rerun is claimed for this batch. cc-unisacc remains
  paused; no push or release.


### Model math-header closure and exact decimal constants (local batch)

- E3 now handles double `!=` as inverted floating equality, including NaN.
  Decimal constants use a delta implementation of exact multiword arithmetic,
  normalisation and nearest/even rounding, for binary32/64, exponents and
  subnormals. No float-parser executor primitive or host-source parsing was
  added. The old small-decimal FCONV generator is no longer called by E3.
  Hexadecimal floating constants remain outside this slice. A 160-limb bound
  is checked before appending; oversized significands reject rather than wrap.
- Casts, sqrt builtins and typed floating arguments share conversion procedures;
  instruction spellings come from irsel. Scalar struct/union member size now
  uses ELSZ rather than treating encoded float types as byte sizes. These two
  latent errors were exposed by whole-tape comparison, after the simple CLI
  example already ran correctly.
- New `floatconstcheck.sh` compares 34 host-cc bit expectations with both table
  and network execution, including half-way cases, f32 double-rounding-sensitive
  values, overflow, subnormals and a long significand cancelling its exponent.
  Five invalid/capacity inputs reject on both routes. It is in the gate.
  Decimal/NaN and full math-header probes are also exercised by the real ASM
  network driver suite, against independently executed host-cc programs.
- Old keep-e3 241/241 stayed byte-identical before adding the two new probes
  (243 now). Both new inputs match the stamped reference on Python and C action
  executors. Actual carried-container runs at O0/O1/O2, memory and native, return
  the expected zero on both; `/tmp/unisacc-float-{keep2,oracles,run}.log`.
  CLI improved to **54/64**, including the complete carried-header case;
  `/tmp/unisacc-float-cli.log`. The other ten failures remain open.
- The long-decimal oracle found a product bug too: `10^1000 * 10^-1000` was
  discarded solely because its decimal exponent was below -800. Host cc exits
  0, the pre-fix reference 1. Product and delta now prove underflow from both
  significand bit length and exponent. `tests/c/b_longdecimal.c` covers double
  and float; generated unisacc.c and the reference .com were rebuilt.
- This batch adds a conversion algorithm to generated model logic; it is not
  a claim of fewer source lines or complete C99 floating support. No default
  route switch, push or release. Full frozen-tree acceptance follows below.
- Frozen-tree local gate: 86 suites, 85 passed and exec-driver failed; the
  failure was the new test's relative models.pkg lookup from the wrong cwd,
  not a semantic mismatch. After the run finished, the harness was corrected
  to run those calls in its isolated package directory. The independently
  rerun exec-driver passed. This records the original red run, not an invented
  86/86 rerun (`/tmp/unisacc-float-full-gate.log`,
  `/tmp/unisacc-float-driver-fixed.log`).
- Final development container: 3,076,544 B, SHA256
  `22c0e8bbca6d9ed3bb2e6d4e06597c3509c2169614a2b023819589ce4d69c377`.
  Both macOS arm64 and Rosetta x86_64 actually executed the decimal, math-header
  and long-decimal probes at O0/O1/O2, in memory and as native images, all as
  expected (`/tmp/unisacc-float-finalrun.log`). Its final CLI result is 54 pass,
  10 fail (`/tmp/unisacc-float-cli-final.log`); no CLI parity is claimed.
- Rebuilt reference product .com: 1,349,952 B, SHA256
  `3f914d98624f82122d5841611bcd4de384746761b7db837a725f0c829a14cd07`.
  This remains the default product route. No Linux or Windows execution of
  this batch's final artifacts was performed; cross-output checks do not
  establish that. cc-unisacc remains paused. No push or release.

### Model driver: source IO diagnostics

- Source reads now report `unisacc: error: cannot open PATH`, exit 1, while
  runtime package/resource failures retain their separate diagnostics. Both
  the first input and a later translation unit are checked; neither truncates
  an existing output. No source parsing moved into the driver.
- Fresh compilercheck passes (host cc, unisacc, ASM driver and network-built
  driver integration); log `/tmp/unisacc-source-io.log`. Rebuilt development
  container executed both missing-source cases on macOS arm64 and Rosetta
  x86_64, with exact stderr/rc and preserved output. Size 3,076,656 B, SHA256
  `83d5322d30fba24ffd2249e35a7d653f396e1119570e3023d6ad0daea0af9b14`.
- Targeted verification only; no new full-gate or full-CLI result claimed.
  Default reference .com unchanged. No push/release; cc-unisacc stays paused.

### Model driver: shared version query

- `src/version.h` is the single product version declaration. build_ref embeds
  it into the standalone unisacc.c; the model driver includes it directly.
  ua_ready hashes it too, so a version-only edit cannot reuse a stale reference.
- `--version` and `-version` return before package/source loading. Fresh
  compilercheck passes, checking cc/unisacc/ASM builds from outside the repo
  without a package. Rebuilt container also executes both queries on macOS
  arm64 and Rosetta x86_64: exact `unisacc 0.0.7`, exit 0, no stderr.
- Development container: 3,076,896 B, SHA256
  `e9214bae33045acb0c5dbc6438de4f7b3383885f3af9aca1184428b65b592f03`.
  Reference .com rebuilt and unchanged byte-for-byte (SHA256
  `3f914d98624f82122d5841611bcd4de384746761b7db837a725f0c829a14cd07`).
  docs: 3 generated tables, stale 0, referee ledger ok. Logs are
  `/tmp/unisacc-version-{check,build,com,docs}.log`. Targeted checks only;
  no new full gate, platform-wide validation, default switch, push or release.

### Model CLI: compatibility arguments and -nostdinc preprocessing

- Driver consumes attached/separate -l/-L/-x arguments, matching the product's
  no-linker, C-only compatibility behaviour. Missing arguments fail explicitly.
  -g and -std= are accepted with the same no-debug/no-dialect-switch behaviour;
  warning options are not silently treated as implemented.
- -nostdinc is a raw resource. E2, not host C, disables built-in include search
  and automatic headers while preserving source-relative and explicit -I
  lookup. CLI resources were expanded by one slot; process/memory/import
  slots moved together. No executor operation was added.
- Fresh compilercheck passes the implemented contracts, including Python
  action-oracle vs network preprocessing. Its printed KNOWN line explicitly
  records that undeclared printf's runtime fallback remains unfinished.
  The complete CLI suite remains red: **58 pass / 6 fail**. Remaining: warning
  flags, -nostdinc printf conversion/lowering, dependency target/header output,
  undefined-function diagnostic, token dump. Logs:
  `/tmp/unisacc-nostd-finalcheck.log`, `/tmp/unisacc-compat-final-cli.log`.
- An attempted fallback expansion was withdrawn after tape comparison exposed
  literal-pool order (argument strings must precede format fragments). Another
  concrete blocker is DO.print's explicit rejection in exec/lower/code.py:
  faithful migration requires the itoa TIns encoder on both ISAs. Scratch
  all-conversion probe and tapes remain under `/tmp/unisacc-nostd-*`; none of
  that incorrect parser trial remains in the tree. This is not printf closure.
- Rebuilt development container: 3,080,406 B, SHA256
  `714b4b41a45cd10f71b8489f7c761c09eb9b43827f4aeebd8ccbbadbf40a7725`.
  Both macOS arm64 and Rosetta x86_64 executed attached/separate compatibility
  options plus -nostdinc with a user header; built-in stdio rejection checked
  on both. No full gate or Linux/Windows execution claimed for this batch.
  Default product unchanged; no push/release; cc-unisacc remains paused.

### Model printf continuation: .print / itoa through both encoders

- Lowering now handles .print with the reference's scratch spill, itoa, and
  write ABI sequence. Print-buffer/address and length arguments are resolved
  by the delta. Both ISA encoders emit the signed decimal count/write loops;
  x86 RIP operands wait for final relaxation, ARM ADRP uses the final layout.
  No formatting or encoder primitive was added to the generic executor.
- Integer conversion selection/control is explicit generated code and emitted
  machine-code templates, not a newly discovered gold-table fact. This adds
  source and model bytes; no code-size reduction is claimed.
- ARM encoding: three OS layouts, page-crossing fixtures and both executors
  agree with reference bytes. x86: 53 fixtures pass, including a branch before
  itoa, and rejects for a register address, missing operand, lone minus and a
  too-wide decimal address. The new x86 address parser bounds each digit before
  wraparound; existing operand paths retain their previous semantics.
- Typed lowering suites for all six targets pass, with .print added to their
  shared fixture; both executors compare the complete typed fields. Fresh
  compilercheck passes decimal fallback tape equality and actual output for
  0, 1, -1, 10, INT64_MAX and INT64_MIN. Logs:
  `/tmp/unisacc-{arm-itoa,x86-itoa-final,print-driver,print-lower-x86,print-lower-arm,print-lower-win}.log`.
- Rebuilt development container: 3,193,943 B, SHA256
  `8693dad1ce748ed72346b3f18b81cf3dbd50a4a69a3e89dd57c37cb13ce329b7`.
  Actual macOS arm64 and Rosetta x86_64 execution at O0/O1/O2, memory and native
  output, prints `0/1/-1/10/9223372036854775807/-9223372036854775808` correctly.
  No new Linux/Windows native execution or full-gate result is claimed.
- Full undeclared printf fallback remains unfinished: pfconv conversion
  dispatch and argument-string-before-format-fragment pool order still need
  migration. The old 58/64 CLI result is not relabelled green by this slice.
  Default product unchanged, no push/release, cc-unisacc stays paused.

### Model printf fallback: conversion dispatch and nested arguments

- E3 now reads pfconv for d/i/u/x/X/o/p/c/s and emits the reference fallback's
  explicit tape helpers. Declared printf remains an ordinary function call.
  This preserves the fallback's zero return and ignored width/precision;
  it is not a claim of full libc printf semantics (notably fallback %p).
- Format strings are decoded before scanning, including escaped percent and
  adjacent literals. Argument literals are pooled before format fragments;
  nested calls record their actual frame slots instead of assuming contiguous
  slots. No executor primitive was added. Source grows; no reduction claimed.
- Final generated E3 preserves the fixed 243 accepting tapes byte for byte.
  Fresh compilercheck passes: all nine conversion letters, nested calls,
  literal-pool order, escaped/adjacent formats, exact tape and actual output
  on cc/unisacc/assembly drivers. Permanent probe:
  exec/parse2/probes/printf_fallback.c. Logs:
  /tmp/unisacc-pf-keep-final.log and /tmp/unisacc-pf-driver.log.
- Fresh development container: 3,211,436 B, SHA256
  49cfc5183488fe36a95fda2466e4fa50cc3ccdb223fd290eb36564cfa778c4f2.
  Actual macOS arm64 and Rosetta x86_64, O0/O1/O2, memory and native output:
  all pass the permanent probe. /tmp/unisacc-pf-native.log.
- Full candidate CLI suite: 59/64, exit 1. Remaining: warning options,
  dependency target/header output, undefined-function diagnostic, token dump.
  /tmp/unisacc-pf-cli.log. No full-gate or Linux/Windows execution claimed.
  Default product unchanged; no push/release; FX conjectures remain unscheduled.

### Model compiler token-dump route

- -dump-tokens selects a package route containing E2 and the existing plain
  E1 network. Runtime C does not decode tokens. The route uses Linux/x86_64
  predefines, matching the reference token instrument's fixed target; normal
  compilation keeps typed E1 output and its selected target. Multiple-source
  token dumps remain explicitly rejected.
- Fresh compilercheck: cc, unisacc and assembly drivers match full reference
  output for declarations, macro expansion, string/hex literals and predefines;
  existing driver/printf tests also pass. /tmp/unisacc-token-driver.log.
- Rebuilt container: 3,254,497 B, SHA256
  14eec9e7764e1c59ed80ef4ab4aa15ba77308545a54e043ad209e755844f7cb6.
  Embedded route executed on macOS arm64 and Rosetta x86_64 and matched the
  reference; /tmp/unisacc-token-native.log. No Linux/Windows run claimed.
- Candidate CLI: 60/64, exit 1. Remaining: warning options, dependency target
  and header output, undefined-function diagnostic. /tmp/unisacc-token-cli.log.
  No full gate claimed. Default product unchanged; local commit, no push.

### Model compiler dependency-file output

- -MD/-MMD/-MF now write make-style input/header prerequisites after successful
  compilation. The generic host adapter optionally records successful disk
  resource reads; the model still decides which include is active and which
  path to request. Carried/process resources and failed reads are excluded.
  No new executor action or C-side include parser was introduced.
- Duplicate paths are canonicalised (including reads shared across units),
  unlike the reference's repeated include entries; the prerequisite set is
  preserved. Formatting/default names follow the reference. Like the existing
  product, no additional make escaping for whitespace in filenames is claimed.
- Fresh compilercheck passes on cc/unisacc/assembly drivers: nested headers,
  inactive missing header, repeated include, multiple inputs, carried stdio,
  explicit/default dependency paths, missing -MF argument and dependency IO
  failure preserving the existing compiled output. All prior driver checks
  pass as well. /tmp/unisacc-deps-driver-final.log. The first trial used a
  Linux-default -S route absent from the single-host test package; the test
  now explicitly selects its packaged target before -S.
- Rebuilt development container: 3,260,145 B, SHA256
  5752a0e8f840175db0219647efa0eb13d2546a87a39c036861154d27d02baaba.
  Actual macOS arm64 and Rosetta x86_64 dependency output matches reference;
  /tmp/unisacc-deps-native.log. Candidate CLI is 62/64, exit 1: warning options
  and undefined-function diagnostic remain. /tmp/unisacc-deps-cli.log.
- Default compiler unchanged; no full-gate or Linux/Windows native result
  claimed for this batch. No push/release. Reconstruction remains incomplete.

### Model compiler deferred undefined-function diagnostics

- E3 no longer rejects an ordinary call merely because the definition has not
  been read yet. Unknown calls initially use the reference's scalar result
  descriptor. After all input units and helper generation, unresolved.py scans
  the actual tape twice: collect labels, then check calls in emission order.
  This includes nested-call ordering and reports each missing name once.
  Prototypes alone do not satisfy the check. This migrates the reference's
  behaviour, including its acceptance of an undeclared call defined later;
  it is not a new C99 conformance claim for implicit declarations.
- Diagnostics and singular/plural error counts are model-produced bytes on
  the existing error stream. Empty rejection reasons no longer cause the host
  to prepend a spurious 'reject:' line. No language-specific runtime primitive
  or C-side tape scan was added. The byte scans add generated rules/work;
  no source-size or speed improvement claimed.
- Fixed 243 accepting tapes remain byte-identical. E3 self-source produces
  3,929,446 bytes equal to the reference under the 60 s watchdog. Fresh
  compilercheck passes on cc/unisacc/assembly drivers: unknown names,
  declaration without definition, duplicate calls, nested calls, later
  same-unit and later-unit definitions, and fail-before-output protection.
  Logs: /tmp/unisacc-ud-{keep,self,driver}.log.
- Rebuilt container: 3,262,731 B, SHA256
  6f79bf96c2b6e91a4d80c48dbb2897ed0545c69a161f185b83bf4bc507e0863b.
  macOS arm64 and Rosetta x86_64 execute the later-definition program with
  expected exit 7 and produce exact nested missing-function diagnostics.
  /tmp/unisacc-ud-native.log. CLI 63/64, exit 1; -Wall remains unmigrated.
  /tmp/unisacc-ud-cli.log. No full gate or new Linux/Windows execution claimed.
- Default product unchanged. Local commit only, no push/release; full
  reconstruction and the final product switch are still incomplete.

### Warning migration prerequisite: successful diagnostic streams

- Audited -Wall: the reference implements unused-variable, return-type,
  printf-format and integer-to-pointer diagnostics; -Wextra shares the switch,
  while -Werror prevents output/run when warnings exist. These cannot be
  replaced by silently accepting a flag. Model CLI still rejects them.
- Fixed a generic host-adapter defect: execute() discarded the model error
  byte stream on successful acceptance. It now forwards a nonempty diagnostic
  stream independently of acceptance. Python's action oracle also preserves
  successful diagnostics via an optional collector (existing return shape
  retained) and its CLI; empty rejection reasons do not prepend a fake line.
  No executor action or compiler-specific primitive added.
- netcheck adds exact status/stdout/stderr evidence for successful diagnostics,
  two-stage order, and a rejecting later stage (no partial stdout or later
  model execution). Same actions pass in the Python oracle. The fixture
  serializer now uses the production '-' spelling for an empty string.
- Passed netcheck with host cc, unisacc, arm64 assembly and Rosetta x86_64
  assembly runtimes. Fresh compilercheck remains green. Logs:
  /tmp/unisacc-warning-channel{,-ua,-arm,-x86}.log and
  /tmp/unisacc-warning-driver.log.
- Rebuilt development container: 3,262,731 B, SHA256
  1effcec46db3a186a378e569e32a1aa6f4fe10149fb68b681b5c162d9b27e7c3.
  Exact missing-function diagnostics and later-definition execution pass on
  both macOS ISAs (/tmp/unisacc-warning-native.log). This is a transport fix,
  not completion of warning analysis; previous CLI 63/64 is not raised.
- Remaining warning work includes original-file positions, header suppression,
  continuation/include adjustments and the four analysis rules. E2 currently
  retains splice/include line records internally but does not export them to
  E1/E3; token streams do not yet carry source positions. Do not infer source
  locations by searching token text. Default product unchanged; no push,
  release, full-gate claim or new Linux/Windows run in this batch.

### Warning source-location prerequisite: E2 diagnostic envelope

- Optional offline `exec/pp/gen.py --locations` now constructs an E2 network
  that emits pp.locations: the exact preprocessed text plus forced/automatic
  prefix counts, splice offsets, and ordered header insertion regions/names.
  The binary envelope is specified in exec/pipeline/formats.md. All framing
  is generated model actions; no host parser or new executor primitive.
- Header names follow the reference diagnostic map's first-62-byte limit.
  A forced-include test in the long macOS scratch path exposed this; the
  diagnostic-only model now reproduces it explicitly. Actual file lookup
  still uses the complete path.
- locationcheck.py builds a scratch-only instrumented current reference at
  the real expandsrc/lex boundary. Nine complete records+text are byte-equal:
  plain, macro, LF/CRLF splice, multiline comment, nested/repeated includes,
  forced include and automatic stdio. Five small cases additionally agree
  with the Python action oracle. cc, unisacc and both macOS ISA assembly
  runtimes pass all nine. Logs /tmp/unisacc-location-{check,ua,arm,x86}.log.
- Default E2 states and action sequences compare exactly with the pre-change
  generator for all six targets (/tmp/unisacc-location-default.log). No
  default container rebuild is needed for this unused optional mode; the
  existing container and default product are unchanged. exec-pploc joins the
  bounded local gate; no full-gate or Linux/Windows native claim in this batch.
- This exports the actual position-recovery inputs only. E1/E3 do not consume
  the envelope yet, the compiler route does not select it, and warning rules
  are not migrated. CLI remains 63/64, not a completed -Wall implementation.
  Next is model consumption/token positions and warning analysis, retaining
  header suppression and the reference's macro-expanded-column convention.
  Local commit only; no push/release. Reconstruction remains active.

### E1 token positions — optional, not yet a warning route

- `exec/lex/gen.py --positions` emits typed tokens with fixed-size binary
  prefixes holding the saved token-start offset in the preprocessed buffer;
  EOF carries the buffer length. Generic model actions perform the output;
  no token/position logic was added to the C or assembly executor.
- `exec/lex/positioncheck.py` builds an instrumented reference in scratch,
  captures the actual lexer input and tpos values, and compares nine cases:
  empty, ordinary, macro, splice, multiline comment, string/char prefixes,
  numeric/operator lookahead, skipped attributes, and offsets above 255.
  All pass on cc and unisacc C builds and both macOS assembly runtimes
  (x86_64 through Rosetta), plus the Python action oracle. Logs:
  `/tmp/unisacc-lex-position{,-ua,-arm,-x86}.log`.
- Removing the six-byte prefix before each token yields exactly ordinary
  typed output. Default and --typed states/action sequences are unchanged
  from 84b18d5. exec-lexpos joins the bounded gate; no full-gate or other
  platform claim in this batch. No product/container rebuild or switch.
- Remaining: connect pp.locations and token offsets to E3, then implement
  warning decisions and rendering. This does not make -Wall available;
  CLI remains last measured 63/64. FX-3 remains an unscheduled conjecture.

### Located E2/E1/E3 route — optional diagnostic input plumbing

- E1 --locations now consumes pp.locations, validates its framing, preserves
  the complete map and emits positioned typed tokens in tokens.locations.
  E3 --locations reads that envelope and retains source text, splice records,
  forced/automatic prefix counts and include-region/name records. Each NEXT
  records source_pos and TOKEN_POS keyed by the original token-buffer position;
  parser rewinds and bounded string/initializer views still use that buffer.
  All operations are model actions; no language-specific host primitive.
- Six E2/E1 joined cases match direct positioned lexing and the Python action
  oracle; eleven bad preprocessing frames reject without output. Eight joined
  E2/E1/E3 cases produce unchanged tape: small, macro, splice, explicit header,
  adjacent strings/array initializer, automatic printf header, fib, strderef.
  A test-only model continuation reads every retained map field back out and
  reproduces the E2 envelope exactly. Ten malformed token frames reject with
  no partial tape. E3 join tests pass on cc, unisacc and both macOS assembly
  runtimes (x86_64 through Rosetta): /tmp/unisacc-e3-location-{chain,ua,arm,x86}.log.
- E1 default/typed/positions serialized models are unchanged. E3 default is
  byte-identical to ac3009d with the same PYTHONHASHSEED=0 (24,349,909 B); an
  initial comparison without a fixed seed differed because generator set
  iteration is seed-dependent. No runtime semantics change is inferred from
  that uncontrolled serialization comparison.
- exec-lexloc and exec-parseloc join the bounded gate. This batch did not run
  the whole gate or Linux/Windows native tests. No product/container rebuild
  or switch: compiler routes do not select this mode. Remaining is diagnostic
  rendering and warning rules, plus multi-unit location framing and CLI
  -Wall/-Wextra/-Werror integration. Last measured CLI remains 63/64.
  Local commit only; no push/release. Full reconstruction remains active.


### Diagnostic renderer and first warning rule — development route

- Optional E3 --warnings (implies --locations) implements reference
  -Wreturn-type decisions and summary, plus source/caret rendering and header
  warning suppression. Compiler CLI still does not select it: full -Wall,
  other warning kinds, multi-unit integration and -Werror remain unfinished.
- Renderer: 11 cases x 2 modes compare with actual reference diagnostic
  functions. Return warnings: 32 cases, 14 with warnings, compare tape and
  stderr; quiet mode also matches tape with empty stderr. Both checks passed
  with cc, unisacc and macOS arm64/x86_64 assembly runtimes (Rosetta for x86).
- These tests exposed an existing E3 floating-condition gap: it omitted
  reference ftruthy conversion. Model FTRUTH/FNOT now cover if/loops/ternary,
  logical operands and unary !, preserving negative-zero and NaN behavior.
  Product/reference code was already correct. float_truth.c runs successfully
  with host cc and the reference; quiet/warning model tapes match reference.
- Before adding float_truth.c, all old 243 kept tapes passed unchanged.
  The new probe is separately verified and raises keep-e3 to 244. Self-source
  tape remains byte-identical (3,929,446 B). compilercheck.sh passed all
  27 mode/level matches and its existing CLI/resource/IO/decimal checks.
- exec-diag and exec-returnwarn join the bounded gate. No whole-gate result,
  Linux/Windows native claim, default .com switch or release in this batch.
  Default E3 model changes for the floating-condition fix; no serialized-model
  identity claim. Runtime remains generic model actions.

- Rebuilt development container (not shipped default): 3,265,064 B,
  SHA-256 b8c29bed5211b39071ee4fc58ae0de255efed0c1199cc2cc7594a225c1a12f63.
  Its float_truth probe compiled and ran with exit 0 as native osx/arm64,
  osx/x86_64 (Rosetta), and memory -run. The subprocess harness must launch
  the APE through /bin/sh on macOS; a direct subprocess exec first raised
  ENOEXEC before compilation, then the corrected harness passed.


### Integer-to-pointer warning rule — optional E3 diagnostic route

- --warnings adds the reference intptr_check rule for assignments (names,
  members, indexed/dereferenced lvalues) and local scalar initializers. It
  preserves the lone numeric-zero exemption and call-result tracking,
  including the difference between direct and function-pointer calls.
  It does not extend checks to returns, compound assignments or global/static
  initializers where the reference does not run intptr_check.
- A chained assignment exposed an older E3 descriptor difference: stores
  returned target facts where the reference normally retains RHS facts.
  AS.result now retains RHS facts except for a floating target's setkind.
  Store width still uses the target. This is reference compatibility, not a
  claim that all reference conversion semantics are proved C-correct.
- 29 cases compare full tape and diagnostic bytes, 16 with actual warnings;
  the quiet route also matches. Passed on cc, unisacc, macOS arm64 assembly
  and x86_64 assembly (Rosetta). Includes zero/parenthesized zero/enum/casts,
  nested assignment, direct/indirect calls, member/index/star stores, header
  suppression and explicit no-check contexts. Existing 32 return-warning
  cases also pass (14 warn). Original 244 kept tapes and self-source tape
  (3,929,446 B) remain byte-identical. No keep-list expansion in this batch.
- exec-intwarn enters the bounded gate. Full -Wall remains unavailable:
  unused-variable and format warnings, multi-unit location integration,
  -Werror and CLI selection remain. No full-gate or new native-platform claim.
  Product source and shipped .com unchanged; local work, no push/release.


### Located reader repair: static-local token ordinals

- Source inspection found that the located NEXT entry bypassed IX.have,
  which assigns stable source-token ordinals for static local labels. A new
  two-function static-local fixture failed before the fix with "identifier
  is not a local" (rc 1). This was a development location-mode defect, not
  a shipped product regression.
- The located reader now preserves IX.have/IX.new after its position prefix;
  it passes the ordinal table explicitly. The isolated diagnostic-renderer
  test continuation has no grammar ordinal table and retains its own entry.
- Nine location-mode tape comparisons and ten malformed-frame checks pass
  on cc, unisacc and both macOS assembly runtimes. The new static case is
  also in the return-warning check: 33 cases, 14 with warnings, exact tape
  and diagnostics on cc. This prevents a green location suite that only
  exercises automatic/global storage.
- Current compilercheck.sh passes 27 mode/level comparisons plus its existing
  CLI/resources/IO/decimal/native and memory-run checks; package 1,250,262 B.
  No full-gate result, default product switch or publication is claimed.
  Remaining warning/CLI work and the full reconstruction goal remain active.


### Unused-variable warnings — optional E3 mode

- The model now tracks binding/use records alongside the existing scope undo
  stack. It restores shadowed bindings, reports in source declaration order,
  excludes parameters and static/global storage as the reference does, and
  uses primary()'s token-context rule for reads versus standalone writes.
  Token facts come from the existing whole-token INDEX walk; declaration
  locations feed the shared diagnostic renderer. No executor primitive added.
- 38 cases (21 warning-producing) match complete reference tape and stderr
  on cc, unisacc, macOS arm64 assembly and x86_64 assembly via Rosetta.
  Cases include shadow/restore (automatic, static and enum), arrays, members,
  indirect calls, sizeof, nested blocks, for scopes, initializer reads,
  multiple functions, name truncation, macros, splices and header suppression.
  An initial splice fixture contained literal backslash-n instead of a
  physical continued line; corrected before the successful runs.
- Existing 33 return-warning and 29 integer-conversion cases also pass on cc.
  Default (non-warning) E3 serialization is unchanged from ec628eb with fixed
  PYTHONHASHSEED=0: 24,570,115 B, directly compared. Thus no default product
  rebuild or new default-route regression claim in this optional-only batch.
- New rule module: 52 source lines plus parser hooks and tests; this migrates
  the rule into model actions, not a code-size reduction. exec-unusedwarn is
  added to the bounded gate. No full-gate/platform/release claim. Format
  warnings, multi-unit warning framing, -Werror and CLI integration remain;
  reconstruction is still active and the shipped .com is unchanged.


### Format warnings and scalar-float default argument promotion

- Optional E3 --warnings now includes the reference printf preflight and
  format checks, using generic model actions and the shared source/caret
  renderer. It preserves literal decoding, star arguments, call exemptions,
  nested diagnostics and the reference label-allocation side effects. Quiet
  and warning tapes are each compared against the matching reference mode.
  No language-specific executor primitive was added.
- 36 cases, 28 with format warnings, pass with complete tape/stderr comparison
  on cc, unisacc, macOS arm64 assembly and x86_64 assembly/Rosetta. The initial
  unsplit unisacc check hit the 60 s alarm (rc 142), not a pass; two disjoint
  18-case shards pass with 15 and 13 warning cases. The bounded gate gains
  these two shards. Existing return/int-conversion/unused checks pass on cc
  (33/29/38 cases). No full-gate result is claimed.
- A quiet-route difference exposed missing default scalar-float promotion
  when no formal kind is recorded. The parser now uses the existing TO.d
  conversion there. Original 244 kept tapes passed before list expansion;
  the new vararg_float probe brings the keep list to 245. Self-source tape
  is still 3,929,446 B and byte-identical; compilercheck passes 27 mode/level
  comparisons and its existing CLI/IO/native/memory checks.
- Fresh development container: 3,265,512 B, SHA-256
  97e8a6a33dc5e9898d1e6489b6dfb05769e30ef25217e2d3a335ab2322ef79fb.
  The new probe prints exactly `1.5 2.5` (rc 0) in memory and as native
  osx/arm64 and osx/x86_64 programs. Shipped unisacc.com is unchanged.
- Four warning rules are implemented in optional single-unit mode; -Wall
  CLI selection, multi-unit locations and -Werror remain. No default product
  switch, new Linux/Windows execution claim, push or release. Reconstruction
  remains active; FX conjectures do not add implementation scope.


### Single-unit warning CLI and Werror acceptance barrier

- The development package now carries target-specific located preprocessors
  plus shared located lexer/warning-parser networks. Its driver routes
  single-source -Wall/-Wextra/-Werror to them. Warning decisions stay in the
  parser model; -Werror is an explicit byte resource, and the model rejects
  after its summary when the warning count is nonzero. Empty rejection keeps
  the reference stderr without adding a runtime reason. No executor action
  added, and no C-side diagnostic-text classification.
- c/warningcheck.sh passes 60 complete rc/stdout/stderr comparisons across
  host-C, unisacc-C and assembly-backed drivers, under alarm 60. It covers
  all three flags, three optimization levels, preprocessing/token dumping,
  and rejection before output/dependency mutation or execution. A clean
  -Werror program still runs. Multi-unit warning mode remains an explicit
  refusal until its source maps are integrated; no full-parity claim.
- This test exposed an existing token-dump mismatch: its preprocessor was
  performing implicit header selection. That route now constructs E2 with
  E2_AUTOINC=0, matching the reference lexer instrument; explicit includes
  remain supported. Compilation routes retain implicit header selection.
- compilercheck.sh remains green (27 mode/level comparisons and its other
  contracts). A rebuilt development container passes tests/cli.sh 64/64
  from an isolated directory, plus six full warning-mode tape/run result
  comparisons. Container: 4,830,280 B; SHA-256
  a4f47e3f329ba8a5a135e460dd8c694a6b7cca24ff986052e02b5e707ac189f6.
  Warning models increased the container from 3,265,512 B; no size reduction
  claimed. Offline generation still uses Python; runtime does not.
- These are local macOS arm64 results, not a full gate or cross-platform
  release result. The default shipped .com is unchanged. No push/release.
  Next: per-unit located framing and warning diagnostics for multiple inputs;
  the complete reconstruction goal remains active.

### Multi-unit warning maps and per-file diagnostic state

- The located unit-framing model emits UNITOK2: filenames and independent
  UNIPP1 source maps followed by unit-indexed positioned tokens. E3 reloads
  the appropriate context on every read/rewind. Static-name isolation stays
  in the existing scanner; C only frames bytes and selects routes.
- The physical-token scanner now retains qualifiers and shares the parser's
  multiline adjacent-string reader. Warning neighbor facts include the unit
  epoch. Warning names and printf recognition use original preprocessed
  spelling, avoiding internal __uN suffixes in diagnostics or missed checks.
- c/multiwarningcheck.sh passes 120 complete rc/tape/stderr comparisons across
  host-C, unisacc-C and assembly-backed drivers: both file orders, four warning
  kinds, headers, splices, static shadowing, adjacent strings, empty/clean units,
  optimization, run and Werror barriers. Single-unit entry points remain.
- unitlocationcheck.py passes on cc, unisacc and both macOS assembly ISAs:
  independently serialized bytes, UTF-8 filenames, reference diagnostics,
  ten malformed map containers and seven malformed input frames. No executor
  primitive added. This is migration evidence, not a proof over all inputs.
- Testing found a product defect: fe_load retained prior units' splice,
  include-name/region and automatic-include maps. It now resets nspl, nireg,
  nfnpool and nautoinc per file, while program warning counts remain shared.
  tests/diagunits checks 18 fixed file/line/column expectations at O0/O1/O2 in
  both orders; host cc independently confirms the warned source line. The
  old shipped artifact fails this check (missing a warning); the rebuilt
  reference passes. Another pre-fix observation reported line 5 for line 4.
- Frozen-tree gate evidence: 101 distinct suites, all rc 0 and each at most
  60 s. The serial aggregate's existing 900 s outer watchdog expired (142)
  after 71 saved successful receipts; a scratch copy of the same gate skipped
  exactly those names and ran the remaining 30 successfully in 208 s. This
  is complete coverage across two invocations, not a successful first aggregate.
- Rebuilt reference unisacc.com: 1,350,016 B, SHA-256
  bbff6f182d8b90060b0e9527432869d180cd58046e48ab972f197aac6e1c9165.
  Its product-facing gate cases passed, including diagunits and CLI 64/64.
- Separate development assembly/network container: 5,294,871 B, SHA-256
  788155c72a685ec924efb16de3c07a5565f98fe4f75f8c51b8d6622b0c5d2cbf.
  Actual container CLI passes 64/64 from an isolated directory, plus 10 full
  result comparisons for multi-unit warning/tape/run modes in both orders,
  including Werror output/dependency preservation. Size increased; no reduction
  claimed. Offline construction still uses Python.
- These are macOS arm64/Rosetta results and cross-generation checks; no Linux
  or Windows VM was run for this batch. No default model-route switch, push
  or release. The overall reconstruction goal remains active.


### Located parser errors and bounded recovery (development route)

- Normal and warning compilation routes now retain per-unit source maps.
  E3 --errors maps unknown identifiers, expression starts and expected
  punctuation to reference diagnostics, unwinds scopes and input views, and
  resumes at a balanced top-level boundary. Any error prevents tape publication.
  Other prototype limitations retain their explicit not-covered reason, gain
  a location, and stop; this is not reference-error equivalence.
- The driver carries -ferror-limit= as an opaque resource; the model reads it
  (default 20, zero unlimited). No diagnostic classification or recovery was
  added to the C executor. Existing warning rules remain model actions.
- errorcheck: 17 complete rc/stdout/stderr comparisons plus one located
  prototype limitation on host C, unisacc C and both macOS assembly cores.
  Every loaded network is enumerated against its table. Multi-unit checks:
  120 warning comparisons plus 18 error/recovery/limit comparisons, both
  file orders, across three drivers. Ordinary multi-unit checks also pass:
  36 tape comparisons, native runs, scope isolation and failure preservation.
- The combined driver suite exceeded its outer 60 s budget (rc 142); it is
  split into core/resources with the same assertions. Both bounded parts
  pass. diag.sh's two legacy 120 s child alarms are now 60 s. These targeted
  results do not claim a new complete gate or new-platform validation.
- Actual development unisacc-next.com: 5,735,300 B, SHA-256
  417a110d6593a5cbb1091fd472858a195c51af744a30e4c66728964807756bb1.
  diag 14/14 (including 40 damaged inputs), CLI 64/64. ccparity at this slice
  is 50 ok, 3 wrong, 1 known: C99 macro examples ex3/ex4/ex7 still fail;
  the known -c object-file incompatibility remains. Macro parity is next.
- Product sources/default .com are unchanged. No default model-route switch,
  push or release; offline construction still uses Python. FX-1..FX-4 remain
  unscheduled conjectures, not prerequisites for this work.


### Active compatibility work: macro invocation and replacement lists

The remaining ccparity macro failures reach explicit E2 limitations: zero-argument
and variadic calls, replacement-list boundary crossing, and hash/paste handling.
Continue the existing model path by reproducing the three examples, then add
only the missing preprocessing semantics with focused reference comparisons.
Runtime decisions must remain generic actions in constructed models. Preserve
existing byte output, hide-set behaviour and located preprocessing; do not
replace the model route with a C macro expander or count a rejection as parity.


### Macro invocation/rescan compatibility accepted (development route)

- E2 now parses zero-parameter and final variadic parameters, preserves the
  remaining commas as variadic argument text, and collects a macro call across
  replacement-frame boundaries without crossing argument-expansion barriers.
  It handles object-like # and the standard # ## # form, strips internal
  separators from operands, and removes a hide mark only on a pasted boundary
  token. Other tokens in the same argument retain their hide marks. This last
  distinction caught and fixed a repeated expansion in a multi-token operand.
- macrocheck.py covers 20 inputs: the three C99 examples from ccparity plus
  focused zero/variadic/cross-frame/stringize/paste/hide cases. Both ordinary
  and located preprocessing produce exact reference bytes (40 comparisons);
  system cc independently agrees on preprocessing tokens. Python simulation
  and actual constructed-network execution agree. All pass with host C,
  unisacc C and both macOS assembly cores; network=table enumeration is run
  for both formats. The suite is in gate as exec-macros.
- Fixed chain list: 96/96 equal, 0 rejected/not-covered/bad/lost. Existing
  location suite: 9 reference envelopes and 5 Python oracle cases pass.
  Full current-source osx/arm64 route: separate stages and shared package
  give the same 743,202 B image as the reference; native N1=N2=N3.
- Actual development container: 5,761,242 B, SHA-256
  36d173b7f138b430eaaf1b574cad8e602cb473291404172e5e176435f2a83826.
  ccparity is now 53 ok, 0 wrong, 1 existing known (-c is not an object file);
  diag remains 14/14 and CLI 64/64. These runs use the container itself.
- Size account: gen.py 1,254 -> 1,307 lines; ordinary E2 642 -> 671 states,
  164,787 -> 172,240 entries. Its Linux/x86-64 network is 64,375 B; the complete
  package is 5,577,930 B. No new executor action; no source-size reduction
  claim. The container grew 25,942 B over the diagnostic slice.
- Every invoked suite/build was bounded at 60 s. This is targeted validation,
  not a new complete release gate or Linux/Windows native run. General
  punctuator/literal paste forms, _Pragma and other declared E2 limitations
  remain; passing these examples does not prove all preprocessing semantics.
  Default product remains the reference; no push/release or switch this batch.


### Broader candidate audit after e291398

Actual model container 36d173b7... passes ccparity but is not ready to replace
all product paths. C99 reports 39/57, 18 refusals: empty variadics, __func__,
_Pragma, hexadecimal floating literals, bool/_Bool, restrict/inline, VLAs and
VLA parameters, flexible/static array parameters, compound literals, va_copy,
math/atexit/div/labs and signal declarations. run.sh passes 11 with one failure:
b_float stops at mixed float/double arithmetic (double-operand limitation).
These remain implementation work, not waived baseline entries.

The C99 harness also loses the original status through `if ! command; then
rc=$?`, and ignores a host/non-host status mismatch on the success branch.
Fix status capture and bound the host build/run before relying on expanded
coverage. Verify with a compiler wrapper that returns correct output then
exits 2; it must fail. No product compiler change is needed for that fix.


C99 harness status fix verified: host compilation and execution are now bounded;
compiler/run status is captured before branching, compared on both paths, and
signal/timeout-shaped exits fail instead of becoming unsupported cases. A
scratch wrapper emitted the reference output, suppressed stderr and exited 2:
the old script falsely passed 57/57 with rc 0, the corrected script reports
57 wrong and exits 1. Normal reference remains 57/57; the actual model
container remains 39/57 with the same 18 refusals and rc 1. The baseline was
not lowered and no known-failure waiver was added. Next: finish the remaining
preprocessor gaps in that list, then front-end type/declarator/expression gaps.


### Next E2 compatibility slice

Migrate the reference's comma-before-final-variadic-argument rule into the
existing macro rewrite delta. Empty raw arguments remove the comma; nonempty
arguments retain it without token pasting. This is GNU-compatible reference
behaviour, not a claim about mandatory C99 semantics. No executor action or
product-source change. Pin both empty and nonempty cases before proceeding
to the remaining _Pragma and front-end gaps.

Comma/variadic slice verified: 25 probes in both ordinary and located formats
(50 full outputs), on host C and both macOS assembly executors. All match the
reference and Python simulator; 24 cases additionally match host preprocessing
tokens. Explicitly empty final arguments have a separate hand token expectation
because host compilers may retain their comma. Network/table enumeration passes.
No executor primitive added; gen.py grows 1,307 -> 1,322 lines.

Actual development candidate is 5,770,202 B, SHA-256
985d90cc87bc837305bc99059d5a5ce0c01b0d89f7a78b6254cce286f650d038;
package 5,586,890 B. C99 is 40/57, 0 wrong, 17 refused (suite correctly exits
1); ccparity remains 53 ok, 0 wrong, 1 existing known difference. The empty
variadics probe is now supported. Logs: /tmp/unisacc-comma-{check,arm,x86,
build,c99,ccparity}.log. Every run bounded at 60 s. This is targeted validation,
not a release gate or default-product switch. _Pragma and the other 16 refusals
remain work; no baseline reduction, push or release.


### E2 _Pragma operator migration

The reference consumes a balanced _Pragma call without honouring its contents
or expanding its arguments. Implement that scan with ordinary delta actions,
including quoted parentheses, replacement-frame boundaries and source newline
accounting. Bare names remain text. Compare exact reference output separately
from host pragma semantics: this is not an implementation of pack or diagnostic
pragmas. Unterminated calls remain an explicit prototype limitation.

_Pragma slice verified: 43 macro probes in both output formats (86 full
outputs), plus two malformed-call refusals per format. Host C and both macOS
assembly runtimes agree with the simulator and reference; constructed network
= table enumeration also passes. Twenty-four probe token sequences additionally
match host cc; nineteen use explicit reference-contract token expectations
(comma extension and ignored pragma behaviour). A self-referential _Pragma
name exposed lost suppression after frame lookahead; the name's active/paint
state is now captured before popping, and the formerly looping case is fixed
on the permanent list. No new executor action. gen.py: 1,322 -> 1,376 lines.

Existing location checks pass (9 reference envelopes, 5 simulator cases),
fixed chain 96/96 with no rejects/not-covered/bad/lost. Actual model container:
5,796,265 B, package 5,612,953 B, SHA-256
8c341754b0354e6f07a02b1055d995d3421c72d9fb2f936dfe8eb57e4cca9ac9.
ccparity remains 53 ok / 0 wrong / 1 known. C99 now 41/57, 0 wrong, 16 refused;
the suite still correctly exits 1. The reference's ignored _Pragma is covered;
pack semantics are not newly implemented or claimed. Logs are
/tmp/unisacc-pragma-{check,arm,x86,location,chain,build,c99,ccparity}.log.
All runs bounded at 60 s. No default-product switch, push or release. Next are
__func__ and the remaining type/declarator/expression/library compatibility gaps.


### E3 predefined function name

Implement __func__ inside function bodies as a pooled name, preserving the
reference's source-order literal numbering and its 120-byte name bound. The
reference currently writes an explicit NUL in this pool entry and the usual
pool terminator, so byte comparison must retain both. Use normal delta string
operations, no executor intrinsic. Outside-function use stays explicitly
unsupported; it is not a C99 use of the predefined identifier.

A required __func__ subscript probe exposed a reference defect: its primary
expression only sets curptr/curelem, retaining curpd and other kind state, so
__func__[0] emits no character load. Model tape has the load and therefore
differs. Fix the product descriptor through setkind before accepting parity;
retain a character-access regression in the existing C99 feature probe. The
Python front end has no __func__ implementation to update. Rebuild the product
.com and run its bounded gate after this product-source change.

The committed-reference binary was rebuilt privately for a before/after test.
With first() returning __func__[0], it printed an address-valued integer
(648511783 in that run) instead of 102. Fixed reference, rebuilt product .com
and host cc print `main 109 109 102`. The first model candidate still rejected
*__func__ through its specialised dereference-name path; wire that path to the
same pooled-name value before reporting model acceptance. The earlier direct
printf argument happened to work and is not the defect's regression proof.
The existing C99 probe now includes this independent function. Old fixed chain
96 passed before adding the two new function-name files (98/98 total).

The first frozen gate was cancelled (rc 137), not counted as validation, after
the model dereference gap became known. Re-run the complete bounded gate after
the fix. An earlier scratch command incorrectly sent Python stdin through
term.sh and timed out; the subsequent test uses a real script file.


### Owner correction: aggregate timeout must remain bounded

The second function-name gate was incorrectly launched with TERM_SH_ALARM=0.
Per-suite alarms did not bound the overall background run. At the owner's
correction the gate and its 10 descendants were killed; no gate success is
claimed. Set terminal/gate defaults to 60 seconds and reject zero or values
over 60. Remaining full validation must be split into bounded batches. The
function-name/product changes remain uncommitted pending that validation.

Timeout correction follow-up: merely setting alarm(60) on the wrapper still
leaves descendants alive. Add a small process-group watchdog that preserves
exit status and kills the whole owned group on timeout/interruption. Gate runs
will require explicit suite selection; --list enumerates the same authoritative
job declarations without executing them. Validate selection before running.
No unbounded aggregate gate will be relaunched.

The new timeout probe passed: an outer 1-second watchdog terminated a nested
30-second watchdog and its sleeping child, returned 142 in 1.29 seconds;
normal exit 2 remained 2. Gate rejects absent/unknown selections before builds.
Next enforce the same outer limit for direct TERM_SH=0 gate calls, and bound
the Terminal handoff wait as well as the command. Full gate is now a sequence
of explicit selected batches; an interrupted batch supplies no passing receipt.

### Test efficiency: rolling queue and shared preparation

Owner explicitly requests parallel queue scheduling and measured efficiency.
Current frozen checks: 102/105 pass; exec-native, exec-multi and exec-memx86
hit the 60-second limit (not passes). All other queued checks finished.
Inspection found elf.sh reconstructs the same six models in every private
suite directory. Separate immutable, content-checked preparation from source
execution; key it by generator/table/header/runtime inputs, target, network
mode and compiler identity, serialize identical builds, and publish atomically.
Cache hits still verify artifact hashes; cold preparation retains full-domain
network checks. This changes preparation reuse, not suite assertions.
Make rolling scheduling reusable with a bounded scheduling window and retained
per-suite results, so work continues without hand-written batch barriers.
The three timeouts must be rerun after the preparation improvement; no waiver.

The first rolling-window rerun retained partial logs: native reaches the stream
chain but not resource-packaged self checks; multi reaches only the first pair.
Both still exceed 53 s under two-way load, so caching alone is insufficient.
Split native by stages / chain / resources; split multi and POSIX memory checks
by compiler backend (cc / ua / asm), retaining the union of assertions. Build
only the backend used in a shard. The queue keeps late-window timeouts pending
for an early full-window attempt; a full-window timeout remains a failure.

Rolling queue validation: 12/12 replacement shards pass; the longest is
46.34 s (Rosetta/UA memory). Native stages/chain/resources took
20.86/30.32/17.36 s. Cold model preparation was 4.36 s, warm verified reuse
0.08 s. A simultaneous same-key cold request built once (4.20 s); the second
waited and reused it (4.09 s), all 21 artifacts identical. Queue/cache controls
pass for rolling refill, resume, changed-input refusal, nonzero exit despite
PASS text, timeout, artifact corruption, missing manifest entry and header
invalidation. Terminal output is now streamed (measured 2.05 s between lines
emitted two seconds apart), not buffered until completion.
The backend split initially repeated backend-independent framing/loader
checks in every shard. Keep those in the cc shard only (and all mode), retaining
their original two-executor / two-build checks, and skip their extra preparation
in ua/asm shards. Every selected backend still runs its entire behavior matrix.

After corruption, a cold rebuild restored all executable tables, networks and
packages byte-for-byte. Two JSON files differed only in dictionary key order
(parsed objects equal). Pin PYTHONHASHSEED=0 in cold preparation so cached
intermediate JSON is also reproducible; this is not a model-behavior change.
The post-deduplication queue passed 10/10, with windows 51.41/48.16/30.70 s.

The final current-key corruption experiment rebuilt the private cache and
restored all 21 artifacts byte-for-byte, now including JSON (4.17/4.25 s cold).
Two concurrent same-key callers build only once; a subsequent warm check was
0.08 s. These are preparation timings, not a claimed whole-suite speedup.
Queue windows retain all completed results, validate the frozen input snapshot,
and use exit 75 for pending work; no pending task is reported green. The legacy
release caller now invokes one bounded queue window and treats pending as not
ready. Its older all.sh release orchestration is not run or certified here.

cc-unisacc supplied two new reported product defects (not yet independently
executed in this session): unsized static-local and file-scope function-pointer
arrays. Source reading finds fpdecl defaults [] to one slot and the local
function-pointer-array path precedes the static-storage branch. Reproduce with
host cc and bounded runs, then fix the declaration/storage rules, not a special
case for 3 or 8 elements. They remain open; test-green is not a C99 proof.
The requested section 5.6 roadmap is retained as explicitly unverified advice,
not a replacement for S-17 acceptance criteria or a completion percentage claim.

Final targeted queue on the finished infrastructure: 6/6 pass in one 35.39 s
window (docs, kernel, C99 57/57, complete self-source through Linux x86_64 and
arm64 network routes, queue/cache controls). The two full-source images equal
the reference at 671422 / 720572 B. This is targeted revalidation after harness
changes, not a claim that one fresh invocation reran the entire new 114-item
list. Earlier bounded runs plus replacement shards cover the old gate's tests.
No push, release, default-route switch, or new cross-platform execution claim.

Function-name slice closed locally: __func__ and its direct dereference use the
same captured function-name pool entry path; the fixed chain is 98/98. Product
C99 is 57/57 including `main 109 109 102`; the actual model container is 42/57,
0 wrong, 15 explicit refusals (not a green C99 claim). Product .com SHA-256:
61d01e7a7017df9d9c31ece1a874a403fe998163c910283a8da342f5226be27e,
1,350,032 B. The remaining declaration/library gaps and reported function-
pointer-array defects remain open. Test infrastructure is committed separately
as 1cad167; no pending unrelated changes are swept into the product slice.


Function-pointer-array correction in progress (2026-09-27): independently
reproduced both reported probes on the current product .com. Host cc exits 0
(argc 1); product local probe prints 652742492, global eight-pointer probe
terminates with SIGSEGV. fpdecl treats an unsized [] as one pointer, and the
local array branch bypasses static storage. Fix the inferred extent and route
static arrays through the existing static-storage initializer. Check explicit
and inferred extents, automatic/global/static storage, and persistence across
calls. Python's declarator/local_decl already represents an unsized array and
infers its initializer count; verify before deciding it needs a change.

Function-pointer-array fix, targeted evidence: product -O0/-O1/-O2 in both
-run and native forms, plus unchanged Python front end, equal host cc on
b_staticfnptr_local (1 71 61 30 20 24 24) and b_fnptr_table8 (56 1 64 72).
These cover inferred/explicit extents, automatic/global/static storage,
zero-filled trailing slots, frame preservation and static persistence.
E3 uses the shared initializer counter and static storage walker; global
function-pointer initializers now enter the existing initializer path.
Old fixed E3 245 + new 2 equal using the C table executor; both new cases
also equal on the Python simulator. Source -> E2/E1/E3 threshold networks:
old 98 + new 2 = 100 equal, 0 lost/bad/refused. Only after those runs were
keep-e3/keep-chain raised to 247/100. E3 = 4492 states, 8777 hidden units,
506991 B network, exhaustive network/table check on 1158680 observations.
Full fresh gate --com queue follows on this tree; targeted success does not
replace it. No release or default-route switch.

Gate observation: closure-c2 found a probe-environment mismatch, not an image
or array-value mismatch. Its `$UA source -run` invocation passes the trailing
-run as a program argument (argc=2), while the tape VM sees argc=1. New local
probe now captures argc at main entry and checks it remains unchanged after
array initialization, rather than requiring a fixed argument count. All six
images already matched; the static-array values were identical. Continue the
frozen run, then recheck the corrected probe through both executors/closure;
do not erase the original failed receipt or call it a passing run.

Function-pointer-array slice verified (frozen candidate based on 31e033d plus
this patch): `/tmp/unisacc-fp-frozen-gate` completed all 114 gate --com items,
112 pass and two original failures (closure-c2, difftest_o), both solely the
new probe's argc=1 assumption. Preserve that failed run. After changing ONLY
the probe to compare argc with its entry value, `/tmp/unisacc-fp-finalcheck`
passed closure-c2 (186 identical images; 31 host runs), difftest_o (390 agree,
0 wrong) and the 100-file network chain. No product/model source hash changed
between these runs. This is full-list evidence plus focused correction, not
one pristine 114/114 run. Longest suite 49.861 s; rolling queue uses two slots,
all windows bounded below 60 s, no timeout failures. Earlier root-tree window
was invalidated on a concurrent example edit; its receipts were not reused.

Both final probes also match host cc with product and actual model container
at -O0/-O1/-O2, in -run and native compiled execution. Unchanged Python front
end agrees. Product .com: 1350992 B, SHA-256
bbb4bd59b4b0e9b5c2f92b5fbd28571ad3408be39b3e08a4e7d4c64edd9424ca.
Development container `/tmp/unisacc-fp-modelcandidate/unisacc-next.com`:
5819785 B, SHA-256
fae217ff8fefbe923912535e17cc639dba3197ac9413ec7960e81d7d3e704bdd.
Fresh model C99: 42/57, 0 wrong, 15 refusals, exit 1; remaining compatibility
work is not waived. Product C99 57/57 is not proof of language completeness.
No default-path switch, push, release, or new VM execution claim.

External example updates 76bf9a0/31e033d/2287f58 are cc-unisacc's independent
work. Optional user-facing process/directory/syscall bindings are reported
as absent, not scheduled in this repair. Their new VM claims were not rerun
by cdx; no such claim is inferred from the image-comparison gates above.

Next compatibility slice: declaration-only `restrict` and `inline` follow the
reference's qualifier handling. Share the token-reader skip policy with the
unit-framing pass, which must preserve physical tokens and static-name spans.
Do not fold static/extern into this policy: they affect storage/linkage. Verify
the C99 probes and fixed E3 list; this does not claim full C99 coverage.

Qualifier slice verified: old E3 247 plus s56_qualifiers all equal on the C
table executor; the new probe is also equal through all three threshold
networks. Old network chain 100/100 remains green. Located multi-unit test
now includes a static inline function with a restrict parameter and passes
independent framing bytes plus reference tape/diagnostics. Both gate jobs
ran in a two-slot bounded queue: 3.26 s and 9.84 s, wall 9.88 s. After those
checks the E3/chain keep sets become 248/101. No executor action added.
Actual rebuilt development container at /tmp/unisacc-qual-candidate:
C99 44/57, 0 wrong, 13 refusals (previously 42/57); restrict and inline
are now accepted with correct output. The suite correctly exits 1 against
the unchanged 57 baseline. No default switch or completeness claim.

Next floating-literal slice: hexadecimal significands reuse the existing
limb ratio normalizer and nearest-even packer. Parse base-16 digits and a
mandatory binary p exponent in the delta, then add the binary exponent to
the normalized ratio exponent; no host floating parser or executor action.
Check host-cc bit patterns at rounding/subnormal/overflow boundaries, and
full source through the actual model compiler. _Bool stays open: its width
and normalization must be represented across declaration/conversion paths.

Hexadecimal floating slice verified: 61 host-cc bit-pattern expectations
(decimal plus hex), table and threshold-network execution agree; 10 invalid
forms reject on both. Covers ties-to-even, normal/subnormal boundary, half
minimum subnormal, maximum finite/overflow, huge signed exponents and a
1001-digit hexadecimal significand whose exponent cancels its scale.
Old E3 248 plus C99/07 all equal, old source/network chain 101/101 retained;
new C99/07 separately equal through the network chain. Keep lists raised
only afterwards to E3 249 and chain 102. Two-slot gate queue: float bits
3.14 s, chain 9.86 s, total 9.90 s. No new executor action.
Rebuilt /tmp/unisacc-hex-candidate/unisacc-next.com SHA-256
048f9444d5bc86fabc88dd0b8857a6f006f1eea273c957c58f7528a7f7f620bf:
actual C99 45/57, wrong 0, refused 12, rc 1 against unchanged baseline 57.
The reference product was not changed; no release/default switch.

va_copy investigation: stdarg.h already expands it to ((destination) =
(source)); the missing model rule is parenthesized lvalues, not a va_copy
builtin. Add a syntax-only lookahead for parenthesized assignment/update,
then compute its address once using existing name/member/index/dereference
walkers. Share the assignment, compound-update and postfix emitters. Verify
ordinary parenthesized rvalues stay unchanged, nested/index side effects
occur once, and copied va_list cursors advance independently.

Parenthesized assignment/update slice verified: old E3 249 plus three
probes equal. The first keep run rejected exec/c/run.c because (I)++n was
misclassified as postfix update; type-name lookahead now preserves the
cast/prefix interpretation, with a dedicated regression in s57. Old chain
102/102 and the three new files independently pass network execution.
Keep sets raised afterwards to 252/105. Self-source tape 3933309 B is
identical; two queued gate jobs took 3.28/9.97 s, wall 10.02 s.
Actual network compiler matches host cc on va_copy, b_lvalue and s57 at
-O0/-O1/-O2: 14; 9 8 9 22 / 9 9; 1 6 7 5 18 respectively. s57 tests
copy after consuming one variadic argument, independent cursor advances,
nested parentheses, one index side effect and cast/prefix disambiguation.
This adds expression-entry parenthesized lvalue handling; it is not a claim
that every possible nested lvalue grammar is covered. No va_copy intrinsic
or executor primitive was added.
Actual /tmp/unisacc-lvalue-candidate/unisacc-next.com SHA-256
6d0e545a03ed637b69b359d17bd835231ad700e186b3ced9d28c5c31e5515aa8:
C99 46/57, 0 wrong, 11 refusals, rc 1 against baseline 57. No product
default switch, release, or platform-validation claim.

Next declaration slice: signal.h exposes a general missing shape, a function
returning a function pointer (`T (*name(params))(result_params)`). Reuse the
existing parameter/body walk and FP signature skipper; carry the wrapper
explicitly until the outer declarator closes. Enable void-return function
pointer casts through the existing FP descriptor. No signal-specific model
rule; test a renamed factory plus the actual header and preserve fixed lists.

Function-pointer return declarations verified: old E3 252 plus signal and
s58 factory = 254 equal. Old network chain 105/105 remains green; both new
files separately equal through threshold networks. Keeps raised afterwards
to 254/107. Self-source tape remains 3933309 B identical. Queue two jobs
3.33/10.12 s, wall 10.16 s. No executor primitive or signal-specific rule.
Actual new container at -O0/-O1/-O2 equals host cc: signal prints
1 / 15 / still here; factory prints 8 4 7 1 12. The factory includes a
prototype, callback parameter, immediate call of the returned pointer,
void callback return/cast and an ordinary function after the wrapped one.
/tmp/unisacc-fpret-candidate/unisacc-next.com SHA-256
11561d7b3c450faeecbd3556f9e2af7dd64a944bb833b345102a2c66edd44f19.
Fresh model C99 47/57, 0 wrong, 10 refusals, rc 1 against baseline 57.
This does not extend the bundled signal implementation to external OS
signal delivery. Product sources and the default product remain unchanged.

_Bool audit before model migration found reference-product defects. A valid
probe prints `1 1 1 1 1 1 0 1 1 0` with host cc but product prints
`1 0 0 0 2 2 0 2 2 0`: floating assignment, cast, function return/parameter,
aggregate initializer and increment fail to normalize. Fix the existing
conversion-kind 9 propagation at these boundaries first; preserve boolean
type metadata through typedefs and declaration lists, and preserve postfix
old values explicitly. Do not reproduce these errors in the delta.

Bool validation: native -O0/-O1/-O2 match cc on b_boolconv. The bounded gate stopped after 27/114 (one failure): fat exposed that Python resolves _Bool as int. Repair the same scalar conversion contract there, keeping its arithmetic axis u8; do not waive the probe.

Python bool repair keeps a boolean flag on the existing u8 type axis; conversions, constant aggregate initialization and old-value postfix semantics now agree with host cc. b_boolconv covers typedefs, float/int/pointer truth, argument/return, arrays/members and updates. Native O0/O1/O2 and Python fat arm64/x86_64 pass. Full gate restarts on the changed tree; earlier 27 results are not reused.

Bool fix acceptance: final unchanged-tree rolling queue /tmp/unisacc-bool-final,
114/114 passed, zero failed/pending, jobs=2, every scheduling window <=55 s
and each child bounded <=60 s. Longest suite exec-multi-ua: 49.655 s.
Product C99 57/57; difftest_o 393/393; fat 132/132 (arm64 and Rosetta
x86_64 executed); fixed model chain 107/107. New b_boolconv agrees with host
cc in native and rebuilt .com at O0/O1/O2; Python fat probe also agrees.
.com 1,354,144 B, SHA256
53556829bb402fe5ab7e2abbb4ee0a9103af71c538d5d67800b7b22ba6467bd4.
No push/release/default-route switch. Model _Bool migration remains pending;
model C99 remains 47/57, not the product's 57/57. This closes the measured
reference conversion defects, not all C99 conformance obligations.

Model bool slice: reserve descriptor 66 between f32 and function-pointer;
map its arithmetic axis to existing u8, its size to one byte. A shared
source-kind-aware TO.b comparison handles integer/pointer and f32/f64;
route declaration, aggregate, assignment, cast, return and parameter
conversion through it. Prefix/postfix updates normalize and preserve the
old postfix value. No executor primitive; validate fixed keeps before
adding bool probes, then actual network compiler against host cc.

Model bool checks: old E3 254 plus C99/13, C99/14 and b_boolconv all equal;
old network chain 107/107 remains green, three additions independently pass
network inference. Keeps raised only afterwards to 257/110. No executor
primitive. Float unary negation now flips the sign bit (needed for -0.0);
boolean floating compound updates use the existing arithmetic tape ops and
normalize their result. Self-source tape 3942700 B identical. Two-slot queue
chain/self/location all pass, 11.03 s wall. Descriptor size and arithmetic
reuse tyinfo u8; boolean conversion remains an explicit semantic rule.

Bool candidate /tmp/unisacc-bool-candidate/unisacc-next.com:
SHA256 6e535da2ca834f1dc7a826d7a5d538787d7ec133e571031e9f793e33c2dfd706,
5,877,754 B. Actual compiler C99 50/57, wrong 0, refused 7, rc 1 against
unchanged baseline 57. Unary floating negation also enables 54_math_c99.
Five probes (13,14,b_boolconv,54,s59) at O0/O1/O2 equal host cc in actual
network-compiler execution. s59 adds f32 conversion/compound update,
negative zero, minimum f32 subnormal, pointer stride and sizeof bool/array.
Math and s59 also independently pass the network source chain; fixed sets
become E3 259, chain 112. Current delta: 4,835 states, 533,503 B network,
1,247,174 finite observations match table including action/string identity.
This is added behavior, not code reduction (gen2 +73/-19, plus shared bool
procedures and static/unit hooks). No executor action, product switch,
release or push. Remaining C99 refusals: VLA, VLA parameter, flexible
array member, static array parameter, scalar/array compound literal,
atexit/div/labs aggregate-return case. Full reconstruction remains open.

### 模型数组形参接入（进行中）

在布尔切片后的模型 C99 50/57 基线上，下一步接入一维数组形参的指针调整。先支持空界、单个整数或标识符界及 static/const/restrict/volatile 修饰；不跳过任意表达式，带副作用的界与多维形参仍明确拒绝，避免复制参考前端忽略界求值的行为。复用现有 DECL、PDB 与指针寻址，不新增执行器动作；固定旧覆盖后再增加用例。

数组形参实现账：旧 E3 259 全保留，另两个 C99 用例 tape 相等；s60 覆盖 int/char/_Bool/double、static 与 const 界，三项经真实网络链路相等后加入固定清单（E3 262、chain 115）。复用参数描述符与 DECL，gen2 净增 10 行，无新执行器原语。候选 `/tmp/unisacc-arrayparam-candidate/unisacc-next.com` 为 5,880,954 B，SHA256 `d20f59d47b3b570b0898ccd39ae846b79985accb6f4fb46bac2ef24ee5c577b6`；三项在 -O0/-O1/-O2 与 host cc 同输出。原 C99 清单实跑 52/57、wrong 0、refused 5、rc 1，未降低 57 的门槛。解析网络 4,850 状态、534,393 B，1,251,044 个观测 network=table。

固定清单队列 `/tmp/unisacc-arrayparam-final` 双槽：chain 115/115（11.83 s）、self 3,942,700 B 相等（3.39 s）、unitlocations（3.43 s），3/3 通过，窗口 11.87 s。实际候选对 `a[n++]` 与 `a[2][3]` 均 rc 1 拒绝；前者在带位置诊断模式报 expected ]，不声称已实现 VLA 界表达式求值。下一项仍为柔性数组成员、自动 VLA、复合字面量及聚合返回局部初始化；本轮不改出货 `.com`。

### 柔性数组成员迁移（进行中）

复用成员布局表，MAR=-1 表示柔性数组、MSZ=0、MFLAT=0，保留元素对齐与类型；成员存在性不能仅看 MSZ。仅接入结构体最后一个、且前有命名成员的柔性数组，沿用数组到指针的访问路径。不新增执行器原语，先验证旧 E3 清单与真实候选运行，再登记覆盖。

柔性数组扩展探针发现参考产品缺陷：`struct P { int count; int *tail[]; }; p->tail[0]=&n` 的成员路径先 load64 再下标，`/tmp/ua_ref` 实跑 rc139。根因是 mbwidth==0 的数组退化错误地限定 mbptr==0，且 mbelem 已变为数组元素宽度8、丢失最终 pointee 宽度。先修产品：独立保存成员基类型宽度，数组成员增加一层指针深度且不提前加载，覆盖固定与柔性指针数组；不复制错误到模型。

产品修复后，固定/柔性指针数组与 s61（char/double/指针/struct 元素）在 host cc 和原生参考 -O0/-O1/-O2 同输出，三项网络链路与修复参考 tape 相同。旧262项在修产品前已通过；现冻结源码，完整 --com 队列验证后才结案。新增三项固定清单：E3 265、chain 118。

### 5.9 附记：getdents/execve 在 Windows 上暴露了一个既有的死数据缺陷（2026-09-27，后台 opus 代理在独立 worktree 里发现，未合并主树）

按 §5.9 的清单，第 1、2 项（`__execve`、`__getdents64`）已经在一个独立 git worktree（`.claude/worktrees/agent-a25e85490a837aeb3`，分支 `worktree-agent-a25e85490a837aeb3`，未合并、未 push、分叉点在 `3359cc5`，**已落后主分支约 40 个提交**，含本节前面的数组形参、柔性数组成员等工作，合并前需要重新对齐）里做出来并验证：Linux/macOS 四个目标 `-run`/`-O0`/`-O2` 均与 host cc 逐字节一致（目录列举内容相同，`execve` 真实执行 `/bin/echo hello` 输出 `hello`）；lnx/x86_64 因为本机 Lima 虚拟机停着没测。

**过程中发现一个真实缺陷，与这次改动本身无关，是既有数据**：`weights/gold/abi.tsv` 里 `clone`/`execve` 的 `win/x86_64`、`win/arm64` 行，`winimp` 字段是字面量 `none`——但 `winimp` 词表本身把字符串 `"none"` 登记为**合法词表项、下标 0**（`#head winimp - none ExitProcess WriteFile ...`），所以 `back_encode.c` 的 `bk_impof` 会正常"查到"它，返回下标 0，代码生成器就会去调 IAT 里下标 0 的导入槎——不对应任何真实 Windows API，运行时行为未定义。这两行数据在这次改动之前是**死数据**：没有任何前端内建能触达 `clone`/`execve`，所以从未被真正编译过。现在 `__execve` 接上前端后，这条路径**变得可达**：`-b win/x86_64` 编译一个用 `__execve` 的程序，编译器返回码 0（接受），没有拒绝，运行时结果未验证（没有 Windows 虚拟机手边）。这正是项目契约最忌讳的一类：**接受了输入却不能正确匹配参考行为，必须拒绝，不能悄悄接受**。`getdents` 的 win 行是这次新加的，抄了 `clone`/`execve` 的既有模式，所以带着同一个问题一起加了进去。

**已安排修复**（同一个后台代理续做，在同一个 worktree 里）：在 lowering 阶段加一道通用检查——`gate` 为 `winapi` 但 `winimp` 恰好是字面量 `"none"` 时一律拒绝（不止 `execve`/`getdents`/`clone` 三个，做成对任何 op 都生效的防御），C 端与 Python 端都要改、行为要一致。结果记入本节下一次更新，或由 cdx 接手核对。

**这一条本身给 FX-4"规格优先"提供了一个具体例证**：这个缺陷之所以潜伏了这么久没被发现，正是因为 `abi.tsv` 里的占位行本身看不出"哪些是真实现、哪些是占位"——`winimp=none` 既可能表示"这个目标真的不需要导入"，也可能表示"没人填"，两种语义共用一个值。分层覆盖或规则化规格（FX-4）如果要求"占位"必须显式区别于"真的是 none"，这类缺陷会在构造期就被查出来，不必等到有真实调用路径才暴露。

门禁来源事故：cc 会话确认其隔离 worktree 后台测试在15:59/16:05覆盖共享 `/tmp/ua_ref.c`/二进制。旧队列 `/tmp/unisacc-flex-final` 的70项记录含 difftest_o 旧版缺陷12错，不能当本轮完整验收；不篡改旧结果。改用本树独占 `/tmp/unisacc-flex-private`（SHA256 3f9a74fcf2118cfcca8cefa51a5ea4e2ac3821d53ef21131b4dd51e7ed3ddd9b），unitlocationcheck 尊重 UA。exec-driver-core 单独排队（此前负载超时53s、单独50.68s），其他113项双槽；两份清单的并集必须恰好等于114项。

队列效率修正：私有参考下 exec-driver-core 单独仍在53s预算处超时。源码检查确认 compilercheck.py 的 core 分支不读取 driver.com，仅 resources 分支使用；compilercheck.sh 现在仅 all/resources 打包 APE，删除 core 的无用准备，保留全部 core 断言及 resources 的包执行测试。

柔性数组收尾证据：新队列 `/tmp/unisacc-flex-isolated` 113/113 全绿，加独立 `/tmp/unisacc-flex-isolated-core2` 的 core=0，程序核对两份请求/结果并集恰好等于 `gate --com` 114 项，无缺无重；私有 UA 哈希始终为 3f9a74fc...。最大单项52.906s，所有调度窗口小于55s。difftest_o396/396、fat133/133、产品C9957/57、chain118/118；E3固定265项用私有token dumper/参考在C表执行器下全部相同。共享污染的旧队列不计入此结论。

产品 `.com` 已重建：1,355,008 B，SHA256 `b3ef68bc2e2e9941309ea394ca7b81138ebebd3cfffe95760ac736b51d61dc53`。独立模型候选 `/tmp/unisacc-flex-candidate/unisacc-next.com`：5,884,843 B，SHA256 `05dce8d8bbd154c46e7b75fe85850910e2c84955ad6e5689a1e3ccfd2025e7e4`。候选真实运行：s61与b_memberptrarray在O0/O1/O2均同host cc；union柔性成员、首个柔性成员、非最后柔性成员均rc1无输出。解析网络4,871状态、535,474 B、1,256,462个观测network=table（含动作与字符串）。本轮增加布局分支与产品基类型元数据，不称代码减少。

模型C99现为53/57、wrong0、refused4、rc1，保留原57基线。余项：自动VLA、结构体复合字面量、数组复合字面量、atexit/div/labs聚合返回初始化。没有切换默认产品路线、发布或推送；cc会话的系统调用扩展与Windows winimp=none修复仍在隔离分支，待本轮提交后另审，不能快进覆盖候选树。

### 系统调用隔离分支合入审查（9228faa，暂未合入）

审查发现：新增execve/getdents与目录头文件仅改abi_audit映射，无持久功能回归；需保存真实execve成功/失败、目录多批读取与Windows双目标拒绝、合法WinAPI不受影响的测试。C/Python guard方向正确，但 exec/lower/code.py 的两个abi循环仍只排除非Windows sysno=none，Windows winapi+winimp=none仍被生成；合入必须同步模型拒绝，不能只修参考。新DIR内部fd/pos/len/base/ent/buf字段未用实现保留名，重现既有头文件宏污染风险；需与先定义同名宏的输入核对。共享/tmp输出必须私有化。本轮不快进，不采用分支的unisacc.c覆盖主线成员指针数组修复，后续合入按最终源码重生成。

### 模型结构体表达式初始化（进行中）

剩余C99的div/ldiv用例拒绝于width：局部 `struct S x = expression` 错走标量STOREV，已有结构体赋值COPYSTRUCT未复用。按参考先保存目标地址，再求RHS并调用已有同类型结构体复制；初始列表保留原路径，无新执行器动作。先用私有参考固定265项回归，再测真实模型候选。

结构体表达式初始化固定清单：旧265项与新增5d/s62（带填充字段、函数返回、声明逗号、3字节结构体、独立副本、调用次数）共267项与私有参考tape一致；新增两项网络链路相同后加入chain，现120项。解析网络4,881状态、536,676 B、1,259,042观测与表相同。实现只将目标地址保留与RHS求值接到已有AS.struct/COPYSTRUCT，不新增动作或复制算法。

结构体表达式初始化收尾：双槽队列 `/tmp/unisacc-structinit-final` chain120/120（12.33s）、自身3,946,271 B tape相等（3.32s）、unitlocations（3.41s），3/3通过，窗口12.37s。真实模型候选 `/tmp/unisacc-structinit-candidate/unisacc-next.com` 5,887,318 B，SHA256 `50399aa6bf889ea83106ba36a400f327a275fd858c78556e3d6750e3595a0910`；5d/s62三个优化级别均同host cc。原C99清单54/57、wrong0、refused3、rc1，未降57基线；剩余自动VLA与两类复合字面量。本片只改模型生成与固定测试集，未改产品源/出货.com，无推送；全重构仍未完成。

### 产品边界越界复核（进行中）

cc报告switch超过256项静默误编译、-Wall的格式解码超过4096字节栈越界。暂停复合字面量扩展，先用私有临时目录复现产品，再为case写入及所有窄字符串decode调用补容量契约；超限明确诊断，不能继续写。同时检查switch嵌套表边界。参考与token dumper仍用私有路径，污染的共享/tmp/ua_ref不参与。

产品边界修复验收：decode 所有窄字符调用显式传入目标sizeof，逐字节写入前检查；switch case表256项与嵌套16层先检查再写。新增parserbounds普通/.com门禁：256/257/1000项switch，4096/4097/9000字节-Wall格式串，4096/4097字节字符串初始化；边界内通过，超限rc1诊断，8项各通过。我的旧产品复现：9000字节-Wall以信号退出；1000case样例出现错误诊断，未重复声称已复现cc的同一错误值。容量未扩大，宽字符串解码及其他报告缺陷未纳入此结论。

本轮冻结树私有UA `/tmp/unisacc-bounds-private` SHA256 e0c574ddb56036d3ffca04cfc364c612ab07610f46a5c4170e514c994e503cc1。完整门禁：双槽 `/tmp/unisacc-bounds-full`115/115，加独占 `/tmp/unisacc-bounds-core`1/1（48.31s）；程序核对请求与结果并集恰好116项、各一次、全rc0，UA哈希未变。每窗口≤55s。产品C9957/57、difftest_o396/396、fat133、chain120；新.com 1,356,272 B，SHA256 38c9ff7782c4607f91ace663fc43693582ca6c104ef7e011c227a1a45cc19c4c。定向首轮com-parserbounds因Python直接exec APE失败，已修为MZ走sh，并在新队列重验；旧失败保留。未发布、未推送。

后续独立审查：cc提供pf_dryrun的-Wall标签编号漂移复现，指出nlab/frameoff/framemax/curcall未回滚。当前仅确认报告，下一片需核实完整状态副作用，不能以只保存四项就宣称无遗漏；重复case诊断、系统调用隔离分支仍未合入。

### 格式警告单次解析（进行中）

审查pf_dryrun：表达式试走可修改标签、帧、类型/符号注册等状态，补四项回滚不足以覆盖。改为实际参数解析后读取格式中的对应转换并检查已有类型，不再为警告调用expr；普通printf与未声明fallback两条路径均接入。模型警告路径同步删除试走，保留既有诊断分类。用-Wall前后tape一致、嵌套调用/临时对象/匿名类型及warning对照验证，独立记录诊断顺序变化。

格式警告收尾：C删除pf_dryrun，pf_argcheck仅解码/扫描格式，实际参数expr后检查已有类型；fallback也在真实解析处检查。模型WF.entry只准备格式，WF.argcheck读取已有vt/vb，无EXPR试走、OCUT或池游标回滚；通用执行器未改。原警告分类保留，嵌套警告按真实解析顺序输出，不再重复试走。代码量不是减少：front_parse +23/-22；formatwarnings +29/-24，gen2 +17/-3。

新增formatonce（普通与.com）：标签分支、结构体临时量、复合字面量+匿名struct、嵌套printf、sizeof内enum，五类×O0/O1/O2，-Wall前后tape完全相同，实际输出同host cc。旧f004934私有参考运行该测试以1失败（labels/O0/-Wall changed tape），证明回归能抓住旧缺陷。最初argc用例因参考-run与原生argc不同而失败，已改volatile输入，不把它算产品缺陷。模型格式用例36→39，全部tape与完整诊断一致；所有警告检查额外要求quiet与-Wall tape相同。

冻结完整门禁：`/tmp/unisacc-format-full`117项双槽，加`/tmp/unisacc-format-core`独占1项47.87s，核对并集恰好118项各一次、全rc0。各窗口≤55s，最大单项51.15s。C9957/57、difftest_o396/396、fat133、chain120；私有UA SHA256 313bfba3b8f099ebc7765a67a9aa389153473cf38af857ecd39ae9702922e8b9。新.com 1,356,064 B，SHA256 fe72c95fa7392f69ff4dc9479f77960b8dd9ecb46292e3a6a4b99005abfee380。无推送/发布/默认模型切换。模型C99剩余VLA与复合字面量仍未闭合；继续迁移，不以本次警告修复作为重构完成。

### 模型复合字面量接入（进行中）

先接入函数内复合字面量，类型与空间取现有ELSZ/TYPECOUNT，匿名对象复用INITLIST的显式元数据入口；未知界数组复用INITCOUNT。结构体声明处参考会直接初始化目标，需复用同一初始化遍历，不能额外生成临时对象后复制。文件作用域与多维类型先保持明确拒绝，后续继续，不能据两项C99用例通过称完整语义完成。

复合字面量接入时发现参考缺陷（私有参考12be67c，非共享/tmp/ua_ref）：`(double){3.0}==3.0`，host cc退出1、参考退出0；`*(int *){&x}`及`*((int *[]){&x})[0]`（x=3），host cc退出3、参考SIGSEGV。参考initaggr的宽度/浮点初始化元数据及返回类型需整体修复，不能复制错误输出。此片先在模型明确拒绝指针元素与浮点元素的复合字面量并保存探针；这两类仍是重构待办，不声称复合字面量已完整。

本片实测：旧E3固定267项先通过，再加入C99的23/24与s63/s64，固定清单271；chain由120增124，124/124网络推理转储相同。匿名对象复用INITVALUE（INITLIST仅负责从符号取元数据）；声明中的结构体复合字面量用只读token识别进入同一遍历，不试解析表达式。s63/s64覆盖显式/推断长度、零补齐、设计符、嵌套标量与结构体、结构体数组、不同对象、递增只求值一次。原有地址获取、标量字面量赋值/后缀修改、文件作用域、多维匿名类型仍未宣称覆盖。
实际模型候选`/tmp/unisacc-compound-candidate/unisacc-next.com`：5,902,565 B，SHA256 441226e99997d39ba5ed3d23d976a62bc28abf1f5757434d9e63c07c2e31bac7；四个新增程序在O0/O1/O2的运行输出与host cc相同。C99为56/57、wrong0、refused1（19_vla），套件按原57基线仍退出1，未降低门槛。双槽队列`/tmp/unisacc-compound-gate`四项全绿，窗口14.31s：chain12.95s、自身3,950,564 B转储3.38s、parseloc7.22s、unitlocations3.49s。plain delta4964状态、1,276,743项；gen2 +46/-4，初始化模块+2/-1，规则增加而非代码减少，无新执行器原语。产品源码与出货.com未改，未推送、未切默认路线。下一项修复指针/浮点复合字面量参考语义，再推进剩余VLA与未覆盖形式；重构目标保持未完成。

### 复合字面量类型修复（进行中）
根因核对：cplitexpr把指针基类型的cw作为存储宽度，未设置initflt，且初始化表达式会覆盖declptr等全局类型状态；返回值又未完整建立curpd/curbase/curuns/curflt。修复按声明类型保存元数据，区分对象存储宽度与指针指向类型；初始化前明确给出转换类型，结束后重建结果描述，不按最后一个初始化表达式猜类型。保留聚合遍历与后缀逻辑。

本轮修复实测：tests/c/b_compoundkind.c覆盖指针/指针数组/二级指针、结构体指针、double/float、unsigned char、_Bool、double数组、嵌套结构体；host cc、私有C参考及重建.com的O0/O1/O2、Python前端均输出`3 3 3 7 / 1 1 1 1 1 4`。另九个小探针（含标量赋值/后缀、sizeof、结构体数组）在C参考三档优化与cc退出值一致。Python前端已正确，未改。模型解除此前指针/浮点字面量拒绝，三例转储一致；旧271项先通过后固定E3扩274、网络chain扩127且127/127通过。综合b_compoundkind模型仍在double operand处明确未覆盖，不算全模型通过。
完整冻结门禁：`/tmp/unisacc-cpkind-full`117项双槽与`/tmp/unisacc-cpkind-core`独占1项48.42s，并集对`gate.sh --list --com`精确118项各一次、全rc0，最大51.302s。两项曾因窗口剩余预算不足延期，后在新窗口通过，不把中止记pass。产品C9957/57，difftest_o399/399，fat134，模型自身转储3,954,383 B一致。私有UA `/tmp/unisacc-cpkind-private` SHA256 a029f11947b802dbb8eebeecbc83e7340f22205e790e39eb7075c01513acc99f；新.com 1,357,120 B，SHA256 c7e8543851c5a7258b7cb8c420c38cc5f817dec053838cdb9cf51841d14baa2f。未推送、未发布、未切默认模型；重构继续，VLA及其余模型边界仍待完成。

### 局部VLA模型接入（进行中）
复用EXPR计算运行时长度、ELSZ取元素宽度、现有指针访问路径；增加每符号字节数槽与每作用域栈快照，普通出块及break/continue按参考恢复。sizeof名字读取保存的字节数，不重新求值长度。常量/动态边界先按参考isconstdim的token判别（不试解析后回滚）。scope undo记录由15扩16并同步unused warning步长，新增元数据不塞进数组维度。无新执行器原语；多维VLA等参考未支持形态保持明确拒绝。

VLA元素类型审计：double/struct/enum边界转储一致，指针VLA发现参考symptrd漏加数组层，`int *a[n]; ... *a[0]`最终以8字节而非4字节加载int。与本次模型结果不同；修参考而不复制错误。VLA分支需在解析长度表达式前保存声明类型（长度表达式中的cast可能改写decl*），再以元素深度+1注册符号，并恢复基类型等元数据。

本轮进度（尚未提交，完整门禁待续）：实际模型候选`/tmp/unisacc-vla-candidate/unisacc-next.com`运行C99为57/57、wrong0/refused0；19_vla、b_vla、s65三档优化运行输出同cc。旧E3 274项先通过，加入三项VLA和b_vlakind后278项全部转储一致。模型九项队列`/tmp/unisacc-vla-gate`全绿35.08s（在VLA参考类型修复前），不能替代后续产品冻结验收。参考修复后b_vlakind在cc/C三档/Python以及新.com一致；旧330bc34私有参考运行同一回归退出139。产品已重建，完整队列`/tmp/unisacc-vla-full`使用UA=/tmp/unisacc-vlakind-private，首窗口4/117通过、退出75待续；独占exec-driver-core尚未跑。继续原队列，勿编辑输入或重用旧参考门禁结果。

VLA冻结验收完成：`/tmp/unisacc-vla-full`双槽117项与`/tmp/unisacc-vla-core`独占1项（48.93s），并集精确匹配`gate.sh --list --com`的118项，各一次、全部rc0，最长52.944s，无失败或跳过。四个新增VLA用例（19_vla、b_vla、b_vlakind、s65）实际模型编译器的-Wall退出码、转储与诊断全部同私有参考。chain固定131项通过，E3固定278项通过；模型自身转储3,956,608 B相同。产品.com为1,358,048 B、SHA256 a56523e877d215a7d5ccdfe5bc21e6d793c4d0174de1acde3b711aed9c14f564；私有UA SHA256 757002812b7840aee7335296c4cd7a30bcb18247f64d3cbad8a754b3d3296021。实际模型候选5,913,317 B、SHA256 387cb14e83dbb5da62bb650e8bf9d1f11dbf90ea00498256a7377101ee889ee7。C99的57个探针全过不等于完整C99或重构完成；多维VLA、其余模型未覆盖形式仍单列，未切默认路线、未发布或推送。

### 浮点二元公共类型迁移（进行中）
下一片移除OPX对float的整类拒绝，并替换double路径里写死的signed cvtid：按type.tsv的ck/res行选择公共浮点类型与合法性，复用TO.d/TO.s转换，irsel供给操作码与反向比较标记。指针与整数路径保持回归；新增f32/f64、混合整数与u64的独立运行证据。赋值/复合赋值等未迁移分支不在此片偷改，执行器不增加语言原语。

浮点二元片实测：type.tsv的ck/res选择f32/f64公共类型与合法性，TO.d/TO.s共享转换，irsel提供运算码及_rev。移除了NOFLT拒绝与写死signed cvtid、手写64位opcode清单；gen2净+2行（+25/-23），无执行器改动。发现并修复旧模型接受错误：u=9223372036854775808UL与double 2.25的u>d、d<u，上一候选输出0 0，本片及host cc输出1 1；s66_float_binary已固定覆盖。20种有浮点操作数的类型配对×10运算转储同私有参考；20程序加s66在实际网络候选O0/O1/O2共63次输出同host cc。旧278项先过，再加s66，E3固定279；网络chain固定132/132。
定向双槽队列`/tmp/unisacc-fpbin-gate`12/12全rc0，两窗口51.95/10.63s，最长43.012s：source-to-ELF、chain、自身转储、位置/错误/警告检查；自身3,956,608 B相同，3.46s。实际候选C9957/57、wrong0/refused0。plain delta5085状态、1,307,897项、28,371,917 B；候选.com 5,944,703 B，SHA256 568518181e26c058c7d27790b97989b5ec53de25c53a7707893cbf991ddde2e3。产品源码和出货.com未改，本轮不是新的118项全门禁。综合b_float/b_compoundkind仍有初始化等double operand拒绝，b_fconv仍有expression拒绝，继续迁移；未推送/发布/切默认。

### 初始化/赋值共用转换（进行中）
把SAMEDBL的局部double特判替换为ASSIGNCV：目标类型选择已有TO.d/s/i/u或BOOLCV，源描述与目标描述分别保留；局部标量初始化、聚合元素、全局值与普通赋值复用同一入口。不改执行器；逐项对照参考转储和host cc运行，旧固定清单先验收再扩充。

参考缺陷实证（私有UA为9da574d产品状态）：unsigned long数组、结构体成员、复合字面量由9223372036854779904.0初始化，cc输出1 1 1，参考输出0 0 0。initflt/slotflt只携带浮点与bool，漏掉u64槽位的转换kind。统一从宽度/指针/unsigned/float/bool生成kind，覆盖成员、数组及匿名对象；不把参考错误复制进模型。需产品回归、重建.com与完整冻结门禁。

初始化/赋值片完成验收：ASSIGNCV供局部标量、聚合槽、静态标量、全局值与普通赋值共用TO转换；删除ISDV及静态f32拒绝等重复分支，模型相关源码净-18行。产品valuekind供dkind和数组/成员/复合字面量槽共用，修复unsigned long大浮点初始化；新增b_aggrunsigned的9个宽无符号比较全为1，控制输出3 4 5 1，cc/C三档/Python/新.com一致。s67另覆盖f32/f64/int/u64/bool的声明、普通赋值、聚合与静态初始化；三个新增用例在实际模型三档优化9次输出同cc。旧279项先过，再纳入三项固定E3=282，网络chain=135全过。
冻结门禁：`/tmp/unisacc-assigncv-full`117项双槽+`/tmp/unisacc-assigncv-core`独占1项49.95s，精确并集118项各一次、全rc0，最大52.62s。difftest-2因窗口余6s延期，完整预算15.87s通过，延期不记pass。产品与实际模型的C9957/57，wrong0/refused0；difftest_o405，fat136；自身转储3,958,707 B一致。产品.com 1,358,336 B，SHA256 38f13433a6ab2bd3f3ebb2915bb527e657d34f1cc4fee66f05d2ae976e1a1e3d；私有UA SHA256 1fc4fdc26a4c881b44b38cb5e028983e33c9d86c25104c1b292d201fef4ff4b1。模型候选5,942,919 B，SHA256 552604d99d6baf6a3017a5aa6bc57732f6d812facbdeff2847ae6870460994e0；plain delta5054状态、28,200,821 B。未推送/发布/切默认。
后续返回转换缺口已实测：`double f(void){return 3;}`再打印(int)f()，私有产品输出3、当前模型候选输出0。S.re1只改目标描述并NARROW，没有转换返回表达式；这是既有模型缺陷，不是已修复的初始化问题。下一片让返回复用同一转换入口并保存回归，不能以本轮固定清单全绿声称全域正确。系统调用/procview隔离分支继续未合并。

### 返回值转换复用（进行中）
返回标量先保存表达式来源类型，再以声明返回类型调用ASSIGNCV；只有整数窄化继续NARROW，float/double/bool和指针不重复窄化。沿用结构体返回拷贝。不改产品源码，先验证既有282项，再以真实网络运行检查混合返回类型。按主人要求后续不再向cc-unisacc发协作消息，自主推进。

返回转换片验收完成：复用ASSIGNCV，删除返回路径的局部float/bool特判，gen2净-5行（+5/-10），无执行器或产品源码改动。s68覆盖int→double、double→float/int/u64/uchar/bool、u64→float/double、float→short。最小double f(){return 3;}旧模型输出0，新模型与cc输出3；两程序三档优化共6次真实模型运行同cc。旧282项先通过，再纳入s68，固定E3=283；网络chain=136/136。
定向双槽队列`/tmp/unisacc-returncv-gate`12/12全rc0，窗口50.97/12.97s，最长43.423s；自身3,958,707 B转储相同。实际模型C9957/57、wrong0/refused0。plain delta5045状态、1,297,615项、28,151,060 B；候选`/tmp/unisacc-returncv-candidate/unisacc-next.com`5,942,411 B，SHA256 01a562a374cfd8c49cafabe718cc647d39ab7a1920b892d3d7b18e546327164b。出货.com仍为ef17e27的产品构建，本片未重跑118项产品门禁；未发布、推送或切换默认路线，剩余转换与覆盖缺口继续推进。

### 十进制u64字面量边界（进行中）
复测b_fconv仍拒绝expression；逐句缩小至首行18446744073709551615UL。共享token读取器NUMD只接受19位，U.ulong已有正确的有符号位模式输出。扩至20位前逐位检查UINT64_MAX，不能只放宽长度令累加回绕；复用现有64位比较动作，不增执行器原语。浮点先由floatconst分类，仍走原精确转换。

十进制u64片实测：NUMD每次乘10前用UINT64_MAX与当前digit计算阈值，C64U拒绝溢出，长度放宽至20位；U.ulong原有NUMOUT打印位模式复用。旧283项先过，再固定b_fconv与s69，E3=285全同，网络chain=138全同。此前b_fconv拒绝的首因就是20位字面量；修复后完整转储同参考，两新增程序实际模型O0/O1/O2六次运行输出同cc。共享数值门禁增加6个整数精确位值、3个溢出token拒绝，table/net两路径都检查tk，避免只看回绕后的nv；原61个浮点位值及10个坏浮点输入保持通过。
双槽定向队列`/tmp/unisacc-u64literal-gate`6/6全rc0，窗口42.90s；source-to-ELF42.78s，自身3,958,707 B一致。plain delta5065状态、1,302,755项、28,248,181 B；候选5,944,748 B，SHA256 cce79501fbb67e55f78760828b5e5b9b62fb4c23ceda51e3df79b90f9715b285。模型共享reader净+4行；产品源码与出货.com未改，不宣称本片重跑完整产品门禁。b_float仍在混合?:类型处未覆盖，下一片审计公共类型及两分支转换；未推送/发布/切默认，整体重构未完成。

### 条件表达式公共浮点类型（进行中）
参考cond在两臂浮点kind不同后回到首臂重发，分别fconv到type.tsv公共类型；模型目前QT.int把f64结果送入仅整数的RESD而拒绝。采用相同的token/output重发边界，保留参考标签分配顺序，复用TO.d/TO.s；两臂运行时仍只执行一臂。嵌套与带副作用表达式需独立检查，不仅比较简单字节。

条件表达式浮点片验收：按type.tsv公共类型重发两臂并复用TO.d/s，结束明确设置公共结果描述；首次探针发现漏设结果类型导致多发cvtid，已修。s70覆盖double/int、float/int、float/double、u64/double、嵌套与函数副作用；实际网络编译器O0/O1/O2均同host cc，计数证明每次只执行选中的一臂。旧285项先过，再固定s70，E3=286全同、chain=139全同。保留参考的重发与标签分配顺序，不宣称消除了手写条件语法规则；gen2净+12行，无执行器原语新增。
双槽队列`/tmp/unisacc-qt-gate`12/12全rc0，窗口49.85/12.83s，最长42.42s；自身3,958,707 B转储相同。plain delta5105状态、1,313,057项、28,464,304 B；实际候选5,950,546 B，SHA256 f1d0a4c96a770e912c189e0aeaee1ee99cfbe05ea0aef5c8e3e4cd6b02409395。b_float已越过条件表达式，仍因浮点复合赋值/递增未覆盖而拒绝；下一片复用OPX与ASSIGNCV处理该边界。产品源码及出货.com未改，本片为定向模型门禁，不称完整产品验收；重构继续，未推送/发布/切默认。

### 浮点复合赋值复用（进行中）
LV.c先允许标量float/double的单位步长，指针仍由STEPTY决定。浮点任一操作数经TAX识别后，保存赋值目标、调用现有OPX处理公共类型与运算，再ASSIGNCV回目标并STOREV；移除仅服务bool目标的重复浮点opcode/转换序列。整数与指针原分支先保持回归，前后缀递增另验，不顺带声称完成。

浮点复合赋值验收：任一浮点操作数经TAX分派后，复用OPX公共类型/irsel运算和ASSIGNCV回目标；删除bool专用浮点opcode及signed转换序列。数组元素和成员复合赋值接到既有LV.c，地址不重复求值。gen2净-9行（+11/-20），状态5105→5072，无新执行器原语。s71覆盖double/float/int/u64/bool、四则复合赋值、成员、n++下标与RHS调用；实际模型三档输出均同cc：8 12 1 1 1 6 1 1 / 6，下标与调用各一次。旧286项先通过，新固定E3=287全同，chain=140全同。
双槽队列`/tmp/unisacc-fpcas-gate`12/12全rc0，窗口49.52/12.82s，最长42.07s；自身3,958,707 B转储一致。plain delta5072状态、1,304,610项、28,209,028 B；候选5,945,596 B，SHA256 6cdf379776cbe3ab89502a7cc1bf922bea85071b221d7d1a9a4a5629113d520c。b_float仍在浮点递增处未覆盖，未称完整通过。产品源码和出货.com未改，定向模型门禁不等于完整产品门禁；下一片处理前后缀浮点更新，重构继续，未推送/发布/切默认。

### 浮点前后缀更新（进行中）
前后缀复用CSTEP处理目标类别，新增共享浮点一步更新（IEEE 1.0位模式，irsel提供add/sub编码）。后缀保存并返回旧位模式，不能以反向运算恢复，因为舍入不可逆；地址只求值一次。前缀返回新值；整数/bool/指针继续既有路径。

浮点前后缀片验收：CSTEP复用目标分类；FPSTEP共享IEEE 1.0位模式与irsel加减opcode，前缀返回新值，后缀保存原位模式，不做逆运算恢复。s72覆盖float/double、前后缀加减、2^24/2^53加一舍入、数组n++和成员；s72与完整b_float在实际网络编译器O0/O1/O2共六次输出同cc。旧287项先过，新固定E3=289全同，网络chain=142全同。gen2净+9行（+17/-8），无新执行器原语。
双槽队列`/tmp/unisacc-fpstep-gate`12/12全rc0，窗口50.14/12.87s，最长42.59s；自身3,958,707 B转储一致。plain delta5104状态、1,312,844项、28,403,652 B；实际候选5,953,878 B，SHA256 07ee7ecdeb6d3b08e38dd5905c636388e07a723e887ed5d798b82bb5488c409f。产品源码和出货.com未改；本片仍是定向模型验证，不宣称新的跨平台运行或全产品门禁。浮点综合用例闭合后重新盘点S-17的剩余覆盖、CLI/错误契约、体积与默认切换阻挡，不以固定清单全绿替代重构完成。未推送/发布/切默认。

### 覆盖盘点与真实应用阻挡（进行中）
5c875eb候选在tests/c、tests/c99、examples及apps、parse2/probes共296份源文件上，以私有参考和双槽12秒子进程界限盘点：273同转储、22未覆盖、1参考拒绝、0 DIFF/工具失败。此清单不同于固定289，不能直接比较分母。结果存/tmp/unisacc-frontier-5c875eb/results.json。两个真实应用exeinfo/wordfreq在带位置的实际网络编译器中均定位到`(unsigned char)*p++`；UD/U标识符路径未接后缀更新。新增共享命名左值更新入口，复用LOOKUP/地址/CSTEP/POST，不另写更新算法。

新产品缺陷实证：struct P{char x;};struct P *v[4];long guard=77，写v[1]=&obj后旧私有参考输出guard地址值而非77、退出1。全局数组.bss分配仅看gstruct而不看gpd，1字节结构体使数组仅4B，指针实际需32B；24字节结构体反而多分配。sizeof已正确，错误在存储分配。修正isarr优先使用元素w（pointer已设8），仅非指针结构体用declsz。不是为字节对齐复制错误；需新的私有参考与完整产品门禁。

本片冻结前实测（尚未提交）：命名后缀更新复用POST后，wordfreq/exeinfo及b_globalstructptr三份转储均同新私有参考；旧289项先通过，清单扩E3=292/chain=145待完整门禁。三份程序在实际模型O0/O1/O2九次输出同cc，Python及重建产品O2的b_globalstructptr输出77 32 24 1。产品.com 1,358,320 B，SHA256 b0db8800a56a7c13c4e952eb02266b7e72d2fe7b3bf14efbd8da4ed11de4c9bb；私有UA=/tmp/unisacc-postoperand-private，SHA256 b20f125ac70101d966d6e5e83fc21e030b0e9d5de9ffbb93bca83b1294958e44。实际模型/tmp/unisacc-postoperand-candidate/unisacc-next.com为5,958,288 B。完整冻结队列使用/tmp/unisacc-postoperand-full，exec-driver-core另独占；所有步骤完成前不提交、不编辑冻结输入，不把局部九次运行称全门禁。

后缀操作数/全局结构体指针数组片冻结验收完成：`/tmp/unisacc-postoperand-full`117项全部执行，其中exec-multi-ua并发53.07s超时（142，保留记录）；`/tmp/unisacc-postoperand-multi`独占48.69s通过；`/tmp/unisacc-postoperand-core`独占50.68s通过。按gate.sh --list --com核对，使用明确记录的独占复跑结果替代超时判定后，并集精确118个不同套件全rc0，最长52.942s；不将原超时改写为pass。C9957/57，difftest_o408，fat137双架构实际运行；自身转储3,958,904 B一致；E3固定292全同、网络chain145全同。模型候选SHA256 5057c049b5303942164c79a8f66b8a9ed55a7e460d29c95a710c4a899ae7ec8e。gen2净+4行，复用既有后缀更新，无新执行器原语；产品只修正全局数组分配条件并重新生成unisacc.c。当前只据本地门禁范围报告，未新跑Linux/Windows VM原生套件。未推送/发布/切默认，剩余模型覆盖与整体切换继续。

### sizeof成员对象复用（进行中）
覆盖盘点中b_arr1memb/b_structarrmember停在sizeof成员数组：普通表达式路径已衰减成指针，无法再由vt/vb恢复完整对象宽度。提取MB.INFO共用成员声明元数据读取，sizeof直接沿命名成员链取得MSZ（不执行地址/载入），普通成员访问仍用同一读取入口；未匹配的表达式退回原路径，不把残留marr当通用类型事实。


sizeof成员片验收：复用MB.INFO读取成员声明元数据，新增命名成员链的sizeof路径；数组成员保留MSZ对象宽度，后续箭头访问仍按数组衰减处理。未增加执行器原语，gen2净增19行。原292项先通过，新增b_arr1memb、b_structarrmember和s73_sizeof_members后固定295项全部逐字节相同；chain固定148项全部通过。实际模型候选在三个用例、三个优化级别上共9次执行与宿主cc相同。相关12项门禁两槽排队，两个窗口50.67/12.97秒，全通过；源码到ELF43.13秒，E3自身源码3,958,904字节相同。候选5,961,195 B，sha256 f4d1a1639b4ca9d5b1d6ff24bac520463a28abb2c5318162e92cbdb73f73ddb1；仍在隔离目录，不替换出货产物。模型5171状态、1,330,083条目、JSON 28,797,582 B。此片为模型覆盖补齐，不宣称完整C99或S-17完成，产品源码未变，未重复整套产品门禁。


### 整数类型拼写收尾（进行中）
b_short仍停在sizeof(long int)：TSPEC已有long/long long和unsigned short的描述符，但未消费可选int。把这几条尾部接入共用的可选int消费状态；保持类型宽度来源与描述符不变。新增声明、参数、转换和sizeof探针，先检查参考字节与真实执行，再扩大固定清单。

整数类型拼写验收：共用TS.intopt/TS.intend消费short/long/long long尾部可选int，unsigned short/long沿同一路径；类型描述符不变，未增加执行器原语，gen2净增2行。原295项先通过，加入b_short与s74_int_spellings后297项全部参考tape相同，chain150项全部通过。两例在实际模型候选-O0/-O1/-O2共6次执行与宿主cc相同。12项相关门禁两槽队列50.37/13.11秒，全通过，源码到ELF42.40秒。模型5175状态、1,331,112条目、JSON28,822,967 B；候选5,961,557 B，sha256 4353fd82c49d224b70dce8da0460efc07833b81eefc6ca334d81370b038dadc9。产品源码与默认.com未变，未宣称任意声明拼写或完整C99已覆盖。


### unsigned一元负号覆盖（进行中）
剩余b_uzext停在unsigned int一元负号。参考在减法后把u32零扩展，模型此前直接拒绝；复用tyinfo派生的NARU掩码，不复制第二套宽度常量。先核对该文件与固定清单，再由实际模型候选执行。

unsigned负号验收：gen2净增1行，以已有NARU生成u32减法后的零扩展，无新执行器原语。b_uzext全文件与参考tape相同，原297项先通过，加入后298项全通过；chain151项全通过。实际模型候选-O0/-O1/-O2均与宿主cc相同。12项相关门禁两槽队列50.73/12.97秒，全通过，源码到ELF42.65秒。模型5177状态、1,331,627条目、JSON28,832,947 B；候选5,962,191 B，sha256 adc7e716a34776e4ab031726ff8fe9b929d3e3bf8c688e59c1b5d24cb9878d0d。产品源码未变，默认.com未切换。


### 函数指针typedef复用（进行中）
覆盖盘点中的b_declfn/b_anon在函数指针typedef处停止。复用已有FPDECL和类型别名表登记路径，不另写函数指针声明解析；数组别名仍需维度元数据，不在此处误当单指针接收。先跑真实文件确认后续阻挡，再扩固定集。

本片首轮对拍发现b_declfn被接收却索引步长51而非8：FPDECL解析数组参数后FN.pfp未执行数组到指针调整。补齐该描述符层数后全文相同；这是模型缺陷，未照搬错误输出。普通别名和函数指针别名共用TD.put，不重复登记逻辑。原298项已先通过。

函数指针typedef验收：gen2净增6行，复用FPDECL及TD.put，数组参数补一层指针；未增加执行器原语。b_declfn加入固定集后299项参考tape相同，chain152项全通过。实际模型候选-O0/-O1/-O2均与宿主cc一致；12项相关门禁两槽队列50.82/13.14秒，全通过，源码到ELF42.80秒。模型5187状态、1,334,198条目、JSON28,891,184 B；候选5,964,832 B，sha256 46da5c34e1985e34d291ccafe6f62cc817c42c4eec660970d83e2fdde3b3d1b0。b_anon后续停在匿名成员，b_typedef仍停在块内别名表达式，未扩大完成口径；产品源码与默认.com未变。


### 当前覆盖盘点与enum声明（进行中）
当前c9c7594模型对tests/c、tests/c99、examples及apps、parse2/probes共299个文件：283相同、15未覆盖、1参考拒绝、0DIFF/工具失败。此集合不含旧parse/probes，不能和固定299项混为同一计数。结果保存/tmp/unisacc-frontier-c9c7594/results.json。b_init3的全局enum对象被顶层EN误当枚举定义；区分定义与类型使用后转回共用FN/TSPEC，不重写对象初始化路径。

enum对象片验收：区分顶层enum定义和类型使用，后者回到共用FN/TSPEC，gen2净增2行，无新执行器原语。原299项先通过，新增b_init3后固定300项参考tape相同，chain153项全通过。实际模型候选-O0/-O1/-O2与宿主cc一致；12项相关门禁两槽队列50.50/13.00秒，全通过，源码到ELF42.27秒。模型5189状态、1,334,713条目、JSON28,901,904 B；候选5,965,116 B，sha256 b87abd8863e96e387d49da3317e4ca1aadfd50ad3e9f6fcc58865a477adc3481。产品源码与默认.com未变；匿名成员、块内typedef等缺口继续保留。


### 结构标签作用域缺陷（进行中）
当前模型接受内层同名struct定义，却复用外层sid并覆盖SSZ：最小例外层int成员、内层long成员，内层结束后sizeof外层对象参考4、模型8。新增标签绑定撤销栈和作用域编号，结构ID改为单调分配；块退出恢复名字绑定但不改写已分配结构描述。与对象BIND分开，避免标签和普通名字混用命名空间。

结构标签片验收：标签有独立绑定撤销栈，函数体和嵌套块退出恢复STAG/TAGLEVEL；sidserial单调分配，nsid只表示当前结构，避免内层覆盖外层尺寸。支持块内单独结构声明。复用现有64槽成员键布局，63个结构接受、第64个明确structure id capacity拒绝（实际模型运行），不让ID溢入相邻名字。gen2净增13行，未新增执行器原语。原300项先通过，加入b_init4及s75_tag_scope后302项参考tape相同，chain155项全通过；新探针覆盖两层同名标签、块退出后新对象声明、函数间恢复。两例三个优化级别共6次实际模型运行与cc相同。12项相关门禁两槽队列49.80/15.13秒全通过，源码到ELF44.36秒。模型5209状态、1,339,861条目、JSON29,020,759 B；候选5,968,613 B，sha256 8b7b018a29bce35cbfdb58ffc6ad53e7a353dbf0e507b9cbf3b4f30c0fb7e0b3。未宣称完整标签作用域（原型作用域、内层纯前置声明等仍需核对），产品源码及默认.com不变。


### 结构标签容量与参考边界复核（29c727f之后）
实际独立复核两项，不把固定集全绿当完成：
- s76_struct128.c：128个不同结构、访问最后一个成员，宿主cc和私有产品参考均rc0，实际模型候选在第64个明确拒绝。原64槽成员键stride只容纳1..63，这是模型容量缺口；后续应统一成员键编码和全部读写/初始化器，再对齐产品MAXSTRUCT=128，不能只提高TAG.alloc上限。
- s76x_forward_scope.c：外层完整struct S、内层`struct S;`后`sizeof(*p)`。宿主cc以不完整类型诊断拒绝，私有产品参考和实际模型都rc0。src/front_parse.c的stparse只在定义带{时建立深层标签，没有处理内层纯前置声明遮蔽；模型忠实复制了这项错误。属于产品和模型共同的拒绝契约缺口，需同步修复并按产品改动重建/整套门禁，不计为已接受合法输入的相等证据。
两份探针已保留在exec/parse2/probes，不加入equal清单。命令均有15秒子进程超时；没有修改产品、模型或默认.com。下一步先处理成员键容量，再统一前置标签规则；不能称标签作用域已完整闭合。


### 成员键容量对齐（进行中）
统一STRUCT_MAX=128、MEMBER_STRIDE=256，覆盖模型ID1..128；写成员、普通读取和指定初始化器三处共用该步长。成员映射迁到独立1<<40间隔区域，避免扩大步长后越过旧的百万格区域；最大名字ID受输入长度域限制，成员键上界仍小于区域宽度。结构内成员序号表的64步长不是同一维度，本片保持原状。

成员键容量验收：三个成员键构造点统一MEMBER_STRIDE=256，并用A64I/A64算地址；6张成员映射各占独立1<<40区域，128结构上限与产品一致。模型状态/条目数不增（5209/1,339,861），JSON29,020,849 B。原302项先通过，扩展s76覆盖64/65/128号结构的本地及全局指定初始化、读取，加入后303项全相同，chain156项全通过。实际模型3例×3优化级别共9次与cc相同，第129个结构rc1且明确容量诊断。最终64位键版本12项相关门禁两槽队列49.63/15.83秒全通过；较早32位键版本的未完成队列不复用、不作为最终证据。候选5,968,805 B，sha256 fcb6375c77b596ee4834391ff556d02be695d39041ebc730658a401a63caeeb7。产品与默认.com未变；下一项为内层纯前置标签的产品/模型共同修复。


### 内层前置标签同步修复（进行中）
C stparse和Python tag_bind需要把标签后直接分号视为当前作用域声明；普通struct S *p仍向外查找。sizeof不完整结构在类型形式和表达式形式均拒绝；模型通过同一TAG.bind/撤销栈建立内层未完成类型。先做针对性验证，然后重建.com并跑产品门禁。

内层前置标签验收：C stparse对更深块的纯标签声明新建类型，Python tag_bind区分普通向外引用与当前作用域绑定，模型沿TAG.bind建立未完成类型并恢复；sizeof结构类型、表达式、常量表达式的不完整类型均拒绝。tests/tagforward.sh进入参考和.com门禁，三种非法形式×cc/Python/C三优化级别共15项编译拒绝（不运行非法输入）；同套件在实际模型候选也通过。b_tagforward合法完成定义与外层恢复，连同s75共6次实际模型运行与cc相同。原303项先通过，新增后固定304项全相同，chain157项全通过。
完整冻结门禁：/tmp/unisacc-forward-full执行119项，exec-multi-ua首次生成errorparse时ENOSPC失败，原记录保留；清理12个本会话过期候选目录的可重建中间文件（保留.com与日志）释放约2.1GB。该项在/tmp/unisacc-forward-multi独占重跑49.30秒通过；exec-driver-core在/tmp/unisacc-forward-core独占51.36秒通过。三份结果合并后严格核对gate --list --com恰好120项全有通过证据，最长51.629秒。普通队列保持两槽、逐窗口不超过55秒；不将原失败队列称为全绿。C99 57/57，difftest_o 411，fat 138双架构实跑，kernel23 stale0，nativeboot通过，自身E3 3,961,598字节相同。没有据此宣称新做了Linux/Windows虚拟机全量测试。
重建产品unisacc.com 1,359,312 B，sha256 42144a233904acd9a66a994344fd1507bd8aa21ee347d75c41c9b9e4233980ec；私有参考sha256 5accbaa49bd418ac5abe03df2fbbb52dc3973bfe2dec691de47282697ae20ca7。实际模型候选5,969,125 B，sha256 4993d8ae1a8820906572e5fd214ac2ab116391c868434ddfdc0d2085985773d9；模型5213状态、1,340,889条目、JSON29,043,953 B。默认产品仍是参考实现，S-17覆盖缺口仍在，不发布、不宣称重构全部完成。


### 可变参数函数指针迁移（进行中）
参考vcall按声明的variadic标记或实参>6选择栈调用；普通间接调用恰好6实参因r5留给callee而拒绝。模型新增独立函数指针基类FPV保存栈约定，沿现有BASE/typedef/成员/作用域撤销传播；共用已有实参反序，不新增执行器原语。先核对b_varargs2及参数、typedef、遮蔽和嵌套调用，再保留固定集。

首轮发现两项需按实证处理：输出数字辅助过程会改写临时t，因此栈清理长度从na重新计算；参考function前瞻把显式嵌套函数指针参数中的...也计入外层stacked约定，模型按该参考行为记录，不能宣称这是C标准要求。

可变参数函数指针验收：FPV沿既有BASE、typedef与作用域撤销传播，共享ISFP分类和CL.vdone反序路径；普通间接调用>6实参也复用栈路径，恰好6实参以明确callee寄存器限制拒绝。gen2净增17行，未新增执行器原语；warning分类复用ISFP。原304项先通过，加入b_varargs2与s77_fpvar后固定306项全相同；chain159项全通过。s77覆盖别名、显式函数指针参数、块内同名普通指针后恢复、嵌套间接调用和8实参；实际构造网络候选两文件×3优化级别共6次与cc一致，s77x合法C六实参探针由参考和模型均rc1拒绝，保留为产品限制不计equal。
12项相关门禁两槽队列50.79/14.81秒全通过，源码到ELF43.18秒，自身E3 3,961,598 B相同。最终模型JSON29,241,601 B；模型候选5,973,434 B，sha256 8cfc9201ac88999fd24bdbaa1a8ba3252a8c168f90df4bfcf321241976a19a3d。默认产品源码和unisacc.com未变；没有重复全产品门禁或宣称S-17完成。参考对嵌套...的调用约定按实测兼容，不外推到任意函数签名。


### 块内typedef与enum绑定复用（进行中）
b_typedef需要块内类型别名、内嵌枚举定义和块退出恢复。把顶层TD改为可调用声明过程，复用BIND/UNWIND保存类型别名三列；普通对象声明同时遮蔽同名typedef。enum定义并入TSPEC共用路径，枚举常量沿普通名字撤销，枚举标签沿标签撤销；不为每种声明另造一套作用域机制。

块内typedef/enum验收：TD.parse成为顶层与块内共用过程；ENUM由TSPEC调用，删除原顶层专用枚举定义路径。BIND/UNWIND记录从16扩至19列，保存TDN/TDB/TDD，对象声明清除当前typedef可见标记；枚举常量也复用BIND，枚举标签复用扩为4列的TAG撤销记录。gen2净增7行，无新执行器原语。原306项先通过，加入b_typedef和s78_typedef_scope后308项全相同，chain161项全相同。s78覆盖三层类型宽度恢复、变量遮蔽别名、枚举常量/标签遮蔽恢复、块内函数指针typedef；两文件×3优化级别共6次实际构造网络运行与cc相同。
12项相关门禁两槽队列50.54/14.69秒全通过，源码到ELF43.15秒，自身E3 3,961,598 B相同；最终固定集复验与候选构建使用两个独立槽并行、子进程均有超时。模型5251状态、1,350,665条目、JSON29,264,672 B；候选5,973,960 B，sha256 d52314c71e497d004d90fcf433ed56068bfa0e74a3172d6da963187c7f54f5d4。释放两个旧候选可重建中间目录约360MB，保留.com与日志。产品源码、默认.com未变；未宣称任意typedef声明形式或全部C作用域已覆盖。


### 匿名聚合成员迁移（进行中）
参考把匿名结构/联合的成员元数据复制到外层，偏移加匿名对象起点；成员顺序与初始化跳过信息一并保留。模型复用SSZ/SAL、成员映射、SMEM与MFLAT，不另写初始化器。该语法是产品支持的C11扩展，不归为C99必需能力；本轮为已支持产品行为的迁移。

匿名成员验收：匿名结构/联合成员的名字与布局映射提升到外层，偏移加对齐后的起点，MFLAT保留初始化跳过信息；普通成员和提升成员共用SB.memberindex。gen2净增16行，未新增执行器原语。现有每结构64项SMEM布局增加写入前检查；实际模型64成员接受且三优化级别与cc一致，65成员rc1明确structure member capacity，不把这一模型上限称为产品充分上界。
原308项先通过，加入b_anon与s79_anon_layout后固定310项全相同；chain163项全相同。s79覆盖对齐、嵌套匿名成员、union别名、数组成员与全局/局部初始化；两正式文件加64成员边界，三优化级别共9次实际构造网络运行与cc一致。b_init5已越过布局，但块内函数原型仍未覆盖，未加入equal。12项相关门禁两槽队列49.99/15.02秒全通过，源码到ELF44.15秒，自身E3 3,961,598 B相同。
模型5272状态、1,356,064条目、JSON29,395,866 B；候选5,979,116 B，sha256 a93c4ff3bc87740f0d2ce4289ee57de4590d38841d5b6a6f5368474c5d27797a。清理上一片旧候选的可重建中间文件约180MB，保留.com及日志。产品源码与默认.com未变；匿名聚合属于已支持的C11扩展迁移，不修改C99覆盖口径。


### 块内函数原型迁移（进行中）
参考block声明在名字后遇到参数括号时只做平衡扫描，既不分配局部槽，也不登记参数签名；模型复用函数指针参数列表扫描，结束后继续逗号声明或分号。该行为不等于完整原型类型检查，尤其不能据此证明默认转换以外的调用正确性。

独立反例s80x_block_float：宿主cc输出3.5，4a84274私有产品参考输出0.5（均rc0）。C局部原型完全跳过签名，Python也跳过参数列表但登记返回类型；不能将模型对参考一致提升为C语义正确。保留探针、不加入equal清单，下一项优先同步修复原型签名及实参转换。组合探针中的函数地址要求被取地址的函数已定义；仅有块内声明、定义在后时模型仍未识别函数值，另记未覆盖，未声称已经迁移。

块内原型迁移验收（不含浮点签名缺陷）：PARAMS由函数指针声明与块内原型共用，普通声明遇到参数列表不分配槽，逗号继续原声明路径；EOF明确拒绝，实际候选未闭合括号rc1报unterminated parameter list。gen2净增3行，无新执行器原语。原310项先通过，加入b_init5与s80_block_proto后固定312项全相同，chain165项全相同；两文件×3优化级别共6次实际构造模型运行与cc一致。仅执行受影响7项门禁，两槽单窗口45.84秒全通过，源码到ELF43.52秒。
模型5278状态、1,357,608条目、JSON29,428,415 B；候选5,980,518 B，sha256 eed6c1f793d347ab401cdd02e1c50621da2152ec6685eea6fe05c1971b4d2306。模型候选在s80x浮点原型反例上也输出0.5，与参考共同偏离cc的3.5；明确登记为未修复，不计入通过证据，下一项同步修复。默认产品源码与.com未变；本提交只完成参考已有块内声明行为的迁移，不称完整原型语义完成。


### 块内原型签名修复（进行中）
修复真实浮点调用错误：C抽出函数定义已有的单参数声明解析，由块内原型共用，只登记函数返回与参数转换信息、不创建参数局部槽或输出指令；Python同样抽取现有参数列表解析。块内逗号声明继续时恢复外层声明描述符。随后模型同步使用签名，重建产品与完整冻结门禁，不以参考旧错误作为答案。


### S-17 设计校正（2026-09-27，用户指出模型设计漂移）
停止以新增equal用例数驱动语法补丁。核对当前源码：gen2.py的开头把目标写成已实现事实；research/e3-structured.md规划的独立grammar.txt/templates.tsv并未成为实际输入。现有路径包含可复用的构造器、运行时模型，以及部分真实表来源，但大量编译规则仍由生成器中的命名状态与动作手写，不能称声明式文法迁移已完成。
当前未提交的块内原型签名修复保留并收尾，不继续扩语法。已实测修复浮点反例与私有参考tape相同，原312项固定集全同；这不等于完整产品验收。之后按声明器、类型转换、绑定/作用域、控制流四个共用机制核对规则来源和组合边界，优先合并重复规则，不能只把分支搬进另一个文件或自创DSL便称模型化完成。运行时模型替代与规则来源简化分别验收，旧参考作为回退，不能偷偷承担模型未覆盖输入而宣称完成。
有限语法规则不等于有限程序集合；C的typedef消歧、类型与作用域需要显式属性/存储规则。撤回无条件LL(1)与“新构造只需一条产生式加模板”的承诺，规模和收益以实现测量为准。该校正服务既定S-17，不新增FX研究工程。


纠偏咨询回报与核对：cc-unisacc只读审阅指出，type/tyinfo已有共用来源，应保留；主要重复在C/Python/显式状态机的识别与控制流程。不能把这两张表的共用外推为所有语义已统一，初始化、布局与调用约定仍有手写规则。核对其候选后，DIMS/DIMSAVE和ELSZ事实上已被多处共用，DECLN只是DECL的名字适配入口，不能为了凑重构数量再抽一层。gen2头部已列出现有共用入口，防止继续绕过它们。当前FN.params/parameter_decl/parameters签名修复正是已确定的重复消除；先收尾它，不新增grammar.txt/DSL项目。咨询的“一次审计结束”只作为这次整理的边界，绝不代替S-17默认模型产物的最终验收。


### 用户重申的权威链路与 E1 实查（2026-09-27）
目标是 *.tsv规则 → 构造模型权重 → 字节流经阶段模型推导 → 字节流，覆盖C99兼容编译全链路。仅运行时使用模型不等于规则来源已完成迁移；不再把新增手写状态当作该目标的直接完成证据。

E1实际链路：exec/lex/gen.py → JSON → exec/c/tbl.py → exec/c/net.py → e1.net；exec/c/buildcompiler.sh将其装入六目标共享包，阶段接口为pp.text→tokens.typed。exec/lex/net.py是旧UNS2实验（推理后回填DENSE），不是当前候选构建入口，不能混作当前运行时证据。

| E1规则 | 当前权威输入 | 当前缺口 |
|---|---|---|
| 字符分派 | weights/gold/lex.tsv | handler把动作类别解释为具体状态/动作，仍手写 |
| 字节分类、空白、特殊词前缀 | lexcls.tsv、lexword.tsv | 部分字符串前缀转移仍直接编码 |
| token种类/词表顺序 | unisa.gold.TOKS、front.lex.TYPEKW | 不是独立TSV输入；还读取旧kernel词表校验 |
| 数字扫描 | build_num/num_start | 转移条件、回退位置和后缀规则均在Python |
| 字符串、字符与注释 | build_str/build_cmt | 转移、EOF和发射动作均在Python |
| 标点最长匹配、关键字识别 | 词表加trie构造 | trie可作为通用构造算法保留；词法专属动作须显式声明 |
| 阶段发射、计数与终止 | emit_kind/build_dispatch | typed/位置选项与输出格式仍在生成器 |

纠偏实现边界：先让完整E1的有限转移/动作、词表与输出格式成为可独立读取的数据，再复用已有权重构造与执行机制。不能只导出一次JSON/改名TSV而继续把gen.py当权威；验收须生成过程不调用原词法规则生成器、不读旧kernel，并删除被替代的手写规则。全域转移/动作对比使用冻结旧生成结果作为迁移裁判，字节流测试另验组合；不得把E1完成外推为E2—E6完成。这是当前整阶段边界核对，不是新增文法框架或继续解析补丁。


E1纠偏实施首步：exec/lex/number.tsv以44条互斥字节范围/默认转移声明完整数字扫描（十/十六进制、小数、指数回退、后缀、入口）。删除gen.py对应条件分支，净-47行；通用byterules.py只展开有限字节集合、检查冲突/全定义并链接动作序列，没有数字语义。动作使用既有执行器原语，number发射序列仍由旧输出格式代码供给，明确未完成整阶段数据化。冻结旧typed生成结果与新结果按状态名和动作内容比较85,123个观测，全部相同；序号不作为语义判据。本步尚未运行端到端候选，不复用此前产品门禁为证据。


E1字符串/注释规则迁移：literal.tsv 29条规则，涵盖字符串拼接/前缀、字符常量、行/块注释及EOF行为；空白集合引用lexcls.tsv，不复制分类答案。与number.tsv共用57行有限规则读取器，gen.py累计+23/-120（净-97）；计入读取器后的Python净减少40行，规则数据74行含两个表头。完整typed E1的85,123个生成域观测转移与动作逐项不变。exec/c/netcheck.py读取新.tbl，实际C推理对86,625个编码域观测/336状态全同，动作/字符串一致，返回0；此编码域含转换后统一结果域，不能与生成域观测数混称。尚有分派、词表、token输出、标识符专属流程未数据化，未切换默认产品、未宣称完整E1或重构完成。


E1词表输入校正：TOKS改读weights/gold/parse.tsv的tok字段（恰好一行、非空唯一），TYPEKW改读iterate/kernel/typekw.tsv（kw行、非空唯一）。默认生成不再读取kernel/unisa_model.inc或src前端文本；--check-declarations显式开启旧词表/源码兼容裁判，并接入原lex/run.sh，缓存补入全部新TSV及裁判输入。带裁判typed生成已通过，85,123个转移/动作与原token顺序全部相同。注意unisa.tsvgold仍通过gold.Stage导入Python模块，尚不能宣称独立于gold.py；仅已移除对Python词表值和旧kernel内容的生成依赖。未修改旧生成物权威或宣称全E1完成。


E1 token输出声明：output.tsv列位置前缀、名字、SPAN/SPAN2及结束动作，spelling.tsv列普通/typed模式是否附原文。emit_kind不再硬编码token编号集合或字节格式，只实例化动作参数；通用@bytes展开静态字符串。四模式两槽比对：plain/typed/positions各85,123、locations100,920个转移及完整动作全部一致，含位置寄存器与字符串输出；每个生成子进程10秒限时，总运行约1秒。该步骤并未迁移CNT计数/EOF输出和标识符控制流程，也未证明完整E1独立输入闭合。


E1标识符固定流程：Unicode转义4/8位消费、非法回退以及属性括号跳过/栈清空，迁入ident-byte.tsv与ident-stack.tsv；同一有限规则读取器支持字节范围和命名栈符号，生成器删除对应条件链与专用循环。完整typed E1的85,123个转移/动作全同。trie构造和词表分派仍在生成器，不能称标识符规则已全部迁完。


E1计数/终止迁移：CNT0字节状态、CNT1余数状态和CNTP栈状态均由count-*.tsv声明，包括计数字符输出及ACCEPT；通用install_rules同时装载byte/result/stack域，复用于字面量和标识符固定流程。生成器删除计数算法专属转移循环。完整typed E1的85,123个转移和动作逐项同旧结果；词法分派/trie专属处理仍待完成，不宣称全部E1来源已闭合。


E1分派动作迁移：entry.tsv声明lex.tsv全部动作类别的入口、字节例外和动作序列，EOF发射在output.tsv中；构造器校验动作集合精确相等，动态trie/数字入口通过显式链接注册。handler不再有词法类别if链，仅查声明并连接机器。四模式全域转移/完整动作同迁移前（85,123×3及100,920），两槽约0.9秒。identifier/punctuator链接内部仍有专属控制，须继续逐项落实而非用链接隐藏剩余规则。


E1 trie边界迁移：ident-flow.tsv声明继续字节类/UCN/结束及特殊词后空白/括号处理；ident-end.tsv按显式优先顺序链接lexword词类到结束动作，保留@next继续匹配，普通token为末项。标点最长匹配的scan.start/advance/accept/rewind及拒绝动作移入output.tsv。构造代码保留词表trie查找、最长接受前缀和声明链接，不再内嵌对应字节行为。四模式全域转移/动作全部与迁移前一致（85,123×3+100,920）。尚须解除gold.Stage间接模块依赖、核对locations附加流程及隔离生成，不能提前称全部E1独立来源完成。


E1独立输入验证：unisa.tsvgold拆出不导入gold的load_table，既有load_stage作为惰性Stage适配器保留；没有复制第二份TSV解析器。适配器18阶段8,484键schema/corpus与gold全同。隔离临时根只复制E1构造源码/locations、通用TSV读取器、E1规则TSV和parse/lex/lexcls/lexword/typekw声明，没有gold.py、kernel或参考编译器；从临时cwd生成plain/typed/positions/locations，四模式全部转移/动作与冻结旧结果一致（两槽约0.8秒）。此证据只证明输入独立，locations.py仍含封装协议控制，尚未称所有规则均为TSV。


E1位置封装迁移：location-byte/result.tsv声明magic、长度解码、边界、记录循环、输出封装与切换输入，locations.py由协议实现缩为14行装载适配。有限规则读取器仅增加观测键左移代入，参数限定0..63，不解释协议语义。四模式生成域转移/动作全同；位置模式实际C网络与表核对104,045观测、403状态，actions/strings一致，netcheck返回0。该数字是编码域，生成域仍100,920。当前未提交改动中，gen.py净-133、locations.py净-54、tsvgold.py净+5、新通用读取器76行，合计Python净-106行（run.sh不变行数）；新增规则数据315行含表头。尚需集成字节流检查、规则完整性和缓存输入核对，不以表相同替代最终默认产物验收。

E1定向字节流集成：双槽队列/tmp/unisacc-e1-rules-gate 2/2返回0，窗口3.32秒；exec-lexpos的9项参考/Python位置用例通过，exec-lexloc的6个拼接输入通过、11个畸形封装拒绝。使用私有UA，不碰/tmp/ua_ref；该结果仅覆盖两项定向门禁。


E1收尾依赖修正：共享models.identity补入iterate/kernel/typekw.tsv；旧pipeline/run.py生成依赖纳入本阶段TSV及四张gold声明；gatequeue冻结指纹补入typekw.tsv，lex/run.sh此前已纳入全部E1 TSV。临时假根分别修改typekw与entry规则，cache identity均变化，无共享输入修改。exec/lex/rules.md列完整输入、有限规则格式、trie/ABI构造边界和验证范围；不是宣称任意词法协议无需适配代码。

E1收尾定向队列/tmp/unisacc-e1-rules-chain两项全rc0，双槽42.25秒：实际网络chain固定165项全部相同，0拒绝/未覆盖/丢项（17.55秒）；exec-srcelf通过（42.21秒）。加此前词法位置与封装两项、全域转换/推理及隔离生成，形成E1声明迁移证据。产品签名修复仍未完成完整冻结门禁，默认产物切换与E2—E6规则来源统一仍未完成。

原型签名收尾：tests/c/b_blockproto覆盖double/float/_Bool参数、逗号原型与块作用域恢复。实际产品.com与实际模型候选各-O0/-O1/-O2均同宿主cc（8.0 2.2 1），Python同；原312固定项先通过，随后将此回归与s80x纳入E3/chain。当前.com为1,361,760 B，尚待本树全门禁，不作发布结论。


### 声明迁移后的冻结验收进度（2026-09-27）
当前HEAD为42a3c56（E1声明规则迁移），原型签名修复仍未提交，源码在门禁期间冻结。主队列/tmp/unisacc-protosig-full以双槽和55秒窗口完成38/119项；exec-driver-core另留独占运行，完整清单共120项，尚未全绿。主队列原始36通过、2失败不改写：exec-multi-ua并发53.05秒超时，独占复跑50.76秒通过；exec-container在net写文件时报ENOSPC，清理本轮旧候选目录的可重建中间物（保留各.com）后独占32.52秒通过。复跑分别存于同名前缀retry-multi/retry-container目录；合计38个不同套件已有通过证据，仍待82项。后续续跑原主队列，不重做已通过项；接近上限的重项独占，不能把负载超时或磁盘失败涂成原轮通过。
本轮产品unisacc.com：1,361,760 B，sha256 a61a882d506cb621c8c101370e78c6f3e0110bcff544e51b007a65b58ddc02ed，来自当前未提交签名修复；不是发布或默认模型切换。实际模型候选/tmp/unisacc-protosig-candidate/unisacc-next.com的sha256为11329391d00a2949d837f5ba008edaa1321f0b14b8574fda5f859dc641089c8c，建于E1声明迁移之前，不能称为最新规则源码重建产物。
E2后续输入审计：exec/pp/gen.py仍从kernel/unisa_model.inc提取DIRV，预定义宏按目标的分支、P0拼接和P1去注释规则仍由Python手写。这是待迁移来源，不以E1完成替代；下一阶段应复用有限规则读取与构造机制，消除旧kernel生成依赖和被替代的专属分支，不另起语法框架。


冻结验收发现测试夹具遗漏：gate-infra的cache identity假根未提供新增的iterate/kernel/typekw.tsv，导致FileNotFoundError。修复限于tests/queuecheck.py：补显式声明并验证改变它会使identity变化，不改变模型生成、缓存算法或编译器。旧队列已完成46/119项，含该失败；因检查源码改变，新冻结队列重新登记，旧记录保留为此前树证据，不冒称新树全通过。


原型签名验收收尾：43bc914补齐cache测试夹具后，/tmp/unisacc-protosig-full2跑完116个套件，原始112通过、fat超时及三个ENOSPC失败保留。磁盘清理只删除本轮旧候选中间物和未占用的旧生成缓存，保留候选.com与日志；disk-retry2的container/tableself/native-stages分别34.36/15.38/19.71秒通过；fat2独占37.30秒通过。预留core2独占52.61秒通过；heavy2的multi-ua/memory-ua/memx86-ua独占50.41/41.87/47.24秒通过。
原始full2与disk-retry2最终未返回整体绿色：协作者在运行期间新增并提交21aa027（examples/apps/colorpack.c及其README），广域指纹因而拒绝整轮结论。没有重写这些退出状态。独立输入审计以43bc914的apps README内容并排除新增colorpack，重算得到完全相同的原指纹3a6c0afaf0eb7feea0f05771f74b014725c29209b750186ca435d3df8d2743bc；证明其余全部冻结文件（含编译器、生成器、测试及.com）未变。所有120个套件命令逐项与当前--plan --com相同，新增colorpack不在固定列表或examples/*.c输入中，apps README也不是构建/运行输入。最终以实际单项rc0并集核对120/120、无遗漏；这是附非输入变更审计的分批验收，不宣称原队列单次rc0。完整对账保存在/tmp/unisacc-protosig-final-evidence.json，原记录目录保留。
本次只闭合已开始的块内原型签名修复与E1迁移回归；默认产品仍非完整模型路线，未发布，E2—E6声明来源迁移与最终默认产物验收继续。


### E2声明输入迁移（进行中）
先冻结六目标、普通/位置两种模式的全部状态转移与动作，作为旧实现裁判；实现去掉kernel/unisa_model.inc的DIRV提取，改用pp.tsv自身schema与已验证的通用读取器。目标预定义宏改成OS/架构/公共项声明，保留当前顺序与当前兼容行为（Windows上的__LP64__仍是现有子集契约，不外推系统ABI）。同步真实构建缓存依赖；之后迁移P0/P1等有限控制规则，不能把本步仅移除输入依赖称为完整E2声明化。

E2首步实测：六目标×普通/位置模式12份JSON的完整状态、转移与动作均与迁移前一致；Linux/macOS普通180,466、位置183,036观测，Windows普通179,951、位置182,521。隔离临时根只放pp构造源码、通用TSV读取器、pp.tsv、predefines.tsv和include头声明，无kernel、gold.py、src；从/tmp生成12份均相同。临时声明把__linux__改为__linux_variant__，只有一个初始化动作序列变化，状态映射不变，证明宏名称确实由声明控制。当前改变的是输入来源，P0/P1和宏控制规则仍手写；Python行数未减少，不称完整E2声明迁移。
E2输入迁移集成：/tmp/unisacc-e2-declarations-gate双槽2/2通过，总2.74秒；宏展开86个完整结果与4个明确拒绝，位置封装9个参考用例与5个Python裁判用例通过。缓存来源同步到pp/run.sh与旧pipeline/run.py，共享models.py原已包含exec TSV和weights。未改产品源码，不把此定向结果写为新一轮全产品门禁。

E2下一步：P0（shebang/续行）和P1（注释/字面量保留）转移与动作迁入有限规则TSV，删除原Python分支。复用E1读取器并移为exec/finite_rules.py公共模块，仅增加显式常量绑定替换；SPLB是执行器存储布局常量，出口是阶段链接，不由回调藏词法决策。普通、位置、自动头开启/关闭均核对完整转移和动作；顺序号可能改变，比较动作内容而非序号。

E2 P0/P1实现：text-byte.tsv与text-result.tsv共46条规则声明shebang清空、续行拼接、字符串/字符保护、行/块注释、换行保留与未终止拒绝。删除gen.py对应分支；E1有限读取器移到exec/finite_rules.py供两阶段共用，增加显式constant绑定替换，不调用语言相关Python回调。六目标×两模式的完整观测/动作同旧结果；关闭自动头的两模式120,842/123,412观测亦同。共享模块迁移后E1四模式85,123×3及100,920观测同旧结果。隔离根无kernel/gold.py/src的12模式生成通过，宏改名输入依赖检查仍通过。
cc-unisacc的死函数清理建议已收到但未实施；其“经典walker不得模型化”的措辞适用于旧产品T-1边界，不代替用户重申的S-17全阶段模型推理目标。本轮不增加产品修改范围。
E2 P0/P1定向集成收尾：/tmp/unisacc-e2-text-gate双槽4/4返回0，总6.05秒；exec-macros、exec-pploc、exec-lexpos、exec-lexloc分别2.84/2.44/3.52/2.72秒，实际网络路径覆盖宏展开与位置封装。计入共享读取器移动及新增常量绑定，本片Python净减少29行，46条规则成为显式输入；没有复制旧读取器。未改产品源码、未重跑全产品门禁；E2剩余宏/指令控制仍待迁移，不称完整重构完成。

E2宏存储规则迁移决定：MFIND/MDEF的定义可见区间、历史链遍历及新记录初始化迁入有限转移声明；字段偏移与存储基址显式绑定，继续使用现有通用读取器。删除对应Python状态分支，不把规则搬到新的生成脚本。以冻结的完整E2转移/动作及实际宏用例验证。

用户再次确认核心：表（*.tsv）构造确定性网络权重，模型推理替换程序逻辑；重构验收要同时记录实际权重输入与删除的专用程序逻辑，不能保留第三套生产规则后只报表覆盖增长。规则本身的信息仍需存在，迁入声明不代表语义复杂度消失；应收缩的是语言专用程序实现及其重复来源。
E2宏存储迁移实测：macro-byte/result.tsv共27条声明（含不可达默认），删除MFIND/MDEF的Python分支，gen.py净减少19行；复用有限读取器，无新增专用框架。十二目标/位置模式的全部转移与完整动作同冻结参考；实际网络exec-macros与exec-pploc双槽2/2返回0，窗口2.71秒（分别2.67/2.49秒）。产品源码未变，不称全产品重验或完整E2完成。

E2自动头扫描迁移决定：将AISTART—APW调用/定义识别及括号扫描、ACP复制收尾移入有限规则声明；沿用通用读取器。头文件导出名字的提取与按头展开的构造暂保留并明确列为剩余逻辑，不宣称整个自动包含已声明化。

E2自动头扫描迁移实测：autoinc-byte/result.tsv共31条规则，删除调用/定义扫描及复制收尾分支；gen.py +5/-31，净减少26行，无新增构造器框架。六目标×普通/位置模式全部状态转移和动作同冻结参考。实际网络双槽exec-macros与exec-pploc 2/2返回0，窗口2.67秒；位置用例含printf自动头路径。头名字提取及按头展开仍在Python，后续继续消除专用控制，不能把本片当整个E2完成。

E2自动头展开收尾方案：复用有限规则读取器，增加显式状态名链接（$name由绑定给出），把逐名字called判定和条件输出头行做成可重复实例化的声明模板。生成器只枚举头/名字并绑定连接、数据字节和布局；不生成第二份展开答案表，不新增语法框架。

E2自动头展开模板实测：autoinc-name/emit的byte/result声明替换按名called判定及按需输出分支；状态链接和字段绑定复用finite_rules，text/macro/autoinc装载统一。gen.py净-5行、共享读取器+6行，本片Python净+1行，不能称本片总代码减少；增长仅为通用显式状态绑定，专用判定已删除。六目标×两模式完整转移/动作不变，E1四模式及E2无自动头两模式也全同。双槽实际网络四项检查4/4返回0，窗口6.09秒。头名字提取、头顺序与输出文本构造仍为剩余来源，尚未完成整个E2声明化。初次替换保留了一段旧代码导致IndentationError，已删除残段后重新完成上述全部验证，未把失败记为通过。

E2表达式归约迁移决定：运算符编号/拼写/优先级/元数成为声明数据；XRED及比较、短路毒值传播、除余零标记、三元和一元归约由有限规则声明给出，删除Python按运算符分派。值栈布局只作为显式绑定。该迁移保持现有行为，不把参考一致性当作完整C预处理语义证明。

E2表达式归约完成本片：operators.tsv声明25个运算符的固定机器编号、拼写、优先级、元数；reduce-byte/result.tsv的67条规则负责运算分派、比较真值、短路毒值、除余零标记及一元/三元归约。编号仍是与语法转移共享的机器ABI，不能任意单独改号；语法扫描与运算栈调度仍待迁移。gen.py净-40行；集中回归测试净+11行，Python合计净-29行。十二目标/位置模式及无自动头两模式完整转移/动作同旧结果。实际网络双槽2/2通过，3.04秒；新增11个独立真假预期覆盖各运算家族与未求值除零，两种封装都与参考完整字节、宿主预处理token和Python执行器一致。共88个完整结果及4个拒绝；不扩大为全C语义证明。

E2表达式控制收尾决定：把剩余XE扫描/defined/宏帧/优先级弹栈调度完整转移迁入声明；优先级仍绑定operators.tsv，不复制优先级答案。通用装载器登记声明中的PUSH返回标签，生成器只绑定存储布局/优先级并装载。删除xpushop/xpopv/XPUSHV/XTOP和build_xe的手写实现；参考实现由已有提交与冻结转移保留，不在生成路径运行。

E2表达式控制迁移实测：expression-byte/result.tsv声明78个状态、241条互斥/默认规则，涵盖数字扫描、defined、对象宏帧、操作符识别、优先级弹栈和括号/三元调度。删除xpushop/xpopv/XPUSHV/XTOP及对应控制分支；build_xe只绑定布局与operators.tsv优先级后装载，返回标签由通用PUSH动作登记。gen.py +8/-139净-131行，回归测试净+1，Python合计净-130。十二目标/位置及无自动头两模式完整转移/动作同冻结参考；E1四模式未变。实际网络两项并行2/2返回0，窗口2.89秒，表达式独立真假用例扩到13项，含defined双形式、嵌套对象宏、未定义名字取零；88完整结果和4拒绝。运算符编号/拼写仍是该机器固定ABI，声明变化须连同相应语法/归约一起改；不宣称任意新增运算符自动成立。E2其他宏/指令流程仍待迁移，未完成整个重构。

E2宏字符串化/拼接迁移决定：完整HSCAN/HX状态规则由TSV提供，保留通用原语和显式字段布局绑定，删除build_hx内部pre/plook及所有专用分支。先以冻结全转移/动作确认等价，再复用实际网络的C99字符串化、拼接、空参数、变参和hide标记用例；不扩原先拒绝的拼接域。

E2字符串化/拼接迁移实测：hash-byte/result.tsv声明68状态、221规则，字段偏移仅由8个布局常量绑定；build_hx及pre/plook等专用控制删除，gen.py +4/-167净-163行，无新执行器原语。十二目标/位置及无自动头两模式完整转移/动作均同冻结参考；E1四模式仍一致。实际网络exec-macros/exec-pploc双槽2/2返回0，窗口3.00秒，88完整结果和4拒绝均保持。规则信息移入声明，网络规模与能力未改变；支持域外的拼接仍拒绝，不宣称新增C99覆盖。E2生成器现968行，指令扫描与宏重扫主体等仍待迁移。

E2命令行资源迁移决定：-D/-U/-include/-nostdinc的有限扫描与宏记录动作迁入声明；位置模式仅绑定附加计数动作，不保留命令行语义分支。复用现有构造器与实际driver-resources门禁，使用私有UA且独占重项，不增加新套件框架。

E2命令行资源迁移实测：cli-byte/result.tsv共19状态、43规则，生成器净-37行；位置模式只绑定行数计数动作，字段/基址显式绑定。十二目标/位置及无自动头两模式完整转移/动作同冻结参考。实际exec-driver-resources独占31.37秒通过（队列窗口31.50秒），覆盖宏参数、头资源、printf/math与隔离容器；未并发重项。一次性迁移脚本首轮误写至hash文件名导致缺cli声明而拒绝生成，随后把生成内容移到cli、从HEAD还原hash（无差异），补全间接ea布局绑定后重新验证；无失败隐藏。指令与宏重扫主体仍待迁移。

E2宏重扫迁移决定：P4对象/函数宏展开、参数帧、跨帧调用、hide标记、UCN及_Pragma控制整体迁入声明；字段布局/容量显式绑定，保留当前行为与拒绝域。删除对应Python流程，不调用旧生成器补答案；位置封装与指令阶段仍单列。

E2宏重扫迁移实测：rescan-byte/result.tsv声明144状态、404规则，覆盖原P4全部对象/函数宏、参数帧、跨帧续接、隐藏标记、UCN、标点与_Pragma处理；删除原流程和无剩余调用的PUSHM/PUSHMB/CRC动作代码。字段/容量26项显式绑定，未增加执行器原语。gen.py +2/-322净-320行，现611行。十二目标/位置及无自动头两模式全部转移/动作同冻结参考，E1四模式同旧。实际网络双槽3/3返回0：固定源码→tape链167/167相同、无拒绝/未覆盖/错误/丢项（17.06秒）；宏88完整结果与4拒绝、位置封装均通过，窗口17.10秒。只改变规则来源与删除重复程序控制，不宣称新增覆盖或默认产品切换；E2指令阶段仍待完成。

E2指令宏体/包含流程迁移决定：DEF0至INCH的参数声明、宏体保存、include资源搜索与拼接流程迁入有限规则；位置附加记录单列声明并显式连接。pp.tsv决策读取和act连接暂保持，不能把生成时选定的决策复制进新表后切断pp.tsv输入。

E2指令宏体/包含迁移实测：directive-body的33状态80规则、include-location的3状态5规则来自显式声明，位置体入口由绑定连接，IRNAME/IRLN/IRNL等保持布局参数。gen.py +4/-91净-87行。十二目标/位置、无自动头两模式的完整转移/动作同冻结参考。资源重项独占31.88秒通过，随后宏2.94秒、位置2.57秒通过，队列3/3返回0，窗口37.66秒；使用私有UA。剩余act及指令条件连接仍保留pp.tsv为决策来源，下一步迁移其动作模板，不复制PP答案。

E2指令动作迁移决定：以（指令名、pp.tsv输出标签）选择有限规则模板；共享读取器增加声明分节选择，不含指令语义。仅登记当前声明域使用的模板，未声明的组合明确失败，不能默默回落。删除act条件链；通过临时修改ifdef结果验证实际表输入仍控制输出。

E2指令动作迁移实测：directive-action两份分节声明共14个（指令、标签）模板、45规则；pp.tsv输出标签直接选择模板，act条件链及无剩余调用的ea动作帮助函数删除。生成器净-43行，共享读取器净+6行，Python合计净-37行。十二目标/位置和无自动头两模式完整转移/动作不变，E1四模式亦同；实际宏/位置网络双槽2/2返回0，3.08秒。隔离临时声明仅改ifdef/0 skip→take，构造tbl/net并核对网络=表，实际预处理结果NO→YES；改为未声明的ifdef/macro则生成失败且无输出。未复制PP决策答案，原读取路径保留。一次性迁移脚本最初字符串格式替换失败，修正后才删除旧act并完成全部验证。

E2指令扫描收尾决定：P3行扫描、条件入口和pragma拒绝规则迁入声明；DSW仅按pp.tsv字段顺序连接到具名状态，动作仍按pp.tsv输出标签选择。目标预定义名字的逐项装配保留为数据连接，扫描分支不保留Python副本。

E2指令扫描迁移实测：directive-scan的41状态86规则替代P3行扫描、指令条件入口与pragma处理；DSW按pp.tsv字段顺序连接具名入口，动作标签选择仍读取pp.tsv。gen.py净-84行（含两个无剩余用途的定义清理），现397行。十二目标/位置模式完整转移/动作同冻结参考，无kernel/gold.py/src的隔离生成十二模式亦同；预定义宏改名只改变相应初始化序列。共享检查确认无自动头两模式及E1四模式仍一致。实际网络双槽三项全rc0，窗口17.01秒，chain固定167/167相同，宏88结果与4拒绝、位置检查通过。剩余为数据/模板装配及locations封装等，不将当前结果称为整个编译器重构完成。

E2位置封装迁移决定：UNIPP1字段输出与记录循环迁入有限规则声明，locations.py仅绑定布局；把现有规则装载/返回标签登记函数移到共享读取模块，避免在辅助文件复制装载实现。协议格式保持不变，以完整位置模式转移及实际封装解码核对。

E2位置封装迁移实测：location-byte/result.tsv声明8状态14规则，locations.py只绑定布局；装载/返回标签登记从pp/gen.py移到共享finite_rules，未复制。十二目标/位置、无自动头两模式与E1四模式完整转移/动作全同。实际网络四项双槽4/4返回0，6.24秒。新增exec/pp/rules.md列声明来源、真实构造链与剩余装配边界，不称全C99或默认产品切换完成。

E2固定装配动作迁移决定：预定义宏写记录、每个自动头扫描的初始化/结束、普通输出ACCEPT迁入assembly分节模板；数据枚举与状态名连接留在构造器。移除不再使用的调用构造方法，不添加另一套装配引擎。

E2固定装配动作迁移实测：十二目标/位置模式的全部观测与动作保持一致；实际网络exec-macros与exec-pploc双槽并行2/2返回0，3.04秒。gen.py净减5行，删除未用G.call，规则转入assembly声明。用户再次明确：迁移应减少手写编译规则与重复实现；仅增加表、保留对应专用逻辑不能算替换完成。总文件行数与网络大小不由该原则保证递减，应分账报告。

旧E3语法副本清理决定：当前stage入口是parse2/gen2.py，parse/gen.py只作为共享构造模块导入。按AST真实属性引用与模块内部依赖闭包核对后，删除退出入口的旧expr/stmt/spec/unit及只被它们调用的帮助函数，不搬到新文件、不改变现有P/词法读取器。保留历史提交供研究追溯。先冻结现用生成入口各模式的完整JSON哈希，再验证删除后相同，并运行实际网络链与共享消费者门禁。此项删除重复实现，不冒称E3现用规则已全部声明化。

旧E3语法副本清理实测：删除20个退出入口的函数及只服务旧语法的加载/存储/内建调用答案表；parse/gen.py由1913行降至450行（净-1463），保留现用P、tokenizer、数字格式、自动头扫描和类型描述符。当前E3入口仍为parse2/gen2.py，退休CLI明确提示新入口。固定PYTHONHASHSEED=0后，原提交模块与当前模块的23种完整JSON逐字节哈希相同：E3五模式、独立单元两模式、优化两级、六目标lowering、两架构raw/ELF/MachO/PE。初轮未固定hashseed的E3/单元JSON哈希不同而失败，未记为通过；重新从HEAD提取旧模块隔离生成后核对。实际网络与共享消费者双槽6/6返回0，38.79秒：chain167/167，E4的139文件两级278同，E5的53夹具同，位置单元/诊断/lowering通过。无产品代码或默认.com更改。

用户要求并行小任务：E3模板、E4固定控制、E5固定编码各分到独立/tmp worktree，由父会话单写主树和prd、统一审查合并；每任务8分钟界限、子命令≤60秒、私有测试路径。交付按实际删除专用逻辑和规则来源验收，不以并发数量充当进度。

并行实施分工细化：E3将现有TEMPL/SPANS迁入声明，push/pop仍绑定现有单一输入；E4迁整个procs固定扫描组（SKIPL/COPYL、SIMPLE与NAMES），SIMPLE仍查询动态opinfo初始化内存，不复制答案；保留构造状态编号契约以核对完整输出；E5只迁完整固定编码过程，catalog等动态数据仍外部绑定。

并行三片已分别审阅合并：E3模板、E4固定行扫描、ARM整数/frame。集成时发现ARM声明展开了公共word字节写出帮助函数；改为已有声明序列绑定复用word，避免固定规则再产生第二个字节序权威。随后统一构造与验证，不重复逐片全门禁。

并行集成验收：三子任务约3分钟交付，均在隔离worktree提交。父合并后复用ARM word动作序列12处（不复制公共字节序实现）；实际网络与共享消费者5项双槽45.41秒全rc0：chain167/167、E4两级278结果、ARM检查、E3位置及诊断。三片Python净-88行，加退休副本净-1463，合计净-1551；声明数据单独增加，未称网络规模减少。无新增执行器原语，准备从统一源码重建开发模型容器。

统一模型候选已从3eab8b9构建：/tmp/unisacc-parallel-rules-candidate/unisacc-next.com，5,985,504 B，sha256 90b10f8057532f622789dc9848f51d57c7a0072c45c4161bf0a48911379fdfff。构建在55秒外限内rc0，所有内部步骤各≤60秒。实际容器生成hello的六目标-O2镜像均与私有参考逐字节相同；hello/fib/b_strderef三程序×O0/O1/O2九次本机运行的输出/错误/退出码与宿主cc相同。宿主cc显式-include stdio.h补齐示例省略的声明，不把自动头扩展冒称标准C。首次测试驱动直接exec APE而报Exec format error，改由sh启动；随后宿主示例缺printf声明导致驱动失败，补头后完整重跑为绿，非产品修复。该候选已包含本轮TSV和网络；不是默认切换、跨平台原生全套或完整重构结案。

第二批并行规则迁移决定：E3优先完整BIND/UNWIND作用域保存恢复组（现有布局保持）；E4完整local()局部优化控制；E5完整armmem内存与zero控制。复用既有finite_rules与P状态标识，动态类型/指令事实仍绑定原表，共用输出动作只绑定不复制。各自隔离worktree，8分钟内交付，父负责主树集成和统一实际网络验证。

默认切换阻挡实查：用3eab8b9真实候选复跑旧盘点16项（编译器哈希前后固定），6项已同参考；7项仍拒绝（含不合法的s36x），1项参考亦拒绝；b_strsizeof/b_wide两项实际上模型rc3步数超限，不能算未覆盖正常拒绝。根因定位到ER.position为了渲染错误再次调用NEXT，遇到token reader的string prefix拒绝会重入自身。修法：共享位置前缀读取与token语法读取分开，诊断只读取位置前缀，不重解析失败token；畸形位置传输本身不进入依赖该传输的诊断定位。补实际网络拒绝回归，不能把宽字符串支持虚报完成。

诊断重入修复实测：位置前缀读取成为共享子过程，正常NEXT继续解码token，错误定位只读取前缀。实际网络错误/警告错误、位置解析、多单元位置四项双槽全部rc0，窗口12.84秒；两个宽字符串用例以带准确位置的拒绝结束，不再反复重解析。不新增宽字符串支持。

第二批并行集成实测：E3 BIND/UNWIND、E4 LOCAL、ARM MEM/ZERO三组已合入，Python合计净-109行；声明增加273行，源文本总计净+164，不称总规模下降。无新增执行器原语或复制动态指令事实。集成五项双槽41.50秒全部rc0：chain167/167、E4两级278同、ARM、两种实际网络错误模式。统一候选来自d487647，5985198 B，sha256 128e9f09a400a3418a9d1c3d67f56d50ff92fb635d52ca987fad4e717f4c24c6；真实容器复查16项，6同参考、9明确拒绝、1参考亦拒绝、0DIFF、0工具失败。b_strsizeof/b_wide已从步数超限变为有位置的string prefix拒绝；宽字符串能力仍缺失。三路下一小组并行处理E3 DECL/MAXF、E4 STFUSE和ARM分支标签，不扩语法或框架；父统一验收。

第三批并行集成实测：DECL/DECLN/MAXF、STFUSE、ARM分支标签三组已合入55a49c1；Python净-75行，声明净+260，全部源净+185。与上一批合计删除184行Python规则，数据增量另计；并非网络规模或全库行数下降。父审查后统一双槽4/4通过，42.52秒：实际网络chain167/167，E4两级278同，ARM与带警告诊断通过。首次队列参数误写不存在的exec-armbranch，调度器以2拒绝、未执行测试；改用现有exec-arm后完整重跑。三子代理本轮交付结束，无共享树写入；默认产品与发布仍未切换。

下一并行批决定：E3共享types过程、E4 peepround/peep_start、ARM浮点完整组分别迁入既有有限规则声明；动态type/tyinfo/peep/catalog事实仍用原表绑定。父核对剩余模块清单与默认模型产物阻挡，避免把逐片搬运当作全重构完成。各任务8分钟、子步骤≤60秒，统一集成验证。

第四批并行集成实测：E3宽度访问/窄化（非整个types）、E4完整peepround、ARM24个浮点操作已合入3860676；Python净减223行，声明增加498行，源合计净增275。动态tyinfo、peep/opinfo以及ARM opcode来源经扰动验证未冻结；公共word与数据输出模板复用。父集中双槽4/4全rc0，41.28秒：实际网络chain167/167，E4两级278同，ARM与警告诊断通过。exec/rules.md列当前各阶段声明与剩余手写构造，README区分历史包装里程碑和当前候选；不是完整重构验收，不称默认产品已切换。本批未重建.com，迁移前后完整转移证据及组合门禁不冒充新产物哈希。

第五批并行决定：E3结构语句控制整组、E4 analysis活跃性/块分析完整组、lower/data稀疏数据布局完整组各交隔离worktree。复用现有有限规则装载，原动态事实仍绑定；父单写主树/清单、集中验证。各8分钟、子步骤≤60秒；不新增语法或执行器原语。

E4 analysis先完成后复用空闲槽继续完整parsers/rtok/anyb组，build外层仍单列；与其他隔离任务并行，不等待空槽、不重复全套。

E4 tape匹配组完成后，在同一隔离槽继续最后build外层及lit/regnum固定控制；动态opinfo/peep初始化保留并列明。父先验收已集成批，后续提交单独组合，不在测试期间改主树。

第五批集中验收：84cfcc8包含结构语句、analysis活跃性、tape匹配、稀疏布局四组，Python共净减389行，声明增加1105行，总源净增716。五种E3模式、两级E4和六目标data/full的完整转移/动作对比均保持；动态布局/答案扰动检查另在隔离树通过。父六项双槽35.11秒全rc0：网络chain167/167、E4两级278同、警告诊断、raw/full lowering、sparse巨大零尾部及拒绝。E4 build外层还在隔离任务中，未包含本次验收；源码清单同步到exec/rules.md。无新默认产物或完整重构结案。

第六批并行决定：E3完整switch/case/default及语句入口派发，lower/code完整指令控制（若目标动态分派需纯数据装配则保留）迁入既有有限规则；E4当前外层任务继续不重启。仍按完整转移、动态输入依赖与实际网络验收，父单写主树；每任务8分钟、每步≤60秒。

E4外层控制已交付并审核，下一空闲槽转ARM arminput完整输入/元数据组；固定扫描规则迁声明，target/schema常量绑定，禁止新增解析框架。

E4固定控制来源收尾实测（e7e15cf）：build外层及lit/regnum迁入rounds声明，Python净减106行，声明+352；gen.py现204行。固定扫描、局部改写、活跃性分析、peephole、匹配与轮次控制已由声明提供；仍保留LEVEL选择、opinfo/peep数据初始化/索引分派、常量/fresh绑定与通用装配，不称Python构造工具完全消失。独立双槽exec-e4/exec-e4self 2/2全rc0，35.09秒：139文件两级278同，编译器自身两级2同（12.85秒），无拒绝/错误。本证据固定E4源码，不代替后续其他阶段组合验收。

第六批集成与产物实测：E3 switch/case/default及入口标签/goto、lower code扫描、ARM输入metadata三组与E4外层已合入0ae4ad2，四组Python净减227行；声明另增786行（不称总源变小）。父组合六项双槽25.66秒全rc0，含chain167/167、三类lowering、ARM和警告诊断；E4专门含自身的2项证据见前条。统一真实模型候选/tmp/unisacc-parallel-sixth-candidate/unisacc-next.com为5,985,531 B，sha256 9b5b710bd97cc590371a9fc73de159ee8aa6f0fcfffa34f297ae0d3a54e1bbc7；构建55秒外限内rc0，模型包5,802,219 B。候选六目标hello镜像与私有参考逐字节一致；hello/fib/b_strderef/b_switch3四程序×O0/O1/O2共12次macOS arm64运行与宿主cc输出、诊断、退出码一致（cc显式补stdio.h）。默认产品未切换；这些是限定产物验证，不是完整C99、六平台原生或整个重构完成。

第七批并行决定：E3类型声明/维度辅助完整组，ARM armlayout地址解析/布局组，以及lower code固定输出/融合组继续迁入既有声明；动态ABI/catalog/tyinfo事实保持原输入，按完整组验证。各隔离8分钟、子步骤≤60秒，父单写主树并集中验收。

父复核剩余源发现清单遗漏：parse/gen.py删除的是退休语法副本，仍有现用tokenizer/prn/numout/fconv规则；units.py实际调用fconv。不能把“共享构造支持”误写成全通用代码。已更正exec/rules.md，后续须完整迁移这些共享规则，不复制进各消费者；本批不碰该共享文件，避免与并行生成对照交叉。

第七批验收（975ee7c）：函数指针/参数平衡/维度/初始化计数/维度保存，lower固定输出/融合/参数输出，ARM地址布局三整组已合入。Python净减151行；声明增加664行，总源净增513。共享strwalk/值栈/模板、动态regmap/reloc/image事实均未复制。父组合双槽6/6全rc0，26.18秒：chain167/167、ARM（三OS地址字节解码含在内）、三类lowering及警告诊断。子代理还做五模式E3、六目标lowering、四封装ARM完整转移动作对比与对应实际探针。父要求把八个mov文本重新变回range枚举，改后六图逐字节等于已测图，避免数据装配源码反向膨胀。最新产物仍为0ae4ad2候选，本批未重打.com；实际剩余共享parse规则已补入清单，完整重构未结案。

第八批并行决定：E3 types中TSPEC/结构布局完整控制组、ARM Windows setup完整组、lower剩余setup/dispatch固定控制各在隔离worktree迁声明，保留动态数据计算及共享输出序列。父只审计共享parse规则的消费者与迁移边界，本轮不修改共享模块。每路约8分钟，子步骤≤60秒，集中双槽验收；不扩语法与框架。

第八批验收（29e2f57）：TAG作用域/SBODY结构布局、ARM Windows元数据/API准备/返回转换、lower setup/dispatch完整组合入。Python净减128行（89+42−3），TSV增加568行，源合计净增440；lower绑定净增3行不称缩减。动态布局/IMPORTS/WINARGS_BODY/regmap依赖扰动及全图比对通过，模板保持原来源。父双槽6/6全rc0，25.94秒，chain167/167及ARM/三lower/警告诊断全通过。候选重新构建于55秒外限内，5,985,519 B，sha256 19a4558210cfe1a7a1cc24e0aeeaf94e2edb2cf091725512c9eb56b2cda4cc1f，模型包5,802,207 B；六目标hello镜像同私有参考，四程序×三优化级共12次macOS arm64实跑同cc，记录/tmp/unisacc-parallel-eighth-candidate/acceptance.json。最新候选未替换默认产品；TSPEC、共享tokenizer/数值/自动引头规则、ABI/syscall及其他剩余模块与兼容差距继续列在exec/rules.md，未称全重构完成。

第九批并行决定：E3剩余TSPEC/指针尺度完整组，共享parse/gen.py的prn/numout/fconv完整数字组，以及lower的prelude/ABI系统调用控制各自隔离迁入既有声明。共享数字变动与E3消费者以同一250daa6基线独立比对，合入后再组合验证；保留tokenizer/自动引头以免任务无界。各8分钟、子步骤≤60秒，不新增框架或重复模型答案。

第九批验收（2273f3e）：共享PRN/PRNW/NUMOUT/FCONV、TSPEC/DSTARS/PWIDTH/SCALE/DSCALE、lower prelude及syscall准备回写已迁声明。Python净减90，TSV增加358，总源净增268。PRN/PRNW审查后共用一节，以宽度/状态绑定两次实例化；数字25消费者全图同。E3新增动态类型词旧新全图同；首轮导入build发现CLI warnings全局依赖，已修并重跑。父双槽9/9全rc0（52.19秒窗口8项，rc75续跑2.25秒完成最后项），含E4自身两级、chain167/167、多单元位置、ARM及三类lower。新候选2273f3e重建与上一批哈希完全相同：5,985,519 B，sha256 19a4558210cfe1a7a1cc24e0aeeaf94e2edb2cf091725512c9eb56b2cda4cc1f；六目标镜像同参考，六程序含b_float/b_fconv×O0/O1/O2共18次实跑同宿主cc。证据/tmp/unisacc-parallel-ninth-candidate/acceptance.json。仍有tokenizer/自动引头、E3表达式与初始化、ABI主体、其余编码/镜像规则和产品兼容缺口，默认产品未切换，完整目标保持。

第十批并行决定：E3完整OPX/CKM/RESD运算尾组，共享tokenizer的固定数字/字符token读取组（动态词典trie保留），lower剩余SYSCALL/SC调用体使用参数化声明迁移。动态type/tyinfo/ABI来源不复制，各取27d704e隔离基线，8分钟/命令60秒；父集中验收。

第十批审查调整：lower全ABI尝试只是将P通用装配换成长绑定字典，Python+25且实质决策仍在，父拒绝合入该包装；保留通用P装配，只把mode/shape参数来源关系迁数据，ABI余控制继续明确未迁。E3完整OPX任务首窗口仅完成CKM/RESD中间态，未合入，继续一个有界窗口完成共享组合模板，避免以局部交付替代整组。

第十批验收（c839770）：完整OPX/CKM/RESD共享参数规则、固定字符/数字token读取、自动引头定义/使用扫描、ABI参数来源关系已合入。Python净减126行，TSV增加470行，源合计净增344。父拒绝ABI机械包装，并删未读取的参数index列，行序唯一决定位置；ABI余控制尚在，不称整组完成。OPX用共享查询/浮点体/指针类别/整数尾，不逐operator复制；五模式全图和动态AX/TYINT/FPU/optext扰动相同，3实际网络探针通过。父双槽6/6全rc0，20.29秒，chain167/167、三lower、多单元位置和警告诊断全绿。本批未重打.com，最新产物仍2273f3e；上一产物补测两个真实编译单元含浮点调用和INT64_MIN，O0/O1/O2三次同cc，证据/tmp/unisacc-parallel-ninth-candidate/multiunit-acceptance.json。

共享token读取审计边界：直接reader把裸0x当整数0是既有行为。父仅编译最小源int main(void){return 0x;}验证：当前私有参考与2273f3e真实候选均rc0，宿主cc rc1，未运行该不合法程序；不是迁移新引入，也不是C99正确性的证据，单列既有诊断缺口（/tmp/unisacc-malformed-token-evidence.json）。

第十一批并行决定：E3完整ladder优先级/短路组，lower armfuse完整融合组，elfimage共享输入/重定位/写出控制完整组迁现有声明。动态prec/opinfo/SHAPE/布局/byte等绑定原来源；E/C或双架构必须共享参数化规则，不逐实例复制。隔离基线f253fd0，每窗约8分钟、子步骤60秒；父审查实际逻辑替换，拒绝机械包装膨胀，集中验证。

第十一批验收（ac11995）：三小代理隔离并行完成ladder、ARM融合、共享镜像控制。Python分别净减10/30/56，共96行；TSV新增70/214/191，共475行，源合计净增379，不能称整体瘦身。E/C优先级共享24参数规则；ARM按动态SHAPE共用匹配模板；镜像六wrapper共用187规则。各路完整图和动作核对相同，优先级/token/SHAPE扰动保持新旧一致。父冻结后双槽队列10/10全部rc0，33.55秒：chain实际网络167/167、ARM lower、双ELF、双Mach-O含本机执行、双PE、sparse、warnings。队列证据/tmp/unisacc-parallel-eleventh-gate。保留ABI/SYS主体、ARM目标形状选择、ELF字段/import枚举及格式子安装，不称全部迁完；本批未重打.com，实际候选仍2273f3e。

第十二批并行决定：以d5dcda4为隔离基线，三个小代理分别迁完整聚合初始化、普通字符串解码/初始化、PE镜像控制；各自仅改所属模块及TSV。共享walk/存储/字段写出保持参数化，动态ESC/类型布局/导入字段保留真实来源；不扩loader、不复制参考算法到新Python。每窗8分钟、每子步骤60秒，完整图动作核对加既有真实网络或镜像检查，父冻结后集中验收。

第十一批实际产物补验：d5dcda4在私有UA下重建/tmp/unisacc-parallel-eleventh-candidate/unisacc-next.com，6,005,111 B，sha256 3ffcb9bdada262e13bc9a9e8ac3c638059e8eab465690e90ffe7c44f3565cf64。六目标hello镜像同参考、六程序O0/O1/O2共18本机运行同cc；两编译单元含浮点调用/INT64_MIN的三优化级也同cc。相对2273f3e包增19,592 B，不称压缩；父独立全图核对E3每观测/后继/动作相同、9806隐藏单元不变，但state/sequence排序不同，E3.net增9761 B。证据同目录acceptance.json、multiunit-acceptance.json、migration-size.json。

第十二批验收（0a9377a）：聚合初始化163规则、普通字符串共享walk及初始化、PE固定控制82规则已合入；Python净减42/25/27共94行，TSV新增253/103/86共442行，总净增348。动态类型布局/ESC/import及布局常量扰动下旧新完整图相同；聚合5网络探针、字符串三存储类/转义探针和双PE字节边界检查通过。父双槽6/6全rc0，20.34秒，含chain实际网络167/167、双PE、多单元位置、警告诊断、sparse。父合并后完整E3图5300states每观测/后继/动作0差异；序列化字节不相同，固定PYTHONHASHSEED=0仍不同，确认为state/seq顺序差异，未把它写成字节不变。证据/tmp/unisacc-parallel-twelfth-gate/combined-graph.json。保留类型ctx共享装配、ESC保留字符筛选、PE字段/import装配；没有新loader原语；本批不重复打包，最近实际候选仍d5dcda4。

第十三批并行决定：d1df8b8隔离基线，完整常量表达式控制、块静态存储控制、Mach-O布局/签名驱动控制三组；算术/比较/布尔规则按运算类别共享参数模板，静态标签MAXTOK与现有布局仍动态来源，Mach-O字段枚举及SHA共用保持不复制。只改所属模块与TSV，现有loader不扩张。每窗8分钟、每步60秒；完整图加动态绑定扰动与实际网络/原生镜像，父组合门禁。

第十三批父任务补充：同步迁sha256delta固定压缩/填充控制，保留src/back_image.c常量真实抽取，以共用初始化序列绑定进声明。Mach-O代理不改该文件；父最后联合验证完整封装图及FIPS/边界摘要。

第十三批验收（5a7a5d3）：三代理完成constexpr76规则、statics完整块静态组、Mach-O43规则；父完成SHA固定控制33规则。Python分别+4/-30/-12/-42，共净减80；声明158/128/47/43，共新增376，总净增296。constexpr装配增加4行，明确不是代码缩减；没有另写运算控制体。五模式、levels/ops/token/enum、MAXTOK/布局及Mach-O参数扰动核对通过，实际常量/块静态/多单元网络与双架构原生镜像通过。父SHA原图及常量/W/K/fresh扰动一致；FIPS3向量与14边界输入重复摘要，查表+模拟和另一次实际net+模拟均正确（30banks/16hidden，7484观测network=table）。父合并后Mach-O两架构1819/1687状态全部观测/后继/动作同旧图。双槽6/6全rc0，29.02秒，日志/tmp/unisacc-parallel-thirteenth-gate，SHA与组合图证据/tmp/unisacc-sha-rules-928。仍保留格式字段枚举、实际常量读取、动态绑定与其他未迁控制；本批未重打包，候选仍d5dcda4。

第十四批并行决定：以0a774b3隔离，VLA完整分配/作用域恢复；未解析调用扫描及未声明printf回退完整组；x86共享REX/MODRM/ALU/字节/MEM过程和分支松弛完整组。只声明替代控制，动态表/模板保留源，过程共用不按调用复制。第二路独占gen2的必要installer接口行，其他路不得改gen2；x86不改共享parse/loader。每窗8分钟、子步60秒，完整图与真实网络/编码检查后父集中验证。

产品候选独立检查阻挡（d5dcda4，sha3ffcb9bd）：C99 57/57、CLI64/64、run12/12通过。父误把tools/difftest的UA参数当作切换入口；读脚本确认两者硬编码Python，队列这5项绿不算模型证据。随后对tools原11项目直接用模型候选-O2写原生镜像同cc比较，6接受却FAILED、5明确拒绝（二维数组参数/成员/typedef、void cast）。最小rot13来源已定位：合法char a[]={"abc"};经典私有UA和模型均sizeof a=1、a[0]为字符串地址低字节；cc正确sizeof4、97/98/99/0。未归罪声明迁移，参考实现本身也错，先修此真实产品差距，不扩大新迁移组。实际结果/tmp/unisacc-candidate-contract-928/tools-results.json。

tools门禁入口修正决定：保留默认Python裁判路径，新增显式TOOLS_UA可执行路径以实测模型/产品；报告实际driver，编译与运行各自bound60并比较退出码。避免UA被静默忽略后把Python绿误报产品绿；沿原11条固定清单，不增测试框架。

字符初始化修复并行收敛：C根数组修复d92477f、模型根数组796d87e及二维行768809c已合入。模型二维行复用STRINGINIT/INITADDR，Python零新增；实际ARCFOUR通过。C二维行待独立审查提出的整行范围检查补齐后合入，不以已有正例代替边界检查。下一并行只针对真实tools拒绝：void转换实现、二维参数/成员/typedef共用维度通路只读定位。tools入口两路实跑：默认Python11/11；旧模型候选0/11（6错误5拒绝），如实保留。TOOLS_UA已加入队列输入指纹，切换被测编译器不能复用原队列结果。

第十四批合并验收（4501650）：VLA、unresolved/printf回退、x86共享过程和松弛三组Python净减101，TSV新增386，总净增285；动态依赖保留，完整图扰动与实际网络/编码由各隔离组实测。父双槽4/4，24.46秒，/tmp/unisacc-parallel-fourteenth-gate。

实际工具兼容修复（080e638）：C根数组/二维行及边界检查、模型根数组/二维行、void转换已合入。void控制4条声明，复用已求值UNARY，无新Python语言分支；原regex2子代理实际net→后端native533字节同cc，regex1继续停在[][4]，不修改真实源。7fd1514候选6,017,681 B，sha256 13b2b3221ed3b5f046761518ec2a00af2a35e75c758a2a1fb16f29ee983abb7d，/tmp/unisacc-string-candidate-928，构造脚本分shared/routes/pack各55秒上限；实际TOOLS_UA候选11项目6通过0错误5拒绝，baseline11仍失败。经典C私有参考同套7通过2错误2拒绝（des/regex1错误，blowfish/tiny-aes拒绝），默认Python11/11；因此三条路线不能混记，shape修复必须依hostcc及Python正确参考核对。父080e638合并后队列6/6、18.28秒：chain实际net167/167，C99 57/57，CLI64、run12、parserbounds8、formatonce5，/tmp/unisacc-string-fix-gate-928。该队列使用新私有C参考；7fd1514候选尚不含void修复。

回归持久化决定：把brace_string/string_rows/void_cast三个自校验探针加入既有compilercheck资源模式的真实ASM网络路径，与既有decimal/math同跑hostcc、内存执行O0/O2和原生镜像；不另建框架。C新增b_string_rows含未定首维[][10]，实际候选仍明确拒绝，作为shape组剩余项保留，不把二维定长行修好外推到推断首维。

字符串修复后验收补充：Python参考9157c00补尾逗号及本地数组尾部zero（+6/-1）；新回归查出解释器鲜内存掩盖过该零填充差距，原生结果作判据。经典unisacc.com已重建1,366,672 B，sha256 6561d036cf4f2566b8a1e179bd23b6b3563ddf60a29ffb5b6d182632d5b152a8；--com全部13项双槽7.73秒全绿（不是整个默认gate），/tmp/unisacc-strings-com-gate-928。新版网络候选6,017,901 B、sha256 d8358e4b1b3f87a076715fead6c15b5216a3143fe82544ea6f9f93f630e06ae7，/tmp/unisacc-void-candidate-928含void；真实tools仍6/11，regex2已过解析但ARM拒绝，子代理定位lower ARG/TXT百万间距碰撞，正在修，不称regex2全链通过。父18次实际产物编译运行比较：classic9/9，模型root/void6/6，含未定首维的b_string_rows3次明确拒绝，/tmp/unisacc-string-accept-928/results.json。持久化资源回归已实跑通过：五probe含新root/定长rows/void经ASM网络内存O0/O2与native同hostcc；默认Python tools11/11。双槽2/2、38.06秒，/tmp/unisacc-string-resource-gate-928。shape和lower碰撞两隔离任务继续，不改产品默认到模型。

复合赋值参考缺陷实证及修正决定：char x=127; int y=(x+=1)在当前经典.com打印-128 128，hostcc为-128 -128，均rc0（/tmp/unisacc-compound-ref-928）。目标存储正确但表达式结果未按目标类型转换。模型共享OPX修复不可为错误参考删除目标窄化；父在C/Python参考复用现有窄化路径修正不同整数类型回写，保持同类型已有结果不重复转换。C抽共用signed窄化辅助替代cast内联，非扩第二套类型阶梯。

复合赋值参考修复实测：目标是否需额外signed窄化由实际运算RST与目标比较，CKT只选操作数转换（避免int<<unsignedlong的冗余窄化）。tests/c/b_compound_result.c覆盖char/short/int(long RHS)的存储值与表达式值，hostcc、C O0/O1/O2和Python原生输出全同，/tmp/unisacc-compound-ref-928/result.json。只读独立审查未发现新增错误；另证经典C位域compound仍返回截断前值（5位signed/unsigned加1：host -16 -16 0 0，旧C -16 16 0 32），本片未修，不把普通标量证明扩大到位域。

测试基础设施收口决定：核查nativeboot.sh仍有alarm300、未检查rc的进程替换cmp和Windows最长450秒轮询。隔离小任务将本地自举/跨写编译改成逐步≤60秒、先检查退出码再比较；Windows保持独立可验证入口，不将跳过算通过，不开新VM框架。主树仍在完成shape/compound/lower真实兼容组，不开启其它迁移功能。

复合赋值与lower合并验收（961c50c）：lower ARG/TXT百万间距改为独立2^40区域，相关索引A64；完整regex2网络链已由隔离任务实跑同hostcc，父三lower门禁3/3、6.59秒。compound共享OPX及ASSIGNCV替代旧整数阶梯，gen2净减13行；旧keep316项309不变、7必要窄化与修正参考全同，实际网络整数/指针/浮点三组同cc。父合并后双槽chain167/167与compiler资源2/2、37.27秒（/tmp/unisacc-compound-network-gate-928）。经典.com按e8d0041重建1,367,408 B，sha256 79b42f2fa1adf6fde62c179778e9e8e919c82eb97c4f1effdc1abfcbfa8d5f7d；13个--com项加difftest_o及nativeboot共15/15、14.55秒。上述不是新网络候选的tools11全绿：该候选仍待shape合并重建实测。

联合shape候选实测（9a38e3b）：17组180条固定shape控制迁TSV，共用DIM/PX/SH.WIDTH/SH.RESULT，compound结果保持同一形状；gen2净增36、TSV净增277、probe34，总增347，属于真实能力补齐而非代码缩减。父新候选6,019,097 B，sha256 d07fed1dd459731bd6b7a5a00e24aefa1f455de44b6890d639a3c1cc891199f0，/tmp/unisacc-shape-candidate-928；E3及六lower重建，其余模型源同7fd1514（git diff核对无变化），package5,835,785 B。实际TOOLS_UA原11项目全11通过、0wrong/unsupported/skip，首次闭合这批真实工具完整网络链路。并行resources包括新增array_shapes与compound两probe，经ASM网络内存O0/O2及native同host；队列2/3用41.11秒，rc75同队列续跑chain167/167用18.43秒，最终3/3（/tmp/unisacc-shape-acceptance-gate-928）。联合E3旧keep316/316实际net同修正参考，0diff/refuse/toolfail；两既有E2拒绝b_pp3/b_ppif用参考预处理供E1，明确不算E2覆盖。证据/tmp/unisacc-shape-keep-928。数组typedef附加suffix、T**、T*成员、T*函数返回、rank9仍拒绝；完整产品验收/默认切换未完成。

自举测试收尾（3203b8b）：本地N1=N2=N3及cross5/5主树复跑通过；正确字节但rc7、rc0空输出、无host均被故障控制拒绝。Windows改显式单目标入口，host55秒进程组/guest30秒编译及kill/exec和poll共享35秒，未启动VM，不声称远端验证。

实际模型候选扩验：C99 57/57、CLI64、run12通过；difftest_o整体在并行队列53秒触限rc142，未算通过。原脚本未支持分片、只在末尾汇总；按用户队列效率要求，下一小任务复用SHARD契约分四片，固定清单统一选择并保证各项恰好覆盖，保持三个优化级与cc比较；编译/执行逐步有界，先检查编译退出码，不增加超时。模型源冻结，本步只改测试调度。

真实tools11闭合后下一并行收口：保持候选与兼容清单，E3标量转换完整共享过程组、ARM基础编码/MOVIMM完整控制组分别迁既有声明；动态type/tyinfo/FPU/ENCSPEC/word序列保持真实来源与单份共享，删除替代Python分支，不扩语法与框架。各小任务8分钟，原图逐状态/后继/动作相等及实际网络验收；测试分片任务独占tests文件，父不在运行门禁期间改树。

联合候选六目标补验：9a38e3b网络包对hello在lnx/osx/win×arm64/x86_64生成的六份镜像，均与修正私有C参考逐字节相同，/tmp/unisacc-shape-candidate-928/six-target-acceptance.json。该项为交叉生成，不能代替六平台原生执行。

优化差分分片实际结果（ca2b2d0）：141原输入按36/35/35/35覆盖各一次，O0/O1/O2共423对；实际网络候选393agree、0wrong、30refuse（10源×3级）。拒绝清单b_decl2/b_decl3参数声明、b_pp2文件域复合字面量、b_compound/b_layout地址取值、b_ppif常量预处理、b_pp3粘贴、b_strsizeof/b_wide宽字符串、b_bits位域。四片19.03/17.15/37.34/39.78秒，双槽队列，不再整批超时；有拒绝的三片rc1保留，未减清单。参考run取raw wait区分正常exit142与信号；cc失败/信号/超时不缓存，bound.pl清理睡眠子孙经实测。父后续以该明确差距补模型声明，不把tools11或C99精选57外推为完整兼容。

转换/ARM基础控制收尾（4df5e53）：ASSIGNCV/NARU迁共享声明，已有TO/NARROW未重复；Python+4、声明+27。ARM mov/mul/ALU/ret/nop/callr/compare/MOVIMM迁声明，RRR及MOVK共用模板，Python−10、声明+21；两组合计Python−6，声明+48，总源+42。原图和动态依赖扰动相等；父主树实际资源、ARM、chain双槽3/3、38.65秒，/tmp/unisacc-conversion-armbase-gate-928。候选仍9a38e3b，未把未重打包版本误称新产物。

剩余兼容按共享过程并行补齐决定：一组处理b_decl2/b_decl3的参数/声明尾，复用PARAMS/FPDECL/shape；一组处理b_ppif的预处理常量表达式，直接补expression/reduce声明并复用既有宏展开；一组处理b_wide/b_strsizeof的宽字符字符串路径，复用共享字符串walk及初始化。隔离分支、各约8分钟，固定逻辑留TSV、Python仅绑定，不新增解释器/语言原语，不修改测试输入，不把未完成项改成knownpass。两个E3任务按参数区域与字符串区域分工，父审合后组合验证。

父确认测试分片未削弱经典基线：主树新difftest_o四片对当前私有经典UA全部423agree/0wrong/0refuse，双槽5.74秒（/tmp/unisacc-sharded-classic-gate-928）。模型候选的30refuse是实际兼容差距，并非测试分片引入。两者墙钟含不同编译路线及缓存，不作为正式性能比值。

参数/宽串联合收口（66709ab）：声明共享尾复用PARAMS/DIMS/SH.NEW，旧keep316/316；宽串共用SPANSTR/walk/ESC与宽度参数，UTF8严格解码，UCN仍拒绝。父重建E2/E3及located units/parse完整候选/tmp/unisacc-decl-wide-candidate-928，原b_decl2/b_decl3/b_wide/b_strsizeof在O0/O1/O2共12次实际六阶段网络运行与host stdout/stderr/rc全同。不是仅网络前端接Python后端。预处理signed字面量plain/located各28host对照及15明确拒绝父复跑通过，b_ppif仍拒绝0u，unsigned及函数宏下一共享组继续；位域只读核查另列，未复制参考已知复合结果问题。wide_strings持久探针纳入资源门禁。下一小窗口地址/复合字面量及位域先核共享过程设计，不另起解析器；原141文件×3级分片复验保持拒绝为失败。

66709ab完整候选复验：6,100,290 B，sha256 b66aaba72e2c3681faec4d04e3ac8d2e240e56fc1236eaf754122595b615133c。原141输入×O0/O1/O2四片双槽34.01秒完成，405agree/0wrong/18refuse，剩b_compound/b_layout/b_pp2/b_ppif/b_pp3/b_bits六文件；三失败片保持rc1，未称门禁全绿。联合resources（含wide_strings）与chain167/167双槽2/2、38.61秒。证据/tmp/unisacc-decl-wide-diffo-928与/tmp/unisacc-decl-wide-integration-928。3ebf70c随后补E2 uintmax类型栈及混合运算，父plain/located各67host选择与18拒绝通过，仍在函数宏M拒绝；尚未重新打包，不混入上述候选证据。将该独立literalcheck纳入队列，避免仅存临时验证。

按主人并发提醒继续按阶段域调度：parse2共享取址保持一人；预处理函数宏/placemarker当前小组交付后，原两代理分别转lower/code.py剩余ABI/SYSCALL整组与enc/arm.py剩余输入契约/数值格式化。动态gold/catalog依赖保持绑定，固定控制迁已有TSV并删除Python同义逻辑，不扩新框架；每组原图全域比较与实际网络门禁，私有树/UA隔离，父合并共同验收。

共享取址c9b75ab及placemarker6962325已合：ADR显式区分值/对象左值/函数/数组，替代U.amp及LP/PRE重复包装，Python净减25、声明增88；代理实际b_layout及副作用正例O0/O2四次同host，六非法取址/修改拒绝，keep316全同。父将副作用正例持久纳入资源门禁。##空参数先决定placemarker后查非空边界，Python0、声明净11；原b_pp3 E2net与私有参考完整相同、host token同，原88+4回归；非空通用标点paste仍拒绝。完整新候选待三阶段合并重验，不借用旧候选405结果。

位域实施契约决定：模型保持现有unisacc目标布局（字段间字节补齐），明确不把系统cc不同packing本身当语言错误，也不承诺系统C ABI互操作。算术/赋值表达式值按存入位宽转换必须正确，父实跑现.com得到16/-16、32/0、34/2，host为-16/-16、0/0、2/2，是真实参考缺陷；父修classic estore返回截断值并检查64位mask，E3并行复用共享位域描述/GET/PUT及OPX/ASSIGNCV，不复制旧错。普通成员不改变；位域取址/sizeof拒绝。原b_bits与持久表达式探针作为验收，不缩减剩余清单。

阶段域并行本轮：ARM输入契约408ac48 Python净减38/声明增86，x86操作数a438713 Python净减34/声明增90；标准及动态扰动四封装全图等价，真实net边界fixture通过。父ARM+chain双槽2/2、19.33秒；取址/宏/整数资源双槽3/3、38.66秒。静态compound7f6c7a8共享INITVALUE/AS.struct，Python净减7/声明净77；原b_pp2/b_compound与身份副作用probe O0/O2六次同host，keep316不变，新结构初始化不称tape字节同classic。E2函数宏2607607删旧专用XUM并复用EB/CF/HX，父联合hash复跑原b_ppif两格式完整tokens同host、67字面量/14宏选择/18拒绝及原macro88+4均通过。全部仍待新完整候选联合重包，不能沿用66709ab产物数字。

经典参考dd37da1位域结果修复：estore返回位宽转换值，63/64位mask使用无符号移位避免溢出。新tests/c/b_bitfield_result.c的assignment/compound/pre/post、signed/unsigned64与63，私有C及实际.com O0/O1/O2均同host。产品.com已重建1,367,696 B，sha256 a97674d56881c1ac1f9f66fd540bb9d4d1cc9e66d6b031f12e8596ec6a7b26a4；13个--com项+四difftest_o分片+nativeboot共18/18、双槽15.57秒，/tmp/unisacc-bitfield-product-gate-928；当前142源426对全同。Windows自举未测，无发布/推送；模型位域仍并行实施，未称默认已切换。

完整候选4d351c5联合收口：/tmp/unisacc-closure-candidate-928/unisacc-next.com，6,150,292 B，sha256 95900b79bad936f874edd6368066be352f2974b86d3e10757bd68a846e6722d2；六目标E2/lower/image及E3与located/units重新构造，E1/E4/O1/通用核源差为空才复用。142原始输入×三个优化级426对，420agree/0wrong/6refuse，仅b_bits与新增b_bitfield_result各三级拒绝；四片双槽36.99秒，失败仍记rc1。原141输入中已420/423，剩位域一文件，不能称全C99。真实TOOLS_UA原11工具再次11/11，资源含address_lvalue/compound_literals完整ASM网络均通过，双槽2/2、42.20秒。原b_decl2/b_decl3/b_compound/b_pp2/b_layout/b_ppif/b_pp3/b_strsizeof/b_wide全链已过。lower syscall67a57b0/1fa97b0及x86 operand主树四门禁4/4、18.72秒；lower Python净增25/声明27如实记录，不称压缩。全平台原生与默认切换仍未完成。

外部corpus验收入口校正决定：tests/corpus.sh当前固定Python，因此不可据其216基线宣称模型通过。增显式CORPUS_UA路径，保留默认Python与原220输入/216通过基线；编译/执行分别bound≤30、记录编译rc、非空产物，信号/超时不伪作knownfail；沿SHARD四片和双槽队列，不改样例、不降低基线。队列指纹纳入driver参数，实际候选与Python证据分开。

实际外部corpus候选4d351c5：220原输入四片双槽35.96秒，201通过/1错值/14拒绝/4原knownfail/0超时；四片rc1，原216基线不降低。00175浮点实参到char/int形参输出0而应99，优先E3共用参数转换；00215 ARM指令拒绝分给enc独立定位，其余拒绝按共用语法分组。证据/tmp/unisacc-model-corpus-928，CORPUS_UA显式为网络候选，不能用Python默认结果覆盖。E4 START/peep分派e5b54b1全图及动态扰动同、实际net fib O1/O2同，Python+9/TSV+9；lower、parse2、enc三域继续私有并行。

corpus三路线口径补全：默认Python四片216通过+4原knownfail、24.28秒全绿；私有经典C产物214通过/2wrong（00038 sizeof不带括号、00204结构参数/返回）+4knownfail、14.73秒，不能将Python216外推产品。实际模型201/1wrong/14拒绝如前。加强后空输入与“写出貌似正确产物再exit2”均被拒绝，/tmp/unisacc-corpus-fault-*。00215已由enc代理全链定位为E3/参考均输出重复函数内u_label，合法不同函数同名标签被混到全局；ARM拒绝正确，禁止放宽后端，后续前端统一函数域命名。x868156a9a删division/DONE固定Python净44，TSV增54，全图/动态扰动与实际原fixture均同，未称总源码减少。

函数标签命名空间修复决定：C/Python/E3统一使用u_<实际函数符号>.<源标签>，点分隔不可出现在C标识符中，函数符号保留多单元static后缀，源标签不经过全局static重命名。只改共享label/goto输出，不放宽后端重复定义检查；原corpus00215及独立双函数同名标签验证，再做原回归。父负责C/Python及持久探针，E3在位域交付后同步模板。

2286757标签修复验收：C/Python使用函数作用域前缀，原00215与新b_labelscope在host/私有C O0/O1/O2/Python行为一致，多单元static同名函数内同名标签亦通过。143差分源×3=429agree/0wrong/0refuse，自举本机通过，双槽5/5 17.68秒。经典.com重建1,368,528 B，sha256 3e2ac266b02a62a7b38620655871ed031eed8f450ec41d878bf0eb8867deac75；13项产物门禁全绿12.26秒，Windows自举仍未测，未发布/推送。模型E3模板须下一组同步，当前候选不借用经典证据。E4/x86联合实际门禁2/2 37.58秒；lower5485a95六目标扰动全图同和三目标NET通过，Python+6/TSV+15；x86剩余壳658054b正常/扰动四图同，Python+10/TSV+37，不声称总源码缩减。

位域27def9e与pragma50fb1e9联合验收决定：将4持久位域probe纳入实际ASM/network资源门禁，按host stdout及rc核对（兼容原空输出probe），pragma plain/located检查列独立队列项。重包候选包含六目标E2/lower/image、E3及位置信息模型，E4本轮声明变更也重建；仅未变E1/内核按源哈希复用。函数标签/实参转换由E3下一私有组处理，不覆盖掉本轮未修状态。

位域/pragma实际整包验收（候选5b018d9）：/tmp/unisacc-bitfield-candidate-928/unisacc-next.com，6,224,902 B，sha256 4b91962b837210ecffd98fa88fdbb3d13703ecba8169fb86504142bba82290eb。E3/E4/O1及六目标E2/lower/image重构，未变E1/汇编核差为空才复用；9原程序/持久probe×O0/O2实际完整网络编译并native运行18/18同host，含b_bits/b_bitfield_result、00128/00218/00206。首次host因旧b_bits未写stdio声明失败，测试环境补-include stdio.h后全重跑；源码未改。资源+pragma主树双槽2/2 48.12秒。位域保留byte-rounded目标布局，不宣称系统ABI；候选函数标签/实参转换仍待E3下一组，未称429/外部216全绿。后续memorylayout0f16928及address已合，全图动态源扰动同、实际NET平台布局边界通过；Python分别+9/-52，TSV+52/+128，总源码仍增，不冒称整体缩减。

前端实参/标签aef24b5合入：复用ASSIGNCV处理声明参数浮点与整数转换，函数标签统一函数域前缀，多单元prepass不再重命名原标签。旧316全过；00175/00215/持久probe实际NET O0/O2同host，多单元整tape同2286757参考。下一冻结候选必须同时重E3、units/located（包构造会重建），再跑原143×3及220corpus；拒绝仍失败。两个独立单文件probe进入资源清单，不将多单元证据冒称已有门禁覆盖。

队列耗时隔离决定：实际模型429差分全部通过（四片108/108/108/105，/tmp/unisacc-call-label-diffo2-928）；PAR1首次两片53秒超时如实失败，PAR2两轮完成。历史耗时目前仅按仓库路径，使classic约5秒污染model约30秒预测，窗口末尾两片错误启动后defer浪费时间。改为按执行配置（UA/TOOLS_UA/CORPUS_UA/PAR等）分离历史，不改55秒窗口/结果判据/原清单；实际候选固定路径不覆写。

实际候选e5c56cb：6,225,287 B，sha256 81c04285acba68ef72cbe902f1474d551e457995c7b167e279b615d6de2cf3a6；/tmp/unisacc-call-label-candidate-928。原143源×三个优化级429全部agree，0wrong/0refuse；PAR2四片32.52/36.25/29.62/29.79秒，窗口末预测错配两次defer后同队列续完，历史配置隔离已修。外部corpus双槽/PAR2在19.79秒四片完成：206通过、0wrong、10拒绝、4原knownfail，仍低于216基线并记rc1。剩00087/00089/00124/00130声明与函数指针、00170/00209枚举前声明、00174浮点一元、00144条件指针、00217 cast左值、00204 long double结构成员。00130后续共享括号声明已合但未计入旧候选通过数。三镜像固定收尾、lower终结、FP与ARM分派并行交付合入，六/十六完整图扰动和双架构550浮点实际NET均通过，最终整体候选还需重验。

新增实际MODEL_COM客机入口tests/modelcross.py：显式hash验证、无ua_ready/classic回退、按目标独立运行、不可用rc77不算pass。代理实跑候选81c04285…在已开的Lima Linux arm64，hello/fib/整数printf三程序的模型-run及客机编译后执行全部同hostcc，guest核对hash；不称自举/全部平台。Windows已查停机，接口尚未客机验，父强制Windows --probe单例以免三例串行越过60秒预算。Linux x86停机未测，最终候选需再次选择正确hash后验。

并行收口738a5ed：x86win完整固定控制迁24共享声明段，Python净减34、TSV增269；代理八种正常/动态扰动图全等，实际NET/SIM setup359B与11API 132指令2381B同参考，约束负例通过，未启动Windows。主树已合，最终整包仍待重验。另一独立线对固定81c04285候选实际编译asmcompiler.c得到N1，N1使用同候选抽出的compiler.pkg再次编译得到N2；完整Mach-O N1=N2，115746B，sha256 cebaed0a260bcffed8f61c6fa2f698ec5fadc87e1e4ba0d27d1c66ae55613f96。包6041975B，sha256 c7de2c66b3b933763601d53632a7c02f0d7b72eeb51edb5295baad4c875a61d4。N2空PATH、UNISA_KERNEL unset、显式同包，fib/printf转换O0/O2运行与native同host；缺包rc2。代理实际证据/tmp/unisacc-modelbootstrap-928；仅本机模型驱动器固定点与固定网络/内核包，不称六平台或APE容器自重建。parse2函数指针描述组仍在独立树修复，未合未完成路径。

下一独立验收决定：将已合192ca2a的local_parenthesized_declarators持久探针纳入真实ASM/network资源清单，与host输出对照；主树资源与x86win验收双槽并发，原清单不减。模型驱动器自举实验证据转为显式候选输入的可重复入口，仍由独立代理负责，不改变默认产品。

父主树a090c3a联合验收：exec-x86win与exec-driver-resources双槽2/2，3.46/50.39秒、墙钟50.55秒，日志/tmp/unisacc-xwin-resource-928；资源含新括号声明且实际ASM/network。固定完整候选81c04285另验：TOOLS_UA显式指向候选，11真实库程序11pass/0wrong/0unsupported/0skip，40.72秒；UA/UA_RUN同指候选，C99 57/57、CLI64/64双槽31.03秒，均rc0。分别/tmp/unisacc-call-label-tools-928、/tmp/unisacc-call-label-cli-c99-928。这些输入与原429差分、206/220 corpus互补，不外推未覆盖corpus或未交付FP修改；主树未在测试运行期间改动，产品默认仍经典。

本轮并行收口决定：enc最终来源审计已落exec/enc/CONTROL_AUDIT.md，承认字段模板调用边/初始化动作仍属Python装配，未称Python零控制。持久x86win门禁加入net构造、全域check-net与原全套夹具，父主树3a17d96复跑6.15秒全过。modelboot入口已合，父用固定81c04285候选独立prepare/bootstrap/probe均通过，N1=N2完整字节cebaed0a…13f96，后代空PATH的O0/O2内存/native同host且缺包拒绝；/tmp/unisacc-parent-modelboot-928。接下来构建脚本按shared/六独立target/pack提供分步入口以便双槽并发，不改默认产品、不另造调度框架；E3完整函数指针组继续，Windows真实候选验收独立执行，客机hash不符先停并诊断而非跳过核对。

Windows候选smoke首次在guest hash核对处失败，尚未执行模型，不归因编译器；独立6MB传输15秒超时。保留真实失败及日志，停止/关闭本次启动VM。父补入口清理范围：push与hash也必须在finally内，hash错误显示实际/期望字节及摘要，便于区分传输不完整；不放宽校验，不重算期望，不把未测计pass。

Windows传输定位进展：父1KB往返相同、6.2MB push12秒超时；gzip压至977980B，上传4.2秒，客机cmd.exe启动PowerShell解压后SHA256确为81c04285…，说明可保留原始候选校验而缩短传输。utmctl存在rc0但stderr报Error from event、exec早返回的行为，入口需明确识别并等待唯一结果。改为gzip运输、客机解压先核原hash再运行，cmd.exe启动脚本、非空结果轮询；不是修改模型包或跳过校验。

Windows父实际验收闭合（候选81c04285…固定不变）：传输用gzip977980B，客机解压后的原6,225,287B哈希先核对；原Start-Process返回空ExitCode及cmd引号问题使早两轮失败，改用项目既有Diagnostics.Process/cmd等待法，路径为自建无空格随机前缀，UTM rc0但Error from event明确拒绝。最终win/arm64及win/x86_64各hello/fib/convert六次独立≤55秒全部PASS；每次含APE x86_64运行-run以及模型编译指定目标后客机实际执行，与host结果相同。两槽并发，实际Windows11 ARM64环境，x86程序经仿真，不称x86实机或完整Windows自举。所有候选只读、未改包；父清理诊断临时文件后关闭本轮启动VM，status=stopped。

最终模型自举验收口径补齐：持久modelboot由N1=N2扩为原先约定N1=N2=N3，后代实测使用N3；三代均以同一NET包编译asmcompiler.c，完整文件比较不剥签。只是固定包驱动器自举，不扩大为模型/内核重新构造。新完整候选将用shared/六target/pack分步双槽构造，source identity必须一致。

函数签名cb4569b合入：共享描述池/参数转换/返回形状/结构兼容查询，旧keep316全同、5正例O0/O2共10次host同，4不兼容签名明确拒绝；Python净18、TSV净169、probe38，非总量缩减。function-signatures纳入持久实际资源检查。主树冻结此组构建新候选；下一私有并行按gen2区域严格分工：E3仅QT/S.star/LV，E4仅U.pos/TSPEC/type-follow，共享fixed控制写声明；剩enum扩展及00204聚合ABI不借首停点修复冒称全过。

父分阶段双槽完整构造候选a001adb（/tmp/unisacc-fp-candidate-928）：6,248,074B，sha256 6e60e3d1fa40880ce22ebb7c09a8b9c9ae605236f7b911c8ffa1d070e04abf1b，六目标与shared/pack全部rc0，每步≤55s；shared四模型及六target各三模型22项--check-net全域映射与actions/strings全同。实际候选143源×O0/O1/O2=429agree/0wrong/0refuse，双槽两窗口36.53+33.55秒，/tmp/unisacc-fp-diffo-928。corpus220原清单20.06秒：210pass/0wrong/6unsupported/4原knownfail/0slow，比旧候选增87/89/124/130四项；00144/170/174/204/209/217仍拒，门禁三片rc1保留，/tmp/unisacc-fp-corpus-928。后续U.pos/longdouble与QT/S.star私有改动未计入该成绩。

旧固定81c04285候选补平台实测：Linux x86_64 QEMU客机hello/fib/convert，模型-run和客机编译/执行均同host（16.87/10.00/10.19秒），未放大watchdog；仅本轮启动实例已关，default保持原Running。Rosetta osx/x86_64同三probe通过，arch强制Darwinx86_64，trace与候选offset138928解压切片/缓存driver逐字节一致、Mach-O CPU=x86_64，非arm交叉编译冒充x86执行。日志/tmp/unisacc-e5-linuxx86-928和/tmp/unisacc-e5-rosetta-928；这些旧候选平台smoke不外推新6e60e3d1，也不是完整六平台套件/自举。

并行收口续行：新6e60e3d1候选固定网络包驱动器三代N1=N2=N3完整Mach-O相同（115746B，cebaed0a260bcffed8f61c6fa2f698ec5fadc87e1e4ba0d27d1c66ae55613f96）；父N3后代probe已实际通过空PATH的O0/O2内存与native对host，缺包rc2，证据/tmp/unisacc-fp-modelboot-928，不扩大为网络/容器重构。聚合va_arg沿既有8B地址槽与ARGCOPY/LOADRAW复用，独立代理keep316及真实NET O0/O2正例通过；原00204继续暴露既有结构体char数组成员字符串初始化错值，明确保留为wrong，下一共享初始化组修复。枚举描述与测试资源/语言拆分在不同私有树并行，不改默认产品。

父合并cafb149/a039e40后双槽复验：exec-driver-resources rc0 37s、exec-driver-language rc0 44s，整体44s、2/2通过；21个真实ASM网络语言probe含scalar_prefix/conditional_deref及6个条件拒绝，原19项保留。vararg_aggregate独立实测已合，但本轮language清单尚未加入它，不冒称覆盖。core仍由独立代理按责任分组解决55s窗口超限；parse2成员字符串初始化与enum描述同时在互不重叠私有树推进，父主树测试期间未编辑。

联合诊断门禁发现旧测试过期：exec-errors仍要求L"ab"两例拒绝，但宽字符串已经迁移，实际rc0。因此保留原两例改为正例全结果对照，并用仍未覆盖的u字符串前缀验证失败定位不会递归重入；不把新支持当失败，也不删除诊断防循环义务。当前六项队列5通过/1失败，修正后须重跑errors与warnings变体。

诊断修正实际闭合：L宽串两例保留为无诊断接受；与旧C tape仅差字符串池末尾额外NUL（非错误状态/定位差异），未以全tape一致作该正例判据，宽串语义由既有真实language probe检验。u前缀仍明确rc1并正确定位，验证拒绝路径不递归。父errors/errors-warn双槽2/2、7秒；原17全结果比较保留。成员字符串初始化2436a5a复用IMEMBER/STRINGINIT.row/INITADDR，Python净0、声明净38，原00204及持久probe实际NET O0/O2全同host；core按modes/contracts/dependencies三职责拆分，原断言不减，下一联合验收保持主树冻结。

收口清单维护决定：exec/rules.md仍把4d351c5当当前状态并保留多批历史候选，容易误导剩余范围。父把它收敛为当前规则来源/真实候选证据/仍需完成的清单，历史数据以本prd保留；不改变S-17完成标准、不把当前通过数当全C99证明。

父联合核心分片实测：当前成员初始化合入树上core-modes与core-contracts双槽各47秒、整体47秒全通过；前者含网络构建驱动器与27模式/优化级对照，后者保留compat/stdin/IO/未定义函数检查。language新增vararg/member两个持久probe，严格拒绝要求rc1与诊断，不将信号或超时算合法拒绝；三core+resources+language清单与旧all保持覆盖关系。

父后续联合分片实测：core-dependencies 26秒、language 46秒双槽2/2通过；language已含23正例（新增聚合va_arg与成员字符串）及6严格拒绝。独立产品接入审计发现发布入口仍造classic、测试包与最终保存包未统一hash；完成前仍不切默认。先补现有gatequeue对显式外部候选/可执行文件的内容指纹，防同路径覆写复用旧结果，不新建调度框架；seed与被测产品须分开，TOOLS_UA/CORPUS_UA不能由UA默认为已选择。

实际产品门禁接入决定：保留组件门禁UA为私有参考seed，gate --com允许显式MODEL_COM且默认仍根unisacc.com；com任务同时设置UA/UA_RUN，tools/corpus用独立选择变量，增加原有C99/工具11/外部corpus四片/优化差分四片的com命名入口。执行前后核同一候选哈希，不在验收末尾重建替换；不切默认、不发布。队列外部内容指纹由独立代理同步补齐。

枚举完整描述48d10d3合入：身份/完成/作用域、尺寸/读写/位域/调用共享查询，314固定NET tape全同，00170与跨unit同名tag实际native同host，14类型负例及59/60容量边界实测。父收紧host拒绝检查为rc1（不收信号/超时），合法enum_forward注册language；00209仍函数指针数组typedef拒绝，由独立组继续。MODEL_COM显式门禁已实跑新6e60e3d1旧候选cli64/run12双槽11秒，报告候选hash；这不把刚合enum算入旧包。现冻结主树构建包含全部已合兼容修复的新候选。

新完整候选e4d1a8f已分阶段双槽构建：/tmp/unisacc-enum-candidate-928/unisacc-next.com，6,275,043B，sha256 4d288bab4610416e4d0e942589cf6df3211ae59ea47adf6d696a59ad172fc29b，package6,091,731B；22个shared/target网络全域--check-net通过。显式MODEL_COM门禁实际corpus220原清单23.21秒：215pass/0wrong/1unsupported（00209）/4原knownfail/0slow，失败仍rc1；143源×三个优化级429全部agree，双槽两个窗口37.06+31.85秒，0wrong/refuse。queue内容指纹父infra实跑通过，记录/tmp/unisacc-enum-corpus-928与/tmp/unisacc-enum-diffo-928。后续printf控制迁移尚不计入此候选，默认仍未切换。

printf/POOL完整规则迁移a20bfdc：五模式完整state/观察/后继/展开动作及字符串全同，动态布局/HEX/模板扰动全同，旧keep316/316与4probe O0/O2实际native同host；Python净减63、声明增232，总源仍增169。父审后合入，保留声明绑定/共享字符串walk/模板，不把格式控制重写进执行器。

父合并7d4bb90后printf警告联合门禁双槽2/2、15秒：原39例完整tape/诊断对照保持。CALL整组固定控制迁声明与00209共享声明形状继续分别在隔离树推进；前者保留EN.VALUE、ARGCOPY、addr和动态SYSCALLS，后者保留FS签名，不重复解析器。

并行收口本轮：三私有域同时完成CALL控制、FN声明控制、addr/fmtwalk共享helper声明化，父冻结主树验固定4d288bab候选。15产品项全部执行，13过/2失败：staticinit实际rc1正确拒绝但文案不同；parserbounds模型接受257 case而测试硬编码经典256数组容量，尚未据此证明错编。保留原失败记录/tmp/unisacc-enum-product-928；测试按原安全义务改为超旧容量时“明确容量拒绝，或实际运行与host一致”，不人为限制模型，也不只凭接受即通过；自动对象静态初始化仍必须rc1和明确诊断。closure48镜像/8运行及tools11全过。随后统一合入6574a08回调数组、1c71ce0函数声明与601bf9d调用控制，注册持久回调probe，再做联合验证；候选215/220的旧记录不会自动改为216。

上述两项契约复核完成：classic与固定4d288bab模型共4套件单槽5.43秒全过。模型8个边界输入全部实际-run同host（1000 case逐项求和并测default、9000字节printf完整输出、4097字符数组尺寸/全内容checksum/尾NUL），classic原越界输入仍按预期rc1容量诊断拒绝。staticinit两路线各9个rc1拒绝，兼容各自明确文案；没有通过放宽退出码消除失败。6574a08已合1f2b91d并将真实回调数组probe注册language，FN/CALL声明合4e2f39c/456b4e7；第三helper组仍私有验证中。

三私有组统一合入70d3952：CALL/FN/addr-fmtwalk固定Python分别净减64/75/2，声明增加379/314/32，总源码不是下降；回调数组兼容修复另计。父联合language25正例与6拒绝、E3自身源码3990936B table tape同参考，双槽51.50秒通过。共享+六目标分别两槽构建、pack最后执行，新完整候选源70d395240e94630f9ee01e9f8c138fb30f30da04：/tmp/unisacc-controls-candidate-928/unisacc-next.com，6278823B，sha256 a1115126d8281c93a1b38c3c3fc491779994201d14cf60cd2005933faa19ffb0，包6095511B；22 shared/target全域network/table检查通过。新候选原220 corpus四片双槽21.86秒：216pass/0wrong/0unsupported/4原knownfail/0slow，最后00209闭合，基线不减。新候选其余优化/平台/固定点证据仍需自身验证，未将旧包测试冒用；默认产物未替换、未发布。

后续并行分工决定：E3完整一元/cast/标识符表达式控制、E4全局声明/初始化控制、E5宽度/类型及ELSZ控制分属私有域；不新增语言，保持完整图与真实共享依赖。父对固定a1115126候选补优化差分/产品契约及平台验收，源码写入与主树测试不重叠，重型任务全机双槽排队，结果不跨候选转用。

当前a1115126候选优化429全部agree；固定包模型驱动N1=N2=N3完整115746B（cebaed0a…13f96），N3空PATH原生/内存O0/O2同host且缺包rc2；Linux两ISA及Rosetta x86各3实际probe通过，Windows验证继续。第二批2738a3f/1737c4a/f3ba57c已合，language25+6主树52.75秒通过，逼近55秒窗口。因此保持language/all聚合接口，门禁把同一25探针清单按固定奇偶位置拆language-1/2，6拒绝只在第一片，各项恰好一次，不增加缓存/框架。父做这个队列拆分；第三轮两个私有域分别MEMB/POSTIX/赋值与局部初始化，第三代理只读列实际剩余控制，防止漏掉import辅助模块。

只读6a208f8来源审计补齐到exec/rules.md固定清单：普通truth/boolean/floatconst以及产品实际装入的诊断、位置、警告、unit framing辅助过程仍有手写固定控制，不能因gen2壳变短漏报。已有动态gold/类型/模板装配不重迁。E5下一独立组为truth+booleans+TO标量转换，E3/E4保持成员与局部初始化域；逐完整过程迁，非导出整图冒充规则源。

固定a1115126候选跨平台smoke已补齐：Linux arm64/x86_64、macOS Rosetta x86_64各hello/fib/convert，模型-run及编译后运行均同host；Windows ARM64客机上x86 APE驱动的-run与分别win/arm64、win/x86_64编译/native六项通过。macOS arm64模型自举及后代probe如前。只称所列smoke，不称客机全套；本次启动的Linux x86/Windows已停，原有default Lima保持运行。实际包账/tmp/unisacc-controls-candidate-928/ledger.json：32个互异N模型5922578B，938stage rows，资源119004B（含7704/7664B两ISA通用核），包6095511B，整个6278823B；驱动/OS/库不能算作几KB核。

语言队列拆片实跑：13正例+6拒绝37.88秒，12正例29.25秒，两片全部rc0；同一25项清单不重不漏。此次父单槽与代理构图单槽并行，不宣称这两个时长为双槽总耗时；各片比原52.75秒有余量。第三批局部声明dea5899和成员/后缀/赋值656ba4b已合，各全图/扰动同及真实NET分别5probe/12运行对host同；标量真假/TO仍独立进行。下一两个互斥完整组为floatconst常量扫描/limb/舍入和diagnostics位置呈现，保留已有共享输入与普通/告警模式接口，不添加语言。

并行调度续行：标量真假/TO组5a1258c合3520fbb，五模式及动态类型/fpu扰动10全图相同，5现有probe真实六阶段NET O2原生与host的stdout/rc一致；Python净+22、TSV+48，如实计为规则源迁移而非缩码。E3继续diagnostics完整组、E4继续floatconst完整组，E5下一独立组为gen2的U.szof/SZ完整sizeof控制；三私有文件域并行静态修改，重型构图/测试全机两槽，E5待任一槽释放才执行。父只集成与固定候选验证，不向这三域插入编辑。

父验收收口决定：buildcompiler的22个shared/target全域检查未包含compilerpack临时产生的token/warning/error/unit模型，不能由22项推称整包32项全证。为最终包保留临时构造的tbl/net配对及SHA清单（仅离线审计目录，不进入运行时），沿用run --check-net逐项检验；不加新模型或新的通用框架，不改变现有compiler_package调用接口默认行为。最终证据按实际互异网络/产物hash核对。

诊断呈现完整组c7ed7a2已合：五模式/导入布局扰动全图一致，11场景×error/warning两模式实际NET与instrumented C renderer完整stdout/stderr/数量22/22同；Python净减38，声明95行。E3下一独立交付为四个warning helper（return/int/unused/format）的完整固定控制，复用同一finite_rules入口与诊断调用接口；不改消息语义、抑制规则或产品支持域。E4 floatconst和E5 sizeof仍各占一个重型槽，新warning组先静态修改后排队验证。

unit位置协议收口按完整过程分工：E4先迁unitlocations.py完整LS适配器，保留tokenlocations.install以及units.py的唯一扫描器和既有标签规则；随后迁完整DL协议读取组和units隔离组，不混改三个语义层。当前E3 warning、E5 sizeof私有验证，父主树产品队列窗口之间只记本决策；不修改队列指纹输入，私有修改继续并行。

sizeof组0999ad5已交待父队列冻结结束后合；全图10组同、6旧probe真实NET O2原生同host，Python净减77/声明增307。E5下一独立整组为return语句及CEXPR/EXPR/LP/QTAIL条件表达式完整控制，保留ASSIGNCV/FS/CKT等已有类型事实和QN空指针证明、共享LP.addr，不迁X.id/更新操作相邻组。先静态工作，等待全机两槽空位，不拆成逐例新增规则。

父固定a1115126候选15项产品队列全部rc0（/tmp/unisacc-controls-product-928），包括C99 57/57、CLI64、ccparity53+known1、closure48镜像同/8运行、diag14、diagunits18、formatonce5、hostile21、multi、parserbounds8、run12、staticinit9拒绝、staticunits、tagforward15拒绝、tools11。单槽与代理重型验证并行，6窗口32.84/18.15/16.94/15.54/14.83/48.29秒；无超时/失败，最后rc0。此证据仅该固定候选；队列结束后才批合d96bf3a浮点常量与0999ad5 sizeof，未称新源已重建为同一包。E4 unitlocations接槽1，E3 warnings槽2，E5 return/表达式静态处理中。

unitlocations 7a9c3bd已合：普通/located与布局扰动全图相同，原实际NET序列化/UTF8/诊断及17坏帧检查通过。下一E4完整tokenlocations DL读取组，保留MAP_FIELDS顺序与调用者ready/ordinal/token_record/multi接口。父独立处理parse/gen.py tokenizer的终端/qualifier/default策略声明化，prefix trie仍由动态WORDS/TK/qualifiers装配；仅此文件域，不动其他代理模块，先对tokenizer完整有限观察表比对，不运行重型构图抢槽。

父tokenizer策略已迁token-policy.tsv与token-prefixes.tsv，WORDS/TK/qualifiers仍动态构造前缀trie。原普通完整240状态/61680观察、新增词表和qualifier及token编号扰动250状态/64250观察，旧新每个后继/展开动作完全相同，两次各约0.10秒；/tmp/unisacc-token-policy-928。只验共享reader有限图，未外推完整编译；Python加入通用规则绑定而非新词法分支，末尾/qualifier/word/span策略由声明提供。

固定a1115126候选补单份包实证：原6095511B compiler.pkg在6278823B容器中完整出现且只出现一次，offset183296，包SHA2198fbeb6687df6914e5622044750ccd286828b63dae33bf0bdbfe323120a376；/tmp/unisacc-controls-candidate-928/package-occurrence.json。这是物理包计数，模型32互异及resource另见ledger，不把驱动库算几KB。warning组全图与原139用例已交待审；E3下一完整errors.py映射/恢复/计数/summary控制，保留动态拒绝源扫描与通用input-frame深度跟踪，不把对应语义埋进通用loader。E4协议读取槽2、E5返回表达式槽1，新组先静态。

warning四helper5650179与返回/表达式7436d7b已审合。warning原139例完整NET tape/诊断全同；返回组五模式+动态10图同，6旧probe完整NET O2/native同host。父下一单槽联合验证以70d3952完整源树为基线，当前全部已合规则迁移后五模式完整展开图对照，分五次有界执行；只证明联合转移相等，不假定表/网络序列编号或容器字节不变。新控制剩X.id/更新/普通解引用、START/unit协调及errors/位置协议/units。

E5下一独立完整组为X.id及标识符查找/自增自减/compound更新、TAX/CSTEP/STEPTY/INTONLY/FPSTEP等与之相连控制，保留真实type/tyinfo、形状描述与共享AS/POSTIX/CALL。剩普通U.str/UNARY/deref及START另计；不把三者混成逐case扩功能。父五模式联合检查单槽执行，E4协议槽2；新组先静态，等空槽。

父联合源迁移验证完成：固定完整基线70d3952对当前2d27cbf，plain/locations/warnings/errors/both五个模式6006/6194/6433/6357/6593状态，每个观察后继及展开action/string与顶层metadata全等；逐模式10.15/10.30/10.65/13.08/13.42秒，各步≤55，源码全程冻结。/tmp/unisacc-joint-controls-928/summary.json及逐模式完整比较文件；涵盖目前已合全部局部迁移和共享token策略，仍不声称packed bytes相同。槽1已交errors，槽2协议结束交identifier/update。

位置协议25ec7ad已合：五E3模式/units两配置及8种接口组合、布局/字段顺序扰动全图同；原unitlocationcheck与locationcheck实际NET序列化/map/tape/诊断/坏帧全通过，Python净减44、声明139。E4下一完整units.py分帧/扫描/文件static隔离组；保留已声明unit-labels、位置信封DL/LS、动态词表和唯一tokenizer，声明builtin选择来源，不能将扫描算法搬进新的Python模块。父现有五模式联合验证不包含此后合入，后面统一最终源再验。

父本轮独立迁gen2启动/unit-marker控制：固定marker识别、输入域守卫、寄存器/名字初始化与AUTO/INDEX/NEXT调用顺序入声明；TYROW/AX/OPS及SYSCALLS/autonames/HEADER保持实际动态来源。用既有structured_control/load_rules装配，不引入运行原语；与E5的X.id更新区域互不交叉，私有代理不读父工作改动。完成后等槽做完整图及现有多文件/函数名字探针，不能只验语法。

并行收口续行：identifier/update 19edc71、errors e43c525、units e1d712a 已交付待父集成。E3下一完整普通U.str/UNARY/deref/S.star/NOARR控制组，明确不碰已由E5交付的U.pinc/U.pdec与START；E4/E5分别只读审计lower/opt及enc/ARM的实际剩余手写规则与既有声明绑定，若无缺口即交清单，不扩语言/框架。父完成startup真实NET检查后批合集成；重型验证两槽，独立私有树静态工作并行。

startup组验收：五模式全展开图与70d3952基线一致；动态POSSPAN/CKT/SYSCALLS/HEADER/autonames/TYROW联合扰动与迁前图一致且异于基线。真实六阶段NET O2→osx/arm64运行hello/funcname/s77_fpvar三例stdout/rc同host；首跑host因hello无stdio声明失败，裁判显式预包含stdio后重跑通过，s9自定义va_list不作host对照，改用已有s77_fpvar。证据/tmp/unisacc-startup-control-928；gen2 Python净减3行，声明增加另计，不称总码量减少。

四组已合7ebfe09/114444d/b8edf74/ff5e0b5。父联合验证ff5e0b5的plain/errors/both/locations/warnings完整展开图均与70d3952全等，各6.45/8.26/8.50/6.59/6.75秒，分两次≤55秒；证据/tmp/unisacc-joint-finalcontrols-928。这包含errors/update与startup组合，units另有普通/located全图及NET证据，不把五模式检查误称全部包已重建。E3普通解引用组私有并行；E4 lower/opt只读审计未发现新增完整待迁控制，但保留ABI/目标模板选择、事实物化、格式绑定，不宣称Python纯通用loader；报告/tmp/unisacc-lower-opt-source-audit-928.md。默认产品未切换、无推送。

最终产品入口收口决定：release.sh旧路径会先验根.com再另造classic包且SUITES=0仍称ready，不满足同一产物验收。独立修改为显式不可变候选与有界队列/证据，未完成队列返回pending，验收前后hash一致，只复制已验候选；不自动发布、不把skip算pass。Makefile模型构建需复用现有shared/六target/pack步骤和双槽队列，保留classic种子入口，默认切换待最终候选全验收。两域私有并行实现，先交最小可审版本；不新增编译器或调度框架。

固定a1115126候选性能/容量实测：同fib源/同osx-arm64/O2/新进程到镜像，5次中位数classic 0.0613s、model 0.6430s（10.48倍），镜像hash相同；含加载不含产物执行。编译unisacc.c默认在11.02s以rc3 timeout（2e8步燃料）拒绝；使用现有自编译测试的UNISA_MAXSTEPS=400000000000后19.37s成功，外45s未触发。这是产品默认燃料与既有selfcheck契约不一致，不能隐藏为bench跳过。修正仅产品驱动默认与已有selfcheck燃料一致，显式环境上限仍生效，独立通用测试runner默认不变；真实后续产物重新验证，性能如实列风险。首轮9s探针外限过窄已失败，不记通过。证据/tmp/unisacc-final-perf-928。

普通unary/deref 289cfe4合b128044，含ID.inc/dec共享模板及staticauto；三处冲突为声明append与SS/FNVAL相邻组，保留两组新实现。父在11a0d56复验五模式全图全部与70d3952基线同（6.41/8.29/8.52/6.84/6.90s），/tmp/unisacc-complete-controls-928；原实际NET 1549292观察全等和五probe O0/O2十运行见代理证据。exec/rules.md同步已声明全组及保留的生成期绑定，不把旧候选当新产物。

最终parser审计11a0d56未发现完整固定过程遗漏；仅strings.py普通escape的保留字符策略/ADV-LDI模板仍明文分支。为避免声明边界含糊，最后小修将此策略及共享escape模板纳入现有strings声明，esc真实映射继续动态绑定；ADR.object单边连接保留为共享过程装配，不另迁。产品燃料7f528cd已验证私有driver默认self 20.57s同前hash、显式预算1拒绝，现合入待新包实测。

最终源冻结准备：escape小策略88e4b46、显式构建9e33bdb与不可变候选验收fbf00d0/72e421e已合。make release去除ref副作用；每次队列55秒、双槽，pending75可续，同hash通过才复制，零skip不误拒，不称本机gate等于跨平台发布。默认com仍classic，冻结新包后以实际artifact完成全部门禁/平台/性能，再决定切换。

冻结源85eaeb9完整新模型候选已建：/tmp/unisacc-final-build-928/candidate/unisacc-next.com，6279167B，sha256 9a0ae470718ea4db28a59ea838344004c741ea0119c771c1370454209bdecd46；包6095823B/a1f364e119ea1be07cd3c8fa2ee9b9fe7c06be89e05ddbe362e6fa52c53332a8，物理整包仅1份(offset183328)。shared与六目标两槽批次7.98/4.13/5.14/4.86s，pack25.20s全rc0。审计保留32个互异网络及32表配对，逐一SHA核对与--check-net全域通过，配对网络集合恰等包内32模型（938stage rows/21资源，网络5922890B）；不是只验原22项。证据source.json/coverage.json/pairs-{0,1}.json。

同一9a0ae470候选全139项本机滚动门禁已启动：/tmp/unisacc-final-gate-928；首窗口39.44秒，tools11/11与bigclosure6目标同均rc0，2/139，pending75正确保留，尚未称全套通过。显式MODEL_COM、UA=/tmp/unisacc-final-build-928/seed，默认产物未替换；后续只改prd证据不更改冻结构建输入。

最终候选冻结队列续行：47/139已执行，44项rc0；exec-bindx86、exec-container、exec-warningdriver在双槽48秒任务预算下rc142，失败原日志保留，不当通过。独立同候选exec-bindx86单槽53秒预算实测38.04秒通过（/tmp/unisacc-final-retry-bindx86-928）。下一步重套件独占、短套件双槽；同源同候选队列重试前将原失败结果与日志归档为attempts，再实际重跑，不伪造通过。containercheck发现仍重建另一包，独立修为显式MODEL_COM直接验证候选；主树尚未合入，不将草案检查计入冻结队列。

容器门禁独占53.06秒仍超时，未得到运行检查结果。合入e74c677最小修复：显式MODEL_COM时不另造包，直接验证候选内P2目录/唯一网络/双核及六目标输出、本机双ISA内存与native运行；无候选保留原开发构建路径。测试源码变化，新建最终队列，旧45项通过和各超时留存，不移植为新队列通过。产品与候选9a0ae470未改。

最终9a0ae470容器直验（22ce210）实跑通过：六目标完整镜像同参考，arm64/x86_64实际-run及native输出同host，32互异网络/两核/单包，无外部模型核。固定最终包模型驱动器N1=N2=N3完整Mach-O 115746B，sha256 6e31c9664f4274f9bac02f46c9b773db7b5e53e0200725985c6b3b4b866dde03；N3空PATH、O0/O2内存/native同host、缺包rc2，证据/tmp/unisacc-final-modelboot-928。不是整个网络包或APE自构造。Linux arm64在既有default Lima实跑hello/fib/convert：候选哈希先核对，-run及指定目标编译后native全同host；仅三程序smoke，不称整平台套件全绿。

基于实测超时修正现有队列调度：7d9f2fb增加显式exclusive-suite，bindx86与warningdriver独占，普通套件仍双槽；独占清单纳入state，变更拒绝复用，窗口仍50/55秒。queuecheck/releasecheck替身已验证互斥、普通重叠和失败传播；合入后重启新冻结队列，不重建未改变的9a0ae470候选。

最终9a0ae470跨目标smoke补齐：lnx/x86_64经Lima/QEMU仿真、win/arm64与win/x86_64经Windows11 ARM64 UTM，各hello/fib/convert全rc0；每次先核候选完整hash，APE-run与指定目标编译后native输出均同host。Windows容器运行x86_64驱动（OS仿真），不称x86实机。9单probe日志与guest-results.json在/tmp/unisacc-final-platform-928，另Linux arm64三probe已录。两台本轮新启VM已关闭，既有default Lima保持；容器本机双ISA运行和固定NET包三代自举另录。新完整门禁状态目录/tmp/unisacc-final-gate-v2-928，重套件独占、其余双槽；只继承调度耗时估计，不继承通过结果。

V2冷缓存bindx86在48秒仍超时，归档后同源55秒窗口独占实跑38.52秒通过，窗口填充后6项全绿。warningdriver独占53.09秒仍超时，只完成cc分支；判定套件本身过长，按原cc/ua/asm三个驱动划分门禁，各保留完整比较项，仅构建选中驱动，不增加超时上限。修复完成前暂停全队列，不继续用超时堆积结果；最终产物和平台证据不变。

警告驱动分片16bb21d已合d624763：cc20、asm20、ua Wall5/Wextra5/Werror10，合计原60个完整rc/stdout/stderr比较及输出屏障全部保留；仅构建选定驱动，ua三片独占。代理私有冷cache最慢ua-Werror实跑46.79s全10项通过，首次未拆ua55s超时保留。完整门禁现143项，重开最终冻结V3队列；对耗时明确区分冷构造与运行，先用现有models.py内容哈希缓存有界准备重套件模型，不缓存测试结果。

最终9a0ae470同口径性能完成：同源、O2、osx/arm64、新/bin/sh APE进程到完整镜像（含watchdog/加载、不运行镜像），无其他重测试、各5次、默认UNISA_MAXSTEPS。fib经典0.06078575s/模型0.62056504s中位，10.209倍；unisacc.c经典0.77623196s/模型19.42672129s，25.027倍。每源全部十个完整镜像hash相同，fib5bda314a…70ec6/self474cdb14…a52146。证据/tmp/unisacc-final-perf-plan-928/summary.json和20单样本；这是当前成本，不是优化收益，按原决定功能验收先行，速度另行优化。

最终门禁 V3 的 `exec-multi-ua` 在单槽、53 秒上限下仍超时；日志已完成第一组双顺序 O0/O1/O2 比较，剩余检查没有回执。`exec-memory-ua` 在双槽并行时也到 53 秒，私有树单独量测约 45 秒可通过，但裕量不足。决定将多单元与内存检查按既有清单拆成独立有界子批：每个原检查恰好由一个子批执行、各子批有实际回执；普通测试仍由队列双槽并行，重负载按耗时独占。允许复用按输入身份校验的模型准备产物，不复用测试结果。拆分可能增加冷准备总耗时，合并后以实测墙钟和每批 <60 秒共同验收。修改测试代码会改变 V3 内容指纹，因此归档原尝试并从新队列重新执行全部检查，不移植旧通过结果；9a0ae470 候选本身未改。

V4 新输入指纹的完整 `--com` 队列 171/171 项全部实际执行并以 0 结束，0 失败/0 待执行；状态 `/tmp/unisacc-final-gate-v4-928`，候选 SHA 9a0ae470…decd46。原多单元 UA 四个 tape 子片双槽约 31–32 秒，内存 UA 的 arm 三片约 34–35 秒、x86 三片约 36–38 秒；分片保留原全部检查。`tests/release.sh` 汇总仍返回 1：其通用 skip 正则把 `run`/`com-run` 的“Windows 未在本机执行”和 `nativeboot` 的“Windows 自编译未做”范围说明当成了本地检查跳过，尽管本机 `run` 明确计数 skipped 0。此审计失败不记通过；修为仅识别确切的已登记范围说明，其他 skip 继续拒绝，并在最终报告保留 Windows 自编译未验。随后将已验9a0候选接入默认构建，冻结最终树并重新执行同一完整队列及汇总；不把 V4 旧结果移植为最终树通过。

本地默认入口已按已验候选切换：`make com UA=/tmp/unisacc-final-build-928/seed MODEL_DIR=/tmp/unisacc-final-default-build-928` 在独立 60 秒上限内完成两槽 shared/六目标/pack 构造，根 `unisacc.com`、本次新构建和 V4 候选三者逐字节相同，均 6,279,167 B、SHA-256 `9a0ae470718ea4db28a59ea838344004c741ea0119c771c1370454209bdecd46`。经典入口改为显式 `make classic-com`，输出 `out/unisacc-classic.com`；已发布 v0.0.7 仍是经典版。release 汇总的定向范围修正需合入，随后最终树重新执行全部门禁；此处不称发布完成。


S-17 本地重构收口（2026-09-28）：冻结代码树 `1ccb16e2bf7577ee24019e53c33ff02305474c6c` 的 V5 完整队列 171/171 实际通过，0 失败/0 待执行；最终 `release.sh` 本地汇总 rc=0。`exec-bindx86` 首次 48 秒预算超时 rc142 已归档，未计通过；同源同候选独占 55 秒窗口重跑 37.98 秒 rc0，原失败与重试身份保留。最终核对根 `.com`、私有候选及默认构建三者 SHA 均为 `9a0ae470718ea4db28a59ea838344004c741ea0119c771c1370454209bdecd46`，6,279,167 B。完整结果、32 个实际网络配对全域检查、包账、固定包驱动三代自举、客机 smoke 和五次中位数性能已持久收录 `research/s17-final-evidence.json`，不再只依赖 /tmp 日志。

本轮到此停止主动扩展：默认 `make com` 已构建模型路线，运行不启动 Python，经典路线是显式回退。仍保留离线 Python 生成期绑定和模板；不是完整 C99 证明，不是整个模型包自构造。Windows 自编译未验，客机 smoke 不替代完整平台套件；fib/自编译延时为经典路线的 10.209/25.027 倍，尚未优化。已发布 v0.0.7 不变，无新 tag、上传或推送。最终验收后的修改仅为文档和证据，不把文档提交伪称为重新执行过全部门禁的代码树。

parse2 FOPS 表格化（本session直改主树，非cdx，2026-09-28）：`exec/parse2/gen2.py` module-level 硬编码字典 `FOPS`（算术运算符→浮点指令后缀，10项）改为 `exec/parse2/operator-float.tsv`，由 `optail()` 通过既有 `tape_rows()` 读取，与文件里其余同类表一致。仅此一处；未触碰 optail()/tytail() 等真正的手写编排逻辑（那部分是按 research/e3-structured.md 已承认的"未实现设计"，本次不动）。验证：改动前后 `python3 exec/parse2/gen2.py OUT.json` 产出 sha256 完全一致（5baca634e4354f2e9519a3f174e5a1946d2cbcd1d87e5b14083ba81e54340aca），states/entries/action seqs/json字节数均未变。这是一次极小、低风险的示范性改动，不代表 gen2.py 整体已声明化；887行里绝大部分仍是围绕 ~40 个 install_rules()/structured_control() 调用的编排脚本，消除编排本身等价于写一个真正的语法编译器，仍是未完成的目标。


<a id="model-function-bytes"></a>
### 模型功能与物理字节账（2026-09-28）

用户校准：近期优先保证功能与性能，通过实际产物测试持续约束；体积暂为观察指标，不为缩小数字删除功能、诊断或测试。下面以 `research/model-bytes.json` 记录的实际 `.com` 为快照（源提交与产物SHA在账本内），不把源码行数、解压后大小或目录引用次数当成磁盘字节。

<!-- model-bytes:begin -->
快照 SHA-256：`01e5c1d9d4528a2402d212883da0df64f63d60b8f17b5e6461d18e41ed053631`；总计 **1,083,311 B**。

| 物理内容 | 字节 | 占整个 .com |
|---|---:|---:|
| 32 个共享网络体 | 671,895 | 62.02% |
| 平台驱动、APE 启动/加载与对齐（混合账） | 230,016 | 21.23% |
| 20 份 C 头文件/库实现源码 | 111,321 | 10.28% |
| 两 ISA 通用推理执行核资源 | 15,520 | 1.43% |
| 目录、记录头与资源键 | 54,543 | 5.03% |
| 尾部 | 16 | 0.00% |

| 模型阶段 / 具体功能 | 物理模型数 | 模型体 B | 占 .com | 阶段行引用数 |
|---|---:|---:|---:|---:|
| `e2`：预处理、目标预定义宏与位置模式 | 12 | 166,187 | 15.34% | 126 |
| `e1`：词法与 token/位置输出 | 1 | 23,064 | 2.13% | 120 |
| `e3`：解析、类型/作用域、tape、错误与警告 | 2 | 223,715 | 20.65% | 216 |
| `e4`：O1/O2 优化 | 2 | 14,389 | 1.33% | 144 |
| `lower`：ABI、调用、目标指令 lowering 与数据布局 | 6 | 85,331 | 7.88% | 144 |
| `elf`：目标指令编码及 ELF/Mach-O/PE 镜像写出 | 6 | 112,067 | 10.34% | 78 |
| `tokenpp`：公开 token 路线的预处理 | 1 | 10,925 | 1.01% | 1 |
| `tokenlex`：公开 token 路线的词法输出 | 1 | 4,477 | 0.41% | 1 |
| `units`：多文件分帧与文件级 static 隔离 | 1 | 31,740 | 2.93% | 108 |

下表是二进制网络的压缩前拆分；与物理压缩体不可相加，百分比为相对整包大小而非物理占比：

| 网络记录 / 含义 | 字节 | 占整个 .com |
|---|---:|---:|
| `H`：阈值/选择网络参数记录 | 941,541 | 86.91% |
| `Q`：动作序列声明（包含编译模板动作） | 880,781 | 81.30% |
| `S`：字节字符串声明 | 27,331 | 2.52% |
| `N`：网络头记录 | 625 | 0.06% |
| `C`：动作序列共享前缀声明 | 459,881 | 42.45% |
<!-- model-bytes:end -->

`e3` 两份对应保留位置/错误的普通解析与启用警告的解析；`e2` 十二份来自六目标及普通/位置模式，取值、预定义宏和资源路径不同，不能仅凭同名认定重复。`elf` 是历史路由名，实际包含目标指令打包和 ELF/Mach-O/PE 写出，不能解释成仅 Linux ELF。`units` 是多单元分帧、扫描和文件级 static 隔离，不是第二个完整 parser。实际模型 index、SHA、引用数见账本的 `artifact_ledger.models`。

**不是 5.92 MB 全部都是数值权重。** 网络序列化还携带展开动作和字节字符串；Q 动作占比最大。这是后续性能/体积分析的方向，不代表现在已经证实可以删除或压缩多少。动作与模板属于编译规则信息，不能把它们排除后再宣称整个编译器只有几 KB。两 ISA 内核资源是代码/常量/导入槽整体；平台驱动里的启动、OS 适配、加载器和已链接 libc 当前没有独立 link-map，不能给出虚构细分。内嵌 19 份头文件源另计，是被编译程序可用的库输入。

**共享口径：** 32 个物理模型被 938 条阶段行引用，其中 906 条是对已有模型的重复引用；若按引用重复展开将计 374,077,053 B，但真实只存 5,922,890 B 模型体。这个逻辑展开量不是运行峰值内存，完整模型包物理仅一份。网络/执行核字节是跨阶段共享，不能按 C 特性如 struct、指针、printf 任意分摊；目前可精确归属的是阶段/模式模型。

**TDD 优先级与判定：** 功能门禁继续同时检查真实模型执行、host/参考的可观察行为、错误与退出状态；网络等于表不能替代语言正确性测试。性能独立测同输入/目标/优化级别、完整输出相等、进程时间与输入/产物身份，避免并行负载把基准变成噪声。已有本机 171 项通过是冻结 `1ccb16e` 的历史证据，新测试或源码变更按影响域验证，不能移植旧通过结果。性能探针将逐次有界采样；失败、超时、空输出与不同输出必须失败，不能以打印了成功文本来豁免。五次 self 采样分五次执行，每次 ≤60秒；功能测试仍用双槽滚动队列，重负载独占。当前 10.209/25.027 倍编译延时是待改善的成本，不作为改善收益。

复现命令：`python3 tests/modelbytes.py` 核对持久快照的物理加总、共享关系和本节表格，已纳入 docs 门禁；`--artifact unisacc.com` 额外核对当前产物 SHA/大小，`--write` 显式更新表格。此测试不把快照设成体积上限。性能用 `python3 tests/modelbench.py --compiler ./unisacc.com --reference PRIVATE_UA --source examples/fib.c --target osx/arm64 --samples 5 --output PRIVATE.json`，self 改 `--source unisacc.c --samples 1` 分次执行；`--max-seconds` 可由同机实测预算显式设置，不自动制造基线。性能测试不进并发计时门禁，只有其故障判定检查 `modelbenchcheck` 进入功能队列。

parse2 function/global/local_control 去重（本session直改主树，TDD验证，2026-09-28）：三个函数原来各自内嵌一段完全同构的"读*-fresh.tsv绑定fresh continuation、再读*-sections.tsv调structured_control"循环，只是tsv前缀和绑定内容不同。抽成共享`_namespace_control(namespace, section, warnings, bindings)`，三处改为薄封装。验证：改动前后`python3 exec/parse2/gen2.py OUT.json`产出sha256一致(5baca634...)；`./tests/gate.sh --suite exec-e3self`及`--suite exec-driver-language-1 --suite exec-driver-language-2`（25个host/ASM网络O0/O2及native探针，含条件拒绝用例）均rc=0，无回归。这是真的、经门禁验证的去重，不是本session前一次仅靠json哈希核对的极小改动。ordinary_control因为多带sequences/word_state前缀与返回值，结构不完全同构，本次未合并，留给理解更深或cdx确认后再做。

本轮验证：docs 生成表与字节账核对通过；修改模型归属字节的副本与错误产物均被拒绝。`modelbenchcheck` 的正常、非零（先输出正确镜像再 exit2）、缺失/空/不同镜像、候选/参考超时、参考非零、显式超严性能阈值及输入输出别名故障全部按预期判定。实际 9a0ae470 候选 fib 五次全新进程编译完整镜像均同参考，中位约0.655秒；自身源码一次约18.967秒，完整镜像同参考。此为探针验证，不以单次 self 或本轮环境与旧中位数差额宣称优化收益。功能、性能工具与文档域修改，未改产品源码/模型，未重跑或冒称新172项全套通过；新增 `modelbenchcheck` 已入队列，后续候选按新清单执行。


### 初心与论文对齐审阅（2026-09-28，完成）

只读运行边界审计未发现执行核隐藏 C 语法、类型、ABI 或编码选择谓词：网络输出下一状态与动作序列，通用核执行算术/存储/字节操作，驱动保留 CLI、文件/资源路径、OS 加载与运行。初心在运行路径上成立，但动作模板仍是程序规则信息，声明化不使信息或总字节自动减少；离线 Python 仍有动态绑定。T1 与有限行为测试不能推出 T2/T3。下一优化先验有序阈值早停（cc-unisacc 已授权占 core/asm 文件域），再按实测考虑解码模型复用或批量 span，暂不并行改三种运行机制。

审阅发现具体 TDD 缺口：运行受 UNISA_MAXSTEPS、UNISA_CONTAINER、UNISA_KERNEL 影响，但滚动队列原身份没有这些值/外部文件字节，性能探针也没有记录外部包/内核身份。本轮补身份和失效控制，避免同一 .com 哈希测到另一份包或改预算后复用旧成功结果。旧经典 bench.sh 仍忽略被测命令 rc，且不验证完整镜像，本轮不把它当当前模型性能证据；实际模型用新有界探针。论文中“E1 尚在实验”“E3 查表原型”“当前产品结构工作留经典代码”等已滞后，下一修改保留历史18表/发布版测量，另列32网络本地候选和实测边界。

模型路线提速一（本session，主人"每次都卡一下"反馈，2026-09-28）：实测 `unisacc.com -run examples/apps/calc.c` 模型路线约0.55秒、经典路线约0.035秒，输出相同；主因是 `#include <stdio.h>`（单独0.41秒）。分段计时：e2 0.023秒、e1约0.002秒、**e3约0.27秒**、lower+elf约0.13秒。插桩（stdio.h单独程序）：e3 3.29M步共扫描602M个阈值单元（平均183/步，71%激活），lower 2.44M步扫描61M单元（仅9%激活）——`core_transition`每步线性扫完一个状态的全部H单元，是运行时间的主体。
决定：net.py按升序发阈值、run.c装载时已强制严格升序（`bad network unit`），故激活单元必为前缀。C核心与两份asm内核改为"二分查找前缀长度（asm用csel/cmov无分支）+只累加这些权重"：同一网络同一求和、不物化任何答案表或前缀和，结果逐位相同。验证：transitioncheck新增独立全和oracle（按net.py定义逐单元求和，不早停），arm64/x86_64各537,620项通过；把比较改成gt的变异体在两架构均被抓住（rc=1）；exec-core/exec-asm/exec-asmx86/exec-net全绿（六个完整镜像同参考、五个native同）。宿主cc链接asm内核实测：stdio.h单独0.302→0.170秒，calc.c 0.407→0.236秒，exeinfo.c 0.672→0.393秒（约-42%），输出逐字节相同。附带发现：线性早停版在cc -O2下反而更慢（破坏自动向量化），二分+定长求和两种编译方式都更快，故C核心也用二分。剩余：e3平均激活单元仍约130/步，进一步需减少单元数（例如构造期调整token/寄存器键的编码顺序）——属于模型构造改动，另议。


本轮审阅结论与验收：

- **初心没有在运行边界上崩坏。** 当前产品包要求实际网络，通用核没有发现 C 语法/类型/ABI 选择谓词；共享动作和模板承载规则信息，驱动仍承担 CLI/文件/OS 适配。程序规则变成数据，并不由此证明所有源码减少或产物更小、更快。旧 C/Python 参考仍须维护，双来源风险不隐去。
- **TDD 修复已实跑红绿。** 队列旧实现未记录空 UNISA_MAXSTEPS，新增控制先 rc1；补设置与外部包/核路径、权限、内容 SHA 后 rc0。同身份续跑，燃料/路径/内容/权限改变拒绝旧 state，缺失/目录/FIFO拒绝；unset 与空值不混同。真实指纹测试在私有最小 git 仓库，未用共享构建。性能探针记录并前后复核实际外部文件；正确镜像后 exit2、外部包中途改变、缺核都拒绝。父独立 queuecheck/modelbenchcheck、docs 与 diff 检查通过，真实9a0候选加显式外部包的 fib 镜像同参考；只验判定和身份，不作为并行环境下提速数据。
- **论文方法核仍成立，现状叙述已修。** Paper A §1.2 新增 S-17 实际32网络、171项冻结门禁、包账、延时与固定包驱动自举；18事实表、DENSE、E0和旧性能保留为历史经典实例。T1 不推出 T2/T3，4/18局部外部裁判不称全语义证明。另修正域商化“不是双射”、有界 tape 枚举“不覆盖任意长度”、任意网络最小宽度与两级逻辑最小化“等价”未证。未新跑 Lean 或全平台，不把文档审阅称新理论证明。
- **优化按单个热点推进。** 已有并行会话独占 core.c 与两 ISA transition；其4f5fd82优化是二分定位激活前缀再累加同一权重，而不是答案表或前缀和。其 host链接内核报告约42%改善只作为该口径证据，根9a0产物目前仍旧核，不能把收益写成已出货.com收益。下一候选须重新构建并做网络全域/独立全和oracle/实际功能门禁及同口径性能对照。解码模型复用与批量SPAN作为后续两项候选，先按stage计时和峰值内存决定；不同时开启新框架/大范围语言扩展。


### 真实系统示例收敛（2026-09-28，进行中）

`examples/apps` 四个系统工具不再默认编造进程、窗口、地址空间或可执行文件。C 分析器读取显式真实快照；Linux memmap 默认读取自己的 `/proc/self/maps`；其他缺少直接系统绑定的入口报缺输入。新增一个有界 `run.sh` 把实际 ps、macOS Mach 区域采集器和现有 CoreGraphics 采集器与分析器衔接，exeinfo 默认分析实际编译器容器。采集器与分析器的进程身份明确区分，不宣称内置 libc 默认转发系统库；缺少接口不启动新的 libc 架构迁移。测试共用一次实时快照对拍 cc / 模型 -run / O2 原生输出，自身映射按结构验，不要求不同二进制映射相等。

另一个会话的 `/tmp/cc_perf/queue` 在83/172记录后外层term.sh以143停止，已查无存活队列进程；不称完整门禁通过。本轮只在其终止后改示例与新增测试，后续输入指纹变更不能复用这轮83项结果。

真实示例本轮红绿：旧应用代码在新增测试的缺输入检查上以0继续显示SAMPLE，测试rc1；修改后四个分析器在同一实时ps/Mach/CoreGraphics快照与实际8b3b909b容器上，cc、模型-run、O2原生输出全部相同；缺文件/空快照与缺默认输入检查通过。新测试纳入apps-real，整轮清单随之增为173项，不能引用旧171/172结果为其完成证明。
模型路线提速二：声明返回（本session，主人"值得做就做"，2026-09-28）：提速一之后插桩：e3 剩余求和量的97%来自同一状态RET（续点返回），栈键、1,537行"续点k→k、共用POP序列"+BOT行，构成2,461个阈值单元，每次过程返回都要累加约1,865个。它是exec/pp/gen.py G.finish 的保留构造，各阶段同形（e2 57行、e3 1,538、e4 66、lower 444、elf 322）。
决定：返回是对无界续点栈的控制，不是有限决策（论文§2.2判据），改为声明原语。net.py：栈键银行中"k→k且共用同一序列"的行（≥2行，取最多的序列）编为声明返回集，网络记录`H 3 lo hi n bn bq rs m k1..km units`，单元只为其余行构造（声明键处视为不观察，不产生断点）。run.c装载：rs∈[0,NQ)、键严格递增且<NS，否则`bad return declaration`；CoreModel末尾加ret_seq/ret_ok（layout.h CM_RET_SEQ/CM_RET_OK，CM_SIZE 152）。C核与两份asm内核在求阈值前先答声明返回：key∈[0,ns)且ret_ok[q][key]→(key, ret_seq[q])；其余观察（含空栈、未声明键）仍由单元回答，拒绝行为不变。表(.tbl)不变，仍是参考。
验证：transitioncheck新增独立行函数oracle（随机栈行函数f，按net.py方式编译，要求内核在全部键上重现f），arm64/x86_64各803,861项通过；四个返回路径变异体（去成员检查、目标写0，各架构）全部被抓；过程中发现测试RNG取LCG低位周期短，声明键几乎只落在key 0，使"目标写0"变异体漏网，已改用高位。netcheck新增H 3编译与四种篡改（返回序列、漏续点、多加非续点、越界键）均被拒。真实各阶段表经新net.py重编后`--check-net`全域相等（e3 1,549,292观察；e3单元10,902→8,442）。门禁：exec-core/net/asm/asmx86/e3self、exec-driver-language-1/2、com-run、com-c99全绿。
实测（.com 6,152,084 B，SHA前缀c94cf5fe35caf6e5，输出与经典逐字节同）：examples/apps全部11个例子相对原9a0ae470约-60%（calc.c 0.556→0.207秒，bf.c 0.524→0.205，exeinfo.c 1.124→0.544）；modelbench同法：fib.c模型中位数0.6206→0.3119秒（经典0.0797，3.9倍），unisacc.c自编译19.43→10.15秒（经典0.790，12.8倍）。论文§1.2/§2.2已同步说明两项改动、判据依据与新身份测量，冻结表保持原身份。
遗留：主人手动停止的172项队列（停止时83项完成0失败）太慢，下一步专门查其慢因；新身份的完整门禁尚未全跑。

用户纠正（2026-09-28）：上一示例交付只由unisacc分析、由system cc构建采集器，不满足“用unisacc.com做真实事情”。该方案撤回为未达目标；去除运行时cc采集路径，补实际程序可用的系统入口后由unisacc编译的程序主动采集，不以run.sh提示代替功能。已有快照对拍仅证明分析器行为，不证明程序自行采集能力。


### 动态系统调用桥（2026-09-28，用户授权，进行中）

用户明确允许采用更好的方案，`-run`是优先落点而非范围上限。先以四个libSystem dl入口、通用六整数参数ABI桥和libffi显式类型向量补能力；原生与内存执行共用声明。系统示例必须由unisacc编译的程序自己采集，不再运行cc采集器。绑定、编码、FFI运行时分文件并行；现阶段仅macOS实测，不把可调用系统API扩大为全部libc已经转发。FILE/分配器/变参家族须保持一致，避免混用不同ABI的对象。

门禁慢因与修正（主人"172项太慢，一定有问题"，2026-09-28）：主人手动停止的那轮队列并非卡住：08:34:36–08:46:56共12.3分钟完成83项、0失败，窗口内槽位利用率87%，term.sh每次约0.4秒、plan/fingerprint约0.1秒，窗口间开销可忽略。慢在套件自身重复工作：约45–50个套件（memorycheck/multicheck/compilercheck/bindingcheck族）各自调用exec/c/compilerpack.py在临时目录重建一个编译器包，其中token/located警告lex/parse/errorparse/units/每目标located pp共7个模型每次都从Python生成器重新构造（实测17.6秒/次），而阶段模型早已走exec/pipeline/models.py的内容哈希缓存（1.8秒）。另：CLAUDE.md推荐的队列命令未设MODEL_COM时exec-container会整体重建编译器（约45秒），release.sh路径则要求显式MODEL_COM。
决定：compilerpack的附加模型走同一缓存基址（UNISACC_MODEL_CACHE，默认$TMPDIR/unisacc-model-cache），键=models.py的文件闭包（抽出为closure()共用）+Python版本+脚本+参数+生成器可导入源码目录（exec/pp、lex、parse、parse2、unisa及finite_rules/tbl/net）中environ/getenv旁出现的全部环境名的有效值（新导入自动覆盖；各检查器的套件设置不进键）。每键flock、临时目录构造后原子rename、命中时逐文件sha256复核manifest。出货构建buildcompiler.sh传--no-model-cache，产物路径不变。
验证：同一输入四种方式（HEAD版、冷缓存、热缓存、--no-model-cache）产出compiler.pkg逐字节相同（f89fd02f…）；热缓存17.6→0.1秒；篡改缓存条目被复核发现并重建、输出相同；改一个生成器源文件键即改变、复原即复原。实测套件：exec-driver-resources 28.3→7秒，exec-multiwarn 27.2→4秒（首轮冷构造后）。

用户纠正测试工具（2026-09-28）：当前执行不再使用Perl看门狗；沿用已有Python测试依赖统一有界调度、返回码及所属进程树清理。保留每步60秒上限，不把测试调度器放进产品编译路径。

看门狗性能纠偏（2026-09-28）：逐探针 Python 包装增加解释器启动成本，difftest_o 的双层 Python 尤其重复。保持不使用 Perl，热路径改为一次构建的原生 POSIX 看门狗；每个冻结套件起点解析 helper 一次，后续直接调用。Python 仅用于外层队列和冷构建限时。保留原始 wait status，不能混淆正常 exit(142) 与超时。前后同分片实测后再确认收益；macOS 已重新父化的完全脱离子进程仍有追踪限制，Linux subreaper 尚待实跑。

原生看门狗验收：本机 macOS arm64，同参考二进制、SHARD=1/16、PAR=2、预热参考缓存，difftest_o 交替3轮均27 agree/0 wrong/0 refuse。bd8622e双Python热路径中位数1.4577秒，直接native中位数0.8309秒（-43.0%）；100次true的3轮中位数native0.5095秒、Python3.6688秒。此为分片/启动成本，不外推全门禁。父独立boundcheck验证两种工具正常退出0/2/142、信号和nested setsid超时清理；native额外核对原始waitstatus。67个原生入口shell语法通过，staticinit9项、exec/c/neg5项通过。全部时间约束不超过60秒。

本轮候选收口（2026-09-28）：FFI桥父独立验证两ISA各96编码、8负例、真实网络原生ABI调用、dyld dlopen/dlsym/strlen及经典内存执行通过；不称Python VM支持hostcall。Q动作序列前缀只改变序列化，32旧产物模型展开QLEN/QA一致，节省829791字节，ASan+UBSan、8畸形拒绝通过；运行时内存不减。候选重建后仍须真实示例、完整有界门禁、同口径速度与功能字节账；当前根.com还不含这些在飞改动，不宣布交付。

候选实际红灯：FFI产品测试在ad239a6候选上报“host intrinsic argument count”，不能交付。源码定位检查发生在CL.pop之后，na已递减到0；已有nar保存真实参数数目。修复使用nar，不新增计数机制。该失败不由ABI桥造成；新候选必须重建后复验。

FFI与实采父验收：narfix候选5,388,386 B；17/17系统ABI探针在模型-run与O2原生输出相同，错误hostcall/hostaddr参数数目以1拒绝。apps-real四应用全绿，procview/memmap/winlayout的原始快照从同一模型候选编译的应用--capture取，cc只作同输入分析器；删除两个cc采集器与相应测试路径。全门禁仍待冻结新树完成。新字节账独立存research/model-bytes.json，旧s17-final-evidence.json不改作新候选证据。

冻结门禁 c227fd5 暴露测试参考构建缺口：exec-pploc、exec-lexpos、exec-diag 自行拼接源码时遗漏新增 src/host_dl.h，宿主 cc 报缺头文件；同结构 returnwarningcheck 也补齐。只改测试构建入口，不改候选产品。参考默认缓存依赖改为全部 src/*.h，防止桥头修改后仍复用旧参考。此次 28 项含 3 构建失败，不记全绿；修复后新冻结队列重新验证。

测试入口修复验收：exec-pploc、exec-lexpos、exec-diag、exec-returnwarn 四项 rc0（19秒双槽）；在私有临时目录修改 host_dl.h，执行 lib.sh 中实际哈希命令，摘要改变，无共享参考写入。候选 .com 哈希与大小未变。

同身份性能验收（macOS arm64、独占运行、-O2 osx/arm64）：a4de871a… 候选 fib 五次中位 0.305987秒，自编译五次中位 9.871967秒；所有十次产物完整 SHA 与私有经典参考相同，均 rc0。参考各一次为0.021474/0.195023秒，不把一次参考当五次中位基线；不同源码/负载的历史数据不用于算优化比例。持久原始记录 research/candidate-bench-20260928.json；当前候选5,388,386 B，比上一6,152,084 B小12.41%，含新FFI能力，仍待本轮177项全套与平台限制报告。

冻结 759240b 门禁实际177项完成、175通过、2失败：multi/com-multi 残留 bound120/200，被新native看门狗拒绝，产品调用未发生。只在该轮终止后修测试入口：所有编译/执行20秒上限，输出比较同时要求被测命令rc0，不能让管道tail隐藏失败；ua_ready的冷参考构建也限30秒。旧红灯记录保留，不改写为绿。文档同步当前8486键、20190键—头判定，明确历史8484/20184身份。

multi入口修复验收：multi、com-multi、docs 三项双槽4秒全rc0；私有替身按输入打印正确8 309 11或5，再exit2，套件仍exit1，三条-run都报告exit2，编译亦失败，不误判绿。候选.com仍a4de871a…/5388386 B。完整冻结759240b的175绿+2红不回写；新测试树需新冻结队列验收。Windows/Linux self日志明确当前仅镜像相同、宿主未运行bootstrap，下一轮平台烟测单列。

候选平台烟测（a4de871a… 同一.com）：已运行Lima default Linux/aarch64看门狗两实现（退出0/2/142、信号、脱离子进程清理）通过；fib模型-run同客机cc（显式-include stdio.h）；模型编出的经典unisacc.c参考N1=N2=N3，SHA e831354f…，不称完整打包.com自重打包。Windows11 arm64 UTM：经ZIP传输后核完整SHA，候选-run、生成arm64原生和x86_64仿真fib均55、rc0；只属于烟测，非Windows全套。VM由本次启动且已停止，原本运行的Lima不动。Linux x86_64/Windows x86_64实机未跑。证据research/candidate-platform-20260928.json。

文档收口核对：README仍写“system libc is not used”，与新增显式FFI/系统API桥冲突；架构首页仍把模型产品.com的源码等同经典unisacc.c、把9a0历史尺寸列为当前快照。修为模型驱动/执行器与经典参考的准确分工，并单列a4de候选状态；历史171门禁不移植新候选，不宣称全部libc默认转发。

产物新鲜度门禁纠偏（2026-09-28）：用户指出根.com可能滞后。根产物与既有候选同为a4de871a…，但现门禁只绑定二进制SHA和测试树，未验证其构建输入身份；旧构建目录在当前保守输入闭包下pack报stale stage dependency，不能据此宣称当前来源已证明。增加构建时生成的来源侧车（产品输入内容哈希、产物SHA/大小、构建提交与设置），make com同时安装；--com队列和直接门禁在启动任何套件前拒绝缺来源、源变化或产物篡改。重新从当前输入构建后核对字节，旧108/177记录保留，不当作新测试树全绿。新鲜度校验不是仅看mtime或版本号，也不自动替旧二进制补来源。

新鲜度纠偏验收：当前输入完整双槽构造/打包（make com，外层60秒）成功，根产物再次得到a4de871ad6a251cbf7cee313e430f23bb08c618500a5285d36e224a4e74e0ece、5388386 B，与旧候选逐字节相同；旧mtime不代表其产品逻辑过时，pack拒绝旧目录反映保守来源闭包变化。新侧车unisacc.com.build.json由此次成功构建生成，非补签旧二进制。隔离夹具证明缺记录、头/汇编/二进制变化、打包期间源变化均拒绝；实际队列和直接--com入口在探针启动前拒绝坏产物。此检查纳入既有gate-infra，不增加套件数。旧266380d队列108/177全绿属旧测试树，保留原记录；新门禁验收另建身份，不宣称177项已完成。

53e6686新鲜度门禁父复验：根.com作为MODEL_COM，私有UA，双槽窗口15.57秒；gate-infra、com-run、ffi-product、apps-real、docs共5/5全绿。com-run实际12运行、wrong0；FFI17/17原生和-run；4个真实示例由应用自身采集。开跑与结束均核sources_sha256和完整artifact_sha256。非177项完整门禁，旧108项不跨测试树移植。

examples/apps 默认行为修正（主人要求本session直接修，2026-09-28）：`./unisacc.com -run examples/apps/exeinfo.c` 无参数时报错退出；改为无参数时由 unisacc 编译的程序自己解剖本机系统可执行文件（macOS `/bin/ls`、`/usr/lib/dyld`；Linux `/proc/self/exe`、`/bin/ls`；Windows `cmd.exe`、`kernel32.dll`），均为真实文件，不造样本。procview 在 macOS 经 FFI 取进程时，对其他用户进程的 PROC_PIDTBSDINFO 被拒，原实现直接丢弃（实测 1,458 中丢 568）；改为回退到无特权可读的 PROC_PIDT_SHORTBSDINFO（flavor 13，64B：pid@0、ppid@4、comm@16）补齐 pid/ppid/名字，任务信息（RSS）被拒的进程保留并显示为 `?`、表头注明原因（只有 setuid-root 的 `/bin/ps` 能读他人 RSS，这是 OS 特权边界，不是编译器限制），只有扫描期间退出的进程才跳过。实测：进程树 1,477 项（ps 约 1,483），569 个他人进程 RSS 标 `?`。tests/appsrealcheck.py 的 exeinfo 期望由"无参数失败"改为"无参数成功且解剖出真实格式、不得含样本"。验证：apps-real 门禁 rc0（cc / 模型 -run / 原生三路、缺失输入、默认、空输入），exeinfo 无参数输出与经典路线逐字节相同。

冻结1dbac50完整门禁177/177全rc0，根.com哈希a4de871a…始终一致。其结束后，外部会话更新procview.c（受限进程BSD短信息与未知RSS标识）和exeinfo.c（无参数读取真实系统镜像）；未覆盖/提交这两份外部改动。追加apps-real+com-ccparity+docs中apps-real红灯：旧测试要求exeinfo无参rc1，新真实默认行为rc0。修正测试契约为默认真实分析的正例（macOS与cc独立读相同系统文件，-run/native均比较），Linux自镜像只结构核对；缺路径仍要求rc1。新检查单列，不改写完整冻结账。

外部示例改动随后提交d46f4f9（仅两应用与appsrealcheck）；完整冻结队列的末次记录10:51:32，两应用写入10:52:34/10:53:04，故该177项结论仍只归属于先前冻结，不移植当前示例。追加当前apps-real+docs为2/2绿。父继续加强exeinfo默认检查：cc、模型-run和模型原生三条路径各自打开系统镜像，macOS/Windows同文件必须逐字节相同；Linux自镜像只结构核对。无需因纯示例变化重建不变的产品，但这几项新验收单列。

本轮收口账：冻结1dbac50完整177项各一次全rc0，源码树在该轮结束时通过原始指纹校验；活跃窗口合计482.34秒，含窗口间核对的经过时间643.84秒，首三窗双槽后四槽，指定重项独占，三条窗口尾部延期均后来真实通过。外部示例后续由apps-real+docs单列复验（强化默认真实分析的cc/-run/native三路线），其余产品源码和共享测试输入未变；不改写冻结结果或把追加检查称177项重跑。证据research/candidate-gate-20260928.json，含逐项日志SHA/摘要与构建来源；根.com仍5388386 B/a4de871a…。已完成尺寸/性能/真实示例/新鲜度门禁这一批，未发布。
后续提速意见登记为未实施：按套件声明输入闭包复用通过结果（先证明闭包，不猜动态依赖）；memory布局单次化、二进制阶段流、库AOT、跨模型记录共享等都需独立同身份测量。当前不把它们包装成已落地收益，也不为记录这些意见再改产品或重开全套。

v0.0.9 优化重点（主人定，2026-09-28）：尺寸、功能、速度、测试套件、演示套件；主人对 `.com` 5 MB 以上的体积有意见，列为首要。
- 尺寸现状（事实）：`.com` 约 94% 是模型体；冻结账中动作序列声明 3,718,873 B 多于阈值参数 2,150,111 B，均为十进制 ASCII 文本。实测 32 个网络中逐行完全相同的记录跨网络重复 28%（下限）；e3 带警告/不带警告两变体屏蔽编号后 94% 相同（各约 1.1 MB）；5 个约 78 KB 网络银行 100% 相同（按目标分的预处理）。
- 尺寸路线：(1) 模型二进制编码（varint/定长，替代十进制文本），先测同内容字节比；(2) 合并重复网络——警告开关、目标预定义改运行时输入，e3/e2 各一份；lower/elf 抽共享核心+目标数据表；(3) 包内记录级内容去重；(4) 包级压缩仅在解压代码与启动时间实测划算时采用。每项以 `tests/modelbytes.py` 字节账与 `--check-net` 全域等价验收。
- 速度路线：memory 布局单次化（-run 现跑两遍 elf）；库 AOT 预编译（stdio.h 每次重编）；阶段间二进制流；动作序列构建期原生化。
- 测试套件：按套件输入闭包复用结果；变体共享一次性准备；编译结果内容缓存。
- 演示套件：examples/apps 全部无参数即用真实数据、由 unisacc 编译的程序自取；Linux/Windows 的 winlayout 与 memmap 同样自取。
目标数字待各项最小实验后填入，不预先承诺。


v0.0.8 发布准备（2026-09-28）：主人授权本地最终树验收后发布，先草稿后确认。发现当前候选 --version 仍为0.0.7；更新版本并重建，发布身份以新产物完整SHA为准，不沿用a4de旧候选验收。私有新种子、最终HEAD本地release队列、Linux两架构与Windows两目标外部验证分别记录；任何条件失败不得降级。v0.0.9尺寸优化暂不实施。

v0.0.9 尺寸实验一（主人直觉：网络结构影响权重大小；打包时权重可做压缩/解压，2026-09-28，scratch 实测未改产品）：对 make com 产出的 32 个网络（模型文本共 5,852,659 B）：deflate-9 → 813,253 B（14%），xz → 236,792 B（4%），仅改 varint 二进制 → 2,683,073 B（46%），二进制+deflate → 676,681 B（12%）。解压耗时：inflate 全部 2.3 ms，xz 15.7 ms；逐网络独立 deflate 合计 808,732 B（14%，与整包相当），最大单网络 inflate 0.4 ms，按路由只解所需网络。结论：逐网络 deflate 是最省事的第一步，预计 `.com` 由约 5.4 MB 降到约 1.5 MB 量级，启动代价毫秒级；inflate 属包装/IO 经典代码（非决策逻辑），需以 unisacc 自身可编译的小型实现放入运行时并纳入字节账。xz 只剩 4% 说明冗余主要是跨网络重复与文本编码——结构性去重（合并 e3 两变体、目标预定义运行时化、键字母表与状态编号重排以减少断点和差分幅度）在压缩后对体积的边际收益变小，但仍决定构建时间、内存占用与维护量，应作为第二步。验收：同身份 `.com` 前后字节、`--check-net` 全域等价、全套门禁、启动与单次编译时间不回退。


v0.0.8 发布阻挡（2026-09-28）：版本更新提交a43844d，随后冻结b9fb8f2；新产物5388402 B，SHA256 948232f00028170d2090983375fbca2a3829ef8f73235baada5deb9db174d737。最终树私有种子队列/tmp/unisacc-release-v008/gate-b9实际11/177通过、0失败、166待执行，不称全绿。cc-minicon确认unisacc尚无产品签名身份、release-policy或签名workflow，不能直接复用minicon专属流程；按发布授权的签名条件与条件不过即停要求，停止推进，不降级未签名发布。未创建草稿、tag或push；未启动VM。候选及原冻结队列保留。后续须决定接入签名流程，或明确变更未签名发布条件，再冻结最终树续验收。


v0.0.8发布条件校正：cc-unisacc转达主人决定签名条件撤回；历代v0.0.1–v0.0.7本为未签名单文件。v0.0.8按惯例未签名发布，说明明确未签名；签名后续另行接入，不阻挡本次。已向cc-minicon讨教：当前APE具空Security Directory但缺VERSIONINFO、没有ZIP，后续签名验证须按实际自定义包定位设计。其余最终冻结树本地全队列、平台60秒分批、关机、草稿确认条件保持。继续验收，不把旧11项移植新树。


v0.0.8 Linux arm64 发布定位（2026-09-28）：冻结07ccaf8本地177/177通过，但客机com-difftest_o第一片出现3个wrong，均为b_compound_result在-O0/-O1/-O2的同一差异。同客机实测：gcc默认plain char为unsigned，输出128/128；gcc -fsigned-char、同源经典与模型均输出-128/-128，其余字段一致。决定将有符号窄化回归明确写为signed char，不改编译器也不忽略差异；新增独立plain-char探针保留平台差异证据（不放入要求跨编译器相等的tests/c清单）。发布说明明确lnx/arm64 plain char符号性与该平台gcc不同，属于实现定义差异，不宣称平台ABI完全一致。按目标跟随符号性留待v0.0.9评估；新测试树需重新冻结验收，不移植旧177项结果。Windows客机RPC失效与Linux x86模拟整套超时分别记为未验，不作通过；本轮启动的两个客机均已关，原本运行的Lima default保留。

Linux差分第二片定位：b_boolconv的`static B literal="x"`被客机gcc拒绝（initializer element is not computable at load time）；加显式`(B)`后同一地址到_Bool转换可编译，实际输出不变。决定此跨编译器回归使用显式转换，不称隐式静态字符串初始化已获gcc对照；原拒绝诊断与四种表达式实验日志保留。其余第三/四片分别108/105项全相同，产品不改。

修正后实测：Linux arm64模型difftest_o四片共429个比较，wrong0/refuse0；macOS第二片经典与模型各108个比较均相同。独立tests/plainchar.c在macOS三路均为plain=-1，在Linux arm64 gcc为plain=255、经典/模型为plain=-1；signed=-1/unsigned=255均一致，模型与经典各-O0/-O1/-O2通过。候选948232f…产品输入未改变，不重建相同产物；本次提交冻结后重跑本地177项。平台未验项将在草稿中逐项注明，最终证据放发布附件，不为更新计数反复改变冻结树。

冻结0f51890新队列发现真实种子缺口：fat拒绝b_boolconv中的`(B)"x"`，而经典C前端与模型接受且结果相同；不是超时。种子const_atom原本解析cast后丢弃类型，随后不认识字符串。决定只修_Bool cast的常量折叠：字符串地址转换为1，整数操作数按非零转换；不把任意地址伪装成整数。b_boolconv继续作回归。bigclosure另有48秒并发超时（完成五目标），保留该失败记录，下一冻结重项独占执行。Linux arm64本次候选编出原生经典N1，再N1编N2、N2编N3，三份SHA f2aded98119b88245bfd2041f0f6d799715c6704a5be76cf5bf5b926b23479a1，版本0.0.8；只称编译器自举，不称模型包自重打包。

发布队列调度修正：bigclosure与fat在release.sh中声明为独占重套件，其内部仍有探针并行；普通独立套件继续双槽并行。不增加超时，不改变比较或通过判据，只避免两个重套件相互争抢CPU。

_Bool种子修复父验：同一个b_boolconv实际运行arm64与Rosetta x86_64两片，fat 1/mismatch0；重建模型.com与原候选逐字节相同，仍5388402 B/SHA948232f…，来源侧车按新的完整输入闭包生成。产品逻辑未变；种子修复与调度变更纳入新的最终冻结。


### v0.0.8 发布结案（2026-09-28）

已发布：https://github.com/partnernetsoftware/unisacc/releases/tag/v0.0.8 。tag 指向 10672e3；未签名的 unisacc.com 为 5,388,402 B，SHA256 为 948232f00028170d2090983375fbca2a3829ef8f73235baada5deb9db174d737。最终冻结本地 release --com 177/177、rc0，附件保留证据及平台限制。Linux arm64完成单列验证与原生编译器三代自举，不含模型容器自重打包；Linux x86_64与Windows两目标本轮未验，不称六平台全绿。主树解除发布冻结；后续改动不回写此次验收。

### v0.0.9 首片决定：逐网络压缩

尺寸优先，先做不改变网络内容的逐网络 raw DEFLATE；结构合并与二进制编码留后，分别测量收益。本次是方案决定，尚未实现。v0.0.8实际模型正文合计5,018,362 B，占.com的93.1%；既有压缩实验支持方向，但1.1–1.5 MB仍是估计，不作交付承诺。

实现范围：compilerpack沿用现有按原内容去重及pack.py的compact_q动作前缀序列化，然后压缩每个独立模型；原长与SHA256均针对compact_q之后、压缩之前的实际网络字节。新包采用显式版本，保留P1/P2读取及独立.net检查路径；记录压缩长度、原长、codec与解压原文CRC32；SHA256保留在构建包审计与门禁。资源、头文件与机器码模板本片不压缩。运行时只解当前路由的模型；解压器是字节到字节的通用机制，不包含编译阶段或语言规则，不改权重、推理函数与答案。

先核对的小项：当前run.c会把整个包读入，再逐阶段loadbytes/unload，并无模型缓存；“按需解压”不等于按需读文件。首片保持生命周期，每阶段只持有需要的解压缓冲，不顺带加入缓存框架。确认loadbytes复制所有执行期数据后才释放缓冲，清除借用指针。包内共享模型索引保留，不按路由复制模型。exec/c没有原生SHA256，但src/back_image.c已有Mach-O签名用C实现，可抽取做吞吐实验；inflate与校验必须可由unisacc编译，并计入体积和耗时。

TDD与验收：先用实际发布包测压缩后的逐模型和总字节；小解码器覆盖stored/fixed/dynamic Huffman及边界，严格检查输入/输出长度、截断、尾部垃圾与摘要，失败后不执行半解码模型。cc与unisacc构建结果一致；解压出的每个模型必须与旧包原文逐字节相同，真实阶段--check-net全域相等。保留旧包读取回归，不扩大畸形输入工程。报告.com总量、模型、codec/哈希代码、资源及峰值内存；同一输入、同一机器、五次中位数测启动和calc.c -run，并与未压缩候选比较，宿主zlib的毫秒数不能代替实际运行时测量。先跑相关有界检查，稳定后冻结树完整门禁；每步最多60秒，独立检查并行，重项独占，不为文档提交反复跑全套。

停止条件：首片只交付格式、解码、等价及尺寸/速度账；如解码或哈希抵消收益，报告测量再选方案。第二片再评估e3警告变体与目标预定义的结构复用，二进制编码随后按压缩后的边际收益决定；不同时混入三种改动。

压缩校验实验决定：先用既有C SHA256、表驱动CRC32与puff小解码器在实际compact_q网络上做独立吞吐测量。puff依赖setjmp，吞吐原型暂将越界输入改为立即退出，只验证有效流，不作为产品解码器或其错误路径验收。若逐网络SHA256明显吃掉收益，运行时可改CRC32，SHA256继续留在构建包审计；尚不凭宿主zlib结果作选择。


压缩校验实验结果（2026-09-28，macOS arm64，原始记录research/compression-codec-bench-20260928.json）：固定v0.0.8包948232f…的32个compact_q正文共5,018,362 B，逐网络raw DEFLATE-9共805,763 B（16.1%，此前14%是另一份未做同样前缀压缩的输入，不能混用）。按osx/arm64/run/O0四阶段再加memory两遍的实际模型索引测量，总原文1,707,892 B/调用；同一原型cc -O2与unisacc -O2分别构建，五组25次减5次的进程耗时差除20，近似去掉进程、文件读入和最终输出成本。unisacc中位数：SHA256 36.067 ms（47.4 MB/s）、表驱动CRC32 6.723 ms（254.0 MB/s）、puff inflate 18.386 ms（92.9 MB/s）；cc为6.468/2.874/3.067 ms。所有32个网络在两份二进制下均与原文、hashlib SHA256及zlib CRC32一致。

据此首片选择解压原文CRC32作运行时损坏检测，SHA256只作构建包身份与门禁审计。CRC32不是签名或对抗篡改验证；它在此用于偶发损坏检查。原型解码器EOF直接退出，不采用setjmp，尚未完成产品错误路径；未接入运行时，未测整机启动/calc、峰值内存或.com总量。本实验支持减少逐装载校验成本，不宣布编译速度不回退。生产解码器仍必须验证长度与流完整性，不能仅凭CRC接受越界输出。


压缩速度评审后下一实验（2026-09-28）：组件inflate+CRC约25ms，不直接等同calc整机回退；新包可减少读取字节，尚需同身份整机证据。先在scratch原型给Huffman加9位前缀查找表（长码回退原解码器），CRC改slice-by-4，测完整32模型一致性及相同路由负载。calc五次中位回退≤5ms作为首片整机验收目标；达不到则暂不交付，先定位。二进制编码保持独立实验，不同时改变三种格式；既有SHA/inflate在unisacc下明显慢于cc，仅登记为该原型热循环的代码质量证据，不外推整个编译器为固定6倍。


查表与CRC局部实验结果（research/compression-fastcodec-bench-20260928.json）：先9位前缀表、长码回退后inflate为10.307ms；消除短码重复bits调用并保持精确消费后9.475ms；slice-by-4 CRC为2.168ms，组件合计11.643ms，比前次25.109ms减少53.6%。同一32网络、同一模型索引/1,707,892 B、unisacc -O2、五组差分中位数；cc为inflate2.344ms、CRC1.102ms。表仅为通用DEFLATE码本，不物化编译答案。104组空流/长度边界/stored/fixed/dynamic策略/跨sync-flush块的有效流在cc、unisacc与ASan+UBSan下分别核CRC和解压原文一致。记录保留原型精确补丁与原始时间。

仍未达到的是整机证据：不能用11.643ms组件成本宣布calc回退≤5ms或超出5ms；读包减少与其他负担都需实际候选验证。产品包及根.com保持v0.0.8原样，生产解码器错误路径未验，不集成出货。下一步只做有界私有压缩候选的实际装载与同身份calc测量；若超出5ms目标，定位后再选方向，二进制编码不并入当前片。热循环质量差异记为独立优化线索，不将约4倍inflate或约5.6倍SHA差异概括成所有代码固定慢6倍。


私有候选实测方案：不改主树产品，复制当前driver到独立目录，加入实验P3读取及同一查表解码/CRC原型；原P2与P3使用同一新driver前缀、同一模型与资源，仅存储格式不同。记录包读取、解压、CRC、原loadbytes解析与执行的分项时间，并对calc -run五次交替测量；同时对当前发布物作对照。原型错误处理不作产品验收。冷读若不能可靠控制缓存则明确未测，不用新文件名或首次进程假冒冷缓存；允许单独记录F_NOCACHE绕过文件缓存的实验，仍不称硬件冷盘。


私有压缩候选实测结果（research/compression-candidate-bench-20260928.json）：最终plain/compressed采用相同新driver前缀，尺寸5,424,018/1,212,021 B，资源与模型原文不变；根发布物仍948232f…/5,388,402 B，未替换。calc默认-run的五次交替中位数：plain191.960ms、compressed200.818ms、发布物189.128ms；压缩相对同driver慢8.858ms，相对发布物慢11.690ms，未达≤5ms目标。第一版慢5.859ms的记录保留；复制循环改为局部指针后未获整机改善，不择最好一轮冒充达标。calc stdout与三路-O2原生镜像分别完全相同。--version中位数8.643/8.513/8.625ms；该路径不装包，不能作为模型装载启动成本。

归因（三项分开）：plain/compressed包读取2.909/0.756ms，目录检查与初始化2.449/2.414ms；压缩新增inflate9.499ms及CRC2.171ms；loadbytes模型解析8.924/9.008ms，执行168.738/168.653ms基本不变。各列是各自中位数，不要求与整机中位数可加。独立相同readstream读取器验证字节数/CRC：缓存热读2.817/0.732ms；经libffi变参调用fcntl F_NOCACHE(48)关闭该fd的数据缓存2.818/0.726ms。后者仅缓存绕过实验，未清全局缓存或硬件缓存，不称可靠冷读；真正冷盘未测。新解码原型32网络及104组有效流在cc/ua/sanitizer一致，生产坏流与全门禁仍未验。

决定：本轮不集成出货。原型已经把inflate输出缓冲直接交loadbytes并在解析复制完成后释放，不存在再删一遍整块模型memcpy的收益；改流式解析会改变装载机制且不保证更快，先不展开。下一实验单独评估二进制网络表示能否减少需解压的字节和十进制解析成本；压缩实测基线固定，不同时混入网络结构合并、缓存框架或答案变更。冷读缺口和速度失败均保留，不为尺寸收益降级验收。


二进制表示独立实验决定（2026-09-28）：scratch模型魔数UNINETB1，原N/S/Q/C/H记录结构与动作前缀引用不变；整数用带符号zigzag/LEB128，字符串用长度加原字节。先构造未压缩二进制包，再构造同一二进制+DEFLATE/CRC包，与同driver文本包对照。Python按既有模型schema独立解码，全部32模型须恢复为原文本逐字节相同；再用C装载器逐项序列化已加载的维度/字符串/展开动作/银行参数与返回集，原文和二进制比较，避免只测calc掩盖读数错误。仍是私有原型，不替换产品；≤5ms为实验目标，若仍不达，向主人呈现尺寸与时间取舍而非静默撤回压缩。执行阶段约169ms的独立路线继续保留：memory单次布局、库AOT、阶段间二进制流，均未实施或承诺收益。


二进制表示实验结果（research/compression-binary-bench-20260928.json）：32模型正文text5,018,362 B→binary2,306,841 B→binary+DEFLATE670,428 B。四种包共用同一driver前缀：text.com5,433,362 B、text+deflate1,221,365 B、binary2,722,155 B、组合1,086,020 B（含私有插桩与snapshot测试代码）；发布物仍5,388,402 B未改。Python按记录schema还原全部32文本逐字节相同（包括Q0/H0尾空格）；unisacc加载后的全部维度、字符串、展开Q数组、H权重与声明返回集序列化逐字节相同；host ASan+UBSan也与这些快照一致，START篡改能被捕获。未把CRC相同冒充加载内容证明。

同机五次交替中位calc默认-run：text188.254ms、text+deflate197.342ms、binary184.978ms、组合190.212ms、发布物188.305ms。组合相对同drivertext+1.958ms、相对发布物+1.907ms，达成本次≤5ms实验目标；不压缩binary较text快3.276ms。三项归因：包读取text3.066ms/组合0.751ms；text+deflate解压9.508ms+CRC2.138ms，组合7.704ms+0.998ms；模型解析text8.881ms/组合6.252ms；执行分别164.954/163.640ms，仍是大头，不将变化归因成执行优化。五种路径calc stdout和-O2生成镜像完全相同，各项中位不必可加。

真实模型装载启动另测最小程序（非--version）：text25.937ms、text+deflate35.752ms、binary22.157ms、组合29.475ms、发布物25.826ms，组合仍多3.538/3.649ms。该差异显式保留，未宣布启动零回退；真正冷盘仍未测。当前证据只覆盖私有有效输入和本机，不代表生产坏流/完整门禁/跨平台通过。

下一步选择该二进制+逐网络DEFLATE组合转入正式的有界格式/解码实现和兼容性验收，不继续混入结构合并等优化；去掉插桩与snapshot产品入口，测试保留独立夹具，候选重新构建后实测。小幅启动代价继续记账，不因尺寸收益改写失败或未验项。如果最终组合仍超过5ms，则向主人呈现真实尺寸/耗时取舍；不将同事建议的5ms扩大成用户已授权的永久硬限制，也不静默放弃压缩。执行热点的memory单次布局、库AOT、阶段流二进制保持独立后续，不声称已改善169ms执行部分。


正式codec首个实现切片：先落独立的有界raw-DEFLATE/CRC组件与批量测试，不切换产品包。采用已测的9位前缀表，EOF改为错误返回而非exit/setjmp，检查距离、输出容量及完整消费；保留Mark Adler puff许可证并明确修改。TDD以zlib为独立字节参考，合法流、截断/随机流/翻位、声明长度错误和手造无效距离/码本在宿主sanitizer与unisacc构建下批量验证；翻位可能仍是合法流，必须用CRC/原长判定损坏，不能宣称任意翻位都会成为格式错误。先完成组件再接包版本与二进制装载，六目标只编译不等于六目标执行通过；测试用同一进程批量解码，避免逐向量启动和重复压包。

正式codec组件父验：cc、unisacc -O2与宿主ASan/UBSan分别通过1,654个批量向量（合法stored/fixed/dynamic、截断、翻位、尾部、多/少原长、保留块/坏码本/越界距离、1,000个随机短流）；与Python zlib独立判断完整流/输出CRC，unisacc构建另检查输入/输出两端16字节哨兵。32份本次二进制实验网络在两份构建下均精确解到声明长度并CRC一致。测试驱动遇到scanf缺失与管道fread短读，改严格数字头和循环读满；这不是解码错误。独立exec-codec加入队列，单入口含sanitizer实跑rc0；未切换pack/runtime或根.com，旧包兼容、六目标执行与最终性能账尚待后续。

codec补充验证：固定非法控制样例先由zlib断言确为非法（保留块类型、四个一位码构成过订阅码本、首个长度引用已有输出之前的距离），另补跨sync-flush块，正式向量变为1,655，cc/ua/sanitizer全部rc0。相同组件已由unisacc交叉编译到六目标；macOS arm64和Rosetta x86_64实际执行各1,655向量通过。Linux/Windows四目标本轮仅编译成功、未执行，不称六平台绿。根.com仍发布物；包版本兼容与压缩内容缓存尚未接入，论文出货数字保持0.0.8。

压缩集成下一切片：采用正式P3（目录/资源同P2，M记录增加原长、codec=1、解压CRC32，正文为UNINETB1的raw DEFLATE）。P1/P2和独立文本.net继续读取，未知版本拒绝。先给pack/compilerpack增加显式--compressed选项，默认旧格式保留，私有候选经同一路径构建；完成读取器/字节账等消费者迁移及候选验收后才切默认并重建根.com，避免先切产品再修门禁。压缩结果按原内容、格式和zlib版本哈希缓存，命中验证存储与原文，出货--no-model-cache同时禁用压缩缓存。CRC仅损坏检测，错误流/错误原长不得进入loadbytes。

P3集成本地结果：P1/P2/P3在生产run.c中由cc sanitizer和unisacc构建执行同一网络均输出Y；10类坏包（未知版本、截断、尾部、原长过大/小、CRC、codec、翻位、非网络正文）与5类坏二进制模型均rc2且无stdout，缓存污染可重建。32个发布网络构建P3后原文/目录/资源完全恢复，32个二进制装载网络在原始审计TSV/table对应的--check-net全域全部相等（双槽队列，各步骤限时）。正式私有APE候选1,078,516 B/SHA2f1287699aac9c10e656e1653770c79d05644b14991059b435e140bd93fb7355；包851,812 B，其中网络压缩体670,428 B，二进制原文2,306,841 B。根.com保持v0.0.8，尚未切默认或宣称完整门禁通过。
五次交替中位数：同驱动P2/plain calc185.285ms，P3候选186.757ms（+1.472ms），发布物188.922ms；实际最小程序装载启动26.156/27.735ms（+1.578ms），发布物26.044ms。六目标hello -O2镜像与经典完全一致，这是生成证据、不是六平台执行。lnx/arm64既有Lima default实际执行codec的1,655向量通过（未改变其原运行状态）。Windows start在10秒超时后发现客机starting，普通stop未生效，--kill已确认stopped；Windows两目标执行仍未验。P3字节账检查压缩物理字节与压缩前记录账分别求和，避免把2.3MB原文误当1.08MB包内占比。

P3相关队列父验：固定私有MODEL_COM与私有UA，exec-net/exec-codec/exec-package/qprefix/docs五项均rc0，双槽3秒墙钟；qprefix使用真实P3候选32网络，动作前缀展开一致。独立sanitizer包检查与完整32模型全域审计另已通过。默认构建与根产物暂保持P2，待正式compilerpack从当前输入构造P3并冻结完整产品门禁后再切；这不是外部阻挡，剩余任务是默认切换前的整机门禁和未验平台证据。

正式compilerpack --compressed入口已用六目标既有manifest及当前追加模型生成器构造包，与上述候选包逐字节一致；重新从3487653的当前输入交叉构建APE，产物仍2f128769…/1,078,516 B，构建开始/结束闭包身份一致并生成私有来源侧车。下一步冻结该私有候选完整--com队列；每个滚动窗口≤55秒、双槽、结果逐套件记录，不修改根发布物或测试树。Windows启动缺口不作门禁降级或通过。

冻结前补漏与验收顺序调整：只读检查发现tests/modelcross.py包头仅接受P1/P2，须接受P3才能实跑平台烟测。为避免先验P2默认树、再改默认重跑完整队列，先将compilerpack默认切到已验证P3，保留--legacy-package与PACK_COMPRESSED=0明确旧路径，根发布物仍不替换；在这一最终构建默认上重新冻结。早期队列7/179、0失败记录保留，不称最终树完整验收，也不跨该输入变化复用。

P3完整队列首次停在69/179（68通过、1失败）：exec-native-resources的隔离自源码夹具仍只复制core.c/core.h，新run.c新增codec.h未复制；缺源文件后E2按头请求回退，因故意不传include目录而拒绝。保持这个隔离检查，不增加搜索路径，不放宽门禁；把codec.h作为实际源码依赖一并复制，单项验证后重新冻结。旧队列证据保留，不冒称完整通过。

P3最终冻结验收（f02e384）：滚动双槽--com队列179/179全部rc0，日志/private/tmp/unisacc-v009-integrated/gate-f02e384；每个窗口≤55秒、内层≤60秒。此前隔离夹具漏codec.h已修，exec-native-resources在本轮重新通过，不沿用失败前结论。决定将同一已验私有候选及其来源侧车复制到根unisacc.com，再更新model-bytes账与论文候选状态；产品字节不变（2f128769…/1,078,516 B），不发布、不推送、不混入其它优化。Linux arm64实际组件及候选烟测另已通过；Windows两目标/Linux x86_64本轮执行仍未验，Windows尝试启动的客机已确认关闭，原先运行的Lima default不改状态。文档同步后只跑docs/字节账与来源身份检查，不把文档提交说成重跑了所有产品套件。

P3根产物与文档收尾：根unisacc.com现为已验2f1287699aac9c10e656e1653770c79d05644b14991059b435e140bd93fb7355/1,078,516 B，相对v0.0.8减少79.98%；来源侧车通过当前输入闭包复核。model-bytes.json与本文件功能/字节表已捕获实际P3产物，压缩模型物理体670,428 B（整包62.16%），不再按原文5MB计算占比；论文区分已发布v0.0.8与本地已验v0.0.9候选。179项逐项rc/耗时收据归入research/compression-integrated-bench-20260928.json；文档生成、字节账和根产物身份检查另均rc0。一次/usr/bin/time -l的calc内存观察：同驱动plain最大RSS23,003,136 B、P3为18,677,760 B；只记单次内存观察，不称峰值界或中位速度，首次执行扫描耗时不并入此前交替计时。尚未发布或推送，首片结束，不追加结构去重/运行时缓存等功能。

-run单次memory私有实验决定：固定e34b91a后的P3产物为基线。先在独立scratch复制源码，增加显式预留资源，由OS选择页对齐基址、保留至多现有相对地址范围的虚拟区间，模型根据实际基址及自身算出的代码/导入长度计算数据位置，一次输出UNIMEM镜像；宿主仅校验范围、提交实际需要的页、复制与设权限。保留原text/data双资源路线作参考，不用MAP_FIXED，不硬编码地址，不把布局或ISA判断搬入C。预留/提交失败明确拒绝，不静默使用未绑定地址；Windows区分MEM_RESERVE与MEM_COMMIT。先做TDD：新资源输出与相同基址的旧双资源模型逐字节相同，缺项/超界拒绝；再做同驱动私有候选的calc与最小程序五次交替中位、应用输出与-O2文件镜像对照，证据未齐不更新根.com。25–30%仅假设；scratch步骤各≤60秒，不新造模型/测试框架。

单次memory私有实验结果（research/memory-once-experiment-20260928.json）：六目标新网络全域等于各自声明表；两个真实程序×两个基址共24组，cc/unisacc执行器输出的新旧绑定UNIMEM镜像逐字节相同，120个缺项/混合绑定/非法容量控制拒绝。修掉原型把资源存在句柄误当bool=1的错误；旧text/data接口继续可用。候选1,083,231 B，SHA bbbfce54c13a432b313ac0b135998e156768dde1453920d13f50027540ed264e；同一候选用私有UNISA_MEMORY_TWOPASS=1切旧路线，五次交替中位calc为214.751ms（双次）/177.665ms（单次），减少37.086ms、17.27%；最小程序31.293/28.732ms，减少2.562ms。同期原P3基线215.187/31.747ms，不与上一轮机器负载下的186ms混算。28组运行结果相同，六目标calc -O2文件镜像与基线不变；FFI17项、四个真实应用、Linux arm64三固定程序实际通过。
边界：2GB以内页对齐虚拟区间由OS选择，只提交实际需要的页，没有MAP_FIXED。Windows预留0x2000/NOACCESS与提交0x1000/READWRITE、失败/错误地址/越界的API契约已用host mock验证，不称Windows执行；host装载器在610MB虚拟数据范围只触碰两页，maxRSS单次约1.46MB。完整610MB C数组源码在新旧两路线都被10秒watchdog停止，未列通过，也未为它增加超时。根产品及源码未切换，完整门禁、Windows两目标和Linux x86_64执行未验；私有实验阶段交付到此，正式集成时把声明和失败控制纳入现有memory门禁，不能用这份有限证据替代完整产品验收。


单次memory正式集成决定（2026-09-28）：采用已测的OS预留地址+模型一次绑定方案，保留UNISA_MEMORY_TWOPASS=1仅作同驱动回归参考。把预留资源/混合绑定拒绝、同基址镜像对拍与预留/提交失败纳入现有memory检查，不增加重复准备的测试族。完成源集成后构建带来源身份的新P3候选，冻结双槽完整队列，最后替换根产物并测叠加的尺寸与整机速度。Windows mock与实机证据分别记账，客机不可用不写通过。
610MB完整C数组源码：新旧两路线均在10秒失败，登记为既有规模问题，明确不在本轮门禁；不放宽超时。门禁验证有界的装载器大虚拟范围/少量实际页和常规真实程序；后续若处理源码规模，先定位共享前端/生成阶段，不把它当单次memory回退。


单次memory正式冻结验收（44fc348）：新P3候选01e5c1d9d4528a2402d212883da0df64f63d60b8f17b5e6461d18e41ed053631/1,083,311 B，完整双槽--com队列179/179 rc0（34个≤55秒窗口，输入与候选全程固定）。现有memory门禁新增同基址单/双绑定逐字节比较、五类错误预留绑定、Windows预留/提交API契约的成功及四类失败，以及610MB虚拟范围仅触碰首/末页的host loader检查；arm64/Rosetta三种运行时实际执行通过。决定将同一冻结候选与来源侧车更新到根产物，完成P3+单次memory的同身份叠加测量和功能字节账。没有发布或推送。
本轮Windows实机启动在10秒限时内未完成，两次stop --kill也超时；按确认为该客机的PID结束QEMU后，utmctl stop确认“虚拟机未运行”。Windows预留/提交仍只有mock证据；Linux arm64私有候选已有实际烟测，正式候选补做同一固定组。Linux x86_64未执行。610MB完整C源码规模问题保持明确的本轮门禁外限制，不把装载器虚拟范围通过写成源码通过。


P3+单次memory正式整机测量：同一44fc348驱动前缀、同一32网络与资源，构造未压缩P2包与P3包；五次交替热运行，输出/退出/诊断完全相同。calc中位数：未压缩双次205.382ms、P3双次207.770ms、P3单次172.749ms；单次相对同驱动P3双次快16.86%（35.020ms），组合相对未压缩双次快15.89%（32.632ms），不混用早期私有实验或旧负载基线。实际最小程序启动29.907/31.749/29.072ms，组合较未压缩双次快0.834ms，较P3双次快2.676ms；只记热运行，不称冷盘结果。28组实际运行与六目标calc -O2文件镜像相同。
体积同驱动对照：未压缩5,436,486 B→P3单次1,083,311 B（-80.07%）；相对已发布v0.0.8的5,388,402 B为-79.90%，发布基线仅用于尺寸，不借用其速度。新增单次路线相比此前P3候选多4,795 B（约0.44%）。根产物SHA01e5c1d9d4528a2402d212883da0df64f63d60b8f17b5e6461d18e41ed053631，来源闭包验证通过。正式候选在原已运行的Lima default上实际完成Linux arm64 hello/fib/convert的模型-run及客机模型编译/native与host输出一致，不改其原运行状态、不称本次Linux自举。Windows两目标及Linux x86执行仍未验。完整179项收据、原始计时与再现脚本见research/memory-once-integrated-bench-20260928.json。本轮集成收尾，不发布、不推送，不继续混入AOT库/二进制阶段流等优化。

补充客机状态口径：Windows QEMU PID已退出，stop明确报客机未运行；但utmctl status仍显示starting（UTM状态残留）。没有运行中的该客机QEMU，不能把残留状态写成实测通过或“status=stopped”；本轮启动/清理异常记录保留，后续实机验收前须确认控制器状态恢复。
