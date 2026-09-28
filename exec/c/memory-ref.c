/* Appended to refshim + unisacc.c by memorycheck.sh. This is only an oracle:
   ask the retained backend to bind a tape in memory, without executing it. */
#undef main
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    FILE *f=fopen(argv[1],"rb"); if (!f) return 2;
    if (fseek(f,0,SEEK_END)) return 2;
    long n=ftell(f); if (n<=0 || n>67108864) return 2;
    rewind(f); char *t=malloc(n+1); if (!t) return 2;
    if (fread(t,1,n,f)!=n || fclose(f)) return 2;
    t[n]=0; model_dims(); setup();
    bk_run(t,(int)n,3,0x123456780);
    long prefix = bkos == 1 ? 32 : 0;
    /* Report the logical user-data base, serialize the physical dl slots too. */
    printf("%ld %ld %ld %ld %ld\n",bk_runtext,bk_rundata,bktlen,bkdlen+prefix,bk_entry);
    if (fwrite((void *)bk_runtext,1,bktlen,stdout)!=bktlen ||
        fwrite((void *)(bk_rundata-prefix),1,bkdlen+prefix,stdout)!=bkdlen+prefix) return 2;
    return fclose(stdout) ? 2 : 0;
}
