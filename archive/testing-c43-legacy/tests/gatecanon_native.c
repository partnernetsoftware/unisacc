/* CPython host adapter for the gate fingerprint stamp operation.
 * Build through tests/gatecanon-native using the actual runtime headers.
 * No Python-runtime independence or abi3 compatibility claimed.
 * API: stamper(portable_callable, default_kind=None) -> callable(value, kind=None).
 * default_kind="auto": omitted kind + list/tuple uses the unified identity domain;
 * explicit kind=None stays legacy. K2b scope remains in the original components.
 * Default: old canonical+portable SHA; whole/narrow: gatecanon Merkle wire v1.
 * Components must remain immutable for the stamper lifetime. Use one stamper per
 * fingerprint collection, with stable portable environment. No top-list cache.
 * References are instance-owned; parts and all caller inputs remain untouched.
 */
#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
#include <errno.h>


/* The standalone JSON core exits on invalid/OOM. Adapt that boundary to Python
 * exceptions and reclaim ALL its temporary allocations, including partial dumps.
 * No Python callback occurs while this arena/setjmp boundary is active. */
typedef struct GNMem { void *p; struct GNMem *next; } GNMem;
typedef struct GNArena { jmp_buf jump; GNMem *head; PyObject *temporary; struct GNArena *previous; } GNArena;
static _Thread_local GNArena *gn_active;
static void gn_abort(void) {
    if(!PyErr_Occurred()) PyErr_SetString(PyExc_ValueError,"gatecanon core rejected the value");
    if(gn_active) longjmp(gn_active->jump,1);
    Py_FatalError("gatecanon: core failure outside conversion arena");
}
static void *gn_malloc(size_t n) {
    void *p=malloc(n?n:1); GNMem *m;
    if(!gn_active) return p;
    if(!p){PyErr_NoMemory();gn_abort();}
    m=malloc(sizeof *m);if(!m){free(p);PyErr_NoMemory();gn_abort();}
    m->p=p;m->next=gn_active->head;gn_active->head=m;return p;
}
static void gn_free(void *p) {
    if(gn_active){GNMem **at=&gn_active->head;while(*at){if((*at)->p==p){GNMem *m=*at;*at=m->next;free(m);break;}at=&(*at)->next;}}
    free(p);
}
static void *gn_calloc(size_t n,size_t size) {
    void *p;if(size && n>SIZE_MAX/size){PyErr_NoMemory();gn_abort();}
    p=gn_malloc(n*size);if(p)memset(p,0,n*size);return p;
}
static void *gn_realloc(void *p,size_t n) {
    GNMem *m;void *next;
    if(!p)return gn_malloc(n);
    if(!gn_active)return realloc(p,n);
    for(m=gn_active->head;m && m->p!=p;m=m->next){}
    if(!m){PyErr_SetString(PyExc_RuntimeError,"gatecanon arena ownership mismatch");gn_abort();}
    next=realloc(p,n?n:1);if(!next){PyErr_NoMemory();gn_abort();}m->p=next;return next;
}
static void gn_core_exit(int code) {(void)code;gn_abort();}
static void gn_arena_clear(GNArena *a) {
    GNMem *m=a->head;while(m){GNMem *next=m->next;free(m->p);free(m);m=next;}a->head=NULL;
}
#define malloc gn_malloc
#define calloc gn_calloc
#define realloc gn_realloc
#define free gn_free
#define exit gn_core_exit
#include "../include/cx.h"
#include "../tests/cxlib/pyjson.cx"
#include "cxlib/gatecanon.cx"
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef exit

/* Only conventional JSON object types; no arbitrary-object default callback. */
static pj_value *gn_convert(PyObject *v,PyObject **anc,unsigned depth) {
    pj_value *out;PyObject *encoded=NULL,*text=NULL;
    if(depth>=200){PyErr_SetString(PyExc_RecursionError,"gatecanon nesting exceeds 200");return NULL;}
    if(v==Py_None)return pj_null();
    if(PyBool_Check(v))return pj_bool(v==Py_True);
    if(PyUnicode_Check(v)){
        encoded=PyUnicode_AsEncodedString(v,"utf-8","surrogatepass");if(!encoded)return NULL;
        /* Keep the transient Python bytes reachable on core allocation failure. */
        gn_active->temporary=encoded;
        {Py_ssize_t n=PyBytes_Size(encoded);char *p=PyBytes_AsString(encoded);
            out=pj_new(PJ_STR);out->n=(size_t)n;out->s=gn_malloc((size_t)n+1);
            memcpy(out->s,p,(size_t)n);out->s[n]=0;}
        gn_active->temporary=NULL;Py_DECREF(encoded);return out;
    }
    if(PyLong_Check(v)){
        /* int.__repr__ ignores subclass __str__/__repr__ overrides as JSON does. */
        text=PyObject_CallMethod((PyObject *)&PyLong_Type,"__repr__","O",v);if(!text)return NULL;
        encoded=PyUnicode_AsEncodedString(text,"ascii","strict");Py_DECREF(text);if(!encoded)return NULL;
        gn_active->temporary=encoded;out=pj_new(PJ_INT);out->n=(size_t)PyBytes_Size(encoded);out->s=gn_malloc(out->n+1);
        memcpy(out->s,PyBytes_AsString(encoded),out->n);out->s[out->n]=0;gn_active->temporary=NULL;Py_DECREF(encoded);return out;
    }
    if(PyFloat_Check(v))return pj_float(PyFloat_AsDouble(v));
    if(PyDict_Check(v)||PyList_Check(v)||PyTuple_Check(v)){
        Py_ssize_t n,pos=0;PyObject *key,*item;
        for(unsigned i=0;i<depth;i++)if(anc[i]==v){PyErr_SetString(PyExc_ValueError,"circular reference detected");return NULL;}
        anc[depth]=v;out=PyDict_Check(v)?pj_dict():pj_list();
        if(PyDict_Check(v)){
            while(PyDict_Next(v,&pos,&key,&item)){
                pj_value *k,*value;
                if(!PyUnicode_Check(key)){PyErr_SetString(PyExc_TypeError,"gatecanon dict keys must be str");return NULL;}
                k=gn_convert(key,anc,depth+1);if(!k)return NULL;
                value=gn_convert(item,anc,depth+1);if(!value)return NULL;
                pj_append(out,k);pj_append(out,value);
            }
        }else{
            n=PyList_Check(v)?PyList_Size(v):PyTuple_Size(v);
            for(pos=0;pos<n;pos++){
                pj_value *value;item=PyList_Check(v)?PyList_GetItem(v,pos):PyTuple_GetItem(v,pos);
                if(!item)return NULL;value=gn_convert(item,anc,depth+1);if(!value)return NULL;pj_append(out,value);
            }
        }
        return out;
    }
    PyErr_SetString(PyExc_TypeError,"gatecanon accepts JSON dict/list/tuple/str/int/bool/None/float only");return NULL;
}
static PyObject *gn_canonical(PyObject *value) {
    /* Heap arena: its modified fields remain defined across longjmp. */
    GNArena *a=calloc(1,sizeof *a);PyObject *result=NULL;PyObject *anc[200];
    if(!a)return PyErr_NoMemory();a->previous=gn_active;gn_active=a;
    if(!setjmp(a->jump)){
        pj_value *tree=gn_convert(value,anc,0);
        if(tree){size_t n;char *s=gc_dump(tree,&n);
            if(n>(size_t)PY_SSIZE_T_MAX)PyErr_SetString(PyExc_OverflowError,"canonical value too large");
            else result=PyUnicode_DecodeUTF8(s,(Py_ssize_t)n,"strict");}
    }
    gn_active=a->previous;Py_XDECREF(a->temporary);gn_arena_clear(a);free(a);return result;
}

#define GN_BUCKETS 4093
typedef struct GNEntry {
    PyObject *object;char hex[65];struct GNEntry *next,*nextshape;
    uint64_t shapesum,shapexor;Py_ssize_t shapecount;
} GNEntry;
typedef struct { PyObject_HEAD PyObject *portable;int auto_kind;GNEntry *buckets[GN_BUCKETS],*shapes[GN_BUCKETS]; } GNStamper;
typedef struct { PyObject *type; } GNState;
static int gn_hash(const char *domain,size_t dn,const char *bytes,size_t n,char hex[65]) {
    char *wire;
    if(n>SIZE_MAX-dn){PyErr_SetString(PyExc_OverflowError,"hash wire too large");return -1;}
    wire=malloc(dn+n?dn+n:1);if(!wire){PyErr_NoMemory();return -1;}
    if(dn)memcpy(wire,domain,dn);if(n)memcpy(wire+dn,bytes,n);
    cx_sha256_hex(wire,dn+n,hex);free(wire);return 0;
}
static int gn_value_hash(GNStamper *self,PyObject *value,int component,char hex[65]) {
    static const char domain[]="unisacc-gate-component-v1";
    PyObject *canonical=gn_canonical(value),*normalized,*bytes;int rc;
    if(!canonical)return -1;
    normalized=PyObject_CallFunctionObjArgs(self->portable,canonical,NULL);Py_DECREF(canonical);if(!normalized)return -1;
    if(!PyUnicode_Check(normalized)){Py_DECREF(normalized);PyErr_SetString(PyExc_TypeError,"portable must return str");return -1;}
    bytes=PyUnicode_AsEncodedString(normalized,"utf-8","strict");Py_DECREF(normalized);if(!bytes)return -1;
    rc=gn_hash(component?domain:NULL,component?sizeof domain:0,PyBytes_AsString(bytes),(size_t)PyBytes_Size(bytes),hex);
    Py_DECREF(bytes);return rc;
}
/* This signature is only a bucket filter, never a proof of equality.
 * Only exact dicts with exact str keys qualify; no subclass callbacks/hash policy.
 * Shallow equality requires identical key contents and identical value pointers.
 * Both reachable values and portable environment must remain immutable. */
static uint64_t gn_mix64(uint64_t x) {
    x^=x>>30;x*=UINT64_C(0xbf58476d1ce4e5b9);
    x^=x>>27;x*=UINT64_C(0x94d049bb133111eb);return x^(x>>31);
}
static int gn_shape(PyObject *value,uint64_t *sum,uint64_t *xr,Py_ssize_t *count) {
    Py_ssize_t pos=0;PyObject *key,*item;*sum=0;*xr=0;*count=0;
    if(!PyDict_CheckExact(value))return 0;
    while(PyDict_Next(value,&pos,&key,&item)){
        Py_hash_t h;uint64_t word;
        if(!PyUnicode_CheckExact(key))return 0;
        h=PyObject_Hash(key);if(h==-1&&PyErr_Occurred())return -1;
        word=gn_mix64((uint64_t)h)^gn_mix64((uint64_t)(uintptr_t)item);
        *sum+=word;*xr^=gn_mix64(word);(*count)++;
    }
    return 1;
}
static int gn_shape_equal(PyObject *a,PyObject *b) {
    Py_ssize_t pos=0;PyObject *key,*item,*other;
    if(PyDict_Size(a)!=PyDict_Size(b))return 0;
    while(PyDict_Next(a,&pos,&key,&item)){
        other=PyDict_GetItemWithError(b,key);
        if(!other){if(PyErr_Occurred())return -1;return 0;}
        if(other!=item)return 0;
    }
    return 1;
}
static const char *gn_component(GNStamper *self,PyObject *value) {
    size_t bucket=((uintptr_t)value/sizeof(void *))%GN_BUCKETS,shape_bucket=0;
    GNEntry *e,*representative=NULL;char hex[65];uint64_t sum=0,xr=0;Py_ssize_t count=0;int shaped;
    /* Hold before canonical/portable can run Python and release caller's refs. */
    Py_INCREF(value);
    for(e=self->buckets[bucket];e;e=e->next)if(e->object==value){Py_DECREF(value);return e->hex;}
    shaped=gn_shape(value,&sum,&xr,&count);if(shaped<0){Py_DECREF(value);return NULL;}
    if(shaped){
        shape_bucket=(size_t)(gn_mix64(sum)^gn_mix64(xr)^(uint64_t)count)%GN_BUCKETS;
        for(e=self->shapes[shape_bucket];e;e=e->nextshape){
            int equal;
            if(e->shapesum!=sum||e->shapexor!=xr||e->shapecount!=count)continue;
            equal=gn_shape_equal(value,e->object);
            if(equal<0){Py_DECREF(value);return NULL;}
            if(equal){representative=e;break;}
        }
    }
    if(representative)memcpy(hex,representative->hex,sizeof hex);
    else if(gn_value_hash(self,value,1,hex)){Py_DECREF(value);return NULL;}
    e=malloc(sizeof *e);if(!e){Py_DECREF(value);PyErr_NoMemory();return NULL;}
    e->object=value;memcpy(e->hex,hex,sizeof hex);e->next=self->buckets[bucket];e->nextshape=NULL;
    e->shapesum=sum;e->shapexor=xr;e->shapecount=count;self->buckets[bucket]=e;
    if(!representative&&shaped){e->nextshape=self->shapes[shape_bucket];self->shapes[shape_bucket]=e;}
    return e->hex;
}
static PyObject *gn_call(PyObject *object,PyObject *args,PyObject *kwargs) {
    GNStamper *self=(GNStamper *)object;PyObject *value,*kind=NULL,*hashes=NULL,*wire=NULL,*canonical=NULL,*bytes=NULL,*answer=NULL,*implicit_kind=NULL,*topcopy=NULL;
    static char *names[]={"value","kind",NULL};char hex[65];
    if(!PyArg_ParseTupleAndKeywords(args,kwargs,"O|O:stamper",names,&value,&kind))return NULL;
    if(!self->portable){PyErr_SetString(PyExc_RuntimeError,"cleared stamper");return NULL;}
    /* Snapshot membership/order BEFORE portable callbacks. Components themselves
       remain shared and are governed by the immutable-collection contract. */
    if(PyList_Check(value))topcopy=PyList_AsTuple(value);
    else if(PyTuple_Check(value))topcopy=PyTuple_GetSlice(value,0,PyTuple_Size(value));
    if(PyList_Check(value)||PyTuple_Check(value)){if(!topcopy)goto done;value=topcopy;}
    if(!kind){
        if(self->auto_kind&&(PyList_Check(value)||PyTuple_Check(value))){
            implicit_kind=PyUnicode_FromString("whole");if(!implicit_kind)goto done;kind=implicit_kind;
        }else kind=Py_None;
    }
    if(kind==Py_None){if(gn_value_hash(self,value,0,hex))goto done;answer=PyUnicode_FromStringAndSize(hex,64);goto done;}
    if(!PyUnicode_Check(kind)||(PyUnicode_CompareWithASCIIString(kind,"whole")&&PyUnicode_CompareWithASCIIString(kind,"narrow"))){PyErr_SetString(PyExc_ValueError,"kind must be None, whole or narrow");goto done;}
    if(!PyTuple_Check(value)){PyErr_SetString(PyExc_TypeError,"Merkle identity must be a list or tuple");goto done;}
    hashes=PyList_New(0);if(!hashes)goto done;
    for(Py_ssize_t i=0;i<PyTuple_Size(value);i++){
        const char *h=gn_component(self,PyTuple_GetItem(value,i));PyObject *s;
        if(!h)goto done;s=PyUnicode_FromStringAndSize(h,64);if(!s)goto done;
        if(PyList_Append(hashes,s)){Py_DECREF(s);goto done;}Py_DECREF(s);
    }
    wire=PyList_New(0);if(!wire)goto done;
    if(PyList_Append(wire,kind)||PyList_Append(wire,hashes))goto done;
    canonical=gn_canonical(wire);if(!canonical)goto done;
    bytes=PyUnicode_AsEncodedString(canonical,"utf-8","strict");if(!bytes)goto done;
    {static const char domain[]="unisacc-gate-identity-v1";
        if(gn_hash(domain,sizeof domain,PyBytes_AsString(bytes),(size_t)PyBytes_Size(bytes),hex))goto done;}
    answer=PyUnicode_FromStringAndSize(hex,64);
done:
    Py_XDECREF(bytes);Py_XDECREF(canonical);Py_XDECREF(wire);Py_XDECREF(hashes);Py_XDECREF(topcopy);Py_XDECREF(implicit_kind);return answer;
}
static int gn_traverse(PyObject *object,visitproc visit,void *arg) {
    GNStamper *self=(GNStamper *)object;Py_VISIT((PyObject *)Py_TYPE(object));Py_VISIT(self->portable);
    for(size_t i=0;i<GN_BUCKETS;i++)for(GNEntry *e=self->buckets[i];e;e=e->next)Py_VISIT(e->object);
    return 0;
}
static int gn_clear(PyObject *object) {
    GNStamper *self=(GNStamper *)object;Py_CLEAR(self->portable);
    /* Shape chains are non-owning aliases of address entries; clear first. */
    memset(self->shapes,0,sizeof self->shapes);
    for(size_t i=0;i<GN_BUCKETS;i++){GNEntry *e=self->buckets[i];self->buckets[i]=NULL;
        while(e){GNEntry *next=e->next;Py_DECREF(e->object);free(e);e=next;}}
    return 0;
}
static void gn_dealloc(PyObject *object) {
    PyTypeObject *type=Py_TYPE(object);PyObject_GC_UnTrack(object);gn_clear(object);PyObject_GC_Del(object);Py_DECREF(type);
}
static PyObject *gn_no_new(PyTypeObject *type,PyObject *args,PyObject *kwargs) {
    (void)type;(void)args;(void)kwargs;PyErr_SetString(PyExc_TypeError,"use _gatecanon.stamper(portable_callable)");return NULL;
}
static PyType_Slot gn_slots[]={
    {Py_tp_call,(void *)gn_call},{Py_tp_dealloc,(void *)gn_dealloc},
    {Py_tp_traverse,(void *)gn_traverse},{Py_tp_clear,(void *)gn_clear},
    {Py_tp_new,(void *)gn_no_new},{0,NULL}
};
static PyType_Spec gn_spec={"_gatecanon.Stamper",sizeof(GNStamper),0,Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,gn_slots};
static PyObject *gn_factory(PyObject *module,PyObject *args,PyObject *kwargs) {
    GNState *state=PyModule_GetState(module);GNStamper *self;PyObject *portable,*default_kind=Py_None;int automatic=0;
    static char *names[]={"portable_callable","default_kind",NULL};
    if(!PyArg_ParseTupleAndKeywords(args,kwargs,"O|O:stamper",names,&portable,&default_kind))return NULL;
    if(default_kind!=Py_None){
        if(!PyUnicode_Check(default_kind)||PyUnicode_CompareWithASCIIString(default_kind,"auto")){PyErr_SetString(PyExc_ValueError,"default_kind must be None or auto");return NULL;}
        automatic=1;
    }
    if(!PyCallable_Check(portable)){PyErr_SetString(PyExc_TypeError,"portable must be callable");return NULL;}
    self=(GNStamper *)PyType_GenericAlloc((PyTypeObject *)state->type,0);if(!self)return NULL;
    Py_INCREF(portable);self->portable=portable;self->auto_kind=automatic;return (PyObject *)self;
}
static PyMethodDef gn_methods[]={{"stamper",(PyCFunction)(void(*)(void))gn_factory,METH_VARARGS|METH_KEYWORDS,"Create a collection-local immutable-component stamper."},{NULL,NULL,0,NULL}};
static int gn_module_traverse(PyObject *module,visitproc visit,void *arg){GNState *s=PyModule_GetState(module);Py_VISIT(s->type);return 0;}
static int gn_module_clear(PyObject *module){GNState *s=PyModule_GetState(module);Py_CLEAR(s->type);return 0;}
static struct PyModuleDef gn_module={PyModuleDef_HEAD_INIT,"_gatecanon","Experimental native stamp bridge; requires CPython.",sizeof(GNState),gn_methods,NULL,gn_module_traverse,gn_module_clear,NULL};
PyMODINIT_FUNC PyInit__gatecanon(void) {
    PyObject *module=PyModule_Create(&gn_module);GNState *state;
    if(!module)return NULL;state=PyModule_GetState(module);state->type=PyType_FromSpec(&gn_spec);
    if(!state->type){Py_DECREF(module);return NULL;}return module;
}
