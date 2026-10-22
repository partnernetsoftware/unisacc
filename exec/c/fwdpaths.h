/* Generated forwarding units live outside the caller's source directory.
   Only their forced-include requests use the caller cwd; originals retain
   their existing input bytes/source identity. No chdir and no header parser. */
/* A .cx unit's forced-include list starts with its built-in prelude cx.h and may end with module interfaces
   (src/cxmod.c); a forwarding unit is plain C, so it takes neither -- only the user's -include entries, which
   compiler.c points this at. */
static const Buf *fwd_plain_list;
static int fwd_front(const char *route,const char *last,Buf *input,const char *source) {
#if defined(__APPLE__) || defined(__linux__)
 int slot=-1;for(int i=0;i<NRI;i++)if(RI[i].n==13&&!memcmp(RI[i].name,"\0cli/includes",13))slot=i;
 ResourceInput saved={0};Buf absolute={0};
 if(slot>=0&&RI[slot].len){
  saved=RI[slot];char cwd[4096];cwd[0]=0;
  const unsigned char *list=saved.data;int len=saved.len;
  if(fwd_plain_list){list=fwd_plain_list->b;len=fwd_plain_list->n;}   /* no cx.h, no module interface */
  int at=0;
  while(at<len){
   int end=at;while(end<len&&list[end])end++;
   if(end==len){free(absolute.b);free(absolute.at);return clierror("forward: malformed forced include input");}
   if(end>at&&list[at]!='/'){
    if(!cwd[0]&&!getcwd(cwd,sizeof cwd)){free(absolute.b);free(absolute.at);return clierror("forward: cannot read caller cwd");}
    for(int k=0;cwd[k];k++)bput(&absolute,(unsigned char)cwd[k],0);bput(&absolute,'/',0);
   }
   for(int k=at;k<end;k++)bput(&absolute,list[k],0);bput(&absolute,0,0);at=end+1;
  }
  RI[slot].data=absolute.b;RI[slot].len=absolute.n;
 }
 int rc=last?runroute_range(route,NULL,last,input,source):runroute(route,input,source);
 if(slot>=0&&saved.name)RI[slot]=saved;
 free(absolute.b);free(absolute.at);return rc;
#else
 return last?runroute_range(route,NULL,last,input,source):runroute(route,input,source);
#endif
}
