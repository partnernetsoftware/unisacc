/* Bounded, policy-free scanning of a tape integer token.  Callers decide
   whether an unsigned value or a leading-zero decimal is permitted. */
#ifndef UNISACC_TAPESCAN_H
#define UNISACC_TAPESCAN_H
static int ts_integer(char *s, int n, unsigned long *magnitude, int *negative,
                      int *leading_zero) {
    unsigned long value; int i; int base; int digit; int c;
    if (n <= 0) return 0;
    i = 0; base = 10; value = 0; *negative = 0; *leading_zero = 0;
    if (s[i] == '+' || s[i] == '-') { *negative = s[i] == '-'; i = i + 1; }
    if (i + 1 < n && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        base = 16; i = i + 2;
    } else if (n - i > 1 && s[i] == '0') *leading_zero = 1;
    if (i == n) return 0;
    while (i < n) {
        c = s[i] & 255; digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        if (digit < 0 || digit >= base || value > (~0UL - digit) / base) return 0;
        value = value * base + digit; i = i + 1;
    }
    *magnitude = value;
    return 1;
}
/* Validate a quoted .str payload without decoding or allocating it. */
static int ts_quoted_length(char *s, int n, int max_bytes, int *decoded) {
    int i; int size; int c; int h;
    if (n < 2 || s[0] != '"') return 0;
    i = 1; size = 0;
    while (i < n && s[i] != '"') {
        if (size >= max_bytes) return 0;
        if (s[i] == '\\') {
            i = i + 1; if (i >= n) return 0;
            if (s[i] == 'x') {
                if (i + 2 >= n) return 0;
                c = s[i + 1] & 255; h = s[i + 2] & 255;
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
                    || !((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F'))) return 0;
                i = i + 2;
            }
        }
        size = size + 1; i = i + 1;
    }
    if (i != n - 1 || s[i] != '"') return 0;
    *decoded = size;
    return 1;
}
#endif
