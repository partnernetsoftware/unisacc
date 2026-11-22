/* K5 speed probe 3b: deps-bc chained variant */
static int c0(int x){return x+1;}
static int c1(int x){return c0(x)*3;}
static int c2(int x){return c1(x)^5;}
static int c3(int x){return c2(x)-7;}
static int c4(int x){return c3(x)+11;}
static int c5(int x){return c4(x)^13;}
int main(void) {
    int v = c5(42);
    return (v == 0) ? 1 : 0;
}
