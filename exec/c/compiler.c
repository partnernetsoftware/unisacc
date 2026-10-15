/* 0.0.21: the compiler itself does not use the Windows host channel (include/sys/_win.h); the
   Python seed and the product lower refuse .hostcall on win until 0.0.22 */
#define _UNISA_NO_HOSTCALL 1
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
#include "tapebin.h"
#include "tapebin_emit.h"
#include "memory.c"
#include "../../src/host_dl.h"
#include "bind.h"
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
/* [TP-7/TP-8] Route dispatch only: validation is the finite model, not C grammar.
   A missing stage must fail closed, including raw -S/-c which bypass the backend. */
static int tape_domain_available(const char *route) {
    for (int i=0;i<PS;i++)
        if (!strcmp(STAGES[i].route,route) && !strcmp(STAGES[i].name,"tapecheck")) return 1;
    return 0;
}
static int tape_domain(const char *route, Buf *in, const char *src) {
    if (!tape_domain_available(route)) return clierror("tape domain: required tapecheck model is missing");
    return runroute_range(route,"tapecheck","tapecheck",in,src);
}
#include "bind.c"

static int tapebin_path(const char *path) {
    size_t n=strlen(path);return n>=8&&!strcmp(path+n-8,".tapebin");
}
static int tape_path(const char *path) {
    size_t n=strlen(path);return n>=5&&!strcmp(path+n-5,".tape");
}
static int tapebin_target(const char *target) {
    static const char *names[]={"lnx/x86_64","lnx/arm64","osx/x86_64","osx/arm64","win/x86_64","win/arm64"};
    for(int i=0;i<6;i++)if(!strcmp(target,names[i]))return i+1;
    return 0;
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
/* Dependency formatting is a driver/file contract, byte for byte gcc -MM's
   (mkdeps.cc): one space between names, " \\\n " once a line would pass
   column 72, make's special characters escaped (space, #, $) in
   prerequisites and -MQ targets, one phony rule per header under -MP.
   -M lists what -MM lists: the bundled headers are not files. The read
   ledger comes from the same host adapter used by C and ASM network execution. */
static int deps_name(FILE *f,const char *name,int quote,int *col) {
    char esc[1100]; int n=0;
    if (quote) {
        for (const char *s=name;*s && n<1090;s++) {
            if (*s==' ' || *s=='#') esc[n++]='\\';
            if (*s=='$') esc[n++]='$';
            esc[n++]=*s;
        }
        esc[n]=0; name=esc;
    } else n=(int)strlen(name);
    int bad=0;
    if (*col) {
        if (*col+n>72) { bad|=fputs(" \\\n ",f)<0; *col=1; }
        else { bad|=fputc(' ',f)<0; *col+=1; }
    }
    bad|=fputs(name,f)<0; *col+=n;
    return bad;
}
static const char *deps_target(const char *out,const char *src,int deponly) {
    static char tname[520];
    if (out && !deponly) return out;
    const char *base=strrchr(src,'/'); base=base ? base+1 : src;
    const char *dot=strrchr(base,'.'); size_t n=dot ? (size_t)(dot-base) : strlen(base);
    if (n>500) n=500;
    memcpy(tname,base,n); tname[n]='.'; tname[n+1]='o'; tname[n+2]=0;
    return tname;
}
static int deps_write(const char *path,const char *target,const char **sources,int count,
                      const char **targets,const int *quoted,int ntargets,int phony) {
    FILE *f=strcmp(path,"-") ? fopen(path,"wb") : stdout;
    if (!f) return clierror("cannot open dependency output");
    int bad=0,col=0;
    if (ntargets) for (int i=0;i<ntargets;i++) bad|=deps_name(f,targets[i],quoted[i],&col);
    else bad|=deps_name(f,target,1,&col);
    bad|=fputc(':',f)<0; col++;
    for (int i=0;i<count;i++) bad|=deps_name(f,sources[i],1,&col);
    for (int i=0;i<FILE_READ_COUNT;i++) bad|=deps_name(f,FILE_READ_PATHS[i],1,&col);
    bad|=fputc('\n',f)<0;
    if (phony) for (int i=0;i<FILE_READ_COUNT;i++) { col=0; bad|=deps_name(f,FILE_READ_PATHS[i],1,&col); bad|=fputs(":\n",f)<0; }
    if (f==stdout) { if (fflush(f)!=0) bad=1; }
    else if (fclose(f)!=0) bad=1;
    return bad ? clierror("cannot write dependency output") : 0;
}
/* 0.0.17 R17-2: the unisacc linker joins -funit unit objects at the tape
   level; the joining is the reference's src/tapelink.c, which only builds a
   tape -- every compilation decision after it is the product route's. */
static Buf linkbuf;
int tl_o(int c) { bput(&linkbuf,(unsigned char)c,0); return 0; }
int tl_fail(char *m) { fputs(m,stderr); exit(1); return 1; }
#define TL_PRODUCT 1
#include "../../src/tapelink.c"
/* 0.0.18 R18-1/R18-2: assembly text (-S -b lnx/ARCH) and `unisacc as` are the
   reference's src/asmtext.c, which reads and writes ELF bytes only */
static Buf asmbuf;
int at_wr(char *p, long n) { for (long k=0;k<n;k++) bput(&asmbuf,(unsigned char)p[k],0); return 0; }
#define AT_PRODUCT 1
#include "../../src/asmtext.c"
/* 0.0.19 R19-10: -run on macOS forwards prototyped undefined functions to the
   host libc.  E3 (with \0cli/run-forward) appends a USLFW1 side-car naming
   them with their USLSIG3 signatures; the stub text is src/fwdstub.c's, the
   same bytes the reference front end writes. */
#include "forwardsignature.h"
#include "../../src/fwdstub.c"
#include "fwdmerge.c"
/* The callback graph carries a C function-pointer type.  Scalar pointees
   are enough for glob's errfunc and similar host APIs; other graphs retain
   the named refusal until their declaration can be printed exactly. */
static const char *fwd_simple_type(const USForwardType *t) {
    if (t->isvoid) return "void";
    if (t->callback || t->structbyval) return 0;
    if (t->kind==1) {
        if (t->depth!=1) return 0;
        switch (t->base) {
            case 0: return "void *";
            case 1: return "char *";
            case 2: return "short *";
            case 4: return "int *";
            case 8: return "long *";
            default: return 0;
        }
    }
    if (t->kind==4) return "float";
    if (t->kind==8) return "double";
    if (t->kind==0) {
        if (t->width==1) return t->uns ? "unsigned char" : "signed char";
        if (t->width==2) return t->uns ? "unsigned short" : "short";
        if (t->width==4) return t->uns ? "unsigned int" : "int";
        if (t->width==8) return t->uns ? "unsigned long" : "long";
    }
    return 0;
}
static int fwd_callback_decl(const USForwardType *t,int index,char *out,size_t cap) {
    USForwardCallback cb={0};
    if (us_forward_callback_decode(t,&cb)) return 1;
    const char *ret=fwd_simple_type(&cb.ret);
    if (!ret) return 1;
    int n=snprintf(out,cap,"%s (*a%d)(",ret,index);
    if (n<0 || (size_t)n>=cap) return 1;
    size_t at=(size_t)n;
    for (int i=0;i<cb.n;i++) {
        const char *type=fwd_simple_type(&cb.args[i]);
        if (!type) return 1;
        n=snprintf(out+at,cap-at,"%s%s",i ? ", " : "",type);
        if (n<0 || (size_t)n>=cap-at) return 1;
        at+=(size_t)n;
    }
    n=snprintf(out+at,cap-at,"%s)",cb.n ? "" : "void");
    return n<0 || (size_t)n>=cap-at;
}
static int fwd_second;                           /* the recompile with the stubs: no side-car */
/* 0.0.30 F2': the per-unit front-end output (unit route: e2, e1) of the first pass, with that unit's
   USLATTR1 records, so the forwarding restart re-runs only the stub unit.  The unit route does not read
   the run-forward resource, so its bytes are the same in both passes; UNISA_FWD_UNITCACHE=0 turns it off. */
static struct { const char *path; unsigned char *b; int n; unsigned char *attr; unsigned nattr; } ucache[256];
static int nucache;
/* F2': units of one multi-source run are independent (the unit cache already replays them), so on
   POSIX hosts the not-yet-cached units compile in forked workers; each writes its bytes and attribute
   payloads to a file and the serial loop consumes them as cache entries.  A worker that fails leaves
   its unit to the serial path, so diagnostics come out exactly as before.  UNISA_JOBS=1 disables. */
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
static int par_get(FILE *f, unsigned *v) { unsigned char q[4]; if (fread(q,1,4,f)!=4) return 0; *v=q[0]|q[1]<<8|q[2]<<16|(unsigned)q[3]<<24; return 1; }
static void par_put(FILE *f, unsigned v) { unsigned char q[4]={v&255,v>>8&255,v>>16&255,v>>24&255}; fwrite(q,1,4,f); }
#endif
static int ucache_find(const char *path) { for (int k=0;k<nucache;k++) if (!strcmp(ucache[k].path,path)) return k; return -1; }
static void ucache_replay(int k) {                /* append unit k's attribute records under ATTR_UNIT */
    if (!ucache[k].nattr) return;
    if (ATTRS.n == 0) { for (int q=0;q<4;q++) attr_put(0); }
    unsigned total = attr_u32(ATTRS.b) + ucache[k].nattr;
    for (unsigned r=0;r<ucache[k].nattr;r++) {
        for (int q=0;q<4;q++) attr_put((ATTR_UNIT >> (8*q)) & 255);
        for (int q=0;q<5;q++) attr_put(ucache[k].attr[5*r+q]);
    }
    for (int q=0;q<4;q++) ATTRS.b[q] = (total >> (8*q)) & 255;
    if (ATTR_SLOT >= 0) { RI[ATTR_SLOT].data = ATTRS.b; RI[ATTR_SLOT].len = ATTRS.n; }
}
static char fwd_path[600];
#include "fwdpaths.h"
static int fwd_sidecar(Buf *in, Buf *tape) {     /* 0: no records (tape set); 1: stubs written; -1: error */
    size_t at=0,n=(size_t)in->n; const unsigned char *b=in->b; uint64_t tl=0,cnt=0;
    if (n<7 || memcmp(b,"USLFW1\n",7)) { fputs("unisacc: error: forwarding side-car missing\n",stderr); return -1; }
    at=7;
    uint64_t ccwc=0;
    if (us_fw_u64(b,n,&at,&tl) || us_fw_u64(b,n,&at,&cnt) || us_fw_u64(b,n,&at,&ccwc) || ccwc>256 || tl>n-at) { fputs("unisacc: error: forwarding side-car malformed\n",stderr); return -1; }
    tape->b=xrealloc(0,tl+1); memcpy(tape->b,b+at,tl); tape->n=(int)tl; at+=tl;
    fwm_reset();
    fwd_ccw=ccwc>0;
    if (cnt==0) return 0;
    nfwdsrc=0;
    for (uint64_t r=0;r<cnt;r++) {
        uint64_t nl=0,sl=0;
        if (us_fw_u64(b,n,&at,&nl) || nl>n-at || nl>1024) return -1;
        const char *nm=(const char *)b+at; at+=nl;
        if (us_fw_u64(b,n,&at,&sl) || sl>n-at) return -1;
        USForwardSig sig; memset(&sig,0,sizeof sig);
        int bad=us_forward_sig_decode(b+at,(size_t)sl,nm,(size_t)nl,&sig); at+=sl;
        if (bad || sig.structbyval || sig.ret.structbyval || sig.n>32) {
            fprintf(stderr,"unisacc: error: undefined function '%.*s'\n",(int)nl,nm); return -1;
        }
        (void)fwm_export((const unsigned char *)nm,(int)nl);
        int kk[33],ww[33],uu[33]; char cbdecl[33][256];
        for (int k=0;k<sig.n;k++) { kk[k]=sig.args[k].kind; ww[k]=sig.args[k].width; uu[k]=0; }
        for (int k=0;k<sig.n;k++) if (sig.args[k].callback) {
            if (fwd_callback_decl(&sig.args[k],k,cbdecl[k],sizeof cbdecl[k])) {
                fwd_pclear();
                fprintf(stderr,"unisacc: error: undefined function '%.*s'\n",(int)nl,nm); return -1;
            }
            fwd_ptype[k]=cbdecl[k];
        }
        int emitted = sig.variadic
            ? fwd_emit_var((char *)nm,(int)nl,sig.n,kk,ww,sig.ret.isvoid,sig.ret.kind,sig.ret.width,sig.ret.uns)
            : fwd_emit((char *)nm,(int)nl,sig.n,kk,ww,uu,sig.ret.isvoid,sig.ret.kind,sig.ret.width,sig.ret.uns);
        fwd_pclear();
        if (!emitted) { fprintf(stderr,"unisacc: error: undefined function '%.*s'\n",(int)nl,nm); return -1; }
    }
    if (nfwdsrc > 0) fwd_emit_libs();
#ifdef _WIN32
    { const char *td=getenv("TEMP"); snprintf(fwd_path,sizeof fwd_path,"%s\\unisacc-forward-%lx.c",td ? td : ".",(unsigned long)(uintptr_t)&at); }
#else
    snprintf(fwd_path,sizeof fwd_path,"/tmp/unisacc-forward-%lx.c",(unsigned long)(uintptr_t)&at);
#endif
    FILE *f=fopen(fwd_path,"wb"); if (!f) return -1;
    int werr=fwrite(fwdsrc,1,nfwdsrc,f)!=(size_t)nfwdsrc; werr|=fclose(f)!=0;
    return werr ? -1 : 1;
}
#include "fwdincrement.c"
static int product_view(int argc,char **argv) {
    int dis=argv[1][0]=='o';
    if (dis ? (argc!=4 || strcmp(argv[2],"-d")) : argc!=3) { fputs(dis ? "usage: unisacc objdump -d FILE.o\n" : "usage: unisacc nm FILE.o\n",stderr); return 2; }
    const char *f=argv[dis ? 3 : 2]; int len=0; unsigned char *b=source_read(f,&len);
    asmbuf.n=0;
    if (dis ? at_dis((char *)b,len) : at_nm((char *)b,len)) { fprintf(stderr,"%s: %s: %s (Linux objects; Mach-O and COFF: 0.0.19)\n",dis ? "objdump" : "nm",f,at_err); free(b); return 1; }
    free(b); fwrite(asmbuf.b,1,asmbuf.n,stdout); return 0;
}
static int product_as(int argc,char **argv) {
    const char *in=0,*out="a.out"; int arch=!strcmp(NATIVE_ARCH,"arm64");
    for (int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"-o") && i+1<argc) out=argv[++i];
        else if (!strcmp(argv[i],"-b") && i+1<argc) {
            const char *t=argv[++i];
            if (!strcmp(t,"lnx/x86_64")) arch=0; else if (!strcmp(t,"lnx/arm64")) arch=1;
            else { fputs("as: targets are lnx/x86_64 and lnx/arm64 (Mach-O and COFF text: 0.0.19)\n",stderr); return 2; }
        }
        else if (argv[i][0]=='-' && argv[i][1]) { fprintf(stderr,"as: unknown option %s\n",argv[i]); return 2; }
        else in=argv[i];
    }
    if (!in) { fputs("usage: unisacc as [-b lnx/x86_64|lnx/arm64] FILE.s [-o FILE.o]\n",stderr); return 2; }
    int len=0; unsigned char *b=source_read(in,&len);
    asmbuf.n=0;
    if (at_asm((char *)b,len,arch)) { fprintf(stderr,"as: %s:%d: %s\n",in,at_lineno,at_err ? at_err : "error"); free(b); return 1; }
    free(b);
    FILE *f=fopen(out,"wb"); if (!f) { fprintf(stderr,"as: cannot write %s\n",out); return 1; }
    int bad=fwrite(asmbuf.b,1,asmbuf.n,f)!=(size_t)asmbuf.n; bad|=fclose(f)!=0;
    return bad;
}
static int object_path(const char *p) {
    size_t n=strlen(p);
    return (n>=2 && (!strcmp(p+n-2,".o") || !strcmp(p+n-2,".a"))) || (n>=4 && !strcmp(p+n-4,".obj"));
}
static int archive_path(const char *p) { size_t n=strlen(p); return n>=2 && !strcmp(p+n-2,".a"); }
static int take_unit(char *t,long tl,int u) {
    if (u==0) { strncpy(tl_first,tl_target,31); tl_first[31]=0; }
    else if (strcmp(tl_first,tl_target)) { fprintf(stderr,"unisacc: error: the objects were compiled for different targets (%s, %s)\n",tl_first,tl_target); return 1; }
    tl_unitver(t,(int)tl); tl_unit(t,(int)tl,u); tl_note_links(t,tl);
    return 0;
}
static int link_objects(const char **paths,int n,Buf *out) {
    int u=0;
    linkbuf.n=0; tl_ng=0; tl_gend=0; tl_nend=0; tl_ndef=0; tl_next=0; tl_ngdef=0;
    for (int p=0;p<n;p++) {                      /* every object is taken */
        if (archive_path(paths[p])) continue;
        int len=0; unsigned char *b=source_read(paths[p],&len); long tl=0;
        char *t=tl_tape((char *)b,len,&tl);
        if (!t) { fprintf(stderr,"unisacc: error: %s carries no unit tape (compile it with -c -b os/arch -funit; objects from other compilers are not linked yet)\n",paths[p]); free(b); return 1; }
        if (take_unit(t,tl,u)) { free(b); return 1; }
        u++; free(b);
    }
    for (int p=0;p<n;p++) {                      /* archives, in order: members that define a wanted name */
        if (!archive_path(paths[p])) continue;
        int len=0; unsigned char *b=source_read(paths[p],&len);
        char *used=calloc(65536,1); int changed=1; char nm[256];
        while (changed) {
            changed=0;
            for (int i=0;i<65536;i++) {
                long ml=0; char *m=tl_ar_member((char *)b,len,i,nm,&ml);
                if (!m) break;
                if (used[i]) continue;
                long tl=0; char *t=tl_tape(m,ml,&tl);
                if (t && tl_needs(t,tl)) { if (take_unit(t,tl,u)) { free(b); free(used); return 1; } u++; used[i]=1; changed=1; }
            }
        }
        free(used); free(b);
    }
    if (tl_check_undef()) return 1;              /* R20-3: shared with the reference linker */
    tl_os("__init:\n",8);
    for (int k=0;k<u;k++) { tl_os("  call __init_u",15); tl_num(k); tl_o(10); }
    tl_os("  ret\n",6);
    *out=linkbuf; memset(&linkbuf,0,sizeof linkbuf);
    return 0;
}
/* `unisacc ar rcs|t|x LIB.a [MEMBER...]` (R17-3): the reference's archive
   code, with stdio for the files */
static int product_ar(int argc,char **argv) {
    if (argc<4) { fputs("usage: unisacc ar rcs|t|x LIB.a [MEMBER...]\n",stderr); return 2; }
    const char *op=argv[2],*lib=argv[3];
    if (op[0]=='r' || op[0]=='q') {
        long cap=8; for (int i=4;i<argc;i++) { int len=0; unsigned char *b=source_read(argv[i],&len); free(b); cap+=len+400+strlen(argv[i]); }
        char *ab=malloc(cap); memcpy(ab,"!<arch>\n",8); long at=8;
        for (int i=4;i<argc;i++) { int len=0; unsigned char *b=source_read(argv[i],&len); at=tl_ar_add(ab,at,argv[i],(char *)b,len); free(b); }
        FILE *f=fopen(lib,"wb"); if (!f) { fprintf(stderr,"ar: cannot write %s\n",lib); return 1; }
        int bad=fwrite(ab,1,at,f)!=(size_t)at; bad|=fclose(f)!=0; free(ab);
        return bad;
    }
    int len=0; unsigned char *b=source_read(lib,&len); char nm[256]; int i=0;
    if (len<8 || memcmp(b,"!<arch>\n",8)) { fputs("ar: not an archive\n",stderr); return 1; }
    for (;;i++) {
        long ml=0; char *m=tl_ar_member((char *)b,len,i,nm,&ml);
        if (!m) break;
        if (op[0]=='t') puts(nm);
        else if (op[0]=='x') { FILE *f=fopen(nm,"wb"); if (!f || fwrite(m,1,ml,f)!=(size_t)ml || fclose(f)) { fprintf(stderr,"ar: cannot write %s\n",nm); return 1; } }
        else { fputs("ar: operation must be r, q, t or x\n",stderr); return 2; }
    }
    free(b); return 0;
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
/* A static compiler image (Linux, built by unisacc) has no host loader in its process, so a run
   whose program forwards to the host libc cannot be bound in memory.  main finishes it as the
   image -o writes (a dynamic ELF) and forward_exec replaces this process with it: argv,
   environment, stdio and exit status are then the program's own.  The image goes to an anonymous
   memory file and is exec'd by descriptor (execveat, AT_EMPTY_PATH), which needs no /proc.
   There is no on-disk fallback: a path in TMPDIR that is written, closed and reopened cannot be
   bound to the bytes that were written, so where memfd is unavailable or may not be executed the
   run is refused and -o is the way. */
#ifdef __linux__
static int fwd_exec;                             /* set once; the stub restart (fwd_second) re-enters main with it.  The CLI
                                                    main is entered once per process: this is not state for a reusable entry point */
#ifdef __aarch64__
#define FX_MEMFD 279
#define FX_EXECVEAT 281
#define FX_CLOSE 57
#define FX_WRITE 64
#define FX_LOADER "/lib/ld-linux-aarch64.so.1"
#else
#define FX_MEMFD 319
#define FX_EXECVEAT 322
#define FX_CLOSE 3
#define FX_WRITE 1
#define FX_LOADER "/lib64/ld-linux-x86-64.so.2"
#endif
static long fx_sys(long nr,long a,long b,long c,long d,long e) {   /* a result, or -errno */
#ifdef __UNISA__
    return __syscall6(nr,a,b,c,d,e);
#else
    long r=syscall(nr,a,b,c,d,e); return r<0 ? -(long)errno : r;
#endif
}
static int fx_fill(long fd,const Buf *image) {
    int done=0;
    while (done<image->n) {
        long wrote=fx_sys(FX_WRITE,fd,(long)(image->b+done),image->n-done,0,0);
        if (wrote==-4) continue;                 /* EINTR */
        if (wrote<=0) return 1;
        done+=(int)wrote;
    }
    return 0;
}
static long fx_exec(long fd,char **argv,char **envp) {
    return fx_sys(FX_EXECVEAT,fd,(long)"",(long)argv,(long)envp,0x1000);   /* AT_EMPTY_PATH */
}
static int forward_exec(const Buf *image,char **argv) {
    long err=0; int noloader=0; char **envp=argv;
    while (*envp) envp++;                        /* the environment follows argv's NULL (runargs) */
    envp++;
    fflush(stdout); fflush(stderr);
    /* MFD_CLOEXEC|MFD_EXEC; kernels before 6.3 reject MFD_EXEC with EINVAL */
    long fd=fx_sys(FX_MEMFD,(long)"unisacc-run",1|16,0,0,0);
    if (fd==-22) fd=fx_sys(FX_MEMFD,(long)"unisacc-run",1,0,0,0);
    if (fd>=0) {
        err=fx_fill(fd,image) ? -5 : fx_exec(fd,argv,envp);   /* returns only on failure */
        noloader=err==-2;                        /* ENOENT by descriptor: the image's PT_INTERP file */
        fx_sys(FX_CLOSE,fd,0,0,0,0);
    }
    else err=fd;
    if (noloader)
        return clierror("this program calls the host C library, and running it needs the host's dynamic loader "
            FX_LOADER ", which could not be opened here (as on a system without glibc); "
            "write the program with -o and run it where glibc is installed");
    fprintf(stderr,"unisacc: model driver: this program calls the host C library, and its image could not be started from an anonymous memory file (errno %ld); write the program with -o and run that\n",-err);
    return 1;
}
#endif

int main(int argc, char **argv) {
    /* S step 1 (opt-in, non-stable CLI): --strict-resource-flow --resource-ledger DIR are taken out of argv first, so the
       option loops below never see the ledger directory as a source. */
    {   int w = 1;
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i],"--strict-resource-flow")) { RF_STRICT = 1; continue; }
            if (!strcmp(argv[i],"--resource-ledger")) {
                char err[512];
                if (i + 1 >= argc) { fprintf(stderr, "unisacc: --resource-ledger needs DIR\n"); return 2; }
                if (rl_load(argv[++i], &RF_LEDGER, err, sizeof err)) { fprintf(stderr, "unisacc: %s\n", err); return 2; }
                RF_LOADED = 1; continue;
            }
            argv[w++] = argv[i];
        }
        argc = w; argv[argc] = 0;
        if (RF_STRICT && !RF_LOADED) { fprintf(stderr, "unisacc: --strict-resource-flow needs --resource-ledger DIR\n"); return 2; }
    }
    const char *src = 0, *out = 0, *target = 0, *pkg = 0, *deps = 0;
    int mode = 0, level = 0, explicit_image = 0, runit = 0, argstart = argc, force_origin=0, emitbin=0;
    int deponly=0, depphony=0, ndeptargets=0, sawc=0, funit=0, defrun=0;
    const char *host_symbols_path=0,*host_provider_path=0;
    const char **deptargets=xrealloc(0,argc*sizeof(char *)); int *depquoted=xrealloc(0,argc*sizeof(int));
    int warnings=0; Buf werror={0}, errorlimit={0};
    const char **sources=xrealloc(0,argc*sizeof(char *)); int nsources=0;
    Buf defs={0}, undefs={0}, forced={0}, incdir={0}, quotedir={0}, nostd={0}, libneed={0}, notrim={0};
    if (argc>=2 && !strcmp(argv[1],"ar")) return product_ar(argc,argv);   /* R17-3 */
    if (argc>=2 && !strcmp(argv[1],"as")) return product_as(argc,argv);   /* R18-1 */
    if (argc>=2 && (!strcmp(argv[1],"nm") || !strcmp(argv[1],"objdump"))) return product_view(argc,argv);   /* R18-4 */
    int sawS=0;
    Buf srcres={0};                   /* \0cli/source: the main source path */
    /* Default mode is RUN (owner 2026-10-01, 0.0.17 R17-10): no mode or output
       flag at all means `-run`; a file is written only on request. */
    { int modeflag=0;
      for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a,"--")) break;
        if (a[0]!='-') continue;
        if (a[1]=='o'||a[1]=='b'||a[1]=='t'||a[1]=='S'||a[1]=='c'||a[1]=='E'||a[1]=='M') modeflag=1;
        if (!strcmp(a,"-run")||!strcmp(a,"-dump-tokens")||!strcmp(a,"--tapebin")||!strcmp(a,"--version")||!strcmp(a,"-version")) modeflag=1;
        if (!strcmp(a,"--models")||!strcmp(a,"-I")||!strcmp(a,"-iquote")||!strcmp(a,"-D")||!strcmp(a,"-U")||!strcmp(a,"-include")||!strcmp(a,"-MF")||!strcmp(a,"-MT")||!strcmp(a,"-MQ")||!strcmp(a,"--emit-host-symbols")||!strcmp(a,"--host-provider")) i++;   /* skip the option's argument */
      }
      if (!modeflag) { runit=1; defrun=1; } }
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a,"--version") || !strcmp(a,"-version")) {
            printf("unisacc %s\n",UNISACC_VERSION); return 0;
        }
        else if (!strcmp(a,"-Wall") || !strcmp(a,"-Wextra")) warnings=1;
        else if (!strcmp(a,"-Werror")) { warnings=1; if (!werror.n) bput(&werror,1,0); }
        else if (!strncmp(a,"-ferror-limit=",14)) { errorlimit.n=0; argbytes(&errorlimit,a+14); }
        else if (!strcmp(a,"-run")) runit = 1;
        else if (!strcmp(a,"--force-origin")) force_origin=1;
        else if (!strcmp(a,"--tapebin")) emitbin=1;
        else if (runit && !strcmp(a,"--")) { argstart=i+1; break; }
        else if (runit && src && a[0]!='-') {
            if(tapebin_path(src)||tape_path(src)){argstart=i;break;}
            size_t len=strlen(a);
            /* .cx script: same as isdotc (src/front_parse.c); break here would turn the rest into argv */
            if ((len>=2 && !strcmp(a+len-2,".c")) || (len>=3 && !strcmp(a+len-3,".cx")) || object_path(a)) sources[nsources++]=a;
            else { argstart=i; break; }
        }
        else if (!strcmp(a,"-MD") || !strcmp(a,"-MMD")) { if (!deps) deps=""; }
        else if (!strcmp(a,"-M") || !strcmp(a,"-MM")) { deponly=1; mode=1; if (!deps) deps=""; }   /* gcc: -E implied, only the .d line */
        else if (!strcmp(a,"-MT") || !strcmp(a,"-MQ")) {
            if (++i>=argc) return clierror("missing dependency target");
            deptargets[ndeptargets]=argv[i]; depquoted[ndeptargets]=a[2]=='Q'; ndeptargets++;
        }
        else if (!strcmp(a,"--emit-host-symbols")) { if(host_symbols_path||++i>=argc)return clierror("invalid --emit-host-symbols argument");host_symbols_path=argv[i]; }
        else if (!strcmp(a,"--host-provider")) { if(host_provider_path||++i>=argc)return clierror("invalid --host-provider argument");host_provider_path=argv[i]; }
        else if (!strcmp(a,"--help")) { puts("unisacc FILE [-S|-c|-o FILE|-run]\nDiagnostic, non-stable: --emit-host-symbols FILE (-S/-c/-o, not -run)\nDiagnostic, non-stable: --host-provider FILE (12-byte capability filter; no addresses)");return 0; }
        else if (!strcmp(a,"-MP")) depphony=1;
        else if (!strcmp(a,"-MF")) {
            if (++i>=argc) return clierror("missing dependency output"); deps=argv[i];
        }
        else if (!strcmp(a,"-nostdinc")) { if (!nostd.n) bput(&nostd,1,0); }
        /* R11-3: library bodies on demand is the model's default; only the opt-out is a resource. */
        else if (!strcmp(a,"-ftrim-libc") || !strcmp(a,"-libneed")) { notrim.n = 0; }
        else if (!strcmp(a,"-fno-trim-libc")) { if (!notrim.n) bput(&notrim,1,0); }
        else if (!strcmp(a,"-dump-tokens")) mode = 4;
        else if (!strcmp(a,"-E")) mode = 1;
        else if (!strcmp(a,"-S") || !strcmp(a,"-c")) { mode = 2; if (a[1]=='c') sawc=1; else sawS=1; }
        else if (!strcmp(a,"-funit")) funit=1;      /* separate compilation (docs/toolchain.md §7) */
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
        else if (!strncmp(a,"-iquote",7)) {
            const char *value=a+7;
            if (!*value) { if (++i >= argc) return clierror("missing quote include path"); value=argv[i]; }
            if (quotedir.n) bput(&quotedir,'\n',0); argbytes(&quotedir,value); quotedir.n--;
        }
        else if (!strcmp(a,"-I")) {
            if (++i >= argc) return clierror("missing include path"); INCDIR = argv[i];
            if (incdir.n) bput(&incdir,'\n',0); argbytes(&incdir,INCDIR); incdir.n--;   /* every -I, in order (R13-0b #22) */
        } else if (a[0]=='-' && a[1]=='I' && a[2]) { INCDIR = a+2;
            if (incdir.n) bput(&incdir,'\n',0); argbytes(&incdir,INCDIR); incdir.n--; }
        /* -l names are retained for host-library forwarding; -L and -x remain
           accepted compatibility options.  None is an input file name. */
        else if (a[0]=='-' && (a[1]=='l' || a[1]=='L' || a[1]=='x')) {
            const char *value = a+2;
            if (!*value) {
                if (++i >= argc) return clierror(defrun && src ? "missing compatibility argument (arguments for the program go after --: unisacc prog.c -- -x)" : "missing compatibility argument");
                value = argv[i];
            }
            if (a[1]=='l' && *value && nfwd_libs < 16 && strcmp(value,"m") && strcmp(value,"c") && strcmp(value,"dl")
                && strcmp(value,"pthread") && strcmp(value,"rt")) fwd_libs[nfwd_libs++] = (char *)value;
        }
        else if (!strcmp(a,"-g") || !strncmp(a,"-std=",5)) {
            /* No separate debug information or dialect switch. */
        }
        else if (!strcmp(a,"-O")) level = 1;
        else if (!strcmp(a,"-O0")) level = 0;
        else if (!strcmp(a,"-O1")) level = 1;
        else if (!strcmp(a,"-O2")) level = 2;
        else if (a[0]=='-' && a[1]) return clierror(defrun && src ? "option not migrated (arguments for the program go after --: unisacc prog.c -- --flag)" : "option not migrated");
        else { sources[nsources++]=a; if (!src) src = a; }
    }
    if (!src) return clierror("expected a C source file");
    /* -c with -b: an object (R17-1).  The product writes objects through its
       own route (\0cli/object, \0cli/funit; exec/enc, in progress) -- until
       that route exists the driver says so instead of writing a tape. */
    /* -c with -b: an object (R17-1).  The Linux targets have the product's
       object route (cdx 7f72206: <os>/<arch>[/warn][/multi]/object/O<n>,
       resources \0cli/object and \0cli/funit); Mach-O and COFF objects are
       still the reference compiler's, and the driver says so by name. */
    int objwant = sawc && target && !object_path(src);
    /* -S -b lnx/ARCH: the object -c -b writes, as GNU assembly; bare -S stays the tape */
    /* ...and only when -o names a .s file; `-S -b T` alone stays the target's tape */
    int asmwant = sawS && target && !object_path(src) && out && strlen(out)>2 && !strcmp(out+strlen(out)-2,".s");
    if (asmwant) {
        if (strncmp(target,"lnx/",4)) { fprintf(stderr,"unisacc: error: assembly text (-S -b ... -o FILE.s) is written for Linux targets in this version (Mach-O and COFF text: 0.0.19); the target was %s\n",target); return 1; }
        objwant=1;
    }
    if (objwant && strncmp(target,"lnx/",4)) return clierror("object output (-c -b) for this target is not on the product route yet (Linux targets are); the reference compiler writes Mach-O and COFF objects");
    if (funit && !objwant) return clierror("-funit needs -c -b os/arch (a unit object)");
    if (objwant) { mode = 5; if (!out) out = deps_target(0,src,1); }
    /* unit objects (-funit): joined into one program tape here, before the
       route is chosen, because the program's target is the objects' */
    int linking=object_path(src); Buf linked={0};
    if (linking) {
        for (int j=0;j<nsources;j++) if (!object_path(sources[j])) return clierror("mix of objects and sources: compile each source with -c -b os/arch -funit first");
        if (link_objects(sources,nsources,&linked)) return 1;
        if (runit && strcmp(tl_first,NATIVE_OS "/" NATIVE_ARCH)) { fprintf(stderr,"unisacc: error: these objects were compiled for another target; running needs this machine's: %s\n",tl_first); return 1; }
        if (target && strcmp(target,tl_first)) { fprintf(stderr,"unisacc: error: -b differs from the target these objects were compiled for: %s\n",tl_first); return 1; }
        if (!target && !runit) { static char tbuf[32]; strcpy(tbuf,tl_first); target=tbuf; }
        nsources=1;
    }
    int isbin=tapebin_path(src), istape=tape_path(src)||linking, tape_input=isbin||istape;
    if(tape_input&&(nsources!=1||mode==1||mode==4))return clierror("tape needs one source and a tape/image/run output");
    if(emitbin&&(mode!=2||runit))return clierror("--tapebin needs -S or -c");
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
#ifdef __linux__
    /* forward_exec, on the stub restart: the same program as an image; no file is written, as in a run */
    int execrun=fwd_exec && runit;
    if (execrun) { runit=0; mode=0; if (!deponly) deps=0; }
#endif
    unsigned host_target=host_target_id(target),host_filter=15;
    if((host_symbols_path||host_provider_path)&&(!host_target||mode==1||mode==4||mode==5||emitbin||asmwant))return clierror("bind: diagnostic option mode/target not supported");
    if(host_symbols_path&&(runit||!strcmp(host_symbols_path,"-")))return clierror("bind: --emit-host-symbols cannot be used with -run or stdout");
    if(host_symbols_path){
        const char *effective_out=out;
        if(!effective_out&&!mode&&!explicit_image&&!runit)effective_out="a.out";
        int same=host_same_output(host_symbols_path,effective_out);
        if(same)return clierror(same>0?"bind: symbol-stream and output are the same path":"bind: cannot-check-symbol-stream path");
        if(deps){
            const char *effective_deps=deps;char depname[4096];
            if(!deps[0]&&deponly)effective_deps=out?out:"-";
            else if(!deps[0]){
                const char *base=out?out:src;int len=(int)strlen(base),dot=len,b=0;
                for(int k=0;k<len;k++){if(base[k]=='/'){dot=len;if(!out)b=k+1;}else if(base[k]=='.')dot=k;}
                if(dot-b+3>(int)sizeof depname)return clierror("bind: dependency path too long");
                memcpy(depname,base+b,(size_t)(dot-b));depname[dot-b]='.';depname[dot-b+1]='d';depname[dot-b+2]=0;effective_deps=depname;
            }
            same=host_same_output(host_symbols_path,effective_deps);
            if(same)return clierror(same>0?"bind: symbol-stream and dependency output are the same path":"bind: cannot-check-symbol-stream path");
        }
    }
    if(host_provider_path&&host_filter_file(host_provider_path,host_target,&host_filter))return 1;
    char route[96];
    int n = mode == 4 ? snprintf(route,sizeof route,"tokens") :
        mode == 1 ? snprintf(route,sizeof route,"%s/pp",target) :
        snprintf(route,sizeof route,"%s/%s%s/O%d",target,nsources>1 ? "multi/" : "",mode==5 ? "object" : mode==3 ? "run" : mode==2 ? "tape" : "image",level);
    if (n < 0 || n >= (int)sizeof route) return clierror("target name too long");
    if (warnings && mode!=1 && mode!=4) {
        n=snprintf(route,sizeof route,"%s/warn/%s%s/O%d",target,nsources>1 ? "multi/" : "",mode==5 ? "object" : mode==3 ? "run" : mode==2 ? "tape" : "image",level);
        if (n<0 || n>=(int)sizeof route) return clierror("target name too long");
    }
    if (!pkg) pkg = getenv("UNISA_CONTAINER");
    ResourceInput cli[256]; memset(cli,0,sizeof cli);int nimports=0;
    unsigned char process_argc[8],process_argv[8],memory_text[8],memory_data[8],process_dl[4][8];
    char **runargs=0;
    if (runit
#ifdef __linux__
        || execrun                               /* forward_exec takes the same vector */
#endif
        ) {
        int count=1+argc-argstart;
        int envn=0; while (process_environment(argc,argv,envn)) envn++;
        runargs=xrealloc(0,(count+envn+2)*sizeof(char *));
        runargs[0]=(char *)src;
        for (int j=argstart;j<argc;j++) runargs[j-argstart+1]=argv[j];
        runargs[count]=0;
        for (int j=0;j<=envn;j++) runargs[count+1+j]=process_environment(argc,argv,j);
        resource_u64(process_argc,count); resource_u64(process_argv,(uintptr_t)runargs);
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
#if defined(__APPLE__) || defined(__linux__)
        static const char *dlkeys[4] = {"\0process/dl/0", "\0process/dl/1", "\0process/dl/2", "\0process/dl/3"};
        for (int j=0;j<4;j++) {
            memset(process_dl[j],0,8); /* host addresses are bound only after lower */
            cli[NRI].name=(const unsigned char *)dlkeys[j]; cli[NRI].n=0;
            cli[NRI].data=process_dl[j]; cli[NRI].len=8; NRI++;
        }
        nimports=4;
#endif
#ifdef _WIN32
        nimports=process_own_imports(cli+9,243,(long)process_own_imports);NRI=9+nimports;
#endif
    }
    /* The MAIN source's path, for the E2 stage to name in a diagnostic (and
       for __FILE__, N17b): E2 is handed token bytes and never sees argv, so
       the name has to travel as a resource like the -I list does.  Same shape:
       the key starts with a NUL byte, `n` is the key length WITHOUT a NUL, and
       the value is the path with no terminator.  It is only meaningful when E2
       reads it, so nothing else has to change.  [N17b prerequisite] */
    { Buf srcbuf={0}; if (src) argbytes(&srcbuf,src); }
    srcres.n=0; if (src) argbytes(&srcres,src);
    ARGRESOURCE(NRI,"\0cli/source",srcres); NRI++;
    ARGRESOURCE(NRI,"\0cli/quote-dir",quotedir); NRI++;
    ARGRESOURCE(NRI,"\0cli/fno-trim-libc",notrim); NRI++;
    ARGRESOURCE(NRI,"\0cli/werror",werror); NRI++;
    static const unsigned char fwd_one=1;
    int fwdwant = (runit || mode==0) && !fwd_second && !tape_input && !linking
        && (!strncmp(target,"osx/",4) || !strncmp(target,"lnx/",4) || !strncmp(target,"win/",4));
    /* 0.0.33 W2: win forwards too (ucrtbase/kernel32/ws2_32 by GetProcAddress, like the reference);
       its hostcall delivers four register arguments, so a wider call is refused by name (src/main.c:26) */
    fwd_maxargs = !strncmp(target,"win/",4) ? 4 : 0;
    if (fwdwant) { cli[NRI].name=(const unsigned char *)"\0cli/run-forward"; cli[NRI].n=16; cli[NRI].data=&fwd_one; cli[NRI].len=1; NRI++; }
    if (fwd_second && fwd_ccw) { cli[NRI].name=(const unsigned char *)"\0cli/ccw"; cli[NRI].n=8; cli[NRI].data=&fwd_one; cli[NRI].len=1; NRI++; }
    int fwd_restart = 0;
    Buf objres={0}, funitres={0};
    if (objwant) bput(&objres,1,0);
    if (funit) bput(&funitres,1,0);
    if (objwant) { ARGRESOURCE(NRI,"\0cli/object",objres); NRI++; }
    if (funit) { ARGRESOURCE(NRI,"\0cli/funit",funitres); NRI++; }
    ARGRESOURCE(NRI,"\0cli/error-limit",errorlimit); NRI++;
    /* 0.0.28 F1: ask E1 for its constructor/destructor side-car; run.c strips it and fills
       \0cli/attributes (empty until a stage writes records) for E3 */
    { static const unsigned char one=1; cli[NRI].name=(const unsigned char *)"\0cli/f1"; cli[NRI].n=7; cli[NRI].data=&one; cli[NRI].len=1; NRI++; }
    free(ATTRS.b); ATTRS.b=0; ATTRS.n=0; ATTRS.cap=0; ATTR_UNIT=0;
    ATTR_SLOT=NRI; cli[NRI].name=(const unsigned char *)"\0cli/attributes"; cli[NRI].n=15; cli[NRI].data=0; cli[NRI].len=0; NRI++;
    /* Target selection is input data; the E2 model chooses its declaration. */
    const char *predefine_target=mode==4 ? "lnx/x86_64" : target;
    cli[NRI].name=(const unsigned char *)"\0cli/target"; cli[NRI].n=11;
    cli[NRI].data=(const unsigned char *)predefine_target;
    cli[NRI].len=(int)strlen(predefine_target); NRI++;

    FILE_READ_RECORD=deps!=0;
    package(pkg ? pkg : argv[0]);
    HostCompileContext host_context={host_target,host_filter,mode,runit,host_symbols_path,cli,process_dl[0]};
    if(host_target&&(mode==0||mode==3||host_symbols_path)&&!host_stage_present("host-symbols","symbols")){unpackage();return clierror("bind: required host-symbols model is missing");}

    if (mode!=1 && mode!=4 && !tape_domain_available(route)) {
        unpackage();
        return clierror("tape domain: required tapecheck model is missing");
    }
    Buf in = {0}; int rc=0;
    if (linking) in=linked;
    else if (nsources==1) {
        in.b = source_read(src,&in.n);
        if(isbin){
            Buf decoded={0};int origin=tapebin_target(target);
            int bad=!origin||tbc_decode(in.b,(size_t)in.n,origin,force_origin,&decoded);
            free(in.b);in.b=0;in.n=0;
            if(bad)return clierror("invalid tapebin or origin target mismatch");
            in=decoded;
        }
    }
    else {
        char unitroute[96]; snprintf(unitroute,sizeof unitroute,"%s/%sunit",target,warnings ? "warn/" : "");
        const char *uc=getenv("UNISA_FWD_UNITCACHE");
        int cacheon = !deps && !(uc && !strcmp(uc,"0"));
#ifndef _WIN32
        const char *uj=getenv("UNISA_JOBS"); int jobs = uj ? atoi(uj) : 8;   /* F4: 8 with a rolling pool */
        if (cacheon && !fwd_second && !warnings && jobs > 1 && nsources > 1 && nucache + nsources <= 256) {
            const char *td=getenv("TMPDIR"); char pf[200]; int me=(int)getpid();
            long base = ATTRS.n ? ATTRS.n : 4;
            /* F4: a rolling pool -- a finished worker's slot takes the next unit at once */
            int pids[256], live=0, next=0; if (jobs>64) jobs=64;
            for (int j=0;j<nsources;j++) pids[j]=0;
            while (next<nsources || live>0) {
                while (next<nsources && live<jobs) {
                    int j=next++; pids[j]=(int)fork();
                    if (pids[j]==0) {
                        if (!freopen("/dev/null","w",stderr)) _exit(1);   /* a failing unit is reported by the serial path, once */
                        Buf unit={0}; ATTR_UNIT=j; unit.b=source_read(sources[j],&unit.n);
                        int r=runroute(unitroute,&unit,sources[j]); if (r) _exit(1);
                        snprintf(pf,sizeof pf,"%s/ua-unit-%d-%d",td&&*td?td:"/tmp",me,j);
                        FILE *f=fopen(pf,"wb"); if (!f) _exit(1);
                        long after = ATTRS.n ? ATTRS.n : 4; unsigned cnt=(unsigned)((after-base)/9);
                        par_put(f,(unsigned)unit.n); fwrite(unit.b,1,unit.n,f); par_put(f,cnt);
                        for (unsigned r2=0;r2<cnt;r2++) fwrite(ATTRS.b+base+9*r2+4,1,5,f);
                        if (fclose(f)) _exit(1);
                        _exit(0);
                    }
                    if (pids[j]>0) live++;
                }
                if (!live) break;
                int st=0, w=(int)waitpid(-1,&st,0), j=-1;
                if (w<=0) break;
                for (int q=0;q<nsources;q++) if (pids[q]==w) { j=q; break; }
                if (j<0) continue;
                live--; pids[j]=0;
                if (!WIFEXITED(st) || WEXITSTATUS(st)) continue;
                snprintf(pf,sizeof pf,"%s/ua-unit-%d-%d",td&&*td?td:"/tmp",me,j);
                FILE *f=fopen(pf,"rb"); if (!f) continue;
                unsigned n=0,cnt=0; int ok=par_get(f,&n);
                unsigned char *b=ok?xrealloc(0,n?n:1):0; if (ok) ok=fread(b,1,n,f)==n && par_get(f,&cnt);
                unsigned char *at=ok?xrealloc(0,cnt?5*cnt:1):0; if (ok) ok=fread(at,1,5*(size_t)cnt,f)==5*(size_t)cnt;
                fclose(f); remove(pf);
                if (!ok || ucache_find(sources[j])>=0) { free(b); free(at); continue; }
                int k=nucache++; ucache[k].path=sources[j]; ucache[k].n=(int)n; ucache[k].b=b; ucache[k].nattr=cnt; ucache[k].attr=at;
            }
        }
        int prefilled = !fwd_second && nucache > 0;
#else
        int prefilled = 0;
#endif
        for (int j=0;j<nsources;j++) {
            Buf unit={0}; ATTR_UNIT=j;
            int hit = cacheon && (fwd_second || prefilled) ? ucache_find(sources[j]) : -1;
            if (hit >= 0) {                   /* the first pass's bytes and records for this unit */
                unit.n=ucache[hit].n; unit.b=xrealloc(0,unit.n?unit.n:1); memcpy(unit.b,ucache[hit].b,unit.n);
                ucache_replay(hit);
            } else {
                long before = ATTRS.n ? ATTRS.n : 4;
                unit.b=source_read(sources[j],&unit.n);
                rc=fwd_second&&!strcmp(sources[j],fwd_path) ? fwd_front(unitroute,0,&unit,sources[j]) : runroute(unitroute,&unit,sources[j]);
                if (rc) { free(unit.b); break; }
                if (cacheon && fwdwant && nucache < 256) {
                    int k=nucache++; ucache[k].path=sources[j];
                    ucache[k].n=unit.n; ucache[k].b=xrealloc(0,unit.n?unit.n:1); memcpy(ucache[k].b,unit.b,unit.n);
                    long after = ATTRS.n ? ATTRS.n : 4; unsigned cnt=(unsigned)((after-before)/9);
                    ucache[k].nattr=cnt; ucache[k].attr=xrealloc(0,cnt?5*cnt:1);
                    for (unsigned r=0;r<cnt;r++) memcpy(ucache[k].attr+5*r, ATTRS.b+before+9*r+4, 5);
                }
            }
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
    if (RF_STRICT || rf_capture_dir()) { int g = rf_identity_gate(in.b,in.n,mode,route); if (g) return g; }   /* SC1 identity, before any stage */
    if (!rc && tape_input) rc=tape_domain(route,&in,src); /* decoded Tapebin uses the same model */
    if (!rc && !(tape_input && mode==2)) {
        if (fwdwant) {
            rc = runroute_range(route,NULL,"e3",&in,src);
            if (!rc) {
                Buf tape={0}; int fr=fwd_sidecar(&in,&tape);
                if (fr<0) rc=1;
                else if (fr==0) { free(in.b); in=tape; rc=tape_domain(route,&in,src); if (!rc) rc=host_compile(route,level ? "e4" : "prune",&in,src,&host_context); }   /* side-car extraction precedes Tape validation */
                else {
#ifdef __linux__
                    if (runit && host_dl_slot(1)==0) {
                        /* Static compiler image: finish as the image route does and exec the result
                           (forward_exec).  Stages up to e3 are the same models on both routes; the
                           run's process and loader resources are withdrawn, as an image has none. */
                        fwd_exec=1; execrun=1; runit=0; mode=0; if (!deponly) deps=0; host_context.native=0;host_context.mode=0;
                        for (int j=5;j<13;j++) cli[j].n=0;
                        n=snprintf(route,sizeof route,"%s/%s%simage/O%d",target,warnings ? "warn/" : "",nsources>1 ? "multi/" : "",level);
                        if (n<0 || n>=(int)sizeof route) return clierror("target name too long");
                    }
#endif
                    Buf merged={0};
                    int fast=fwd_ccw ? 0 : fwd_incremental(target,level,&tape,nsources,&merged);
                    if (fast>0) {
                        free(in.b); free(tape.b); in=merged;
                        rc=tape_domain(route,&in,src);
                        if (!rc) rc=host_compile(route,level ? "e4" : "prune",&in,src,&host_context);
                    } else if (fast<0) { free(tape.b); rc=1; }
                    else { free(tape.b); fwd_restart=1; }
                }
            }
        } else
        rc = host_compile(route,tape_input ? (level ? "e4" : "prune") : 0,&in,src,&host_context);
    }
    if (!rc && fwd_restart) {                    /* compile again with the stubs as the last unit */
        unpackage(); RI=0; NRI=0; ATTR_SLOT=-1;
        char **av=xrealloc(0,(argc+2)*sizeof(char *)); int j=0,put=0;
        for (int i=0;i<argc;i++) { av[j++]=argv[i]; if (!put && argv[i]==src) { av[j++]=fwd_path; put=1; } }
        av[j]=0; fwd_second=1;
        return main(j,av);
    }
    if(!rc&&mode==2&&host_symbols_path){Buf symbols={0};HostRequirements requirements;rc=host_symbol_stream(route,&in,src,&host_context,&symbols,&requirements);if(!rc)rc=host_side_write(host_symbols_path,&symbols);free(symbols.b);}
    MemoryImage plan; MemoryMap mapping;
    if (!rc && runit) {
        snprintf(route,sizeof route,"%s/memory",target);
        const char *twopass=getenv("UNISA_MEMORY_TWOPASS");
        if (!twopass || strcmp(twopass,"1")) {
            unsigned char memory_capacity[8];
            memory_reserve(&mapping);
            resource_u64(memory_text,(uintptr_t)mapping.base);
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
            resource_u64(memory_text,(uintptr_t)mapping.base);resource_u64(memory_data,(uintptr_t)(mapping.base+mapping.dataoff)
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
    unpackage(); RI=0; NRI=0; ATTR_SLOT=-1;
    free(defs.b); free(defs.at); free(undefs.b); free(undefs.at);
    free(forced.b); free(forced.at); free(incdir.b); free(incdir.at);
    free(quotedir.b); free(quotedir.at);
    free(srcres.b); free(srcres.at);
    free(werror.b); free(werror.at);
    if (rc) return rc;
    if(emitbin&&tbc_encode_product(&in,tape_input ? 0 : tapebin_target(target)))
        return clierror("cannot encode tapebin");
    if (deps && (deponly || (mode!=1 && mode!=4 && !runit))) {
        char *name=0;
        if (!deps[0] && deponly) deps=out ? out : "-";       /* gcc: -M goes to -o or stdout */
        else if (!deps[0]) {
            /* -MD: -o with suffix .d, else the input's basename .d in the current directory */
            const char *base=out ? out : src; int len=strlen(base),dot=len,b=0;
            for (int k=0;k<len;k++) { if (base[k]=='/') { dot=len; if (!out) b=k+1; } else if (base[k]=='.') dot=k; }
            name=xrealloc(0,dot-b+3);memcpy(name,base+b,dot-b);name[dot-b]='.';name[dot-b+1]='d';name[dot-b+2]=0;deps=name;
        }
        int drc=deps_write(deps,deps_target(out,src,deponly),sources,nsources,deptargets,depquoted,ndeptargets,depphony);
        free(name);if (drc) return drc;
        if (deponly) { free(sources); free(deptargets); free(depquoted); free(in.b); return 0; }
    }
    free(sources); free(deptargets); free(depquoted);
    if (runit) return memory_enter(&mapping,&plan,&in);
#ifdef __linux__
    if (execrun) return forward_exec(&in,runargs);
#endif
    if (!out && !mode && !explicit_image) {
#ifdef _WIN32
        out = "a.exe";
#else
        out = "a.out";
#endif
    }
    if (asmwant) {
        asmbuf.n=0;
        if (at_dis((char *)in.b,in.n)) { fprintf(stderr,"unisacc: error: -S: %s\n",at_err); free(in.b); return 1; }
        free(in.b); in.b=asmbuf.b; in.n=asmbuf.n; memset(&asmbuf,0,sizeof asmbuf);
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
