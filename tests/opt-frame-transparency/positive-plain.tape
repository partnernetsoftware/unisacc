_start:
  imm r0, 7
  .frame 8
  store64 [r7+0], r0
  load64 r1, [r7+0]
  .frame -8
  imm r2, 24
  sub64 r3, r6, r2
  load64 r3, [r3+0]
  store64 [r6-8], r1
  load64 r4, [r6-8]
  imm r2, 32
  sub64 r5, r6, r2
  mov r0, r1
  store64 [r5+0], r4
  jump .unitx
.unitx:
  .exit r4
.end
