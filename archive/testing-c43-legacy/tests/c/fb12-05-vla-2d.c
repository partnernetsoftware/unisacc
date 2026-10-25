/* R13-0 #05 P0 -- a two-dimensional VLA.
   Expected: compiles and exits 8.
   Got on 0.0.12 (df8cc9b4): `fb12-05-vla-2d.c:7:13: error: expected ';'`
   at the second subscript.  One-dimensional `int v[n];` works.  If 2-D VLA is
   deliberately unsupported it must say so -- "2-D VLA is not supported" with a
   position -- rather than read as the user's syntax error. */
int main(void) {
    int n = 3;
    int m[n][4];
    m[2][2] = 8;
    return m[2][2];
}
