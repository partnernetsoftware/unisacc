int s(int **pp, int *p){ *pp = p; int **w = &p; int v = **w * 2 + *p; *p = v - 1; return v + *&v;}
int main(){int a=5; int *b; int k=s(&b, &a); return k + *b;}
