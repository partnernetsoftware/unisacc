/* Product-driver migration: IO and route selection only. All source handling
   is performed by the package's networks. Unsupported CLI remains an explicit
   error until migrated; no call to a reference compiler. Not the default yet. */
#include "../../src/version.h"
/* Match the existing selfcheck fuel budget; explicit environment wins. */
#ifndef UNISA_DEFAULT_MAXSTEPS
#define UNISA_DEFAULT_MAXSTEPS 400000000000LL
#endif
#define UNISA_RUNTIME_LIBRARY
#include "run.c"
#undef UNISA_RUNTIME_LIBRARY
#include "memory.c"
#include "../../src/host_dl.h"
#ifdef _WIN32
#include "winprocess.c"
#endif

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
/* Source IO has the public compiler diagnostic; package/resource IO keeps
   the runtime diagnostic. No source interpretation happens in this helper. */
static unsigned char *source_read(const char *path, int *len) {
    if (!strcmp(path,"-")) return readstream(0,"stdin",len);
    long fd=io_open(path);
    if (fd < 0) {
        fprintf(stderr,"unisacc: error: cannot open %s\n",path);
        exit(1);
    }
    unsigned char *bytes=readstream(fd,path,len);
    if (io_close(fd)<0) die("close failed");
    return bytes;
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
/* Dependency formatting is a driver/file contract. The read ledger comes
   from the same host adapter used by C and ASM network execution. */
static int deps_write(const char *path,const char *target,const char **sources,int count) {
    FILE *f=fopen(path,"wb");
    if (!f) return clierror("cannot open dependency output");
    int bad=fprintf(f,"%s:",target)<0;
    for (int i=0;i<count;i++) if (fprintf(f," \\\n  %s",sources[i])<0) bad=1;
    for (int i=0;i<FILE_READ_COUNT;i++) if (fprintf(f," \\\n  %s",FILE_READ_PATHS[i])<0) bad=1;
    if (fwrite("\n",1,1,f)!=1) bad=1;
    if (fclose(f)!=0) bad=1;
    return bad ? clierror("cannot write dependency output") : 0;
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
    const char *src = 0, *out = 0, *target = 0, *pkg = 0, *deps = 0;
    int mode = 0, level = 0, explicit_image = 0, runit = 0, argstart = argc;
    int warnings=0; Buf werror={0}, errorlimit={0};
    const char **sources=xrealloc(0,argc*sizeof(char *)); int nsources=0;
    Buf defs={0}, undefs={0}, forced={0}, incdir={0}, nostd={0}, libneed={0};
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a,"--version") || !strcmp(a,"-version")) {
            printf("unisacc %s\n",UNISACC_VERSION); return 0;
        }
        else if (!strcmp(a,"-Wall") || !strcmp(a,"-Wextra")) warnings=1;
        else if (!strcmp(a,"-Werror")) { warnings=1; if (!werror.n) bput(&werror,1,0); }
        else if (!strncmp(a,"-ferror-limit=",14)) { errorlimit.n=0; argbytes(&errorlimit,a+14); }
        else if (!strcmp(a,"-run")) runit = 1;
        else if (runit && !strcmp(a,"--")) { argstart=i+1; break; }
        else if (runit && src && a[0]!='-') {
            size_t len=strlen(a);
            if (len>=2 && !strcmp(a+len-2,".c")) sources[nsources++]=a;
            else { argstart=i; break; }
        }
        else if (!strcmp(a,"-MD") || !strcmp(a,"-MMD")) { if (!deps) deps=""; }
        else if (!strcmp(a,"-MF")) {
            if (++i>=argc) return clierror("missing dependency output"); deps=argv[i];
        }
        else if (!strcmp(a,"-nostdinc")) { if (!nostd.n) bput(&nostd,1,0); }
        else if (!strcmp(a,"-ftrim-libc") || !strcmp(a,"-libneed")) { if (!libneed.n) bput(&libneed,1,0); }
        else if (!strcmp(a,"-dump-tokens")) mode = 4;
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
        /* Same build-system compatibility as the reference product: no
           external linker/search path, and only the C language. These
           arguments are consumed, not interpreted as input file names. */
        else if (a[0]=='-' && (a[1]=='l' || a[1]=='L' || a[1]=='x')) {
            if (!a[2] && ++i >= argc) return clierror("missing compatibility argument");
        }
        else if (!strcmp(a,"-g") || !strncmp(a,"-std=",5)) {
            /* No separate debug information or dialect switch. */
        }
        else if (!strcmp(a,"-O")) level = 1;
        else if (!strcmp(a,"-O0")) level = 0;
        else if (!strcmp(a,"-O1")) level = 1;
        else if (!strcmp(a,"-O2")) level = 2;
        else if (a[0]=='-' && a[1]) return clierror("option not migrated");
        else { sources[nsources++]=a; if (!src) src = a; }
    }
    if (!src) return clierror("expected a C source file");
    if (nsources>1 && (mode==1 || mode==4)) return clierror("multiple preprocessing outputs not migrated");
#ifdef UNISA_SINGLE_TARGET
    if (target && strcmp(target,UNISA_SINGLE_TARGET))
        return clierror("target is outside this single-target compiler");
    target = UNISA_SINGLE_TARGET;
    if (runit) mode=3;
#else
    if (!target) target = mode ? "lnx/x86_64" : NATIVE_OS "/" NATIVE_ARCH;
    if (runit) { target=NATIVE_OS "/" NATIVE_ARCH; mode=3; }
#endif
    char route[96];
    int n = mode == 4 ? snprintf(route,sizeof route,"tokens") :
        mode == 1 ? snprintf(route,sizeof route,"%s/pp",target) :
        snprintf(route,sizeof route,"%s/%s%s/O%d",target,nsources>1 ? "multi/" : "",mode==3 ? "run" : mode==2 ? "tape" : "image",level);
    if (n < 0 || n >= (int)sizeof route) return clierror("target name too long");
    if (warnings && mode!=1 && mode!=4) {
        n=snprintf(route,sizeof route,"%s/warn/%s%s/O%d",target,nsources>1 ? "multi/" : "",mode==3 ? "run" : mode==2 ? "tape" : "image",level);
        if (n<0 || n>=(int)sizeof route) return clierror("target name too long");
    }
    if (!pkg) pkg = getenv("UNISA_CONTAINER");
    if (INCDIR) { argbytes(&incdir,INCDIR); incdir.n--; }
    ResourceInput cli[256]; memset(cli,0,sizeof cli);int nimports=0;
    unsigned char process_argc[8],process_argv[8],memory_text[8],memory_data[8],process_dl[4][8];
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
    ARGRESOURCE(4,"\0cli/nostdinc",nostd);
    RI=cli; NRI=5;
    if (runit) {
        cli[5].name=(const unsigned char *)"\0process/argc";cli[5].n=13;cli[5].data=process_argc;cli[5].len=8;
        cli[6].name=(const unsigned char *)"\0process/argv";cli[6].n=13;cli[6].data=process_argv;cli[6].len=8;
        NRI=9;
#ifdef __APPLE__
        static const char *dlkeys[4] = {"\0process/dl/0", "\0process/dl/1", "\0process/dl/2", "\0process/dl/3"};
        for (int j=0;j<4;j++) {
            resource_u64(process_dl[j],host_dl_slot(j));
            cli[NRI].name=(const unsigned char *)dlkeys[j]; cli[NRI].n=13;
            cli[NRI].data=process_dl[j]; cli[NRI].len=8; NRI++;
        }
        nimports=4;
#endif
#ifdef _WIN32
        nimports=process_own_imports(cli+9,243,(long)process_own_imports);NRI=9+nimports;
#endif
    }
    ARGRESOURCE(NRI,"\0cli/ftrim-libc",libneed); NRI++;
    ARGRESOURCE(NRI,"\0cli/werror",werror); NRI++;
    ARGRESOURCE(NRI,"\0cli/error-limit",errorlimit); NRI++;
    /* Target selection is input data; the E2 model chooses its declaration. */
    const char *predefine_target=mode==4 ? "lnx/x86_64" : target;
    cli[NRI].name=(const unsigned char *)"\0cli/target"; cli[NRI].n=11;
    cli[NRI].data=(const unsigned char *)predefine_target;
    cli[NRI].len=(int)strlen(predefine_target); NRI++;

    FILE_READ_RECORD=deps!=0;
    package(pkg ? pkg : argv[0]);
    Buf in = {0}; int rc=0;
    if (nsources==1) in.b = source_read(src,&in.n);
    else {
        char unitroute[96]; snprintf(unitroute,sizeof unitroute,"%s/%sunit",target,warnings ? "warn/" : "");
        for (int j=0;j<nsources;j++) {
            Buf unit={0}; unit.b=source_read(sources[j],&unit.n);
            rc=runroute(unitroute,&unit,sources[j]);
            if (rc) { free(unit.b); break; }
            int namelen=strlen(sources[j]);
            long framed=(long)unit.n+4+(long)namelen;
            if (framed>0x7fffffff) return clierror("unit frame too large");
            for (int k=0;k<4;k++) bput(&in,(framed>>(8*k))&255,0);
            {
                for (int k=0;k<4;k++) bput(&in,(namelen>>(8*k))&255,0);
                for (int k=0;k<namelen;k++) bput(&in,(unsigned char)sources[j][k],0);
            }
            for (int k=0;k<unit.n;k++) bput(&in,unit.b[k],0);
            free(unit.b);
        }
    }
    free(in.at); in.at=0; /* Framing positions are not input to model inference. */
    if (!rc) rc = runroute(route,&in,src);
    MemoryImage plan; MemoryMap mapping;
    if (!rc && runit) {
        snprintf(route,sizeof route,"%s/memory",target);
        const char *twopass=getenv("UNISA_MEMORY_TWOPASS");
        if (!twopass || strcmp(twopass,"1")) {
            unsigned char memory_capacity[8];
            memory_reserve(&mapping);
            resource_u64(memory_text,(long)mapping.base);
            resource_u64(memory_capacity,mapping.reserved);
            cli[7].name=(const unsigned char *)"\0memory/text";cli[7].n=12;cli[7].data=memory_text;cli[7].len=8;
            cli[8].name=(const unsigned char *)"\0memory/reserve";cli[8].n=15;cli[8].data=memory_capacity;cli[8].len=8;
            rc=runroute(route,&in,src);
            if (!rc) { memory_image(&in,&plan);memory_commit(&plan,&mapping); }
        } else {
        Buf first={0};first.n=in.n;first.b=xrealloc(0,in.n);memcpy(first.b,in.b,in.n);
        rc=runroute(route,&first,src);
        if (!rc) {
            memory_image(&first,&plan);memory_map(&plan,&mapping);free(first.b);
            resource_u64(memory_text,(long)mapping.base);resource_u64(memory_data,(long)(mapping.base+mapping.dataoff)
#ifdef __APPLE__
                +32
#endif
            );
            cli[7].name=(const unsigned char *)"\0memory/text";cli[7].n=12;cli[7].data=memory_text;cli[7].len=8;
            cli[8].name=(const unsigned char *)"\0memory/data";cli[8].n=12;cli[8].data=memory_data;cli[8].len=8;
            rc=runroute(route,&in,src);
        }
        }
    }
    unpackage(); RI=0; NRI=0;
    free(defs.b); free(defs.at); free(undefs.b); free(undefs.at);
    free(forced.b); free(forced.at); free(incdir.b); free(incdir.at);
    free(werror.b); free(werror.at);
    if (rc) return rc;
    if (deps && mode!=1 && mode!=4 && !runit) {
        char *name=0;
        if (!deps[0]) {
            const char *base=out ? out : src; int len=strlen(base),dot=len;
            for (int k=0;k<len;k++) { if (base[k]=='/') dot=len; else if (base[k]=='.') dot=k; }
            name=xrealloc(0,dot+3);memcpy(name,base,dot);name[dot]='.';name[dot+1]='d';name[dot+2]=0;deps=name;
        }
        int drc=deps_write(deps,out ? out : "a.out",sources,nsources);
        free(name);if (drc) return drc;
    }
    free(sources);
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
