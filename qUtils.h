/*
 * qsiFunc.h
 *
 *  Created on: Aug 20, 2013
 *      Author: lester
 */

#ifndef QSIFUNC_H_
#define QSIFUNC_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

typedef int q_bool;


#define q_false ((q_bool) 0)
#define q_true (!q_false)

#define Q_MAX_ALLOC_SIZE (1024*1024*64)  //64MB

#ifdef __GNUC__
#define Q_LIKELY(x) (__builtin_expect(!!(x),1))
#define Q_UNLIKELY(x) (__builtin_expect(!!(x),0))
#define Q_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define Q_LIKELY(x) (x)
#define Q_UNLIKELY(x) (x)
#define Q_PRETTY_FUNCTION ""
#endif

#define q_nothing() do {} while (q_false)

#ifdef Q_ASSERT
#define q_assert(expr)                                                                            \
    do {                                                                                            \
        if (Q_UNLIKELY(!(expr))) {                                                                \
            fprintf(stderr, "tm-daemon : Expr '%s' failed at %s:%u, function '%s'. Aborting\n",    \
                        #expr , __FILE__, __LINE__, Q_PRETTY_FUNCTION);                           \
            abort();                                                                                \
        }                                                                                           \
    } while (0)
#else
#define q_assert(expr) q_nothing()
#endif

#define qerror(expr, ...)                                                   \
    do {                                                                    \
        fprintf(stderr, "tm-daemon : %s,%d: ",__FUNCTION__, __LINE__);     \
        fprintf(stderr, expr,  ##__VA_ARGS__);                              \
        fprintf(stderr, "\n");                                              \
    } while (0)

#define q_dbg(expr, ...)                        \
    do {                                        \
        printf("tm-daemon : ");                \
        printf(expr,  ##__VA_ARGS__);           \
        printf("\n");                           \
    } while (0)

#ifdef __GNUC__
#define Q_MAX(a,b)                            \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a > _b ? _a : _b;                  \
        })
#define Q_MIN(a,b)                            \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a < _b ? _a : _b;                  \
        })
#else
#define Q_MAX(a, b) 			((a) > (b) ? (a) : (b))
#define Q_MIN(a, b) 			((a) < (b) ? (a) : (b))
#endif

#define Q_ABS(a) 				((a) < 0 ? -(a) : (a))

#define Q_BIT_SET(var, bits)	((var) |= (bits))
#define Q_BIT_CLR(var, bits)	((var) &= ~(bits))
#define Q_BIT_AND(var, bits)	((var) &= (bits))
#define Q_BIT_VAL(var, bits)	((var) & (bits))
#define Q_GET_BITS(var, bits)	((var) & ((1ULL << bits)-1)

#define Q_ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

static inline const char *q_strnull(const char *x)
{
    return x ? x : "(null)";
}

#define offsetof(s, m)   (size_t)&(((s *)0)->m)

#define container_of(ptr, type, member) 					\
({  														\
 	const typeof( ((type *)0)->member ) *__mptr = (ptr); 	\
 	(type *)( (char *)__mptr - offsetof(type,member) );		\
})

// ===============================================
// allocate and free
// ===============================================
void* q_malloc(size_t size);
void* q_calloc(size_t size);
void  q_free(void *p);
char* q_strdup(const char *s);


// ===============================================
// sting copy
// ===============================================
char *q_strlcpy(char *b, const char *s, size_t l);


// ===============================================
// read and write
// ===============================================
ssize_t q_read(int fd, void *buf, size_t count);
ssize_t q_write(int fd, const void *buf, size_t count, int *type);
ssize_t q_loop_read(int fd, void *data, size_t size);
ssize_t q_loop_write(int fd, const void*data, size_t size, int *type);


// ===============================================
// file
// ===============================================
int q_close(int fd);


// ===============================================
// atomic
// ===============================================
typedef struct q_atomic {
    volatile int value;
} q_atomic_t;
typedef struct q_atomic_ptr {
    volatile unsigned long value;
} q_atomic_ptr_t;

#define Q_ATOMIC_INIT(i)	{ (i) }
#define Q_ATOMIC_PTR_INIT(v) { .value = (long) (v) }

#ifdef Q_ARM_A8

static inline void q_memory_barrier(void) {
    asm volatile ("mcr  p15, 0, r0, c7, c10, 5  @ dmb");
}


static inline int q_atomic_read(const q_atomic_t *a) {
    q_memory_barrier();
    return a->value;
}

static inline void q_atomic_set(q_atomic_t *a, int i) {
    a->value = i;
    q_memory_barrier();
}

/* Returns the previously set value */
static inline int q_atomic_add( int i, q_atomic_t *a) {
    unsigned long not_exclusive;
    int new_val, old_val;

    q_memory_barrier();
    do {
        asm volatile ("ldrex    %0, [%3]\n"
                      "add      %2, %0, %4\n"
                      "strex    %1, %2, [%3]\n"
                      : "=&r" (old_val), "=&r" (not_exclusive), "=&r" (new_val)
                      : "r" (&a->value), "Ir" (i)
                      : "cc");
    } while(not_exclusive);
    q_memory_barrier();

    return old_val;
}

/* Returns the previously set value */
static inline int q_atomic_sub( int i, q_atomic_t *a) {
    unsigned long not_exclusive;
    int new_val, old_val;

    q_memory_barrier();
    do {
        asm volatile ("ldrex    %0, [%3]\n"
                      "sub      %2, %0, %4\n"
                      "strex    %1, %2, [%3]\n"
                      : "=&r" (old_val), "=&r" (not_exclusive), "=&r" (new_val)
                      : "r" (&a->value), "Ir" (i)
                      : "cc");
    } while(not_exclusive);
    q_memory_barrier();

    return old_val;
}

static inline int q_atomic_inc(q_atomic_t *a) {
    return q_atomic_add(1, a);
}

static inline int q_atomic_dec(q_atomic_t *a) {
    return q_atomic_sub(1, a);
}

static inline void* q_atomic_ptr_load(const q_atomic_ptr_t *a) {
    q_memory_barrier();
    return (void*) a->value;
}

static inline void q_atomic_ptr_store(q_atomic_ptr_t *a, void *p) {
    a->value = (unsigned long) p;
    q_memory_barrier();
}

#else
#ifdef CONFIG_SMP
#define LOCK_PREFIX_HERE \
                 ".section .smp_locks,\"a\"\n"   \
                 ".balign 4\n"                   \
                 ".long 671f - .\n" /* offset */ \
                 ".previous\n"                   \
                 "671:"
#define LOCK_PREFIX LOCK_PREFIX_HERE "\n\tlock; "
#else
#define LOCK_PREFIX_HERE ""
#define LOCK_PREFIX ""
#endif

static inline int q_atomic_read(const q_atomic_t *v)
{
	return (*(volatile int *)&(v)->value);
}

static inline void q_atomic_set(q_atomic_t *v, int i)
{
	v->value = i;
}

static inline void q_atomic_add(int i, q_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (v->value)
		     : "ir" (i));
}
static inline void q_atomic_sub(int i, q_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "subl %1,%0"
		     : "+m" (v->value)
		     : "ir" (i));
}
static inline void q_atomic_inc(q_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->value));
}
static inline void q_atomic_dec(q_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (v->value));
}
#endif


// ===============================================
// thread
// ===============================================
typedef void (*q_thread_func_t) (void *userdata);

typedef struct q_thread {
    pthread_t id;
    q_thread_func_t thread_func;
    void *userdata;
    q_bool joined;
    q_atomic_t running;
}q_thread;

q_thread* q_thread_new(q_thread_func_t thread_func, void *userdata);
void 		q_thread_delet(q_thread *t);
void 		q_thread_free(q_thread *t);
int 		q_thread_join(q_thread *t);
void* 		q_thread_get_data(q_thread *t);


// ===============================================
// mutex
// ===============================================
typedef struct q_mutex {
    pthread_mutex_t mutex;
}q_mutex;

typedef struct q_cond {
    pthread_cond_t cond;
}q_cond;

q_mutex* 	q_mutex_new(q_bool recursive, q_bool inherit_priority);
void 		q_mutex_free(q_mutex *m);
void 		q_mutex_lock(q_mutex *m);
q_bool 	    q_mutex_try_lock(q_mutex *m);
void 		q_mutex_unlock(q_mutex *m);

q_cond*	    q_cond_new();
void 		q_cond_free(q_cond *c);
void 		q_cond_signal(q_cond *c, int broadcast);
int 		q_cond_wait(q_cond *c, q_mutex *m);

typedef struct q_static_mutex {
    q_atomic_ptr_t ptr;
} q_static_mutex;


// ===============================================
// sempaphore
// ===============================================
typedef struct q_semaphore {
    sem_t sem;
}q_semaphore;
q_semaphore* 	q_semaphore_new(unsigned value);
void 			q_semaphore_free(q_semaphore *s);
void 			q_semaphore_post(q_semaphore *s);
void 			q_semaphore_wait(q_semaphore *s);

typedef struct q_static_semaphore {
    q_atomic_ptr_t ptr;
} q_static_semaphore;


// ===============================================
// queue
// ===============================================
typedef struct q_queue {
  int front;
  int rear;
  int len_buf;
  char *buf;
}q_queue;

q_queue* 	q_create_queue(int size);
void 		q_destroy_queue(q_queue* q);
int 		q_get_queue(q_queue* q, char* buf, size_t len);
int 		q_set_queue(q_queue* q, void* buf, size_t len, q_bool expand);
int 		q_pop_queue(q_queue* q, char* item);
int 		q_add_queue(q_queue* q, void* item, q_bool expand);
int 		q_peek_queue(q_queue* q, char* item, int idx);
size_t 		q_size_queue(q_queue* q);
void 		q_expand_queue(q_queue* q);
void 		q_show_queue(q_queue* q);

static inline q_bool q_isempty_queue(q_queue* q)
{
	q_assert(q);
	return (q->front == q->rear) ? q_true:q_false ;
}

static inline q_bool q_isfull_queue(q_queue* q)
{
	q_assert(q);
	return (q->front == (q->rear + 1)%(q->len_buf) ) ? q_true:q_false ;
}

#endif /* QSIFUNC_H_ */
