/* Windows POSIX layer batch 2 (0.0.21): dirent, broken-down time, and
   ftruncate/fsync/lstat/chmod/pipe.
   Portable and deterministic: directory names are filtered ("." and "..")
   and sorted; times are fixed instants (plus a local round trip that holds
   in any zone), so the same probe runs under hosthdr and tests/winposix.sh. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

int main(void) {
    char names[8][64]; char tmp[64]; char buf[128]; int n, i, j, fd, dirs, all, again;
    DIR *d; struct dirent *e; time_t t, u; struct tm g, l, *p;
#ifdef _WIN32
    chdir("C:\\u");
#endif
    unlink("wp_e/a.txt"); unlink("wp_e/b.txt"); rmdir("wp_e/c"); rmdir("wp_e");
    mkdir("wp_e", 0755); mkdir("wp_e/c", 0755);
    fd = open("wp_e/b.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); close(fd);
    fd = open("wp_e/a.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); close(fd);
    d = opendir("wp_e"); printf("opendir %d\n", d != 0);
    n = 0; dirs = 0; all = 0; errno = 0;
    while ((e = readdir(d)) != 0) {
        all++;
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        if (n < 8) { strcpy(names[n], e->d_name); n++; }
    }
    printf("eof errno %d\n", errno);
    for (i = 0; i < n; i++) for (j = i + 1; j < n; j++) if (strcmp(names[i], names[j]) > 0) { strcpy(tmp, names[i]); strcpy(names[i], names[j]); strcpy(names[j], tmp); }
    for (i = 0; i < n; i++) printf("entry %s\n", names[i]);
    rewinddir(d); again = 0; while (readdir(d) != 0) again++;
    printf("rewind same %d\n", again == all);
    printf("closedir %d\n", closedir(d));
    errno = 0; d = opendir("wp_nodir"); printf("opendir missing %d enoent %d\n", d != 0, errno == ENOENT);
    (void)dirs;
    unlink("wp_e/a.txt"); unlink("wp_e/b.txt"); rmdir("wp_e/c"); rmdir("wp_e");
    {   struct stat st; int pf[2]; char c2[8];
        fd = open("wp_t.txt", O_RDWR | O_CREAT | O_TRUNC, 0644); write(fd, "0123456789", 10);
        printf("ftruncate %d", ftruncate(fd, 4)); fstat(fd, &st); printf(" size %ld", (long)st.st_size);
        printf(" pos %ld fsync %d\n", (long)lseek(fd, 0, SEEK_CUR), fsync(fd)); close(fd);
        printf("lstat %d reg %d\n", lstat("wp_t.txt", &st), S_ISREG(st.st_mode) != 0);
        printf("chmod ro %d", chmod("wp_t.txt", 0444)); errno = 0; fd = open("wp_t.txt", O_WRONLY, 0);
        printf(" open-w %d eacces %d", fd >= 0, errno == EACCES); if (fd >= 0) close(fd);
        printf(" rw %d\n", chmod("wp_t.txt", 0644)); unlink("wp_t.txt");
        printf("pipe %d", pipe(pf)); printf(" w %ld", (long)write(pf[1], "pq", 2)); memset(c2, 0, 8);
        printf(" r %ld [%s]\n", (long)read(pf[0], c2, 2), c2); close(pf[0]); close(pf[1]);
    }
    t = 1234567890; p = gmtime(&t);
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S %a %b %j %u %w", p); printf("gmtime %s\n", buf);
    printf("asctime %s", asctime(p));
    t = 951782400 + 86399; gmtime_r(&t, &g); printf("leap %d/%d yday %d\n", g.tm_mon + 1, g.tm_mday, g.tm_yday);
    t = -86400; gmtime_r(&t, &g); strftime(buf, sizeof buf, "%F %T", &g); printf("pre1970 %s\n", buf);
    t = 1700000000; localtime_r(&t, &l); u = mktime(&l); printf("mktime round %d\n", u == t);
    gmtime_r(&t, &g); printf("offset quarter %d\n", ((l.tm_hour * 60 + l.tm_min) - (g.tm_hour * 60 + g.tm_min) + 1440 * 2) % 15 == 0);
    printf("ctime len %d\n", (int)strlen(ctime(&t)));
    return 0;
}
