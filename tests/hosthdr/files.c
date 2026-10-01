#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <termios.h>
#include <sys/ioctl.h>
int main(void) {
    struct stat st; int fd; struct pollfd p; DIR *d; struct dirent *e; int n = 0; struct termios t; struct winsize w;
    mkdir("hh_dir", 0755);
    fd = open("hh_dir/f.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, "hello\n", 6); close(fd);
    if (stat("hh_dir/f.txt", &st) != 0) { puts("stat failed"); return 1; }
    printf("size %ld reg %d dir %d\n", (long)st.st_size, S_ISREG(st.st_mode) != 0, S_ISDIR(st.st_mode) != 0);
    fd = open("hh_dir/f.txt", O_RDONLY, 0); fstat(fd, &st); printf("fstat size %ld flags %d\n", (long)st.st_size, (fcntl(fd, F_GETFL, 0) & O_ACCMODE) == O_RDONLY);
    p.fd = fd; p.events = POLLIN; p.revents = 0; printf("poll %d in %d\n", poll(&p, 1, 0), (p.revents & POLLIN) != 0); close(fd);
    chmod("hh_dir/f.txt", 0600); stat("hh_dir/f.txt", &st); printf("mode %o\n", (unsigned)(st.st_mode & 0777));
    d = opendir("hh_dir"); while ((e = readdir(d))) if (e->d_name[0] != '.') { n++; printf("entry %s\n", e->d_name); } closedir(d);
    printf("tty %d getattr %d winsz %d\n", isatty(0), tcgetattr(0, &t) == 0, ioctl(0, TIOCGWINSZ, &w) == 0);
    unlink("hh_dir/f.txt"); printf("lstat gone %d\n", lstat("hh_dir/f.txt", &st) != 0);
    return 0;
}
