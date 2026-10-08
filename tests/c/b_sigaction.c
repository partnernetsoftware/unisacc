#include <stdio.h>
#include <signal.h>
static volatile int hits;
static void h(int s){hits=hits+s;}
int main(void){struct sigaction a,o;sigset_t m;sigemptyset(&m);sigaddset(&m,SIGINT);a.sa_handler=h;a.sa_mask=m;a.sa_flags=0;
if(sigaction(SIGTERM,&a,0)!=0)return 1;raise(SIGTERM);sigaction(SIGTERM,0,&o);
printf("%d %d %d %d\n",hits,o.sa_handler==h,sigismember(&m,SIGINT),sigismember(&m,SIGHUP));return 0;}
