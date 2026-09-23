#include <stdio.h>
#include <stdarg.h>
static int twice(int n, ...){ va_list a,b; va_start(a,n); va_copy(b,a); int s=0; for(int i=0;i<n;i++) s+=va_arg(a,int); for(int i=0;i<n;i++) s+=va_arg(b,int); va_end(a); va_end(b); return s; }
int main(void){ printf("%d\n", twice(2,3,4)); return 0; }
