/* journal_checkpoint.c  已实现未测试；本件无 main，仅 checkpoint 单函数。
 * 前提：空闲、单写者、无外部替换。不把 stat/fclose 当持久提交。
 */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stddef.h>

#define JC_MAX_OFF 9007199254740991LL

static void checkpoint_set_why(char *why, size_t cap, const char *msg)
{
    if (why == NULL || cap == 0)
        return;
    strncpy(why, msg, cap - 1);
    why[cap - 1] = '\0';
}

int journal_checkpoint(const char *path, long long *offset, char *why, size_t cap)
{
    int fd;
    struct stat st;
    long long sz;

    if (offset == NULL) {
        checkpoint_set_why(why, cap, "null offset");
        return -1;
    }
    *offset = 0;

    if (path == NULL || path[0] != '/') {
        checkpoint_set_why(why, cap, "path not absolute");
        return -1;
    }
    if (strlen(path) > 4095) {
        checkpoint_set_why(why, cap, "path too long");
        return -1;
    }

    fd = open(path, O_RDWR | O_NOFOLLOW);
    if (fd < 0) {
        checkpoint_set_why(why, cap, "open failed");
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        checkpoint_set_why(why, cap, "fstat failed");
        close(fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        checkpoint_set_why(why, cap, "not a regular file");
        close(fd);
        return -1;
    }
    if (st.st_uid != getuid()) {
        checkpoint_set_why(why, cap, "not owner");
        close(fd);
        return -1;
    }
    sz = (long long)st.st_size;
    if (sz < 0 || sz > JC_MAX_OFF) {
        checkpoint_set_why(why, cap, "size out of range");
        close(fd);
        return -1;
    }

    if (fsync(fd) != 0) {
        checkpoint_set_why(why, cap, "fsync failed");
        close(fd);
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        checkpoint_set_why(why, cap, "final fstat failed");
        close(fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode) || (long long)st.st_size < 0 ||
        (long long)st.st_size > JC_MAX_OFF) {
        checkpoint_set_why(why, cap, "final size out of range");
        close(fd);
        return -1;
    }

    if (close(fd) != 0) {
        checkpoint_set_why(why, cap, "close failed");
        return -1;
    }

    if (offset != NULL)
        *offset = (long long)st.st_size;
    checkpoint_set_why(why, cap, "ok");
    return 0;
}
