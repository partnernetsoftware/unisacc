#include <stdio.h>
/* call libc puts through a different C name */
int my_puts(const char*) __asm__("puts");
int main(void){my_puts("via-label");return 0;}
