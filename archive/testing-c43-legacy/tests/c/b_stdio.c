/* stdio over the syscall gate, and printf's hex/octal conversions */
#include <stdio.h>
int main() {
    puts("hello");
    fputs("a", stdout);
    fputc('b', stdout);
    putchar('c');
    putchar('\n');
    fwrite("xyz\n", 1, 4, stdout);
    printf("[%x][%X][%o][%08x][%-8x|][%x][%x]\n",
           48879, 48879, 64, 255, 255, 0, 1);
    return 0;
}
