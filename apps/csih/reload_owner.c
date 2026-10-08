/* Independent POSIX ownership library; see reload_owner.h assumptions. */
#include "reload_owner.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

static int owner_fail(char *why, size_t cap, const char *message) {
    if (why && cap) snprintf(why, cap, "%s", message);
    return -1;
}

int reload_owner_acquire(const char *path, int *out_fd, char *why, size_t cap) {
    char parent[4096];
    const char *slash;
    size_t len, dlen;
    int dfd, fd;
    struct stat st;
    struct flock lock;
    if (out_fd) *out_fd = -1;
    if (!path || !out_fd) return owner_fail(why, cap, "null argument");
    len = strlen(path);
    if (len == 0 || len > 4095 || path[0] != '/' || path[len - 1] == '/')
        return owner_fail(why, cap, "lock path not absolute or invalid length");
    slash = strrchr(path, '/');
    dlen = slash == path ? 1 : (size_t)(slash - path);
    memcpy(parent, path, dlen); parent[dlen] = 0;
    dfd = open(parent, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dfd < 0) return owner_fail(why, cap, "open parent failed");
    if (fstat(dfd, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 0777) != 0700) {
        close(dfd); return owner_fail(why, cap, "parent not owned 0700 directory");
    }
    if (close(dfd) != 0) return owner_fail(why, cap, "parent close failed");
    fd = open(path, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0) return owner_fail(why, cap, "open lock failed");
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 0777) != 0600) {
        close(fd); return owner_fail(why, cap, "lock not owned 0600 regular file");
    }
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
        close(fd); return owner_fail(why, cap, "lock close-on-exec failed");
    }
    memset(&lock, 0, sizeof lock);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; /* whole file, including future growth */
    if (fcntl(fd, F_SETLK, (long)&lock) != 0) {
        close(fd); return owner_fail(why, cap, "ownership lock unavailable");
    }
    *out_fd = fd;
    if (why && cap) snprintf(why, cap, "ok");
    return 0;
}

int reload_owner_release(int *fd, char *why, size_t cap) {
    int owned;
    if (!fd || *fd < 0) return owner_fail(why, cap, "no owned fd");
    owned = *fd;
    *fd = -1;
    if (close(owned) != 0)
        return owner_fail(why, cap, "close failed; ownership return not confirmed");
    if (why && cap) snprintf(why, cap, "ok");
    return 0;
}
