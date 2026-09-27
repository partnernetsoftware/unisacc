/* Bare and braced string rows share ordinary rank-two aggregate row layout. */
char global[3][10] = {"Key", "Wiki", "Secret"};
unsigned char padded[4][4] = {{"a"}, {"bc",}, {"defg"},};
int main(void) {
    char local[3][10] = {{"Key"}, {"Wiki"}, {"Secret"},};
    unsigned char bytes[2][4] = {"\101" "b", "abcd"};
    static char saved[3][10] = {"Key", "Wiki", "Secret"};
    static unsigned char sb[3][4] = {{"a"}, {"bc",}, {"defg"},};
    char mixed[3][4] = {"ab", {67,68}, {"ef",}};
    int i, j;
    for (i=0;i<3;i++) for(j=0;j<10;j++)
        if(global[i][j]!=local[i][j] || global[i][j]!=saved[i][j]) return 1;
    return sizeof(global)!=30 || global[0][0]!=75 || global[1][0]!=87
        || global[2][0]!=83 || global[2][6]!=0 || local[0][9]!=0
        || padded[0][1]!=0 || padded[1][1]!=99 || padded[2][3]!=103
        || padded[3][0]!=0 || bytes[0][0]!=65 || bytes[0][2]!=0 || bytes[1][3]!=100
        || sb[0][1]!=0 || sb[1][1]!=99 || sb[2][3]!=103
        || mixed[0][1]!=98 || mixed[1][0]!=67 || mixed[1][2]!=0 || mixed[2][1]!=102;
}
