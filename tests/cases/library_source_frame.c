/* Exercise the library's multi-source identity frame in its tracked domain. */
#include "../../exec/c/libunisacc.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    if (argc!=3 || setenv("UNISACC_RF_CAPTURE",argv[2],1)) return 2;
    us_context *c=us_new(argv[1]);
    if (!c) return 3;
    if (us_add_source(c,"a.c","int helper(void){return 7;}\n") ||
        us_add_source(c,"b.c","int helper(void); int answer(void){return helper();}\n") ||
        us_compile(c,"lnx/x86_64",1) || us_relocate(c)) {
        fprintf(stderr,"library source frame: %s\n",us_error(c)); us_free(c); return 4;
    }
    int (*answer)(void)=(int (*)(void))us_sym(c,"answer");
    if (!answer || answer()!=7) { us_free(c); return 5; }
    us_free(c);
    puts("LIBRARY_SOURCE_FRAME result=7");
    return 0;
}
