/* a call's width is the callee's return type: char/short/int/long returns in arithmetic, and an int local against 0x80000000 */
short s(void) { return -1; }
char c(void) { return 100; }
int i(void) { return 7; }
long l(void) { return 9; }
int main(void) { int x; x = 3; return s() + c() * 2 + (i() >> 1) + l() - 1 + (s() < 0) + (x - 0x80000000 > 0); }
