/* an unsigned int constant (hex in (INT_MAX, UINT_MAX]) against int-width operands */
int main(void) { int x; char c; long y; x = 3; c = 1; y = 5;
  return (0x80000000 >> x) + (x / 0xfffffffe) + (c == 0xffffffff) + (0x80000000 % x) + (x * 0x80000001 != 0) + (y + 0x80000000 > 0); }
