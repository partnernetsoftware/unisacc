/* Windows body for the POSIX calls the host cx driver uses.
   include/cx.h includes <spawn.h> on Windows. MinGW does not ship one, so the
   driver adds -I tools/cxcc-hostinc and this file is found.
   The shape follows include/spawn.h, include/sys/stat.h mkdir, and the
   mkstemp and realpath in include/stdlib.h:
   posix_spawnp quotes a command line and calls CreateProcessA (NULL
   application name, so PATH is searched and .exe is appended);
   waitpid waits on that process and stores (exit & 0xff) << 8;
   mkstemp replaces trailing XXXXXX and creates the file with open(O_EXCL);
   mkdir is CreateDirectoryA (the mode is not mapped);
   realpath opens the path, asks GetFinalPathNameByHandleA, and falls back
   to GetFullPathNameA. A null environment inherits.
   pid_t on this C library is narrower than a HANDLE, so the HANDLE lives in
   a small table and the pid is the slot. These are direct calls, not _ux_sym.
   The host compile uses -I tools/cxcc-hostinc so this file, not the product
   include/spawn.h, satisfies <spawn.h>. Linux and OSX do not include it.
   It has not been executed on Windows. */
#ifndef CXCC_HOST_SPAWN_H
#define CXCC_HOST_SPAWN_H
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 36
#endif

#define waitpid cxcc_waitpid
#define mkstemp cxcc_mkstemp
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

enum { CXCC_FA_MAX = 16 };
struct cxcc_fa_act { int op; int fd; int nfd; int oflag; int mode; char path[260]; };
struct cxcc_spawn_fa { int n; struct cxcc_fa_act a[CXCC_FA_MAX]; };
struct cxcc_spawn_at { int flags; };
typedef struct cxcc_spawn_fa posix_spawn_file_actions_t;
typedef struct cxcc_spawn_at posix_spawnattr_t;

static int posix_spawn_file_actions_init(posix_spawn_file_actions_t *fa) {
    if (!fa) return EINVAL;
    fa->n = 0;
    return 0;
}
static int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fa) {
    if (!fa) return EINVAL;
    fa->n = 0;
    return 0;
}
static int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fa, int fd, int nfd) {
    if (!fa || fd < 0 || nfd < 0) return EBADF;
    if (fa->n >= CXCC_FA_MAX) return ENOMEM;
    fa->a[fa->n].op = 1;
    fa->a[fa->n].fd = fd;
    fa->a[fa->n].nfd = nfd;
    fa->n++;
    return 0;
}
static int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fa, int fd) {
    if (!fa || fd < 0) return EBADF;
    if (fa->n >= CXCC_FA_MAX) return ENOMEM;
    fa->a[fa->n].op = 2;
    fa->a[fa->n].fd = fd;
    fa->n++;
    return 0;
}
static int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fa, int fd,
                                            const char *path, int oflag, int mode) {
    int i;
    if (!fa || fd < 0 || !path) return EBADF;
    if (fa->n >= CXCC_FA_MAX) return ENOMEM;
    for (i = 0; path[i]; i++) {
        if (i >= 259) return ENAMETOOLONG;
        fa->a[fa->n].path[i] = path[i];
    }
    fa->a[fa->n].path[i] = 0;
    fa->a[fa->n].op = 3;
    fa->a[fa->n].fd = fd;
    fa->a[fa->n].oflag = oflag;
    fa->a[fa->n].mode = mode;
    fa->n++;
    return 0;
}

enum { CXCC_HMAX = 64, CXCC_CMDLINE = 32768 };
static void *cxcc_hnd[CXCC_HMAX];

static int cxcc_hput(void *h, pid_t *pid) {
    int i;
    for (i = 1; i < CXCC_HMAX; i++) if (!cxcc_hnd[i]) {
        cxcc_hnd[i] = h;
        if (pid) *pid = (pid_t)i;
        return 0;
    }
    CloseHandle(h);
    return EAGAIN;
}

static int cxcc_cmdarg(char *d, int k, int max, const char *s) {
    int i, b, q;
    q = s[0] == 0;
    for (i = 0; s[i]; i++) if (s[i] == ' ' || s[i] == '\t' || s[i] == '"') q = 1;
    if (!q) {
        for (i = 0; s[i]; i++) { if (k >= max) return -1; d[k++] = s[i]; }
        return k;
    }
    if (k >= max) return -1;
    d[k++] = '"';
    for (i = 0; ; i++) {
        b = 0;
        while (s[i] == '\\') { b++; i++; }
        if (s[i] == 0) b *= 2;
        else if (s[i] == '"') b = b * 2 + 1;
        while (b > 0) { if (k >= max) return -1; d[k++] = '\\'; b--; }
        if (s[i] == 0) break;
        if (k >= max) return -1;
        d[k++] = s[i];
    }
    if (k >= max) return -1;
    d[k++] = '"';
    return k;
}

static int cxcc_win_errno(unsigned long e) {
    if (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND) return ENOENT;
    if (e == ERROR_ACCESS_DENIED) return EACCES;
    return EAGAIN;
}

static int posix_spawnp(pid_t *pid, const char *file, const posix_spawn_file_actions_t *fa,
                        const posix_spawnattr_t *at, char *const argv[], char *const envp[]) {
    static char cmd[CXCC_CMDLINE];
    static char env[CXCC_CMDLINE];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE stdh[3];
    int opened[CXCC_FA_MAX];
    int i, k, j, nopen, tgt;
    intptr_t osf;
    (void)file;
    (void)at;
    if (!argv || !argv[0]) return EINVAL;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);
    stdh[0] = GetStdHandle((unsigned long)-10);
    stdh[1] = GetStdHandle((unsigned long)-11);
    stdh[2] = GetStdHandle((unsigned long)-12);
    nopen = 0;
    if (fa) for (i = 0; i < fa->n; i++) {
        if (fa->a[i].op == 2) continue;
        tgt = fa->a[i].op == 1 ? fa->a[i].nfd : fa->a[i].fd;
        if (tgt < 0 || tgt > 2) {
            while (nopen) close(opened[--nopen]);
            return ENOSYS;
        }
        if (fa->a[i].op == 1) {
            if (fa->a[i].fd <= 2) stdh[tgt] = stdh[fa->a[i].fd];
            else {
                osf = _get_osfhandle(fa->a[i].fd);
                if (osf == (intptr_t)-1) {
                    while (nopen) close(opened[--nopen]);
                    return EBADF;
                }
                stdh[tgt] = (HANDLE)osf;
            }
        } else {
            k = open(fa->a[i].path, fa->a[i].oflag | O_BINARY, fa->a[i].mode);
            if (k < 0) {
                while (nopen) close(opened[--nopen]);
                return errno ? errno : EIO;
            }
            opened[nopen++] = k;
            osf = _get_osfhandle(k);
            if (osf == (intptr_t)-1) {
                while (nopen) close(opened[--nopen]);
                return EBADF;
            }
            SetHandleInformation((HANDLE)osf, 1, 1);
            stdh[tgt] = (HANDLE)osf;
        }
    }
    if (fa && fa->n) {
        si.dwFlags = 0x100;
        si.hStdInput = stdh[0];
        si.hStdOutput = stdh[1];
        si.hStdError = stdh[2];
    }
    k = 0;
    for (i = 0; argv[i]; i++) {
        if (i) {
            if (k >= CXCC_CMDLINE - 2) { while (nopen) close(opened[--nopen]); return E2BIG; }
            cmd[k++] = ' ';
        }
        k = cxcc_cmdarg(cmd, k, CXCC_CMDLINE - 2, argv[i]);
        if (k < 0) { while (nopen) close(opened[--nopen]); return E2BIG; }
    }
    cmd[k] = 0;
    if (envp) {
        k = 0;
        for (i = 0; envp[i]; i++) {
            for (j = 0; envp[i][j]; j++) {
                if (k >= CXCC_CMDLINE - 3) { while (nopen) close(opened[--nopen]); return E2BIG; }
                env[k++] = envp[i][j];
            }
            env[k++] = 0;
        }
        env[k++] = 0;
    }
    if (!CreateProcessA(0, cmd, 0, 0, 1, 0, envp ? env : 0, 0, &si, &pi)) {
        k = cxcc_win_errno(GetLastError());
        while (nopen) close(opened[--nopen]);
        return k;
    }
    while (nopen) close(opened[--nopen]);
    CloseHandle(pi.hThread);
    return cxcc_hput(pi.hProcess, pid);
}

static pid_t cxcc_waitpid(pid_t pid, int *st, int opt) {
    void *h;
    unsigned long code, w, ms;
    if (pid <= 0 || pid >= CXCC_HMAX || !cxcc_hnd[pid]) { errno = ECHILD; return -1; }
    h = cxcc_hnd[pid];
    ms = (opt & 1) ? 0 : 0xFFFFFFFFUL;
    w = WaitForSingleObject(h, ms);
    if (w == 0x102UL) return 0;
    if (w != 0) { errno = ECHILD; return -1; }
    code = 0;
    if (!GetExitCodeProcess(h, &code)) { errno = EIO; return -1; }
    CloseHandle(h);
    cxcc_hnd[pid] = 0;
    if (st) *st = (int)((code & 0xff) << 8);
    return pid;
}

static int cxcc_mkstemp(char *t) {
    static unsigned long seq;
    int n, j, tries, fd;
    unsigned long v;
    n = 0;
    while (t[n]) n++;
    if (n < 6) { errno = EINVAL; return -1; }
    for (j = n - 6; j < n; j++) if (t[j] != 'X') { errno = EINVAL; return -1; }
    for (tries = 0; tries < 100; tries++) {
        seq++;
        v = (unsigned long)((uintptr_t)&tries >> 4) * 2654435761UL + seq * 40503UL;
        for (j = n - 6; j < n; j++) { t[j] = (char)('a' + (int)(v % 26)); v = v / 26 + seq * 7; }
        fd = open(t, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);
        if (fd >= 0) return fd;
        if (errno != EEXIST) return -1;
    }
    return -1;
}

/* Directory for mkstemp. A Windows TMPDIR (drive or UNC), else GetTempPathA. */
static int cxcc_temp_template(char *dst, size_t cap) {
    const char *td;
    unsigned long n;
    size_t i;
    const char *suf = "cxccXXXXXX";
    td = getenv("TMPDIR");
    if (td && td[0] && (strchr(td, ':') || td[0] == '\\' || (td[0] == '/' && td[1] == '/'))) {
        if (strlen(td) + 16 >= cap) return -1;
        i = 0;
        while (td[i]) { dst[i] = td[i]; i++; }
        dst[i] = 0;
        if (i && dst[i - 1] != '\\' && dst[i - 1] != '/') dst[i++] = '\\';
        dst[i] = 0;
    } else {
        if (cap < 16) return -1;
        n = GetTempPathA((unsigned long)(cap - 16), dst);
        if (n == 0 || n >= cap - 16) return -1;
        i = strlen(dst);
    }
    if (i + 11 >= cap) return -1;
    memcpy(dst + i, suf, 11);
    return 0;
}

/* POSIX mkdir. MinGW's own mkdir takes one argument. The mode is ignored,
   matching include/sys/stat.h on Windows. */
static int cxcc_mkdir(const char *p, int mode) {
    (void)mode;
    if (!p) { errno = EINVAL; return -1; }
    if (CreateDirectoryA(p, 0)) return 0;
    if (GetLastError() == ERROR_ALREADY_EXISTS) errno = EEXIST;
    else errno = EACCES;
    return -1;
}

/* POSIX realpath. The buffer is PATH_MAX bytes when the caller passes one.
   Directories open because FILE_FLAG_BACKUP_SEMANTICS is set. */
static char *cxcc_realpath(const char *p, char *out) {
    char tmp[PATH_MAX];
    unsigned long n, a, off, k;
    HANDLE h;
    if (!p) { errno = EINVAL; return 0; }
    n = 0;
    h = CreateFileA(p, 0x80000000u, 7, 0, 3, 0x02000000u, 0);
    if (h != INVALID_HANDLE_VALUE) {
        n = GetFinalPathNameByHandleA(h, tmp, PATH_MAX, 0);
        CloseHandle(h);
        if (n == 0 || n >= PATH_MAX) n = 0;
    }
    if (n >= 4 && tmp[0] == '\\' && tmp[1] == '\\' && tmp[2] == '?' && tmp[3] == '\\') {
        off = 4;
        k = 0;
        if (n >= 8 && (tmp[4] == 'U' || tmp[4] == 'u') && (tmp[5] == 'N' || tmp[5] == 'n') &&
            (tmp[6] == 'C' || tmp[6] == 'c') && tmp[7] == '\\') {
            tmp[0] = '\\';
            tmp[1] = '\\';
            off = 8;
            k = 2;
        }
        a = off;
        while (a <= n) { tmp[k++] = tmp[a++]; }
        n = k - 1;
    }
    if (n == 0) {
        if (GetFileAttributesA(p) == 0xFFFFFFFFu) { errno = ENOENT; return 0; }
        n = GetFullPathNameA(p, PATH_MAX, tmp, 0);
        if (n == 0 || n >= PATH_MAX) { errno = ENAMETOOLONG; return 0; }
    }
    if (!out) {
        out = (char *)malloc((size_t)n + 1);
        if (!out) { errno = ENOMEM; return 0; }
    }
    memcpy(out, tmp, (size_t)n + 1);
    return out;
}

static int cxcc_kill(pid_t pid, int sig) {
    if (pid <= 0 || pid >= CXCC_HMAX || sig < 0) { errno = EINVAL; return -1; }
    if (!cxcc_hnd[pid]) { errno = ESRCH; return -1; }
    if (sig == 0) return 0;
    if (!TerminateProcess(cxcc_hnd[pid], (unsigned)(128 + (sig & 0x7f)))) {
        errno = EPERM;
        return -1;
    }
    return 0;
}

static int cxcc_usleep(unsigned us) {
    Sleep(us / 1000u + (us % 1000u ? 1u : 0u));
    return 0;
}

#define mkdir(p, m) cxcc_mkdir((p), (m))
#define realpath(p, o) cxcc_realpath((p), (o))
#define kill(p, s) cxcc_kill((p), (s))
#define usleep(u) cxcc_usleep((unsigned)(u))
#endif
#endif
