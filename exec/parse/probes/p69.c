/* file-scope function pointers: stored as pointers; no call through them */
static void (*fa[4])(void);
static int (*g1)(int, char *(*)(int));
int main() { fa[2] = 0; g1 = 0; return fa[2] == 0; }
