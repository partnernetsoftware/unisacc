/* Prefix member/element updates: signed/narrow/unsigned/pointer kinds,
   nested members, and a subscript whose side effect must occur once.
   Independent expected exit: 10+9+0+3+1+8+8+1 = 40. */
struct Inner { unsigned char c; short s; };
struct P { long n; unsigned int u; int *p; struct Inner in; };
int main(void) {
    struct P x; struct P *q=&x; int a[4]; int i; int sum;
    a[0]=7; a[1]=8; a[2]=9; a[3]=10;
    x.n=9; x.u=4294967295u; x.in.c=255; x.in.s=4; x.p=a;
    sum=++q->n;
    sum+=--x.n;
    sum+=++q->in.c;
    sum+=--x.in.s;
    sum+=(++q->u==0);
    sum+=*(++q->p);
    i=0; sum+=++a[i++]; sum+=i;
    return sum;
}
