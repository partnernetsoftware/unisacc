int f(int n){if(n<=1)return 1;return n*f(n-1);}int main(void){printf("%d\n",f(6));return 0;}
