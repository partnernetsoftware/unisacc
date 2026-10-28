/* Host compiler declarations for the CX module dependencies.  Product CX
 * compilation obtains these interfaces from the module graph instead. */
#ifndef UNISA_MODEL_DECODE_HOST_H
#define UNISA_MODEL_DECODE_HOST_H
typedef struct UnisaModel {
    int loaded;
    int ns,nq,nrg,nstr,start,isnet,topmax,nqa;
    char **str; int *strl; int *qoff,*qlen; int64_t *qa;
    int *smode,*rowc,**rowk,**rown,**rowq; int *nlo,*nhi,*bn,*bq,*rets; unsigned char **retok;
} CxModel;
int cxmn_number(const unsigned char *, long, long *, int, long long *);
void cxml_release(CxModel *, void (*)(void *));
#endif
