#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned long n; char owner[128], kind[32]; } Row;
static Row a[10000], b[10000];
static int readrows(const char *path, Row *v) {
    FILE *f=fopen(path,"r"); int n=0; char line[512], extra;
    if(!f) return -1;
    while(fgets(line,sizeof line,f)) {
        if(n==10000 || sscanf(line,"%lu\t%127[^\t]\t%31[^\n]%c",&v[n].n,v[n].owner,v[n].kind,&extra)!=4 || extra!='\n' || v[n].n!=(unsigned long)n+1) {fclose(f);return -1;}
        n++;
    }
    if(ferror(f)){fclose(f);return -1;} fclose(f); return n;
}
int main(int argc,char **argv) {
    int na,nb,i=0,j=0,skips=0;
    if(argc!=4) return 2;
    na=readrows(argv[1],a); nb=readrows(argv[2],b);
    if(na<1 || nb<1) return 2;
    while(j<nb) {
        if(i<na && !strcmp(a[i].owner,b[j].owner) && !strcmp(a[i].kind,b[j].kind)){i++;j++;continue;}
        if(strcmp(b[j].owner,"BC") || strcmp(b[j].kind,"b")){fprintf(stderr,"unexpected sequence difference old=%d new=%d\n",i+1,j+1);return 1;}
        printf("insert new=%lu owner=%s kind=%s\n",b[j].n,b[j].owner,b[j].kind);skips++;j++;
    }
    if(i!=na || skips!=atoi(argv[3]) || nb-na!=skips) return 1;
    printf("PASS old=%d new=%d: exactly %d BC/b insertions; every other owner/kind and sequential numbering preserved\n",na,nb,skips);return 0;
}
