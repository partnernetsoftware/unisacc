/* Host format/lifetime boundaries, not compilation or six-platform proof. */
#include "../exec/c/libunisacc.c"
#include <assert.h>
static int check_library_decode(unsigned char *b,int n) {
    us_context *c=us_new("unused");assert(c);active=c;
    Buf bytes={0};bytes.b=b;bytes.n=n;
    MemoryImage m;MemoryMap mapping={0};mapping.base=(unsigned char *)0x100000;
    volatile int ok=0;
    if (!setjmp(failure)) {library_image(c,&bytes,&m,&mapping);ok=1;}
    active=0;us_free(c);return ok;
}
int main(void) {
    unsigned char b[128]={0};memcpy(b,"UNILIB1\n",8);
    resource_u64(b+8,1);resource_u64(b+16,8);resource_u64(b+24,1);
    memcpy(b+42,"SYMS1\n",6);resource_u64(b+48,1);
    b[56]=1;resource_u64(b+57,1);resource_u64(b+65,0x104000);b[73]='g';
    assert(check_library_decode(b,74));
    for (int n=0;n<74;n++) assert(!check_library_decode(b,n));
    b[74]=0;assert(!check_library_decode(b,75));
    b[56]=2;assert(!check_library_decode(b,74));b[56]=1;
    b[73]=0;assert(!check_library_decode(b,74));b[73]='g';
    resource_u64(b+65,0x100000);assert(!check_library_decode(b,74));resource_u64(b+65,0x104000);
    resource_u64(b+48,2);memcpy(b+74,b+56,18);assert(!check_library_decode(b,92));resource_u64(b+48,1);
    us_context *c=us_new("unused");assert(c);active=c;
    long page=sysconf(_SC_PAGESIZE);assert(page>0);
    long addr=library_mmap(0,3*page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(addr>0 && c->guest_maps && c->guest_maps->length==(size_t)(3*page));
    assert(library_munmap(addr+page,page)==0);
    assert(c->guest_maps && c->guest_maps->next && !c->guest_maps->next->next);
    assert(library_munmap(addr,page)==0);
    assert(library_munmap(addr+2*page,page)==0 && !c->guest_maps);
    assert(library_munmap(addr,page)==-EINVAL);
    assert(library_mmap(addr,page,PROT_READ,MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0)==-EINVAL);
    addr=library_mmap(0,1,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(addr>0 && c->guest_maps->length==(size_t)page);
    assert(library_munmap(addr,1)==0 && !c->guest_maps);
    active=0;us_free(c);
    puts("library format: truncation, collision, bounds; guest mapping split and release: ok");return 0;
}
