.bss fnptr 8
_start:
  .lea r1, fnptr
  .lea r4, target
  store64 [r1+0], r4
  .argc r5
  imm r6, 1
  eq r5, r5, r6
  jumpz r5, .Lbad
  callm [r1+0]
  .exit r0
.Lbad:
  imm r1, 0
  callm [r1+0]
  imm r0, 1
  .exit r0
target:
  imm r0, 0
  ret
.end
