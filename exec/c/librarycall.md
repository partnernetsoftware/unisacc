# Host to script bridge (macOS first slice)

The bridge adapts an actual script code address to its **custom** register and
return-stack protocol. It does not turn the raw symbol map into a native ABI.
Only six fixed 64-bit integer or pointer argument slots and a 64-bit scalar
integer/pointer result are supported. FP, aggregates, variadic signatures and
more than six arguments fail the signature guard. Signature discovery and
public export filtering are separate work; callers must supply a correct
signature. Each invocation needs a private downward-growing, writable soft
stack with a 16-aligned top, sufficient depth, and any desired guard pages.

- ARM: host x0/x1/x2 hold entry/args/top. Save x19–x30 and d8–d15 on the
  16-aligned hardware stack. Load script x0–x5, write bridge continuation at
  top−8, set x7 there and branch to entry. Script ret consumes that slot using
  x17, restoring x7 to top. Restore the host hardware-stack frame and return.
- x86: host rdi/rsi/rdx hold entry/args/top. Save rbp/rbx/r12–r15. Switch rsp
  to top−16 and put the host rsp in that sentinel slot. Load script
  rax/rdi/rsi/rdx/rcx/r8 and call entry. The script's ret leaves rsp at the
  sentinel; restore host rsp from memory and then the preserved registers.
  No script register is assumed to preserve the host rsp.

Normal balanced script returns are covered. Script exit, traps, stack overflow,
non-local jumps and interrupted calls require host-context recovery mechanisms;
this bridge alone does not catch them. Other OS ABIs are not supported here.

`librarycallcheck.py PRIVATE_UNISACC ARCH` runs on a frozen private snapshot.
The supplied unisacc builds a real source fixture and Mach-O image. Its actual
text bytes are compared with independent reference assembly and embedded into
a host harness; the bridge calls the actual emitted functions, not reference
bytes. The fixture covers six parameters, recursive/nested calls, local arrays,
pointer return/mutation and 100 same-thread repetitions. An assembly control
clobbers every platform callee-saved GPR (and ARM d8–d15), checks their exact
restoration and the hardware SP, and tests signature/alignment rejection.
All subprocesses have a 25-second bound; wrap each ISA run with ≤60 seconds.


## Declared ALL_STACK entry

`us_library_bridge_stack_raw(entry, slots, softtop, count)` is the mechanical
ALL_STACK ScriptPlan implementation. The model selects that plan; the bridge
copies all `count` raw uint64 words in source order. FP occupies raw bits and
an aggregate slot contains a separately owned object's address. This operation
does not convert native types or discover a signature. It does not by itself
make `us_sym` support FP, aggregates, variadic signatures or >6 parameters.

The checked `us_library_call_stack` wrapper rejects a misaligned top, null
entry/output, missing nonempty slots, count >1024 and insufficient supplied
stack space before entering the script. Its size check covers the argument
block plus bridge overhead, **not an arbitrary callee's stack depth**. The
context must supply sufficient guarded stack for that depth.

ARM places the continuation at `top - 8*(count+1)` and parameter zero eight
bytes above it; the compiled callee then exposes FP+16+8*k. Hardware SP stays
on its native frame. x86 copies the parameters before the return-address
push and uses the script ABI's preserved caller frame register r6 (r9) to
recover the native saved frame. Only balanced compiler-produced C entries
that preserve r6 satisfy this contract; arbitrary raw instructions do not.
The legacy register bridge and its adversarial clobber control are unchanged.
Windows adapters preserve their host nonvolatile registers and emit host-frame
unwind records; exception propagation while on a guest stack remains unproved.

`librarystackcheck.py PRIVATE_MODEL_COM ARCH` invokes actual model-produced
Mach-O function bytes: nine mixed integer/FP/pointer parameters, nine float
parameters, a nine-parameter by-value struct argument (caller unchanged),
seventeen weighted integer parameters and a nested all-stack call.
It checks raw result bits, 100 repetitions, hardware SP, a low-stack canary and
pre-entry guards. Native ARM and Rosetta x86 runs are separate gate jobs. This
is a script-frame test, not complete native FFI or six-platform qualification.
