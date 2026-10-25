#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static int cmp30(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
int f30(const char *s) { int v[3] = {3,1,2}; char b[64]; double d = atof(s); qsort(v, 3, sizeof v[0], cmp30); snprintf(b, sizeof b, "%s%g", strerror(0), d); if (strchr(b, 'x') || strstr(b, "y") || isalpha(s[0])) perror("z"); return v[0] + (int)strtoul(s, 0, 10) + (int)strlen(getenv("HOME") ? "a" : "") + (int)memcmp(b, s, 0) + 30; }
