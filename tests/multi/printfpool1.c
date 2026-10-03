/* A real variadic printf must keep its complete format string in the pool. */
int printf(const char *, ...);

void printfpool_first(void) { printf("A:%d:B", 7); }
