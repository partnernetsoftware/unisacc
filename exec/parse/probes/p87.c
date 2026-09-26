static int vf(int n, ...) { return n; }
int main(void) { long a; a = 5; return vf(3, 7, a, 9); }
