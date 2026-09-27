/* Fixed array shapes share declaration, sizeof, indexing and pointer steps. */
typedef unsigned char Row[2][3];
typedef Row Alias;
Alias global = {{0}, {0, 0, 1}};
struct Box { Row value; int flexible[]; };
int rows(int a[][3]) { return sizeof a[0] == 12 && a[1][2] == 42; }
int step(Row *p) {
    Row *q = p + 1;
    Row *r = 1 + p;
    long distance = q - p;
    int first = (*(p++))[1][2];
    int again = (*(--p))[1][2];
    return distance == 1 && q == r && first == 1 && again == 1 &&
           (*(++p))[1][2] == 42 && (*(q - 1))[1][2] == 1;
}
int compound(Row *p) {
    Row *q;
    int value = (*(p += 1))[1][2];
    p -= 1;
    q = p += 1;
    return value == 42 && (*q)[1][2] == 42 && (*(p -= 1))[1][2] == 1;
}
int main(void) {
    unsigned char data[2][2][3] = {{{0}, {0, 0, 1}}, {{0}, {0, 0, 42}}};
    char *strings[][4] = {{"a", "b", "c", "d"}, {"e", "f", "g", "h"}};
    int a[2][3] = {{0}, {0, 0, 42}};
    int n = 3; int vla[n];
    Alias local;
    int i = 0; int j = 1;
    local[i++][j++] = 7;
    return !(sizeof(Row) == 6 && sizeof global == 6 && sizeof local == 6 &&
             sizeof strings == 64 && sizeof vla == 12 && strings[1][2][0] == 'g' &&
             i == 1 && j == 2 && local[0][1] == 7 && rows(a) && step((Row*)data) && compound((Row*)data));
}
