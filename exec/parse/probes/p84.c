struct T { long a; int b; };
struct T g1(long x) { struct T t; t.a = x; t.b = 3; return t; }
int main(void) { struct T u; struct T w; u = g1(5); w = u; return (int)w.a + w.b; }
