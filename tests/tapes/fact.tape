; 5! printed, then newline
.str nl "\n"
main:
  imm    r0, 1          ; acc
  imm    r1, 5          ; n
loop:
  jumpz  r1, done
  mul64  r0, r0, r1
  imm    r2, 1
  sub64  r1, r1, r2
  jump   loop
done:
  .print r0
  .lea   r2, nl
  imm    r3, 1
  .write r2, r3
  imm    r0, 0
  .exit  r0
