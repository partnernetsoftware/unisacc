/* setenv takes a value of any length and any number of names; the carried libc
   kept 16 names of at most 255 bytes ("NAME=VALUE" included) and answered ENOMEM past that. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    static char big[5001]; char name[32]; int k, bad = 0; const char *v;
    memset(big, 'v', 5000); big[5000] = 0;
    if (setenv("UNISA_LONG", big, 1) != 0) bad |= 1;
    v = getenv("UNISA_LONG");
    if (!v || strlen(v) != 5000 || strcmp(v, big) != 0) bad |= 2;
    big[300] = 0;
    if (setenv("UNISA_LONG", big, 1) != 0 || !(v = getenv("UNISA_LONG")) || strlen(v) != 300) bad |= 4;
    for (k = 0; k < 40; k++) {
        sprintf(name, "UNISA_MANY_%d", k);
        if (setenv(name, name + 6, 1) != 0) bad |= 8;
    }
    for (k = 0; k < 40; k++) {
        sprintf(name, "UNISA_MANY_%d", k);
        if (!(v = getenv(name)) || strcmp(v, name + 6) != 0) bad |= 16;
    }
    printf("long bad=%d len=%d\n", bad, (int)strlen(getenv("UNISA_LONG") ? getenv("UNISA_LONG") : ""));
    return bad;
}
