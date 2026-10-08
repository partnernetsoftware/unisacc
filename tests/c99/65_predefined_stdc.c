#include <stdio.h>
int main(void){
#if defined(__STDC__) && defined(__STDC_VERSION__) && defined(__STDC_HOSTED__)
    printf("%d %d %d\n", __STDC__, __STDC_VERSION__ >= 199901L, __STDC_HOSTED__);
#else
    printf("missing\n");
#endif
    return 0;
}
