/* local arrays: a scalar after an array, arrays in a nested block, unsigned long / short arrays, a pointer from an array */
int main() { char c[5]; int y; unsigned long u[3]; short s[2]; char *p; y = 1; { char d[7]; int z; d[0] = 1; z = d[0]; } u[2] = y; s[1] = u[2]; p = c; p = c + 2; return s[1] + *p; }
