/* 0.0.28 H2: grp.h, fnmatch.h, glob.h, regex.h and sys/resource.h forward to the system C library */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <fnmatch.h>
#include <glob.h>
#include <regex.h>
#include <sys/resource.h>
int main(void) {
    struct group *g = getgrgid(getgid());
    int fm = fnmatch("*.c", "posix2.c", 0) == 0 && fnmatch("a/*", "a/b/c", FNM_PATHNAME) == FNM_NOMATCH
          && fnmatch(".*", ".hidden", FNM_PERIOD) == 0 && fnmatch("*", ".hidden", FNM_PERIOD) == FNM_NOMATCH;
    glob_t gl; int gr = glob("/", 0, 0, &gl); int gok = gr == 0 && gl.gl_pathc == 1 && strcmp(gl.gl_pathv[0], "/") == 0;
    if (gr == 0) globfree(&gl);
    int gn = glob("/no/such/dir/*", 0, 0, &gl) == GLOB_NOMATCH;
    regex_t re; regmatch_t m[2]; int rok = 0;
    if (regcomp(&re, "b(c+)d", REG_EXTENDED) == 0) {
        rok = regexec(&re, "abcccde", 2, m, 0) == 0 && m[0].rm_so == 1 && m[0].rm_eo == 6 && m[1].rm_so == 2 && m[1].rm_eo == 5
           && regexec(&re, "xyz", 0, 0, 0) == REG_NOMATCH && re.re_nsub == 1;
        regfree(&re);
    }
    regex_t rn; int nsub = regcomp(&rn, "a.b", REG_NEWLINE | REG_NOSUB) == 0 && regexec(&rn, "a\nb", 0, 0, 0) == REG_NOMATCH;
    if (nsub) regfree(&rn);
    struct rlimit rl; int rlok = getrlimit(RLIMIT_NOFILE, &rl) == 0 && rl.rlim_cur > 0 && (rl.rlim_max == RLIM_INFINITY || rl.rlim_max >= rl.rlim_cur);
    struct rusage ru; int ruok = getrusage(RUSAGE_SELF, &ru) == 0 && ru.ru_maxrss > 0;
    printf("grp %d fnmatch %d glob %d %d regex %d %d rlimit %d rusage %d\n", g && g->gr_name != 0, fm, gok, gn, rok, nsub, rlok, ruok);
    return 0;
}
