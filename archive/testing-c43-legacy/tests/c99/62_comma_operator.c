/* C99 6.5.17: the comma operator -- left operand evaluated first, its value discarded, result is the right operand. */
#include <stdio.h>
int main(void){ int a = 0, b; b = (a = 3, a + 4); int i, j; for (i = 0, j = 10; i < j; i++, j--) ; printf("%d %d %d %d\n", a, b, i, j); return 0; }
