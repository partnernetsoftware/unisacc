int main(void){int a;int *p;int **q;a=7;p=&a;q=&p;**q=9;printf("%d\n",a);return 0;}
