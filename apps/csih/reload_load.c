/* reload_load.c — reload_io_load 实现（唯一新源码，无 main）。
 * 采信 load 成功不删文件；仅已提交恢复后由 consume 收尾。
 */
#include "reload_io.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 复用：状态校验，成功返回 1。 */
extern int reload_state_validate_any(const char *text, size_t len,
                                 char *why, size_t cap);


#define LOAD_MAX_BYTES 131072
#define WHY_CAP_DEFAULT 256

static void set_why(char *why, size_t cap, const char *msg)
{
    if (why != NULL && cap > 0) {
        strncpy(why, msg, cap - 1);
        why[cap - 1] = '\0';
    }
}

static int check_dir(const char *path, char *why, size_t cap)
{
    char dir[4096];
    size_t n = strlen(path);
    int fd;
    struct stat st;

    if (n == 0 || n > 4095 || path[0] != '/') {
        set_why(why, cap, "path length or prefix out of range");
        return -1;
    }
    if (path[n - 1] == '/') {
        set_why(why, cap, "target path must not end with slash");
        return -1;
    }
    {
        size_t slash = 0;
        for (size_t i = n; i > 0; i--) {
            if (path[i - 1] == '/') {
                slash = i - 1;
                break;
            }
        }
        if (slash == 0) {
            dir[0] = '/';
            dir[1] = '\0';
        } else {
            memcpy(dir, path, slash);
            dir[slash] = '\0';
        }
    }
    fd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (fd < 0) {
        set_why(why, cap, "open parent dir failed");
        return -1;
    }
    if (fstat(fd, &st) != 0) {
        set_why(why, cap, "fstat parent dir failed");
        close(fd);
        return -1;
    }
    if (!S_ISDIR(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 0777) != 0700) {
        set_why(why, cap, "parent dir not trusted");
        close(fd);
        return -1;
    }
    return fd;
}

static int read_all(int fd, char *buf, size_t size)
{
    size_t off = 0;
    while (off < size) {
        ssize_t r = read(fd, buf + off, size - off);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (r == 0) {
            return -1; /* short read */
        }
        off += (size_t)r;
    }
    char extra;
    for (;;) {
        ssize_t r = read(fd, &extra, 1);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r < 0) {
            return -1;
        }
        if (r == 0) {
            return 0; /* EOF confirmed */
        }
        return -1; /* grew */
    }
}

static int str_eq(const char *v, size_t vlen, const char *expect)
{
    if (expect == NULL) {
        return 0;
    }
    size_t el = strlen(expect);
    return vlen == el && memcmp(v, expect, el) == 0;
}

int reload_io_load(const char *path, const char *expected_session,
                   const char *expected_handoff, const char *expected_hash,
                   char *out, size_t out_cap, size_t *out_len,
                   reload_io_token *token, char *why, size_t cap)
{
    int dfd = -1, fd = -1, rc = -1;
    char *buf = NULL;
    struct stat st;
    size_t size = 0;
    jvalue *root = NULL, *v;
    const char *sv = NULL;
    size_t svl = 0;

    if (out_len != NULL) {
        *out_len = 0;
    }
    if (token != NULL) {
        memset(token, 0, sizeof(*token));
    }
    if (out != NULL && out_cap > 0) {
        out[0] = '\0';
    }
    if (path == NULL || out == NULL || out_len == NULL || token == NULL ||
        expected_session == NULL || expected_handoff == NULL ||
        expected_hash == NULL) {
        set_why(why, cap, "null argument");
        return -1;
    }

    dfd = check_dir(path, why, cap);
    if (dfd < 0) {
        return -1;
    }
    fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        set_why(why, cap, "open target failed");
        goto done;
    }
    if (fstat(fd, &st) != 0) {
        set_why(why, cap, "fstat target failed");
        goto done;
    }
    if (!S_ISREG(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 0777) != 0600) {
        set_why(why, cap, "target not trusted");
        goto done;
    }
    if (st.st_size < 0 || (unsigned long long)st.st_size > LOAD_MAX_BYTES) {
        set_why(why, cap, "target size out of range");
        goto done;
    }
    size = (size_t)st.st_size;
    if (out_cap < size + 1) {
        set_why(why, cap, "out buffer too small");
        goto done;
    }
    buf = (char *)malloc(size + 1);
    if (buf == NULL) {
        set_why(why, cap, "malloc failed");
        goto done;
    }
    if (read_all(fd, buf, size) != 0) {
        set_why(why, cap, "read failed or size changed");
        goto done;
    }
    buf[size] = '\0';
    if (close(fd) != 0) {
        fd = -1;
        set_why(why, cap, "close target failed");
        goto done;
    }
    fd = -1;
    if (reload_state_validate_any(buf, size, why, cap) != 1) {
        goto done;
    }
    root = json_parse(buf, size, why, cap);
    if (root == NULL) {
        goto done;
    }
    v = jget(root, "session_id");
    sv = v != NULL ? jstr(v) : NULL;
    if (sv == NULL) {
        set_why(why, cap, "missing session_id field");
        goto done;
    }
    if (!str_eq(sv, strlen(sv), expected_session)) {
        set_why(why, cap, "session_id mismatch");
        goto done;
    }
    v = jget(root, "handoff_id");
    sv = v != NULL ? jstr(v) : NULL;
    if (sv == NULL) {
        set_why(why, cap, "missing handoff_id field");
        goto done;
    }
    if (!str_eq(sv, strlen(sv), expected_handoff)) {
        set_why(why, cap, "handoff_id mismatch");
        goto done;
    }
    v = jget(root, "candidate_hash");
    sv = v != NULL ? jstr(v) : NULL;
    if (sv == NULL) {
        set_why(why, cap, "missing candidate_hash field");
        goto done;
    }
    if (!str_eq(sv, strlen(sv), expected_hash)) {
        set_why(why, cap, "candidate_hash mismatch");
        goto done;
    }
    memcpy(out, buf, size + 1);
    *out_len = size;
    token->dev = st.st_dev;
    token->ino = st.st_ino;
    strcpy(token->session_id, expected_session);
    strcpy(token->handoff_id, expected_handoff);
    strcpy(token->candidate_hash, expected_hash);
    set_why(why, cap, "ok");
    rc = 0;

done:
    if (rc != 0) {
        *out_len = 0;
        if (out_cap > 0) {
            out[0] = '\0';
        }
        memset(token, 0, sizeof(*token));
    }
    if (root != NULL) {
        jfree(root);
    }
    free(buf);
    if (fd >= 0) {
        close(fd);
    }
    if (dfd >= 0) {
        if (close(dfd) != 0) {
            rc = -1;
            set_why(why, cap, "close parent failed");
            *out_len = 0;
            if (out_cap > 0) {
                out[0] = '\0';
            }
            memset(token, 0, sizeof(*token));
        }
    }
    return rc;
}
