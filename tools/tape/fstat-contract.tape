.bss statbuf 144
_start:
  imm r4, 1
  .argv r0, r4
  imm r1, 0
  imm r2, 0
  .sys open, r0, r1, r2
  mov r6, r0
  .lea r1, statbuf
  .sys fstat, r0, r1, r2
  jumpz r0, .Lfields
  jump .Lbad
.Lfields:
  .lea r1, statbuf
  load64 r4, [r1+48]
  imm r5, 3
  eq r4, r4, r5
  jumpz r4, .Lbad
  .ld r4, [r1+24], 4
  imm r5, 61440
  and64 r4, r4, r5
  imm r5, 32768
  eq r4, r4, r5
  jumpz r4, .Lbad
  mov r0, r6
  .sys close, r0, r1, r2
  .exit r0
.Lbad:
  imm r0, 1
  .exit r0
.end
