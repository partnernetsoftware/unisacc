/* examples/win/mailbox.c -- the working rendezvous: a file, stdio and four
 * argument kernel32 calls.
 *
 * refused/shm.c shows that a named shared-memory section is out of reach: the
 * two calls it needs take six and five arguments and a Windows forward
 * delivers four. This is the substitute that works today, and it is not a
 * consolation prize -- for a mailbox the file IS the interface.
 *
 * The protocol is one line of text per record, which keeps it readable by
 * anything (a shell, a Python agent, another unisacc program) and keeps the
 * writer's state on disk where a crashed reader cannot lose it:
 *
 *   >seq 7 command build
 *   <seq 7 ok bytes 24176
 *
 * The writer appends and flushes; the reader polls GetTickCount64 for its
 * deadline. Both sides are ≤ four argument forwards plus stdio, so every call
 * in this file is measured working (tick.c, fileio.c).
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o mailbox.exe examples/win/mailbox.c
 *   ./mailbox.exe
 */
#include <stdio.h>

#define MAILBOX "unisacc-win-mailbox.txt"
#define SEQ 7UL

unsigned long GetTickCount64(void);
int GetFileAttributesA(const char *path);
int DeleteFileA(const char *path);
int MoveFileExA(const char *from, const char *to, unsigned long flags);

static int exists(const char *path)
{
    unsigned long attr;
    attr = GetFileAttributesA(path);
    return attr != 4294967295UL;
}

int main(void)
{
    unsigned long started;
    unsigned long elapsed;
    unsigned long seq;
    FILE *f;
    int round;

    if (exists(MAILBOX)) {
        printf("stale mailbox removed %d\n", DeleteFileA(MAILBOX));
    }

    for (round = 0; round < 3; round = round + 1) {
        f = fopen(MAILBOX, "ab");
        if (f == 0) {
            printf("cannot open mailbox\n");
            return 1;
        }
        seq = SEQ + (unsigned long)round;
        fprintf(f, ">seq %lu command build\n", seq);
        fclose(f);

        started = GetTickCount64();
        elapsed = 0;
        while (elapsed < 2000UL) {
            f = fopen(MAILBOX, "rb");
            if (f != 0) {
                char line[128];
                while (fgets(line, 128, f) != 0) {
                    if (line[0] == '>' || line[1] == 's') {
                        printf("reader saw %.40s", line);
                    }
                }
                fclose(f);
            }
            elapsed = GetTickCount64() - started;
            if (round == 0) {
                break;
            }
        }
        printf("round %d elapsed %lu\n", round, elapsed);
    }

    printf("final %d\n", exists(MAILBOX));
    return 0;
}