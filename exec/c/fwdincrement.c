/* Compile only the generated forwarding source through E2/E1/E3, then attach
   its tape to the first E3 result. Unsupported tape shapes use the full replay. */
static int fwd_incremental(const char *target,int level,const Buf *base,int unit,Buf *merged) {
    if (getenv("UNISA_FWD_REPLAY") || fwm_overflow) return 0;
    static const char dummy[]="\nint main(void){return 0;}\n";
    if (nfwdsrc<=0 || nfwdsrc>131000) return 0;
    Buf unit_tape={0};
    unit_tape.n=nfwdsrc+(int)sizeof(dummy)-1;
    unit_tape.b=xrealloc(0,(size_t)unit_tape.n+1);
    memcpy(unit_tape.b,fwdsrc,(size_t)nfwdsrc);
    memcpy(unit_tape.b+nfwdsrc,dummy,sizeof(dummy)-1);
    unit_tape.b[unit_tape.n]=0;
    char route[96];
    int rn=snprintf(route,sizeof route,"%s/tape/O%d",target,level);
    if (rn<0 || rn>=(int)sizeof route) { free(unit_tape.b); return 0; }
    int fslot=-1,sslot=-1;
    for (int j=0;j<NRI;j++) {
        if (RI[j].n==16 && !memcmp(RI[j].name,"\0cli/run-forward",16)) fslot=j;
        if (RI[j].n==11 && !memcmp(RI[j].name,"\0cli/source",11)) sslot=j;
    }
    ResourceInput oldf={0},olds={0};
    if (fslot>=0) { oldf=RI[fslot]; RI[fslot].n=0; }
    if (sslot>=0) {
        olds=RI[sslot];
        RI[sslot].data=(const unsigned char *)fwd_path;
        RI[sslot].len=(int)strlen(fwd_path);
    }
    Buf oldattrs=ATTRS; int oldunit=ATTR_UNIT;
    ATTRS.b=0; ATTRS.n=0; ATTRS.cap=0; ATTR_UNIT=0;
    if (ATTR_SLOT>=0) { RI[ATTR_SLOT].data=0; RI[ATTR_SLOT].len=0; }
    int rc=runroute_range(route,NULL,"e1",&unit_tape,fwd_path);
    static const unsigned char funit_one=1;
    if (!rc && NRI<255) {
        RI[NRI].name=(const unsigned char *)"\0cli/funit";
        RI[NRI].n=10; RI[NRI].data=&funit_one; RI[NRI].len=1;
        NRI++;
        rc=runroute_range(route,"e3","e3",&unit_tape,fwd_path);
        NRI--;
    } else rc=1;
    free(ATTRS.b); ATTRS=oldattrs; ATTR_UNIT=oldunit;
    if (ATTR_SLOT>=0) { RI[ATTR_SLOT].data=ATTRS.b; RI[ATTR_SLOT].len=ATTRS.n; }
    if (fslot>=0) RI[fslot]=oldf;
    if (sslot>=0) RI[sslot]=olds;
    if (rc) { free(unit_tape.b); free(unit_tape.at); return -1; }
    int ok=fwm_merge(base,&unit_tape,&unit_tape,unit,merged);
    free(unit_tape.b); free(unit_tape.at);
    return ok ? 1 : 0;
}
