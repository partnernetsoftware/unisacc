/* a local function pointer; a function pointer parameter passed on; (*f)() of a global */
int add(int a, int b) { return a + b; }
static int (*gp)(int, int);
int ap(int (*f)(int, int), int x) { return f(x, 1); }
int aq(int (*f)(int, int)) { return ap(f, 2); }
int main() { int (*lp)(int, int); lp = add; gp = lp; return lp(1, 2) + (*gp)(3, 4) + aq(add); }
