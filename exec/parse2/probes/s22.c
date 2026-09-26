unsigned int g;
unsigned f(unsigned a) { return a + 1; }
int main(void){unsigned int u; unsigned v; int i; u = 7; v = u; u++; ++u; u += 2; i = -1; u = i; u = (unsigned)i; i = u; g = f(u); return u < v || u == 3;}
