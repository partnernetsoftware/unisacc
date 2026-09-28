/* Explicit capability paths only; no raw callback execution or registry mock. */
#include "exec/c/librarynative.h"
#include "exec/c/libraryresolver.h"
static unsigned char *readfile(const char *p,size_t *n){FILE *f=fopen(p,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<0){fclose(f);return NULL;}unsigned char *b=malloc(z?(size_t)z:1);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"callback plan line %d: %s\n",__LINE__,error);goto done;}}while(0)
int main(int argc,char **argv){
 if(argc!=6)return 2;unsigned char *data[5]={0};size_t n[5]={0};int rc=1;char error[160]={0};
 us_exports graph={0},other={0};us_bindings bindings={0},oldbindings={0};us_resolver resolver={0};us_native_plans plans={0},oldplans={0},frozenplans={0};us_native_templates templates={0};unsigned char *wire=NULL;size_t length=0;uint64_t handle=0;
 for(int i=0;i<5;i++){data[i]=readfile(argv[i+1],&n[i]);CHECK(data[i]);}
 CHECK(us_exports_load(&other,data[0],n[0],error,sizeof error));
 CHECK(!us_exports_load_bridge(&graph,data[0],n[0],error,sizeof error));
 CHECK(graph.count==1&&us_export_bridge_supported(graph.items)&&!us_export_supported(graph.items));
 CHECK(!graph.items[0].argtypes[0].ffi&&graph.items[0].argtypes[0].signature->argtypes[0].signature->count==9);
 for(int i=1;i<=2;i++){
  CHECK(!us_exports_load_bridge(&other,data[i],n[i],error,sizeof error));
  CHECK(!us_binding_graph_equal(&graph.items[0].argtypes[0],&other.items[0].argtypes[0]));us_exports_clear(&other);
 }
 us_export *retained=graph.items;
 for(size_t i=0;i<n[0];i++)CHECK(us_exports_load_bridge(&graph,data[0],i,error,sizeof error)&&graph.items==retained);
 CHECK(us_native_plan_add(&oldplans,3,data[0],n[0],&handle,error,sizeof error)&&!handle&&!oldplans.head);
 CHECK(!us_native_plan_add_bridge(&plans,3,data[0],n[0],&handle,error,sizeof error)&&handle);
 us_native_plan *p=us_native_plan_find(&plans,handle);CHECK(p&&p->bridge_required&&p->signature.count==2&&!p->args);
 us_native_arena dummy={0};dummy.plan=p;CHECK(us_native_call(&dummy));
 CHECK(us_bindings_add_function_typed(&oldbindings,"host_drive",3,data[0],n[0],error,sizeof error)&&!oldbindings.count);
 CHECK(!us_bindings_add_function_typed_bridge(&bindings,"host_drive",3,data[0],n[0],error,sizeof error)&&bindings.count==1&&bindings.items[0].supported);
 CHECK(!us_resolver_declare_typed_bridge(&resolver,&bindings,"host_drive",data[0],n[0],error,sizeof error));uint64_t generation=resolver.generation;
 for(int i=1;i<=2;i++)CHECK(us_resolver_accept_injection_typed_bridge(&resolver,"host_drive",data[i],n[i],error,sizeof error)&&resolver.generation==generation);
 CHECK(!us_resolver_accept_injection_typed_bridge(&resolver,"host_drive",data[0],n[0],error,sizeof error));
 CHECK(!us_resolver_freeze_with_templates_bridge(&resolver,&bindings,123,456,&frozenplans,&templates,&wire,&length,error,sizeof error));
 CHECK(length>16&&!memcmp(wire,"USBIND3\n",8)&&wire[length-1]==1&&frozenplans.head&&frozenplans.head->bridge_required);
 free(wire);wire=NULL;us_native_plans_clear(&frozenplans);us_native_templates_clear(&templates);
 CHECK(us_resolver_freeze_with_templates(&resolver,&bindings,123,456,&frozenplans,&templates,&wire,&length,error,sizeof error));
 /* A variadic callback candidate remains unsupported, not a fatal freeze error. */
 data[0][36]=1;data[0][37]=1;data[0][n[0]-1]=0;
 us_bindings variadic={0};us_resolver_bytes bytes={calloc(1,16),16,0};
 CHECK(bytes.p&&!us_bindings_add_function_typed_bridge(&variadic,"host_drive",3,data[0],n[0],error,sizeof error));
 int candidate=us_resolver_candidate3_bridge(&bytes,variadic.items,3,0,0,123,456,&frozenplans,&templates,error,sizeof error);
 int supported=bytes.n>16 ? bytes.p[bytes.n-1]:1;free(bytes.p);us_bindings_clear(&variadic);
 CHECK(!candidate&&!supported&&!frozenplans.head&&!templates.head);
 data[0][36]=0;data[0][37]=0;data[0][n[0]-1]=1;
 CHECK(!us_native_plan_add_bridge(&oldplans,3,data[3],n[3],&handle,error,sizeof error)&&!handle);
 CHECK(!us_native_plan_add(&oldplans,3,data[4],n[4],&handle,error,sizeof error)&&handle&&oldplans.head&&!oldplans.head->bridge_required);
 puts("callback plan: bridge-only full Relay/Leaf9, nested mismatch, unknown refused, legacy plain, atomic load and explicit resolver freeze passed");rc=0;
done:free(wire);us_exports_clear(&graph);us_exports_clear(&other);us_bindings_clear(&bindings);us_bindings_clear(&oldbindings);us_resolver_clear(&resolver);us_native_plans_clear(&plans);us_native_plans_clear(&oldplans);us_native_plans_clear(&frozenplans);us_native_templates_clear(&templates);for(int i=0;i<5;i++)free(data[i]);return rc;
}
