/* Native loading of UNIMEM1 byte images. No source/tape/ISA decoding.
   The model supplies relocated bytes, import slots and the entry offset. */
#if !defined(__UNISA__) && !defined(_WIN32)
#include <sys/mman.h>
#endif
#if defined(_WIN32) && !defined(__UNISA__)
#include <windows.h>
#endif

typedef struct { int text, extent, stored, entry; } MemoryImage;
typedef struct { unsigned char *base; int64_t size, dataoff, reserved; } MemoryMap;

static int memory_field(const unsigned char *p) {
    long n=0;
    for (int j=7;j>=0;j--) {
        if (n > (2147483647-p[j])/256) die("memory image extent too large");
        n=n*256+p[j];
    }
    return (int)n;
}
static void memory_image(Buf *b, MemoryImage *m) {
    if (b->n<40 || memcmp(b->b,"UNIMEM1\n",8)) die("bad memory image");
    m->text=memory_field(b->b+8); m->extent=memory_field(b->b+16);
    m->stored=memory_field(b->b+24); m->entry=memory_field(b->b+32);
    if (m->text<=0 || m->entry>=m->text || m->stored>m->extent ||
        (int64_t)m->text+m->stored+40!=b->n) die("bad memory image bounds");
}
static void memory_map(MemoryImage *m, MemoryMap *x) {
    x->dataoff=((int64_t)m->text+16383)&-16384;
    x->size=x->dataoff+(((int64_t)m->extent+16383)&-16384);
    if (x->size>2147483647) die("memory mapping exceeds relative-address range");
#ifdef _WIN32
#ifdef __UNISA__
    x->base=(unsigned char *)__mmap(0,x->size,0x3000,4,0,0);
#else
    x->base=VirtualAlloc(0,(SIZE_T)x->size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#endif
#else
#ifdef __UNISA__
#ifdef __linux__
    x->base=(unsigned char *)__mmap(0,x->size,3,0x22,-1,0);
#else
    x->base=(unsigned char *)__mmap(0,x->size,3,0x1002,-1,0);
#endif
#else
    x->base=mmap(0,(size_t)x->size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
#endif
#endif
    if (x->base==(unsigned char *)(intptr_t)-1 || !x->base) die("cannot map native memory");
}
/* The OS selects the actual base; the model binds its image once. */
static void memory_reserve(MemoryMap *x) {
    x->reserved=2147467264; x->size=x->reserved; x->dataoff=0;
#ifdef _WIN32
#ifdef __UNISA__
    x->base=(unsigned char *)__mmap(0,x->reserved,0x2000,1,0,0);
#else
    x->base=VirtualAlloc(0,(SIZE_T)x->reserved,MEM_RESERVE,PAGE_NOACCESS);
#endif
#else
#ifdef __UNISA__
#ifdef __linux__
    x->base=(unsigned char *)__mmap(0,x->reserved,0,0x22,-1,0);
#else
    x->base=(unsigned char *)__mmap(0,x->reserved,0,0x1002,-1,0);
#endif
#else
    x->base=mmap(0,(size_t)x->reserved,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
#endif
#endif
    if (x->base==(unsigned char *)(intptr_t)-1 || !x->base) die("cannot reserve native memory");
}
static void memory_commit(MemoryImage *m,MemoryMap *x) {
    x->dataoff=((int64_t)m->text+16383)&-16384;
    x->size=x->dataoff+(((int64_t)m->extent+16383)&-16384);
    if (x->size>x->reserved) die("native image exceeds reserved memory");
#ifdef _WIN32
#ifdef __UNISA__
    unsigned char *p=(unsigned char *)__mmap((long)x->base,x->size,0x1000,4,0,0);
#else
    unsigned char *p=VirtualAlloc(x->base,(SIZE_T)x->size,MEM_COMMIT,PAGE_READWRITE);
#endif
    if (p!=x->base) die("cannot commit reserved native memory");
#else
#ifdef __UNISA__
    if (__mprotect((long)x->base,x->size,3)) die("cannot commit reserved native memory");
#else
    if (mprotect(x->base,(size_t)x->size,PROT_READ|PROT_WRITE)) die("cannot commit reserved native memory");
#endif
#endif
}
static int memory_protect_code(MemoryMap *x,int text) {
#ifdef _WIN32
#ifdef __UNISA__
    return __mprotect((long)x->base,x->dataoff,0x20);
#else
    DWORD old;
    if(!VirtualProtect(x->base,(SIZE_T)x->dataoff,PAGE_EXECUTE_READ,&old))return 1;
    return !FlushInstructionCache(GetCurrentProcess(),x->base,(SIZE_T)text);
#endif
#else
#ifdef __UNISA__
    return __mprotect((long)x->base,x->dataoff,5);
#else
    __builtin___clear_cache((char *)x->base,(char *)x->base+text);
    return mprotect(x->base,(size_t)x->dataoff,PROT_READ|PROT_EXEC);
#endif
#endif
}
static int memory_enter(MemoryMap *x,MemoryImage *plan,Buf *bytes) {
    MemoryImage m; memory_image(bytes,&m);
    if (m.text!=plan->text || m.extent!=plan->extent || m.entry!=plan->entry)
        die("memory layout changed after binding");
    memcpy(x->base,bytes->b+40,m.text);
    memcpy(x->base+x->dataoff,bytes->b+40+m.text,m.stored);
    free(bytes->b); bytes->b=0;
    if(memory_protect_code(x,m.text))die("cannot protect native code");
    int (*entry)(long,long)=(int (*)(long,long))(x->base+m.entry);
    return entry(0,0);
}
static void resource_u64(unsigned char *dst,uint64_t value) {
    for (int i=0;i<8;i++) { dst[i]=value&255; value=value>>8; }
}
