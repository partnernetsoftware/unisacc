int main(void) { int k; char *e; k = __argc() + 1; e = __argv(k); while ((e = __argv(k)) != 0) k = k + 1; return k; }
