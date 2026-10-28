/* Exercise actual host-library cache cleanup and retry after a runtime error. */
#include "../../exec/c/libunisacc.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    for (int i=0;i<3;i++) {
        us_context *c=us_new(argv[1]);
        if(!c || us_add_source(c,"probe.c","int answer(void){return 7;}")) return 3;
        if(setenv("UNISA_MAXSTEPS","0",1) || !us_compile(c,"lnx/x86_64",1)) return 4;
        if(setenv("UNISA_MAXSTEPS","200000000",1) || us_compile(c,"lnx/x86_64",1) || us_relocate(c)) {
            fprintf(stderr,"retry: %s\n",us_error(c)); us_free(c); return 5;
        }
        int (*answer)(void)=(int (*)(void))us_sym(c,"answer");
        if(!answer || answer()!=7) { us_free(c); return 6; }
        us_free(c);
    }
    unsetenv("UNISA_MAXSTEPS");
    puts("CACHE_RECOVERY cycles=3");
    return 0;
}
