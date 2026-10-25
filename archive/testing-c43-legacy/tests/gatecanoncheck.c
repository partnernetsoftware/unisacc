/* C API controls for the native stamp adapter.
 * Build with the chosen pythonM.m-config --includes and --embed --ldflags.
 * usage: gatecanoncheck ABS_PYTHON_HOME ABS_SCRATCH_ROOT [ABS_CAPTURE ABS_SHA]
 * Parent runs in its existing isolation with an outer bound <=55 seconds.
 * Imports existing json/hashlib/gatequeue; does not launch suites or fingerprint.
 * Only oracle is existing json.dumps(sort_keys=True) + gatequeue.portable + hashlib.
 * No embedded Python source, exec/eval, or duplicate Merkle implementation.
 */
#define _POSIX_C_SOURCE 200809L
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static PyObject *clear_target;
static PyObject *cb_noop(PyObject *self,PyObject *text) {
    (void)self; return Py_NewRef(text);
}
static PyObject *cb_clear(PyObject *self,PyObject *text) {
    (void)self;
    if(!clear_target){PyErr_SetString(PyExc_RuntimeError,"missing clear target");return NULL;}
    if(PyList_SetSlice(clear_target,0,PyList_Size(clear_target),NULL))return NULL;
    return Py_NewRef(text);
}
static PyObject *cb_error(PyObject *self,PyObject *text) {
    (void)self;(void)text;PyErr_SetString(PyExc_RuntimeError,"intentional portable failure");return NULL;
}
static PyMethodDef noop_def={"probe_noop",cb_noop,METH_O,NULL};
static PyMethodDef clear_def={"probe_clear",cb_clear,METH_O,NULL};
static PyMethodDef error_def={"probe_error",cb_error,METH_O,NULL};
static char *read_file(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb");char *p;long n;
    if(!f)return NULL;
    if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||n>16000000||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    p=malloc((size_t)n+1);if(!p){fclose(f);return NULL;}
    if(fread(p,1,(size_t)n,f)!=(size_t)n||ferror(f)){free(p);fclose(f);return NULL;}
    fclose(f);p[n]=0;*size=(size_t)n;return p;
}
static int equal(PyObject *a,PyObject *b) { return PyObject_RichCompareBool(a,b,Py_EQ)==1; }
static PyObject *legacy(PyObject *stamper,PyObject *value) {
    return PyObject_CallFunctionObjArgs(stamper,value,Py_None,NULL);
}
static PyObject *whole(PyObject *stamper,PyObject *value) {
    PyObject *kind=PyUnicode_FromString("whole"),*r;
    if(!kind)return NULL;r=PyObject_CallFunctionObjArgs(stamper,value,kind,NULL);Py_DECREF(kind);return r;
}
static PyObject *factory(PyObject *module,PyObject *portable) {
    return PyObject_CallMethod(module,"stamper","O",portable);
}
static PyObject *old_sha(PyObject *json,PyObject *hashlib,PyObject *portable,PyObject *value) {
    PyObject *dumps=NULL,*args=NULL,*kw=NULL,*canonical=NULL,*normal=NULL,*bytes=NULL,*hash=NULL,*answer=NULL;
    dumps=PyObject_GetAttrString(json,"dumps");args=PyTuple_Pack(1,value);kw=PyDict_New();
    if(!dumps||!args||!kw||PyDict_SetItemString(kw,"sort_keys",Py_True))goto done;
    canonical=PyObject_Call(dumps,args,kw);if(!canonical)goto done;
    normal=PyObject_CallOneArg(portable,canonical);if(!normal)goto done;
    bytes=PyUnicode_AsEncodedString(normal,"utf-8","strict");if(!bytes)goto done;
    hash=PyObject_CallMethod(hashlib,"sha256","O",bytes);if(!hash)goto done;
    answer=PyObject_CallMethod(hash,"hexdigest",NULL);
 done:
    Py_XDECREF(hash);Py_XDECREF(bytes);Py_XDECREF(normal);Py_XDECREF(canonical);
    Py_XDECREF(kw);Py_XDECREF(args);Py_XDECREF(dumps);return answer;
}
static PyObject *loads(PyObject *json,const char *s,size_t n) {
    PyObject *text=PyUnicode_DecodeUTF8(s,(Py_ssize_t)n,"strict"),*r;
    if(!text)return NULL;r=PyObject_CallMethod(json,"loads","O",text);Py_DECREF(text);return r;
}
#define REQUIRE(x,msg) do {if(!(x)){fprintf(stderr,"FAIL: %s\n",msg);goto done;}}while(0)
#define DROP(x) do {Py_CLEAR(x);}while(0)
int main(int argc,char **argv) {
    PyConfig config;PyStatus status;int initialized=0,rc=1;size_t n=0,en=0;
    char *raw=NULL,*expected_raw=NULL,*program=NULL,*tests_path=NULL;
    PyObject *path=NULL,*entry=NULL,*native=NULL,*json=NULL,*hashlib=NULL,*queue=NULL,*gc=NULL,*portable=NULL;
    PyObject *noop=NULL,*clear=NULL,*error=NULL,*stamper=NULL,*other=NULL,*value=NULL,*top=NULL,*component=NULL;
    PyObject *a=NULL,*b=NULL,*c=NULL,*expected=NULL,*refs=NULL,*unsupported=NULL;
    Py_ssize_t before=0;
    if((argc!=3&&argc!=5)||argv[1][0]!='/'||!argv[1][1]||argv[2][0]!='/'||!argv[2][1]||
       (argc==5&&(argv[3][0]!='/'||!argv[3][1]||argv[4][0]!='/'||!argv[4][1]))) {
        fputs("usage: gatecanoncheck ABS_PYTHON_HOME ABS_SCRATCH_ROOT [ABS_CAPTURE ABS_SHA]\n",stderr);return 2;
    }
    { size_t h=strlen(argv[1]),r=strlen(argv[2]);int written;
      if(h>(size_t)-1-64||r>(size_t)-1-7){fputs("path size overflow\n",stderr);return 2;}
      program=malloc(h+64);tests_path=malloc(r+7);
      if(!program||!tests_path){free(program);free(tests_path);fputs("path allocation failed\n",stderr);return 2;}
      written=snprintf(program,h+64,"%s/bin/python%d.%d",argv[1],PY_MAJOR_VERSION,PY_MINOR_VERSION);
      if(written<0||(size_t)written>=h+64){free(program);free(tests_path);return 2;}
      memcpy(tests_path,argv[2],r);memcpy(tests_path+r,"/tests",7);
    }
    PyConfig_InitIsolatedConfig(&config);config.write_bytecode=0;config.site_import=0;
    status=PyConfig_SetBytesString(&config,&config.home,argv[1]);
    if(PyStatus_Exception(status))goto config_error;
    status=PyConfig_SetBytesString(&config,&config.program_name,program);
    if(PyStatus_Exception(status))goto config_error;
    status=Py_InitializeFromConfig(&config);
    if(PyStatus_Exception(status))goto config_error;
    initialized=1;PyConfig_Clear(&config);
    path=PySys_GetObject("path");REQUIRE(path,"sys.path");
    entry=PyUnicode_DecodeFSDefault(tests_path);REQUIRE(entry,"tests path string");
    REQUIRE(PyList_Insert(path,0,entry)==0,"insert tests path");DROP(entry);
    json=PyImport_ImportModule("json");hashlib=PyImport_ImportModule("hashlib");
    queue=PyImport_ImportModule("gatequeue");gc=PyImport_ImportModule("gc");
    REQUIRE(json&&hashlib&&queue&&gc,"import existing oracle and queue modules");
    native=PyObject_GetAttrString(queue,"_native_stamp");
    REQUIRE(native&&PyModule_Check(native),"queue native module available");
    portable=PyObject_GetAttrString(queue,"portable");REQUIRE(portable,"gatequeue.portable");
    noop=PyCFunction_New(&noop_def,NULL);clear=PyCFunction_New(&clear_def,NULL);error=PyCFunction_New(&error_def,NULL);
    REQUIRE(noop&&clear&&error,"C callbacks");
    if(argc==5) {
    stamper=factory(native,portable);REQUIRE(stamper,"real portable factory");
    raw=read_file(argv[3],&n);REQUIRE(raw,"read captured input");
    value=loads(json,raw,n);REQUIRE(value,"parse captured input using json");
    a=legacy(stamper,value);b=old_sha(json,hashlib,portable,value);REQUIRE(a&&b&&equal(a,b),"captured native legacy vs json/hashlib");
    expected_raw=read_file(argv[4],&en);REQUIRE(expected_raw&&en>=64,"read captured legacy oracle digest");
    expected=PyUnicode_FromStringAndSize(expected_raw,64);REQUIRE(expected&&equal(a,expected),"captured v1 SHA file");
    DROP(expected);DROP(a);DROP(b);DROP(value);DROP(stamper);
    puts("PASS: optional captured legacy SHA against stdlib and supplied historical digest");
    }
    /* Unicode includes lone surrogate and astral; all are decoded by existing json. */
    { const char *text="{\"z\":[1.0,-0.0,1e-7,1e20,5e-324,1.7976931348623157e308],\"\\ud800\":\"\\udc80\",\"\\ue000\":\"\\ud800\\udc00\",\"integer\":123456789012345678901234567890123456789012345678901234567890,\"a\":true}";
      value=loads(json,text,strlen(text)); }
    REQUIRE(value,"Unicode/float/int fixture parse");stamper=factory(native,portable);REQUIRE(stamper,"data factory");
    a=legacy(stamper,value);b=old_sha(json,hashlib,portable,value);REQUIRE(a&&b&&equal(a,b),"Unicode/float/big-int old SHA comparison");
    DROP(a);DROP(b);DROP(value);DROP(stamper);
    /* Frozen vectors independently produced from explicit wire bytes + OpenSSL. */
    { static const char *vectors[][3] = {
        {"[{\"z\":2,\"a\":1},[\"tail\",true]]", "whole", "ae608ba5f75570a37fba8fdfcf146a7afc2df12b774f4ce38410cd6c9b3345f9"},
        {"[{\"z\":2,\"a\":1},[\"tail\",true]]", "narrow", "08921b400f460e3e9644fc321f3c69065994f4fd8bf8ad040e4ecb9a5e00b215"},
        {"[[\"tail\",true],{\"z\":2,\"a\":1}]", "whole", "5486e4c26a26c8a6b075997cda4156e04aab64302807f50a7357e6e8f2b4ea4a"},
        {"[{\"z\":2,\"a\":1},[\"tail\",true],{\"z\":2,\"a\":1}]", "whole", "16bf890dc388c12b7ebaea0008843cfb5550fc768e587d16c7988b4774f0fe16"},
        {"[]", "whole", "c0d51636ca0dae19545c5d950880fc6b3a0eb0f26161494e9e214f1c4b376f2c"},
      };
      stamper=factory(native,noop);REQUIRE(stamper,"wire factory");
      for(size_t i=0;i<sizeof vectors/sizeof vectors[0];i++) {
        value=loads(json,vectors[i][0],strlen(vectors[i][0]));entry=PyUnicode_FromString(vectors[i][1]);
        REQUIRE(value&&entry,"wire fixture");
        a=PyObject_CallFunctionObjArgs(stamper,value,entry,NULL);b=PyUnicode_FromString(vectors[i][2]);
        REQUIRE(a&&b&&equal(a,b),"independent OpenSSL wire digest");
        DROP(value);DROP(entry);DROP(a);DROP(b);
      }
      DROP(stamper);puts("PASS: five independent OpenSSL identity wire vectors");
    }
    /* Top list owns its only component reference when callback clears it. */
    top=PyList_New(0);component=PyDict_New();REQUIRE(top&&component,"snapshot setup");
    value=PyLong_FromLong(7);REQUIRE(value&&PyDict_SetItemString(component,"x",value)==0,"snapshot component");DROP(value);
    REQUIRE(PyList_Append(top,component)==0,"snapshot append");DROP(component);
    stamper=factory(native,noop);other=factory(native,clear);REQUIRE(stamper&&other,"snapshot factories");
    a=whole(stamper,top);REQUIRE(a,"noop baseline");DROP(stamper); /* baseline no longer pins component */
    clear_target=Py_NewRef(top);b=whole(other,top);DROP(clear_target);
    REQUIRE(b&&PyList_Size(top)==0&&equal(a,b),"clearing portable retains top snapshot and component ownership");
    DROP(a);DROP(b);DROP(other);DROP(top);
    /* Failure must propagate original exception, retaining no component cache. */
    component=PyDict_New();top=PyList_New(0);REQUIRE(component&&top,"error setup");
    REQUIRE(PyList_Append(top,component)==0,"error append");before=Py_REFCNT(component);
    stamper=factory(native,error);REQUIRE(stamper,"error factory");a=whole(stamper,top);
    REQUIRE(!a&&PyErr_ExceptionMatches(PyExc_RuntimeError),"RuntimeError from portable propagated");PyErr_Clear();
    REQUIRE(Py_REFCNT(component)==before,"error path released temporary component ownership");
    DROP(stamper);REQUIRE(Py_REFCNT(component)==before,"factory destruction after error releases refs");
    DROP(top);DROP(component);
    /* Successful component cache has exactly one extra owning reference. */
    component=PyDict_New();top=PyList_New(0);REQUIRE(component&&top,"cache setup");
    REQUIRE(PyList_Append(top,component)==0,"cache append");before=Py_REFCNT(component);
    stamper=factory(native,noop);REQUIRE(stamper,"cache factory");a=whole(stamper,top);REQUIRE(a,"cache stamp");
    REQUIRE(Py_REFCNT(component)==before+1,"component cache INCREF");
    b=whole(stamper,top);REQUIRE(b&&equal(a,b)&&Py_REFCNT(component)==before+1,"repeat cache adds no refs");
    refs=PyObject_CallMethod(gc,"get_referents","O",stamper);REQUIRE(refs&&PyList_Check(refs),"GC referents");
    { int found=0;for(Py_ssize_t i=0;i<PyList_Size(refs);i++)if(PyList_GetItem(refs,i)==(PyObject *)Py_TYPE(stamper))found=1;
      REQUIRE(found,"tp_traverse reports owning heap type reference"); }
    DROP(refs);DROP(stamper);REQUIRE(Py_REFCNT(component)==before,"cache destruction restores component refcount");
    DROP(a);DROP(b);DROP(top);DROP(component);
    /* Nonfatal refusal: process continues after each error. Break cycle ourselves. */
    stamper=factory(native,noop);REQUIRE(stamper,"rejection factory");value=PyList_New(0);REQUIRE(value,"cycle setup");
    REQUIRE(PyList_Append(value,value)==0,"self cycle");a=legacy(stamper,value);
    REQUIRE(!a&&PyErr_ExceptionMatches(PyExc_ValueError),"cycle raises ValueError");PyErr_Clear();
    REQUIRE(PyList_SetSlice(value,0,PyList_Size(value),NULL)==0,"break cycle");DROP(value);
    unsupported=PySet_New(NULL);REQUIRE(unsupported,"unsupported set setup");a=legacy(stamper,unsupported);
    REQUIRE(!a&&PyErr_ExceptionMatches(PyExc_TypeError),"unsupported raises TypeError, no exit");PyErr_Clear();
    puts("PASS: data legacy SHA, callback snapshot/errors, cache refs, heap-type GC, nonfatal rejection");
    if(argc==3)puts("capture: NOT REQUESTED (no historical sample checked)");rc=0;
 done:
    if(initialized&&PyErr_Occurred())PyErr_Print();
    DROP(clear_target);DROP(unsupported);DROP(refs);DROP(expected);DROP(a);DROP(b);DROP(c);
    DROP(value);DROP(top);DROP(component);DROP(stamper);DROP(other);DROP(entry);
    DROP(noop);DROP(clear);DROP(error);DROP(portable);DROP(queue);DROP(gc);DROP(hashlib);DROP(json);DROP(native);
    free(raw);free(expected_raw);free(program);free(tests_path);
    if(initialized&&Py_FinalizeEx()<0)rc=1;
    return rc;
 config_error:
    fprintf(stderr,"Python configuration failed: %s\n",status.err_msg?status.err_msg:"unknown");
    PyConfig_Clear(&config);free(program);free(tests_path);return 2;
}
