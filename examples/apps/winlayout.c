/* winlayout: window-stack analysis.
 *
 * Input, one window per line, listed bottom to top (the last line is the
 * topmost):
 *
 *     screen WIDTH HEIGHT          (optional, default 1440 900)
 *     X Y W H title...             (X and Y may be negative)
 *     # comment
 *
 * On macOS, no arguments queries CoreGraphics/CoreFoundation via libffi.
 * A file or stdin with "-" selects explicit geometry. On X11, wmctrl -lG gives
 * the geometry; put it in stacking order and reshape it with awk.
 *
 * The visible area of every window is exact.  All rectangle edges are
 * collected, sorted and de-duplicated, which cuts the screen into a grid whose
 * cells lie wholly inside or wholly outside every window; each cell then
 * belongs to the topmost window that covers it.  The same grid gives the
 * uncovered desktop and its largest empty rectangle, and a text minimap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <unisacc_ffi.h>
#endif

#define MAXW 64
#define MAXE (2 * MAXW + 2)
#define COLS 64
#define ROWS 20

struct win {
    long x, y, w, h;         /* as given */
    long x0, y0, x1, y1;     /* clipped to the screen; empty when x0 >= x1 */
    long area, clip, vis;
    char title[40];
};

static struct win W[MAXW];
static int NW;
static long SW = 1440, SH = 900;
static long xs[MAXE], ys[MAXE];
static int nx, ny;
static int owner[MAXE][MAXE];      /* owner[row][col]: topmost window or -1 */



static long lmin(long a, long b) { return a < b ? a : b; }
static long lmax(long a, long b) { return a > b ? a : b; }

static char label(int i)
{
    if (i < 26) return (char)('A' + i);
    if (i < 52) return (char)('a' + i - 26);
    return '#';
}

static void add_line(char *s)
{
    char *e;
    long v[4];
    int k;
    struct win *w;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '#' || *s == '\n' || *s == '\r' || *s == 0) return;
    if (strncmp(s, "screen", 6) == 0) {
        long a = strtol(s + 6, &e, 10);
        long b = strtol(e, &e, 10);
        if (a > 0 && b > 0 && a <= 16384 && b <= 16384) { SW = a; SH = b; }
        return;
    }
    if (NW >= MAXW) return;
    for (k = 0; k < 4; k++) {
        v[k] = strtol(s, &e, 10);
        if (e == s) return;
        s = e;
    }
    if (v[2] <= 0 || v[3] <= 0) return;          /* an empty window */
    while (*s == ' ' || *s == '\t') s++;
    w = &W[NW++];
    w->x = v[0]; w->y = v[1]; w->w = v[2]; w->h = v[3];
    for (k = 0; s[k] && s[k] != '\n' && s[k] != '\r' && k < 39; k++) w->title[k] = s[k];
    if (k == 0) { strcpy(w->title, "(untitled)"); } else w->title[k] = 0;
}


#ifdef __APPLE__
/* CoreGraphics/CoreFoundation values remain opaque host objects. */
static void *cgfn[8];
static int cgcall(int fn, int ret, int *kinds, void **values, int n, void *out)
{
    int rc = uffi_call(cgfn[fn], ret, kinds, values, n, -1, out);
    if (rc) fprintf(stderr, "winlayout: FFI call %d failed (%d)\n", fn, rc);
    return rc == 0;
}
static int scan_mac_windows(void)
{
    void *cg, *cf, *wins, *window, *bounds, *name, *keybounds, *keyowner;
    void *values[4], *buffer, *typeargs[1], *elements[5];
    struct { unsigned long size; unsigned short alignment, type; void **elements; } recttype;
    double rect[4], screen[4];
    unsigned int options = 17, relative = 0, display, encoding = 0x08000100;
    long result, count, index, limit;
    int pp[2] = { UFFI_POINTER, UFFI_POINTER };
    int uu[2] = { UFFI_UINT, UFFI_UINT };
    int pl[2] = { UFFI_POINTER, UFFI_LONG };
    int stringk[4] = { UFFI_POINTER, UFFI_POINTER, UFFI_LONG, UFFI_UINT };
    int onep[1] = { UFFI_POINTER };
    long bufsize = 40;
    const char *names[8] = { "CGWindowListCopyWindowInfo", "CFArrayGetCount",
        "CFArrayGetValueAtIndex", "CFDictionaryGetValue", "CGRectMakeWithDictionaryRepresentation",
        "CFStringGetCString", "CFRelease", "CGMainDisplayID" };
    void *screenfn;
    int i;
    struct win *w;
    cg = uffi_dlopen("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics", 2);
    cf = uffi_dlopen("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", 2);
    if (!cg || !cf) { fprintf(stderr, "winlayout: framework load failed\n"); return 0; }
    for (i = 0; i < 8; i++) {
        cgfn[i] = uffi_dlsym(i == 0 || i == 4 || i == 7 ? cg : cf, names[i]);
        if (!cgfn[i]) { fprintf(stderr, "winlayout: missing %s\n", names[i]); return 0; }
    }
    keybounds = uffi_dlsym(cg, "kCGWindowBounds"); keyowner = uffi_dlsym(cg, "kCGWindowOwnerName");
    screenfn = uffi_dlsym(cg, "CGDisplayBounds");
    if (!keybounds || !keyowner || !screenfn) return 0;
    keybounds = *(void **)keybounds; keyowner = *(void **)keyowner;
    if (!cgcall(7, UFFI_UINT, 0, 0, 0, &result)) return 0;
    display = result;
    /* CGRect is four doubles (two nested two-double structs in the SDK).
       libffi classifies this layout for each target ABI, including HFA return. */
    for (i = 0; i < 4; i++) elements[i] = uffi_type(UFFI_DOUBLE);
    elements[4] = 0; recttype.size = 0; recttype.alignment = 0;
    recttype.type = 13; recttype.elements = elements;
    typeargs[0] = uffi_type(UFFI_UINT); values[0] = &display;
    if (uffi_call_types(screenfn, &recttype, typeargs, values, 1, -1, screen)) return 0;
    if (screen[2] <= 0 || screen[3] <= 0 || screen[2] > 16384 || screen[3] > 16384) return 0;
    SW = screen[2]; SH = screen[3];
    values[0] = &options; values[1] = &relative;
    if (!cgcall(0, UFFI_POINTER, uu, values, 2, &wins) || !wins) {
        fprintf(stderr, "winlayout: no GUI window-server session\n"); return 0;
    }
    values[0] = &wins;
    if (!cgcall(1, UFFI_LONG, onep, values, 1, &count) || count < 0) return 0;
    limit = count > MAXW ? MAXW : count;
    if (count > MAXW) fprintf(stderr, "winlayout: analysing the frontmost %d of %ld windows\n", MAXW, count);
    /* Native array is front-to-back; the analyser stores bottom-to-top. */
    for (index = limit - 1; index >= 0; index--) {
        values[0] = &wins; values[1] = &index;
        if (!cgcall(2, UFFI_POINTER, pl, values, 2, &window)) return 0;
        values[0] = &window; values[1] = &keybounds;
        if (!cgcall(3, UFFI_POINTER, pp, values, 2, &bounds)) return 0;
        values[1] = &keyowner;
        if (!cgcall(3, UFFI_POINTER, pp, values, 2, &name)) return 0;
        if (!bounds || !name) continue;
        buffer = rect; values[0] = &bounds; values[1] = &buffer;
        if (!cgcall(4, UFFI_UINT8, pp, values, 2, &result)) return 0;
        if (!result || rect[2] <= 0 || rect[3] <= 0) continue;
        w = &W[NW]; buffer = w->title;
        values[0] = &name; values[1] = &buffer; values[2] = &bufsize; values[3] = &encoding;
        if (!cgcall(5, UFFI_UINT8, stringk, values, 4, &result)) return 0;
        if (!result) strcpy(w->title, "(owner name unavailable)");
        w->title[39] = 0;
        /* Geometry is in main-display points, not Retina physical pixels. */
        w->x = rect[0] - screen[0]; w->y = rect[1] - screen[1];
        w->w = rect[2]; w->h = rect[3];
        if (w->w > 0 && w->h > 0) NW++;
    }
    values[0] = &wins;
    if (!cgcall(6, UFFI_VOID, onep, values, 1, 0)) return 0;
    uffi_dlclose(cf); uffi_dlclose(cg);
    return 1;
}
#endif

static int edge(long *v, int n, long e)
{
    int i, j;
    for (i = 0; i < n; i++) if (v[i] == e) return n;
    for (i = n; i > 0 && v[i - 1] > e; i--) ;
    for (j = n; j > i; j--) v[j] = v[j - 1];
    v[i] = e;
    return n + 1;
}

static int topmost(long px, long py)
{
    int j;
    for (j = NW - 1; j >= 0; j--)
        if (W[j].clip > 0 && px >= W[j].x0 && px < W[j].x1 && py >= W[j].y0 && py < W[j].y1)
            return j;
    return -1;
}

int main(int argc, char **argv)
{
    char buf[512];
    FILE *f;
    int i, j, c, r, pairs = 0, pi = -1, pj = -1;
    long covered = 0, pbest = 0;
    long fx = 0, fy = 0, fw = 0, fh = 0, farea = 0;
    static char rowok[MAXE];

    if (argc > 1) {
        f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
        if (f == 0) { fprintf(stderr, "winlayout: cannot open %s\n", argv[1]); return 1; }
        while (fgets(buf, sizeof buf, f)) add_line(buf);
    } else {
#ifdef __APPLE__
        if (!scan_mac_windows()) { fprintf(stderr, "winlayout: live window query failed\n"); return 1; }
#else
        fprintf(stderr, "winlayout: provide real window geometry on this platform\n"); return 1;
#endif
    }
    if (NW == 0) { fprintf(stderr, "winlayout: no windows\n"); return 1; }

    nx = edge(xs, 0, 0); nx = edge(xs, nx, SW);
    ny = edge(ys, 0, 0); ny = edge(ys, ny, SH);
    for (i = 0; i < NW; i++) {
        struct win *w = &W[i];
        w->x0 = lmax(w->x, 0); w->y0 = lmax(w->y, 0);
        w->x1 = lmin(w->x + w->w, SW); w->y1 = lmin(w->y + w->h, SH);
        w->area = w->w * w->h;
        w->clip = (w->x1 > w->x0 && w->y1 > w->y0) ? (w->x1 - w->x0) * (w->y1 - w->y0) : 0;
        if (w->clip > 0) {
            nx = edge(xs, nx, w->x0); nx = edge(xs, nx, w->x1);
            ny = edge(ys, ny, w->y0); ny = edge(ys, ny, w->y1);
        }
    }
    for (r = 0; r + 1 < ny; r++)
        for (c = 0; c + 1 < nx; c++) {
            long cell = (xs[c + 1] - xs[c]) * (ys[r + 1] - ys[r]);
            int o = topmost(xs[c], ys[r]);
            owner[r][c] = o;
            if (o >= 0) { W[o].vis += cell; covered += cell; }
        }

    printf("== screen %ldx%ld, %d windows (topmost first)\n", SW, SH, NW);
    for (i = NW - 1; i >= 0; i--) {
        struct win *w = &W[i];
        printf("%c %-12s %5ld,%-5ld %4ldx%-4ld area %8ld visible %3ld%%  ", label(i), w->title,
               w->x, w->y, w->w, w->h, w->area, w->vis * 100 / w->area);
        if (w->clip == 0) printf("OFF-SCREEN");
        else if (w->vis == 0) printf("HIDDEN");
        else if (w->vis < w->clip) printf("partly covered");
        else printf("unobstructed");
        if (w->clip > 0 && w->clip < w->area)
            printf(", %ld%% off-screen", (w->area - w->clip) * 100 / w->area);
        printf("\n");
    }

    for (i = 0; i < NW; i++)
        for (j = i + 1; j < NW; j++) {
            long ix = lmin(W[i].x1, W[j].x1) - lmax(W[i].x0, W[j].x0);
            long iy = lmin(W[i].y1, W[j].y1) - lmax(W[i].y0, W[j].y0);
            if (W[i].clip > 0 && W[j].clip > 0 && ix > 0 && iy > 0) {
                pairs++;
                if (ix * iy > pbest) { pbest = ix * iy; pi = i; pj = j; }
            }
        }

    /* Largest empty rectangle: for every span of columns [i, j] keep the rows
     * that are empty across the whole span, and take the tallest run of them. */
    for (i = 0; i + 1 < nx; i++) {
        for (r = 0; r + 1 < ny; r++) rowok[r] = 1;
        for (j = i; j + 1 < nx; j++) {
            int start = -1;
            for (r = 0; r + 1 < ny; r++) if (owner[r][j] >= 0) rowok[r] = 0;
            for (r = 0; r < ny; r++) {
                if (r + 1 < ny && rowok[r]) { if (start < 0) start = r; continue; }
                if (start >= 0) {
                    long ww = xs[j + 1] - xs[i], hh = ys[r] - ys[start];
                    if (ww * hh > farea) { farea = ww * hh; fx = xs[i]; fy = ys[start]; fw = ww; fh = hh; }
                    start = -1;
                }
            }
        }
    }

    printf("\n== coverage\n");
    printf("desktop covered %ld%%, uncovered %ld%%\n", covered * 100 / (SW * SH),
           (SW * SH - covered) * 100 / (SW * SH));
    if (farea > 0) printf("largest empty rectangle %ldx%ld at %ld,%ld\n", fw, fh, fx, fy);
    printf("overlapping pairs %d", pairs);
    if (pi >= 0) printf(", biggest overlap %s / %s = %ld px^2", W[pi].title, W[pj].title, pbest);
    printf("\n");
    for (i = 0; i < NW; i++)
        if (W[i].clip > 0 && W[i].vis == 0)
            printf("hidden: %s -- raise it or close it\n", W[i].title);
        else if (W[i].clip == 0)
            printf("off-screen: %s at %ld,%ld -- move it back\n", W[i].title, W[i].x, W[i].y);

    printf("\n== minimap (letters are windows, . is desktop)\n+");
    for (c = 0; c < COLS; c++) putchar('-');
    printf("+\n");
    for (r = 0; r < ROWS; r++) {
        putchar('|');
        for (c = 0; c < COLS; c++) {
            int o = topmost((2L * c + 1) * SW / (2 * COLS), (2L * r + 1) * SH / (2 * ROWS));
            putchar(o >= 0 ? label(o) : '.');
        }
        printf("|\n");
    }
    printf("+");
    for (c = 0; c < COLS; c++) putchar('-');
    printf("+\n");
    return 0;
}
