/* Product-driver migration: IO and route selection only. All source handling
   is performed by the package's networks. Unsupported CLI remains an explicit
   error until migrated; no call to a reference compiler. Not the default yet. */
#define UNISA_RUNTIME_LIBRARY
#include "run.c"
#undef UNISA_RUNTIME_LIBRARY
#include "memory.c"

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
static void argbytes(Buf *b, const char *s) {
    while (*s) { bput(b,(unsigned char)*s,0); s++; }
    bput(b,0,0);
}
static char *process_environment(int argc,char **argv,int i) {
#ifdef __UNISA__
    /* main's argv is a compiler-created copy; the intrinsic names the
       process vector, which also contains envp after its terminating NULL. */
    return __argv(__argc()+1+i);
#else
    return argv[argc+1+i];
#endif
}
#define ARGRESOURCE(i,k,v) cli[i].name=(const unsigned char *)k; cli[i].n=sizeof(k)-1; cli[i].data=v.b; cli[i].len=v.n

int main(int argc, char **argv) {
    const char *src = 0, *out = 0, *target = 0, *pkg = 0;
    int mode = 0, level = 0, explicit_image = 0, runit = 0, argstart = argc;
    Buf defs={0}, undefs={0}, forced={0}, incdir={0};
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a,"-run")) runit = 1;
        else if (runit && !strcmp(a,"--")) { argstart=i+1; break; }
        else if (runit && src && a[0]!='-') {
            size_t len=strlen(a);
            if (len>=2 && !strcmp(a+len-2,".c")) return clierror("multiple inputs not migrated");
            argstart=i; break;
        }
        else if (!strcmp(a,"-E")) mode = 1;
        else if (!strcmp(a,"-S") || !strcmp(a,"-c")) mode = 2;
        else if (a[0]=='-' && (a[1]=='D' || a[1]=='U')) {
            const char *value=a+2;
            if (!*value) { if (++i >= argc) return clierror("missing macro argument"); value=argv[i]; }
            argbytes(a[1]=='D' ? &defs : &undefs,value);
        } else if (!strcmp(a,"-include")) {
            if (++i >= argc) return clierror("missing forced include"); argbytes(&forced,argv[i]);
        }
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
#ifdef _WIN32
    if (runit) return clierror("native memory imports not migrated on Windows");
#endif
    if (runit) { target=NATIVE_OS "/" NATIVE_ARCH; mode=3; }
    char route[96];
    int n = mode == 1 ? snprintf(route,sizeof route,"%s/pp",target) :
        snprintf(route,sizeof route,"%s/%s/O%d",target,mode==3 ? "run" : mode==2 ? "tape" : "image",level);
    if (n < 0 || n >= (int)sizeof route) return clierror("target name too long");
    if (!pkg) pkg = getenv("UNISA_CONTAINER");
    if (INCDIR) { argbytes(&incdir,INCDIR); incdir.n--; }
    ResourceInput cli[8];
    unsigned char process_argc[8],process_argv[8],memory_text[8],memory_data[8];
    char **runargs=0;
    if (runit) {
        int count=1+argc-argstart;
        int envn=0; while (process_environment(argc,argv,envn)) envn++;
        runargs=xrealloc(0,(count+envn+2)*sizeof(char *));
        runargs[0]=(char *)src;
        for (int j=argstart;j<argc;j++) runargs[j-argstart+1]=argv[j];
        runargs[count]=0;
        for (int j=0;j<=envn;j++) runargs[count+1+j]=process_environment(argc,argv,j);
        resource_u64(process_argc,count); resource_u64(process_argv,(long)runargs);
    }
    ARGRESOURCE(0,"\0cli/defines",defs);
    ARGRESOURCE(1,"\0cli/undefines",undefs);
    ARGRESOURCE(2,"\0cli/includes",forced);
    ARGRESOURCE(3,"\0cli/include-dir",incdir);
    RI=cli; NRI=4;
    if (runit) {
        cli[4].name=(const unsigned char *)"\0process/argc";cli[4].n=13;cli[4].data=process_argc;cli[4].len=8;
        cli[5].name=(const unsigned char *)"\0process/argv";cli[5].n=13;cli[5].data=process_argv;cli[5].len=8;
        NRI=6;
    }
    package(pkg ? pkg : argv[0]);
    Buf in = {0}; in.b = !strcmp(src,"-") ? readstream(0,"stdin",&in.n) : readfile(src,&in.n,0);
    int rc = runroute(route,&in,src);
    MemoryImage plan; MemoryMap mapping;
    if (!rc && runit) {
        snprintf(route,sizeof route,"%s/memory",target);
        Buf first={0};first.n=in.n;first.b=xrealloc(0,in.n);memcpy(first.b,in.b,in.n);
        rc=runroute(route,&first,src);
        if (!rc) {
            memory_image(&first,&plan);memory_map(&plan,&mapping);free(first.b);
            resource_u64(memory_text,(long)mapping.base);resource_u64(memory_data,(long)(mapping.base+mapping.dataoff));
            cli[6].name=(const unsigned char *)"\0memory/text";cli[6].n=12;cli[6].data=memory_text;cli[6].len=8;
            cli[7].name=(const unsigned char *)"\0memory/data";cli[7].n=12;cli[7].data=memory_data;cli[7].len=8;
            NRI=8;rc=runroute(route,&in,src);
        }
    }
    unpackage(); RI=0; NRI=0;
    free(defs.b); free(defs.at); free(undefs.b); free(undefs.at);
    free(forced.b); free(forced.at); free(incdir.b); free(incdir.at);
    if (rc) return rc;
    if (runit) return memory_enter(&mapping,&plan,&in);
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
