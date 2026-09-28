/* winlist: the live window list, straight from the macOS window server.
 *
 * Asks CoreGraphics (CGWindowListCopyWindowInfo) through the explicit FFI
 * bridge in <unisacc_ffi.h> for the windows, and prints per window its id,
 * owning application and pid, layer, on-screen state, alpha, bounds and
 * title; then windows per application, per layer, and the largest on-screen
 * windows by area.  Every value is the window server's own, fetched by the
 * program unisacc compiled -- no collector built by another compiler.
 *
 * macOS hides other applications' window titles unless the terminal has
 * Screen Recording permission; such titles print as "-".  Elsewhere there is
 * no window-server binding yet, so the program says so instead of inventing
 * windows.
 *
 *   unisacc -run examples/apps/winlist.c          # on-screen windows
 *   unisacc -run examples/apps/winlist.c -- --all # every window, hidden too
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <unisacc_ffi.h>
#endif

#define MAXW 1024
struct win {
    long id, pid, layer, onscreen;
    double alpha, x, y, w, h;
    char owner[48], title[64];
};
static struct win W[MAXW];
static int NW;
static long total;

#ifdef __APPLE__
#define F_LIST 0
#define F_COUNT 1
#define F_AT 2
#define F_GET 3
#define F_RECT 4
#define F_STR 5
#define F_REL 6
#define F_NUM 7
#define F_BOOL 8
#define NF 9
static void *fn[NF];

static int call(int f, int ret, int *kinds, void **values, int n, void *out)
{
    int rc = uffi_call(fn[f], ret, kinds, values, n, -1, out);
    if (rc) fprintf(stderr, "winlist: FFI call %d failed (%d)\n", f, rc);
    return rc == 0;
}

/* CFDictionaryGetValue(dict, key): a borrowed object or NULL. */
static void *get(void *dict, void *key)
{
    int pp[2] = { UFFI_POINTER, UFFI_POINTER };
    void *values[2], *out = 0;
    values[0] = &dict; values[1] = &key;
    if (!call(F_GET, UFFI_POINTER, pp, values, 2, &out)) return 0;
    return out;
}

/* CFNumberGetValue as a 64-bit integer (kCFNumberSInt64Type = 4). */
static int number(void *num, long *v)
{
    int kinds[3] = { UFFI_POINTER, UFFI_LONG, UFFI_POINTER };
    long type = 4, ok = 0;
    void *values[3], *dst = v;
    if (!num) return 0;
    values[0] = &num; values[1] = &type; values[2] = &dst;
    return call(F_NUM, UFFI_UINT8, kinds, values, 3, &ok) && ok;
}

/* CFNumberGetValue as a double (kCFNumberDoubleType = 13). */
static int real(void *num, double *v)
{
    int kinds[3] = { UFFI_POINTER, UFFI_LONG, UFFI_POINTER };
    long type = 13, ok = 0;
    void *values[3], *dst = v;
    if (!num) return 0;
    values[0] = &num; values[1] = &type; values[2] = &dst;
    return call(F_NUM, UFFI_UINT8, kinds, values, 3, &ok) && ok;
}

/* CFStringGetCString into buf as UTF-8; "-" when absent or unreadable. */
static void text(void *str, char *buf, long size)
{
    int kinds[4] = { UFFI_POINTER, UFFI_POINTER, UFFI_LONG, UFFI_UINT };
    unsigned int encoding = 0x08000100;
    long ok = 0;
    void *values[4], *dst = buf;
    strcpy(buf, "-");
    if (!str) return;
    values[0] = &str; values[1] = &dst; values[2] = &size; values[3] = &encoding;
    if (!call(F_STR, UFFI_UINT8, kinds, values, 4, &ok) || !ok || !buf[0]) strcpy(buf, "-");
}

static int scan(int all)
{
    void *cg, *cf, *list, *window, *key[8], *values[4], *buffer;
    const char *names[NF] = { "CGWindowListCopyWindowInfo", "CFArrayGetCount",
        "CFArrayGetValueAtIndex", "CFDictionaryGetValue", "CGRectMakeWithDictionaryRepresentation",
        "CFStringGetCString", "CFRelease", "CFNumberGetValue", "CFBooleanGetValue" };
    const char *keys[8] = { "kCGWindowNumber", "kCGWindowOwnerPID", "kCGWindowLayer",
        "kCGWindowIsOnscreen", "kCGWindowAlpha", "kCGWindowBounds", "kCGWindowOwnerName",
        "kCGWindowName" };
    unsigned int options, relative = 0;
    int uu[2] = { UFFI_UINT, UFFI_UINT };
    int pl[2] = { UFFI_POINTER, UFFI_LONG };
    int pp[2] = { UFFI_POINTER, UFFI_POINTER };
    int onep[1] = { UFFI_POINTER };
    double rect[4];
    long count, index, ok;
    int i;
    struct win *w;
    cg = uffi_dlopen("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics", 2);
    cf = uffi_dlopen("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", 2);
    if (!cg || !cf) { fprintf(stderr, "winlist: framework load failed\n"); return 0; }
    for (i = 0; i < NF; i++) {
        fn[i] = uffi_dlsym(i == F_LIST || i == F_RECT ? cg : cf, names[i]);
        if (!fn[i]) { fprintf(stderr, "winlist: missing %s\n", names[i]); return 0; }
    }
    for (i = 0; i < 8; i++) {
        void *addr = uffi_dlsym(cg, keys[i]);
        if (!addr) { fprintf(stderr, "winlist: missing %s\n", keys[i]); return 0; }
        key[i] = *(void **)addr;
    }
    /* kCGWindowListOptionAll = 0, OnScreenOnly = 1 */
    options = all ? 0 : 1;
    values[0] = &options; values[1] = &relative;
    list = 0;
    if (!call(F_LIST, UFFI_POINTER, uu, values, 2, &list) || !list) {
        fprintf(stderr, "winlist: no GUI window-server session\n"); return 0;
    }
    values[0] = &list;
    if (!call(F_COUNT, UFFI_LONG, onep, values, 1, &count) || count < 0) return 0;
    total = count;
    if (count > MAXW) fprintf(stderr, "winlist: listing the frontmost %d of %ld windows\n", MAXW, count);
    for (index = 0; index < count && NW < MAXW; index++) {
        void *bounds, *flag;
        values[0] = &list; values[1] = &index;
        if (!call(F_AT, UFFI_POINTER, pl, values, 2, &window) || !window) return 0;
        w = &W[NW];
        memset(w, 0, sizeof *w);
        number(get(window, key[0]), &w->id);
        number(get(window, key[1]), &w->pid);
        number(get(window, key[2]), &w->layer);
        flag = get(window, key[3]);
        w->onscreen = 0;
        if (flag) {
            ok = 0; values[0] = &flag;
            if (call(F_BOOL, UFFI_UINT8, onep, values, 1, &ok)) w->onscreen = ok != 0;
        }
        if (!real(get(window, key[4]), &w->alpha)) w->alpha = 1;
        bounds = get(window, key[5]);
        if (bounds) {
            ok = 0; buffer = rect; values[0] = &bounds; values[1] = &buffer;
            if (call(F_RECT, UFFI_UINT8, pp, values, 2, &ok) && ok) {
                w->x = rect[0]; w->y = rect[1]; w->w = rect[2]; w->h = rect[3];
            }
        }
        text(get(window, key[6]), w->owner, sizeof w->owner);
        text(get(window, key[7]), w->title, sizeof w->title);
        NW++;
    }
    values[0] = &list;
    call(F_REL, UFFI_VOID, onep, values, 1, 0);
    uffi_dlclose(cf); uffi_dlclose(cg);
    return 1;
}
#endif

static double area(const struct win *w) { return w->w > 0 && w->h > 0 ? w->w * w->h : 0; }

/* Largest first; equal areas keep front-to-back order, because qsort is
   not stable and the C library is free to order ties either way. */
static int by_area(const void *a, const void *b)
{
    const struct win *p = *(const struct win *const *)a, *q = *(const struct win *const *)b;
    double x = area(p), y = area(q);
    if (x != y) return x < y ? 1 : -1;
    return p < q ? -1 : p > q ? 1 : 0;
}

int main(int argc, char **argv)
{
    static const struct win *order[MAXW];
    static char seen[MAXW];
    int all = 0, i, j, n, k, onscreen = 0, titled = 0;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--all")) all = 1;
        else { fprintf(stderr, "usage: winlist [--all]\n"); return 2; }
    }
#ifdef __APPLE__
    if (!scan(all)) return 1;
#else
    fprintf(stderr, "winlist: the window server is only reachable on macOS so far\n");
    return 1;
#endif
    for (i = 0; i < NW; i++) {
        onscreen += W[i].onscreen != 0;
        titled += strcmp(W[i].title, "-") != 0;
    }
    printf("== %d windows (%s), front to back; %d on screen, %d with a readable title\n",
           NW, all ? "all" : "on-screen", onscreen, titled);
    printf("%6s %6s %5s %2s %5s %7s %7s %6s %6s  %s\n",
           "id", "pid", "layer", "on", "alpha", "x", "y", "w", "h", "owner / title");
    for (i = 0; i < NW; i++) {
        struct win *w = &W[i];
        printf("%6ld %6ld %5ld %2s %5.2f %7.0f %7.0f %6.0f %6.0f  %s / %s\n",
               w->id, w->pid, w->layer, w->onscreen ? "y" : "n", w->alpha,
               w->x, w->y, w->w, w->h, w->owner, w->title);
    }

    printf("\n== windows per application\n");
    memset(seen, 0, sizeof seen);
    for (i = 0; i < NW; i++) {
        if (seen[i]) continue;
        n = 0; k = 0;
        for (j = i; j < NW; j++)
            if (!seen[j] && W[j].pid == W[i].pid && !strcmp(W[j].owner, W[i].owner)) {
                seen[j] = 1; n++; k += W[j].onscreen != 0;
            }
        printf("%-32s pid %-6ld %3d window%s, %d on screen\n", W[i].owner, W[i].pid, n, n == 1 ? " " : "s", k);
    }

    printf("\n== windows per layer (0 is the normal application layer)\n");
    memset(seen, 0, sizeof seen);
    for (i = 0; i < NW; i++) {
        if (seen[i]) continue;
        n = 0;
        for (j = i; j < NW; j++) if (!seen[j] && W[j].layer == W[i].layer) { seen[j] = 1; n++; }
        printf("layer %-6ld %3d\n", W[i].layer, n);
    }

    for (i = 0, n = 0; i < NW; i++) if (W[i].onscreen && area(&W[i]) > 0) order[n++] = &W[i];
    qsort(order, n, sizeof order[0], by_area);
    printf("\n== largest on-screen windows (points)\n");
    for (i = 0; i < n && i < 5; i++)
        printf("%9.0f  %s / %s\n", area(order[i]), order[i]->owner, order[i]->title);
    if (total > NW) printf("\n(%ld windows reported, %d listed)\n", total, NW);
    return 0;
}
