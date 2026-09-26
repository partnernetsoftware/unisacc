/* equal: char-array string initialisers -- sized (N > len, N == len: no NUL), unsized, escapes, global and local */
char g[] = "hi\n";
char h[8] = "ab";
char k[3] = "abc";
int main(void) { char l[] = "xyz"; char m[2] = "xy"; char *p = "pp"; return g[0] + h[1] + k[2] + l[2] + m[1] + p[0]; }
