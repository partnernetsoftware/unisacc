/* Product-internal ABI binding for the separately assembled execution kernel.
   Included by run.c after its byte reader. No language/model rule lives here.
   The immutable executable mapping is retained for this process's lifetime. */
#ifndef __UNISA__
#error The_blob_bridge_requires_the_unisacc_tape_calling_convention
#endif
static unsigned char *kernel_code;
static long kernel_entry;

static long kernel_service(long op,long a,long b,long c,long d) {
    if (op==0) return (long)calloc(a,b);
    if (op==1) return (long)realloc((void *)a,b);
    if (op==2) { free((void *)a); return 0; }
    if (op==3) return (long)memcpy((void *)a,(void *)b,c);
    if (op==4) return memcmp((void *)a,(void *)b,c);
    if (op==5) return (long)memset((void *)a,b,c);
    if (op==6) return strlen((char *)a);
    if (op==7) return core_host_fetch((unsigned char *)a,b,(unsigned char **)c,(int *)d);
    if (op==8) core_host_panic((char *)a);
    die("bad kernel service"); return 0;
}
static int kernel_field(unsigned char *p) {
    long n=0;
    for (int j=7;j>=0;j--) {
        if (n>(2147483647-p[j])/256) die("bad kernel extent");
        n=n*256+p[j];
    }
    return n;
}
static void kernel_load(void) {
    const char *path=getenv("UNISA_KERNEL");
    if (!path || !*path) die("assembly kernel not specified");
    int n; unsigned char *bytes=readfile(path,&n,0);
    if (n<40 || memcmp(bytes,"UNIKERN1",8)) die("bad kernel header");
    int isa=kernel_field(bytes+8),entry=kernel_field(bytes+16);
    int slot=kernel_field(bytes+24),length=kernel_field(bytes+32);
#ifdef __aarch64__
    if (isa!=1) die("wrong kernel ISA");
#else
    if (isa!=2) die("wrong kernel ISA");
#endif
    if (length!=n-40 || entry>=length || slot>length-8 || (slot&7)) die("bad kernel bounds");
    long mapped=((long)length+16383)&-16384;
#ifdef _WIN32
    unsigned char *code=(unsigned char *)__mmap(0,mapped,0x3000,4,0,0);
#else
#ifdef __linux__
    unsigned char *code=(unsigned char *)__mmap(0,mapped,3,0x22,-1,0);
#else
    unsigned char *code=(unsigned char *)__mmap(0,mapped,3,0x1002,-1,0);
#endif
#endif
    if ((long)code<0 || !code || ((long)code&4095)) die("cannot map kernel memory");
    memcpy(code,bytes+40,length); free(bytes);
    *(long *)(code+slot)=(long)kernel_service;
#ifdef _WIN32
    if (__mprotect((long)code,mapped,0x20)) die("cannot protect kernel memory");
#else
    if (__mprotect((long)code,mapped,5)) die("cannot protect kernel memory");
#endif
    kernel_code=code;kernel_entry=entry;
}
static long kernel_call(long *args) {
    if (!kernel_code) kernel_load();
    long (*entry)(long *)=(long (*)(long *))(kernel_code+kernel_entry);
    return entry(args);
}
int core_run(const CoreModel *m,unsigned char *input,int inputn,
             const char *src,I maxsteps,CoreResult *result) {
    long args[7];args[0]=0;
    args[1]=(long)m;args[2]=(long)input;args[3]=inputn;
    args[4]=(long)src;args[5]=maxsteps;args[6]=(long)result;
    return kernel_call(args);
}
const char *core_transition(const CoreModel *m,int q,int key,int *nx,int *sq) {
    long args[7];args[0]=1;
    args[1]=(long)m;args[2]=q;args[3]=key;
    args[4]=(long)nx;args[5]=(long)sq;args[6]=0;
    return (const char *)kernel_call(args);
}
