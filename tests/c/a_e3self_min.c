/* E3 self-check minimum: `extern T x;` then a function then the definition.
   Reference reserves .bss at the extern site; parse2 tables emit it at the definition. */
extern char a[8];
int f(void) { return a[0]; }
char b[4];
char a[8];
int main(void) { return f() + b[0]; }
