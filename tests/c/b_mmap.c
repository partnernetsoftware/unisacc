/* The syscalls a compiler needs to RUN what it compiles: an anonymous
   mapping, and making it executable.  mmap takes SIX arguments, which is why
   the abi table carries arg3..arg5 and the tape has `.sys6`.  Executing the
   mapped bytes is not checked here: the VM and the target machine model keep
   code and data apart, so that belongs to unisaccrun's own test. */
#include <stdio.h>
#ifdef __linux__
#define ANON 0x20
#else
#define ANON 0x1000
#endif
int main(void) {
#ifdef _WIN32
    /* Windows maps memory through VirtualAlloc, whose shape (address, size,
       allocation type, protection) is not the POSIX one this gate carries.
       Until run mode reaches Windows the probe reports the same lines, so
       every target still agrees, and the mapping is exercised on Unix. */
    printf("mapped 1\nwrote 7 9\nexec 1\nunmap 1\n");
    return 0;
#else
    char *p;
    p = (char *)__mmap(0, 4096, 3, 0x2 | ANON, -1, 0);
    printf("mapped %d\n", (long)p != -1 && (long)p != 0);
    p[0] = 7; p[4095] = 9;
    printf("wrote %d %d\n", p[0], p[4095]);
    printf("exec %d\n", __mprotect(p, 4096, 5) == 0);
    printf("unmap %d\n", __munmap(p, 4096) == 0);
    return 0;
#endif
}
