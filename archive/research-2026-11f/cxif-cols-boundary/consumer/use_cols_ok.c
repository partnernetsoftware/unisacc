/* POSITIVE sample: consumer sees only the public set (via projection / future CXIF load).
 * Research-only. Not wired into csih.sh or tests/gate.sh.
 * Expected (design): accept — only csih_next_cp / csih_cp_cols used.
 */
/* In product today this would be -include csih_cols.h; under CXIF, load cols.cxif.json instead. */
int csih_next_cp(const char *s, unsigned int *cp);
int csih_cp_cols(unsigned int cp);

int sample_measure(const char *s) {
    unsigned int cp;
    int n, cols = 0;
    if (!s) return 0;
    while ((n = csih_next_cp(s, &cp)) > 0) {
        cols += csih_cp_cols(cp);
        s += n;
    }
    return cols;
}

int main(void) {
    return sample_measure("A") == 1 ? 0 : 1;
}
