#include <stdio.h>
int main(int argc, char **argv){ printf("%d %s\n", argc, argv[0] ? "have" : "none"); return 0; }
