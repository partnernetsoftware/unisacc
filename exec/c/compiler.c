/* Product-driver migration: IO and route selection only. All source handling
   is performed by the package's networks. Unsupported CLI remains an explicit
   error until migrated; no call to a reference compiler. Not the default yet. */
#define UNISA_RUNTIME_LIBRARY
#include "run.c"
#undef UNISA_RUNTIME_LIBRARY

#ifdef __aarch64__
#define NATIVE_ARCH "arm64"
#else
#define NATIVE_ARCH "x86_64"
#endif
#ifdef _WIN32
#define NATIVE_OS "win"
#else
#ifdef __APPLE__
#define NATIVE_OS "osx"
#else
#define NATIVE_OS "lnx"
#endif
#endif

static int clierror(const char *s) {
    fprintf(stderr, "unisacc: model driver: %s\n", s); return 1;
}
static long output_open(const char *path) {
#ifdef __UNISA__
#ifdef _WIN32
    return __open((char *)path, 0x40000000, 2);
#else
#ifdef __linux__
    return __open((char *)path, 1|64|512, 493);
#else
    return __open((char *)path, 1|512|1024, 493);
#endif
#endif
#else
    return open(path, O_WRONLY|O_CREAT|O_TRUNC, 0755);
#endif
}
static long output_write(long fd, const void *p, long n) {
#ifdef __UNISA__
    return __write(fd, (char *)p, n);
#else
    return write((int)fd, p, (size_t)n);
#endif
}
int main(int argc, char **argv) {
    const char *src = 0, *out = 0, *target = 0, *pkg = 0;
    int mode = 0, level = 0, explicit_image = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a,"-E")) mode = 1;
        else if (!strcmp(a,"-S") || !strcmp(a,"-c")) mode = 2;
        else if (!strcmp(a,"-b") || !strcmp(a,"-t")) {
            mode = a[1] == 'b' ? 0 : 2; explicit_image = mode == 0;
            if (++i >= argc) return clierror("missing target"); target = argv[i];
        } else if (!strcmp(a,"--models")) {
            if (++i >= argc) return clierror("missing package"); pkg = argv[i];
        } else if (!strcmp(a,"-o")) {
            if (++i >= argc) return clierror("missing output path"); out = argv[i];
        } else if (a[0]=='-' && a[1]=='o' && a[2]) out = a+2;
        else if (!strcmp(a,"-I")) {
            if (++i >= argc) return clierror("missing include path"); INCDIR = argv[i];
        } else if (a[0]=='-' && a[1]=='I' && a[2]) INCDIR = a+2;
        else if (!strcmp(a,"-O")) level = 1;
        else if (!strcmp(a,"-O0")) level = 0;
        else if (!strcmp(a,"-O1")) level = 1;
        else if (!strcmp(a,"-O2")) level = 2;
        else if (a[0]=='-' && a[1]) return clierror("option not migrated");
        else { if (src) return clierror("multiple inputs not migrated"); src = a; }
    }
    if (!src) return clierror("expected a C source file");
    if (!target) target = mode ? "lnx/x86_64" : NATIVE_OS "/" NATIVE_ARCH;
    char route[96];
    int n = mode == 1 ? snprintf(route,sizeof route,"%s/pp",target) :
        snprintf(route,sizeof route,"%s/%s/O%d",target,mode==2 ? "tape" : "image",level);
    if (n < 0 || n >= (int)sizeof route) return clierror("target name too long");
    if (!pkg) pkg = getenv("UNISA_CONTAINER");
    package(pkg ? pkg : argv[0]);
    Buf in = {0}; in.b = !strcmp(src,"-") ? readstream(0,"stdin",&in.n) : readfile(src,&in.n,0);
    int rc = runroute(route,&in,src); unpackage();
    if (rc) return rc;
    if (!out && !mode && !explicit_image) {
#ifdef _WIN32
        out = "a.exe";
#else
        out = "a.out";
#endif
    }
    /* No destination is opened before successful compilation. */
    long fd = out && strcmp(out,"-") ? output_open(out) : 1;
    if (fd < 0) { free(in.b); return clierror("cannot open output"); }
    int done = 0;
    while (done < in.n) {
        long wrote = output_write(fd,in.b+done,in.n-done);
        if (wrote <= 0 || wrote > in.n-done) {
            if (fd != 1) io_close(fd); free(in.b); return clierror("short output write");
        }
        done += (int)wrote;
    }
    free(in.b);
    if (fd != 1 && io_close(fd) < 0) return clierror("cannot close output");
    return 0;
}
