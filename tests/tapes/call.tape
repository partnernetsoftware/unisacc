; fib(10) via recursion -> 55
.str nl "\n"
main:
  imm    r0, 10
  call   fib
  .print r0
  .lea   r1, nl
  imm    r2, 1
  .write r1, r2
  imm    r0, 0
  .exit  r0
fib:
  imm    r1, 2
  slt64  r2, r0, r1
  jumpz  r2, rec
  ret
rec:
  .frame 16
  store64 [r7+0], r0
  imm    r1, 1
  sub64  r0, r0, r1
  call   fib
  store64 [r7+8], r0
  load64 r0, [r7+0]
  imm    r1, 2
  sub64  r0, r0, r1
  call   fib
  load64 r1, [r7+8]
  add64  r0, r0, r1
  imm    r1, 16
  add64  r7, r7, r1
  ret
