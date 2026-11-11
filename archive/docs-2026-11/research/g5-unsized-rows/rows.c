#include <stdio.h>
static const char s[][4] = {"ab", "cd", "e"};
int f1[][2] = {1, 2, 3, 4, 5};
int f2[][3] = {{1}, 2, 3, 4, {5, 6}};
int f3[][2] = {[3] = {1, 2}, {4}};
int f4[][2][2] = {{{1, 2}, {3}}, {{4}}, 5, 6};
long f5[][3] = {{(1 + 2), (3, 4)}, {sizeof(int[2])}};
int main(void) {
    int l1[][2] = {{1}, {2}, {3, 4}};
    static short l2[][4] = {1, 2, 3, 4, 5};
    printf("%d %d %d %d %d %d %d %d\n", (int)sizeof s, (int)sizeof f1, (int)sizeof f2, (int)sizeof f3,
           (int)sizeof f4, (int)sizeof f5, (int)sizeof l1, (int)sizeof l2);
    printf("%s %d %d %d %d %ld %d %d\n", s[2], f1[2][0], f2[2][0], f3[4][0], f4[2][0][1], f5[1][0], l1[2][1], l2[1][0]);
    return 0;
}
