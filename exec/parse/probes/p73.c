/* unsigned long as a return type; a cast to unsigned long */
static long g(long a) { return a - 1; }
static unsigned long h(long a) { return (unsigned long)g(a); }
int main() { return (int)(h(5) / 2); }
