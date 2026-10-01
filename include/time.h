/* <time.h> for the unisa C subset: the types and the arithmetic, not the
 * clock.  `time` and `clock` need a system call the catalog does not carry
 * on every target (macOS has no clock_gettime syscall -- the abi audit
 * [A-51] found the number that used to stand in for it), so they are not
 * declared here: a program that calls them is refused at compile time
 * rather than handed a made-up time. */
#ifndef _UNISA_TIME_H
#define _UNISA_TIME_H
#include <stddef.h>
#define NULL 0
#define CLOCKS_PER_SEC 1000000
#ifndef _UNISA_TIME_T
#define _UNISA_TIME_T
typedef long time_t;
#endif
typedef long clock_t;
struct tm {
    int tm_sec; int tm_min; int tm_hour; int tm_mday; int tm_mon;
    int tm_year; int tm_wday; int tm_yday; int tm_isdst;
#ifndef _WIN32
    long tm_gmtoff; const char *tm_zone;   /* the host libc's layout (glibc and macOS): -run forwarding writes them (R19-10) */
#endif
};
/* time (0.0.18 R18-5): seconds from gettimeofday on Linux and macOS (the
   clock note above still holds for clock; Windows has no time yet) */
#if !defined(_WIN32) && (!__UNISA_FTRIM_LIBC || __UN_time)
#if !__UNISA_FTRIM_LIBC || __UN_time
static time_t time(time_t *__u_t) {
    long __u_tv[2]; long __u_r;
    __u_tv[0] = 0; __u_tv[1] = 0;
    __u_r = __gettimeofday((char *)__u_tv, 0, 0);
    if (__u_r < 0) return (time_t)(0 - 1);
    if (__u_t) *__u_t = __u_tv[0];
    return __u_tv[0];
}
#endif
#endif
/* ---- broken-down time (0.0.19, dsh): gmtime/localtime/mktime/strftime in C.
   localtime reads the zone from TZif data (RFC 8536, the 64-bit section):
   TZ unset -> /etc/localtime; TZ "" / "UTC" / "GMT" -> UTC; ":Name" or
   "Name" -> /usr/share/zoneinfo/Name; "/path" -> that file.  POSIX rule
   strings in TZ ("EST5EDT,...") are not parsed: such a TZ falls back to
   the zoneinfo file of that name, else UTC.  Transitions after the table's
   last one use the last type (the footer rule is not evaluated).
   The C locale only. */
#ifndef _WIN32
#if !__UNISA_FTRIM_LIBC || __UN__unisa_days_from_civil
static long _unisa_days_from_civil(long y, int m, int d) {
    long era; long yoe; long doy; long doe;
    y = y - (m <= 2); era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400; doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_civil
static void _unisa_civil(long z, long *py, int *pm, int *pd) {
    long era; long doe; long yoe; long doy; long mp; long y; int m;
    z = z + 719468; era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097; yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400; doy = doe - (365 * yoe + yoe / 4 - yoe / 100); mp = (5 * doy + 2) / 153;
    *pd = (int)(doy - (153 * mp + 2) / 5 + 1); m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *pm = m; *py = y + (m <= 2);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_leap
static int _unisa_leap(long y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_fill
static struct tm *_unisa_fill(long t, long off, int dst, const char *zone, struct tm *r) {
    long days; long sec; long y; int m; int d; static int cum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    t = t + off; days = t / 86400; sec = t % 86400; if (sec < 0) { sec = sec + 86400; days = days - 1; }
    _unisa_civil(days, &y, &m, &d);
    r->tm_sec = (int)(sec % 60); r->tm_min = (int)(sec / 60 % 60); r->tm_hour = (int)(sec / 3600);
    r->tm_mday = d; r->tm_mon = m - 1; r->tm_year = (int)(y - 1900);
    r->tm_wday = (int)((days % 7 + 11) % 7);          /* 1970-01-01 was a Thursday */
    r->tm_yday = cum[m - 1] + d - 1 + (m > 2 && _unisa_leap(y));
    r->tm_isdst = dst; r->tm_gmtoff = off; r->tm_zone = zone;
    return r;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_gmtime_r
static struct tm *gmtime_r(const time_t *__u_t, struct tm *__u_r) { return _unisa_fill((long)*__u_t, 0, 0, "UTC", __u_r); }
#endif
static struct tm _unisa_tmbuf;
#if !__UNISA_FTRIM_LIBC || __UN_gmtime
static struct tm *gmtime(const time_t *__u_t) { return gmtime_r(__u_t, &_unisa_tmbuf); }
#endif
/* the zone, loaded once */
static unsigned char _unisa_tz[65536]; static long _unisa_tzn; static int _unisa_tzstate;   /* 0 not loaded, 1 TZif, 2 UTC */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_be
static long _unisa_be(const unsigned char *p, int n) { long v; int k; v = 0; k = 0; while (k < n) { v = (v << 8) | p[k]; k = k + 1; } if (n == 4 && (v & 0x80000000L)) v = v - 4294967296L; return v; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_envget
static char *_unisa_envget(const char *nm) {
    int k; int i; char *e; k = __argc() + 1;
    while ((e = __argv(k)) != 0) { i = 0; while (nm[i] && e[i] == nm[i]) i = i + 1; if (nm[i] == 0 && e[i] == 61) return e + i + 1; k = k + 1; }
    return 0;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_tzload
static void _unisa_tzload(void) {
    char path[512]; char *tz; long fd; int q; char *pre;
    if (_unisa_tzstate) return;
    _unisa_tzstate = 2; tz = _unisa_envget("TZ");
    if (tz == 0) tz = "/etc/localtime";
    else {
        if (*tz == 58) tz = tz + 1;
        if (*tz == 0 || (tz[0] == 85 && tz[1] == 84 && tz[2] == 67 && tz[3] == 0) || (tz[0] == 71 && tz[1] == 77 && tz[2] == 84 && tz[3] == 0)) return;
    }
    q = 0;
    if (*tz != 47) { pre = "/usr/share/zoneinfo/"; while (*pre) { path[q] = *pre; q = q + 1; pre = pre + 1; } }
    while (*tz && q < 500) { if (tz[0] == 46 && tz[1] == 46) return; path[q] = *tz; q = q + 1; tz = tz + 1; }
    path[q] = 0;
    fd = __open(path, 0, 0); if (fd < 0) return;
    _unisa_tzn = __read(fd, (char *)_unisa_tz, 65536); __close(fd);
    if (_unisa_tzn >= 44 && _unisa_tz[0] == 84 && _unisa_tz[1] == 90 && _unisa_tz[2] == 105 && _unisa_tz[3] == 102) _unisa_tzstate = 1;
}
#endif
/* offset, dst and abbreviation in effect at UTC time t */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_tzat
static long _unisa_tzat(long t, int *dst, const char **zone) {
    unsigned char *h; long isut; long isstd; long leap; long tc; long ty; long ch; int tsz; long p; long i; long idx; unsigned char *tt;
    _unisa_tzload(); *dst = 0; *zone = "UTC";
    if (_unisa_tzstate != 1) return 0;
    h = _unisa_tz; tsz = 4;
    isut = _unisa_be(h + 20, 4); isstd = _unisa_be(h + 24, 4); leap = _unisa_be(h + 28, 4); tc = _unisa_be(h + 32, 4); ty = _unisa_be(h + 36, 4); ch = _unisa_be(h + 40, 4);
    if (h[4] >= 50) {                                   /* version 2+: use the 64-bit section */
        p = 44 + tc * 5 + ty * 6 + ch + leap * 8 + isstd + isut;
        if (p + 44 > _unisa_tzn) return 0;
        h = _unisa_tz + p; tsz = 8;
        isut = _unisa_be(h + 20, 4); isstd = _unisa_be(h + 24, 4); leap = _unisa_be(h + 28, 4); tc = _unisa_be(h + 32, 4); ty = _unisa_be(h + 36, 4); ch = _unisa_be(h + 40, 4);
    }
    if (ty <= 0 || (h - _unisa_tz) + 44 + tc * (tsz + 1) + ty * 6 + ch > _unisa_tzn) return 0;
    idx = 0 - 1; i = 0;
    while (i < tc) { if (_unisa_be(h + 44 + i * tsz, tsz) <= t) idx = h[44 + tc * tsz + i]; else break; i = i + 1; }
    if (idx < 0) { idx = 0; i = 0; while (i < ty) { if (h[44 + tc * (tsz + 1) + i * 6 + 4] == 0) { idx = i; break; } i = i + 1; } }
    if (idx >= ty) return 0;
    tt = h + 44 + tc * (tsz + 1) + idx * 6;
    *dst = tt[4];
    if (tt[5] < ch) *zone = (const char *)(h + 44 + tc * (tsz + 1) + ty * 6 + tt[5]);
    return _unisa_be(tt, 4);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_localtime_r
static struct tm *localtime_r(const time_t *__u_t, struct tm *__u_r) {
    int dst; const char *zone; long off;
    off = _unisa_tzat((long)*__u_t, &dst, &zone);
    return _unisa_fill((long)*__u_t, off, dst, zone, __u_r);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_localtime
static struct tm *localtime(const time_t *__u_t) { return localtime_r(__u_t, &_unisa_tmbuf); }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_mktime
static time_t mktime(struct tm *__u_tm) {
    long y; long m; long days; long t; long off; int dst; const char *zone;
    m = __u_tm->tm_mon; y = __u_tm->tm_year + 1900L + m / 12; m = m % 12; if (m < 0) { m = m + 12; y = y - 1; }
    days = _unisa_days_from_civil(y, (int)m + 1, 1) + __u_tm->tm_mday - 1;
    t = days * 86400 + __u_tm->tm_hour * 3600L + __u_tm->tm_min * 60L + __u_tm->tm_sec;
    off = _unisa_tzat(t, &dst, &zone); off = _unisa_tzat(t - off, &dst, &zone);
    t = t - off;
    localtime_r((time_t *)&t, __u_tm);
    return (time_t)t;
}
#endif
static const char *_unisa_wday[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *_unisa_mon[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
#if !__UNISA_FTRIM_LIBC || __UN__unisa_put
static size_t _unisa_put(char *s, size_t max, size_t n, const char *t, int k) { int i; i = 0; while (i < k && t[i]) { if (n + 1 < max) s[n] = t[i]; n = n + 1; i = i + 1; } return n; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_num
static size_t _unisa_num(char *s, size_t max, size_t n, long v, int w, int pad) {
    char d[24]; int k; int neg; neg = v < 0; if (neg) v = 0 - v; k = 0;
    do { d[k] = (char)(48 + v % 10); v = v / 10; k = k + 1; } while (v > 0);
    while (k < w) { d[k] = (char)pad; k = k + 1; }
    if (neg) { if (n + 1 < max) s[n] = 45; n = n + 1; }
    while (k > 0) { k = k - 1; if (n + 1 < max) s[n] = d[k]; n = n + 1; }
    return n;
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_strftime
static size_t strftime(char *__u_s, size_t __u_max, const char *__u_f, const struct tm *__u_tm) {
    size_t n; int c; long y;
    n = 0; y = __u_tm->tm_year + 1900L;
    while (*__u_f) {
        if (*__u_f != 37) { if (n + 1 < __u_max) __u_s[n] = *__u_f; n = n + 1; __u_f = __u_f + 1; continue; }
        __u_f = __u_f + 1; c = *__u_f; if (c == 0) break; __u_f = __u_f + 1;
        if (c == 69 || c == 79) { c = *__u_f; if (c == 0) break; __u_f = __u_f + 1; }   /* %E, %O: C locale */
        if (c == 89) n = _unisa_num(__u_s, __u_max, n, y, 1, 48);
        else if (c == 67) n = _unisa_num(__u_s, __u_max, n, y / 100, 2, 48);
        else if (c == 121) n = _unisa_num(__u_s, __u_max, n, (y % 100 + 100) % 100, 2, 48);
        else if (c == 109) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_mon + 1, 2, 48);
        else if (c == 100) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_mday, 2, 48);
        else if (c == 101) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_mday, 2, 32);
        else if (c == 72) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_hour, 2, 48);
        else if (c == 73) n = _unisa_num(__u_s, __u_max, n, (__u_tm->tm_hour + 11) % 12 + 1, 2, 48);
        else if (c == 77) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_min, 2, 48);
        else if (c == 83) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_sec, 2, 48);
        else if (c == 106) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_yday + 1, 3, 48);
        else if (c == 117) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_wday == 0 ? 7 : __u_tm->tm_wday, 1, 48);
        else if (c == 119) n = _unisa_num(__u_s, __u_max, n, __u_tm->tm_wday, 1, 48);
        else if (c == 97) n = _unisa_put(__u_s, __u_max, n, _unisa_wday[__u_tm->tm_wday % 7], 3);
        else if (c == 65) n = _unisa_put(__u_s, __u_max, n, _unisa_wday[__u_tm->tm_wday % 7], 99);
        else if (c == 98 || c == 104) n = _unisa_put(__u_s, __u_max, n, _unisa_mon[__u_tm->tm_mon % 12], 3);
        else if (c == 66) n = _unisa_put(__u_s, __u_max, n, _unisa_mon[__u_tm->tm_mon % 12], 99);
        else if (c == 112) n = _unisa_put(__u_s, __u_max, n, __u_tm->tm_hour < 12 ? "AM" : "PM", 2);
        else if (c == 110) n = _unisa_put(__u_s, __u_max, n, "\n", 1);
        else if (c == 116) n = _unisa_put(__u_s, __u_max, n, "\t", 1);
        else if (c == 37) n = _unisa_put(__u_s, __u_max, n, "%", 1);
        else if (c == 90) n = _unisa_put(__u_s, __u_max, n, __u_tm->tm_zone ? __u_tm->tm_zone : "", 99);
        else if (c == 122) { long o; o = __u_tm->tm_gmtoff; n = _unisa_put(__u_s, __u_max, n, o < 0 ? "-" : "+", 1); if (o < 0) o = 0 - o; n = _unisa_num(__u_s, __u_max, n, o / 3600 * 100 + o / 60 % 60, 4, 48); }
        else if (c == 115) { struct tm __u_c; __u_c = *__u_tm; n = _unisa_num(__u_s, __u_max, n, (long)mktime(&__u_c), 1, 48); }
        else if (c == 68) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%m/%d/%y", __u_tm);
        else if (c == 70) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%Y-%m-%d", __u_tm);
        else if (c == 84 || c == 88) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%H:%M:%S", __u_tm);
        else if (c == 82) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%H:%M", __u_tm);
        else if (c == 114) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%I:%M:%S %p", __u_tm);
        else if (c == 120) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%m/%d/%y", __u_tm);
        else if (c == 99) n = n + strftime(__u_s + (n < __u_max ? n : __u_max), n < __u_max ? __u_max - n : 0, "%a %b %e %H:%M:%S %Y", __u_tm);
        else { if (n + 1 < __u_max) __u_s[n] = 37; n = n + 1; if (n + 1 < __u_max) __u_s[n] = (char)c; n = n + 1; }
    }
    if (n >= __u_max) { if (__u_max) __u_s[0] = 0; return 0; }   /* did not fit: C99 7.23.3.5p8 */
    __u_s[n] = 0;
    return n;
}
#endif
static char _unisa_asc[32];
#if !__UNISA_FTRIM_LIBC || __UN_asctime
static char *asctime(const struct tm *__u_tm) { strftime(_unisa_asc, 32, "%a %b %e %H:%M:%S %Y\n", __u_tm); return _unisa_asc; }
#endif
#if !__UNISA_FTRIM_LIBC || __UN_ctime
static char *ctime(const time_t *__u_t) { return asctime(localtime(__u_t)); }
#endif
#endif
#if !__UNISA_FTRIM_LIBC || __UN_difftime
static double difftime(time_t __u_a, time_t __u_b) { return (double)(__u_a - __u_b); }
#endif
#endif
