/* stringize, token paste, variadic macros, prefix ++/--, varargs prototypes */
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a, b) a ## b
#define N 7
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
int myvar(const char *fmt, ...);
int ab7 = 99;
int main() {
    int x;
    char *const p = "hi";
    x = 5;
    printf("%s %s %d %s\n", STR(hello), XSTR(N), CAT(ab, 7), p);
    LOG("%d %d\n", 1, 2);
    /* one side effect per statement: the order of evaluation of function
       arguments is unspecified, and gcc and clang disagree about it */
    ++x;
    printf("%d ", x);
    --x;
    printf("%d ", x);
    x++;
    printf("%d\n", x);
    return 0;
}
