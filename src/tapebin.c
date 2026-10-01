/* tapebin v1 -> canonical tape text.  Container mechanics only: this code
   cannot choose an opcode, lower an instruction, or write an image.  The
   authoritative opcode/shape table is exported from unisa/tape.py. */
#include "tapebin_shape.inc"

#define TB_MAXPOOL 262144
char tb_file[MAXOUT];
int tb_name_at[TB_MAXPOOL], tb_name_len[TB_MAXPOOL], tb_nn;
int tb_const_at[TB_MAXPOOL], tb_const_len[TB_MAXPOOL], tb_nc;
int tb_bad;

long tb_le(char *p, int n) {
    long v; int i;
    v = 0; i = n - 1;
    while (i >= 0) { v = (v << 8) | (p[i] & 255); i = i - 1; }
    return v;
}
int tb_same(char *a, char *b, int n) {
    int i; i = 0;
    while (i < n) { if (a[i] != b[i]) return 0; i = i + 1; }
    return 1;
}
int tb_fail(void) { tb_bad = 1; return 0; }
int tb_put(int ch) {
    if (nout >= MAXOUT - 1) return tb_fail();
    out[nout] = ch; nout = nout + 1; return 1;
}
int tb_bytes(char *s, int n) {
    int i; i = 0;
    if (n < 0 || n > MAXOUT - 1 - nout) return tb_fail();
    while (i < n) { out[nout] = s[i]; nout = nout + 1; i = i + 1; }
    return 1;
}
int tb_word(char *s) {
    int n; n = 0; while (s[n]) n = n + 1;
    return tb_bytes(s, n);
}
int tb_udecimal(unsigned long mag) {
    char buf[24]; int n; int i;
    n = 0;
    do { buf[n] = 48 + mag % 10; mag = mag / 10; n = n + 1; } while (mag);
    i = n - 1; while (i >= 0) { tb_put(buf[i]); i = i - 1; }
    return !tb_bad;
}
int tb_decimal(long value) {
    if (value < 0) { tb_put(45); return tb_udecimal((unsigned long)(0 - (value + 1)) + 1); }
    return tb_udecimal((unsigned long)value);
}
int tb_hex(int b) { tb_put("0123456789abcdef"[(b >> 4) & 15]); return tb_put("0123456789abcdef"[b & 15]); }
int tb_quote(char *p, int n) {
    int i; int c; tb_put(34); i = 0;
    while (i < n) {
        c = p[i] & 255;
        if (c == 10) tb_word("\\n");
        else if (c == 9) tb_word("\\t");
        else if (c == 13) tb_word("\\r");
        else if (c == 0) tb_word("\\0");
        else if (c == 92) tb_word("\\\\");
        else if (c == 34) tb_word("\\\"");
        else if (c >= 32 && c < 127) tb_put(c);
        else { tb_word("\\x"); tb_hex(c); }
        i = i + 1;
    }
    tb_put(34); return !tb_bad;
}
/* Bounds checks precede every read.  Bad LEB encodings fail before any tape
   text is passed to the back end. */
long tb_uint(char *buf, int *pos, int end) {
    unsigned long value; int shift; int b; int k;
    value = 0; shift = 0; k = 0;
    while (k < 10) {
        if (*pos >= end) { tb_fail(); return 0; }
        b = buf[*pos] & 255; *pos = *pos + 1;
        if (shift == 63 && (b & 127) > 1) { tb_fail(); return 0; }
        value = value | ((unsigned long)(b & 127) << shift);
        k = k + 1;
        if ((b & 128) == 0) {
            if (k > 1 && (b & 127) == 0) tb_fail();
            return (long)value;
        }
        shift = shift + 7;
    }
    tb_fail(); return 0;
}
long tb_sint(char *buf, int *pos, int end) {
    unsigned long value; int shift; int b; int k;
    value = 0; shift = 0; k = 0;
    while (k < 10) {
        if (*pos >= end) { tb_fail(); return 0; }
        b = buf[*pos] & 255; *pos = *pos + 1;
        if (shift == 63 && (b != 0 && b != 127)) { tb_fail(); return 0; }
        value = value | ((unsigned long)(b & 127) << shift);
        k = k + 1;
        if ((b & 128) == 0) {
            if (shift < 57 && (b & 64)) value = value | (~0UL << (shift + 7));
            if (k > 1 && ((b == 0 && (buf[*pos - 2] & 64) == 0)
                       || (b == 127 && (buf[*pos - 2] & 64) != 0))) tb_fail();
            return (long)value;
        }
        shift = shift + 7;
    }
    tb_fail(); return 0;
}
int tb_pool(char *buf, int from, int to, int expect, int *at, int *lens, int *count, int names) {
    int p; long n; long size; int i; int j;
    p = from; n = tb_uint(buf, &p, to);
    if (tb_bad || n < 0 || n != expect || n > TB_MAXPOOL) return tb_fail();
    *count = n; i = 0;
    while (i < n) {
        size = tb_uint(buf, &p, to);
        if (tb_bad || size < 0 || size > to - p) return tb_fail();
        if (names) {
            if (size == 0) return tb_fail();
            j = 0; while (j < size) {
                int c; c = buf[p + j] & 255;
                if (c == 0 || c == 32 || c == 9 || c == 10 || c == 13 || c == 58) return tb_fail();
                j = j + 1;
            }
        }
        at[i] = p; lens[i] = size; p = p + size; i = i + 1;
    }
    if (p != to) return tb_fail();
    return 1;
}
int tb_name(long id) {
    if (id < 0 || id >= tb_nn) return tb_fail();
    return tb_bytes(tb_file + tb_name_at[id], tb_name_len[id]);
}
int tb_records(int from, int to, int expected) {
    int p; long n; int i; int kind; long id; int op; char *shape; int j; int nr; int ri; int packed[4]; int tag;
    p = from; n = tb_uint(tb_file, &p, to);
    if (tb_bad || n < 0 || n != expected || n > MAXOUT) return tb_fail();
    i = 0;
    while (i < n && !tb_bad) {
        if (p >= to) return tb_fail();
        kind = tb_file[p] & 255; p = p + 1;
        if (kind == TB_RECORD_GLOBAL || kind == TB_RECORD_EXTERN) {
            id = tb_uint(tb_file, &p, to);
            tb_word(kind == TB_RECORD_GLOBAL ? ".global " : ".extern "); tb_name(id); tb_put(10);
        } else if (kind < 3) {
            id = tb_uint(tb_file, &p, to);
            if (kind == 0) { tb_name(id); tb_word(":\n"); }
            else if (kind == 1) {
                long ci; ci = tb_uint(tb_file, &p, to);
                if (ci < 0 || ci >= tb_nc) return tb_fail();
                tb_word(".str "); tb_name(id); tb_put(32);
                tb_quote(tb_file + tb_const_at[ci], tb_const_len[ci]); tb_put(10);
            } else {
                long count; count = tb_uint(tb_file, &p, to);
                if (count < 0 || count > 2147483647) return tb_fail();
                tb_word(".bss "); tb_name(id); tb_put(32); tb_decimal(count); tb_put(10);
            }
        } else if (kind == 3) {
            if (p >= to) return tb_fail();
            op = tb_file[p] & 255; p = p + 1;
            if (op >= TB_OPCOUNT) return tb_fail();
            shape = tb_shapes[op]; nr = 0; j = 0;
            while (shape[j]) { if (shape[j] == 114) nr = nr + 1; j = j + 1; }
            if (nr > 8 || (nr + 1) / 2 > to - p) return tb_fail();
            j = 0; while (j < (nr + 1) / 2) { packed[j] = tb_file[p + j] & 255; j = j + 1; }
            p = p + (nr + 1) / 2;
            if (nr % 2 && (packed[nr / 2] >> 4) != 15) return tb_fail();
            tb_word("  "); tb_word(tb_opnames[op]); ri = 0; j = 0;
            while (shape[j]) {
                long v;
                if (j == 0) tb_put(32); else tb_word(", ");
                if (shape[j] == 114) {
                    v = (packed[ri / 2] >> (4 * (ri % 2))) & 15;
                    if (v > 7) return tb_fail();
                    tb_put(114); tb_put(48 + v); ri = ri + 1;
                } else {
                    if (p >= to) return tb_fail();
                    tag = tb_file[p] & 255; p = p + 1;
                    if (tag > 1) return tb_fail();
                    if (shape[j] == 105) {
                        if (tag == 0) v = tb_sint(tb_file, &p, to);
                        else v = tb_uint(tb_file, &p, to);
                        if (tag == 0) tb_decimal(v); else tb_udecimal((unsigned long)v);
                    } else if (tag == 0) {
                        v = tb_sint(tb_file, &p, to); tb_decimal(v);
                    } else {
                        v = tb_uint(tb_file, &p, to); tb_name(v);
                    }
                }
                j = j + 1;
            }
            tb_put(10);
        } else return tb_fail();
        i = i + 1;
    }
    if (p != to || tb_bad) return tb_fail();
    return 1;
}
int tb_decode(int bytes, int target, int force_origin) {
    int n; int p; int i; int kind; int flags; long off; long len; long count;
    int sec_at[5]; int sec_len[5]; int sec_count[5]; int expected;
    char digest[32];
    tb_bad = 0; nout = 0;
    if (bytes < 136 || !tb_same(tb_file, "UTAPEBIN", 8)) return tb_fail();
    if (tb_le(tb_file + 8, 2) != 1 || tb_le(tb_file + 10, 2) != 0
       || tb_le(tb_file + 12, 2) != 1 || tb_le(tb_file + 14, 2) != 0
       || tb_le(tb_file + 16, 4) != 0 || tb_le(tb_file + 24, 4) != 64) return tb_fail();
    n = tb_le(tb_file + 20, 4);
    if (n < 3 || n > 4 || 64 + 24 * n > bytes) return tb_fail();
    if (tb_le(tb_file + 60, 4) > 6) return tb_fail();
    if (!force_origin && target && tb_le(tb_file + 60, 4) != 0
        && tb_le(tb_file + 60, 4) != target) return tb_fail();
    sha_init(); i = 64; while (i < bytes) { sha_byte(tb_file[i] & 255); i = i + 1; }
    sha_final(digest);
    if (!tb_same(digest, tb_file + 28, 32)) return tb_fail();
    p = 64 + 24 * n; i = 0;
    while (i < n) {
        int d; d = 64 + 24 * i;
        kind = tb_le(tb_file + d, 2); flags = tb_le(tb_file + d + 2, 2);
        if (kind != i + 1 || flags != (kind < 4 ? 1 : 0)) return tb_fail();
        off = tb_le(tb_file + d + 4, 8); len = tb_le(tb_file + d + 12, 8);
        count = tb_le(tb_file + d + 20, 4);
        expected = p + ((8 - p % 8) % 8);
        if (off != expected || len < 0 || len > bytes - off || count < 0) return tb_fail();
        while (p < off) { if (tb_file[p] != 0) return tb_fail(); p = p + 1; }
        sec_at[i] = off; sec_len[i] = len; sec_count[i] = count;
        p = off + len; i = i + 1;
    }
    if (p != bytes) return tb_fail();
    if (!tb_pool(tb_file, sec_at[0], sec_at[0] + sec_len[0], sec_count[0],
                 tb_name_at, tb_name_len, &tb_nn, 1)) return 0;
    if (!tb_pool(tb_file, sec_at[1], sec_at[1] + sec_len[1], sec_count[1],
                 tb_const_at, tb_const_len, &tb_nc, 0)) return 0;
    if (n == 4 && (sec_len[3] != 32 || sec_count[3] != 1)) return tb_fail();
    return tb_records(sec_at[2], sec_at[2] + sec_len[2], sec_count[2]);
}
int tb_read(char *path, int target, int force_origin) {
    int fd; int n; int got;
    model_dims(); setup();
    fd = ropen(path);
    if (fd < 0) return enoinput(path);
    n = 0;
    while (n < MAXOUT) {
        got = __read(fd, tb_file + n, MAXOUT - n);
        if (got < 0) { n = 0 - 1; break; }
        if (got == 0) break;
        n = n + got;
    }
    __close(fd);
    if (n <= 0 || n >= MAXOUT || !tb_decode(n, target, force_origin)) {
        __write(2, "tapebin: invalid or unsupported package\n", 40);
        return 1;
    }
    return 0;
}

int tb_target_id(char *target) {
    if (strsame(target, "lnx/x86_64")) return 1;
    if (strsame(target, "lnx/arm64")) return 2;
    if (strsame(target, "osx/x86_64")) return 3;
    if (strsame(target, "osx/arm64")) return 4;
    if (strsame(target, "win/x86_64")) return 5;
    if (strsame(target, "win/arm64")) return 6;
    return 0;
}

void tb_hash(char *dst, char *bytes, int n) {
    int i; sha_init(); i = 0;
    while (i < n) { sha_byte(bytes[i] & 255); i = i + 1; }
    sha_final(dst);
}
#include "tapebin_encode.inc"
