/* R13-0 #03 P0 -- printf reaches the program only through the bundled
   assert() macro, and the default -ftrim-libc rejects it.
   Expected: compiles, runs, and exits 0 (argc == 1 holds).
   Got on 0.0.12 (df8cc9b4): rc=1, `unisacc: error: undefined function 'printf'`
   with no file:line:col -- the libneed closure missed the identifier the
   bundled header's macro expands to, and the diagnostic carries no position.
   The second path, "an argument makes the assertion fire", cannot be spelled
   here: this suite compiles once and runs once.  It is a separate gate entry
   (assert-fires) added with the fix, per cc-unisacc's R13-0 ruling. */
#include <stdio.h>
#include <assert.h>
int main(int argc, char **argv) {
    assert(argc == 1);
    return 0;
}
