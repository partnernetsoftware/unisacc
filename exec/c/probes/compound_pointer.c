/* Compound pointer steps use pointee widths, including pointer-to-pointer. */
struct P {int a; long b;};
int main(void){
 struct P a[3]; struct P *p=a; struct P *q[3]; struct P **pp=q;
 int x[3]; int *ip=x;
 p+=2; p-=1; pp+=2; pp-=1; ip+=2; ip-=1;
 return p-a!=1 || pp-q!=1 || ip-x!=1;
}
