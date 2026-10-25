#include <stdio.h>
static int same(void) { return 4; }
int f(void) { goto same; same: return 1; }
int g(void) { goto same; same: return 2; }
int main(void) { printf("%d %d %d\n", f(), g(), same()); return f()+g()-3; }
