/* Signature facts and ordinary script calls; no native callback bridge. */
typedef float (*F9)(float,float,float,float,float,float,float,float,float);
typedef float (*G9)(float,float,float,float,float,float,float,float,float);
typedef F9 (*Relay)(G9);
typedef float (*F17)(int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,float);
typedef float (*G17)(int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,float);
typedef float (*Wrong17)(int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,int,double);
typedef double (*V9)(float,float,float,float,float,float,float,float,int,...);
float weighted(float a,float b,float c,float d,float e,float f,float g,float h,float i){return a+2*b+3*c+4*d+5*e+6*f+7*g+8*h+9*i;}
float seventeen(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j,int k,int l,int m,int n,int o,int p,float q){return a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p+q;}
F9 pick(G9 p){return p;}
float outer(F9 a,Relay b,F17 c){return b(a)(1,2,3,4,5,6,7,8,9.0)+c(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17.0);}
int main(void){F9 a=weighted;G9 b=a;F17 c=seventeen;G17 d=c;Relay r=pick;return outer(b,r,d)!=438.0f;}
