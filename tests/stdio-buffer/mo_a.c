#include <stdio.h>
void b_done(void);
int main(void) { int i; for (i = 0; i < 2000; i++) printf("line %d from A\n", i); b_done(); return 0; }
