/* string literals and the label counter, one function */
int main() { char *p; char *q; int u; u = 0; p = "ab"; if (u) u = 2; q = u ? "x" : "y"; p = "cd"; if (u) u = 3; return p[0]; }
