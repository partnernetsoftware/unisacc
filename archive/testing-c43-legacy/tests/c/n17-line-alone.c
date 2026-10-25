/* __LINE__ as the only preprocessing on a line, in a file with no macro use:
   the network used to discard that line's rescan (no macro had "changed" it),
   so the product saw the bare identifier (0.0.14 release queue finding). */
int printf(const char *, ...);
int a = __LINE__;
int main(void) { printf("%d %d\n", a, __LINE__); return 0; }
