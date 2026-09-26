/* a float return type: return (float) of a double call */
static double g(long a) { return (double)a; }
static float h(long a) { return (float)g(a); }
int main() { return 0; }
