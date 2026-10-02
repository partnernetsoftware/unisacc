# cc interop plan (R20-4, plans/v0.0.21.md item 1)

Status: DESIGN ONLY, nothing run. [read] = seen in the tree on 2026-10-02; [prop] = proposed.

## 0. What exists [read]
- Private convention: args pushed on the tape stack left to right, r9 frame pointer, result rax, caller pops (README rows 421/428, prd W-16).
- `__hostcall(fn,long a[6])`: fixed byte/word sequences in src/back_encode.c: BK_HOST_X86 (SysV/Darwin x86: rdi rsi rdx rcx r8 r9, `xor eax,eax`, aligns rsp to 16, saves the tape regs it clobbers), BK_HOST_WIN_X86 (rcx rdx r8 r9 + a[4],a[5] at [rsp+32/40], 32-byte shadow, 0x80 frame), BK_HOST_ARM (x0-x5 from x17 array, `blr x16`, saves x1-x7, x9, x30, sp aligned). Integer only; no FP registers; indirect call through a register.
- Objects: undefined names become BK_OBJUND + (id<<20)+off; a call to an undefined label emits `E8 rel32` with bk_relcall type 900 (-> R_X86_64_PLT32 in ELF, X86_64_RELOC_BRANCH in Mach-O) (back_encode.c:1065, back_image.c:336/435). arm64 analogue goes through adrp/add relocations.
- bk_symplan (back_image.c:262): kind 1 local, 2 global (bklink_global or _start), 3 undefined; bk_sprefix drops a `g_` prefix for exported/undefined DATA; Mach-O adds a leading `_`.
- Front end: sympk (param kinds), sympkw (widths, 8 for pointers), symretw (int return width 1/2/4, 0 = 8/other), fwd_stub (front_parse.c:6099) and src/fwdstub.c already derive per-function C stubs from those for -run.
- Tests: tests/elfobj.sh/.py link `-c -b` whole-program objects with ld.lld / ld / ld64 / lld-link, run arm64 Linux in Lima (ELFOBJ_VM) and COFF in the Windows VM only when ELFOBJ_WINVM is set; tests/linkunits.sh compiles units with `-funit` and links with unisacc's own linker, compared against `cc a.c b.c`.

## 1. Outbound: our code calls an extern cc function [prop]
For every called function that is declared but undefined in all units (bksp_kind 3 candidate) and that has a prototype:
1. Front end records a signature descriptor: per parameter class {INT(width,signed), PTR, F32, F64}, return class, variadic flag + fixed count. Source: sympk/sympkw/symretw; add a return kind (F32/F64/INT/PTR) since symretw only carries int width. No prototype -> refuse by name (`interop: no prototype for NAME`), never guess.
2. Emit a thunk `__cc_NAME` (local, kind 1) as tape code; the call site keeps the private convention and calls the thunk. The thunk:
   - reads the pushed args from the tape frame (r9-relative, arg k at fixed offset since pushes are left to right);
   - assigns them per the target table (section 3): next INT reg / next FP reg / stack slot (Win64: positional, slot i decides reg for both classes);
   - saves the tape registers the host may clobber (same set the BK_HOST_* sequences save today, plus r9), aligns sp to 16, reserves Win64 32-byte shadow;
   - `call NAME` via a relocated direct call to the undefined symbol;
   - moves the result: INT rax/x0 (sign/zero-extend by width), FP xmm0/d0/s0 -> our FP result location (the regmap's FP result reg; cvtss for F32 as the private convention uses);
   - restores and returns; the caller still pops.
3. Missing ops (tape + ISA):
   - TO_HOSTARG_I k, TO_HOSTARG_F k (move tape value to host INT reg k / FP reg k, single or double); TO_HOSTSTK off (store outgoing stack arg); TO_HOSTRES_I w/s, TO_HOSTRES_F f32/f64.
   - TO_CALLX sym: relocated direct call to an undefined symbol. x86: E8 + type 900 (exists for labels; generalise to a named symbol). arm64: `bl` + R_AARCH64_CALL26 / ARM64_RELOC_BRANCH26 / IMAGE_REL_ARM64_BRANCH26 (new reloc type, e.g. 901). COFF x64: IMAGE_REL_AMD64_REL32.
   - Encoders: movq xmmN,reg / movd (x86), fmov dN,xN / fmov sN,wN (arm64), cvtsd2ss for float args; Darwin x86 variadic `mov al,nfp`.
   - Simplest route: generalise BK_HOST_* into a generated sequence instead of fixed bytes (they become the k=6 all-INT special case; keep the old bytes until equal by test).

## 2. Inbound: cc calls our exported function [prop]
- For each defined function with external linkage that is address-taken or exported (non-static and the unit is `-c` without -funit, or a new `-fcc-export` / per-object policy): emit wrapper with the C name NAME and rename the body to a tape-private name (e.g. `__u_NAME`, kind 1).
- Wrapper entry (host ABI): save callee-saved host regs that tape code uses as scratch (x86 SysV: rbx rbp r12-r15; Win64 also rdi rsi xmm6-15; AAPCS64: x19-x28, x29, x30, d8-d15) - plus r9/x? frame register setup; push args in the private order (left to right) from INT/FP host regs and incoming stack slots; `call __u_NAME`; pop; move rax/FP result to host return reg (extend per width); restore; ret.
- Symbol plan: wrapper gets bksp_kind 2 under the plain C name (Mach-O `_NAME`, no `g_` stripping needed for functions); body stays kind 1. Calls inside our own units still go to `__u_NAME` directly (no double hop). Callbacks: taking `&f` of an exported/address-escaping function yields the wrapper address, so a function pointer passed to cc (qsort comparator) is host-ABI. Conversely, an indirect call through a pointer whose target may be foreign needs a dynamic thunk: proposed rule - function pointers ARE host-ABI everywhere once interop is on (all address-taken functions get wrappers; indirect calls use the outbound thunk sequence with the pointer as target). That is the only consistent choice for callbacks both directions.
- -funit units keep their own linker path; interop wrappers only in system-linker objects at first.

## 3. Per-target ABI table [prop, from the public ABIs]
| target | INT args | FP args | stack | align | variadic | callee-saved we must respect | notes |
|---|---|---|---|---|---|---|---|
| SysV x86-64 (lnx, osx/x86_64) | rdi rsi rdx rcx r8 r9 | xmm0-7 | rest right-to-left, 8-byte slots | rsp%16==0 at call | al = #xmm used | rbx rbp r12-r15 | r9 is our frame ptr AND arg 6: thunk must load r9 last, after reading the tape frame |
| Win64 x64 | rcx rdx r8 r9 positional | xmm0-3 positional (same slot index) | slots 5+ at [rsp+32+8i], 32-byte shadow always | 16 | FP vararg also copied to INT reg | rbx rbp rdi rsi r12-r15 xmm6-15 | BK_HOST_WIN_X86 already does shadow/positional for ints |
| AAPCS64 Linux | x0-x7 | v0-v7 (independent counters) | 8-byte slots | sp%16 always | variadic same as fixed (va_list struct) | x19-x28 x29 x30(lr) d8-d15 low 64 | x16/x17 IP0/IP1 scratch (veneers clobber them) |
| Darwin arm64 | x0-x7 | v0-v7 | narrow args packed at natural alignment on stack (not 8-byte) | 16 | ALL variadic args on stack, 8-byte slots | same as AAPCS64; x18 reserved | caller extends narrow int args to 32 bits only; callee must not assume 64-bit extension |
| Win arm64 | x0-x7 | v0-v7 | 8-byte slots | 16 | variadic: FP passed in INT regs x0-x7 | x19-x28 x29 x30 d8-d15; x18 TEB reserved | no shadow space |
Our side: the tape-reserved registers (r9 frame; the arm64 equivalents per regmap, and BK_HOST_ARM's saved x9) must be saved across every host call; x18 must never be used by tape code on Darwin/Windows arm64 (check regmap).

## 4. Test plan [prop]
New suite tests/ccinterop.sh + tests/ccinterop.py, modelled on elfobj.py (same skip/STRICT semantics, bound 60 s, one probe per row):
- Fixtures tests/ccinterop/*: pairs `a_cc.c` (compiled by cc -c) + `b_ua.c` (unisacc -c -b HOST), and the swapped roles. Rows: ints of widths 1/2/4/8 signed/unsigned (extension bugs), pointers (strings, arrays, struct pointers), double, float, mixed 9+ args (stack spill + interleaved INT/FP counters, Win64 positional), returns of each class, variadic outbound to printf with doubles (Darwin arm64 stack rule), callbacks: cc qsort with our comparator, our code calling a cc function pointer, recursion across the boundary (cc->ua->cc).
- Oracle: output of `cc a_cc.c b_ua.c` both halves by cc; must be byte-equal.
- Targets: osx/arm64 native, osx/x86_64 via Rosetta (cc -arch x86_64), lnx/arm64 + lnx/x86_64 in Lima (cc and ld inside the VM; tree piped in like tests/linux.sh), win/* in the UTM VM: Windows VM has no cc guarantee -> link on host with lld-link against the cc half compiled by `clang --target=*-windows-msvc -c` (elfobj already links COFF with lld-link) and run in the VM; skip by name if lld-link/clang targets absent.
- Plus refusal rows: unprototyped extern, struct by value (until slice 3) -> exact error text.

## 5. Minimal first slice [prop]
Slice 1: outbound only, INT/PTR args (<=6, no stack args) and INT/PTR returns, osx/arm64 + lnx/arm64 + lnx/x86_64. Work: signature descriptor, thunk generation from existing BK_HOST_* register order, TO_CALLX with ELF/Mach-O relocations, refusal for FP/variadic/struct with names.
Gate: ccinterop rows int/ptr/width-extension on the three targets green, elfobj + linkunits unchanged, ua self-build byte-identical (thunks only emitted when an undefined prototyped function is called in non-funit -c mode, so existing images do not change). Slice 2: FP + stack args + inbound wrappers + callbacks; slice 3: Win64 x64/arm64, Darwin variadic, osx/x86_64; slice 4: small structs by value.

## 6. What the product (exec/ delta networks) must mirror [prop]
- Front-end δ: the signature descriptor (param/return classes, variadic) as a table output, same refusal text.
- Lowering δ (exec/lower): thunk/wrapper tape code generation, deterministic order and names (`__cc_NAME`, `__u_NAME`) so tape is byte-equal with unisa/lower.py / src/back_lower.c.
- Encoder δ (exec/enc): new ops TO_HOSTARG_I/F, TO_HOSTSTK, TO_HOSTRES_*, TO_CALLX, and the new relocation type(s) in elfimage/machoimage/coff layouts; symbol plan kinds for wrapper vs body.
- Anything not mirrored must be refused by the δ as "not covered", never accepted with different bytes (CLAUDE.md exec/ rule).
