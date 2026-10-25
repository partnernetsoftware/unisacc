/* Matching token ordinals in two units must not merge block statics. */
int f(void) { static int x = 1; return x++; }
int g(void);
int main(void) { int a = f(); int b = g(); return a * 10 + b; }
