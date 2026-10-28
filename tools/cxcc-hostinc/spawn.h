/* Windows body for the POSIX calls the host cx driver uses.
   include/cx.h includes <spawn.h> on Windows. MinGW does not ship one, so the
   driver adds -idirafter tools/cxcc-hostinc and this file is found.
   The shape follows include/spawn.h and the mkstemp in include/stdlib.h:
   posix_spawnp quotes a command line and calls CreateProcessA (NULL
   application name, so PATH is searched and .exe is appended);
   waitpid waits on that process and stores (exit & 0xff) << 8;
   mkstemp replaces trailing XXXXXX and creates the file with open(O_EXCL).
   pid_t on this C library is narrower than a HANDLE, so the HANDLE lives in
   a small table and the pid is the slot. These are direct calls, not _ux_sym.
   Linux and OSX do not include this file. It has not been executed on Windows. */
#ifndef CXCC_HOST_SPAWN_H
#define CXCC_HOST_SPAWN_H
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <windows.h>

#define waitpid cxcc_waitpid
#define mkstemp cxcc_mkstemp

struct cxcc_spawn_fa { int n; };
struct cxcc_spawn_at { int flags; };
typedef struct cxcc_spawn_fa posix_spawn_file_actions_t;
typedef struct cxcc_spawn_at posix_spawnattr_t;

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
    int i, k, j;
    (void)file;
    (void)at;
    if (fa && fa->n) return ENOSYS;
    if (!argv || !argv[0]) return EINVAL;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);
    k = 0;
    for (i = 0; argv[i]; i++) {
        if (i) { if (k >= CXCC_CMDLINE - 2) return E2BIG; cmd[k++] = ' '; }
        k = cxcc_cmdarg(cmd, k, CXCC_CMDLINE - 2, argv[i]);
        if (k < 0) return E2BIG;
    }
    cmd[k] = 0;
    if (envp) {
        k = 0;
        for (i = 0; envp[i]; i++) {
            for (j = 0; envp[i][j]; j++) {
                if (k >= CXCC_CMDLINE - 3) return E2BIG;
                env[k++] = envp[i][j];
            }
            env[k++] = 0;
        }
        env[k++] = 0;
    }
    if (!CreateProcessA(0, cmd, 0, 0, 1, 0, envp ? env : 0, 0, &si, &pi))
        return cxcc_win_errno(GetLastError());
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
        v = ((unsigned long)&tries >> 4) * 2654435761UL + seq * 40503UL;
        for (j = n - 6; j < n; j++) { t[j] = (char)('a' + (int)(v % 26)); v = v / 26 + seq * 7; }
        fd = open(t, O_RDWR | O_CREAT | O_EXCL, 0600);
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
#endif
#endif
