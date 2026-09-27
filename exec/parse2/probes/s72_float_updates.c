#include <stdio.h>
int main(void){
 float f=1.25f;double d=2.5;
 float a=f++;float b=++f;double c=d--;double e=--d;
 printf("%d %d %d %d %d %d\n",(int)(a*4),(int)(b*4),(int)(f*4),(int)(c*4),(int)(e*4),(int)(d*4));
 float wide=16777216.0f;float old=wide++;
 double huge=9007199254740992.0;double was=huge++;
 printf("%d %d %d %d\n",old==16777216.0f,wide==old,was==9007199254740992.0,huge==was);
 float arr[2]={3,4};int n=0;float x=arr[n++]++;
 struct P{double d;} p={3}; double y=++p.d;double z=(p.d)--;
 printf("%d %d %d %d %d %d\n",(int)x,(int)arr[0],n,(int)y,(int)z,(int)p.d);
 return 0;
}
