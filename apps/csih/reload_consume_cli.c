/* reload_consume_cli.c — 独立探针 CLI，仅供临时验收 reload_io_consume。
 * 不执行会话恢复，不做生产接管；不自行 mkdir/chmod/unlink。
 */
#include "reload_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char out[131073];
static reload_io_token tok;

/* NEXT: 参数计数与模式白名单必须在 reload_io_load 之前完成。 */
int main(int argc, char **argv) {
  const char *path, *session, *handoff, *hash, *mode, *repl = NULL;
  size_t len = 0;
  int load_rc, consume_rc;
  char why[512];

  if ((argc != 7 && argc != 8) || strcmp(argv[1], "consume") != 0) {
    fprintf(stderr, "usage: %s consume PATH SESSION HANDOFF HASH MODE [REPLACEMENT]\n", argv[0]);
    return 2;
  }

  path = argv[2];
  session = argv[3];
  handoff = argv[4];
  hash = argv[5];
  mode = argv[6];
  if (strcmp(mode, "replace") == 0) {
    if (argc != 8) {
      fprintf(stderr, "replace needs REPLACEMENT\n");
      return 2;
    }
    repl = argv[7];
  } else if (argc != 7) {
    fprintf(stderr, "too many arguments for mode %s\n", mode);
    return 2;
  }

  if (strcmp(mode, "direct") != 0 && strcmp(mode, "stale") != 0 &&
      strcmp(mode, "bad-id") != 0 && strcmp(mode, "unterminated") != 0 &&
      strcmp(mode, "replace") != 0) {
    fprintf(stderr, "unknown mode: %s\n", mode);
    return 2;
  }

  load_rc = reload_io_load(path, session, handoff, hash, out, sizeof(out),
                           &len, &tok, why, sizeof(why));
  if (load_rc != 0) {
    printf("load_rc=%d why=%s\n", load_rc, why);
    return 1;
  }

  if (strcmp(mode, "direct") == 0) {
    /* token 不动 */
  } else if (strcmp(mode, "stale") == 0) {
    tok.ino += 1;
  } else if (strcmp(mode, "bad-id") == 0) {
    strcpy(tok.session_id, "other");
  } else if (strcmp(mode, "unterminated") == 0) {
    memset(tok.session_id, 'x', sizeof(tok.session_id));
  } else if (strcmp(mode, "replace") == 0) {
    if (rename(repl, path) != 0) {
      fprintf(stderr, "rename failed: %s -> %s\n", repl, path);
      return 2;
    }
  }

  consume_rc = reload_io_consume(path, &tok, why, sizeof(why));
  printf("consume_rc=%d why=%s\n", consume_rc, why);
  if (consume_rc == 0) return 0;
  if (consume_rc == -1) return 1;
  if (consume_rc == -2) return 3;
  return 1;
}
