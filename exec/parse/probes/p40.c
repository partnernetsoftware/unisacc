static long len(const char *s) { long n; n = 0; while (s[n]) n = n + 1; return n; }
int main(int argc, char **argv) { const char *p; p = (const char *)argv[0]; if (len(p) > 0) return 7; return 3; }
