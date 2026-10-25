/* main(argc, argv).  Both front ends called main without ever loading r0/r1,
   so argc was whatever __init left behind; osx/x86_64 read argc from [rsp],
   where dyld had left a return address; Windows had no argv at all; and the
   x86 argvget loaded the element into r11 and stopped.  No probe looked.
   argv[0] is a path that differs between runs, so print only what C fixes. */
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    int i;
    int n;
    n = 0;
    for (i = 0; i < argc; i++) n = n + (int)strlen(argv[i]);
    printf("argc>=1 %d  argv[argc]==0 %d  argv[0] nonempty %d  lengths>0 %d\n",
           argc >= 1, argv[argc] == 0, argv[0][0] != 0, n > 0);
    return 0;
}
