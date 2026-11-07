enum { A=3, Z=0, N=-2 };
#define ALIAS A
int main(void) {
    int i=0;
    int a[2][3];
    if (sizeof(A)!=sizeof(int) || sizeof(Z)!=sizeof(int) || sizeof(N)!=sizeof(int)) return 1;
    if (sizeof((A))!=sizeof(int) || sizeof(ALIAS)!=sizeof(int) || sizeof(A+Z)!=sizeof(int)) return 2;
    { char A=0; if (sizeof(A)!=1) return 3; }
    if (sizeof(A)!=sizeof(int)) return 4;
    if (sizeof(a[i++])!=3*sizeof(int) || i!=0) return 5;
    return 0;
}
