/* the runtime formatter, written in the subset itself */
#include <stdio.h>
int main() {
    char b[64];
    sprintf(b, "[%d][%5s][%-4d][%04x][%c][%%]", -12, "ab", 7, 255, 'Z');
    puts(b);
    fprintf(stdout, "fp %s %d\n", "ok", 99);
    snprintf(b, 6, "%s", "abcdefgh");
    puts(b);
    sprintf(b, "[%lx][%x][%lu][%ld][%o][%X]", 1099511627775L, 255,
            4294967296L, -5L, 64, 48879);
    puts(b);
    return 0;
}
