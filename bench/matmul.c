#include <stdio.h>
static double a[400][400],b[400][400],c[400][400];
int main(void){int n=400;for(int i=0;i<n;i++)for(int j=0;j<n;j++){a[i][j]=i+j;b[i][j]=i-j;}
for(int i=0;i<n;i++)for(int j=0;j<n;j++){double s=0;for(int k=0;k<n;k++)s+=a[i][k]*b[k][j];c[i][j]=s;}printf("%.0f\n",c[7][9]);return 0;}
