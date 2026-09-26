/* calls through file-scope function pointers: an array element and a pointer variable */
int add(int a, int b) { return a + b; }
void nop(void) { }
static int (*gp)(int, int);
static void (*fa[4])(void);
int main() { int k; k = 1; gp = add; fa[k] = nop; fa[k](); return gp(k, 4); }
