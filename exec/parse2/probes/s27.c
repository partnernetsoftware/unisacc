int main(void){int x; int y; x = 2; y = 0;
 switch (x) { case 1: y = 10; case 7: y = 3; }
 { int z; z = 1; y = y + z; }
 return y; }
