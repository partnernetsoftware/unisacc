/* R13-0 #01 P1 -- the printf `#` flag is ignored.
   Expected stdout: [0xff] [010] [0XFF]
   Got on 0.0.12 (df8cc9b4): [ff] [10] [FF]
   Both the walker's printf desugaring and the vsnprintf body need the flag. */
#include <stdio.h>
int main(void) {
    char b[32];
    printf("[%#x] [%#o] [%#X]\n", 255, 8, 255);
    sprintf(b, "[%#x] [%#o]", 255, 8);
    printf("%s\n", b);
    return 0;
}
