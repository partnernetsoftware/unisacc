_start:
.unit 0 1:62
  imm r0, 7
.unit 0 0:
  .frame 8
.unit 0 1:63
  store64 [r7+0], r0
.unit 0 1:61
  load64 r1, [r7+0]
.unit 0 1:62
  .frame -8
.unit 0 0:
  imm r2, 24
.unit 0 1:63
  sub64 r3, r6, r2
.unit 0 1:61
  load64 r3, [r3+0]
.unit 0 1:62
  store64 [r6-8], r1
.unit 0 0:
  load64 r4, [r6-8]
.unit 0 1:63
  imm r2, 32
.unit 0 1:61
  sub64 r5, r6, r2
.unit 0 1:62
  mov r0, r1
.unit 0 0:
  store64 [r5+0], r4
.unit 0 1:63
  jump .unitx
.unit 0 1:61
.unitx:
.unit 0 1:62
  .exit r4
.end
