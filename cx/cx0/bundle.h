/* CXIF v0 module bundle src-sha256 2c0e18b5a715e447 -- generated, do not edit */
#ifndef CXIF_bundle_2c0e18b5a715e447
#define CXIF_bundle_2c0e18b5a715e447
typedef struct {int language;const unsigned char *name;size_t name_bytes;const unsigned char *source;size_t source_bytes;const unsigned char *includes;int include_count;} cxb_unit;
typedef struct {int kind;const unsigned char *name;size_t name_bytes;const unsigned char *data;size_t bytes;} cxb_resource;
typedef struct {unsigned char *storage;size_t bytes;int unit_count,resource_count;cxb_unit *units;cxb_resource *resources;char target[32],abi[40],model[65];} cxb_bundle;
typedef struct {const char *target,*abi,*model;} cxb_options;
typedef struct {int kind;const char *name;const void *data;size_t bytes;} cxb_input;
typedef struct {int status;unsigned char *data;size_t bytes;char why[256];} cxb_result;
void cxb_clear(cxb_bundle *b);
void cxb_release(cxb_result *r);
int cxb_read(const void *input,size_t n,cxb_bundle *out,char *why,size_t cap);
cxb_result cxb_seal(char **sources,int count,const char *directory,const cxb_options *options,const cxb_input *extra,int nextra);
#endif
