/* examples/win/fileio.c -- files, and the honest edge of the forward.
 *
 * Everything here works: attribute queries, a rename, a delete, a directory
 * create and remove, plus a stdio round trip. What does NOT work is
 * handle-based Win32 I/O, because CreateFileA takes seven arguments and a
 * forward past six arguments needs the libffi bridge, which does not exist on
 * Windows (examples/win/refused/createfile7.c is that boundary, measured).
 * So the honest shape of file I/O in this compiler today is: stdio for
 * content, kernel32 for metadata and naming, and examples/win/shm.c for
 * everything that genuinely needs a handle.
 *
 * Exercises: 1, 2 and 3 argument forwards, a void return, the INVALID_FILE_
 * ATTRIBUTES sentinel, and a flag argument (MOVEFILE_REPLACE_EXISTING).
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o fileio.exe examples/win/fileio.c
 *   ./fileio.exe
 */
#include <stdio.h>

#define INVALID_FILE_ATTRIBUTES 4294967295UL
#define MOVEFILE_REPLACE_EXISTING 1UL
#define FILE_ATTRIBUTE_DIRECTORY 16UL

unsigned long GetFileAttributesA(const char *path);
int CreateDirectoryA(const char *path, void *security);
int RemoveDirectoryA(const char *path);
int DeleteFileA(const char *path);
int MoveFileExA(const char *from, const char *to, unsigned long flags);
unsigned long GetLastError(void);

int main(void)
{
    const char *dir = "unisacc-win-probe-dir";
    const char *name = "unisacc-win-probe.txt";
    const char *moved = "unisacc-win-probe-moved.txt";
    unsigned long attr;
    FILE *f;
    char line[64];
    int i;

    if (CreateDirectoryA(dir, 0)) {
        printf("mkdir %s ok\n", dir);
    } else {
        printf("mkdir %s failed %lu\n", dir, GetLastError());
    }
    attr = GetFileAttributesA(dir);
    printf("dir attr isdir %d\n",
           attr != INVALID_FILE_ATTRIBUTES &&
           (attr & FILE_ATTRIBUTE_DIRECTORY) != 0);

    f = fopen(name, "wb");
    if (f == 0) {
        printf("fopen %s failed\n", name);
        return 1;
    }
    for (i = 0; i < 3; i = i + 1) {
        fprintf(f, "line %d\n", i);
    }
    fclose(f);

    attr = GetFileAttributesA(name);
    printf("file attr exists %d\n", attr != INVALID_FILE_ATTRIBUTES);

    f = fopen(name, "rb");
    if (f == 0) {
        printf("reopen failed\n");
        return 1;
    }
    i = 0;
    while (fgets(line, 64, f) != 0) {
        printf("read %d %s", i, line);
        i = i + 1;
    }
    fclose(f);

    if (MoveFileExA(name, moved, MOVEFILE_REPLACE_EXISTING)) {
        printf("move ok\n");
    } else {
        printf("move failed %lu\n", GetLastError());
    }
    printf("old gone %d\n", GetFileAttributesA(name) == INVALID_FILE_ATTRIBUTES);
    printf("new there %d\n", GetFileAttributesA(moved) != INVALID_FILE_ATTRIBUTES);

    if (DeleteFileA(moved)) {
        printf("delete ok\n");
    } else {
        printf("delete failed %lu\n", GetLastError());
    }
    if (RemoveDirectoryA(dir)) {
        printf("rmdir ok\n");
    } else {
        printf("rmdir failed %lu\n", GetLastError());
    }
    return 0;
}
