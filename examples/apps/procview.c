/* procview: process-tree analysis.
 *
 * On Linux it enumerates real /proc numeric directories using getdents64,
 * then reads their status; no subprocess and no fixed PID scan ceiling.
 * Missing/inaccessible status files and capacity limits are reported.
 *
 * On macOS it calls real libproc through libffi for the process list and
 * PID/parent/RSS/name information. Inaccessible or disappearing processes
 * are counted explicitly. A file or "-" accepts a real ps-format snapshot.
 *
 * It prints the tree with subtree sums, the heaviest processes, a per-command
 * roll-up, and the ways a snapshot can be odd: orphans (parent not listed),
 * self-parents, and parent cycles, which are found without recursing forever.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <unisacc_ffi.h>
#endif

#define MAXP 4096
#define MAXD 256
#ifdef __linux__
#include <dirent.h>
#ifndef PROCVIEW_PROC_ROOT
#define PROCVIEW_PROC_ROOT "/proc"
#endif
#endif

struct proc {
    long pid, ppid, rss, sub;
    int parent, depth, kids, first, next, norss;
    char name[48];
};

struct grp {
    long n, sum;
    const char *name;
};

static struct proc P[MAXP];
static struct grp G[MAXP];
static int ord[MAXP];
static int NP;
static long total;
static int NORSS;
static long minkb;



static void add_line(const char *s)
{
    char *e;
    long pid, ppid, rss;
    const char *n, *base;
    struct proc *p;
    int k;
    if (NP >= MAXP) return;
    pid = strtol(s, &e, 10);
    if (e == s) return;
    s = e;
    ppid = strtol(s, &e, 10);
    if (e == s) return;
    s = e;
    rss = strtol(s, &e, 10);
    if (e == s) return;
    n = e;
    while (*n == ' ' || *n == '\t') n++;
    if (*n == 0 || *n == '\n') return;
    base = n;
    for (k = 0; n[k] && n[k] != '\n' && n[k] != '\r'; k++)
        if (n[k] == '/') base = n + k + 1;
    p = &P[NP++];
    p->pid = pid; p->ppid = ppid; p->rss = rss;
    for (k = 0; base[k] && base[k] != '\n' && base[k] != '\r' && k < 47; k++)
        p->name[k] = base[k];
    while (k > 0 && p->name[k - 1] == ' ') k--;
    p->name[k] = 0;
}

#ifdef __linux__
/* Enumerate directory entries, then independently read each current status. */
static int scan_proc(void)
{
    char path[1024], line[256], out[320];
    DIR *dir; struct dirent *entry; char *end; int skipped = 0, limited = 0;
    long pid, ppid, rss;
    char name[64];
    FILE *f;
    dir = opendir(PROCVIEW_PROC_ROOT);
    if (!dir) { fprintf(stderr, "procview: cannot enumerate proc directory (errno %d)\n", errno); return 0; }
    while ((entry = readdir(dir)) != 0) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        pid = strtol(entry->d_name, &end, 10);
        if (*end || pid <= 0 || pid > 2147483647L) continue;
        if (NP >= MAXP) { limited = 1; break; }
        if (strlen(PROCVIEW_PROC_ROOT) + strlen(entry->d_name) + 9 >= sizeof path) {
            fprintf(stderr, "procview: proc path exceeds capacity\n"); closedir(dir); return 0;
        }
        sprintf(path, "%s/%s/status", PROCVIEW_PROC_ROOT, entry->d_name);
        f = fopen(path, "r");
        if (f == 0) { skipped++; continue; }
        ppid = 0; rss = 0; name[0] = 0;
        while (fgets(line, sizeof line, f)) {
            if (strncmp(line, "Name:", 5) == 0) {
                char *s = line + 5, *e;
                while (*s == ' ' || *s == '\t') s++;
                for (e = s; *e && *e != '\n'; e++) ;
                *e = 0;
                strncpy(name, s, 63); name[63] = 0;
            } else if (strncmp(line, "PPid:", 5) == 0) {
                ppid = strtol(line + 5, 0, 10);
            } else if (strncmp(line, "VmRSS:", 6) == 0) {
                rss = strtol(line + 6, 0, 10);
            }
        }
        fclose(f);
        if (name[0] == 0) { skipped++; continue; }
        sprintf(out, "%ld %ld %ld %s", pid, ppid, rss, name);
        add_line(out);
    }
    if (!limited && errno) { fprintf(stderr, "procview: directory read failed (errno %d)\n", errno); closedir(dir); return 0; }
    if (closedir(dir)) { fprintf(stderr, "procview: directory close failed\n"); return 0; }
    if (skipped) fprintf(stderr, "procview: %d status files inaccessible or disappeared\n", skipped);
    if (limited) fprintf(stderr, "procview: process capacity reached\n");
    return NP > 0;
}
#endif


#ifdef __APPLE__
/* SDK sys/proc_info.h: proc_bsdinfo=136, proc_taskinfo=96 bytes.
 * Kernel output buffers are native layout, never the bundled libc's structs. */
static int scan_mac_proc(void)
{
    static int pids[MAXP];
    long bsd[17], shortbsd[8], task[12], result, zero = 0;
    unsigned int type = 1, typeinfo = 0;
    int capacity = sizeof pids, i, count, vanished = 0, norss = 0, flavor, size, pid;
    int lk[4] = { UFFI_UINT, UFFI_UINT, UFFI_POINTER, UFFI_INT };
    int pk[5] = { UFFI_INT, UFFI_INT, UFFI_ULONG, UFFI_POINTER, UFFI_INT };
    void *buffer = pids, *values[5], *lib, *list, *info;
    struct proc *p;
    lib = uffi_dlopen("/usr/lib/libSystem.B.dylib", 2);
    if (!lib || !(list = uffi_dlsym(lib, "proc_listpids")) ||
        !(info = uffi_dlsym(lib, "proc_pidinfo"))) {
        fprintf(stderr, "procview: cannot resolve libproc APIs\n"); return 0;
    }
    values[0] = &type; values[1] = &typeinfo; values[2] = &buffer; values[3] = &capacity;
    if (uffi_call(list, UFFI_INT, lk, values, 4, -1, &result) || result <= 0 || result % 4) {
        fprintf(stderr, "procview: proc_listpids failed\n"); return 0;
    }
    if (result >= sizeof pids) { fprintf(stderr, "procview: PID capacity reached\n"); return 0; }
    count = result / 4;
    for (i = 0; i < count; i++) {
        pid = pids[i]; if (pid <= 0) continue;
        flavor = 3; size = sizeof bsd; buffer = bsd;
        values[0] = &pid; values[1] = &flavor; values[2] = &zero;
        values[3] = &buffer; values[4] = &size;
        if (uffi_call(info, UFFI_INT, pk, values, 5, -1, &result)) return 0;
        if (NP >= MAXP) { fprintf(stderr, "procview: process capacity reached\n"); break; }
        p = &P[NP];
        if (result == sizeof bsd) {
            p->pid = *(unsigned int *)((char *)bsd + 12);
            p->ppid = *(unsigned int *)((char *)bsd + 16);
            memcpy(p->name, (char *)bsd + 64, 32); p->name[32] = 0;
            if (!p->name[0]) { memcpy(p->name, (char *)bsd + 48, 16); p->name[16] = 0; }
        } else {
            /* Other users' full BSD info is denied; the short record
               (proc_bsdshortinfo, 64 bytes: pid, ppid, comm at 16) is not. */
            flavor = 13; size = sizeof shortbsd; buffer = shortbsd;
            if (uffi_call(info, UFFI_INT, pk, values, 5, -1, &result)) return 0;
            if (result != sizeof shortbsd) { vanished++; continue; }
            p->pid = *(unsigned int *)((char *)shortbsd + 0);
            p->ppid = *(unsigned int *)((char *)shortbsd + 4);
            memcpy(p->name, (char *)shortbsd + 16, 16); p->name[16] = 0;
        }
        flavor = 4; size = sizeof task; buffer = task;
        if (uffi_call(info, UFFI_INT, pk, values, 5, -1, &result)) return 0;
        if (result == sizeof task) p->rss = (unsigned long)task[1] / 1024;
        else { p->rss = 0; p->norss = 1; norss++; }
        NP++;
    }
    NORSS = norss;
    if (vanished) fprintf(stderr, "procview: %d processes exited during the scan\n", vanished);
    uffi_dlclose(lib);
    return NP > 0;
}
#endif

static const char *mb(long kb)
{
    static char buf[4][24];
    static int rot;
    long v = kb * 10 / 1024;
    char *b = buf[rot++ & 3];
    sprintf(b, "%ld.%ldM", v / 10, v % 10);
    return b;
}

static int by_tree(const void *a, const void *b)
{
    const struct proc *x = &P[*(const int *)a];
    const struct proc *y = &P[*(const int *)b];
    if (x->parent != y->parent) return x->parent < y->parent ? -1 : 1;
    if (x->sub != y->sub) return x->sub > y->sub ? -1 : 1;
    return x->pid < y->pid ? -1 : (x->pid > y->pid);
}

static int by_rss(const void *a, const void *b)
{
    const struct proc *x = &P[*(const int *)a];
    const struct proc *y = &P[*(const int *)b];
    if (x->rss != y->rss) return x->rss > y->rss ? -1 : 1;
    return x->pid < y->pid ? -1 : (x->pid > y->pid);
}

static int by_name(const void *a, const void *b)
{
    return strcmp(P[*(const int *)a].name, P[*(const int *)b].name);
}

static int by_sum(const void *a, const void *b)
{
    const struct grp *x = (const struct grp *)a;
    const struct grp *y = (const struct grp *)b;
    if (x->sum != y->sum) return x->sum > y->sum ? -1 : 1;
    return strcmp(x->name, y->name);
}

static void line(int i, const char *pre, const char *branch)
{
    printf("%s%s%s (%ld)  %s", pre, branch, P[i].name, P[i].pid, P[i].norss ? "    ?" : mb(P[i].rss));
    if (P[i].first >= 0) printf("  sum %s", mb(P[i].sub));
    printf("\n");
}

static void show(int i, const char *pre, const char *branch, int level)
{
    char sub[MAXD + 8];
    int c, hidden = 0;
    long hsum = 0;
    line(i, pre, branch);
    if (level >= 40 || strlen(pre) + 4 >= sizeof sub) return;
    strcpy(sub, pre);
    strcat(sub, branch[0] == 0 ? "" : (branch[0] == '`' ? "   " : "|  "));
    for (c = P[i].first; c >= 0; c = P[c].next) {
        if (P[c].sub < minkb) { hidden++; hsum += P[c].sub; continue; }
        show(c, sub, P[c].next >= 0 ? "|- " : "`- ", level + 1);
    }
    if (hidden > 0)
        printf("%s`- ... %d smaller (%s)\n", sub, hidden, mb(hsum));
}

int main(int argc, char **argv)
{
    char buf[512];
    int capture = argc == 2 && strcmp(argv[1], "--capture") == 0;
    FILE *f;
    int i, j, k, ng, roots = 0, orphans = 0, selfp = 0, cyc = 0;
    int fan = -1, top;
    int rootfirst = -1;

    if (argc > 1 && !capture) {
        f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
        if (f == 0) { fprintf(stderr, "procview: cannot open %s\n", argv[1]); return 1; }
        while (fgets(buf, sizeof buf, f)) add_line(buf);
    } else {
#ifdef __linux__
        if (!scan_proc()) { fprintf(stderr, "procview: cannot read live /proc data\n"); return 1; }
#else
#ifdef __APPLE__
        if (!scan_mac_proc()) { fprintf(stderr, "procview: live process query failed\n"); return 1; }
#else
        fprintf(stderr, "procview: provide a real process snapshot on this platform\n"); return 1;
#endif
#endif
    }
    if (NP == 0) { fprintf(stderr, "procview: no processes\n"); return 1; }

    if (capture) {
        for (i = 0; i < NP; i++) printf("%ld %ld %ld %s\n", P[i].pid, P[i].ppid, P[i].rss, P[i].name);
        return 0;
    }

    for (i = 0; i < NP; i++) {
        P[i].parent = -1; P[i].first = -1; P[i].next = -1;
        total += P[i].rss;
    }
    for (i = 0; i < NP; i++) {
        if (P[i].ppid == P[i].pid) { selfp++; continue; }
        for (j = 0; j < NP; j++)
            if (P[j].pid == P[i].ppid) { P[i].parent = j; break; }
    }
    /* Each process adds its RSS to every ancestor; a chain longer than MAXD
     * can only be a cycle, and is marked depth -1 instead of looping. */
    for (i = 0; i < NP; i++) {
        int n = i, steps = 0;
        while (n >= 0 && steps <= MAXD) { P[n].sub += P[i].rss; n = P[n].parent; steps++; }
        P[i].depth = steps > MAXD ? -1 : steps - 1;
        if (P[i].depth < 0) cyc++;
    }
    for (i = 0; i < NP; i++)
        if (P[i].depth >= 0 && P[i].parent >= 0) P[P[i].parent].kids++;
    for (i = 0; i < NP; i++) ord[i] = i;
    qsort(ord, NP, sizeof ord[0], by_tree);
    for (k = NP - 1; k >= 0; k--) {
        i = ord[k];
        if (P[i].depth < 0) continue;
        if (P[i].parent >= 0) { P[i].next = P[P[i].parent].first; P[P[i].parent].first = i; }
        else { P[i].next = rootfirst; rootfirst = i; }
    }
    minkb = NP > 40 ? total / 100 : 0;

    if (NORSS)
        printf("== process tree (%d processes, %s resident; RSS of %d other users' processes needs privileges, shown as ?)\n", NP, mb(total), NORSS);
    else
        printf("== process tree (%d processes, %s resident)\n", NP, mb(total));
    for (i = rootfirst; i >= 0; i = P[i].next)
        if (P[i].ppid != P[i].pid) show(i, "", "", 0);

    printf("\n== heaviest by own RSS\n");
    for (i = 0; i < NP; i++) ord[i] = i;
    qsort(ord, NP, sizeof ord[0], by_rss);
    top = NP < 5 ? NP : 5;
    for (k = 0; k < top; k++)
        printf("%2d. %-28s pid %-5ld %9s  %2ld%%\n", k + 1, P[ord[k]].name, P[ord[k]].pid,
               mb(P[ord[k]].rss), total ? P[ord[k]].rss * 100 / total : 0);

    printf("\n== by command\n");
    for (i = 0; i < NP; i++) ord[i] = i;
    qsort(ord, NP, sizeof ord[0], by_name);
    ng = 0;
    for (k = 0; k < NP; k++) {
        if (ng > 0 && strcmp(G[ng - 1].name, P[ord[k]].name) == 0) {
            G[ng - 1].n++; G[ng - 1].sum += P[ord[k]].rss;
        } else {
            G[ng].n = 1; G[ng].sum = P[ord[k]].rss; G[ng].name = P[ord[k]].name; ng++;
        }
    }
    qsort(G, ng, sizeof G[0], by_sum);
    for (k = 0; k < ng && k < 6; k++)
        printf("%-28s x%-3ld %9s\n", G[k].name, G[k].n, mb(G[k].sum));

    printf("\n== oddities\n");
    for (i = 0; i < NP; i++) {
        if (P[i].depth == 0 && P[i].ppid != 0 && P[i].ppid != P[i].pid) {
            orphans++;
            printf("orphan: %s (%ld) wants parent %ld, not in the snapshot\n",
                   P[i].name, P[i].pid, P[i].ppid);
        }
        if (P[i].depth == 0 && P[i].ppid != P[i].pid) roots++;
        if (P[i].kids > 0 && (fan < 0 || P[i].kids > P[fan].kids)) fan = i;
    }
    if (selfp) printf("self-parent: %d process(es)\n", selfp);
    if (cyc) printf("parent cycle: %d process(es) never reach a root\n", cyc);
    if (fan >= 0) printf("widest fan-out: %s (%ld) with %d children\n", P[fan].name, P[fan].pid, P[fan].kids);
    for (i = 0; i < NP; i++)
        if (P[i].rss * 4 > total)
            printf("heavy: %s (%ld) holds %ld%% of all resident memory\n",
                   P[i].name, P[i].pid, P[i].rss * 100 / total);
    printf("roots %d, orphans %d\n", roots, orphans);
    return 0;
}
