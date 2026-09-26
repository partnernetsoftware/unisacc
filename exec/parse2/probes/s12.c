static void (*at[32])(void);
static int nat = 0;
static void ex(int c) { while (nat > 0) { nat = nat - 1; at[nat](); } __exit(c); }
int main(void) { return 0; }
