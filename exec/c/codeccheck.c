/* Batch codec runner. Red zones also check the unisacc build without ASan. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codec.h"
static int number(int end) {
 int c = getchar(), n = 0;
 if (c == -1) return -1;
 if (c < 48 || c > 57) return -2;
 while (c >= 48 && c <= 57) {
  if (n > 400000) return -2;
  n = n * 10 + c - 48; c = getchar();
 }
 return c == end ? n : -2;
}
int main(void) {
 int n, m, count = 0;
 crc_init();
 while (1) {
  n = number(32); if (n == -1) break; m = number(10);
  if (n < 0 || m < 0 || n > 4000000 || m > 4000000) return 2;
  unsigned char *in = malloc(n + 32), *out = malloc(m + 32);
  if (!in || !out) return 2;
  memset(in, 165, n + 32); memset(out, 165, m + 32);
  int at = 0;
  while (at < n) {
   int got = fread(in + 16 + at, 1, n - at, stdin);
   if (got <= 0) return 2; at += got;
  }
  int rc = unisa_inflate(out + 16, m, in + 16, n);
  for (int i = 0; i < 16; i++)
   if (out[i] != 165 || out[m+16+i] != 165 || in[i] != 165 || in[n+16+i] != 165) return 3;
  printf("%d %ld\n", rc, rc ? 0 : crc((char *)out + 16, m));
  free(in); free(out); count++;
 }
 return count ? 0 : 2;
}
