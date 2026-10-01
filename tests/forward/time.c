#include <stdio.h>
#include <time.h>
struct tm *gmtime_r(const time_t *t, struct tm *r);
unsigned long strftime(char *s, unsigned long max, const char *fmt, const struct tm *tm);
int main(void){ time_t t = 86400 * 365; struct tm tm; char b[64]; gmtime_r(&t, &tm); strftime(b, sizeof b, "%Y-%m-%d %H:%M", &tm); printf("%s year %d\n", b, tm.tm_year); return 0; }
