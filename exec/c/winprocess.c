/* OS resource adapter: enumerate named imports of a loaded PE32+ image.
   No compiler import list, instruction rules or source-language decisions.
   The model requests the names it needs. Ordinal imports are not exported. */
static long process_u32(const unsigned char *p) {
    return p[0]|((long)p[1]<<8)|((long)p[2]<<16)|((long)p[3]<<24);
}
static void process_span(long at,long n,long extent) {
    if (at<0 || n<0 || at>extent || n>extent-at) die("process import outside image");
}
static int process_string(const unsigned char *p,long at,long extent) {
    long n=0; process_span(at,1,extent);
    while (at+n<extent && p[at+n]) n++;
    if (at+n==extent || n>4095) die("bad process import name");
    return (int)n;
}
static int process_imports(ResourceInput *items,int capacity,unsigned char *base,long extent) {
    process_span(0,64,extent);
    if (base[0]!='M' || base[1]!='Z') die("not a process PE image");
    long pe=process_u32(base+60);process_span(pe,24+128,extent);
    if (process_u32(base+pe)!=0x4550 || base[pe+24]!=0x0b || base[pe+25]!=2)
        die("not a PE32+ process");
    long d=process_u32(base+pe+24+120);int count=0;
    if (!d) return 0;
    for (;;) {
        process_span(d,20,extent);
        long dll=process_u32(base+d+12);if (!dll) break;
        int dn=process_string(base,dll,extent);
        long ilt=process_u32(base+d),iat=process_u32(base+d+16);
        if (!ilt) die("process has no import-name table");
        for (long k=0;;k+=8) {
            process_span(ilt+k,8,extent);process_span(iat+k,8,extent);
            unsigned char *entry=base+ilt+k;
            long name=process_u32(entry);
            if (!name && !process_u32(entry+4)) break;
            if (entry[7]&128) continue;
            if (process_u32(entry+4)) die("bad process import RVA");
            process_span(name,3,extent);name+=2;
            int nn=process_string(base,name,extent);
            if (count==capacity) die("too many process imports");
            const char *prefix="\0process/import/";int pn=sizeof("\0process/import/")-1;
            unsigned char *key=xrealloc(0,pn+dn+1+nn),*value=xrealloc(0,8);
            memcpy(key,prefix,pn);
            for (int j=0;j<dn;j++) { int c=base[dll+j];key[pn+j]=c>='A'&&c<='Z'?c+32:c; }
            key[pn+dn]='/';memcpy(key+pn+dn+1,base+name,nn);memcpy(value,base+iat+k,8);
            items[count].name=key;items[count].n=pn+dn+1+nn;
            items[count].data=value;items[count].len=8;count++;
        }
        d+=20;
    }
    return count;
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
