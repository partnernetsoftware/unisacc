/* R13-0c #32: identifiers of 32 characters or more could not be looked up at
   all.  The symbol-name store was 32 bytes wide and `sfind_is` compared by the
   NUL it hoped was inside; a name that long had no room for it, so the scan
   ran into the next entry and the length never matched.  The symptom depended
   on where the name was used: as an array bound "a constant is required
   here", as a type name "expected ')'", as a plain variable "unknown
   identifier".  gcc accepted every construct below all along.

   Found while working on fb12-23 (miniz): `MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE`
   is exactly 32 characters, and shortening that one name moved the fixture
   past this defect.  The threshold was exact and depended on nothing else --
   31 characters passed, 32 failed, with or without underscores (measured).

   All three routes must accept this file now: gcc, the Python reference and
   the product.  The boundary is 63 (C99 6.4.2.1 requires that many significant
   characters); 64 and up are refused by the reference with a position -- that
   is the separate fb12-multi fixture 32-identifier-too-long, not this one. */
#include <stdio.h>

/* 32 characters, and 40 characters, in each of the four roles a name can
   play.  The 40-character forms are also here because a fix that widens the
   store to exactly 32 would still truncate them. */

enum { ENUM_CONSTANT_NAME_32_CHARS_OK = 11 };
enum { ENUM_CONSTANT_NAME_LONGER_FORTY_CHARS_XX = 22 };
/* exactly 63 -- the widest name C99 6.4.2.1 requires an implementation to
   keep distinct.  64 is refused, which is why the boundary is tested here and
   the refusal lives in the fb12-multi fixture instead. */
enum { N63_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_Z = 33 };

typedef int TypedefNameOfThirtyTwoCharacters;
typedef int TypedefNameThatIsQuiteALotLongerFortyX;

/* a global variable, defined and then read */
int GlobalVariableNameThirtyTwoChars;
int GlobalVariableNameFortyCharactersLongXxxx = 7;

int main(void) {
    /* a local array whose bound is an enum constant of 32 and of 40 chars */
    char local_array_bound_32[ENUM_CONSTANT_NAME_32_CHARS_OK];
    char local_array_bound_40[ENUM_CONSTANT_NAME_LONGER_FORTY_CHARS_XX];
    char local_array_bound_63[N63_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_Z];
    /* a local variable with each name length */
    int LocalVariableNameIsThirtyTwoChars = 3;
    int LocalVariableNameIsFortyCharactersLongXx = 4;
    /* a typedef used to declare a variable */
    TypedefNameOfThirtyTwoCharacters td32 = 5;
    TypedefNameThatIsQuiteALotLongerFortyX td40 = 6;

    GlobalVariableNameThirtyTwoChars = 1;
    local_array_bound_32[0] = 0;
    local_array_bound_40[0] = 0;
    local_array_bound_63[0] = 0;
    printf("%d %d %d %d %d %d\n",
           (int)sizeof local_array_bound_32 + (int)sizeof local_array_bound_40
           + (int)sizeof local_array_bound_63,
           LocalVariableNameIsThirtyTwoChars,
           LocalVariableNameIsFortyCharactersLongXx,
           (int)td32 + (int)td40,
           GlobalVariableNameThirtyTwoChars,
           GlobalVariableNameFortyCharactersLongXxxx);
    return 0;
}
