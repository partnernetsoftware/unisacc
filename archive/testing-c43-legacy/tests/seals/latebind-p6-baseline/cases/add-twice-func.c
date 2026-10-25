long hostadd(long a, long b);
long f(long a){return hostadd(a,1)+hostadd(a,2);}
long answer(void){return f(3);}
