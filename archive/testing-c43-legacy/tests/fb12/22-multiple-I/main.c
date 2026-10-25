/* With more than one -I option only the LAST directory is searched.
   Headers used: 22-multiple-I/incA/a.h and 22-multiple-I/incB/b.h.
   Hit by miniz (-I. for its headers plus -I for a generated
   miniz_export.h) - every real build with two include dirs fails with
   the location-less "reject: no such file for #include" (see bug 07). */
#include <stdio.h>
#include "a.h"
#include "b.h"
int main(void) { printf("%d\n", A_VAL + B_VAL); return 0; }
