/* OS resource adapter: enumerate named imports of a loaded PE32+ image.
   No compiler import list, instruction rules or source-language decisions.
   The model requests the names it needs. Ordinal imports are not exported. */
static long process_u32(const unsigned char *p) {
    return p[0]|((long)p[1]<<8)|((long)p[2]<<16)|((long)p[3]<<24);
}
/* Named-import arithmetic is cx/runtime/peimport.cx. This adapter only maps
   its codes onto the historical die strings and still allocates with xrealloc. */
typedef struct { const unsigned char *name; int n; const unsigned char *data; int len; } PeiItem;
int pei_imports(PeiItem *, int, const unsigned char *, long long, void *(*)(void *, long long));
static void *pei_host_alloc(void *p, long long n) { (void)p; return xrealloc(0, (size_t)n); }
static int process_imports(ResourceInput *items,int capacity,unsigned char *base,long extent) {
    int rc=pei_imports((PeiItem *)items,capacity,base,(long long)extent,pei_host_alloc);
    if (rc>=0) return rc;
    if (rc==-1) die("process import outside image");
    if (rc==-2) die("bad process import name");
    if (rc==-3) die("not a process PE image");
    if (rc==-4) die("not a PE32+ process");
    if (rc==-5) die("process has no import-name table");
    if (rc==-6) die("bad process import RVA");
    die("too many process imports");
    return 0;
}
#ifdef _WIN32
static int process_own_imports(ResourceInput *items,int capacity,long here) {
    /* Loaded code and headers share one reserved image, as in bk_win_imports.
       This walk inspects the current trusted process, not an input PE file. */
    long at=here&-4096;
    while (at>65536) {
        unsigned char *p=(unsigned char *)at;
        if (p[0]=='M' && p[1]=='Z') {
            long pe=process_u32(p+60);
            if (pe<4096 && process_u32(p+pe)==0x4550)
                return process_imports(items,capacity,p,process_u32(p+pe+24+56));
        }
        at-=4096;
    }
    die("cannot find current process image");return 0;
}
#endif
