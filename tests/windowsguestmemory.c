/* Native Win32 allocation/lifetime adapter, independent of compiler models. */
#include "../exec/c/libunisacc.c"
#define REQUIRE(x) do {if(!(x)){fprintf(stderr,"failed line %d error %lu\n",__LINE__,GetLastError());return 1;}}while(0)
int main(void){
 us_context c={0};active=&c;size_t page=library_page_size();
 REQUIRE(page>0);
 int64_t p=library_mmap(0,page*3,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE,0,0);
 REQUIRE(p>0 && c.guest_maps && !c.guest_maps->next);
 ((unsigned char *)(uintptr_t)p)[0]=19;((unsigned char *)(uintptr_t)p)[page*3-1]=23;
 REQUIRE(library_munmap(p+page,page)==-1 && c.guest_maps);
 REQUIRE(library_munmap(p,0)==0 && !c.guest_maps);
 MEMORY_BASIC_INFORMATION info;REQUIRE(VirtualQuery((void *)(uintptr_t)p,&info,sizeof info) && info.State==MEM_FREE);
 p=library_mmap(0,page*3,MEM_RESERVE,PAGE_READWRITE,0,0);REQUIRE(p>0);
 REQUIRE(library_mmap(p+page+1,1,MEM_COMMIT,PAGE_READWRITE,0,0)==p+page);
 ((unsigned char *)(uintptr_t)p)[page]=42;
 REQUIRE(library_mmap(p+page*3,page,MEM_COMMIT,PAGE_READWRITE,0,0)==0);
 void *foreign=VirtualAlloc(0,page,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);REQUIRE(foreign);
 REQUIRE(library_mmap((int64_t)(uintptr_t)foreign,page,MEM_COMMIT,PAGE_READWRITE,0,0)==0);
 REQUIRE(library_munmap((int64_t)(uintptr_t)foreign,page)==-1);
 REQUIRE(VirtualFree(foreign,0,MEM_RELEASE));
 REQUIRE(library_mmap(0,page,((int64_t)1<<32)|MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE,0,0)==0);
 REQUIRE(library_mmap(0,page,MEM_RESERVE|MEM_COMMIT,((int64_t)1<<32)|PAGE_READWRITE,0,0)==0);
 REQUIRE(library_mmap(0,-1,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE,0,0)==0);
 REQUIRE(library_mmap(0,page,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE,1,0)==0);
 REQUIRE(library_mmap(0,page,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE,0,1)==0);
 discard_image(&c);REQUIRE(!c.guest_maps);
 REQUIRE(VirtualQuery((void *)(uintptr_t)p,&info,sizeof info) && info.State==MEM_FREE);
 active=0;puts("Windows owned reserve/commit/release: 15 controls passed");return 0;
}
