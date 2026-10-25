/* `extern const char x[];` far before its definition (lua_ident): the size comes from the definition, not from the next `=` in the file -- compared with cc */
#include <stdio.h>
extern const char id[];
extern int arr[];
static int other[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
const char id[] = "$Version: " "5.4" " $";
int arr[3] = {7, 8, 9};
int main(void) { printf("%s %d %d %d %d\n", id, (int)sizeof id, (int)sizeof other, arr[2], other[8]); return 0; }
