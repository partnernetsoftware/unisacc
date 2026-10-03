/* examples/win/refused/wideargs.c -- MEASURED FAILURE, and the fix in one file.
 *
 * These two calls are the measurement that produced the 0.0.23 fix, kept as a
 * regression test. Both are at most six arguments wide in the source, and both
 * used to be emitted anyway:
 *
 *   SearchPathA(0, "notepad.exe", 0, buf, 520)              five arguments
 *   GetPrivateProfileStringA(sec, key, def, out, 128, file)  six
 *
 * Before the fix the generated hostcall delivered only the four register
 * slots, so SearchPathA returned the correct length (32) and left `buf` full
 * of zeroes, and GetPrivateProfileStringA returned its fallback even when the
 * file existed: no crash, no diagnostic, plausible numbers, no data. That is
 * why src/fwdstub.c now carries fwd_maxargs -- a target declares how many
 * arguments it can actually deliver (four on Windows) and a wider forward is
 * refused BY NAME at compile time instead of being emitted.
 *
 * Expected, and this is the point of the probe:
 *
 *   unisacc: error: host function 'SearchPathA' takes 5 arguments, and this
 *   target's host call delivers 4. Put the wide call in a bundled libc body,
 *   where it goes through the import table instead.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o wideargs.exe examples/win/refused/wideargs.c
 */
#include <stdio.h>

#define FILE_NAME "unisacc-win-probe.ini"

unsigned long SearchPathA(const char *path, const char *file,
                          const char *ext, char *buffer, unsigned long size);
unsigned long GetPrivateProfileStringA(const char *section, const char *key,
                                       const char *fallback, char *out,
                                       unsigned long size, const char *file);

int main(void)
{
    char found[520];
    char got[128];
    unsigned long n;

    found[0] = 0;
    n = SearchPathA(0, "notepad.exe", 0, found, 520);
    printf("search n %lu [%s]\n", n, found);

    got[0] = 0;
    n = GetPrivateProfileStringA("probe", "answer", "DEFAULT", got, 128, FILE_NAME);
    printf("profile n %lu [%s]\n", n, got);
    return 0;
}
