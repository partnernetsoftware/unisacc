/* Native loading of UNIMEM1 byte images. No source/tape/ISA decoding.
   The model supplies relocated bytes, import slots and the entry offset. */
#if !defined(__UNISA__) && !defined(_WIN32)
#include <sys/mman.h>
#endif

typedef struct { int text, extent, stored, entry; } MemoryImage;
typedef struct { unsigned char *base; long size, dataoff; } MemoryMap;

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
        (long)m->text+m->stored+40!=b->n) die("bad memory image bounds");
}
static void memory_map(MemoryImage *m, MemoryMap *x) {
    x->dataoff=((long)m->text+16383)&-16384;
    x->size=x->dataoff+(((long)m->extent+16383)&-16384);
    if (x->size>2147483647) die("memory mapping exceeds relative-address range");
#ifdef _WIN32
    x->base=(unsigned char *)__mmap(0,x->size,0x3000,4,0,0);
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
    if ((long)x->base<0 || !x->base) die("cannot map native memory");
}
static int memory_enter(MemoryMap *x,MemoryImage *plan,Buf *bytes) {
    MemoryImage m; memory_image(bytes,&m);
    if (m.text!=plan->text || m.extent!=plan->extent || m.entry!=plan->entry)
        die("memory layout changed after binding");
    memcpy(x->base,bytes->b+40,m.text);
    memcpy(x->base+x->dataoff,bytes->b+40+m.text,m.stored);
    free(bytes->b); bytes->b=0;
#ifdef _WIN32
    /* The platform gate performs VirtualProtect and FlushInstructionCache. */
    if (__mprotect((long)x->base,x->dataoff,0x20)) die("cannot protect native code");
#else
#ifdef __UNISA__
    if (__mprotect((long)x->base,x->dataoff,5)) die("cannot protect native code");
#else
    __builtin___clear_cache((char *)x->base,(char *)x->base+m.text);
    if (mprotect(x->base,(size_t)x->dataoff,PROT_READ|PROT_EXEC)) die("cannot protect native code");
#endif
#endif
    int (*entry)(long,long)=(int (*)(long,long))(x->base+m.entry);
    return entry(0,0);
}
static void resource_u64(unsigned char *dst,long value) {
    for (int i=0;i<8;i++) { dst[i]=value&255; value=(unsigned long)value>>8; }
}
