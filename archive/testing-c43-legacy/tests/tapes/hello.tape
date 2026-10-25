.str msg "hello from C99\n"
main:
  .lea   r0, msg
  imm    r1, 15
  .write r0, r1
  imm    r0, 0
  .exit  r0
