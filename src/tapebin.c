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
        if (kind < 3) {
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

/* Text encoder.  The source is the exact tape currently in out[0..nout),
   so emitting a package does not ask Python or re-enter the C front end. */
#define TB_CONSTCAP 4194304
char tb_constdata[TB_CONSTCAP]; int tb_constused;
char tb_lit[1048576];
int tb_rec_count; int tb_outpos;

int tb_emit(int c) {
    if (tb_outpos >= MAXOUT) return tb_fail();
    tb_file[tb_outpos] = c; tb_outpos = tb_outpos + 1; return 1;
}
int tb_emit_bytes(char *s, int n) {
    int i; if (n < 0 || n > MAXOUT - tb_outpos) return tb_fail();
    i = 0; while (i < n) { tb_emit(s[i] & 255); i = i + 1; }
    return 1;
}
int tb_emit_u(unsigned long v) {
    while (v >= 128) { tb_emit((v & 127) | 128); v = v >> 7; }
    return tb_emit(v);
}
int tb_emit_s(long v) {
    int b; int done;
    do {
        b = v & 127; v = v >> 7;
        done = (v == 0 && b < 64) || (v == -1 && b >= 64);
        tb_emit(b | (done ? 0 : 128));
    } while (!done);
    return !tb_bad;
}
int tb_wle(int at, unsigned long v, int n) {
    int i; i = 0;
    while (i < n) { tb_file[at + i] = v & 255; v = v >> 8; i = i + 1; }
    return 0;
}
int tb_wsame(char *a, char *b, int n) {
    int i; i = 0;
    while (i < n) { if (a[i] != b[i]) return 0; i = i + 1; }
    return 1;
}
int tb_name_index(int at, int len) {
    int i;
    if (len <= 0 || tb_nn >= TB_MAXPOOL) return tb_fail();
    i = 0; while (i < len) {
        int c; c = out[at + i] & 255;
        if (c == 0 || c == 32 || c == 9 || c == 10 || c == 13 || c == 58) return tb_fail();
        i = i + 1;
    }
    i = 0;
    while (i < tb_nn) {
        if (len == tb_name_len[i] && tb_wsame(out + at, out + tb_name_at[i], len)) return i;
        i = i + 1;
    }
    tb_name_at[tb_nn] = at; tb_name_len[tb_nn] = len;
    tb_nn = tb_nn + 1; return tb_nn - 1;
}
int tb_const_index(int len) {
    int i;
    if (len < 0 || tb_nc >= TB_MAXPOOL) return tb_fail();
    i = 0;
    while (i < tb_nc) {
        if (len == tb_const_len[i] && tb_wsame(tb_lit, tb_constdata + tb_const_at[i], len)) return i;
        i = i + 1;
    }
    if (len > TB_CONSTCAP - tb_constused) return tb_fail();
    tb_const_at[tb_nc] = tb_constused; tb_const_len[tb_nc] = len;
    i = 0; while (i < len) { tb_constdata[tb_constused] = tb_lit[i]; tb_constused = tb_constused + 1; i = i + 1; }
    tb_nc = tb_nc + 1; return tb_nc - 1;
}
int tb_ws(int c) { return c == 32 || c == 9 || c == 44 || c == 91 || c == 93; }
int tb_skip(int p, int end) { while (p < end && tb_ws(out[p] & 255)) p = p + 1; return p; }
int tb_token_end(int p, int end) {
    while (p < end && !tb_ws(out[p] & 255) && out[p] != 59) p = p + 1;
    return p;
}
int tb_number(int at, int end, unsigned long *value, int *negative) {
    int p; int base; int digit; unsigned long v;
    p = at; *negative = 0; v = 0; base = 10;
    if (p < end && (out[p] == 45 || out[p] == 43)) { *negative = out[p] == 45; p = p + 1; }
    if (p + 1 < end && out[p] == 48 && (out[p + 1] == 120 || out[p + 1] == 88)) { base = 16; p = p + 2; }
    if (p >= end) return tb_fail();
    while (p < end) {
        int c; c = out[p] & 255;
        if (c >= 48 && c <= 57) digit = c - 48;
        else if (base == 16 && (c | 32) >= 97 && (c | 32) <= 102) digit = (c | 32) - 87;
        else return tb_fail();
        if (digit >= base || v > (~0UL - digit) / base) return tb_fail();
        v = v * base + digit; p = p + 1;
    }
    if (*negative && v > (1UL << 63)) return tb_fail();
    *value = v; return 1;
}
int tb_literal(int start, int end) {
    int p; int n; int c;
    p = start; n = 0;
    if (p >= end || out[p] != 34) return tb_fail();
    p = p + 1;
    while (p < end && out[p] != 34) {
        c = out[p] & 255;
        if (c == 92) {
            p = p + 1; if (p >= end) return tb_fail();
            c = out[p] & 255;
            if (c == 120) {
                if (p + 2 >= end) return tb_fail();
                c = bk_hex(out[p + 1] & 255) * 16 + bk_hex(out[p + 2] & 255);
                p = p + 2;
            } else if (c == 110) c = 10;
            else if (c == 116) c = 9;
            else if (c == 114) c = 13;
            else if (c == 48) c = 0;
        }
        if (n >= 1048576) return tb_fail();
        tb_lit[n] = c; n = n + 1; p = p + 1;
    }
    if (p >= end || out[p] != 34) return tb_fail();
    p = p + 1; while (p < end) { if (out[p] != 32 && out[p] != 9) return tb_fail(); p = p + 1; }
    return n;
}
int tb_text_pass(int pass, int textlen) {
    int line; int end; int next; int p; int q; int op; int i; int idx; int name; int regs[8]; int nr;
    int quoted; int escape;
    unsigned long values[8]; int neg[8]; int isname[8]; int regcount; char *shape;
    line = 0; tb_rec_count = 0;
    while (line < textlen && !tb_bad) {
        end = line; while (end < textlen && out[end] != 10) end = end + 1;
        next = end + 1;
        quoted = 0; escape = 0; p = line;
        while (p < end) {
            if (quoted) {
                if (escape) escape = 0;
                else if (out[p] == 92) escape = 1;
                else if (out[p] == 34) quoted = 0;
            } else if (out[p] == 34) quoted = 1;
            else if (out[p] == 59) { end = p; break; }
            p = p + 1;
        }
        while (end > line && (out[end - 1] == 32 || out[end - 1] == 9)) end = end - 1;
        p = line; while (p < end && (out[p] == 32 || out[p] == 9)) p = p + 1;
        if (p == end) { line = next; continue; }
        tb_rec_count = tb_rec_count + 1;
        if (end - p >= 5 && tb_wsame(out + p, ".str ", 5)) {
            p = p + 5; q = tb_token_end(p, end); name = tb_name_index(p, q - p);
            p = q; while (p < end && (out[p] == 32 || out[p] == 9)) p = p + 1;
            idx = tb_literal(p, end); if (tb_bad) return 0;
            i = tb_const_index(idx);
            if (pass) { tb_emit(1); tb_emit_u(name); tb_emit_u(i); }
        } else if (end - p >= 5 && tb_wsame(out + p, ".bss ", 5)) {
            unsigned long v; int nneg;
            p = p + 5; q = tb_token_end(p, end); name = tb_name_index(p, q - p);
            p = tb_skip(q, end); q = tb_token_end(p, end);
            tb_number(p, q, &v, &nneg);
            if (tb_bad || nneg || v > 2147483647 || tb_skip(q, end) != end) return tb_fail();
            if (pass) { tb_emit(2); tb_emit_u(name); tb_emit_u(v); }
        } else if (out[end - 1] == 58) {
            name = tb_name_index(p, end - p - 1);
            if (pass) { tb_emit(0); tb_emit_u(name); }
        } else {
            q = tb_token_end(p, end); op = 0;
            while (op < TB_OPCOUNT) {
                int oplen; oplen = 0; while (tb_opnames[op][oplen]) oplen = oplen + 1;
                if (q - p == oplen && tb_wsame(out + p, tb_opnames[op], q - p)) break;
                op = op + 1;
            }
            if (op >= TB_OPCOUNT) return tb_fail();
            shape = tb_shapes[op]; p = q; nr = 0; i = 0;
            while (shape[i]) {
                p = tb_skip(p, end); q = tb_token_end(p, end);
                if (q <= p) return tb_fail();
                if (shape[i] == 114) {
                    if (q - p > 2 && out[p] == 114 &&
                        (out[p + 2] == 43 || out[p + 2] == 45)) q = p + 2;
                    if (q - p != 2 || out[p] != 114 || out[p + 1] < 48 || out[p + 1] > 55) return tb_fail();
                    regs[nr] = out[p + 1] - 48; nr = nr + 1;
                } else if (shape[i] == 105) {
                    tb_number(p, q, &values[i], &neg[i]); isname[i] = 0;
                } else {
                    isname[i] = 1; values[i] = tb_name_index(p, q - p);
                }
                if (tb_bad || i >= 7 || nr > 8) return tb_fail();
                p = q; i = i + 1;
            }
            p = tb_skip(p, end);
            if (p < end && out[p] != 59) return tb_fail();
            if (pass) {
                tb_emit(3); tb_emit(op);
                regcount = 0; while (regcount < nr) {
                    int hi; hi = regcount + 1 < nr ? regs[regcount + 1] : 15;
                    tb_emit(regs[regcount] | (hi << 4)); regcount = regcount + 2;
                }
                i = 0; while (shape[i]) {
                    if (shape[i] == 105) {
                        if (neg[i]) {
                            tb_emit(0); tb_emit_s(0 - (long)(values[i] - 1) - 1);
                        } else if (values[i] < (1UL << 63)) {
                            tb_emit(0); tb_emit_s((long)values[i]);
                        } else { tb_emit(1); tb_emit_u(values[i]); }
                    } else if (shape[i] != 114) { tb_emit(1); tb_emit_u(values[i]); }
                    i = i + 1;
                }
            }
        }
        line = next;
    }
    return !tb_bad;
}
int tb_encode(int textlen, int origin) {
    int i; int p; int at1; int at2; int at3; int end1; int end2; int nr; char digest[32];
    if (origin < 0 || origin > 6) return 0;
    tb_bad = 0; tb_nn = 0; tb_nc = 0; tb_constused = 0;
    if (!tb_text_pass(0, textlen)) return 0;
    nr = tb_rec_count; tb_outpos = 64 + 3 * 24;
    at1 = tb_outpos; tb_emit_u(tb_nn);
    i = 0; while (i < tb_nn) { tb_emit_u(tb_name_len[i]); tb_emit_bytes(out + tb_name_at[i], tb_name_len[i]); i = i + 1; }
    end1 = tb_outpos;
    while (tb_outpos % 8) tb_emit(0);
    at2 = tb_outpos; tb_emit_u(tb_nc);
    i = 0; while (i < tb_nc) { tb_emit_u(tb_const_len[i]); tb_emit_bytes(tb_constdata + tb_const_at[i], tb_const_len[i]); i = i + 1; }
    end2 = tb_outpos;
    while (tb_outpos % 8) tb_emit(0);
    at3 = tb_outpos; tb_emit_u(nr);
    if (!tb_text_pass(1, textlen) || tb_rec_count != nr || tb_bad) return 0;
    p = tb_outpos;
    tb_wle(0, 0, 64); /* zero the header before filling its fields */
    tb_emit_bytes("", 0);
    i = 0; while (i < 8) { tb_file[i] = "UTAPEBIN"[i]; i = i + 1; }
    tb_wle(8, 1, 2); tb_wle(12, 1, 2); tb_wle(20, 3, 4); tb_wle(24, 64, 4); tb_wle(60, origin, 4);
    tb_wle(64, 1, 2); tb_wle(66, 1, 2); tb_wle(68, at1, 8); tb_wle(76, end1 - at1, 8);
    tb_wle(84, tb_nn, 4);
    tb_wle(88, 2, 2); tb_wle(90, 1, 2); tb_wle(92, at2, 8); tb_wle(100, end2 - at2, 8); tb_wle(108, tb_nc, 4);
    tb_wle(112, 3, 2); tb_wle(114, 1, 2); tb_wle(116, at3, 8); tb_wle(124, p - at3, 8); tb_wle(132, nr, 4);
    sha_init(); i = 64; while (i < p) { sha_byte(tb_file[i] & 255); i = i + 1; }
    sha_final(digest); i = 0; while (i < 32) { tb_file[28 + i] = digest[i]; i = i + 1; }
    return p;
}
