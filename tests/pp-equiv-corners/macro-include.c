#define PICK_THROUGH(x) x
#define HEADER(x) PICK_THROUGH(x)
#include HEADER("sub/noeol.h")
int main(void){return NOEOL;}
