/* string literals and the label counter */
int g(char *s) { return s[1]; }
int h() { char *p; p = "ab"; return g(p); }
int k() { return g("cd"); }
int main() { if (h()) return 1; return k(); }
