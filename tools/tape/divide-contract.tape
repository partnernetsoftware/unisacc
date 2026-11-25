_start:
  imm r1, -13
  imm r2, 5
  .div r3, r1, r2
  .mod r4, r1, r2
  imm r5, -2
  ne r3, r3, r5
  imm r5, -3
  ne r4, r4, r5
  or64 r6, r3, r4
  imm r1, -1
  imm r2, 16
  .udiv r3, r1, r2
  .umod r4, r1, r2
  imm r5, 1152921504606846975
  ne r3, r3, r5
  imm r5, 15
  ne r4, r4, r5
  or64 r6, r6, r3
  or64 r0, r6, r4
  .exit r0
.end
