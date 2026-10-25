extern int hostvalue;
long hostadd(long a, long b);
long g(long a){return a+1;}
long f(long a){return hostadd(g(a),hostvalue);}
long answer(void){return f(2);}
