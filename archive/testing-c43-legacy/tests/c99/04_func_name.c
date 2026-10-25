#include <stdio.h>
int first(void){ return __func__[0]; }
int main(void){
    const char *p = __func__;
    printf("%s %d %d %d\n", p, __func__[0], *__func__, first());
    return 0;
}
