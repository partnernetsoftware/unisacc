/* Independent platform observation, not a cross-compiler equality probe.
 * Run with host cc and unisacc -run; plain char signedness is implementation-defined.
 * unisacc currently uses signed plain char, including on Linux arm64 where gcc does not.
 */
#include <stdio.h>
int main(void) {
    char plain = (char)255;
    signed char signed_value = (signed char)255;
    unsigned char unsigned_value = (unsigned char)255;
    printf("plain=%d signed=%d unsigned=%d\n", plain, signed_value, unsigned_value);
    return 0;
}
