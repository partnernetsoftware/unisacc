static char ch;
int main() { long r; int k; ch = 72; r = __write(1, &ch, 1); ch = 10; k = __write(1, &ch, 1) + 2; r = r + __read(0, &ch, 0); return (int)r + k; }
