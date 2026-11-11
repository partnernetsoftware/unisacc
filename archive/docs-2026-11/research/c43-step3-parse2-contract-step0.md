# c43 step 3 draft: parse2 external-contract stream (design only; every claim below is "read", nothing was run)

Scope note: "parse2" = stage E3 (exec/pipeline/stages.tsv:E3, `gen.py parse2`). Paths are relative to the repo.

## 1. Inventory of what E3 reads besides tokens
Resources reach the core through `core_host_fetch` (exec/c/run.c:610-640): lookup order is process resources `RI` (CLI/library adapters), then packaged resources `RES`, then a disk fallback. [read]

| Input | Where it enters | Class |
|---|---|---|
| Model tables: exec/parse2/*-manifest.tsv, *-template.tsv, exec/facts/*.tsv (e.g. libraryimports.tsv, signature-layout.tsv, k2-libraryenv.tsv; header lines list input sha256) | baked into the model package; E3 rows name facts, e.g. exec/parse2/libraryimports-manifest.tsv:3-14 | (a) language rule / fixed tables |
| `\0hdr/*` bundled headers (include/*.h) | carried in the package (exec/c/modelboot.py:37 requires them; exec/c/PACKAGE.md:85); found via `RES` first (run.c:614) | (a), pinned by package identity |
| `\0cli/defines,undefines,includes,include-dir,nostdinc,source,fno-trim-libc,werror,error-limit,f1,attributes,target,run-forward,ccw,object,funit` | exec/c/compiler.c:677-733 (CLI), exec/c/libunisacc.c:625-643 (library) | (b) per-compilation request. `\0cli/target` is a request, but it selects layout/ABI facts (compiler.c:732-736) |
| `\0library/bindings` (USBIND3 blob: names, ids, **addresses**, dispatcher/plan, typed sigs) | libunisacc.c:645-648 from `us_resolver_freeze...` (libunisacc.c:610-613); E3 reads the address at exec/parse2/libraryimports-result.tsv:235-254 | (b) by shape, **(c) by content**: the address field is a live host pointer, and E3 writes it into the Tape (cut2-step0.md:7, result.tsv:505,516,534) |
| `\0library/symbols,module,callables,callablemake,callablecall,signatureversion,sourcefacts` | libunisacc.c:631-659 | `symbols/module/callables/signatureversion/sourcefacts` = (b) switches (value 1/3). `callablemake/callablecall` = **(c)** (raw function addresses of the library's dispatchers, libunisacc.c:626) |
| `\0process/argc,argv,dl/0..3` (+ Windows own-imports) | compiler.c:675-700 (-run only) | (c) host addresses and process vector; in the parse2/lower side they feed `.argc/.argv/.hostaddr`. dl slots are zeroed at this point (compiler.c:690 comment: bound after lower) |
| `\0memory/text,reserve,data` | compiler.c:890-905 | (c) mapping addresses, but only on the lower/enc route, not E3 |
| USLFW1 side-car (names of undefined libc functions + USLSIG3 signatures) | E3 output when `\0cli/run-forward` is set (compiler.c:164-166) | product of E3, not an input |
| Disk fallback for any non-`\0hdr/` key, and `INCDIR/<hdr>` when the header is not in `RES` | run.c:618-626; documented in research/c43-host-env-inputs.md:19 | (c) |
| `UNISA_MAXSTEPS`, `UNISA_ATTRIBUTES`, `UNISA_CONTAINER` env | run.c:648-650, 756-779 | (c) in dev builds; maxsteps is a constant in product builds (run.c:646-648) |
| `getcwd` in forward front | exec/c/fwdpaths.h:10-14 | (c), adapter-side (makes forced-include paths absolute) |

## 2. Class (c) items and how they enter
1. Addresses inside `\0library/bindings` (E3 turns them into immediates in the Tape; libraryimports-result.tsv:534) and `\0library/callablemake/callablecall`. Source: the library's own registered/host symbols (resolver, libunisacc.c:610).
2. `\0process/*` and `\0memory/*` addresses/vectors (-run, lower/enc side).
3. Disk fallback in `core_host_fetch` (any key not found; INCDIR for headers).
4. Env: `UNISA_MAXSTEPS` (non-product builds only), `UNISA_ATTRIBUTES` (reads and writes a host file; run.c:767-779).
5. `getcwd` (adapter, fwdpaths.h) - only rewrites forced-include paths.
No host header, host ABI probe or compiler/OS query was found in the E3 input set: layouts come from the language model plus `\0cli/target`. [read; "none found" for host headers]

## 3. How external declarations reach E3 today
- Bundled-header functions (printf, malloc, ...): declarations are ordinary C text in include/*.h, preprocessed by E2 into the token stream. Signatures and layouts are **reconstructed by E3 from source** - there is no separate contract stream. This path is pure (a).
- libc forwarding (-run): E3 emits the USLFW1 side-car from those same prototypes (compiler.c:164-166); the host symbol is looked up later by the generated stub (`dlsym`, src/fwdstub.c) - after E3. So E3 sees no host facts. [read]
- Library mode (`us_add_symbol`, `us_declare_import[_typed]`, libunisacc.h:54-74): declarations go into `c->bindings` / `c->resolver`, are frozen into the USBIND3 blob with addresses (libunisacc.c:610-613) and passed as `\0library/bindings`. Signature/layout facts are explicit inputs here (USLSIG2/3 descriptors inside the blob), but they are mixed with addresses in one resource, so the stage identity of E3 depends on host pointers.
- New (uncommitted, working tree): `us_binding_contract` (libunisacc.c:303-325) produces address-free USLCON1 by a separate route `library-contract`; comment says "Declaration observer only", it is not yet an E3 input. The dirty tree adds `rows librarycontractimports` to libraryimports-manifest.tsv:5 and `unisa/libraryfunctionimports.py` (reference for ordinary kind=0/callkind=0 wrappers; `.libraryfn r1, ID` instead of an address immediate). [read]
- Include dir ABI table tests/abi.manifest lists bridge hashes, callbacks (`\0process/*`, `\0library/*` with their sources), exports, resources, tables, dlslots (sections counted: resource 70, table 185, callback 17, ...). It describes the **host/library bridge ABI**, not per-program declarations; it is a container for reviewed ABI drift, not a stream E3 can read (c43-plan-proposal.md:20 says the same).

## 4. Proposed minimal first cut
Goal: E3 gets library function facts only from USLCON1; addresses never enter E3. This is cut2b's stream, promoted to "the" parse2 contract input. No new format.

Stream (existing, research/c43-late-binding-cut2-step0.md:7): `magic USLCON1\n, target u8, version u8=1, reserved u16, count u32` then records `len u32, id u32, kind u8, callkind u8, sigv u8, flags u8, nameN u32, sigN u32, extent u64, nargs u32, variadic u8, reserved[3], name, sig`. Ids 256+sorted-name index. Sig = 48-byte descriptors (sigv0) or USLSIG3 (sigv3). Unknown bit/target/duplicate id rejected.

Replaces for the covered subset: the address, dispatcher and plan fields in `\0library/bindings` for kind=0, callkind=0 (cut2b a/b). Keep for this cut: typed, variadic, callback, data, bridges, `\0process/*`, `\0memory/*` (old path, named not-covered where mixed). Do not widen to bundled headers: they stay source text.

Two cheap additions that fit step 3 and need no new format:
1. Make the contract the only per-compilation declaration resource: `\0library/contract` = USLCON1; adapter (libunisacc.c:645) stops handing addresses to E3 for the covered kinds.
2. Close the disk fallback for contract/resource keys (run.c:618-626) in strict mode, so a missing contract is "resource missing", never a file read.

Acceptance check (dynamic C black box, product CLI/library API; sketch like tests/liblatebindcheck.cx):
- Library probe declares `long hostadd(long,long)` twice in two contexts with the same source but different addresses A and B (`us_add_symbol`). Compile both. Require: `us_binding_contract` bytes equal; the **E3 Tape bytes (`us_tape`) equal** (this fails today because the address immediate is in the Tape, libraryimports-result.tsv:534); after relocate, calls return A's and B's results. Negative: a declared import with no provider fails at relocate with a named missing-provider error, not at compile. Today the first assertion (Tape equality) fails; after the cut it passes for ordinary-function imports. Whole-Tape equality is stronger than cut2b's fragment API; if the Tape still carries other addresses (process/dl slots at the old path), restrict the program to ordinary calls and no `-run` features.
- A second check for the stream itself: change only one field of one contract record (e.g. an argument width); the E3 output must change in exactly that function's wrapper, and a missing or truncated record must reject by name. (matches c43-plan-proposal.md:20 "字段改动精确改变相应阶段输入；缺签名明确拒绝".)

## 5. Tempting but conflicting
- Hashing the whole tests/abi.manifest into E3 identity: any bridge/ABI change would invalidate every stage (c43-plan-proposal.md:20 forbids it); also it holds function body hashes, not declarations.
- Generating the contract from host headers/`dlsym`/`nm`: puts host state in the stream; breaks determinism across hosts.
- Moving bundled headers to a contract stream: it would replace the source-text path that already matches the reference byte-for-byte (the reference E3 reads the same text); the reference has no stream, so byte equality would need a second reference. Keep them as source.
- Dropping addresses without changing `lower`: the lower/enc inputs still carry them (cut2b b, `exec/lower/librarymodule-result.tsv:25,32`); claiming "E3 address-free" is fine, "whole pipeline address-free" is not.
- Making `\0cli/target` implicit/host-detected: target selects layout; it must stay an explicit request (compiler.c:732-736), never `uname`.
- Running the contract through the old USBIND3 decoder with address=0: would change the old-path bytes and mix owners (cut2b a: duplicate-owner/missing-owner rules).
- Using `UNISA_MAXSTEPS` or env-based budgets as "contract": product must keep the constant (research/c43-host-env-inputs.md:25).
- Treating a USLCON1 with sigv0 as ABI-certified: it only covers six-register integer/pointer functions (unisa/libraryfunctionimports.py docstring); all other shapes must reject "not covered", not fall back silently.

## 审读意见（cc，2026-10-14）

- 第 3 步的第一刀与晚绑定 2b 是同一件事：把无地址的 USLCON1 作为 E3 的按次显式输入、只覆盖普通函数。不另开一刀，验收并入 2b（`tests/liblatebind2b.c`）。
- 本稿列出的宿主派生输入，按归属排期：`\0library/bindings` 里的地址（2b 迁普通函数，其余类别后续刀）；`\0library/callablemake`/`callablecall`、`\0process/*`、`\0memory/*` 运行桥（后续刀）；`core_host_fetch` 的磁盘回退（第 1 步，现状记录见 `research/c43-host-env-inputs.md`）；`UNISA_MAXSTEPS`（cc2 窄修，出货为常量）。
- 新增一项待查：`exec/c/fwdpaths.h` 用 `getcwd` 把转发合成单元里的相对 `-include` 改成绝对路径（df1ce7ed）。要确认这个绝对路径是否会进入核心阶段可观测的输出（诊断、`__FILE__`）；若会，则与“逻辑源名 = 命令行写法”的决定冲突，需改为只在宿主读文件时用 cwd。
- 本稿未执行任何命令，所有结论为读源码。
