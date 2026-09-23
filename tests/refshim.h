/* Only for building unisacc.c with a reference compiler: our intrinsics become
 * POSIX calls, and argc/argv come from a real main().  unisa itself lowers
 * these through the `.sys` gate and the loader's argv. */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define __open(...)    open(__VA_ARGS__)
#define __read(f,b,n)  (int)read((f),(b),(n))
#define __write(f,b,n) (int)write((f),(b),(n))
#define __close(f)     close(f)
#include <sys/mman.h>
/* run mode maps memory and makes it executable; the intrinsics unisa lowers
   through the `.sys6`/`.sys` gate are these calls here */
#define __mmap(a,n,p,f,d,o) (long)mmap((void *)(long)(a),(n),(p),(f),(d),(o))
#define __mprotect(a,n,p)   mprotect((void *)(long)(a),(n),(p))
#define __munmap(a,n)       munmap((void *)(long)(a),(n))
#include <stdlib.h>
#define __exit(c)      exit(c)
static int   G_argc;
static char **G_argv;
#define __argc()   G_argc
#define __argv(i)  G_argv[i]
#define main       unisacc_main
