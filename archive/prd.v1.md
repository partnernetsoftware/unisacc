# UNISA SH as a CLI (no UI)

You are implementing **UNISA SH** from spec. Do not invent a web app, React, or a demo page. Ship a **command-line compiler + trainer + inferencer** that trains tiny table nets, compiles a C99 subset to a generic tape, lowers the tape onto 6 ISAs, executes with identical stdout, and writes real weight blobs + object images to disk.

Language: **Python 3.11+**, stdlib only (no numpy/torch). One package `unisa/`. Optional later: a tiny C gemv kernel. Do not start with Rust unless Python CLI already passes the acceptance tests below.

Work until `unisa train && unisa run examples/hello.c --fold` prints 6/6 match. Then stop.

---

## Thesis (non-negotiable)

Shell = **inferencer + executor + model data**.

- Walker / reloc / ELF-MachO-PE headers are **classic code** (algebra).
- Nets only do **last-mile table selection** (discrete keys → 1 class).
- Gold tables are the verifier. Until a net reaches **acc ≥ 0.85** on its **full** gold corpus, the walker uses gold. After that, use the net. Hot-skip training when acc ≥ 0.995 except every 6th step.
- One kernel everywhere: **embed → gemv → ReLU → gemv → argmax**. Deploy has **no softmax, no libm**.
- 6 targets, bit64 only: `lnx|osx|win` × `x86_64|arm64`. Same tape, six images, stdout/exit must match.
- Swap weights to change capability. Kernel does not change.

Do not neuralize the recursive-descent walker, symbol table, or object headers. Do neuralize every table-shaped stage.

---

## CLI

```
unisa train [--epochs N] [--holdout none|random15|osx/arm64] [--out weights/]
unisa acc
unisa compile in.c -o out --target lnx/x86_64
unisa run in.c [--target T] [--fold]
unisa tape in.c
unisa lower OP --target T
unisa ship [--dtype i8|f16|f32] [--out kit.zip]
unisa dump-weights [--dtype i8] --out weights/
```

`unisa run file.c --fold` is the product. Train on first run if weights missing.

---

## Pipeline (C → image)

```
src
 → pp      TableNet  dir × defined → take|skip|pop|macro
 → lex     TableNet  charclass × peekclass → act
 → parse   TableNet  NT × TOK → production
 → type    TableNet  t1 × op × t2 → ty
 → scope   TableNet  ctx × kind → action
 → irsel   TableNet  family × flavor → recipe
 → isel    StageNet  op×arch → form,symbol,gate
 → abi     StageNet  op×os×arch + bilinear → sysno,arg0-2,ret,tls
 → enc     TableNet  op × os × arch → syscall|svc|winapi|x86|arm
 → reloc   TableNet  jmpkind × arch → rel32|arm26|arm19
 → exec    classic   assemble + wrap ELF/Mach-O/PE + interpret
```

Gold fallback until acc ≥ 0.85 on the **full** corpus. Drive: `spec` (default) = isel∘abi; `combo` = one UnisaNet for 9 heads.

---

## TableNet

```
embed keys (d=8) → gemv W1[h0,16]+b1 → ReLU → gemv W2[16,nout]+b2 → argmax
train: softmax CE, Adam β1=0.9 β2=0.999
init: E N(0,0.08); W1 N(0,sqrt(2/h0)); b=0; W2 N(0,sqrt(2/h))
rng: mulberry32
```

lex/pp d=6 h=12; reloc d=6 h=8. Seeds: parse13 type17 scope19 pp23 enc29 lex31 reloc37 irsel11.

`fit`: hot-skip if acc≥0.995 and step%6≠0. Always evaluate FULL gold, never batch acc for ready.

NET_READY=0.85 NET_HOT=0.995

---

## StageNet / combo

isel: emb op12+arch8, h1=24 h2=16, heads form/symbol/gate, seed 3.
abi: emb op12+os8+arch8, h1=32 h2=20, heads sysno/arg0-2/ret/tls, interact bilinear 8d, seed 5.

combo ~10.5kθ: E_op[N,16] E_os[3,8] E_arch[2,8]; factor Wos/War → 12d ReLU; h0=52; W1 52×48 W2 48×32; 9 heads; shared W_reg for 4 reg heads.

LR: <18 → 0.032; <50 → 0.014; <90 → 0.006; else 0.0025. Batch 16.

---

## Gold (copy)

**parse** NT={top,stmt,unary,postfix,after_name} × TOKS → PRODS
default top=global stmt=expr unary=prim postfix=done after_name=var_def
overrides: top/eof=end top/typedef=typedef top/struct=struct top/enum=enum after_name/(=fn_sig stmt/type=decl stmt/{=block stmt/if,while,for,do,switch,case,default,return,break,continue = same-name unary/-=neg unary/!=not unary/*=deref unary/&=addr unary/sizeof=sizeof postfix/[=index postfix/(=call postfix/++/--=inc postfix/./->=field

TOKS = eof type id num str if else while for do switch case default return break continue sizeof struct typedef enum { } ( ) [ ] ; , = += -= *= /= ? : + - * / % == != < > <= >= && || ! & ++ -- . ->
PRODS = end fn global typedef struct enum decl if while for do switch case default return break continue block expr neg not deref addr sizeof prim index call inc field done fn_sig var_def

**type** TYS={void,i8,i32,i64,ptr,arr,struct,fn} × TOPS={+ - * / % < == = & [] . call sizeof , un*} → TYS|illegal
default illegal (keep 1/19 illegal in train). numeric arith→i32 if either i8 else i64; <>== → i64; = → lhs; sizeof→i64; ptr/arr+num→ptr; []→i64; ptr-ptr→i64; ptr un*→i64; fn call→i64; i64&i64→ptr; struct. → i64. TY_SIZE void=1 i8=1 i32=4 else 8.

**scope** CTX={top,param,local,expr,sizeof,field} × KIND={type_kw,id,typedef_id,star,lparen} → ACTS={bind_global,bind_param,bind_local,lookup,type_name,fn_name,field}
default lookup. top/type_kw|typedef_id=type_name top/id=bind_global top/lparen=fn_name param/id=bind_param local/id=bind_local local/type_kw|typedef_id=type_name sizeof/type_kw|typedef_id=type_name field/id=field

**pp** DIRS={ifdef,ifndef,if,elif,else,endif,define,include,undef} × {0,1} → take|skip|pop|macro
default skip. ifdef 1=take 0=skip; ifndef inverted; if/elif/else follow flag; endif=pop; define/undef=macro; include=skip.

**lex** CHARC={ws,nl,A,d,q,sq,slash,star,punct,eof,other} × peek → skip|nl|ident|num|str|charlit|cmt|linecmt|op|bad
ws→skip nl→nl A→ident d→num q→str sq→charlit slash/star/punct→op eof→skip other→bad; slash×slash=linecmt slash×star=cmt

**enc** op×os×arch → syscall|svc|winapi|x86|arm
win && not instr → winapi; arm64 && not instr → svc; syscall form → syscall; else arch.

**reloc** {jmp,jz,call}×arch → rel32|arm26|arm19
x86_64=rel32; arm64 jz=arm19; else arm26.

**irsel** family×flavor → recipe
alu:add/sub/mul/lt/le/gt/ge/eq/ne/neg → add64/sub64/mul64/slt64/sle64/slt64/sle64/eq/ne/sub64
mem:load/store/lea/ld/st/zero → load64/store64/lea/ld/st/zero
ctrl:jump/jumpz/ret → jump/jumpz/ret
call:call/push/arg/frame → call/callpush/arg/frame
lit:imm/print/write/exit → imm/print/write/exit

---

## Catalog syscalls (lnx-x64 / lnx-arm / osx / win)

exit 60/93/1 ExitProcess; read 0/63/3 ReadFile; write 1/64/4 WriteFile; open 2/56/5 CreateFileW (arm lnx=openat); close 3/57/6 CloseHandle; mmap 9/222/197 VirtualAlloc; munmap 11/215/73 VirtualFree; mprotect 10/226/74 VirtualProtect; getpid 39/172/20 GetCurrentProcessId; clock_gettime 228/113/116 QPC (osx gettimeofday); nanosleep 35/101/240 Sleep; futex 202/98/515 WaitOnAddress (osx ulock_wait); socket 41/198/97 WSASocketW; connect 42/203/98; bind 49/200/104; listen 50/201/106; accept 43/202/30; clone 56/220/360 CreateThread (osx bsdthread_create); execve 59/221/59 CreateProcessW.

osx sysno = 0x02000000|nr. ALU/mem/ctrl also: add64 sub64 xor64 mul64 slt64 sle64 load64 store64 jump jumpz call ret nop cas64 fence syscall_gate tls_base cycle_counter stack_enter.

ABI: lnx/osx x64 args rdi rsi rdx r10, ret rax, gate syscall; lnx arm x0.. gate svc#0; osx arm svc#0x80; win x64 rcx rdx r8 r9 form winapi.

Bytes: add64 `48 01 f0` / `00 00 01 8b`; ret `c3` / `c0 03 5f d6`; syscall `0f 05` / `01 00 00 d4`. Derive 9-head gold from a function, do not hand-label.

---

## Tape / C99 / images

Tape: line ops, labels `L:`, builtins `.print .frame .arg .st .ld .lea .zero .div .mod .write .exit`. r0–r7, 64KB LE mem. Interpreter is source of truth for --fold.

C99 walker: #if family, fn/ptr/arr/struct/typedef, if/for/while/do/switch, arith, printf→.write. Examples hello/fact/ptr/fib/switch/struct/do must 6/6. host.c may differ by OS, not by arch.

Wrap ELF64 / Mach-O 64 / PE32+. Do not execve; interpret the exec stream.

---

## UNS1

Header 16B: `UNS1` dtype u8 flags u8 nTensors u16 nParams u32 acc f32. Tensor: name[16] rows u16 cols u16 scale f32 payload pad4. i8 scale=maxabs/127.

Ship zip: weights/*.unisa + MANIFEST.json + kernel/unisa_boot.c + images. ~50KB i8.

---

## Train / accept

Epoch loop: combo+isel+abi batch16; front nets; eval FULL gold. Stop combo≥0.985 and all tables≥0.85 and epoch>30, or epoch 90.

Must: `unisa run examples/hello.c --fold` → 6/6 "hello from C99\n"; fact→120; switch→6; do→3; ELF magic 7f454c46; parse.i8.unisa starts 554e5331.

Order: TableNet+gold → lowering → tape VM → C walker → wrap → ship. If acc stuck <0.85, gold key encoding is wrong — do not widen the net.


