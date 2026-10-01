/* Shared v1 text encoder: classic reference and model-driver IO adapter. */
/* Text encoder.  The source is the exact tape currently in out[0..nout),
   so emitting a package does not ask Python or re-enter the C front end. */
#define TB_CONSTCAP 4194304
char tb_constdata[TB_CONSTCAP]; int tb_constused;
char tb_lit[1048576];
int tb_rec_count; int tb_outpos; int tb_minor;

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
    int quoted; int escape; int unit; int gdef;
    unsigned long values[8]; int neg[8]; int isname[8]; int regcount; char *shape;
    line = 0; tb_rec_count = 0; unit = 0; gdef = 0;
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
        if (end - p >= 6 && tb_wsame(out + p, ".unit", 5) && (out[p + 5] == 32 || out[p + 5] == 9)) {
            p = p + 6; while (p < end && (out[p] == 32 || out[p] == 9)) p = p + 1;
            q = tb_token_end(p, end);
            if (tb_rec_count != 1 || q - p != 1 || out[p] != 50 || tb_skip(q, end) != end) return tb_fail();
            unit = 1;
            if (pass) { tb_emit(TB_RECORD_UNIT); tb_emit_u(2); }
        } else if (end - p >= 7 &&
                   ((end - p >= 8 && (tb_wsame(out + p, ".global", 7) || tb_wsame(out + p, ".extern", 7)) && (out[p + 7] == 32 || out[p + 7] == 9)) ||
                    (tb_wsame(out + p, ".gdef", 5) && (out[p + 5] == 32 || out[p + 5] == 9)))) {
            int rk; rk = out[p + 1] == 103 && out[p + 2] == 108 ? TB_RECORD_GLOBAL :
                         (out[p + 1] == 101 ? TB_RECORD_EXTERN : TB_RECORD_GDEF);
            p = p + (rk == TB_RECORD_GDEF ? 6 : 8);
            while (p < end && (out[p] == 32 || out[p] == 9)) p = p + 1;
            q = tb_token_end(p, end);
            i = q; while (i < end && (out[i] == 32 || out[i] == 9)) i = i + 1;
            if (q == p || i != end) return tb_fail();
            i = p; while (i < q) {
                int c; c = out[i];
                if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95
                    || (i > p && ((c >= 48 && c <= 57) || c == 46 || c == 36)))) return tb_fail();
                i = i + 1;
            }
            name = tb_name_index(p, q - p);
            if (rk == TB_RECORD_GDEF) gdef = 1;
            if (pass) { tb_emit(rk); tb_emit_u(name); }
        } else if (end - p >= 5 && tb_wsame(out + p, ".str ", 5)) {
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
    if (gdef && !unit) return tb_fail();
    if (pass && unit != tb_minor) return tb_fail();
    if (!pass) tb_minor = unit;
    return !tb_bad;
}
int tb_encode(int textlen, int origin) {
    int i; int p; int at1; int at2; int at3; int end1; int end2; int nr; char digest[32];
    if (origin < 0 || origin > 6) return 0;
    tb_bad = 0; tb_nn = 0; tb_nc = 0; tb_constused = 0; tb_minor = 0;
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
    tb_wle(8, 1, 2); tb_wle(10, tb_minor, 2); tb_wle(12, 1, 2); tb_wle(20, 3, 4); tb_wle(24, 64, 4); tb_wle(60, origin, 4);
    tb_wle(64, 1, 2); tb_wle(66, 1, 2); tb_wle(68, at1, 8); tb_wle(76, end1 - at1, 8);
    tb_wle(84, tb_nn, 4);
    tb_wle(88, 2, 2); tb_wle(90, 1, 2); tb_wle(92, at2, 8); tb_wle(100, end2 - at2, 8); tb_wle(108, tb_nc, 4);
    tb_wle(112, 3, 2); tb_wle(114, 1, 2); tb_wle(116, at3, 8); tb_wle(124, p - at3, 8); tb_wle(132, nr, 4);
    tb_hash(digest, tb_file + 64, p - 64); i = 0; while (i < 32) { tb_file[28 + i] = digest[i]; i = i + 1; }
    return p;
}
