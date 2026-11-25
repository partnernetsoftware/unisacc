_start:
  imm r0, 0
  imm r1, 4096
  imm r2, 3
  imm r3, 34
  imm r4, -1
  imm r5, 0
  .sys6 mmap, r0, r1, r2, r3, r4, r5
  imm r4, 0
  slt64 r4, r0, r4
  jumpz r4, .Lunmap
  imm r0, 1
  .exit r0
.Lunmap:
  imm r1, 4096
  imm r2, 0
  .sys munmap, r0, r1, r2
  .exit r0
.end
