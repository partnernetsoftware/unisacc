int g;
int h = 5;
static int sq(int a) { return a * a; }
int main() { g = 3; g += h; h++; return sq(g) + h; }
