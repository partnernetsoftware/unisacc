#include <sys/wait.h>
#include <sys/socket.h>
int main(void){ return socket(AF_INET, SOCK_STREAM, 0) == -1; }
