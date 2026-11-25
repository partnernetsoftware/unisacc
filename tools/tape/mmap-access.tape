_start:
  imm r0, 0
  imm r1, 4096
  imm r2, 3
  .argc r6
  imm r3, 1
  eq r6, r6, r3
  jumpz r6, .Lreadonly
  jump .Lmap
.Lreadonly:
  imm r2, 1
.Lmap:
  imm r3, 34
  imm r4, -1
  imm r5, 0
  .sys6 mmap, r0, r1, r2, r3, r4, r5
  mov r6, r0
  imm r4, 65
  store64 [r6+0], r4
  load64 r5, [r6+0]
  eq r5, r4, r5
  jumpz r5, .Lbad
  imm r0, 1
  mov r1, r6
  imm r2, 1
  .sys write, r0, r1, r2
  mov r0, r6
  imm r1, 4096
  imm r2, 0
  .sys munmap, r0, r1, r2
  .exit r0
.Lbad:
  imm r0, 1
  .exit r0
.end
