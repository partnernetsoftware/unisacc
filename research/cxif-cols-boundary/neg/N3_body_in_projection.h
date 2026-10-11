/* NEG N3: public projection must not smuggle function bodies / private types.
 * Design MUST reject such a projection when building CXIF.
 * Research-only counterexample — do not treat as a valid public header.
 */
#ifndef CSIH_COLS_BAD_PROJ_H
#define CSIH_COLS_BAD_PROJ_H
int csih_next_cp(const char *s, unsigned int *cp);
int csih_cp_cols(unsigned int cp);

/* foul: body in projection */
static int csih_cp_cols(unsigned int cp) {
    return cp < 0x1100 ? 1 : 2;
}

/* foul: private complete type leaked beside public decls */
struct csih_cols_priv_state {
    int cache[32];
};
#endif
