int add(int a,int b){return a+b;}int main(void){int(*f)(int,int);f=add;printf("%d\n",f(2,3));return 0;}
