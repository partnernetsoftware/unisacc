/* 两个 fd，poll 等首个可读。 */
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
int main(void) {
    struct pollfd fds[2];
    fds[0].fd = 0; fds[0].events = POLLIN; fds[0].revents = 0;
    fds[1].fd = -1; fds[1].events = POLLIN; fds[1].revents = 0;
    int r = poll(fds, 2, 100);
    if (r < 0) { printf("UNAVAILABLE poll\n"); return 1; }
    printf("ok ready %d\n", r);
    return 0;
}
