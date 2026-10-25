#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define DUP(s) strdup(s)
static int ulen(const char *s) { return (int)strlen(s); }
int main(void){ char *d = DUP("abc"); printf("%d %s\n", ulen("hello"), d); free(d); return 0; }
