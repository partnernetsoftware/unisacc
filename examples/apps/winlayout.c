/* winlayout: window-stack analysis.
 *
 * Input, one window per line, listed bottom to top (the last line is the
 * topmost):
 *
 *     screen WIDTH HEIGHT          (optional, default 1440 900)
 *     X Y W H title...             (X and Y may be negative)
 *     # comment
 *
 * From a file, from stdin with "-", or -- with no argument -- a built-in
 * desktop, so the default output is deterministic.  On X11, wmctrl -lG gives
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

static const char *sample[] = {
    "screen 1440 900",
    "# bottom",
    "120 90 900 620 Notes",
    "60 40 1200 800 Browser",
    "-100 640 300 320 Music",
    "20 560 700 320 Terminal",
    "700 60 340 220 Calculator",
    "1100 300 420 500 Slack",
    "1180 20 240 90 Clock",
    "1600 200 300 200 Parked",
    "# top",
    0
};

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
        /* No window-server access is possible from this compiler's own C
         * library: reading real geometry needs X11/Wayland sockets, the
         * Win32 API or CoreGraphics, none of which this front end binds.
         * That is a real gap, not a shortcut -- say so plainly instead of
         * letting the realistic-looking sample pass as live data. */
        fprintf(stderr, "winlayout: no input given -- showing a built-in "
                        "SAMPLE, not real data. Try:\n"
                        "  wmctrl -lG | awk '{print $3,$4,$5,$6,$7}' | "
                        "unisacc -run winlayout.c -\n");
        for (i = 0; sample[i]; i++) {
            strcpy(buf, sample[i]);
            add_line(buf);
        }
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
