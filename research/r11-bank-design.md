<!-- 设计报告（子代理按脱敏简报撰写，2026-09-29）；全部为设计，未实际执行。用于 R11-0 ⑤ 的范围决定。 -->
# BANK route: register-bank call plans — design report

Status of every claim below: **design, not executed.** No code was run, no fixture was compiled. ABI facts cited are from the SysV x86_64 psABI and AAPCS64 as commonly documented; they must be confirmed by the native probes described in §6.

## 1. Plan format (byte layout) — design

All integers little-endian. Record is self-delimiting: a header gives total length; every section has a count; no pointers, only offsets inside the record.

### 1.1 Header (fixed 32 bytes + prototype)

| Off | Size | Field | Rule (decoder check) |
|---|---|---|---|
| 0 | 4 | magic `"BNK1"` | exact |
| 4 | 2 | version (=1) | exact; unknown → reject |
| 6 | 1 | profile id (0..5: mac-a64, lnx-a64, win-a64, mac-x64, lnx-x64, win-x64) | <6; must match host profile string |
| 7 | 1 | flags: b0 result MEMORY, b1 variadic (v1: must be 0), b2..7 = 0 | reserved bits zero |
| 8 | 4 | total record length L | ≥ 32+P, == buffer length |
| 12 | 4 | prototype length P | ≤ 4096 |
| 16 | 1 | nparams N | ≤ 32 |
| 17 | 1 | GP used (0..8) | ≤ ISA max |
| 18 | 1 | FP used (0..8) | ≤ 8 |
| 19 | 1 | al value | SysV: == FP used; AAPCS64: 0 |
| 20 | 2 | stack bytes S | multiple of 16, ≤ 512 |
| 22 | 2 | move count M | ≤ 256 |
| 24 | 1 | result capture count R | ≤ 8 |
| 25 | 1 | hidden-return register (0xFF none, else GP index: SysV 0=rdi, A64 8=x8) | consistent with flag b0 |
| 26 | 2 | memcopy scratch bytes C (caller-side copies for by-ref args) | multiple of 16, ≤ 1024 |
| 28 | 4 | CRC32 of bytes [32, L) | integrity only, not identity |
| 32 | P | original prototype bytes (owned) | replayed; host compares byte-equal to the call's prototype |

Profile string binding: the id is the canonical binding; the certificate envelope (§4) also carries the profile string, and the decoder checks id↔string via a 6-row constant table.

### 1.2 Move entry (8 bytes each, M entries, sorted by (param, src_off))

| Off | Size | Field |
|---|---|---|
| 0 | 1 | param index (< N) |
| 1 | 1 | src kind: 0 = slot value, 1 = bytes at slot pointer, 2 = address of memcopy scratch |
| 2 | 2 | src offset (kind1: byte offset into arg; kind2: offset into scratch) |
| 4 | 1 | dst kind: 0 GP, 1 FP-lo, 2 FP-hi, 3 STACK, 4 SCRATCH (memcopy target) |
| 5 | 1 | dst index (GP/FP register index; unused for STACK/SCRATCH) |
| 6 | 1 | width: 1,2,4,8,16 (16 only for FP-lo full-lane or STACK/SCRATCH) |
| 7 | 1 | ext / stack-offset-hi: for GP: 0 zero-ext,1 sign-ext of width; for STACK/SCRATCH: high byte of destination offset (low byte in dst index) |

Destination byte offset for STACK/SCRATCH = `dst_index | (b7<<8)`, ≤ S (resp. C).

### 1.3 Result capture entry (4 bytes, R entries)

| Off | Size | Field |
|---|---|---|
| 0 | 1 | source: 0 rax/x0, 1 rdx/x1, 2 xmm0/v0, 3 xmm1/v1, 4 v2, 5 v3 |
| 1 | 1 | width 1/2/4/8 |
| 2 | 2 | destination byte offset in result buffer |

MEMORY result: R = 0, result buffer address itself is loaded into the hidden register.

### 1.4 Mechanical validity (decoder, no ABI knowledge) — design
1. Header checks above; L exactly consumed: `32+P+8M+4R == L`.
2. GP/FP destinations: index < used counts; each (reg, byte range) covered at most once (bitmask per register of 16 bits).
3. STACK: `off+width ≤ S`, offsets strictly increasing across entries in record order (monotonic), `off % min(width,8) == 0`.
4. SCRATCH: same, ≤ C; every kind-2 source references an offset that is the start of a SCRATCH run earlier in the list.
5. Kind-1 source: `src_off+width ≤ size of param` (size taken from replayed prototype via the same certified size table — the decoder reads a pre-emitted per-param size array, see §4, not the graph).
6. Hidden register not also a move destination.

## 2. Allocator as a finite table — design

Carried state (fits in five byte counters + one flag):
`ngrn` (0..6 / 0..8), `nsrn` (0..8), `nsaa` (stack bytes, 0..512, step 8), `scr` (scratch bytes), `fp_disabled` (A64 only; realised by setting nsrn=8), plus the move-emitter cursor `m`.

Per-parameter inputs already computed by the certifier: size, align, class kind (SCALAR_INT, SCALAR_FP, AGG), for AGG ≤16: eightbyte count `k∈{1,2}`, lane masks `cls[0..1] ∈ {INT, SSE}`, for A64: `hfa_n` (0..4) and `hfa_w` (4/8); a `memory` bit (SysV unaligned field / >16 / refused lane mix), and a `refuse` bit for negative controls.

### 2.1 SysV x86_64

```
ngrn = 0; nsrn = 0; nsaa = 0; m = 0
if result.memory: emit_hidden(GP0); ngrn = 1          # rdi consumed
for p in 0 .. N-1:                                      # N ≤ 32, declared
  if p.refuse: REJECT
  if p.kind == SCALAR_INT:
     if ngrn < 6: emit(p,val,GP[ngrn],w); ngrn += 1
     else:        emit(p,val,STACK[nsaa],8); nsaa += 8
  elif p.kind == SCALAR_FP:
     if nsrn < 8: emit(p,val,FPlo[nsrn],w); nsrn += 1
     else:        emit(p,val,STACK[nsaa],8); nsaa += 8
  elif p.memory or p.size > 16:
     if p.size > 64: REJECT
     nsaa = round_up(nsaa, max(8,p.align))              # align ≤ 16
     for j in 0 .. ceil(p.size/8)-1:                    # ≤ 8 iterations
        emit(p,bytes@8j,STACK[nsaa+8j], min(8,p.size-8j))
     nsaa += round_up(p.size,8)
  else:                                                 # 1 or 2 eightbytes
     needg = count(cls[j]==INT for j<k); needs = k - needg
     if ngrn + needg ≤ 6 and nsrn + needs ≤ 8:
        for j in 0 .. k-1:                              # ≤ 2
          if cls[j]==INT: emit(p,bytes@8j,GP[ngrn],wj); ngrn += 1
          else:           emit(p,bytes@8j,FPlo[nsrn],wj); nsrn += 1
     else:                                              # whole spill; counters unchanged
        nsaa = round_up(nsaa, max(8,p.align))
        for j in 0 .. k-1: emit(p,bytes@8j,STACK[nsaa+8j],wj)
        nsaa += 8k
al = nsrn; S = round_up(nsaa,16)
```

Key point: on whole spill the counters are *not* advanced, so later scalars still take registers (fixture family B). Tail widths `wj` are byte-accurate (`size-8j` for last lane, padded read forbidden → emitter uses width ≤ remaining bytes; the gateway zero-fills the bank first).

### 2.2 AAPCS64 (macOS/Linux, non-variadic)

```
ngrn = 0; nsrn = 0; nsaa = 0; scr = 0
if result.memory: hidden = x8                           # does NOT consume x0
for p in 0 .. N-1:
  if p.refuse: REJECT
  if p.kind == SCALAR_FP or p.hfa_n > 0:
     n = (p.kind==SCALAR_FP) ? 1 : p.hfa_n
     if nsrn + n ≤ 8:
        for e in 0 .. n-1: emit(p,bytes@e*w,FPlo[nsrn+e],w)   # ≤ 4
        nsrn += n
     else:
        nsrn = 8                                        # C.3: disable SIMD
        nsaa = round_up(nsaa, max(8, p.natural_align))  # see note
        for e in 0 .. n-1: emit(p,bytes@e*w,STACK[nsaa+e*w],w)
        nsaa += round_up(n*w, 8)
     continue
  if p.kind == AGG and p.size > 16:
     if p.size > 64: REJECT
     scr = round_up(scr,16)
     for j in 0..ceil(p.size/8)-1: emit(p,bytes@8j,SCRATCH[scr+8j],..)
     treat as SCALAR_INT whose value = addr(SCRATCH[scr]); scr += round_up(p.size,16)
  words = (p.kind==AGG) ? ceil(p.size/8) : 1            # 1 or 2
  if p.align == 16 and words == 2: ngrn = round_up(ngrn,2)
  if ngrn + words ≤ 8:
     for j in 0..words-1: emit(p,..,GP[ngrn+j],wj); ngrn += words
  else:
     ngrn = 8                                           # C.13 : NGRN = 8
     nsaa = round_up(nsaa, max(8,p.align))
     for j in 0..words-1: emit(p,..,STACK[nsaa+8j],wj); nsaa += 8*words
S = round_up(nsaa,16); al = 0
```

Notes (design): Apple arm64 differs from AAPCS64 in *stack* placement of small non-variadic scalars (Apple packs them at natural alignment, not 8-byte slots). This is not variadic-only; the plan must carry profile-dependent slot rounding: `slot = (profile is mac-a64) ? natural_size : 8`. Treat this as a separate column in the table and a dedicated fixture; until probed, **refuse** mac-a64 plans whose stack area contains any scalar < 8 bytes. Win64 fits the same record: 4 GP/FP positional slots, 32-byte shadow space expressed as `S` base offset 32, >8 or non-power-of-two by reference via SCRATCH.

State footprint: 4 counters × ≤10 bits plus 1 flag; the table threads them in its 64-slot frame; each parameter step is a bounded sub-machine (≤ 8 lane iterations), so total iterations ≤ 32 × 8.

## 3. Gateway design — design

Single C-callable entry per ISA: `bank_call(const BankFrame *f, void *fn)` where

| Off | Size | BankFrame field |
|---|---|---|
| 0 | 64 (x64: 48 used) | GP[8] |
| 64 | 128 | FP[8] × 16 bytes |
| 192 | 8 | hidden return pointer |
| 200 | 8 | stack bytes S (multiple of 16) |
| 208 | 8 | pointer to stack image |
| 216 | 8 | al |
| 224 | 64 | captured: rax/x0, rdx/x1, v0..v3 lo (8 each), v0/v1 hi |

SysV x86_64 (~70 lines): push rbp; mov rbp,rsp; push rbx; push r12 (keeps 16-alignment); save frame ptr in rbx, fn in r12; `sub rsp,S`; rep movsb from stack image to rsp (rcx=S) — rsp is 16-aligned before the call; load xmm0–7 via movdqu from FP; load rdi..r9 from GP; if hidden: mov rdi,[hidden] (plan already routed so allocator reserved GP0); `mov eax, al`; call r12; store rax, rdx, movq/movdqu xmm0, xmm1 into captured; `lea rsp,[rbp-16]`; pop r12; pop rbx; pop rbp; ret. Red zone irrelevant (we are a caller). Load r11/r10 never.

AAPCS64 (~60 lines): stp x29,x30,[sp,-32]!; mov x29,sp; stp x19,x20,[sp,16]; x19=frame, x20=fn; sub sp,sp,S; copy loop 16 bytes per ldp/stp over S; ldp q0..q7 from FP; ldr x8 hidden; ldp x0..x7; blr x20; stp x0,x1 and str q0..q3 into captured; mov sp,x29; ldp x19,x20; ldp x29,x30,[sp],32; ret.

Windows later: same frame, plus 32-byte shadow space below the image (x64) and x8 hidden rules on win-a64.

Harness: for every fixture a generator emits a C file with (a) a callee that memcpy's every argument into a global witness buffer and returns a pattern-derived result, (b) a direct compiler-generated call, and (c) a BANK call using the plan. Compare witness buffers and result bytes, with 0/5+7/7+8 preceding register-pressure args; poison-fill (0xA5) bank registers not in the plan to catch missed zeroing; compile at O0/O1/O2 with ASan/UBSan (gateway assembly excluded from instrumentation).

## 4. Interaction with the carrier route — design

Rule: BANK only when the carrier certifier returns "not covered" *and* the BANK rule family accepts. The certifier evaluates carrier first; BANK states are reachable only from carrier-refusal states, so already-accepted classes keep bit-identical certificates (regression: re-emit all existing fixtures, byte compare).

Envelope: `{tag:1 byte (1=CARRIER, 2=BANK, 0=REFUSED), profile string, version, body length, body}`; BANK body = §1 record plus a per-param size array (N × 2 bytes) so the host validator never reads the graph. Host dispatch is a switch on the tag; no field inspection beyond the validator.

## 5. Ideas with precedent and minimal experiment — design

1. **Counter-threaded allocator emitted as moves (libffi `ffi_prep_cif_machdep` / LLVM CCState).** Precedent: libffi x86-64 `examine_argument` + `ffi_call_unix64` flags; LLVM `CCState::AllocateReg`. Experiment: Python oracle for §2.1 over 20 prototypes; compare stack offsets to clang `-S` output.
2. **Assembly gateway with fixed BankFrame (libffi `unix64.S`, LuaJIT `lj_vm` call-C path, Go `asmcgocall`).** Experiment: hand-write the x64 gateway, call a 9-int-arg callee with 3 stack words; compare to direct call.
3. **Witness-callee differential harness (GCC `compat` testsuite struct-layout-1, libffi testsuite).** Experiment: generator for fixture family A, 100 iterations × 3 pressures, O0/O1/O2.
4. **Byte-lane masks as the only classification primitive (SysV eightbyte merge algorithm).** Already computed; experiment: exhaustively enumerate all 2-eightbyte layouts ≤ 16 B of 1/2/4/8/f/d fields and check the oracle's lane mask vs clang IR `{i64,double}` coercion.
5. **Hidden-return as a pre-seeded counter (LLVM sret demotion).** Experiment: struct {char[24]} return on SysV and A64, verify rdi vs x8 and that x0 is still free on A64.
6. **Scratch copies for by-reference aggregates (AAPCS64 B.4, Win64 >8 B; Clang `CodeGen` indirect args).** Experiment: 40-byte struct on A64; callee mutates its copy, caller's original must remain unchanged.
7. **Profile column for Apple arm64 stack packing (Apple "Writing ARM64 code for Apple platforms").** Experiment: 9 GP args where the 9th and 10th are `char`/`short`; dump sp-relative addresses in the callee.
8. **Plan canonicalisation and replay identity (Wasm/JIT deterministic trampolines, Cranelift ABI sig to `ABIArgSlot`).** Experiment: emit plan twice from table simulator and compiled net, byte-compare; mutate one prototype byte and confirm host rejects.

**Recommend first:** (1) the counter-threaded allocator with Python oracle, and (3)+(2) the witness-callee harness driving the x64 gateway — together these make family A and B green end to end.

**Tempting ideas that conflict:**
- Let the host classify or "fix up" a plan — violates no host classification.
- libffi raw API / `ffi_call` with MEMORY types — depends on the FFI library and its own classification.
- Universal trampoline loading all registers without a plan — correct values but no certified destination; hides missed moves and breaks al semantics.
- Recursing into nested aggregates at plan time beyond the 64-slot frame — unbounded.
- Passing the caller's pointer directly for A64 >16 B instead of a copy — callee may mutate caller memory.
- Reusing the carrier catalogue's "mixed union as u64" for SysV mixed FP/INT lanes — may pick the wrong register class.

## 6. Negative controls and first fixture families — design

Must keep refusing in slice 1: long double x87 (SysV) and IEEE128 (Linux A64); SSEUP / `__m128`/`__m256` vectors; `_Complex` of any rank; `__int128`/`_BitInt>64`; any variadic prototype with BANK (al path reserved but flag b1 must be 0); inbound callbacks/closures; aggregates > 64 B; alignment > 16; flexible array members / zero-size structs; bitfields spanning an eightbyte boundary in packed layouts; SysV unions whose lane merge is undecided by the catalogue *and* contains FP (refuse rather than guess until probed); mac-a64 plans with sub-8-byte stack scalars (until idea 7 probed); all Windows profiles (BANK tag not emitted).

**Family A — SysV packed 5-byte struct as MEMORY:** `struct __attribute__((packed)) {char c; int i;}` passed by value, alone and after 0/5/7 GP args, plus as return (hidden rdi). Expect: 8-byte stack slot holding 5 bytes (tail width 1 + 4 moves), counters unchanged, `S` rounded to 16; return variant: rdi = result buffer, GP counter starts at 1.

**Family B — SysV `{u64, double}` whole spill under GP exhaustion:** six preceding `long` args, then the struct, then a trailing `double`. Expect: struct wholly on stack at offset 0 (both eightbytes), trailing double in xmm0, al = 1. Variants: SSE exhaustion (eight preceding doubles, then `{double,u64}` spills, trailing `long` takes a GP) and the both-available control (struct in rdi-slot + xmm).

Each family ships with oracle bytes, probe at 3 pressures × O0/O1/O2 × 100 iterations, and its negative mirror (same shape with `long double` or `__int128` member → REFUSED).
