/* Exercise the actual ledger loader with CX cleanup and an injected domain. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
static void *live[64];static int count,calls,fail_at,invalid;
static void *tracked_resize(void *p,size_t n) {
    if (++calls==fail_at) return 0;
    int i=0;if(p){while(i<count&&live[i]!=p)i++;if(i==count){invalid++;return 0;}}
    void *q=realloc(p,n?n:1);if(!q)return 0;
    if(p)live[i]=q;else live[count++]=q;return q;
}
static void tracked_release(void *p) {
    if(!p)return;int i=0;while(i<count&&live[i]!=p)i++;
    if(i==count){invalid++;return;}free(p);live[i]=live[--count];
}
#define realloc tracked_resize
#define free tracked_release
#define UNISA_CX_SOURCE_FRAME
#include "../../exec/c/resourceledger.h"
#undef free
#undef realloc
typedef struct {unsigned char *key;int kn,present;unsigned char *value;int vn;char blob[64];} RowShape;
typedef struct {RowShape *rows;int n,has_id,trace,cursor;char stage[128],input_hex[65];} LedgerShape;
_Static_assert(sizeof(RowShape)==sizeof(RLRow),"row size");
_Static_assert(sizeof(LedgerShape)==sizeof(RLLedger),"ledger size");
_Static_assert(offsetof(LedgerShape,input_hex)==offsetof(RLLedger,input_hex),"ledger offset");
int main(void) {
    const char text[]="#SC1\tstage=e3:lnx/x86_64/tape/O0\tinput_sha256=0000000000000000000000000000000000000000000000000000000000000000\n00\t0\t-\t0\t-\n01\t1\tempty\t0\te3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n";
    RLFileView views[]={{"resources.tsv",(const unsigned char *)text,sizeof text-1},{"empty",(const unsigned char *)"",0}};
    int failures=0,points=0,success=0;
    for(int stop=1;stop<64;stop++) {
        RLLedger l={0};char why[256]={0};calls=0;fail_at=stop;
        int rc=rl_load_views("memory",views,2,&l,why,sizeof why);
        if(!rc){success=1;if(l.n!=2||!l.has_id||!l.rows[1].present)failures++;rl_clear(&l);break;}
        points++;if(l.rows||l.n||l.has_id||!strstr(why,"allocation"))failures++;
        rl_clear(&l);if(count||invalid)failures++;
    }
    if(!success||count||invalid||points<4)failures++;
    printf("LEDGER_CX_OWNERSHIP failure_points=%d failed=%d\n",points,failures);
    return failures?1:0;
}
