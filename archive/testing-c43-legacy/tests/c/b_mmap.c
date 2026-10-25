/* The syscalls a compiler needs to RUN what it compiles: an anonymous
   mapping, and making it executable.  mmap takes SIX arguments, which is why
   the abi table carries arg3..arg5 and the tape has `.sys6`.

   The shapes are the platform's own, so the program picks them the way
   <stdio.h> picks its O_* bits: Windows has VirtualAlloc(address, size,
   type, protection) where POSIX has mmap(addr, len, prot, flags, fd, off).

   Executing the mapped bytes is not checked here: the VM and the target
   machine model keep code and data apart, so that belongs to `-run`. */
#include <stdio.h>
#ifdef _WIN32
#define M_ARGS 0, 4096, 0x3000, 4, 0, 0      /* MEM_COMMIT|RESERVE, RW */
#define M_EXEC 0x20                          /* PAGE_EXECUTE_READ */
#else
#ifdef __linux__
#define ANON 0x20
#else
#define ANON 0x1000
#endif
#define M_ARGS 0, 4096, 3, 0x2 | ANON, -1, 0
#define M_EXEC 5                             /* PROT_READ | PROT_EXEC */
#endif
int main(void) {
    char *p;
    p = (char *)__mmap(M_ARGS);
    printf("mapped %d\n", (long)p != -1 && (long)p != 0);
    p[0] = 7; p[4095] = 9;
    printf("wrote %d %d\n", p[0], p[4095]);
    printf("exec %d\n", __mprotect(p, 4096, M_EXEC) == 0);
    printf("unmap %d\n", __munmap(p, 4096) == 0);
    return 0;
}
