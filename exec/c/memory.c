/* Native loading of UNIMEM1 byte images. No source/tape/ISA decoding.
   The model supplies relocated bytes, import slots and the entry offset. */
#if !defined(__UNISA__) && !defined(_WIN32)
#include <sys/mman.h>
#endif
#if defined(_WIN32) && !defined(__UNISA__) && !defined(UNISA_MEMORY_CONTRACT_DOUBLE)
#include <windows.h>
#endif

typedef struct { int text, extent, stored, entry; } MemoryImage;
typedef struct { unsigned char *base; int64_t size, dataoff, reserved; } MemoryMap;

/* Layout arithmetic lives in cx/runtime/memlayout.cx. This file keeps the OS map. */
int mem_field(const unsigned char *, int *);
int mem_image(const unsigned char *, int, int *, int *, int *, int *);
void mem_place(int, int, long long *, long long *);
int mem_span_ok(long long);
int lw_put(unsigned char *, int, unsigned long long);
int mw_flush(int, int);
int mr_apple(int);
int mr_host_at(int);
int mr_host_cli(int);
int mr_host_n(void);
int mr_rows(void);
int mr_rec_len(void);
int mw_image(int);
const char *mw_image_why(int);
const char *mw_map_why(void);
const char *mw_reserve_why(void);
const char *mw_span_why(void);
const char *mw_commit_why(void);
const char *mw_over_why(void);
const char *mw_layout_why(void);
const char *mw_protect_why(void);
const char *mw_lib_protect_why(void);
const char *mw_stack_reserve_why(void);
const char *mw_stack_commit_why(void);
int mm_reserve_size(void);
int mm_map_prot(int, int *);
int mm_map_flags(int, int *);
int mm_map_fd(int, int *);
int mm_reserve_prot(int, int *);
int mm_reserve_flags(int, int *);
int mm_commit_prot(int, int *);
int mm_protect_prot(int, int *);
int mm_base_bad(int, int);
int mm_layout_same(int, int, int, int, int, int);
int mm_commit_over(long long, long long);
#ifdef __UNISA__
static int mm_words(int plat, int kind, int *prot, int *flags, int *fd) {
    int rp, rf, rd;
    if (kind==1) rp=mm_reserve_prot(plat, prot);
    else if (kind==2) rp=mm_commit_prot(plat, prot);
    else rp=mm_map_prot(plat, prot);
    if (kind==1) rf=mm_reserve_flags(plat, flags);
    else rf=mm_map_flags(plat, flags);
    rd=mm_map_fd(plat, fd);
    if (rp==0 && rf==0 && rd==0) return 0;
    return 1;
}
#ifdef _WIN32
#define MM_PLAT 4
#elif defined(__linux__)
#define MM_PLAT 2
#else
#define MM_PLAT 3
#endif
#endif
static void memory_image(Buf *b, MemoryImage *m) {
    int rc=mem_image(b->b, b->n, &m->text, &m->extent, &m->stored, &m->entry);
    int why=mw_image(rc);
    if (why) die(mw_image_why(why));
}
static void memory_apply(MemoryImage *m, MemoryMap *x) {
    long long dataoff, size;
    mem_place(m->text, m->extent, &dataoff, &size);
    x->dataoff=dataoff; x->size=size;
}
static void memory_map(MemoryImage *m, MemoryMap *x) {
    memory_apply(m, x);
    if (!mem_span_ok(x->size)) die(mw_span_why());
#ifdef _WIN32
#ifdef __UNISA__
    { int prot, flags, fd;
      if (mm_words(MM_PLAT,0,&prot,&flags,&fd)) x->base=0;
      else x->base=(unsigned char *)__mmap(0,x->size,prot,flags,fd,0); }
#else
    x->base=VirtualAlloc(0,(SIZE_T)x->size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#endif
#else
#ifdef __UNISA__
    { int prot, flags, fd;
      if (mm_words(MM_PLAT,0,&prot,&flags,&fd)) x->base=0;
      else x->base=(unsigned char *)__mmap(0,x->size,prot,flags,fd,0); }
#else
    x->base=mmap(0,(size_t)x->size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
#endif
#endif
    if (mm_base_bad(x->base==(unsigned char *)(intptr_t)-1, x->base==0)) die(mw_map_why());
}
/* The OS selects the actual base; the model binds its image once. */
static void memory_reserve(MemoryMap *x) {
    x->reserved=mm_reserve_size(); x->size=x->reserved; x->dataoff=0;
#ifdef _WIN32
#ifdef __UNISA__
    { int prot, flags, fd;
      if (mm_words(MM_PLAT,1,&prot,&flags,&fd)) x->base=0;
      else x->base=(unsigned char *)__mmap(0,x->reserved,prot,flags,fd,0); }
#else
    x->base=VirtualAlloc(0,(SIZE_T)x->reserved,MEM_RESERVE,PAGE_NOACCESS);
#endif
#else
#ifdef __UNISA__
    { int prot, flags, fd;
      if (mm_words(MM_PLAT,1,&prot,&flags,&fd)) x->base=0;
      else x->base=(unsigned char *)__mmap(0,x->reserved,prot,flags,fd,0); }
#else
    x->base=mmap(0,(size_t)x->reserved,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
#endif
#endif
    if (mm_base_bad(x->base==(unsigned char *)(intptr_t)-1, x->base==0)) die(mw_reserve_why());
}
static void memory_commit(MemoryImage *m,MemoryMap *x) {
    memory_apply(m, x);
    if (mm_commit_over(x->size, x->reserved)) die(mw_over_why());
#ifdef _WIN32
#ifdef __UNISA__
    int prot, flags, fd; unsigned char *p=0;
    if (mm_words(MM_PLAT,2,&prot,&flags,&fd)==0)
        p=(unsigned char *)__mmap((long)x->base,x->size,prot,flags,fd,0);
#else
    unsigned char *p=VirtualAlloc(x->base,(SIZE_T)x->size,MEM_COMMIT,PAGE_READWRITE);
#endif
    if (mw_commit_base(p==x->base)) die(mw_commit_why());
#else
#ifdef __UNISA__
    int prot;
    if (mm_commit_prot(MM_PLAT,&prot) || __mprotect((long)x->base,x->size,prot)) die(mw_commit_why());
#else
    if (mprotect(x->base,(size_t)x->size,PROT_READ|PROT_WRITE)) die(mw_commit_why());
#endif
#endif
}
static int memory_protect_code(MemoryMap *x,int text) {
#ifdef _WIN32
#ifdef __UNISA__
    { int prot; if (mm_protect_prot(MM_PLAT,&prot)) return 1;
      return __mprotect((long)x->base,x->dataoff,prot); }
#else
    DWORD old;
    if(!VirtualProtect(x->base,(SIZE_T)x->dataoff,PAGE_EXECUTE_READ,&old))return mw_flush(0, 0);
    return mw_flush(1, FlushInstructionCache(GetCurrentProcess(),x->base,(SIZE_T)text) ? 1 : 0);
#endif
#else
#ifdef __UNISA__
    { int prot; if (mm_protect_prot(MM_PLAT,&prot)) return 1;
      return __mprotect((long)x->base,x->dataoff,prot); }
#else
    __builtin___clear_cache((char *)x->base,(char *)x->base+text);
    return mprotect(x->base,(size_t)x->dataoff,PROT_READ|PROT_EXEC);
#endif
#endif
}
static int memory_enter(MemoryMap *x,MemoryImage *plan,Buf *bytes) {
    MemoryImage m; memory_image(bytes,&m);
    if (mm_layout_same(m.text, plan->text, m.extent, plan->extent, m.entry, plan->entry))
        die(mw_layout_why());
    memcpy(x->base,bytes->b+40,m.text);
    memcpy(x->base+x->dataoff,bytes->b+40+m.text,m.stored);
    free(bytes->b); bytes->b=0;
    if(memory_protect_code(x,m.text))die(mw_protect_why());
    int (*entry)(long,long)=(int (*)(long,long))(x->base+m.entry);
    return entry(0,0);
}

