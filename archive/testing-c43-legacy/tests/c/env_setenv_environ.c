/* setenv/unsetenv edit environ itself (POSIX).  environ is what execv, execvp, the
   execl family, system and popen hand a child, so a variable set here is one the child
   sees.  The carried libc kept setenv in a side table only getenv read: environ never
   changed and the child of execvp saw nothing (found porting tests/boundnested.cx). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
extern char **environ;
static int count(const char *kv) {
    char **e; int n = 0;
    for (e = environ; *e; e++) if (strcmp(*e, kv) == 0) n++;
    return n;
}
static int names(const char *name) {
    char **e; int n = 0; size_t k = strlen(name);
    for (e = environ; *e; e++) if (strncmp(*e, name, k) == 0 && (*e)[k] == '=') n++;
    return n;
}
#else   /* no environ to walk on Windows: getenv alone */
static int count(const char *kv) {
    const char *eq = strchr(kv, '='); char nm[64]; const char *v;
    memcpy(nm, kv, eq - kv); nm[eq - kv] = 0; v = getenv(nm);
    return v && strcmp(v, eq + 1) == 0;
}
static int names(const char *name) { return getenv(name) != 0; }
#define environ ((char **)1)
#endif
int main(void) {
    int bad = 0;
    unsetenv("UNISA_ENV_A"); unsetenv("UNISA_ENV_B");
    if (!environ || names("UNISA_ENV_A") || names("UNISA_ENV_B")) bad |= 1;
    if (setenv("UNISA_ENV_A", "one", 1) != 0 || setenv("UNISA_ENV_B", "gone", 1) != 0) bad |= 2;
    if (setenv("UNISA_ENV_A", "two", 0) != 0) bad |= 2;              /* no overwrite: stays "one" */
    if (count("UNISA_ENV_A=one") != 1 || count("UNISA_ENV_B=gone") != 1) bad |= 4;
    if (unsetenv("UNISA_ENV_B") != 0 || names("UNISA_ENV_B") || getenv("UNISA_ENV_B")) bad |= 8;
    if (setenv("UNISA_ENV_A", "three", 1) != 0 || names("UNISA_ENV_A") != 1 || count("UNISA_ENV_A=three") != 1) bad |= 16;
    if (!getenv("UNISA_ENV_A") || strcmp(getenv("UNISA_ENV_A"), "three") != 0) bad |= 16;
    if (setenv("", "x", 1) != -1 || setenv("A=B", "x", 1) != -1 || unsetenv("A=B") != -1) bad |= 32;
    printf("environ bad=%d\n", bad);
    return bad;
}
