/* Actual production API/W prepare probe. Faults follow a real certificate; no mirrored function. */
#define US_LATE_TEST_OBSERVERS 1
#include "exec/c/libunisacc.c"
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"callback wiring failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static long hostcallback(long(*fn)(long),long x){(void)fn;return x;}
static const char real_definition[]="long hostcallback(long (*fn)(long),long value){return value;}";
static const char caller[]="long hostcallback(long (*fn)(long),long value); long guest(long x){return x+2;} long answer(void){return hostcallback(guest,40);}";
static us_native_plan *empty_attachment(unsigned kind){
 us_native_plan *p=calloc(1,sizeof *p);if(!p)return NULL;
 if(kind==2){p->attachment=calloc(1,sizeof(us_callable));p->release_attachment=us_callback_graph_release;}
 else{p->attachment=calloc(1,sizeof(UsLateTypedFrame));p->release_attachment=kind?us_late_callsite_release:us_late_typed_release;}
 if(!p->attachment){free(p);return NULL;}return p;
}
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"usage: probe DEFAULT_PRODUCT CALLBACK_PACKAGE TARGET\n");return 2;}
 us_context *sig=us_new(argv[1]);REQUIRE(sig);REQUIRE(!us_set_signature_version(sig,3));REQUIRE(!us_add_source(sig,"real-callback.c",real_definition));REQUIRE(!us_compile(sig,argv[3],0));
 size_t wn=sig->signatures_length;unsigned char *wire=malloc(wn);REQUIRE(wire&&wn>=16);memcpy(wire,sig->signatures,wn);REQUIRE(!memcmp(wire,"USLSIG3\n",8));us_free(sig);
 const int faults[]={0,1,2,7};
 for(unsigned k=0;k<sizeof faults/sizeof faults[0];k++){
  us_context*c=us_new(argv[2]);REQUIRE(c);REQUIRE(!us_set_signature_version(c,3));REQUIRE(!us_add_symbol_typed(c,"hostcallback",(void*)hostcallback,wire,wn));REQUIRE(!us_add_source(c,"caller.c",caller));REQUIRE(!us_compile(c,argv[3],0));
  static const unsigned char expected[17]={'U','S','L','S','E','L','1','\n',1,0,0,0,0,1,0,0,1};
  REQUIRE(c->late_callback_enabled&&c->late_selection_n==17&&!memcmp(c->late_selection,expected,17));REQUIRE(!c->native_plans.head);
  size_t before=us_private_typed_release_count();us_late_test_callback_carrier_status=faults[k];int rc=us_relocate(c);us_late_test_callback_carrier_status=0;
  REQUIRE(rc==1);REQUIRE(!c->native_plans.head&&!c->image&&!c->symbol_count);
  if(!faults[k]){REQUIRE(strstr(us_error(c),"not covered: callback-provider-bridge"));REQUIRE(us_private_typed_release_count()==before+1);}
  else{REQUIRE(us_private_typed_release_count()==before);
   if(faults[k]==1)REQUIRE(!strcmp(us_error(c),"test callback certificate mechanism failure"));
   else if(faults[k]==2)REQUIRE(strstr(us_error(c),"not covered: callback-native-ABI"));
   else REQUIRE(strstr(us_error(c),"internal callback-native-ABI status"));
  }
  size_t settled=us_private_typed_release_count();us_free(c);REQUIRE(us_private_typed_release_count()==settled);
  printf("callback prepare fault=%d expected-refusal rollback/count PASS\n",faults[k]);
 }
 free(wire);
 size_t n=us_private_typed_release_count();us_callback_graph_release(NULL);us_late_typed_release(NULL);us_late_callsite_release(NULL);REQUIRE(us_private_typed_release_count()==n);
 us_native_plans owner={0};owner.head=empty_attachment(0);REQUIRE(owner.head);owner.head->next=empty_attachment(1);REQUIRE(owner.head->next);owner.head->next->next=empty_attachment(2);REQUIRE(owner.head->next->next);
 us_native_plans_clear(&owner);REQUIRE(!owner.head&&us_private_typed_release_count()==n+3);us_native_plans_clear(&owner);REQUIRE(us_private_typed_release_count()==n+3);
 puts("callback wiring: real API/W path, rc partition, mixed release accounting PASS; provider/ABI unimplemented");return 0;
}
