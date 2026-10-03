/* examples/win/procsnap.c -- the process list, the way a task manager gets it.
 *
 * Toolhelp32 is the cheapest process enumeration Windows offers: a snapshot
 * handle, then a callback-free loop over a caller-sized structure. It is also
 * the shape a shell needs for anything that lists or targets a window's
 * owner process, so this probe is deliberately close to what a taskbar wants.
 *
 * The structure is read at explicit offsets rather than through declared
 * fields: PROCESSENTRY32A has a 4-byte hole before its ULONG_PTR member, and
 * a hand-declared struct that guesses wrong still runs. examples/win/structs.c
 * is the same idea applied to the kernel's own structures; the sizeof line
 * below is printed so the declared layout can be compared with the 304 bytes
 * the kernel writes.
 *
 * Build and run:
 *   out/ua-ref-win.exe -b win/x86_64 -o procsnap.exe examples/win/procsnap.c
 *   ./procsnap.exe
 */
#include <stdio.h>

#define TH32CS_SNAPPROCESS 2UL
#define ERROR_NO_MORE_FILES 18UL
#define ENTRY_SIZE 304UL
#define EXE_NAME_OFFSET 44UL

void *CreateToolhelp32Snapshot(unsigned long flags, unsigned long process_id);
/* Note the names: Toolhelp32 exports Process32First and Process32FirstW --
   there is no ANSI "A" variant. Asking for Process32FirstA compiles cleanly
   and fails at run time with "unisacc: no host function Process32FirstA"
   and exit status 127, because a name that is not exported cannot be
   resolved at all. */
int Process32First(void *snapshot, void *entry);
int Process32Next(void *snapshot, void *entry);
int CloseHandle(void *handle);
unsigned long GetLastError(void);

struct entry {
    unsigned int dw_size;
    unsigned int cnt_usage;
    unsigned int th32_process_id;
    unsigned long th32_default_heap_id;
    unsigned int th32_module_id;
    unsigned int cnt_threads;
    unsigned int th32_parent_process_id;
    int pc_pri_class_base;
    unsigned int dw_flags;
    unsigned char sz_exe_file[260];
};

int main(void)
{
    unsigned char buf[ENTRY_SIZE];
    struct entry *e;
    void *snapshot;
    unsigned long count;
    unsigned long i;
    unsigned long pid;
    unsigned long parent;
    unsigned long threads;

    e = (struct entry *)buf;
    printf("declared sizeof %lu kernel writes %lu\n",
           (unsigned long)sizeof(struct entry), ENTRY_SIZE);

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == 0) {
        printf("snapshot failed %lu\n", GetLastError());
        return 1;
    }

    for (i = 0; i < ENTRY_SIZE; i = i + 1) {
        buf[i] = 0;
    }
    e->dw_size = (unsigned int)ENTRY_SIZE;

    count = 0;
    if (Process32First(snapshot, e)) {
        do {
            /* Read through the byte offsets, not the declared fields. */
            pid = (unsigned long)buf[8]
                + ((unsigned long)buf[9] << 8)
                + ((unsigned long)buf[10] << 16)
                + ((unsigned long)buf[11] << 24);
            threads = (unsigned long)buf[28]
                + ((unsigned long)buf[29] << 8)
                + ((unsigned long)buf[30] << 16)
                + ((unsigned long)buf[31] << 24);
            parent = (unsigned long)buf[32]
                + ((unsigned long)buf[33] << 8)
                + ((unsigned long)buf[34] << 16)
                + ((unsigned long)buf[35] << 24);
            if (count < 8UL) {
                printf("pid %lu ppid %lu threads %lu %s\n",
                       pid, parent, threads, &buf[EXE_NAME_OFFSET]);
            }
            count = count + 1;
        } while (Process32Next(snapshot, e));
    } else {
        printf("first failed %lu\n", GetLastError());
    }

    printf("processes %lu\n", count);
    printf("more files %d\n", GetLastError() == ERROR_NO_MORE_FILES);
    printf("close %d\n", CloseHandle(snapshot));
    return 0;
}