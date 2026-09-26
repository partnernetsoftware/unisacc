int **k(int **q, int n){return q-2+n;} int main(){int a=3; int *p=&a; int **pp=&p; return **(k(pp,2)) == 3;}
