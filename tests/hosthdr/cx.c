/* 0.0.34 X1: include/cx.h (run with output, timeout, whole-file read/write, sha256) = cc */
#include <cx.h>
int main(void){ char *o; size_t n; char h[65]; int rc;
 rc=cx_run("printf 'a\\nb'; exit 5",&o,&n); printf("run rc=%d n=%lu [%s]\n",rc,(unsigned long)n,o); free(o);
 cx_sha256_hex("",0,h); printf("%s\n",h); cx_sha256_hex("abc",3,h); printf("%s\n",h);
 { static char big[1000]; memset(big,'a',1000); cx_sha256_hex(big,1000,h); printf("%s\n",h); cx_sha256_hex(big,56,h); printf("%s\n",h); cx_sha256_hex(big,64,h); printf("%s\n",h);}
 printf("w=%d\n",cx_write("cx_probe.txt","hello\n",6)); o=cx_read("cx_probe.txt",&n); printf("r n=%lu [%s]",(unsigned long)n,o); free(o);
 printf("nofile=%d\n",cx_read("/nonexistent/x",&n)==0);
 rc=cx_run_timeout("echo start; sleep 5; echo never",1,&o,&n); printf("to rc=%d [%s]\n",rc,o); free(o);
 rc=cx_run_timeout("echo quick; exit 2",3,&o,&n); printf("q rc=%d [%s]\n",rc,o); free(o);
 rc=cx_run("kill -9 $$",0,0); printf("sig rc=%d\n",rc);
 return 0;}
