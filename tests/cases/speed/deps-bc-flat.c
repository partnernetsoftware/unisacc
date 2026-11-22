/* K5 speed probe 3a: deps-bc flat variant */
static int f0(int x){return x+1;} static int f1(int x){return x*3;}
static int f2(int x){return x^5;} static int f3(int x){return x-7;}
static int f4(int x){return x+11;} static int f5(int x){return x^13;}
int main(void) {
    int v = 42;
    v = f0(v) + f1(v) + f2(v) + f3(v) + f4(v) + f5(v);
    return (v == 0) ? 1 : 0;
}
