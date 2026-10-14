/* Generated forwarding units live outside the caller's source directory.
   Only their forced-include requests use the caller cwd; originals retain
   their existing input bytes/source identity. No chdir and no header parser. */
static int fwd_front(const char *route,const char *last,Buf *input,const char *source) {
#if defined(__APPLE__) || defined(__linux__)
 int slot=-1;for(int i=0;i<NRI;i++)if(RI[i].n==13&&!memcmp(RI[i].name,"\0cli/includes",13))slot=i;
 ResourceInput saved={0};Buf absolute={0};
 if(slot>=0&&RI[slot].len){
  saved=RI[slot];char cwd[4096];cwd[0]=0;
  int at=0;
  while(at<saved.len){
   int end=at;while(end<saved.len&&saved.data[end])end++;
   if(end==saved.len){free(absolute.b);free(absolute.at);return clierror("forward: malformed forced include input");}
   if(end>at&&saved.data[at]!='/'){
    if(!cwd[0]&&!getcwd(cwd,sizeof cwd)){free(absolute.b);free(absolute.at);return clierror("forward: cannot read caller cwd");}
    for(int k=0;cwd[k];k++)bput(&absolute,(unsigned char)cwd[k],0);bput(&absolute,'/',0);
   }
   for(int k=at;k<end;k++)bput(&absolute,saved.data[k],0);bput(&absolute,0,0);at=end+1;
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
