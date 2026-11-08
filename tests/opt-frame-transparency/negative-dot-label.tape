_start:
  imm r0, 7
  .frame 8
  store64 [r7+0], r0
.unitx:
.endx:
  load64 r1, [r7+0]
  .frame -8
  .exit r1
.end
