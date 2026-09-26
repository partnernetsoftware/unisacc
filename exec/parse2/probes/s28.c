unsigned char c(void){ return 200; }
unsigned short us(void){ return 7; }
int main(void){ int x; long y; x = 300; y = (unsigned char)(x + 1); y = y + (unsigned short)x; return c() + us() + (int)y; }
