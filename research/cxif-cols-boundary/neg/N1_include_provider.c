/* NEG N1: textual include of provider source — design MUST reject in strict .cx / CXIF mode.
 * Research-only counterexample. Do not "fix" by making include work.
 */
#include "../provider/cols.public.h" /* stand-in; real foul is #include of provider .cx/.c bodies */
/* Worse form (documented, not compiled here): #include "apps/csih/cols.cx" */

int main(void) {
    unsigned int cp;
    return csih_next_cp("A", &cp) == 1 ? 0 : 1;
}
