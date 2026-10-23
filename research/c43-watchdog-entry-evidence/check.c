#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
static double now(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))exit(2);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char **argv){double start,end;pid_t p;int status,i;if(argc!=5)return 2;start=now();p=fork();if(p<0)return 2;if(!p){execl(argv[1],argv[1],argv[2],argv[3],argv[4],(char*)0);_exit(127);}if(waitpid(p,&status,0)!=p)return 2;end=now();printf("supervisor_elapsed %.6f\n",end-start);if(!WIFEXITED(status)||WEXITSTATUS(status)!=1||end-start<29.0||end-start>32.25)return 1;for(i=3;i<5;i++){FILE *f=fopen(argv[i],"r");long q;if(!f||fscanf(f,"%ld",&q)!=1||q<=1)return 2;fclose(f);errno=0;if(kill((pid_t)q,0)==0||errno!=ESRCH){kill((pid_t)q,SIGKILL);fprintf(stderr,"residual_pid %ld\n",q);return 1;}printf("pid_reaped %ld ESRCH\n",q);}puts("watchdog_cleanup PASS command30 outer32.25 detached-grandchild reaped");return 0;}
