#include <sys/mman.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
static void stage(const char*s){fprintf(stderr,"STAGE\t%s\n",s);fflush(stderr);}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 int jit=!strcmp(argv[1],"map-jit"),rwx=!strcmp(argv[1],"rwx");
 if(!jit&&!rwx&&strcmp(argv[1],"rw-rx"))return 2;
 stage("entry");
 int (*supported)(void)=(int(*)(void))dlsym(RTLD_DEFAULT,"pthread_jit_write_protect_supported_np");
 void (*protect)(int)=(void(*)(int))dlsym(RTLD_DEFAULT,"pthread_jit_write_protect_np");
 int can=supported?supported():0;
 fprintf(stderr,"JIT_API\tsupported_symbol=%d\tprotect_symbol=%d\tsupported=%d\n",supported!=0,protect!=0,can);fflush(stderr);
 int flags=0x1002|(jit?0x800:0),prot=(jit||rwx)?7:3;
 stage("mmap-start");
 unsigned char*p=mmap(0,16384,prot,flags,-1,0);
 if(p==MAP_FAILED){fprintf(stderr,"MMAP_FAIL\terrno=%d\t%s\n",errno,strerror(errno));return 20;}
 fprintf(stderr,"MMAP_OK\tptr=%p\tprot=%d\tflags=0x%x\n",p,prot,flags);fflush(stderr);
 if(jit&&can&&protect){stage("jit-write-disable");protect(0);}else if(jit)stage("jit-toggle-NOT_SUPPORTED-not-called");
 static const unsigned char code[]={0xb8,7,0,0,0,0xc3};
 stage("copy-start");memcpy(p,code,sizeof code);stage("copy-done");
 if(jit&&can&&protect){stage("jit-write-enable");protect(1);}
 if(!jit&&!rwx){stage("mprotect-RX-start");if(mprotect(p,16384,5)){fprintf(stderr,"MPROTECT_FAIL\terrno=%d\t%s\n",errno,strerror(errno));munmap(p,16384);return 21;}stage("mprotect-RX-done");}
 stage("call-start");int result=((int(*)(void))p)();fprintf(stderr,"CALL_RESULT\t%d\n",result);fflush(stderr);stage("call-done");
 if(munmap(p,16384))return 22;return result;
}
