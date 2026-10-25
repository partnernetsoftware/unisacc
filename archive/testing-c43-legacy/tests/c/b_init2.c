/* aggregate initialisers: unsized, nested, struct, string, pointer */
struct P { int x; int y; };
int ga[] = {1, 2, 3, 4};
char gs[] = "hello";
char *gp = "ptr";
struct P gq = {7, 8};
int g2[2][2] = {{1, 2}, {3, 4}};
int gflat[2][2] = {1, 2, 3, 4};
int main() {
    int la[] = {5, 6, 7};
    char ls[] = "loc";
    struct P lp = {9, 10};
    int l2[2][2] = {{1, 2}, {3, 4}};
    printf("%d %d %s %s %d %d %d %d\n", ga[0], ga[3], gs, gp, gq.x, gq.y,
           g2[1][0], gflat[1][1]);
    printf("%d %d %s %d %d %d %d\n", la[0], la[2], ls, lp.x, lp.y, l2[1][1],
           (int)sizeof(ga));
    return 0;
}
