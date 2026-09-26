int g(int a){long x=a; x=x*100000; x=x*100000; return x>0;}
int main(){char c=-1; long k=1; k=k<<40; c=c+k; return g(c)+c+(k>1);}
