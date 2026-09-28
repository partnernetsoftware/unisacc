/* Test-only native process: no Python/ctypes allocation during long measurement.
   Includes a private runtime copy to inspect transaction ownership, not a public API. */
#include "exec/c/libunisacc.c"
#ifdef __APPLE__
#include <mach/mach.h>
#include <malloc/malloc.h>
#endif
static unsigned long long samples[13][4];
static unsigned long long resident(void) {
#ifdef __APPLE__
 mach_task_basic_info_data_t t;mach_msg_type_number_t n=MACH_TASK_BASIC_INFO_COUNT;
 if(task_info(mach_task_self(),MACH_TASK_BASIC_INFO,(task_info_t)&t,&n))return 0;
 return t.resident_size;
#else
 FILE*f=fopen("/proc/self/statm","r");unsigned long long total=0,pages=0;
 if(!f)return 0;int ok=fscanf(f,"%llu %llu",&total,&pages);fclose(f);
 return ok==2?pages*(unsigned long long)sysconf(_SC_PAGESIZE):0;
#endif
}
int main(int argc,char **argv) {
 if(argc!=2)return 2;
 us_context*c=us_new(argv[1]);if(!c)return 3;
 if(us_add_source(c,"hosted.c","int main(int argc,char **argv){return argc+37;}"))return 4;
 const char *const args[]={"hosted","arg",NULL};
 for(int k=0;k<1300;k++) {
  if(us_compile(c,library_native_target(),0)){fprintf(stderr,"compile: %s\n",us_error(c));return 5;}
  int status=0;if(us_run_main(c,2,args,&status)||status!=39)return 6;
  if(allocations)return 7;
  if((k+1)%100==0) {
   int i=k/100;samples[i][0]=resident();if(!samples[i][0])return 8;
#ifdef __APPLE__
   malloc_statistics_t m;malloc_zone_statistics(NULL,&m);
   samples[i][1]=m.blocks_in_use;samples[i][2]=m.size_in_use;samples[i][3]=m.size_allocated;
#endif
  }
 }
 us_free(c);
 unsigned long long lo=samples[3][0],hi=lo;
 for(int i=3;i<13;i++){if(samples[i][0]<lo)lo=samples[i][0];if(samples[i][0]>hi)hi=samples[i][0];}
 printf("{\"warmup_iterations\":300,\"actual_iterations\":1000,\"runtime_allocations_after_each_call\":0,\"resident_range_bytes\":%llu,\"tolerance_bytes\":524288,\"samples\":[",hi-lo);
 for(int i=0;i<13;i++)printf("%s[%d,%llu,%llu,%llu,%llu]",i?",":"",100*(i+1),samples[i][0],samples[i][1],samples[i][2],samples[i][3]);
 puts("]}");
 if(hi-lo>524288){fprintf(stderr,"resident growth beyond 512KiB warm plateau\n");return 9;}
 return 0;
}
