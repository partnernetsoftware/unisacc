static char gc;
static short gs;
long gl;
int gi = 5, gj;
int main() { gc = 300; gs = 70000; gl = 100000; gl = gl * gl; gj = gi + 1; printf("%d %d %d %d\n", gc, gs, gj, (int)(gl / 1000000)); return gc; }
