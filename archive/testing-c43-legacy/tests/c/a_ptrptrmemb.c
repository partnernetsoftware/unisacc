/* TS **hash as a struct member: tb->hash[i] steps by 8, not by sizeof(TS) (lua lstring.c stringtable) -- compared with cc */
#include <stdio.h>
typedef struct TS { long a, b, c, d; } TS;
typedef struct st { TS **hash; int nuse; int size; } st;
int main(void) { TS *v[8]; st t; st *tb = &t; TS x; int i; t.hash = v; for (i = 0; i < 8; i++) v[i] = 0; tb->hash[5] = &x;
  printf("%d %d\n", (int)((char *)&tb->hash[5] - (char *)v), v[5] == &x); return 0; }
