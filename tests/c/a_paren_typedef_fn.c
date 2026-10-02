/* A typedef return type before (name)(args), as in Csmith safe_math.h. */
#include <stdio.h>
typedef signed char I8;
#define FUNC_NAME(x) (safe_##x)
static I8 FUNC_NAME(add)(I8 x) { return x + 1; }
int main(void) { printf("%d\n", safe_add(41)); return 0; }
