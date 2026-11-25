_start:
  .lea r4, target
  .argc r5
  imm r6, 1
  eq r5, r5, r6
  jumpz r5, .Lbad
  callr r4
  .exit r0
.Lbad:
  imm r4, 0
  callr r4
  imm r0, 1
  .exit r0
target:
  imm r0, 0
  ret
.end
