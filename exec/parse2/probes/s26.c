int main(void){int x; int y; x = 2; y = 0;
 switch (x) { case 1: y = 10; break; case 2: y = 20; case 3: y = y + 1; break; default: y = 5; }
 return y; }
