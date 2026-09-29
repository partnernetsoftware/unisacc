/* Exact catalogue transaction/lifecycle checks; no model build or package. */
#include "exec/c/libunisacc.c"
static unsigned char *catalog_read_fixture(const char *p,size_t *n){FILE *f=fopen(p,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<0){fclose(f);return NULL;}unsigned char *b=malloc(z?(size_t)z:1);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"catalogue line %d: %s\n",__LINE__,us_error(c));goto done;}}while(0)
int main(int argc,char **argv){
 if(argc<5)return 2;int rc=1;size_t one_n,two_n,three_n;unsigned char *one=catalog_read_fixture(argv[1],&one_n),*two=catalog_read_fixture(argv[2],&two_n),*three=catalog_read_fixture(argv[3],&three_n);us_context *c=us_new("unused");if(!c||!one||!two||!three)return 2;
 CHECK(!library_callable_catalog_load(c,three,three_n));CHECK(c->callable_count==2&&c->callable_site_count==2&&!c->import_alias_count&&!library_import_alias(c,1));
 for(size_t i=0;i<three_n;i++)CHECK(library_callable_catalog_load(c,three,i)&&c->callable_count==2);
 CHECK(!library_callable_catalog_load(c,one,one_n));CHECK(c->callable_count==1&&!c->callable_site_count&&library_callable_signature(c,11));
 CHECK(!library_callable_catalog_load(c,two,two_n));CHECK(c->callable_count==2&&c->callable_site_count==2&&library_callable_signature(c,22));
 const LibraryCallableSite *a=library_callable_site(c,22,100),*b=library_callable_site(c,22,101);
 CHECK(a&&b&&a->fixed==1&&a->signature.count==2&&b->signature.count==1&&!library_callable_site(c,23,100));
 LibraryCallableDeclaration *old=c->callable_declarations;LibraryCallableSite *sites=c->callable_sites;
 for(size_t i=0;i<two_n;i++)CHECK(library_callable_catalog_load(c,two,i)&&c->callable_declarations==old&&c->callable_sites==sites&&c->callable_count==2&&c->callable_site_count==2);
 for(int i=4;i<argc;i++){
  size_t n;unsigned char *bad=catalog_read_fixture(argv[i],&n);CHECK(bad);int rejected=library_callable_catalog_load(c,bad,n);free(bad);
  CHECK(rejected&&c->callable_declarations==old&&c->callable_sites==sites&&library_callable_site(c,22,100)==a&&!c->import_alias_count);
 }
 for(int repeat=0;repeat<100;repeat++){
  CHECK(!library_callable_catalog_load(c,one,one_n)&&c->callable_count==1&&!c->callable_site_count&&!library_callable_site(c,22,100));
  CHECK(!library_callable_catalog_load(c,two,two_n)&&c->callable_count==2&&c->callable_site_count==2);
  discard_image(c);CHECK(c->callable_count==2&&c->callable_site_count==2);
 }
 invalidate_bindings(c);CHECK(!c->callable_count&&!c->callable_site_count&&!c->callable_declarations&&!c->callable_sites);
 CHECK(!library_callable_catalog_load(c,two,two_n));
 puts("CALL1/CALL2/CALL3: fixed/variadic/zero-tail/empty-alias maps, all truncations/malformed atomic, replacement/re-relocate/clear/free lifecycle passed");rc=0;
done:us_free(c);free(one);free(two);return rc;
}
