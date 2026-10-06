/* 0.0.31 H3: the minimal pthread set, forwarded to the host.  Four threads count under a mutex,
   a condition variable hands a value over, pthread_once runs its initialiser once, a key's
   destructor runs at thread exit, a detached thread and a recursive mutex.  Output is fixed. */
#include <stdio.h>
#include <pthread.h>
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t key;
static long count; static int ready; static int value; static int inits; static int dtors;
static void init(void) { inits = inits + 1; }
static void dtor(void *p) { pthread_mutex_lock(&mu); dtors = dtors + (int)(long)p; pthread_mutex_unlock(&mu); }
static void *work(void *p) {
    int i;
    pthread_once(&once, init);
    pthread_setspecific(key, p);
    for (i = 0; i < 50000; i++) { pthread_mutex_lock(&mu); count = count + 1; pthread_mutex_unlock(&mu); }
    return (void *)((long)pthread_getspecific(key) * 10);
}
static void *producer(void *p) {
    pthread_mutex_lock(&mu); value = 42; ready = 1; pthread_cond_signal(&cv); pthread_mutex_unlock(&mu);
    return p;
}
int main(void) {
    pthread_t t[4]; pthread_t pr; pthread_attr_t at; pthread_mutex_t rm; pthread_mutexattr_t ma;
    long sum = 0; int i; void *r;
    pthread_key_create(&key, dtor);
    for (i = 0; i < 4; i++) pthread_create(&t[i], 0, work, (void *)(long)(i + 1));
    for (i = 0; i < 4; i++) { pthread_join(t[i], &r); sum = sum + (long)r; }
    printf("count %ld sum %ld inits %d dtors %d\n", count, sum, inits, dtors);
    pthread_create(&pr, 0, producer, 0);
    pthread_mutex_lock(&mu); while (!ready) pthread_cond_wait(&cv, &mu); pthread_mutex_unlock(&mu);
    pthread_join(pr, 0);
    printf("handed %d\n", value);
    pthread_mutexattr_init(&ma); pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rm, &ma);
    printf("recursive %d %d %d\n", pthread_mutex_lock(&rm), pthread_mutex_lock(&rm), pthread_mutex_trylock(&rm));
    pthread_mutex_unlock(&rm); pthread_mutex_unlock(&rm); pthread_mutex_unlock(&rm);
    pthread_mutex_destroy(&rm); pthread_mutexattr_destroy(&ma);
    pthread_attr_init(&at); pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
    printf("self %d\n", pthread_equal(pthread_self(), pthread_self()) != 0);
    pthread_attr_destroy(&at);
    pthread_key_delete(key);
    return 0;
}
