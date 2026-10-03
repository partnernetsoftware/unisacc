/* examples/win/refused/shm.c -- MEASURED FAILURE: shared memory needs six
 * arguments, and now it says so.
 *
 * The cheapest mailbox between two Windows processes is a named section: a
 * view, a struct both sides agree on, no serialization. It is also the shape an
 * agent harness wants between a driver and the thing it drives.
 *
 * CreateFileMappingA takes six arguments and MapViewOfFile takes five, and a
 * Windows host call delivers four. Before 0.0.23 this program printed two
 * cheerful lines and died on the cleanup call with an access violation: the
 * calls returned something that was not a handle, because the name never
 * arrived. It is now refused at compile time by name, which is the whole
 * improvement -- the failure moved from a corrupted frame to a diagnostic.
 *
 * What replaces it: mailbox.c, which uses stdio and a file and works today.
 * The section-based mailbox becomes available the moment the host-call
 * template grows the stack slots -- a 32-byte shadow area and rsp+0x20 /
 * rsp+0x28 -- which is a back-end change, not a policy one.
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