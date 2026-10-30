_start:
  .sys write, r0, r1, r2
  .sys6 syscall, r0, r1, r2, r3, r4, r5
  .sys poll, r0, r1, r2
  .sys nosuchop, r0
  .sys write, r0, r1, r2
__ccw_0:
  ret
