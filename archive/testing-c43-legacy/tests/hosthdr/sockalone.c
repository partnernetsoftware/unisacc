/* 0.0.19 (dsh): socket.h alone -- no other header pulls the helper in */
#include <sys/socket.h>
#include <stdio.h>
int main(void) { int s = socket(AF_INET, SOCK_STREAM, 0); printf("socket ok %d\n", s >= 0); return 0; }
