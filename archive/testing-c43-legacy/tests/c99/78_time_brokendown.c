/* C99 7.23.2.3 mktime, 7.23.3.1 asctime, 7.23.3.2 ctime, 7.23.3.3 gmtime,
 * 7.23.3.4 localtime, 7.23.3.5 strftime, in forms whose result does not depend
 * on the host's time zone: gmtime of fixed instants, strftime and asctime of a
 * fixed broken-down time, mktime compared with itself (equal inputs, one day
 * apart in January, field normalisation, round trip through localtime), and
 * ctime as the composition asctime(localtime()).  The exit code is 0 when every
 * check passes; each set bit names a failed check.  Nothing is printed. */
#include <time.h>
#include <string.h>
static int same(const struct tm *a, const struct tm *b)
{
    return a->tm_sec == b->tm_sec && a->tm_min == b->tm_min && a->tm_hour == b->tm_hour
        && a->tm_mday == b->tm_mday && a->tm_mon == b->tm_mon && a->tm_year == b->tm_year
        && a->tm_wday == b->tm_wday && a->tm_yday == b->tm_yday;
}
int main(void)
{
    time_t t0 = 0, t1 = 1700000000, r1, r2, r3;
    struct tm g, x, y, z, *p;
    char buf[64];
    int sum = 0;
    p = gmtime(&t0);
    sum += (p != 0 && p->tm_year == 70 && p->tm_mon == 0 && p->tm_mday == 1 && p->tm_hour == 0
            && p->tm_min == 0 && p->tm_sec == 0 && p->tm_wday == 4 && p->tm_yday == 0) * 1;
    p = gmtime(&t1);
    sum += (p != 0 && p->tm_year == 123 && p->tm_mon == 10 && p->tm_mday == 14 && p->tm_hour == 22
            && p->tm_min == 13 && p->tm_sec == 20 && p->tm_wday == 2 && p->tm_yday == 317) * 2;
    g = *p;
    memset(buf, 0, sizeof buf);
    sum += (strftime(buf, sizeof buf, "%Y", &g) == 4 && strcmp(buf, "2023") == 0
            && strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S %a %b %j", &g) == 31
            && strcmp(buf, "2023-11-14 22:13:20 Tue Nov 318") == 0
            && strftime(buf, 4, "%Y", &g) == 0) * 4;
    sum += (strcmp(asctime(&g), "Tue Nov 14 22:13:20 2023\n") == 0) * 8;
    memset(&x, 0, sizeof x); x.tm_year = 124; x.tm_mon = 0; x.tm_mday = 10; x.tm_hour = 12; x.tm_isdst = -1;
    y = x; y.tm_mday = 11;
    z = x; z.tm_mday = 41;          /* normalises to February 10 */
    r1 = mktime(&x); r2 = mktime(&y); r3 = mktime(&z);
    sum += (r1 != (time_t)-1 && r2 - r1 == 86400 && x.tm_wday == 3 && x.tm_yday == 9
            && y.tm_wday == 4 && y.tm_mday == 11) * 16;
    sum += (z.tm_mon == 1 && z.tm_mday == 10 && r3 - r1 == 31 * 86400L && z.tm_wday == 6) * 32;
    p = localtime(&r1);
    sum += (p != 0 && same(p, &x) && mktime(p) == r1) * 64;
    p = localtime(&t1);
    sum += (p != 0 && strcmp(ctime(&t1), asctime(p)) == 0 && strlen(ctime(&t1)) == 25
            && mktime(localtime(&t1)) == t1) * 128;
    return 255 - sum;
}
