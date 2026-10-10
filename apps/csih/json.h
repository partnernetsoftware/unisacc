/* json.h — JSON 树类型与 json.c 对外函数的公共声明（仅声明，无函数体）。
 * 由 csih.sh 以 -include 注入每个翻译单元；消费方与 json.c 均不 #include 本文件。
 * 单一真源：jvalue 的布局只在此定义，禁止各模块重述副本。 */
#ifndef CSIH_JSON_H
#define CSIH_JSON_H

#include <stddef.h>

typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jkind;

typedef struct jvalue {
    jkind kind;
    int    b;
    double n;
    char  *s;
    struct jvalue **items;  size_t len;
    char **keys; struct jvalue **vals; size_t nkeys;
} jvalue;

jvalue *json_parse(const char *text, size_t len, char *errbuf, size_t errlen);
size_t  json_value_end(const char *text, size_t len);
void    jfree(jvalue *v);
size_t  json_escape(const char *in, char *out, size_t outlen, size_t *in_used);
size_t  json_rec(char *out, size_t cap,
                 const char *k1, const char *v1,
                 const char *k2, const char *v2,
                 const char *k3, const char *v3);
size_t  json_msg(char *buf, size_t cap, int comma,
                 const char *role, const char *lead, const char *content);
size_t  json_model(char *out, size_t cap, const char *msgs, size_t n);
jvalue *jget(jvalue *obj, const char *key);
const char *jstr(jvalue *v);
double  jnum(jvalue *v, double dflt);
size_t  jlen(jvalue *v);

#endif /* CSIH_JSON_H */
