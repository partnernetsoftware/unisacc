/* POSIX directory walk for the host cx driver on Windows.
   include/cx.h includes <dirent.h>. The product include/dirent.h calls
   _ux_sym. -I tools/cxcc-hostinc finds this file first. FindFirstFileA
   returns "." and ".." as well; the caller filters them. Linux and OSX
   do not use this file. It has not been executed on Windows. */
#ifndef CXCC_HOST_DIRENT_H
#define CXCC_HOST_DIRENT_H
#ifdef _WIN32
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>
#define lstat(p, st) stat((p), (st))
#ifndef ENOTDIR
#define ENOTDIR 20
#endif
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
struct dirent {
    unsigned long d_ino;
    unsigned char d_type;
    char d_name[260];
};
typedef struct cxcc_dir {
    HANDLE h;
    WIN32_FIND_DATAA data;
    int pending;
    int done;
    struct dirent ent;
} DIR;

static DIR *opendir(const char *path) {
    char pat[PATH_MAX];
    size_t n;
    DIR *d;
    if (!path || !path[0]) { errno = ENOENT; return 0; }
    n = strlen(path);
    if (n + 3 >= sizeof pat) { errno = ENAMETOOLONG; return 0; }
    memcpy(pat, path, n);
    if (pat[n - 1] != '\\' && pat[n - 1] != '/') pat[n++] = '\\';
    pat[n++] = '*';
    pat[n] = 0;
    d = (DIR *)malloc(sizeof *d);
    if (!d) { errno = ENOMEM; return 0; }
    memset(d, 0, sizeof *d);
    d->h = FindFirstFileA(pat, &d->data);
    if (d->h == INVALID_HANDLE_VALUE) {
        DWORD attr, err;
        err = GetLastError();
        attr = GetFileAttributesA(path);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) errno = ENOTDIR;
        else if (err == ERROR_ACCESS_DENIED) errno = EACCES;
        else errno = ENOENT;
        free(d);
        return 0;
    }
    d->pending = 1;
    errno = 0;
    return d;
}

static struct dirent *readdir(DIR *d) {
    size_t i;
    if (!d) { errno = EBADF; return 0; }
    errno = 0;
    if (d->done) return 0;
    if (!d->pending) {
        if (!FindNextFileA(d->h, &d->data)) { d->done = 1; return 0; }
    }
    d->pending = 0;
    for (i = 0; i < 259 && d->data.cFileName[i]; i++) d->ent.d_name[i] = d->data.cFileName[i];
    d->ent.d_name[i] = 0;
    d->ent.d_ino = 0;
    d->ent.d_type = (d->data.dwFileAttributes & 0x10) ? 4 : 8;
    return &d->ent;
}

static int closedir(DIR *d) {
    if (!d) { errno = EBADF; return -1; }
    if (d->h != INVALID_HANDLE_VALUE && d->h) FindClose(d->h);
    free(d);
    return 0;
}
#endif
#endif
