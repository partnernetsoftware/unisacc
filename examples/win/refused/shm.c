/* examples/win/refused/shm.c -- MEASURED FAILURE: shared memory needs six args.
 *
 * The cheapest mailbox between two Windows processes is a named section: a
 * view, a struct both sides agree on, no serialization. It is also the shape
 * an agent harness wants between a driver and the thing it drives.
 *
 * It does not work here, and the reason is arithmetic rather than policy.
 * CreateFileMappingA takes six arguments and MapViewOfFile takes five, while
 * a Windows forward delivers exactly four (outparam.c measures this: the
 * fourth register slot arrives, the fifth does not). So both calls return
 * something that is not a handle, the program prints two cheerful lines, and
 * it dies on the cleanup call with an access violation. The section was
 * never created: the name never arrived.
 *
 * What replaces it: mailbox.c, which uses stdio and a file, and works today.
 * The section-based mailbox becomes available the moment the host-call
 * template grows the stack slots -- see the note in examples/win/README.md.
 *
 * Expected: "mapping 1", "view 1", then an access violation, exit 3221225477.
 *
 *   out/ua-ref-win.exe -b win/x86_64 -o shm.exe examples/win/refused/shm.c
 */
#include <stdio.h>

#define PAGE_READWRITE 4UL
#define FILE_MAP_WRITE 4UL
#define MAILBOX_SIZE 64UL

void *CreateFileMappingA(void *file, void *attrs, unsigned long protect,
                         unsigned long max_high, unsigned long max_low,
                         const char *name);
void *MapViewOfFile(void *mapping, unsigned long access, unsigned long off_high,
                    unsigned long off_low, unsigned long bytes);
int UnmapViewOfFile(void *base);
int CloseHandle(void *handle);
unsigned long GetLastError(void);

int main(void)
{
    void *mapping;
    unsigned char *view;

    mapping = CreateFileMappingA(0, 0, PAGE_READWRITE, 0, MAILBOX_SIZE,
                                 "Local\\unisacc-win-mailbox");
    printf("mapping %d\n", mapping != 0);
    fflush(stdout);

    view = (unsigned char *)MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0,
                                          MAILBOX_SIZE);
    printf("view %d\n", view != 0);
    fflush(stdout);

    if (view != 0) {
        view[0] = 1;
        printf("unmap %d close %d\n", UnmapViewOfFile(view), CloseHandle(mapping));
    }
    return 0;
}