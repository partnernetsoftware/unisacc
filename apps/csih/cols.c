/* module csih.cols: provider, bodies from csih_cols.h, static removed */
int csih_next_cp(const char *s, unsigned int *cp) {
    unsigned char c;
    int need = 1, i;
    if (!s || !s[0]) return 0;
    c = (unsigned char)s[0];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xe0) == 0xc0) need = 2;
    else if ((c & 0xf0) == 0xe0) need = 3;
    else if ((c & 0xf8) == 0xf0) need = 4;
    else return 0;
    *cp = c & (0xff >> (need + 1));
    for (i = 1; i < need; i++) {
        unsigned char d = (unsigned char)s[i];
        if ((d & 0xc0) != 0x80) return 0;
        *cp = (*cp << 6) | (d & 0x3f);
    }
    return need;
}
int csih_cp_cols(unsigned int cp) {
    if (cp < 0x1100) return 1;
    if (cp <= 0x115f) return 2;
    if (cp >= 0x2e80 && cp <= 0xa4cf) return 2;
    if (cp >= 0xac00 && cp <= 0xd7a3) return 2;
    if (cp >= 0xf900 && cp <= 0xfaff) return 2;
    if (cp >= 0xfe10 && cp <= 0xfe6f) return 2;
    if (cp >= 0xff00 && cp <= 0xff60) return 2;
    if (cp >= 0xffe0 && cp <= 0xffe6) return 2;
    if (cp >= 0x1f300 && cp <= 0x1f9ff) return 2;
    return 1;
}
