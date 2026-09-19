struct P{int x;int y;};int main(void){struct P a[2];a[0].x=1;a[1].y=2;printf("%d\n",a[0].x+a[1].y);return 0;}
