/* macOS-only collector, built with system cc. Prints THIS collector's real
 * mappings in /proc/maps syntax for memmap.c; it does not inspect the analyser.
 * No synthetic regions and no compiler/runtime change. */
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>

int main(void)
{
    mach_vm_address_t address = 0;
    int count = 0;
    for (;;) {
        mach_vm_size_t size = 0;
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t n = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t object = MACH_PORT_NULL;
        kern_return_t rc = mach_vm_region(mach_task_self(), &address, &size,
            VM_REGION_BASIC_INFO_64, (vm_region_info_t)&info, &n, &object);
        if (object != MACH_PORT_NULL) mach_port_deallocate(mach_task_self(), object);
        if (rc == KERN_INVALID_ADDRESS) break;
        if (rc != KERN_SUCCESS || size == 0 || address + size <= address) {
            fprintf(stderr, "selfmaps: region query failed (%d)\n", rc);
            return 1;
        }
        if (++count > 2048) { fprintf(stderr, "selfmaps: too many regions\n"); return 1; }
        printf("%llx-%llx %c%c%cp %llx 00:00 0\n",
            (unsigned long long)address, (unsigned long long)(address + size),
            info.protection & VM_PROT_READ ? 'r' : '-',
            info.protection & VM_PROT_WRITE ? 'w' : '-',
            info.protection & VM_PROT_EXECUTE ? 'x' : '-',
            (unsigned long long)info.offset);
        address += size;
    }
    return count ? 0 : 1;
}
