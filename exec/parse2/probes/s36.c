/* equal: 2-D row braces, short rows padded by .zero, empty row, flat list; global and local */
int g2[3][2] = {{1}, {3, 4}, {}};
int gf[2][2] = {1, 2, 3, 4};
int main() { int l[3][2] = {{1}, {3, 4}, {}}; int m[2][2] = {1, 2, 3}; return g2[1][1] + gf[1][0] + l[1][1] + m[1][0]; }
