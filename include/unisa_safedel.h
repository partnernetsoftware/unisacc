#ifndef UNISA_SAFEDEL_H
#define UNISA_SAFEDEL_H
/* unisa_safedel.h: the one deletion guard (commissar ruling 2026-10-23, after the 05:22 wipe of the checkout: an empty
   `--tools=` became Path('') == '.' and shutil.rmtree emptied the repository, .git included).  Every deletion in the
   repository goes through unisa_safedel(): in .cx through cx_remove/cx_rmtree/cx_trash (include/cx.h), from sh, make,
   CI and Python through the argv-only CLI tests/safedel.cx.  C99 over the carried libc; POSIX only (on Windows every
   call is refused: fail closed).  Every body sits alone in its own -ftrim-libc guard (tests/libneed.sh).

   int unisa_safedel(const char *path, unsigned flags, long max_files, char *why, int whylen)
     flags SAFEDEL_TRASH (0, the default) | SAFEDEL_PURGE | SAFEDEL_DRYRUN | SAFEDEL_RECURSIVE;
     SAFEDEL_OK 0, SAFEDEL_FAILED 1 (an I/O error after every check passed), SAFEDEL_REFUSED 3; WHY gets one line.
   The checks, in this order, on the resolved target (realpath; a symlink resolves as realpath(its parent)/its name
   and is removed as a link, never followed; a trailing slash on a symlink means its directory):
     1. refusal list: NULL, "", a leading "~", "/" (or only slashes), a last component "." or "..", $HOME or an
        ancestor of it, the cwd or an ancestor of it, the cwd's repository root or an ancestor of it, a path inside a
        .git directory, a repository root (a directory with a .git entry); a top-level symlink whose destination is
        refused by checks 1-2 (a link into a repository, to the cwd, ...);
     2. whitelist: inside a repository (nearest ancestor with a .git entry) the target must lie strictly under its
        out/ or .trash/; elsewhere strictly under a temp root: realpath($TMPDIR) and realpath(/tmp), both whitelisted
        (a relative TMPDIR, /, $HOME or an ancestor of $HOME is no temp root; with neither root everything outside
        repositories is refused); a temp root itself or an ancestor of one is refused;
     3. the walk (lstat only, links never followed) counts every entry, the target included, and refuses on an entry
        named .git, on a mount point, deeper than SAFEDEL_DEPTH; a non-empty directory needs SAFEDEL_RECURSIVE;
     4. more entries than max_files (<= 0: SAFEDEL_MAX_FILES) is refused.
   SAFEDEL_DRYRUN stops there.  Trash (the default): rename into HOME/.trash/<UTC stamp>-<pid>-<n>/<name>, HOME =
   the target's repository or its temp root (the deeper one when both hold it); .trash must be a real directory owned by the caller, closed to others.
   Purge: unlink and rmdir depth first, each directory re-checked (device and inode) when it is opened.  A missing
   target whose path passes the checks is success (rm -f).
   int unisa_safedel_purge_trash(const char *home, long hours, long max_files, unsigned flags, char *why, int whylen)
     purges the entries of HOME/.trash (HOME = a temp root or a repository root) older than HOURS, the ceiling over
     their summed entries; SAFEDEL_DRYRUN only counts.  The clock is the file system's (the mtime of a directory
     made for the purpose), so no <time.h> statics land in every .cx program. */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(_GNU_SOURCE)
#define _DEFAULT_SOURCE 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif
#ifdef __UNISA__
#include <sys/_word.h>
#elif !defined(__u_word) && defined(_WIN64)   /* a host compiler over its own libc (seed/compilerpack.c): no carried sys/_word.h */
#define __u_word long long
#define __u_uword unsigned long long
#elif !defined(__u_word)
#define __u_word long
#define __u_uword unsigned long
#endif

#define SAFEDEL_TRASH 0u
#define SAFEDEL_PURGE 1u
#define SAFEDEL_DRYRUN 2u
#define SAFEDEL_RECURSIVE 4u
#define SAFEDEL_OK 0
#define SAFEDEL_FAILED 1
#define SAFEDEL_REFUSED 3
#define SAFEDEL_MAX_FILES 1000L
#define SAFEDEL_DEPTH 128
#define SAFEDEL_PATHLEN 4096

struct unisa_safedel_env { char home[SAFEDEL_PATHLEN], cwd[SAFEDEL_PATHLEN], cwdrepo[SAFEDEL_PATHLEN], tmp[2][SAFEDEL_PATHLEN]; const char *tmpwhy[2]; };

static int unisa_safedel(const char *path, unsigned flags, __u_word max_files, char *why, int whylen);
static int unisa_safedel_purge_trash(const char *home, __u_word hours, __u_word max_files, unsigned flags, char *why, int whylen);

/* RC, with "PATH: A B" in WHY */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_say
static int _unisa_sd_say(char *why, int whylen, int rc, const char *path, const char *a, const char *b) {
    if (why && whylen > 0) snprintf(why, (size_t)whylen, "%s: %s%s", path ? path : "(null)", a ? a : "", b ? b : "");
    return rc;
}
#endif
/* 1 when P lies strictly below ROOT (both absolute and resolved) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_below
static int _unisa_sd_below(const char *p, const char *root) {
    size_t n = strlen(root);
    if (!n || p[0] != '/') return 0;
    if (n == 1) return root[0] == '/' && p[1] != 0;
    return strncmp(p, root, n) == 0 && p[n] == '/' && p[n + 1] != 0;
}
#endif
/* 1 when P is ROOT or lies below it */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_within
static int _unisa_sd_within(const char *p, const char *root) { return root[0] && (strcmp(p, root) == 0 || _unisa_sd_below(p, root)); }
#endif
/* OUT = A/B (SAFEDEL_PATHLEN bytes); -1 when too long */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_join
static int _unisa_sd_join(char *out, const char *a, const char *b) {
    size_t n = strlen(a);
    int k = snprintf(out, SAFEDEL_PATHLEN, "%s%s%s", a, (n && a[n - 1] == '/') ? "" : "/", b);
    return k > 0 && k < SAFEDEL_PATHLEN ? 0 : -1;
}
#endif
/* realpath of P into OUT (SAFEDEL_PATHLEN bytes); 0, or -1 with errno */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_real
static int _unisa_sd_real(const char *p, char *out) {
    char *r = realpath(p, 0); size_t n;
    if (!r) return -1;
    n = strlen(r);
    if (n >= SAFEDEL_PATHLEN || r[0] != '/') { free(r); errno = ENAMETOOLONG; return -1; }
    memcpy(out, r, n + 1); free(r); return 0;
}
#endif
/* the nearest of P (absolute) and its ancestors with a .git entry, into OUT: 1, or 0 when there is none */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_repo
static int _unisa_sd_repo(const char *p, char *out) {
#ifdef _WIN32
    (void)p; (void)out; return 0;
#else
    char d[SAFEDEL_PATHLEN], g[SAFEDEL_PATHLEN]; struct stat st; size_t n = strlen(p); char *s;
    if (n >= SAFEDEL_PATHLEN || p[0] != '/') return 0;
    memcpy(d, p, n + 1);
    for (;;) {
        if (_unisa_sd_join(g, d, ".git") == 0 && lstat(g, &st) == 0) { memcpy(out, d, strlen(d) + 1); return 1; }
        if (d[1] == 0) return 0;
        s = strrchr(d, '/');
        if (s == d) d[1] = 0; else *s = 0;
    }
#endif
}
#endif
/* 1 when a component of P is .git */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_ingit
static int _unisa_sd_ingit(const char *p) {
    const char *s = p;
    if (!strcmp(p, ".git") || !strncmp(p, ".git/", 5)) return 1;
    while ((s = strstr(s, "/.git")) != 0) { if (s[5] == 0 || s[5] == '/') return 1; s += 5; }
    return 0;
}
#endif
/* a fresh number per call in this process (the trash and clock directory names) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_seq
static __u_uword _unisa_sd_seq(void) { static __u_uword n; return n++; }
#endif
/* $HOME, the cwd, the cwd's repository and the temp root, resolved */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_env
static void _unisa_sd_env(struct unisa_safedel_env *e) {
#ifdef _WIN32
    e->home[0] = e->cwd[0] = e->cwdrepo[0] = e->tmp[0][0] = e->tmp[1][0] = 0; e->tmpwhy[0] = e->tmpwhy[1] = "Windows";
#else
    const char *h = getenv("HOME"), *t = getenv("TMPDIR"); char c[SAFEDEL_PATHLEN]; int i;
    e->home[0] = e->cwd[0] = e->cwdrepo[0] = e->tmp[0][0] = e->tmp[1][0] = 0; e->tmpwhy[0] = e->tmpwhy[1] = 0;
    if (h && h[0] && _unisa_sd_real(h, e->home) != 0) e->home[0] = 0;
    if (!getcwd(c, sizeof c) || _unisa_sd_real(c, e->cwd) != 0) e->cwd[0] = 0;
    if (e->cwd[0] && !_unisa_sd_repo(e->cwd, e->cwdrepo)) e->cwdrepo[0] = 0;
    if (!t || !t[0]) t = "/tmp";
    for (i = 0; i < 2; i++) {   /* root 0: $TMPDIR (/tmp when unset or empty); root 1: /tmp */
        const char *r = i ? "/tmp" : t;
        if (r[0] != '/') e->tmpwhy[i] = "TMPDIR is not absolute";
        else if (_unisa_sd_real(r, e->tmp[i]) != 0) e->tmpwhy[i] = "the temp root does not resolve";
        else if (!strcmp(e->tmp[i], "/")) e->tmpwhy[i] = "the temp root is /";
        else if (e->home[0] && _unisa_sd_within(e->home, e->tmp[i])) e->tmpwhy[i] = "the temp root is $HOME or an ancestor of it";
        if (e->tmpwhy[i]) e->tmp[i][0] = 0;
    }
    if (e->tmp[1][0] && !strcmp(e->tmp[0], e->tmp[1])) e->tmp[1][0] = 0;   /* one root, once */
#endif
}
#endif
/* checks 1-2 on the resolved T; HOME gets the trash home (the repository or the temp root).  DEST: T is where a
   top-level symlink points, and only checks 1 and the repository rule apply (removing a link to /usr/bin/cc is fine) */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_policy
static int _unisa_sd_policy(const struct unisa_safedel_env *e, const char *path, const char *t, int dest, char *home, char *why, int whylen) {
    char r[SAFEDEL_PATHLEN], o[SAFEDEL_PATHLEN], x[SAFEDEL_PATHLEN];
    if (t[0] != '/' || t[1] == 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "resolves to /", "");
    if (e->home[0] && _unisa_sd_within(e->home, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is $HOME or an ancestor of it: ", t);
    if (_unisa_sd_within(e->cwd, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the cwd or an ancestor of it: ", t);
    if (e->cwdrepo[0] && _unisa_sd_within(e->cwdrepo, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the repository root or an ancestor of it: ", t);
    if (_unisa_sd_ingit(t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "lies inside a .git directory: ", t);
    if (_unisa_sd_repo(t, r)) {
        if (!strcmp(r, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a repository root (it has a .git entry): ", t);
        if (_unisa_sd_join(o, r, "out") || _unisa_sd_join(x, r, ".trash") || (!_unisa_sd_below(t, o) && !_unisa_sd_below(t, x)))
            return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "lies in a repository outside its out/ and .trash/: ", t);
        memcpy(home, r, strlen(r) + 1);
        return SAFEDEL_OK;
    }
    if (dest) return SAFEDEL_OK;
    if (!e->tmp[0][0] && !e->tmp[1][0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is outside every repository and there is no temp root: ", e->tmpwhy[0]);
    {   /* the deeper of the temp roots that holds T (TMPDIR=/tmp/x: /tmp/x/y trashes into /tmp/x/.trash); a temp root
           itself, or an ancestor of one ($TMPDIR=/tmp/x from /tmp's view), is never a target */
        int i, k = -1;
        for (i = 0; i < 2; i++) if (e->tmp[i][0] && _unisa_sd_within(e->tmp[i], t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a temp root or an ancestor of one: ", t);
        for (i = 0; i < 2; i++) if (e->tmp[i][0] && _unisa_sd_below(t, e->tmp[i]) && (k < 0 || strlen(e->tmp[i]) > strlen(e->tmp[k]))) k = i;
        if (k < 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is not strictly under a whitelisted root ($TMPDIR, /tmp, <repo>/out, <repo>/.trash): ", t);
        memcpy(home, e->tmp[k], strlen(e->tmp[k]) + 1);
    }
    return SAFEDEL_OK;
}
#endif
/* the lexical refusals and the resolution of PATH into T; then the policy.  *ABSENT: nothing is there */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_check
static int _unisa_sd_check(const struct unisa_safedel_env *e, const char *path, char *t, char *home, int *islink, int *absent, char *why, int whylen) {
#ifdef _WIN32
    (void)e; (void)t; (void)home; (void)islink; (void)absent;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "deletion is not supported on Windows", "");
#else
    char p[SAFEDEL_PATHLEN], dir[SAFEDEL_PATHLEN], rd[SAFEDEL_PATHLEN], dest[SAFEDEL_PATHLEN], dh[SAFEDEL_PATHLEN], dw[512];
    const char *base; char *s; size_t n; int slash = 0, rc; struct stat st;
    *islink = 0; *absent = 0; home[0] = 0;
    if (!path || !path[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "\"\"", "empty path", "");
    if (path[0] == '~') return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "starts with ~ (an unexpanded home)", "");
    n = strlen(path);
    if (n >= SAFEDEL_PATHLEN) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "(long path)", "path too long", "");
    memcpy(p, path, n + 1);
    while (n > 1 && p[n - 1] == '/') { p[--n] = 0; slash = 1; }
    if (!strcmp(p, "/")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is /", "");
    s = strrchr(p, '/'); base = s ? s + 1 : p;
    if (!strcmp(base, ".") || !strcmp(base, "..")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "ends in . or ..", "");
    if (!e->cwd[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "the cwd cannot be determined", "");
    if (!s) { dir[0] = '.'; dir[1] = 0; }
    else if (s == p) { dir[0] = '/'; dir[1] = 0; }
    else { memcpy(dir, p, (size_t)(s - p)); dir[s - p] = 0; }
    if (lstat(p, &st) != 0) {
        if (errno != ENOENT && errno != ENOTDIR) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat: ", strerror(errno));
        *absent = 1;
        if (_unisa_sd_real(dir, rd) != 0) { memcpy(t, p, n + 1); return SAFEDEL_OK; }   /* not even the parent exists */
        if (_unisa_sd_join(t, rd, base)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", "");
    } else if (S_ISLNK(st.st_mode) && !slash) {
        *islink = 1;
        if (_unisa_sd_real(dir, rd) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot resolve the parent: ", strerror(errno));
        if (_unisa_sd_join(t, rd, base)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", "");
        if (_unisa_sd_real(p, dest) == 0 && (rc = _unisa_sd_policy(e, path, dest, 1, dh, dw, (int)sizeof dw)) != 0)
            return _unisa_sd_say(why, whylen, rc, path, "is a symlink to a protected path; ", dw);
    } else if (_unisa_sd_real(p, t) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot resolve: ", strerror(errno));
    return _unisa_sd_policy(e, path, t, 0, home, why, whylen);
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_free
static void _unisa_sd_free(char **v, size_t n) { size_t i; for (i = 0; i < n; i++) free(v[i]); free(v); }
#endif
/* the names in directory P, which must still be the directory ST describes (device and inode); 0, or -1 */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_list
static int _unisa_sd_list(const char *p, const struct stat *st, char ***out, size_t *outn) {
#ifdef _WIN32
    (void)p; (void)st; *out = 0; *outn = 0; return -1;
#else
    DIR *d = opendir(p); struct dirent *x; struct stat ds; char **v = 0, **g; size_t n = 0, cap = 0, l; int bad = 0;
    *out = 0; *outn = 0;
    if (!d) return -1;
    if (fstat(dirfd(d), &ds) != 0 || ds.st_dev != st->st_dev || ds.st_ino != st->st_ino) { closedir(d); return -1; }
    while (!bad && (x = readdir(d)) != 0) {
        if (!strcmp(x->d_name, ".") || !strcmp(x->d_name, "..")) continue;
        if (n == cap) {
            cap = cap ? 2 * cap : 16;
            g = (char **)realloc(v, cap * sizeof *v);
            if (!g) { bad = 1; break; }
            v = g;
        }
        l = strlen(x->d_name) + 1;
        if (!(v[n] = (char *)malloc(l))) { bad = 1; break; }
        memcpy(v[n++], x->d_name, l);
    }
    closedir(d);
    if (bad) { _unisa_sd_free(v, n); return -1; }
    *out = v; *outn = n; return 0;
#endif
}
#endif
/* check 3: count P (described by ST) and what lies below it into *CNT, stopping once it passes CAP; refuse a .git
   entry, another device than ROOT's, a depth past SAFEDEL_DEPTH */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_walk
static int _unisa_sd_walk(const char *p, const struct stat *st, const struct stat *root, int depth, __u_word *cnt, __u_word cap, char *why, int whylen) {
#ifdef _WIN32
    (void)p; (void)st; (void)root; (void)depth; (void)cnt; (void)cap;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    char **v; size_t n, i; char c[SAFEDEL_PATHLEN]; struct stat cs; int rc = 0;
    *cnt += 1;
    if (*cnt > cap || !S_ISDIR(st->st_mode)) return SAFEDEL_OK;
    if (depth >= SAFEDEL_DEPTH) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is nested deeper than SAFEDEL_DEPTH", "");
    if (_unisa_sd_list(p, st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot read the directory (or it changed)", "");
    for (i = 0; i < n && !rc && *cnt <= cap; i++) {
        if (!strcmp(v[i], ".git")) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "contains a .git entry", "");
        else if (_unisa_sd_join(c, p, v[i])) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "an entry's path is too long", "");
        else if (lstat(c, &cs) != 0) rc = _unisa_sd_say(why, whylen, SAFEDEL_FAILED, c, "cannot lstat: ", strerror(errno));
        else if (cs.st_dev != root->st_dev) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, c, "is another file system (a mount point)", "");
        else rc = _unisa_sd_walk(c, &cs, root, depth + 1, cnt, cap, why, whylen);
    }
    _unisa_sd_free(v, n);
    return rc;
#endif
}
#endif
/* permanent removal of P, depth first; each directory is re-checked when it is opened */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_purge
static int _unisa_sd_purge(const char *p, const struct stat *root, int depth, char *why, int whylen) {
#ifdef _WIN32
    (void)p; (void)root; (void)depth;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    char **v; size_t n, i; char c[SAFEDEL_PATHLEN]; struct stat st; int rc = 0;
    if (lstat(p, &st) != 0) return errno == ENOENT ? SAFEDEL_OK : _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot lstat: ", strerror(errno));
    if (st.st_dev != root->st_dev) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is another file system (a mount point)", "");
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(p) != 0 && errno != ENOENT) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot unlink: ", strerror(errno));
        return SAFEDEL_OK;
    }
    if (depth >= SAFEDEL_DEPTH) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "is nested deeper than SAFEDEL_DEPTH", "");
    if (_unisa_sd_list(p, &st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot read the directory (or it changed)", "");
    for (i = 0; i < n && !rc; i++) {
        if (!strcmp(v[i], ".git")) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "contains a .git entry", "");
        else if (_unisa_sd_join(c, p, v[i])) rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, p, "an entry's path is too long", "");
        else rc = _unisa_sd_purge(c, root, depth + 1, why, whylen);
    }
    _unisa_sd_free(v, n);
    if (!rc && rmdir(p) != 0) rc = _unisa_sd_say(why, whylen, SAFEDEL_FAILED, p, "cannot rmdir: ", strerror(errno));
    return rc;
#endif
}
#endif
/* the file system's clock: the mtime of a directory made (and removed) in DIR; -1 on error */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_now
static __u_word _unisa_sd_now(const char *dir) {
#ifdef _WIN32
    (void)dir; return -1;
#else
    char p[SAFEDEL_PATHLEN]; struct stat st; long t = -1; int k;
    k = snprintf(p, sizeof p, "%s/.now-%ld-%lu", dir, (long)getpid(), _unisa_sd_seq());
    if (k <= 0 || k >= (int)sizeof p || mkdir(p, 0700) != 0) return -1;
    if (lstat(p, &st) == 0) t = (long)st.st_mtime;
    rmdir(p);
    return t;
#endif
}
#endif
/* T (seconds since 1970, UTC) as YYYYMMDDTHHMMSSZ */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_stamp
static void _unisa_sd_stamp(__u_word t, char out[20]) {
    __u_word z = t / 86400, s = t % 86400, era, doe, yoe, y, doy, mp, d, m;
    if (s < 0) { s += 86400; z -= 1; }
    z += 719468; era = (z >= 0 ? z : z - 146096) / 146097; doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100); mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9; if (m <= 2) y += 1;
#if defined(_WIN64)
    snprintf(out, 20, "%04lld%02lld%02lldT%02lld%02lld%02lldZ", y, m, d, s / 3600, s / 60 % 60, s % 60);
#else
    snprintf(out, 20, "%04ld%02ld%02ldT%02ld%02ld%02ldZ", y, m, d, s / 3600, s / 60 % 60, s % 60);
#endif
}
#endif
/* the time a trash entry name starts with (YYYYMMDDTHHMMSSZ), or -1 */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_unstamp
static __u_word _unisa_sd_unstamp(const char *nm) {
    __u_word v[6], y, m, era, yoe, doy, doe; int i, j, k = 0, w;
    for (i = 0; i < 6; i++) {
        if (i == 3 && nm[k++] != 'T') return -1;
        w = i == 0 ? 4 : 2; v[i] = 0;
        for (j = 0; j < w; j++, k++) { if (nm[k] < '0' || nm[k] > '9') return -1; v[i] = v[i] * 10 + (nm[k] - '0'); }
    }
    if (nm[k] != 'Z' || v[1] < 1 || v[1] > 12 || v[2] < 1 || v[2] > 31 || v[3] > 23 || v[4] > 59 || v[5] > 60) return -1;
    y = v[0]; m = v[1]; if (m <= 2) y -= 1;
    era = (y >= 0 ? y : y - 399) / 400; yoe = y - era * 400;
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + v[2] - 1; doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (era * 146097 + doe - 719468) * 86400 + v[3] * 3600 + v[4] * 60 + v[5];
}
#endif
/* HOME/.trash: made 0700 when missing; it must be a real directory of this user, closed to others.  0, or a refusal */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_trashdir
static int _unisa_sd_trashdir(const char *home, char *tr, int make, char *why, int whylen) {
#ifdef _WIN32
    (void)home; (void)tr; (void)make;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "", "Windows", "");
#else
    struct stat st;
    if (_unisa_sd_join(tr, home, ".trash")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "path too long", "");
    if (make && mkdir(tr, 0700) != 0 && errno != EEXIST) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot make: ", strerror(errno));
    if (lstat(tr, &st) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot lstat: ", strerror(errno));
    if (!S_ISDIR(st.st_mode)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "is not a directory (a symlink?)", "");
    if (st.st_uid != geteuid() || (st.st_mode & 022)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "is not this user's or is writable by others", "");
    return SAFEDEL_OK;
#endif
}
#endif
/* unisa_safedel, with the counted entries in *COUNT */
#if !__UNISA_FTRIM_LIBC || __UN__unisa_sd_run
static int _unisa_sd_run(const char *path, unsigned flags, __u_word max_files, __u_word *count, char *why, int whylen) {
#ifdef _WIN32
    (void)flags; (void)max_files; *count = 0;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "deletion is not supported on Windows", "");
#else
    struct unisa_safedel_env e; char t[SAFEDEL_PATHLEN], home[SAFEDEL_PATHLEN], tr[SAFEDEL_PATHLEN], sd[SAFEDEL_PATHLEN], dest[SAFEDEL_PATHLEN], stamp[20];
    int islink, absent, rc, k, er; struct stat st; long cnt = 0, now; unsigned long i;
    *count = 0;
    if (max_files <= 0) max_files = SAFEDEL_MAX_FILES;
    _unisa_sd_env(&e);
    rc = _unisa_sd_check(&e, path, t, home, &islink, &absent, why, whylen);
    if (rc) return rc;
    if (absent) return _unisa_sd_say(why, whylen, SAFEDEL_OK, path, "absent (nothing to delete)", "");
    if (lstat(t, &st) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat: ", strerror(errno));
    {   /* a mount point (a bind mount of $HOME under /tmp, say) is another file system than its parent: refused */
        struct stat ps; size_t l = (size_t)(strrchr(t, '/') - t);
        memcpy(sd, t, l ? l : 1); sd[l ? l : 1] = 0;
        if (lstat(sd, &ps) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot lstat the parent: ", strerror(errno));
        if (ps.st_dev != st.st_dev) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a mount point", "");
    }
    if (islink || !S_ISDIR(st.st_mode)) cnt = 1;
    else {
        rc = _unisa_sd_walk(t, &st, &st, 0, &cnt, max_files, why, whylen);
        if (rc) return rc;
        if (cnt > 1 && !(flags & SAFEDEL_RECURSIVE)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is a non-empty directory (not recursive)", "");
    }
    if (cnt > max_files) {
        char m[96]; snprintf(m, sizeof m, "more than %ld entries, the ceiling (pass an explicit force count)", max_files);
        return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, m, "");
    }
    *count = cnt;
    if (flags & SAFEDEL_DRYRUN) {
        char m[96]; snprintf(m, sizeof m, "would %s %ld entries: ", (flags & SAFEDEL_PURGE) ? "purge" : "trash", cnt);
        return _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, t);
    }
    if (flags & SAFEDEL_PURGE) {
        rc = _unisa_sd_purge(t, &st, 0, why, whylen);
        if (!rc) { char m[64]; snprintf(m, sizeof m, "purged %ld entries: ", cnt); _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, t); }
        return rc;
    }
    rc = _unisa_sd_trashdir(home, tr, 1, why, whylen);
    if (rc) return rc;
    if (_unisa_sd_within(tr, t)) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "is the trash or holds it (purge it instead)", "");
    now = _unisa_sd_now(tr);
    if (now < 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the clock: ", strerror(errno));
    _unisa_sd_stamp(now, stamp);
    for (i = 0;; i++) {
        k = snprintf(sd, sizeof sd, "%s/%s-%ld-%lu", tr, stamp, (long)getpid(), _unisa_sd_seq());
        if (k <= 0 || k >= (int)sizeof sd) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, "path too long", "");
        if (mkdir(sd, 0700) == 0) break;
        if (errno != EEXIST || i >= 1000) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, sd, "cannot make: ", strerror(errno));
    }
    if (_unisa_sd_join(dest, sd, strrchr(t, '/') + 1)) { rmdir(sd); return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, path, "path too long", ""); }
    if (rename(t, dest) != 0) {
        er = errno; rmdir(sd);
        if (er == EXDEV) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "lies on another file system than its trash; not moved (purge it instead)", "");
        return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, path, "cannot move into the trash: ", strerror(er));
    }
    { char m[64]; snprintf(m, sizeof m, "trashed %ld entries to ", cnt); _unisa_sd_say(why, whylen, SAFEDEL_OK, path, m, dest); }
    return SAFEDEL_OK;
#endif
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_safedel
static int unisa_safedel(const char *path, unsigned flags, __u_word max_files, char *why, int whylen) {
    __u_word n; return _unisa_sd_run(path, flags, max_files, &n, why, whylen);
}
#endif
#if !__UNISA_FTRIM_LIBC || __UN_unisa_safedel_purge_trash
static int unisa_safedel_purge_trash(const char *home, __u_word hours, __u_word max_files, unsigned flags, char *why, int whylen) {
#ifdef _WIN32
    (void)hours; (void)max_files; (void)flags;
    return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "deletion is not supported on Windows", "");
#else
    struct unisa_safedel_env e; char r[SAFEDEL_PATHLEN], tr[SAFEDEL_PATHLEN], c[SAFEDEL_PATHLEN], m[128];
    struct stat st; char **v; char *pick; size_t n, i; long now, t0, total = 0, k, entries = 0; int rc = 0;
    if (!home || !home[0]) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, "\"\"", "empty trash home", "");
    if (hours < 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "a negative age", "");
    if (max_files <= 0) max_files = SAFEDEL_MAX_FILES;
    _unisa_sd_env(&e);
    if (_unisa_sd_real(home, r) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "does not resolve", "");
    if (!(e.tmp[0][0] && !strcmp(r, e.tmp[0])) && !(e.tmp[1][0] && !strcmp(r, e.tmp[1]))) {
        if (_unisa_sd_join(c, r, ".git") || lstat(c, &st) != 0)
            return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "is neither a temp root nor a repository root: ", r);
    }
    if (_unisa_sd_join(tr, r, ".trash")) return _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, home, "path too long", "");
    if (lstat(tr, &st) != 0) {
        if (errno == ENOENT) return _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, "no trash", "");
        return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot lstat: ", strerror(errno));
    }
    rc = _unisa_sd_trashdir(r, tr, 0, why, whylen);
    if (rc) return rc;
    if (lstat(tr, &st) != 0 || (now = _unisa_sd_now(tr)) < 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the clock", "");
    if (_unisa_sd_list(tr, &st, &v, &n) != 0) return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "cannot read the directory", "");
    pick = (char *)calloc(n ? n : 1, 1);
    if (!pick) { _unisa_sd_free(v, n); return _unisa_sd_say(why, whylen, SAFEDEL_FAILED, tr, "out of memory", ""); }
    for (i = 0; i < n && !rc; i++) {
        struct stat es;
        if (!strncmp(v[i], ".now-", 5) || _unisa_sd_join(c, tr, v[i]) || lstat(c, &es) != 0) continue;
        t0 = _unisa_sd_unstamp(v[i]);
        if (t0 < 0) t0 = (long)es.st_mtime;
        if (now - t0 < hours * 3600) continue;
        rc = _unisa_sd_run(c, SAFEDEL_PURGE | SAFEDEL_RECURSIVE | SAFEDEL_DRYRUN, max_files, &k, why, whylen);
        if (!rc) { pick[i] = 1; total += k; entries++; }
    }
    if (!rc && total > max_files) {
        snprintf(m, sizeof m, "%ld expired entries hold more than %ld, the ceiling (pass an explicit force count)", entries, max_files);
        rc = _unisa_sd_say(why, whylen, SAFEDEL_REFUSED, tr, m, "");
    }
    if (!rc && (flags & SAFEDEL_DRYRUN)) {
        snprintf(m, sizeof m, "would purge %ld expired entries (%ld files)", entries, total);
        _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, m, "");
    } else if (!rc) {
        for (i = 0; i < n && !rc; i++)
            if (pick[i] && !_unisa_sd_join(c, tr, v[i])) rc = _unisa_sd_run(c, SAFEDEL_PURGE | SAFEDEL_RECURSIVE, total, &k, why, whylen);
        if (!rc) { snprintf(m, sizeof m, "purged %ld expired entries (%ld files)", entries, total); _unisa_sd_say(why, whylen, SAFEDEL_OK, tr, m, ""); }
    }
    free(pick); _unisa_sd_free(v, n);
    return rc;
#endif
}
#endif
#endif
