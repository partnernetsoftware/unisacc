/* 0.0.27 H1: dlfcn.h, pwd.h, sys/mman.h and the user ids forward to the system C library */
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/mman.h>
int main(void) {
    void *h = dlopen(0, RTLD_NOW);
    void *f = h ? dlsym(h, "strlen") : 0;
    struct passwd *pw = getpwuid(getuid());
    char *m = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    int ok = m != MAP_FAILED;
    if (ok) { strcpy(m, "mapped"); ok = strcmp(m, "mapped") == 0 && munmap(m, 4096) == 0; }
    printf("dl %d %d pw %d mmap %d\n", h != 0, f != 0, pw && pw->pw_name && pw->pw_name[0], ok);
    return 0;
}
