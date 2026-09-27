/* procview: process-tree analysis.
 *
 * Input is `ps -axo pid=,ppid=,rss=,comm=` (RSS in KiB): from a file, from
 * stdin with "-", or -- with no argument -- a built-in snapshot, so the
 * default output is deterministic.
 *
 *   ps -axo pid=,ppid=,rss=,comm= | unisacc -run procview.c -
 *
 * It prints the tree with subtree sums, the heaviest processes, a per-command
 * roll-up, and the ways a snapshot can be odd: orphans (parent not listed),
 * self-parents, and parent cycles, which are found without recursing forever.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXP 4096
#define MAXD 256

struct proc {
    long pid, ppid, rss, sub;
    int parent, depth, kids, first, next;
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
static long minkb;

static const char *sample[] = {
    "    1     0  11208 /sbin/launchd",
    "  101     1   9016 /usr/sbin/syslogd",
    "  102     1  14520 /usr/libexec/logd",
    "  118     1  62344 /usr/libexec/UserEventAgent",
    "  201     1 188420 /System/Library/PrivateFrameworks/SkyLight.framework/Resources/WindowServer",
    "  244     1  41872 /usr/sbin/coreaudiod",
    "  310     1  71408 /System/Library/CoreServices/loginwindow.app/Contents/MacOS/loginwindow",
    "  322   310 148900 /System/Library/CoreServices/Finder.app/Contents/MacOS/Finder",
    "  330   310  93216 /System/Library/CoreServices/Dock.app/Contents/MacOS/Dock",
    "  341   310  58120 /System/Library/CoreServices/SystemUIServer.app/Contents/MacOS/SystemUIServer",
    "  402   310 176640 /Applications/Utilities/Terminal.app/Contents/MacOS/Terminal",
    "  410   402   5120 /usr/bin/login",
    "  411   410   6784 -zsh",
    "  437   411  98432 tmux",
    "  438   437   6912 -zsh",
    "  441   438 312560 node",
    "  447   441  47120 node",
    "  452   441  46880 node",
    "  455   438  22016 cc",
    "  456   455  18432 cc1",
    "  460   437   6640 -zsh",
    "  466   460 421376 python3",
    "  520   310 612288 /Applications/Safari.app/Contents/MacOS/Safari",
    "  531   520 233120 Safari Web Content",
    "  532   520 187264 Safari Web Content",
    "  533   520 141056 Safari Web Content",
    "  534   520  38224 com.apple.WebKit.Networking",
    "  601   310 274816 /Applications/Slack.app/Contents/MacOS/Slack",
    "  612   601 143360 Slack Helper (Renderer)",
    "  613   601  60288 Slack Helper (GPU)",
    "  620   601  31744 Slack Helper",
    "  812   899   7424 /usr/local/bin/orphaned-worker",
    "  700   701   1024 cyc-a",
    "  701   700   1024 cyc-b",
    "  777   777   2048 self-parent",
    0
};

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
    printf("%s%s%s (%ld)  %s", pre, branch, P[i].name, P[i].pid, mb(P[i].rss));
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
    FILE *f;
    int i, j, k, ng, roots = 0, orphans = 0, selfp = 0, cyc = 0;
    int fan = -1, top;
    int rootfirst = -1;

    if (argc > 1) {
        f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
        if (f == 0) { fprintf(stderr, "procview: cannot open %s\n", argv[1]); return 1; }
        while (fgets(buf, sizeof buf, f)) add_line(buf);
    } else {
        for (i = 0; sample[i]; i++) add_line(sample[i]);
    }
    if (NP == 0) { fprintf(stderr, "procview: no processes\n"); return 1; }

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

    printf("== process tree (%d processes, %s resident)\n", NP, mb(total));
    for (i = rootfirst; i >= 0; i = P[i].next)
        if (P[i].ppid != P[i].pid) show(i, "", "", 0);

    printf("\n== heaviest by own RSS\n");
    for (i = 0; i < NP; i++) ord[i] = i;
    qsort(ord, NP, sizeof ord[0], by_rss);
    top = NP < 5 ? NP : 5;
    for (k = 0; k < top; k++)
        printf("%2d. %-28s pid %-5ld %9s  %2ld%%\n", k + 1, P[ord[k]].name, P[ord[k]].pid,
               mb(P[ord[k]].rss), P[ord[k]].rss * 100 / total);

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
