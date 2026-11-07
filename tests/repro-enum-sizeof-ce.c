enum { A=3 };
int a[sizeof(A)];
int main(void){return sizeof(a)==sizeof(int)*sizeof(int)?0:1;}
