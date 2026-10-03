/* examples/win/clockfmt.c -- the clock a taskbar needs, from kernel32 alone.
 *
 * GetSystemTime / GetLocalTime write a SYSTEMTIME: nine 16-bit fields in a
 * fixed order. The struct is declared here rather than pulled from a header
 * because there is no Win32 SDK header in this compiler; the layout below is
 * the documented one and the probe prints the fields, so a layout mistake
 * would be visible rather than silent.
 *
 * SIZES ARE THE CALLER'S JOB. A forward carries no type and no size: the
 * stub passes the pointer through and the kernel writes the full structure.
 * TIME_ZONE_INFORMATION is 172 bytes (bias, two 32-wchar names, two
 * SYSTEMTIMEs, two biases), so tz_info below is padded to its real size. An
 * 8-byte declaration compiles, runs, returns TIME_ZONE_ID_SUCCESS, and
 * quietly lets the kernel write 164 bytes past the object -- which is a
 * corrupted frame, not a diagnostic. Declare the real thing.
 *
 * This is the arithmetic behind "2026/10/3  09:41" on an ORB or a taskbar:
 * no CRT, no locale, no timezone database.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o clockfmt.exe examples/win/clockfmt.c
 *   ./clockfmt.exe
 */
#include <stdio.h>

/* SYSTEMTIME, in the order the kernel writes it: 18 bytes. */
struct sys_time {
    unsigned short year;
    unsigned short month;
    unsigned short day_of_week;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short millis;
};

/* TIME_ZONE_INFORMATION: the bias is a 32-bit LONG, so it is declared int,
   not long -- long is 8 bytes on a win64 target and reading 8 bytes here
   picks up the first wchar of the name that follows it. The rest of the
   structure is padding to its real 172 bytes. */
struct tz_info {
    int bias;
    unsigned char rest[168];
};

void GetSystemTime(void *st);
void GetLocalTime(void *st);
unsigned long GetTimeZoneInformation(void *tz);
int SystemTimeToTzSpecificLocalTime(void *tz, void *in, void *out);

static void show(const char *tag, struct sys_time *st)
{
    printf("%s %04u/%02u/%02u %02u:%02u:%02u.%03u wd%u\n",
           tag,
           (unsigned int)st->year,
           (unsigned int)st->month,
           (unsigned int)st->day,
           (unsigned int)st->hour,
           (unsigned int)st->minute,
           (unsigned int)st->second,
           (unsigned int)st->millis,
           (unsigned int)st->day_of_week);
}

int main(void)
{
    struct sys_time local;
    struct sys_time converted;
    struct tz_info tz;
    unsigned long status;

    GetLocalTime(&local);
    show("local", &local);

    /* One pointer argument, a structure the compiler has never seen, and a
       172-byte object it must not truncate. */
    status = GetTimeZoneInformation(&tz);
    printf("tz status %lu bias minutes %d\n", status, (int)tz.bias);

    /* Three pointer arguments, two of them structures, and the result
       differs from the input by exactly the bias. */
    if (SystemTimeToTzSpecificLocalTime(&tz, &local, &converted)) {
        show("tz", &converted);
    } else {
        printf("tz conversion refused\n");
    }
    return 0;
}
