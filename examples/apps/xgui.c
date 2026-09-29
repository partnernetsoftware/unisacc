/* xgui: a real X11 window from unisacc, with no Xlib -- Linux x86-64 only.
 *
 * Opens a 640 x 400 window on the local X server and draws into it: a
 * Mandelbrot image computed in doubles and sent with PutImage, filled
 * arcs, line segments, bars and text.  It speaks the X11 wire protocol
 * itself over the Unix socket /tmp/.X11-unix/X<N> (N from $DISPLAY,
 * default 0); window, GC, font, property and drawing requests are
 * hand-encoded little-endian byte strings.  Redraws on every Expose; q or
 * Esc (evdev keycodes 24 and 9) or closing the window ends it.
 *
 * PLATFORMS: Linux x86-64 with a running X server that accepts
 * connections without an auth cookie (a local Xvfb/Xvnc/Xorg desktop is
 * typical).  Every other target -- macOS, Windows, Linux arm64 -- prints
 * an explicit refusal and exits 1; nothing is simulated.
 *
 * THE TRICK.  The bundled libc has no socket()/connect() and there is no
 * dlopen bridge on Linux, only a few fixed system-call gates (__mmap,
 * __mprotect, __read, ...).  So the program builds its own system call:
 * it copies the 49-byte machine-code stub below into a page from
 * __mmap, flips the page to read+execute with __mprotect (never writable
 * and executable at once), and calls it through a function pointer.
 *
 * The stub must follow unisacc's PRIVATE calling convention, not the SysV
 * AMD64 ABI: unisacc-compiled code passes every argument on the stack
 * (first argument at the lowest address, just above the return address),
 * keeps r9 as its frame pointer, and takes the result in rax.  A SysV stub
 * that reads rdi/rsi/rdx... gets garbage and hangs.  So the stub
 *     push r9; push rcx; push r11     r9 is also syscall arg 6; the
 *                                     syscall instruction clobbers rcx/r11
 *     mov rax,[rsp+32] .. r9,[rsp+80] number and six arguments, from the
 *                                     caller's stack slots (3 saves +
 *                                     return address = 32 bytes below)
 *     syscall
 *     pop r11; pop rcx; pop r9; ret   result stays in rax
 * This convention was found empirically against unisacc 0.0.11/0.0.12.  It
 * is undocumented and may change in any release; treat this as a
 * demonstration of the compiler's reach, not an API.
 *
 *   DISPLAY=:0 unisacc -run examples/apps/xgui.c
 *   unisacc -run examples/apps/xgui.c -- --once   # exit after first draw
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) && defined(__x86_64__)
typedef long (*scfn)(long, long, long, long, long, long, long);
static const unsigned char stub[] = {
    0x41, 0x51, 0x51, 0x41, 0x53,             /* push r9; push rcx; push r11 */
    0x48, 0x8b, 0x44, 0x24, 0x20,             /* mov rax, [rsp+0x20]  nr     */
    0x48, 0x8b, 0x7c, 0x24, 0x28,             /* mov rdi, [rsp+0x28]  a1     */
    0x48, 0x8b, 0x74, 0x24, 0x30,             /* mov rsi, [rsp+0x30]  a2     */
    0x48, 0x8b, 0x54, 0x24, 0x38,             /* mov rdx, [rsp+0x38]  a3     */
    0x4c, 0x8b, 0x54, 0x24, 0x40,             /* mov r10, [rsp+0x40]  a4     */
    0x4c, 0x8b, 0x44, 0x24, 0x48,             /* mov r8,  [rsp+0x48]  a5     */
    0x4c, 0x8b, 0x4c, 0x24, 0x50,             /* mov r9,  [rsp+0x50]  a6     */
    0x0f, 0x05,                               /* syscall                     */
    0x41, 0x5b, 0x59, 0x41, 0x59, 0xc3        /* pop r11; pop rcx; pop r9; ret */
};
static scfn sc;

#define SYS_read 0
#define SYS_write 1
#define SYS_socket 41
#define SYS_connect 42

#define W 640
#define H 400
#define IW 360
#define IH 240
static long fd;
static unsigned char ob[300000];
static int on;
static unsigned char rb[65536];
static unsigned long idbase, root, wid, gc, fid;
static unsigned int *img;
static int exposes;

static void p8(int v) { ob[on++] = (unsigned char)v; }
static void p16(int v) { ob[on++] = v & 255; ob[on++] = (v >> 8) & 255; }
static void p32(unsigned long v) { p16((int)(v & 0xffff)); p16((int)((v >> 16) & 0xffff)); }
static void pad4(void) { while (on & 3) ob[on++] = 0; }
static void flush(void)
{
    int done = 0;
    long r;
    while (done < on) {
        r = sc(SYS_write, fd, (long)(ob + done), on - done, 0, 0, 0);
        if (r <= 0) { printf("xgui: write failed %ld\n", r); exit(1); }
        done += (int)r;
    }
    on = 0;
}
static int readn(unsigned char *b, int n)
{
    int got = 0;
    long r;
    while (got < n) {
        r = sc(SYS_read, fd, (long)(b + got), n - got, 0, 0, 0);
        if (r <= 0) return -1;
        got += (int)r;
    }
    return got;
}
static unsigned long g32(unsigned char *b) { return b[0] | (b[1] << 8) | (b[2] << 16) | ((unsigned long)b[3] << 24); }
static int g16(unsigned char *b) { return b[0] | (b[1] << 8); }

/* X11 core requests: opcode, data byte, length in 4-byte units, body. */
static void setfg(unsigned long c) { p8(56); p8(0); p16(4); p32(gc); p32(4); p32(c); }   /* ChangeGC foreground */
static void rect(int x, int y, int w, int h, unsigned long c)                         /* PolyFillRectangle */
{
    setfg(c); p8(70); p8(0); p16(5); p32(wid); p32(gc); p16(x); p16(y); p16(w); p16(h);
}
static void arc(int x, int y, int w, int h, unsigned long c)                          /* PolyFillArc */
{
    setfg(c); p8(71); p8(0); p16(6); p32(wid); p32(gc);
    p16(x); p16(y); p16(w); p16(h); p16(0); p16(360 * 64);
}
static void text(int x, int y, const char *s, unsigned long c, unsigned long bg)      /* ImageText8 */
{
    int n = (int)strlen(s);
    p8(56); p8(0); p16(5); p32(gc); p32(4 | 8); p32(c); p32(bg);                      /* ChangeGC fg+bg */
    p8(76); p8(n); p16(4 + (n + 3) / 4); p32(wid); p32(gc); p16(x); p16(y);
    memcpy(ob + on, s, n); on += n; pad4();
}
static void line(int x0, int y0, int x1, int y1, unsigned long c)                     /* PolySegment */
{
    setfg(c); p8(66); p8(0); p16(5); p32(wid); p32(gc); p16(x0); p16(y0); p16(x1); p16(y1);
}

static void render(void)
{
    int x, y, i;
    for (y = 0; y < IH; y++)
        for (x = 0; x < IW; x++) {
            double cr = -2.2 + 3.2 * x / IW, ci = -1.2 + 2.4 * y / IH, zr = 0, zi = 0, t;
            for (i = 0; i < 64 && zr * zr + zi * zi < 4.0; i++) { t = zr * zr - zi * zi + cr; zi = 2 * zr * zi + ci; zr = t; }
            img[y * IW + x] = i == 64 ? 0x101020 : ((i * 9 & 255) << 16) | ((i * 5 & 255) << 8) | (255 - i * 3);
        }
}
static void putimage(int dx, int dy)                                                  /* PutImage, ZPixmap */
{
    int y0, rows, n;
    for (y0 = 0; y0 < IH; y0 += rows) {
        rows = IH - y0 < 60 ? IH - y0 : 60;
        n = IW * rows * 4;
        p8(72); p8(2); p16(6 + n / 4); p32(wid); p32(gc);
        p16(IW); p16(rows); p16(dx); p16(dy + y0); p8(0); p8(24); p16(0);
        memcpy(ob + on, img + y0 * IW, n); on += n;
        flush();
    }
}
static void draw(void)
{
    int i;
    char buf[64];
    rect(0, 0, W, H, 0x203040);
    rect(0, 0, W, 28, 0x3060a0);
    text(10, 19, "unisacc raw-X11 GUI (no Xlib, syscall stub)", 0xffffff, 0x3060a0);
    putimage(10, 40);
    for (i = 0; i < 6; i++) arc(390 + i * 38, 50 + (i & 1) * 30, 34, 34, 0xff0000 >> (i * 4) | 0x2020);
    for (i = 0; i < 12; i++) line(390, 140 + i * 8, 620, 240 - i * 8, 0x40ff40 + i * 0x1000);
    rect(390, 260, 230, 20, 0xe0c020);
    text(396, 275, "bars from a unisacc double loop:", 0x000000, 0xe0c020);
    for (i = 0; i < 10; i++) {
        int h = (int)(60.0 * (i + 1) * (i + 1) / 100.0);
        rect(392 + i * 23, 350 - h, 18, h, 0x30c0ff);
    }
    exposes++;
    sprintf(buf, "expose #%d  window 0x%lx  press q/Esc to close", exposes, wid);
    text(10, 300, buf, 0xffff80, 0x203040);
    text(10, 320, "struct/double/printf all compiled by unisacc.com", 0xc0c0c0, 0x203040);
    flush();
}

int main(int argc, char **argv)
{
    unsigned char *p;
    int i, n, vlen, nfmt, extra, once = 0, dnum = 0;
    char path[108];
    unsigned char sa[110];
    const char *d = getenv("DISPLAY");
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--once")) once = 1;
        else { fprintf(stderr, "usage: xgui [--once]\n"); return 2; }
    }
    if (d && d[0] == ':') dnum = atoi(d + 1);

    /* The system-call stub: write it, then make the page read+execute. */
    p = (unsigned char *)__mmap(0, 4096, 3, 0x22, 0 - 1, 0);    /* RW, MAP_PRIVATE|MAP_ANONYMOUS */
    if ((long)p <= 0) { fprintf(stderr, "xgui: mmap failed\n"); return 1; }
    for (i = 0; i < (int)sizeof(stub); i++) p[i] = stub[i];
    __mprotect(p, 4096, 5);                                        /* R+X */
    sc = (scfn)p;

    img = (unsigned int *)malloc(IW * IH * 4);
    if (!img) { fprintf(stderr, "xgui: out of memory\n"); return 1; }
    render();

    sprintf(path, "/tmp/.X11-unix/X%d", dnum);
    fd = sc(SYS_socket, 1, 1, 0, 0, 0, 0);                         /* AF_UNIX, SOCK_STREAM */
    if (fd < 0) { fprintf(stderr, "xgui: socket failed %ld\n", fd); return 1; }
    memset(sa, 0, sizeof sa); sa[0] = 1; strcpy((char *)sa + 2, path);
    if (sc(SYS_connect, fd, (long)sa, 2 + (long)strlen(path) + 1, 0, 0, 0) < 0) {
        fprintf(stderr, "xgui: connect %s failed (is an X server running? set DISPLAY)\n", path);
        return 1;
    }
    printf("connected to %s fd=%ld\n", path, fd);

    /* Connection setup: little-endian, protocol 11.0, no authorization. */
    p8(0x6c); p8(0); p16(11); p16(0); p16(0); p16(0); p16(0); flush();
    if (readn(rb, 8) < 0 || rb[0] != 1) { fprintf(stderr, "xgui: setup refused (%d); the server may require an auth cookie\n", rb[0]); return 1; }
    extra = g16(rb + 6) * 4;
    if (extra > (int)sizeof rb - 8) { fprintf(stderr, "xgui: setup reply too large (%d)\n", extra); return 1; }
    if (readn(rb + 8, extra) < 0) { fprintf(stderr, "xgui: setup reply truncated\n"); return 1; }
    idbase = g32(rb + 12); vlen = g16(rb + 24); nfmt = rb[29];
    p = rb + 40 + ((vlen + 3) & ~3) + 8 * nfmt;                     /* first SCREEN */
    root = g32(p);
    printf("idbase=0x%lx root=0x%lx screen=%dx%d depth=%d\n", idbase, root, g16(p + 20), g16(p + 22), p[38]);
    wid = idbase + 1; gc = idbase + 2; fid = idbase + 3;

    /* CreateWindow (background pixel, event mask: KeyPress|Exposure|StructureNotify) */
    p8(1); p8(0); p16(10); p32(wid); p32(root); p16(80); p16(60); p16(W); p16(H); p16(1); p16(1); p32(0);
    p32(0x802); p32(0x203040); p32(0x8000 | 1 | 0x20000);
    n = 32;                                                         /* ChangeProperty WM_NAME, STRING */
    p8(18); p8(0); p16(6 + 8); p32(wid); p32(39); p32(31); p8(8); p8(0); p16(0); p32(n);
    memcpy(ob + on, "unisacc raw X11 demo            ", n); on += n; pad4();
    p8(45); p8(0); p16(3 + 2); p32(fid); p16(5); p16(0); memcpy(ob + on, "fixed", 5); on += 5; pad4();  /* OpenFont */
    p8(55); p8(0); p16(6); p32(gc); p32(wid); p32(0x4004); p32(0xffffff); p32(fid);                   /* CreateGC */
    p8(8); p8(0); p16(2); p32(wid);                                                                     /* MapWindow */
    flush();

    for (;;) {
        if (readn(rb, 32) < 0) { printf("connection closed\n"); return 0; }
        if (rb[0] == 0) printf("X error code=%d major=%d\n", rb[1], rb[10]);
        else if ((rb[0] & 0x7f) == 12 && g16(rb + 16) == 0) {       /* last Expose of a batch */
            draw();
            printf("drew (expose %d)\n", exposes);
            if (once) return 0;
        }
        else if ((rb[0] & 0x7f) == 2 && (rb[1] == 24 || rb[1] == 9)) { printf("quit key\n"); return 0; }
        else if (rb[0] == 1) readn(rb + 32, (int)g32(rb + 4) * 4);  /* a reply: skip its extra data */
    }
}
#else
int main(void)
{
    fprintf(stderr, "xgui: raw X11 through a syscall stub needs Linux x86-64; not available on this platform\n");
    return 1;
}
#endif
