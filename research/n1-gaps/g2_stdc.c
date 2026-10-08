#include <stdio.h>
int main(void){
#if defined(__STDC__) && defined(__STDC_VERSION__) && defined(__STDC_HOSTED__)
 printf("%d %ld %d\n", __STDC__, (long)__STDC_VERSION__, __STDC_HOSTED__);
#else
 printf("missing\n");
#endif
 return 0; }
