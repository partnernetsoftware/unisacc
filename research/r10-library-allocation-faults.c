#include "exec/c/libunisacc-fault.c"
int main(void){
 us_context *c=us_new("unused");assert(c);active=c;
 unsigned char *p=tracked_realloc(0,16);p[0]=17;
 Allocation *a=*allocation_slot(p);review_same_realloc=1;
 assert(tracked_realloc(p,8)==p && *allocation_slot(p)==a && p[0]==17);
 uintptr_t old_address=(uintptr_t)p;review_move_realloc=1;unsigned char *q=tracked_realloc(p,64);
 assert((uintptr_t)q!=old_address && q[0]==17 && *allocation_slot(q)==a && a->p==q);
 review_fail_realloc=1;if(!setjmp(failure)){tracked_realloc(q,128);assert(0);}
 assert(strstr(c->error,"out of memory") && *allocation_slot(q)==a && q[0]==17);
 cleanup();assert(!allocations);
 review_fail_node=1;if(!setjmp(failure)){tracked_realloc(0,16);assert(0);}
 assert(!allocations);cleanup();
 if(!setjmp(failure)){tracked_calloc(SIZE_MAX,2);assert(0);}
 assert(strstr(c->error,"allocation overflow") && !allocations);
 for(int i=0;i<ALLOCATION_BUCKETS;i++)assert(!allocation_buckets[i]);
 active=0;us_free(c);puts("forced realloc same/move/fail, node failure and overflow cleanup: ok");return 0;
}
