int f(void){static int n;n++;return n;}int main(void){f();f();printf("%d\n",f());return 0;}
