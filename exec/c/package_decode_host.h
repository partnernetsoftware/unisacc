/* Host compiler interfaces normally supplied by CX use declarations. */
#ifndef UNISA_PACKAGE_DECODE_HOST_H
#define UNISA_PACKAGE_DECODE_HOST_H
typedef struct { long long size,off,mtime,mtime_ns,ctime,ctime_ns,ino,dev; unsigned tag; int valid; } CxPackageStamp;
typedef struct { char *route,*name,*in,*out; int model; } CxPackageStage;
typedef struct { int name,n,data,len; } CxPackageResource;
typedef struct UnisaPackage {
    char *path; CxPackageStamp stamp; int complete;
    unsigned char *file,*b; int n,m,s,ver;
    int *off,*len,*raw; uint32_t *crc; CxPackageStage *stages; CxPackageResource *res; int nr;
} CxPackage;
void cxpl_release(CxPackage *,void (*)(void *));
int cxmn_tag(const unsigned char *,long,long *,int,int *);
int cxmn_number(const unsigned char *,long,long *,int,long long *);
#endif
