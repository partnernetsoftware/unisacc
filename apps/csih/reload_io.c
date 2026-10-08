/* reload_io.c - S2 save 单件实现 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define RELOAD_IO_MAX 131072

extern int reload_state_validate_any(const char *text, size_t len, char *why, size_t cap);

static void rset(char *why, size_t cap, const char *m) {
  if (!why || cap == 0) return;
  size_t n = strlen(m);
  if (n >= cap) n = cap - 1;
  memcpy(why, m, n);
  why[n] = '\0';
}

static int dir_fsync_fd(int dfd, char *why, size_t cap) {
  if (fsync(dfd) != 0) { rset(why, cap, "rename 后目录 fsync 未确认持久"); return -2; }
  return 0;
}

int reload_io_save(const char *path, const char *text, size_t len, char *why, size_t cap) {
  if (!path || !text || !why || cap == 0) { rset(why, cap, "参数为空"); return -1; }
  if (path[0] != '/') { rset(why, cap, "path 非绝对路径"); return -1; }
  if (strlen(path) > 4095) { rset(why, cap, "path 总长超过 4095"); return -1; }
  if (len > RELOAD_IO_MAX) { rset(why, cap, "len 超过 131072"); return -1; }
  if (reload_state_validate_any(text, len, why, cap) == 0) return -1;
  const char *slash = strrchr(path, '/');
  if (!slash || slash == path || slash[1] == '\0') { rset(why, cap, "path 非法目录或空名"); return -1; }
  size_t dlen = (size_t)(slash - path);
  char dir[4096];
  if (dlen >= sizeof(dir)) { rset(why, cap, "父目录路径过长"); return -1; }
  memcpy(dir, path, dlen);
  dir[dlen] = '\0';

  int dfd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (dfd < 0) { rset(why, cap, "打开父目录失败: 非目录或链接"); return -1; }
  struct stat ds;
  if (fstat(dfd, &ds) != 0) { rset(why, cap, "fstat 父目录失败"); close(dfd); return -1; }
  if (!S_ISDIR(ds.st_mode)) { rset(why, cap, "父目录非普通目录"); close(dfd); return -1; }
  if (ds.st_uid != getuid()) { rset(why, cap, "父目录非本人 owner"); close(dfd); return -1; }
  if ((ds.st_mode & 0777) != 0700) { rset(why, cap, "父目录权限非 0700"); close(dfd); return -1; }
  char tmp[4096];
  int tfd = -1;
  for (unsigned seq = 0; seq < 10000; seq++) {
    int n = snprintf(tmp, sizeof(tmp), "%s/.reload_io.%ld.%u.tmp", dir, (long)getpid(), seq);
    if (n < 0 || (size_t)n >= sizeof(tmp)) { rset(why, cap, "临时路径过长"); close(dfd); return -1; }
    tfd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (tfd >= 0) break;
    if (errno != EEXIST) { rset(why, cap, "创建临时文件失败"); close(dfd); return -1; }
  }
  if (tfd < 0) { rset(why, cap, "临时文件冲突过多"); close(dfd); return -1; }
  struct stat ts;
  if (fstat(tfd, &ts) != 0 || !S_ISREG(ts.st_mode) || ts.st_uid != getuid() || (ts.st_mode & 0777) != 0600) {
    rset(why, cap, "临时文件权限/owner 异常"); close(tfd); unlink(tmp); close(dfd); return -1;
  }
  size_t off = 0;
  while (off < len) {
    ssize_t w = write(tfd, text + off, len - off);
    if (w < 0) {
      if (errno == EINTR) continue;
      rset(why, cap, "写入临时文件失败"); close(tfd); unlink(tmp); close(dfd); return -1;
    }
    if (w == 0) { rset(why, cap, "写入返回 0"); close(tfd); unlink(tmp); close(dfd); return -1; }
    off += (size_t)w;
  }
  if (fsync(tfd) != 0) { rset(why, cap, "临时文件 fsync 失败"); close(tfd); unlink(tmp); close(dfd); return -1; }
  if (close(tfd) != 0) { rset(why, cap, "临时文件 close 失败"); unlink(tmp); close(dfd); return -1; }
  if (rename(tmp, path) != 0) { rset(why, cap, "rename 失败"); unlink(tmp); close(dfd); return -1; }
  int rc = dir_fsync_fd(dfd, why, cap);
  if (rc == -2) { close(dfd); return -2; }
  if (rc != 0) { rset(why, cap, "目录 fsync 失败"); close(dfd); return -1; }
  if (close(dfd) != 0) { rset(why, cap, "目标已替换，目录 close 失败未确认持久"); return -2; }
  rset(why, cap, "ok");
  return 0;
}






