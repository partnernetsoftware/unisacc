#include <stdio.h>
/* Strings initialize complete character rows, with optional row braces. */
unsigned char global[][10] = {{"Key"}, "Wiki", {"Secret",}};
char exact[2][3] = {"abc", {"xyz"}};
int main(void) {
    unsigned char local[3][10] = {"Key", {"Wiki",}, "Secret"};
    static unsigned char saved[][10] = {{"Key"}, "Wiki", {"Secret"}};
    char designated[3][4] = {[1] = {"abc"}, [2] = "xy"};
    int i; int j;
    printf("%ld %ld %ld %ld\n", (long)sizeof global, (long)sizeof local,
           (long)sizeof saved, (long)sizeof global[0]);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 10; j++)
            printf("%d %d %d ", global[i][j], local[i][j], saved[i][j]);
        printf("\n");
    }
    printf("%d %d %d %d %d\n", exact[0][2], exact[1][2], designated[0][0],
           designated[1][3], designated[2][1]);
    return 0;
}
