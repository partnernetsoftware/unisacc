/* R13-0 #02 P1 -- the `hh` and `h` length modifiers do not narrow.
   Expected stdout: [44] [4464] [255] [1]
   Got on 0.0.12 (df8cc9b4): [300] [70000] [511] [65537]
   %hhd of 300 is 44, %hd of 70000 is 4464, %hhu of 511 is 255, %hu of 65537 is 1.
   Same two places as #01: the printf desugaring and the vsnprintf body. */
#include <stdio.h>
int main(void) {
    printf("[%hhd] [%hd] [%hhu] [%hu]\n", 300, 70000, 511, 65537);
    return 0;
}
