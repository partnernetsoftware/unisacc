/* reload_consume.c — reload_io_consume 实现（唯一新源码，无 main）。
 * 前提：调用者已提交恢复后才可调用；本代码不执行恢复。
 * 信任假设：可信祖先目录、同一 session 单写者；不声称检查到
 * unlink 无竞争，也不声称排除外部替换。
 */
#include "reload_io.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

static void cwhy(char *why, size_t cap, const char *m) {
  if (why == NULL || cap == 0) return;
  size_t n = strlen(m);
  if (n >= cap) n = cap - 1;
  memcpy(why, m, n);
  why[n] = '\0';
}

int reload_io_consume(const char *path, const reload_io_token *token,
                      char *why, size_t cap) {
  int rc = -1;
  int dfd = -1;
  int removed = 0;
  char *buf = NULL;
  reload_io_token fresh;
  size_t len = 0;

  if (why != NULL && cap > 0) why[0] = '\0';
  memset(&fresh, 0, sizeof(fresh));

  if (path == NULL || token == NULL) {
    cwhy(why, cap, "null path or token");
    return -1;
  }
  if (why == NULL || cap == 0) {
    return -1;
  }
  if (path[0] != '/' || strlen(path) > 4095) {
    cwhy(why, cap, "path not absolute or too long");
    return -1;
  }

  /* 先确认 token 各身份数组有 NUL，避免被当作 C 串越界读取。 */
  if (memchr(token->session_id, 0, sizeof(token->session_id)) == NULL ||
      memchr(token->handoff_id, 0, sizeof(token->handoff_id)) == NULL ||
      memchr(token->candidate_hash, 0, sizeof(token->candidate_hash)) == NULL) {
    cwhy(why, cap, "token field not NUL-terminated");
    return -1;
  }

  buf = malloc(131073);
  if (buf == NULL) {
    cwhy(why, cap, "buffer allocation failed");
    return -1;
  }

  if (reload_io_load(path, token->session_id, token->handoff_id,
                     token->candidate_hash, buf, 131073, &len, &fresh,
                     why, cap) != 0) {
    free(buf);
    buf = NULL;
    return -1;
  }

  if (fresh.dev != token->dev || fresh.ino != token->ino) {
    cwhy(why, cap, "file replaced: dev/ino mismatch");
    free(buf);
    buf = NULL;
    return -1;
  }

  free(buf);
  buf = NULL;

  {
    char dir[4096];
    const char *slash = strrchr(path, '/');
    size_t dlen;

    if (slash == NULL) {
      cwhy(why, cap, "path lost its slash");
      return -1;
    }
    dlen = (slash == path) ? 1 : (size_t)(slash - path);
    if (dlen + 1 > sizeof(dir)) {
      cwhy(why, cap, "parent path too long");
      return -1;
    }
    if (dlen == 1 && path[0] == '/') {
      dir[0] = '/';
      dir[1] = '\0';
    } else {
      memcpy(dir, path, dlen);
      dir[dlen] = '\0';
    }

    dfd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (dfd < 0) {
      cwhy(why, cap, "open parent directory failed");
      return -1;
    }

    {
      struct stat ds;
      if (fstat(dfd, &ds) != 0 || !S_ISDIR(ds.st_mode) ||
          ds.st_uid != geteuid() || (ds.st_mode & 0777) != 0700) {
        cwhy(why, cap, "parent dir not owned 0700 directory");
        close(dfd);
        dfd = -1;
        return -1;
      }
    }
  }

  if (unlink(path) != 0) {
    cwhy(why, cap, "unlink failed");
    goto done;
  }
  removed = 1;

  if (fsync(dfd) != 0) {
    cwhy(why, cap, "unlinked but fsync parent failed: not durable");
    rc = -2;
    goto done;
  }

  rc = 0;
  cwhy(why, cap, "ok");

done:
  if (dfd >= 0) {
    if (close(dfd) != 0) {
      if (removed) {
        cwhy(why, cap, "unlinked but close parent failed: not durable");
        rc = -2;
      } else {
        cwhy(why, cap, "close parent failed");
        rc = -1;
      }
    }
    dfd = -1;
  }
  free(buf);
  buf = NULL;
  return rc;
}
