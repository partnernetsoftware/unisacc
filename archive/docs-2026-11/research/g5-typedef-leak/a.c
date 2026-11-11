typedef struct { int x; } code;
int g(void){ code c; c.x = 1; return c.x; }
