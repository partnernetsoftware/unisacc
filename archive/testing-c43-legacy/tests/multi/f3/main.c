#include <stdio.h>
#include "u.h"
int main(void){ char *d = DUP("abc"); printf("%d %s\n", ulen("hello"), d); free(d); return 0; }
