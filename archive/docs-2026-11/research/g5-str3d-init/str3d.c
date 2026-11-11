/* a string initialising the innermost char subarray of a 3-D array (C99 6.7.8p14,p20) */
char g[2][2][4] = {"a", "b", "c"};
int main(void) {
    char l[2][2][3] = {{"a", "b"}, "c"};
    return !(g[1][0][0] == 99 && g[0][1][0] == 98 && l[1][0][0] == 99 && l[0][1][0] == 98);
}
