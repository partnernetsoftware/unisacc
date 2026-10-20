/* system [C99 7.20.4.6]: system(NULL) asks whether a command processor is
   available and returns nonzero when it is.  No command is run. */
#include <stdlib.h>
#include <stdio.h>
int main(void) {
    int r = system(0);
    printf("command processor %s\n", r != 0 ? "available" : "absent");
    return r != 0 ? 0 : 1;
}
