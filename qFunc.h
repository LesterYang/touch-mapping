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

typedef int q_bool;

#ifndef FALSE
#define FALSE ((q_bool) 0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define QSI_MAX_ALLOC_SIZE (1024*1024*64)  //64MB

#ifdef __GNUC__
#define QSI_LIKELY(x) (__builtin_expect(!!(x),1))
#define QSI_UNLIKELY(x) (__builtin_expect(!!(x),0))
#define QSI_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define QSI_LIKELY(x) (x)
#define QSI_UNLIKELY(x) (x)
#define QSI_PRETTY_FUNCTION ""
#endif

#define qsi_nothing() do {} while (FALSE)

#ifdef QSI_ASSERT
#define qsi_assert(expr)                                                                            \
    do {                                                                                            \
        if (QSI_UNLIKELY(!(expr))) {                                                                \
            fprintf(stderr, "qTs-daemon : Expr '%s' failed at %s:%u, function '%s'. Aborting\n",    \
                        #expr , __FILE__, __LINE__, QSI_PRETTY_FUNCTION);                           \
            abort();                                                                                \
        }                                                                                           \
    } while (0)
#else
#define qsi_assert(expr) qsi_nothing()
#endif

#define qerror(expr, ...)                                                   \
    do {                                                                    \
        fprintf(stderr, "qTs-daemon : %s,%d: ",__FUNCTION__, __LINE__);     \
        fprintf(stderr, expr,  ##__VA_ARGS__);                              \
        fprintf(stderr, "\n");                                              \
    } while (0)

#define q_dbg(expr, ...)                        \
    do {                                        \
        printf("qTs-daemon : ");                \
        printf(expr,  ##__VA_ARGS__);           \
        printf("\n");                           \
    } while (0)

#ifdef __GNUC__
#define QSI_MAX(a,b)                            \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a > _b ? _a : _b;                  \
        })
#define QSI_MIN(a,b)                            \
    __extension__ ({                            \
            typeof(a) _a = (a);                 \
            typeof(b) _b = (b);                 \
            _a < _b ? _a : _b;                  \
        })
#else
#define QSI_MAX(a, b) 			((a) > (b) ? (a) : (b))
#define QSI_MIN(a, b) 			((a) < (b) ? (a) : (b))
#endif

#define QSI_ABS(a) 				((a) < 0 ? -(a) : (a))

#define QSI_BIT_SET(var, bits)	((var) |= (bits))
#define QSI_BIT_CLR(var, bits)	((var) &= ~(bits))
#define QSI_BIT_AND(var, bits)	((var) &= (bits))
#define QSI_BIT_VAL(var, bits)	((var) & (bits))
#define QSI_GET_BITS(var, bits)	((var) & ((1ULL << bits)-1)

#define QSI_ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

static inline const char *qsi_strnull(const char *x)
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
void* qsi_malloc(size_t size);
void* qsi_calloc(size_t size);
void  qsi_free(void *p);
char* qsi_strdup(const char *s);


// ===============================================
// sting copy
// ===============================================
char *qsi_strlcpy(char *b, const char *s, size_t l);


// ===============================================
// read and write
// ===============================================
ssize_t qsi_read(int fd, void *buf, size_t count, int *type);
ssize_t qsi_write(int fd, const void *buf, size_t count, int *type);
ssize_t qsi_loop_read(int fd, void *data, size_t size, int *type);
ssize_t qsi_loop_write(int fd, const void*data, size_t size, int *type);


// ===============================================
// file
// ===============================================
int qsi_close(int fd);


// ===============================================
// atomic
// ===============================================
typedef struct qsi_atomic {
    volatile int value;
} qsi_atomic_t;
typedef struct qsi_atomic_ptr {
    volatile unsigned long value;
} qsi_atomic_ptr_t;

#define QSI_ATOMIC_INIT(i)	{ (i) }
#define QSI_ATOMIC_PTR_INIT(v) { .value = (long) (v) }

#ifdef QSI_ARM_A8

static inline void qsi_memory_barrier(void) {
    asm volatile ("mcr  p15, 0, r0, c7, c10, 5  @ dmb");
}


static inline int qsi_atomic_read(const qsi_atomic_t *a) {
    qsi_memory_barrier();
    return a->value;
}

static inline void qsi_atomic_set(qsi_atomic_t *a, int i) {
    a->value = i;
    qsi_memory_barrier();
}

/* Returns the previously set value */
static inline int qsi_atomic_add( int i, qsi_atomic_t *a) {
    unsigned long not_exclusive;
    int new_val, old_val;

    qsi_memory_barrier();
    do {
        asm volatile ("ldrex    %0, [%3]\n"
                      "add      %2, %0, %4\n"
                      "strex    %1, %2, [%3]\n"
                      : "=&r" (old_val), "=&r" (not_exclusive), "=&r" (new_val)
                      : "r" (&a->value), "Ir" (i)
                      : "cc");
    } while(not_exclusive);
    qsi_memory_barrier();

    return old_val;
}

/* Returns the previously set value */
static inline int qsi_atomic_sub( int i, qsi_atomic_t *a) {
    unsigned long not_exclusive;
    int new_val, old_val;

    qsi_memory_barrier();
    do {
        asm volatile ("ldrex    %0, [%3]\n"
                      "sub      %2, %0, %4\n"
                      "strex    %1, %2, [%3]\n"
                      : "=&r" (old_val), "=&r" (not_exclusive), "=&r" (new_val)
                      : "r" (&a->value), "Ir" (i)
                      : "cc");
    } while(not_exclusive);
    qsi_memory_barrier();

    return old_val;
}

static inline int qsi_atomic_inc(qsi_atomic_t *a) {
    return qsi_atomic_add(1, a);
}

static inline int qsi_atomic_dec(qsi_atomic_t *a) {
    return qsi_atomic_sub(1, a);
}

static inline void* qsi_atomic_ptr_load(const qsi_atomic_ptr_t *a) {
    qsi_memory_barrier();
    return (void*) a->value;
}

static inline void qsi_atomic_ptr_store(qsi_atomic_ptr_t *a, void *p) {
    a->value = (unsigned long) p;
    qsi_memory_barrier();
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

static inline int qsi_atomic_read(const qsi_atomic_t *v)
{
	return (*(volatile int *)&(v)->value);
}

static inline void qsi_atomic_set(qsi_atomic_t *v, int i)
{
	v->value = i;
}

static inline void qsi_atomic_add(int i, qsi_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "addl %1,%0"
		     : "+m" (v->value)
		     : "ir" (i));
}
static inline void qsi_atomic_sub(int i, qsi_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "subl %1,%0"
		     : "+m" (v->value)
		     : "ir" (i));
}
static inline void qsi_atomic_inc(qsi_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "incl %0"
		     : "+m" (v->value));
}
static inline void qsi_atomic_dec(qsi_atomic_t *v)
{
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (v->value));
}
#endif


// ===============================================
// thread
// ===============================================
typedef void (*qsi_thread_func_t) (void *userdata);

typedef struct qsi_thread {
    pthread_t id;
    qsi_thread_func_t thread_func;
    void *userdata;
    q_bool joined;
    qsi_atomic_t running;
}qsi_thread;

qsi_thread* qsi_thread_new(qsi_thread_func_t thread_func, void *userdata);
void 		qsi_thread_delet(qsi_thread *t);
void 		qsi_thread_free(qsi_thread *t);
int 		qsi_thread_join(qsi_thread *t);
void* 		qsi_thread_get_data(qsi_thread *t);


// ===============================================
// mutex
// ===============================================
typedef struct qsi_mutex {
    pthread_mutex_t mutex;
}qsi_mutex;

typedef struct qsi_cond {
    pthread_cond_t cond;
}qsi_cond;

qsi_mutex* 	qsi_mutex_new(q_bool recursive, q_bool inherit_priority);
void 		qsi_mutex_free(qsi_mutex *m);
void 		qsi_mutex_lock(qsi_mutex *m);
q_bool 	qsi_mutex_try_lock(qsi_mutex *m);
void 		qsi_mutex_unlock(qsi_mutex *m);

qsi_cond*	qsi_cond_new();
void 		qsi_cond_free(qsi_cond *c);
void 		qsi_cond_signal(qsi_cond *c, int broadcast);
int 		qsi_cond_wait(qsi_cond *c, qsi_mutex *m);

typedef struct qsi_static_mutex {
    qsi_atomic_ptr_t ptr;
} qsi_static_mutex;


// ===============================================
// sempaphore
// ===============================================
typedef struct qsi_semaphore {
    sem_t sem;
}qsi_semaphore;
qsi_semaphore* 	qsi_semaphore_new(unsigned value);
void 			qsi_semaphore_free(qsi_semaphore *s);
void 			qsi_semaphore_post(qsi_semaphore *s);
void 			qsi_semaphore_wait(qsi_semaphore *s);

typedef struct qsi_static_semaphore {
    qsi_atomic_ptr_t ptr;
} qsi_static_semaphore;


// ===============================================
// queue
// ===============================================
typedef struct qsi_queue {
  int front;
  int rear;
  int len_buf;
  char *buf;
}qsi_queue;

qsi_queue* 	qsi_create_q(int size);
void 		qsi_destroy_q(qsi_queue* q);
int 		qsi_get_q(qsi_queue* q, char* buf, size_t len);
int 		qsi_set_q(qsi_queue* q, char* buf, size_t len, q_bool expand);
int 		qsi_pop_q(qsi_queue* q, char* item);
int 		qsi_add_q(qsi_queue* q, char* item, q_bool expand);
int 		qsi_peek_q(qsi_queue* q, char* item, int idx);
size_t 		qsi_size_q(qsi_queue* q);
void 		qsi_expand_q(qsi_queue* q);
void 		qsi_show_q(qsi_queue* q);

static inline q_bool qsi_isempty_q(qsi_queue* q)
{
	qsi_assert(q);
	return (q->front == q->rear) ? TRUE:FALSE ;
}

static inline q_bool qsi_isfull_q(qsi_queue* q)
{
	qsi_assert(q);
	return (q->front == (q->rear + 1)%(q->len_buf) ) ? TRUE:FALSE ;
}

#endif /* QSIFUNC_H_ */
