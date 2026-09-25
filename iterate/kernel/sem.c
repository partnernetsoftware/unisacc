/* sem.c -- the semantic check of a (mixed) unisa_model.inc [J10 step 3].
 *
 * check.sh builds ONE file: a model.inc, then kernel/unisa_core.c with its
 * #include lines removed (the REAL kernel: infer() and inf() as shipped),
 * then this driver.  For every stage, every key of the stage's domain (field
 * values enumerated with the last field fastest, as DENSE is laid out) and
 * every head, infer() evaluating MODEL must return DENSE's answer -- which
 * genmodel wrote from the gold TSV labels -- and the maximum logit z[] must
 * be unique.  DENSE's layout (STAGE_DOFF, DENSE_LEN) is checked on the way.
 * Exit 0 only when nothing is wrong, nothing ties and every byte of DENSE
 * was visited once.
 */
int main(void) {
    int s; int m; int nh; int nk; int f; int idx; int r; int h; int got;
    int want; int c; int cnt; int best; int ncl;
    int key[4];
    int bad; int tie; int keys; int dec; int doff; int layout;
    model_dims();
    bad = 0; tie = 0; keys = 0; dec = 0; doff = 0; layout = 0;
    s = 0;
    while (s < NSTAGE) {
        m = STAGE_M[s]; nh = STAGE_NH[s];
        if (STAGE_DOFF[s] != doff) layout = layout + 1;
        nk = 1;
        f = 0;
        while (f < m) { nk = nk * STAGE_VN[s * 4 + f]; f = f + 1; }
        idx = 0;
        while (idx < nk) {
            r = idx;
            f = m - 1;
            while (f >= 0) { key[f] = r % STAGE_VN[s * 4 + f]; r = r / STAGE_VN[s * 4 + f]; f = f - 1; }
            h = 0;
            while (h < nh) {
                got = infer(s, key, h);
                want = DENSE[STAGE_DOFF[s] + idx * nh + h] & 255;
                if (got != want) {
                    if (bad < 5) printf("sem: stage %d key %d head %d: infer %d, TSV label %d\n", s, idx, h, got, want);
                    bad = bad + 1;
                }
                if (got >= 0) {
                    ncl = STAGE_NCLS[s * 16 + h];
                    best = z[got]; cnt = 0; c = 0;
                    while (c < ncl) { if (z[c] == best) cnt = cnt + 1; if (z[c] > best) cnt = cnt + 100; c = c + 1; }
                    if (cnt != 1) {
                        if (tie < 5) printf("sem: stage %d key %d head %d: max not unique\n", s, idx, h);
                        tie = tie + 1;
                    }
                }
                dec = dec + 1;
                h = h + 1;
            }
            keys = keys + 1;
            idx = idx + 1;
        }
        doff = doff + nk * nh;
        s = s + 1;
    }
    if (doff != DENSE_LEN) layout = layout + 1;
    printf("sem: %d stages, %d keys, %d (key, head) decisions, %d wrong, %d not unique, %d layout errors\n", NSTAGE, keys, dec, bad, tie, layout);
    return bad != 0 || tie != 0 || layout != 0 || dec == 0;
}
