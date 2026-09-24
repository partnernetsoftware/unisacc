#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(void)
{
    errno = ENOENT;
    printf("%s|%s\n", strerror(ENOENT), strerror(EINVAL));
    perror("open");
    errno = 0;
    printf("%d\n", errno);
    return 0;
}
