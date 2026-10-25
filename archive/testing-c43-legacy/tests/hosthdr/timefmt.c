/* 0.0.19 (dsh): gmtime/localtime/mktime/strftime in UTC and in fixed zones */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void) {
    time_t ts[4] = { 0, 951782400, 1709251199, 4102444800L }; char b[512]; struct tm tm; int i;
    for (i = 0; i < 4; i++) {
        gmtime_r(&ts[i], &tm);
        strftime(b, sizeof b, "%Y-%m-%d %H:%M:%S %a %A %b %B %j %u %w %y %C %e %p %I|%D|%F|%T|%R|%r|%c", &tm);
        printf("utc %s\n", b);
        localtime_r(&ts[i], &tm);
        strftime(b, sizeof b, "%F %T %Z %z dst=", &tm);
        printf("local %s%d\n", b, tm.tm_isdst > 0);
        printf("mktime back %d\n", mktime(&tm) == ts[i]);
    }
    printf("small buffer %d\n", (int)strftime(b, 4, "%Y-%m-%d", &tm));
    return 0;
}
