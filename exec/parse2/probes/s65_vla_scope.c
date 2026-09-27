/* Dynamic bounds are evaluated once; scope and jump exits reclaim storage. */
#include <stdio.h>
int calc(int n) {
 int out=0; int a[n++]; a[0]=5;
 { int a[n+1]; a[0]=7; out+=(int)sizeof a+a[0]; }
 out+=a[0]+(int)sizeof a;
 for(int i=0;i<4;i++) {
  int b[n+i]; b[0]=i;
  { char c[n+2]; c[0]=1; if(i==1)continue; out+=c[0]; }
  switch(i){case 2:{int d[n+3]; d[0]=2; out+=d[0];break;}default:out+=b[0];}
  if(i==3)break;
 }
 for(int k=n, b[k];k>0;k--){b[0]=k;out+=b[0];if(k==2)break;}
 {int a=11;out+=a+(int)sizeof a;}
 return out+n;
}
int main(void){printf("%d %d\n",calc(2),calc(5));return 0;}
