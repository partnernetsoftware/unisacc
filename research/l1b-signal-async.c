/* Diagnostic probe; requires the parked real-sigaction header and experimental backend. */
#include <signal.h>
#include <unistd.h>
static volatile int na,nb,bad;
static void inner(int s){if(s!=SIGUSR2)bad=1;nb++;}
static long work(int d,long seed);
static void outer(int s){int q;if(s!=SIGALRM)bad=1;na++;for(q=0;q<100;q++)if(work(6,100)!=27489)bad=1;kill(getpid(),SIGUSR2);}
static long work(int d,long seed){volatile long a[33];long sum=0;int k;for(k=0;k<33;k++)a[k]=seed+k; if(d)sum=work(d-1,seed+1);for(k=0;k<33;k++){if(a[k]!=seed+k)bad=1;sum+=a[k];}return sum;}
int main(void){struct sigaction sa;int k;long v;sa.sa_handler=inner;sa.sa_flags=0;sigemptyset(&sa.sa_mask);if(sigaction(SIGUSR2,&sa,0))return 2;sa.sa_handler=outer;if(sigaction(SIGALRM,&sa,0))return 3;write(1,"R\n",2);for(k=0;k<100000;k++){v=work(6,100);if(v!=27489)bad=1;}if(bad||nb<na||na==0)return 4;write(1,"OK\n",3);return 0;}
