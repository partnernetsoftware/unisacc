/* Host-only harness for the actual runtime model-number adapter. */
#define UNISA_RUNTIME_LIBRARY
#include "../../exec/c/run.c"

static int digit(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
int main(int argc, char **argv) {
    unsigned char data[256]; int n = 0;
    if (argc != 3) return 2;
    if (!strcmp(argv[1], "hex")) { printf("%d\n",hexv((unsigned char)argv[2][0])); return 0; }
    if (!strcmp(argv[1], "tag") || !strcmp(argv[1], "btag")) {
        LBIN=argv[1][0]=='b'; LB=(unsigned char *)argv[2]; LN=(int)strlen(argv[2]); LP=0;
        int c=lchar(); printf("%d %d\n",c,LP); return 0;
    }
    if (!strcmp(argv[1], "budget")) {
        I value=42; int ok=runtime_step_budget(argv[2],&value);
        printf("%d %lld\n",ok,(long long)value); return 0;
    }
    LBIN = argv[1][0] == '1';
    if (LBIN) {
        const char *s = argv[2];
        while (*s) {
            int a = digit(*s++); if (!*s || a < 0 || n == 256) return 2;
            int b = digit(*s++); if (b < 0) return 2;
            data[n++] = (unsigned char)(16 * a + b);
        }
    } else {
        n = (int)strlen(argv[2]); if (n > 256) return 2;
        memcpy(data, argv[2], n);
    }
    LB = data; LN = n; LP = 0;
    I value = lnum();
    printf("%lld %d\n", (long long)value, LP);
    return 0;
}
