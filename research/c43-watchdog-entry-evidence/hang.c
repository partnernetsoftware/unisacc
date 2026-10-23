#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
static int mark(const char *p){FILE *f=fopen(p,"w");if(!f)return 1;fprintf(f,"%ld\n",(long)getpid());return fclose(f)!=0;}
int main(int argc,char **argv){pid_t p;if(argc!=3)return 2;signal(SIGTERM,SIG_IGN);p=fork();if(p<0)return 2;if(!p){if(setsid()<0||mark(argv[2]))return 2;for(;;)pause();}if(mark(argv[1]))return 2;for(;;)pause();}
