/* Only for building unisacc.c with a reference compiler: our intrinsics become
 * POSIX calls, and argc/argv come from a real main().  unisa itself lowers
 * these through the `.sys` gate and the loader's argv. */
#include <errno.h> /* cxm_rd distinguishes absence from other host I/O failures. */
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <io.h>
#include <fcntl.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
/* The compiler's fd contract is int. Let the CRT own the native HANDLE;
   never narrow a HANDLE to int or close it twice. */
static HANDLE unisa_ref_handle(int fd) {
    if (fd < 0) return INVALID_HANDLE_VALUE;
    return (HANDLE)_get_osfhandle(fd);
}
static int unisa_ref_open(const char *path, unsigned long access, unsigned long creation) {
    HANDLE h; int fd; int flags;
    h = CreateFileA(path, (DWORD)access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, (DWORD)creation, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    flags = _O_BINARY;
    if ((access & GENERIC_READ) && (access & GENERIC_WRITE)) flags |= _O_RDWR;
    else if (access & GENERIC_WRITE) flags |= _O_WRONLY;
    else flags |= _O_RDONLY;
    fd = _open_osfhandle((intptr_t)h, flags);
    if (fd < 0) CloseHandle(h);
    return fd;
}
static int unisa_ref_read(int fd, void *buf, int count) {
    DWORD actual = 0; HANDLE h = unisa_ref_handle(fd);
    if (count < 0 || h == INVALID_HANDLE_VALUE || !ReadFile(h, buf, (DWORD)count, &actual, NULL)) return -1;
    return (int)actual;
}
static int unisa_ref_write(int fd, const void *buf, int count) {
    DWORD actual = 0; HANDLE h = unisa_ref_handle(fd);
    if (count < 0 || h == INVALID_HANDLE_VALUE || !WriteFile(h, buf, (DWORD)count, &actual, NULL)) return -1;
    return (int)actual;
}
static int64_t unisa_ref_lseek(int fd, int64_t offset, int method) {
    LARGE_INTEGER distance, result;
    HANDLE h = unisa_ref_handle(fd); distance.QuadPart = offset;
    if (h == INVALID_HANDLE_VALUE || !SetFilePointerEx(h, distance, &result, (DWORD)method)) return -1;
    return result.QuadPart;
}
static intptr_t unisa_ref_mmap(intptr_t address, size_t length, unsigned long allocation,
                              unsigned long protection, intptr_t descriptor, int64_t offset) {
    if (descriptor != 0 || offset != 0) { SetLastError(ERROR_NOT_SUPPORTED); return -1; }
    return (intptr_t)VirtualAlloc((void *)address, length, (DWORD)allocation, (DWORD)protection);
}
static int unisa_ref_mprotect(intptr_t address, size_t length, unsigned long protection) {
    DWORD old;
    if (!VirtualProtect((void *)address, length, (DWORD)protection, &old)) return -1;
    if ((protection & 0xf0) && !FlushInstructionCache(GetCurrentProcess(), (void *)address, length)) return -1;
    return 0;
}
static int unisa_ref_munmap(intptr_t address, size_t length) {
    (void)length;
    return VirtualFree((void *)address, 0, MEM_RELEASE) ? 0 : -1;
}
#define __open(...) unisa_ref_open(__VA_ARGS__)
#define __read(f,b,n) unisa_ref_read((f),(b),(n))
#define __write(f,b,n) unisa_ref_write((f),(b),(n))
#define __close(f) _close(f)
#define __lseek(f,o,w) unisa_ref_lseek((f),(o),(w))
#define __mmap(a,n,p,f,d,o) unisa_ref_mmap((a),(n),(p),(f),(d),(o))
#define __mprotect(a,n,p) unisa_ref_mprotect((a),(n),(p))
#define __munmap(a,n) unisa_ref_munmap((a),(n))
#define __exit(c) ExitProcess((unsigned int)(c))
#ifdef __argc
#undef __argc
#endif
#ifdef __argv
#undef __argv
#endif
#else
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#define __open(...)    open(__VA_ARGS__)
#define __read(f,b,n)  (int)read((f),(b),(n))
#define __write(f,b,n) (int)write((f),(b),(n))
#define __close(f)     close(f)
#define __lseek(f,o,w) (long)lseek((f),(o),(w))
#include <sys/mman.h>
/* run mode maps memory and makes it executable; the intrinsics unisa lowers
   through the `.sys6`/`.sys` gate are these calls here */
#define __mmap(a,n,p,f,d,o) (long)mmap((void *)(long)(a),(n),(p),(f),(d),(o))
#define __mprotect(a,n,p)   mprotect((void *)(long)(a),(n),(p))
#define __munmap(a,n)       munmap((void *)(long)(a),(n))
#include <stdlib.h>
#define __exit(c)      exit(c)
#endif
static int   G_argc;
static char **G_argv;
#define __argc()   G_argc
#define __argv(i)  G_argv[i]
#define main       unisacc_main
