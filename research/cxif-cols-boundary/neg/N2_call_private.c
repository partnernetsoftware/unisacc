/* NEG N2: call a name outside the public set — design MUST reject.
 * csih_cols_internal_width is intentionally NOT in cols.public.set / CXIF public[].
 * Research-only counterexample.
 */
int csih_next_cp(const char *s, unsigned int *cp);
int csih_cp_cols(unsigned int cp);
/* forged private — must not be visible via CXIF */
int csih_cols_internal_width(unsigned int cp);

int main(void) {
    unsigned int cp = 0x4e00;
    /* leak: consumer depends on non-public symbol */
    return csih_cols_internal_width(cp) + csih_cp_cols(cp) > 0 ? 0 : 1;
}
