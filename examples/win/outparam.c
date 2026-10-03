/* examples/win/outparam.c -- are arguments five and six actually delivered?
 *
 * argceil.c showed that five- and six-argument forwards return plausible
 * values. It also showed something quieter: SearchPathA returned the right
 * length and left the caller'"'"'s buffer empty. A forward that computes the
 * right answer while dropping an argument is the most expensive kind of wrong,
 * because the program keeps working and the data never arrives.
 *
 * So this probe uses calls whose RESULT DEPENDS on the arguments past the
 * fourth register slot, never a call that merely returns a count:
 *
 *   - the .ini file is written with stdio, so it exists before any forward
 *     runs and a "file not found" answer cannot be mistaken for a lost
 *     argument;
 *   - GetPrivateProfileStringA puts the output buffer in slot four and the
 *     length in slot five, and returns the file'"'"'s value only if BOTH
 *     arrived;
 *   - SearchPathA is repeated with the buffer dumped as bytes, so "empty" and
 *     "not NUL-terminated" are different failures.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o outparam.exe examples/win/outparam.c
 *   ./outparam.exe
 */
#include <stdio.h>

#define FILE_NAME "unisacc-win-probe.ini"

unsigned long GetPrivateProfileStringA(const char *section, const char *key,
                                       const char *fallback, char *out,
                                       unsigned long size, const char *file);
unsigned long SearchPathA(const char *path, const char *file,
                          const char *ext, char *buffer, unsigned long size);
int WritePrivateProfileStringA(const char *section, const char *key,
                               const char *value, const char *file);
int GetDiskFreeSpaceExA(const char *path, unsigned long *free_to_me,
                        unsigned long *total, unsigned long *total_free);
unsigned long GetLastError(void);

static void hex(const char *tag, char *p, unsigned long n)
{
    unsigned long i;
    printf("%s", tag);
    for (i = 0; i < n; i = i + 1) {
        printf(" %02x", (unsigned int)p[i]);
    }
    printf("\n");
}

int main(void)
{
    char got[128];
    char found[520];
    unsigned long n;
    unsigned long i;
    unsigned long free_to_me;
    unsigned long total;
    unsigned long total_free;
    FILE *f;

    /* The file is created with stdio, so it is guaranteed to exist before
       the first forward reads it. */
    f = fopen(FILE_NAME, "wb");
    if (f == 0) {
        printf("cannot create %s\n", FILE_NAME);
        return 1;
    }
    fprintf(f, "[probe]\nanswer=forty-two\n");
    fclose(f);

    for (i = 0; i < 128; i = i + 1) {
        got[i] = 0;
    }
    n = GetPrivateProfileStringA("probe", "answer", "DEFAULT", got, 128, FILE_NAME);
    printf("read n %lu err %lu\n", n, GetLastError());
    printf("read [%s]\n", got);
    hex("read bytes", got, 12);

    for (i = 0; i < 520; i = i + 1) {
        found[i] = 0;
    }
    n = SearchPathA(0, "notepad.exe", 0, found, 520);
    printf("search n %lu\n", n);
    hex("search bytes", found, 16);
    printf("search [%s]\n", found);

    /* The control: FOUR arguments, three of them out-parameters. If these
       come back, the register slots are delivered and the fault above is
       exactly the stack slots -- argument four of SearchPathA and arguments
       five and six of GetPrivateProfileStringA. */
    free_to_me = 0;
    total = 0;
    total_free = 0;
    if (GetDiskFreeSpaceExA("C:\\", &free_to_me, &total, &total_free)) {
        printf("4 args free %lu total %lu\n", free_to_me, total);
        printf("4 args out-param %d\n", total > 0 && free_to_me > 0);
    } else {
        printf("4 args failed %lu\n", GetLastError());
    }
    return 0;
}