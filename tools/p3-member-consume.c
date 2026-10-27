/* P3 member consume: D stage->M, inflate body vs ctx04 job under pack normalize,
 * same-window clean/pos/neg product routes. Host-gcc O2 ELF; no hostseed edit.
 * Normalize: exec/c/pack.py compact_q then networkformat.encode (UNINETB1);
 * package stores raw-DEFLATE of that binary (codec=1). Raw text .net differs. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "packagefooter.h"
#include "../include/unisa_sha256.h"
/* Exact product codec (puff + slice CRC); same refuse path as run.c inflate. */
#define UNISA_RUNTIME_STATE static
#include "codec.h"

static int checks, failed;
static char root[4096], ctx[4096], pc[4096], work[4096];
static void die(const char *s) {
  fprintf(stderr, "p3-member-consume: REFUSE %s errno=%d\n", s, errno);
  exit(2);
}
static void ck(int ok, const char *s) {
  checks++;
  if (!ok) failed++;
  printf("%s\tp3-member-consume.%s\n", ok ? "PASS" : "FAIL", s);
}
static void path(char *out, size_t cap, const char *a, const char *b) {
  int n = snprintf(out, cap, "%s/%s", a, b);
  if (n < 0 || (size_t)n >= cap) die("path-bound");
}
static unsigned char *slurp(const char *p, size_t *n) {
  struct stat st;
  if (lstat(p, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > 134217728)
    die("regular-input");
  FILE *f = fopen(p, "rb");
  if (!f) die("open");
  unsigned char *b = malloc((size_t)st.st_size);
  if (!b || fread(b, 1, (size_t)st.st_size, f) != (size_t)st.st_size) die("read");
  fclose(f);
  *n = (size_t)st.st_size;
  return b;
}
static void hex(const unsigned char *b, size_t n, char out[65]) {
  unisa_sha256_hex(b, n, out);
}
static int parse_u(const unsigned char *s, size_t n, size_t *at, unsigned long *out) {
  unsigned long v = 0;
  size_t i = *at;
  if (i >= n || s[i] < '0' || s[i] > '9') return 0;
  while (i < n && s[i] >= '0' && s[i] <= '9') {
    if (v > (~0UL) / 10) return 0;
    v = v * 10 + (unsigned)(s[i] - '0');
    i++;
  }
  *at = i;
  *out = v;
  return 1;
}

typedef struct {
  long off, clen, rlen;
  uint32_t crc;
} Memb;
typedef struct {
  char route[96];
  char name[48];
  int model;
} Stage;

static int run_product(const char *ape, char *const argv_tail[], const char *label, int want_icm) {
  char bound[4400], *argv[32];
  int n = 0;
  path(bound, sizeof bound, root, "tools/bound");
  argv[n++] = bound;
  argv[n++] = "12";
  argv[n++] = "sh";
  argv[n++] = (char *)ape;
  for (int i = 0; argv_tail[i]; i++) argv[n++] = argv_tail[i];
  argv[n] = 0;
  int pfd[2];
  if (pipe(pfd)) die("pipe");
  pid_t pid = fork();
  if (pid < 0) die("fork");
  if (!pid) {
    dup2(pfd[1], 2);
    close(pfd[0]);
    close(pfd[1]);
    execv(bound, argv);
    _exit(127);
  }
  close(pfd[1]);
  char err[512];
  size_t en = 0;
  for (;;) {
    ssize_t z = read(pfd[0], err + en, sizeof err - 1 - en);
    if (z < 0 && errno == EINTR) continue;
    if (z <= 0) break;
    en += (size_t)z;
    if (en >= sizeof err - 1) break;
  }
  close(pfd[0]);
  err[en] = 0;
  int st = 0;
  if (waitpid(pid, &st, 0) < 0) die("wait");
  int rc = WIFEXITED(st) ? WEXITSTATUS(st) : 99;
  int icm = rc == 2 && strstr(err, "invalid compressed model") != 0;
  int ok = want_icm ? icm : (rc == 0 && !icm);
  ck(ok, label);
  return ok;
}

static unsigned char *inflate_raw(const unsigned char *blob, size_t clen, size_t rlen, uint32_t expect_crc) {
  unsigned char *out = malloc(rlen);
  if (!out) die("allocation");
  crc_init();
  if (unisa_inflate(out, (unsigned long)rlen, blob, (unsigned long)clen)) {
    free(out);
    die("inflate-extent");
  }
  if (crc((char *)out, (int)rlen) != expect_crc) {
    free(out);
    die("inflate-crc");
  }
  return out;
}

static void write_corrupt(const unsigned char *ape, size_t ape_n, long pkg_off, const Memb *m, const char *path) {
  unsigned char *a = malloc(ape_n);
  if (!a) die("allocation");
  memcpy(a, ape, ape_n);
  size_t abs = (size_t)pkg_off + (size_t)m->off + 16;
  if (abs >= ape_n) die("flip-bound");
  a[abs] ^= 1;
  int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
  if (fd < 0) die("exclusive-corrupt");
  size_t at = 0;
  while (at < ape_n) {
    ssize_t z = write(fd, a + at, ape_n - at);
    if (z < 0 && errno == EINTR) continue;
    if (z <= 0) die("corrupt-write");
    at += (size_t)z;
  }
  if (fsync(fd) || close(fd)) die("corrupt-close");
  free(a);
}

int main(int argc, char **argv) {
#if !defined(__linux__) || !defined(__x86_64__)
  fputs("NOT_RUN p3-member-consume needs Linux/x86_64\n", stderr);
  return 3;
#endif
  if (argc != 5 || !realpath(argv[1], root) || !realpath(argv[2], ctx) || !realpath(argv[3], pc) ||
      argv[4][0] != '/')
    die("usage ROOT SOURCE_CTX PACK_CTX NEW_ABS_OUT");
  {
    char parent[4096], base[4096], leaf[4096];
    snprintf(parent, sizeof parent, "%s", argv[4]);
    char *slash = strrchr(parent, '/');
    if (!slash || !slash[1]) die("output-leaf");
    snprintf(leaf, sizeof leaf, "%s", slash + 1);
    if (slash == parent)
      slash[1] = 0;
    else
      *slash = 0;
    if (!realpath(parent, base)) die("output-parent");
    int z = snprintf(work, sizeof work, "%s%s%s", base, !strcmp(base, "/") ? "" : "/", leaf);
    if (z < 0 || (size_t)z >= sizeof work) die("output-path");
    if (mkdir(work, 0700)) die("new-output-required");
  }

  char product[4400], pkg_path[4400], src[4400];
  path(product, sizeof product, root, "unisacc.com");
  path(pkg_path, sizeof pkg_path, pc, "compiler.pkg");
  path(src, sizeof src, work, "ret7.c");
  {
    int fd = open(src, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    const char *body = "int main(void){return 7;}\n";
    if (fd < 0 || write(fd, body, strlen(body)) != (ssize_t)strlen(body) || close(fd)) die("ret7");
  }

  size_t ape_n, pkg_n;
  unsigned char *ape = slurp(product, &ape_n);
  unsigned char *pc_pkg = slurp(pkg_path, &pkg_n);
  PackageFooter ft;
  if (!packagefooter_locate_bytes(ape, (long)ape_n, &ft)) die("footer");
  ck(ft.package_length == (long)pkg_n && !memcmp(ape + ft.package_offset, pc_pkg, pkg_n), "emb-pkg-eq-pc04");
  free(pc_pkg);

  const unsigned char *b = ape + ft.package_offset;
  size_t bn = (size_t)ft.package_length;
  size_t at = 0;
  unsigned long pm, ps, nr;
  if (bn < 4 || memcmp(b, "P 3 ", 4)) die("not-p3");
  at = 4;
  if (!parse_u(b, bn, &at, &pm) || b[at++] != ' ') die("hdr");
  if (!parse_u(b, bn, &at, &ps) || b[at++] != ' ') die("hdr");
  if (!parse_u(b, bn, &at, &nr) || b[at++] != '\n') die("hdr");
  ck(pm == 32 && ps == 1479, "pm32-ps1479");

  Stage *stages = calloc(ps, sizeof *stages);
  Memb *ms = calloc(pm, sizeof *ms);
  if (!stages || !ms) die("allocation");
  for (unsigned long i = 0; i < ps; i++) {
    if (at + 2 > bn || b[at] != 'D' || b[at + 1] != ' ') die("D");
    at += 2;
    /* route */
    size_t s0 = at;
    while (at < bn && b[at] != ' ') at++;
    if (at >= bn || at - s0 >= sizeof stages[i].route) die("route");
    memcpy(stages[i].route, b + s0, at - s0);
    stages[i].route[at - s0] = 0;
    at++;
    s0 = at;
    while (at < bn && b[at] != ' ') at++;
    if (at >= bn || at - s0 >= sizeof stages[i].name) die("name");
    memcpy(stages[i].name, b + s0, at - s0);
    stages[i].name[at - s0] = 0;
    at++;
    while (at < bn && b[at] != ' ') at++;
    if (at >= bn || b[at] != ' ') die("in");
    at++;
    while (at < bn && b[at] != ' ') at++;
    if (at >= bn || b[at] != ' ') die("out");
    at++;
    unsigned long mi;
    if (!parse_u(b, bn, &at, &mi) || b[at++] != '\n' || mi >= pm) die("model-index");
    stages[i].model = (int)mi;
  }
  for (unsigned long i = 0; i < pm; i++) {
    unsigned long clen, rlen, codec, crc;
    if (at + 2 > bn || b[at] != 'M' || b[at + 1] != ' ') die("M");
    at += 2;
    if (!parse_u(b, bn, &at, &clen) || b[at++] != ' ') die("clen");
    if (!parse_u(b, bn, &at, &rlen) || b[at++] != ' ') die("rlen");
    if (!parse_u(b, bn, &at, &codec) || b[at++] != ' ') die("codec");
    if (!parse_u(b, bn, &at, &crc) || b[at++] != '\n') die("crc");
    if (codec != 1 || at + clen > bn) die("body");
    ms[i].off = (long)at;
    ms[i].clen = (long)clen;
    ms[i].rlen = (long)rlen;
    ms[i].crc = (uint32_t)crc;
    at += clen;
  }

  int m_pp = -1, m_lower = -1;
  for (unsigned long i = 0; i < ps; i++) {
    if (!strcmp(stages[i].route, "lnx/x86_64/pp") && !strcmp(stages[i].name, "e2")) m_pp = stages[i].model;
    if (!strcmp(stages[i].route, "lnx/x86_64/image/O0") && !strcmp(stages[i].name, "lower"))
      m_lower = stages[i].model;
  }
  ck(m_pp == 3, "D-shared-pp-e2-M3");
  ck(m_lower == 13, "D-nonshared-image-lower-M13");

  /* job text == PC model text (shared + nonshared) */
  static const struct {
    const char *job;
    const char *model;
    int mi;
    const char *text_sha;
    const char *raw_sha;
  } cases[] = {
      {"net-shared-e2", "shared-e2.net", 3,
       "4ef38e6c02707f11771c9eb1fe2b14ad59ebb90e7399a188ab6f1a9889af3a9f",
       "4347eb99eeac3d1d7ba356210192f7fef102949e6ed56ec4447d3fb5cc6e2cd7"},
      {"net-lnx-x86_64-lower", "lnx-x86_64-lower.net", 13,
       "16d99cdb459f52dcacea4544b57551cfa285443480b594147bbb7d7d3c81feb5",
       "3f4c80e2500315ddf9db4086e7bd7bffab0ac6c5944186a1e391581d71ea3ad5"},
  };
  puts("ALGORITHM\tcomparable-id=SHA256(inflate(M.body));equals=SHA256(networkformat.encode(pack.compact_q(job.result.net)));"
       "construction=exec/c/pack.py:build compact_q then compressed_model;raw-.net-text-not-byte-equal");
  for (int c = 0; c < 2; c++) {
    char jp[4400], mp[4400], jh[65], mh[65], rh[65];
    path(jp, sizeof jp, ctx, cases[c].job);
    /* job path is absolute in model-inputs; use ctx/jobs/... */
    snprintf(jp, sizeof jp, "%s/jobs/%s/result.net", ctx, cases[c].job);
    path(mp, sizeof mp, pc, "models");
    char mfull[4400];
    path(mfull, sizeof mfull, mp, cases[c].model);
    size_t jn, mn;
    unsigned char *jb = slurp(jp, &jn);
    unsigned char *mb = slurp(mfull, &mn);
    ck(jn == mn && !memcmp(jb, mb, jn), c == 0 ? "shared-job-eq-pc-text" : "nonshared-job-eq-pc-text");
    hex(jb, jn, jh);
    hex(mb, mn, mh);
    ck(!strcmp(jh, cases[c].text_sha) && !strcmp(mh, cases[c].text_sha),
       c == 0 ? "shared-text-sha-pin" : "nonshared-text-sha-pin");
    free(jb);
    free(mb);
    int mi = cases[c].mi;
    unsigned char *raw =
        inflate_raw(b + ms[mi].off, (size_t)ms[mi].clen, (size_t)ms[mi].rlen, ms[mi].crc);
    ck(ms[mi].rlen >= 9 && !memcmp(raw, "UNINETB1N", 9), c == 0 ? "shared-raw-magic" : "nonshared-raw-magic");
    hex(raw, (size_t)ms[mi].rlen, rh);
    ck(!strcmp(rh, cases[c].raw_sha), c == 0 ? "shared-raw-eq-encode-compact_q-pin" : "nonshared-raw-eq-encode-compact_q-pin");
    /* pin documents offline equality to encode(compact_q(job)); construction proof pack.py */
    free(raw);
  }

  /* same-window dynamic: clean / used-M ICM / unused-M OK */
  char out_e[4400], out_c[4400], bad3[4400], bad13[4400], bad0[4400];
  path(out_e, sizeof out_e, work, "clean.i");
  path(out_c, sizeof out_c, work, "clean.com");
  path(bad3, sizeof bad3, work, "corrupt-m3.com");
  path(bad13, sizeof bad13, work, "corrupt-m13.com");
  path(bad0, sizeof bad0, work, "corrupt-m0.com");
  write_corrupt(ape, ape_n, ft.package_offset, &ms[3], bad3);
  write_corrupt(ape, ape_n, ft.package_offset, &ms[13], bad13);
  write_corrupt(ape, ape_n, ft.package_offset, &ms[0], bad0);

  {
    char *tail_e[] = {"-b", "lnx/x86_64", "-E", src, "-o", out_e, 0};
    char *tail_c[] = {"-b", "lnx/x86_64", src, "-o", out_c, 0};
    run_product(product, tail_e, "clean-E-ok", 0);
    run_product(bad3, tail_e, "corrupt-M3-E-ICM", 1);
    run_product(bad0, tail_e, "corrupt-M0-E-still-ok", 0);
    run_product(product, tail_c, "clean-compile-ok", 0);
    run_product(bad13, tail_c, "corrupt-M13-compile-ICM", 1);
    run_product(bad3, tail_c, "corrupt-M3-compile-still-ok", 0);
  }

  free(stages);
  free(ms);
  free(ape);
  puts("NOT_CLAIMED\thostseed-payload-trust-rewrite-complete-N22-all-32-CLI-routes-foreign-hosts");
  printf("P3_MEMBER_CONSUME\tassertions=%d\tfailed=%d\tpm=32\tshared=M3/shared-e2\tnonshared=M13/"
         "lnx-x86_64-lower\n",
         checks, failed);
  return failed ? 1 : 0;
}
