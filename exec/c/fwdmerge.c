/* Incrementally attach the generated host-forwarding unit to the first E3 tape.
   The stub is compiled alone with a throwaway main; only its definitions,
   initializer and string rows survive.  A shape we cannot prove safe falls
   back to the original full E3 replay in compiler.c. */
typedef struct { const unsigned char *p; int n; } FwmLine;
typedef struct { const unsigned char *p; int n; } FwmName;
static char *fwm_exports[512];
static int fwm_nexports;
static int fwm_overflow;
static void fwm_reset(void) {
    for (int i=0;i<fwm_nexports;i++) free(fwm_exports[i]);
    fwm_nexports=0; fwm_overflow=0;
}
static int fwm_export(const unsigned char *p,int n) {
    if (n<=0 || n>1024 || fwm_nexports>=512) { fwm_overflow=1; return 1; }
    char *s=xrealloc(0,(size_t)n+1); memcpy(s,p,n); s[n]=0;
    fwm_exports[fwm_nexports++]=s; return 0;
}
static int fwm_isid(int c) {
    return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='$';
}
static int fwm_eq(FwmLine l,const char *s) {
    int n=(int)strlen(s); return l.n==n && !memcmp(l.p,s,(size_t)n);
}
static int fwm_prefix(FwmLine l,const char *s) {
    int n=(int)strlen(s); return l.n>=n && !memcmp(l.p,s,(size_t)n);
}
static int fwm_lines(const Buf *b,FwmLine **out) {
    int cap=256,count=0,at=0; FwmLine *v=xrealloc(0,(size_t)cap*sizeof *v);
    while (at<b->n) {
        int end=at; while (end<b->n && b->b[end]!='\n') end++;
        if (end==b->n) { free(v); return -1; }
        if (count==cap) { cap*=2; v=xrealloc(v,(size_t)cap*sizeof *v); }
        v[count].p=b->b+at; v[count].n=end-at; count++; at=end+1;
    }
    *out=v; return count;
}
static int fwm_nameeq(FwmName a,const unsigned char *p,int n) {
    return a.n==n && !memcmp(a.p,p,(size_t)n);
}
static int fwm_hasname(FwmName *v,int count,const unsigned char *p,int n) {
    for (int i=0;i<count;i++) if (fwm_nameeq(v[i],p,n)) return 1;
    return 0;
}
static int fwm_isexport(const unsigned char *p,int n) {
    for (int i=0;i<fwm_nexports;i++)
        if ((int)strlen(fwm_exports[i])==n && !memcmp(fwm_exports[i],p,(size_t)n)) return 1;
    return 0;
}
static int fwm_addname(FwmName *v,int *count,const unsigned char *p,int n) {
    if (n<=0 || n>1024 || *count>=4096) return 1;
    for (int i=0;i<*count;i++) if (fwm_nameeq(v[i],p,n)) return 0;
    v[*count].p=p; v[*count].n=n; (*count)++; return 0;
}
static int fwm_number(const unsigned char *p,int n,int prefix,int *value) {
    if (n<=prefix) return 0;
    int x=0;
    for (int i=prefix;i<n;i++) {
        if (p[i]<'0'||p[i]>'9'||x>1000000) return 0;
        x=x*10+p[i]-'0';
    }
    *value=x; return 1;
}
static void fwm_put(Buf *o,const unsigned char *p,int n) {
    if (n<=0) return;
    if (o->n+n>o->cap) {
        int cap=o->cap ? o->cap : 1024;
        while (cap<o->n+n) { if (cap>0x3fffffff) die("forward tape too large"); cap*=2; }
        o->b=xrealloc(o->b,(size_t)cap); o->cap=cap;
    }
    memcpy(o->b+o->n,p,(size_t)n); o->n+=n;
}
static void fwm_text(Buf *o,const char *s) { fwm_put(o,(const unsigned char *)s,(int)strlen(s)); }
static void fwm_int(Buf *o,int x) {
    char d[32]; int n=snprintf(d,sizeof d,"%d",x);
    if (n<=0 || n>=(int)sizeof d) die("forward tape number");
    fwm_put(o,(const unsigned char *)d,n);
}
static int fwm_rewrite(Buf *o,FwmLine l,FwmName *locals,int nlocals,int labshift,int strshift,int unit) {
    int start=0,end=l.n;
    if (fwm_prefix(l,"  .sys ") || fwm_prefix(l,"  .sys6 ")) {
        while (start<l.n && l.p[start]!=',') start++;
        if (start==l.n) return 1;
        start++;
        fwm_put(o,l.p,start);
    }
    if (fwm_prefix(l,".str ")) {
        for (int i=start;i<l.n;i++) if (l.p[i]=='"') { end=i; break; }
    }
    for (int i=start;i<end;) {
        if (!fwm_isid(l.p[i])) { fwm_put(o,l.p+i,1); i++; continue; }
        int j=i+1; while (j<end && fwm_isid(l.p[j])) j++;
        const unsigned char *p=l.p+i; int n=j-i,v=0,done=0;
        if (n>11 && !memcmp(p,"__unisacc_",10) && (p[10]=='L'||p[10]=='R') && fwm_number(p,n,11,&v)) {
            fwm_put(o,p,11); fwm_int(o,v+labshift); done=1;
        } else if (p[0]=='S' && fwm_number(p,n,1,&v)) {
            fwm_text(o,"S"); fwm_int(o,v+strshift); done=1;
        } else if (n>2 && p[0]=='l' && p[1]=='s' && fwm_number(p,n,2,&v)) {
            fwm_text(o,"ls"); fwm_int(o,v+2097152*unit); done=1;
        } else {
            for (int k=0;k<nlocals;k++) if (fwm_nameeq(locals[k],p,n)) {
                fwm_put(o,p,n); fwm_text(o,"__u"); fwm_int(o,unit); done=1; break;
            }
        }
        if (!done) fwm_put(o,p,n);
        i=j;
    }
    if (end<l.n) fwm_put(o,l.p+end,l.n-end);
    fwm_text(o,"\n"); return 0;
}
static void fwm_line(Buf *o,FwmLine l) { fwm_put(o,l.p,l.n); fwm_text(o,"\n"); }
static int fwm_merge(const Buf *base,const Buf *stub,const Buf *unit_tape,int unit,Buf *out) {
    FwmLine *a=0,*b=0,*u=0; FwmName *locals=0,*globals=0;
    int na=fwm_lines(base,&a),nb=fwm_lines(stub,&b),nu=fwm_lines(unit_tape,&u);
    int nglobals=0;
    int ok=0,body=-1,main=-1,sinit=-1,sret=-1,ainit=-1,aret=-1,lastret=-1;
    if (na<0 || nb<0 || nu<0 || !fwm_nexports || fwm_overflow) goto done;
    if (nu<1 || !fwm_eq(u[0],".unit 2")) goto done;
    globals=xrealloc(0,4096*sizeof *globals);
    for (int i=0;i<nu;i++) if (fwm_prefix(u[i],".global ")) {
        const unsigned char *p=u[i].p+8; int n=u[i].n-8;
        if (fwm_addname(globals,&nglobals,p,n)) goto done;
    }
    body=1;
    for (int i=0;i<nb;i++) {
        if (main<0 && fwm_eq(b[i],"main:")) main=i;
        if (sinit<0 && fwm_eq(b[i],"__init_u:")) sinit=i;
        if (sret<0 && fwm_eq(b[i],"__main_ret:")) sret=i;
    }
    for (int i=0;i<na;i++) {
        if (ainit<0 && fwm_eq(a[i],"__init:")) ainit=i;
        if (aret<0 && fwm_eq(a[i],"__main_ret:")) aret=i;
    }
    if (!(0<body && body<main && main<sinit && sinit<sret && sret+1<nb && 0<ainit && ainit<aret)) goto done;
    if (!fwm_eq(b[sret-1],"  ret")) goto done;
    for (int i=ainit+1;i<aret;i++) if (fwm_eq(a[i],"  ret")) lastret=i;
    if (lastret<0) goto done;
    int insert_at=lastret;
    while (insert_at>ainit+1 && fwm_prefix(a[insert_at-1],"  call ")) insert_at--;
    int stub_calls=sret-1;
    while (stub_calls>sinit+1 && fwm_prefix(b[stub_calls-1],"  call ")) stub_calls--;
    if (!fwm_eq(b[sret+1],"  .exit r0") &&
        !(fwm_eq(b[sret+1],"  call exit") && sret+2<nb && fwm_eq(b[sret+2],"  .exit r0"))) goto done;
    for (int i=sret+1;i<nb;i++) if (!fwm_prefix(b[i],".str ") &&
        !fwm_eq(b[i],"  .exit r0") && !fwm_eq(b[i],"  call exit") &&
        !fwm_eq(b[i],".extern __init")) goto done;
    int nlocals=0,labmax=0,strmax=-1;
    locals=xrealloc(0,4096*sizeof *locals);
    for (int i=body;i<main;i++) {
        if (fwm_prefix(b[i],".global ") || fwm_prefix(b[i],".extern ") ||
            fwm_prefix(b[i],".gdef ")) continue;
        const unsigned char *p=0; int n=0;
        if (fwm_prefix(b[i],".bss ")) {
            p=b[i].p+5; while (5+n<b[i].n && fwm_isid(p[n])) n++;
        } else if (b[i].n>1 && b[i].p[0]!=' ' && b[i].p[b[i].n-1]==':') {
            p=b[i].p; n=b[i].n-1;
        }
        if (p && n && !fwm_isexport(p,n) && !fwm_hasname(globals,nglobals,p,n) &&
            !(n>11 && !memcmp(p,"__unisacc_",10)))
            if (fwm_addname(locals,&nlocals,p,n)) goto done;
    }
    for (int i=0;i<na;i++) {
        int quoted=fwm_prefix(a[i],".str "),inquote=0;
        for (int j=0;j<a[i].n;) {
            if (quoted && a[i].p[j]=='"') inquote=1;
            if (inquote || !fwm_isid(a[i].p[j])) { j++; continue; }
            int k=j+1; while (k<a[i].n && fwm_isid(a[i].p[k])) k++;
            const unsigned char *p=a[i].p+j; int n=k-j,v=0;
            if (n>11 && !memcmp(p,"__unisacc_",10) && (p[10]=='L'||p[10]=='R') && fwm_number(p,n,11,&v) && v>labmax) labmax=v;
            if (p[0]=='S' && fwm_number(p,n,1,&v) && v>strmax) strmax=v;
            j=k;
        }
    }
    if (labmax>1000000 || strmax>1000000) goto done;
    for (int i=0;i<na;i++) {
        if (i==ainit) for (int j=body;j<main;j++) {
            if (fwm_prefix(b[j],".global ") || fwm_prefix(b[j],".extern ") ||
                fwm_prefix(b[j],".gdef ")) continue;
            if (fwm_rewrite(out,b[j],locals,nlocals,labmax,strmax+1,unit)) goto done;
        }
        if (i==insert_at) for (int j=sinit+1;j<stub_calls;j++)
            if (fwm_rewrite(out,b[j],locals,nlocals,labmax,strmax+1,unit)) goto done;
        if (i==lastret) for (int j=stub_calls;j<sret-1;j++)
            if (fwm_rewrite(out,b[j],locals,nlocals,labmax,strmax+1,unit)) goto done;
        fwm_line(out,a[i]);
    }
    for (int j=sret+1;j<nb;j++) if (fwm_prefix(b[j],".str "))
        if (fwm_rewrite(out,b[j],locals,nlocals,labmax,strmax+1,unit)) goto done;
    ok=1;
done:
    free(a); free(b); free(u); free(locals); free(globals);
    if (!ok) { free(out->b); memset(out,0,sizeof *out); }
    return ok;
}
