union U{int i;char c;};int main(void){union U u;u.i=65;printf("%d\n",u.i);return 0;}
