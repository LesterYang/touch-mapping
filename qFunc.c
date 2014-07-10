/*
 * qsiFunc.c
 *
 *  Created on: Aug 20, 2013
 *      Author: lester
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <sys/errno.h>
#include "qFunc.h"

void* qsi_malloc(size_t size)
{
    void *p;

    if (size > 0 && size < QSI_MAX_ALLOC_SIZE){
    	if (!(p = malloc(size))){
    		fprintf(stderr, "qsiFinc : malloc error");
    		return NULL;
    	}
    }
    else{
    	fprintf(stderr, "qsiFinc : allocation size error");
    	return NULL;
    }
    return p;
}

void* qsi_calloc(size_t size)
{
    void *p;

    if (size > 0 && size < QSI_MAX_ALLOC_SIZE){
    	if (!(p =  calloc(1, size))){
    		fprintf(stderr, "qsiFinc : calloc error");
    		return NULL;
    	}
    }
    else{
    	fprintf(stderr, "qsiFinc : allocation size error");
    	return NULL;
    }
    return p;
}

void qsi_free(void *p)
{
    if (!p)
        return;
    free(p);
}

char* qsi_strdup(const char *s)
{
    if (!s)
        return NULL;
    else {
    	int l;
    	l=strlen(s)+1;
        char *r = (char*)qsi_calloc(l);
        memcpy(r, s, l);
        return r;
    }
}

char *qsi_strlcpy(char *b, const char *s, size_t l)
{
    size_t k;
    k = strlen(s);
    if (k > l-1)
        k = l-1;
    memcpy(b, s, k);
    b[k] = 0;
    return b;
}
ssize_t qsi_read(int fd, void *buf, size_t count, int *type)
{
    for (;;) {
        ssize_t r;
        if ((r = read(fd, buf, count)) < 0)
            if (errno == EINTR)
                continue;
        return r;
    }
}

ssize_t qsi_write(int fd, const void *buf, size_t count, int *type)
{
    if (!type || *type == 0) {
        ssize_t r;
        for (;;) {
            if ((r = send(fd, buf, count, MSG_NOSIGNAL)) < 0) {
                if (errno == EINTR)
                    continue;
                break;
            }
            return r;
        }
        if (errno != ENOTSOCK)
            return r;
        if (type)
            *type = 1;
    }
    for (;;) {
        ssize_t r;
        if ((r = write(fd, buf, count)) < 0)
            if (errno == EINTR)
                continue;
        return r;
    }
}

ssize_t qsi_loop_read(int fd, void *data, size_t size, int *type)
{
    ssize_t ret = 0;
    int _type;

    if (!type) {
        _type = 0;
        type = &_type;
    }
    while (size > 0) {
        ssize_t r;
        if ((r = qsi_read(fd, data, size, type)) < 0)
            return r;
        if (r == 0)
            break;
        ret += r;
        data = (uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}

ssize_t qsi_loop_write(int fd, const void *data, size_t size, int *type)
{
    ssize_t ret = 0;
    int _type;

    if (!type) {
        _type = 0;
        type = &_type;
    }
    while (size > 0) {
        ssize_t r;
        if ((r = qsi_write(fd, data, size, type)) < 0)
            return r;

        if (r == 0)
            break;
        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}

int qsi_close(int fd)
{
    for (;;) {
        int r;
        if ((r = close(fd)) < 0)
            if (errno == EINTR)
                continue;
        return r;
    }
}


static void* internal_thread_func(void *userdata)
{
    qsi_thread *t = (qsi_thread *)userdata;
    qsi_assert(t);

    t->id = pthread_self();
    qsi_atomic_inc(&t->running);
    t->thread_func(t->userdata);
    qsi_atomic_sub(2, &t->running);
    return NULL;
}

qsi_thread* qsi_thread_new(qsi_thread_func_t thread_func, void *userdata)
{
	qsi_assert(thread_func);

	qsi_thread *t;
	t=(qsi_thread *)qsi_malloc(sizeof(qsi_thread));
	t->thread_func = thread_func;
	t->userdata = userdata;
	t->joined = FALSE;

	qsi_atomic_set(&t->running,0);
	if (pthread_create(&t->id, NULL, internal_thread_func, t) < 0) {
        qsi_free(t);
        fprintf (stderr, "qsiFunc : thread_new error\n");
        return NULL;
    }
    qsi_atomic_inc(&t->running);
	return t;
}

void qsi_thread_delet(qsi_thread *t)
{
	qsi_assert(t);
	qsi_free(t);
	t = NULL;
}

void qsi_thread_free(qsi_thread *t)
{
	qsi_assert(t);
	qsi_thread_join(t);
	qsi_free(t);
	t = NULL;
}

int qsi_thread_join(qsi_thread *t)
{
	if (t->joined)
	        return -1;
	t->joined = TRUE;
	return pthread_join(t->id, NULL);
}

void* qsi_thread_get_data(qsi_thread *t)
{
    qsi_assert(t);
    return t->userdata;
}

qsi_mutex* qsi_mutex_new(q_bool recursive, q_bool inherit_priority)
{
	qsi_mutex *m;
	pthread_mutexattr_t attr;
	int r;

	m = (qsi_mutex *)qsi_calloc(sizeof(qsi_mutex));

	qsi_assert(pthread_mutexattr_init(&attr) == 0);

	if (recursive)
		qsi_assert(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0);

	if (inherit_priority)
		qsi_assert(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT) == 0);

    if ((r = pthread_mutex_init(&m->mutex, &attr))) {
        //back to normal mutexes.
        qsi_assert((r == ENOTSUP) && inherit_priority);

        qsi_assert(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE) == 0);
        qsi_assert(pthread_mutex_init(&m->mutex, &attr) == 0);
    }

    return m;
}

void qsi_mutex_free(qsi_mutex *m)
{
	qsi_assert(m);
    qsi_assert(pthread_mutex_destroy(&m->mutex) == 0);
    qsi_free(m);
    m = NULL;
}

void qsi_mutex_lock(qsi_mutex *m)
{
	qsi_assert(m);
	qsi_assert(pthread_mutex_lock(&m->mutex) == 0);
}

q_bool qsi_mutex_try_lock(qsi_mutex *m)
{
    int r;
    qsi_assert(m);

    if ((r = pthread_mutex_trylock(&m->mutex)) != 0) {
        qsi_assert(r == EBUSY);
        return FALSE;
    }
    return TRUE;
}

void qsi_mutex_unlock(qsi_mutex *m)
{
	qsi_assert(m);
	qsi_assert(pthread_mutex_unlock(&m->mutex) == 0);
}

qsi_cond *qsi_cond_new()
{
	qsi_cond *c;
	c = (qsi_cond *)qsi_malloc(sizeof(qsi_cond));
	qsi_assert(pthread_cond_init(&c->cond, NULL) == 0);
	return c;
}

void qsi_cond_free(qsi_cond *c)
{
	qsi_assert(c);
	qsi_assert(pthread_cond_destroy(&c->cond) == 0);
	qsi_free(c);
}

void qsi_cond_signal(qsi_cond *c, int broadcast)
{
	qsi_assert(c);

	if (broadcast)
		qsi_assert(pthread_cond_broadcast(&c->cond) == 0);
	else
		qsi_assert(pthread_cond_signal(&c->cond) == 0);
}

int qsi_cond_wait(qsi_cond *c, qsi_mutex *m)
{
    qsi_assert(c);
    qsi_assert(m);
    return pthread_cond_wait(&c->cond, &m->mutex);
}

qsi_semaphore* 	qsi_semaphore_new(unsigned value)
{
	qsi_semaphore* s;
	s = (qsi_semaphore*)qsi_malloc(sizeof(qsi_semaphore));
	qsi_assert(sem_init(&s->sem, 0, value) == 0);
	return s;
}

void qsi_semaphore_free(qsi_semaphore *s)
{
	qsi_assert(s);
	qsi_assert(sem_destroy(&s->sem) == 0);
	qsi_free(s);
	s = NULL;
}

void qsi_semaphore_post(qsi_semaphore *s)
{
	qsi_assert(s);
	qsi_assert(sem_post(&s->sem) == 0);
}

void qsi_semaphore_wait(qsi_semaphore *s)
{
    int ret;
    qsi_assert(s);

    do {
        ret = sem_wait(&s->sem);
    } while (ret < 0 && errno == EINTR);

    qsi_assert(ret == 0);
}

qsi_queue* qsi_create_q(int size)
{
	qsi_queue *q;

	q = (qsi_queue *)qsi_malloc(sizeof(qsi_queue));
	q->buf = (char *)qsi_calloc(size);
	
	q->front = -1;
	q->rear = -1;
	q->len_buf = size;
	
	return q;
}

void qsi_destroy_q(qsi_queue* q)
{
	qsi_assert(q);

	qsi_free(q->buf);
	qsi_free(q);
	q = NULL;
}

int qsi_get_q(qsi_queue* q, char* buf, size_t len)
{
	qsi_assert(q);
	qsi_assert(buf);

	uint32_t i;
	
	for (i=0 ;i<len ;i++){
		if(qsi_pop_q(q, buf+i))
			break;
	}

	return i;
}

int qsi_set_q(qsi_queue* q, char* buf, size_t len, q_bool expand)
{
	qsi_assert(q);
	qsi_assert(buf);

	uint32_t i;

	for (i=0 ;i<len ;i++){
		qsi_add_q(q, buf+i, expand);
	}

	return i;
}

int qsi_add_q(qsi_queue* q, char* item, q_bool expand)
{
	qsi_assert(q);

	if (qsi_isfull_q(q)){
		if (expand)
			qsi_expand_q(q);
		else
			return -1;
	}

	q->rear = (q->rear + 1 ) % q->len_buf;
	q->buf[q->rear] = *item;
	return 0;
}

int qsi_pop_q(qsi_queue* q, char* item)
{
	qsi_assert(q);

	if (qsi_isempty_q(q)){
		fprintf(stderr, "q is empty\n");
		return -1;
	}

	q->front = (q->front  + 1 ) % q->len_buf;

	*item = q->buf[q->front];
	return 0;
}

int qsi_peek_q(qsi_queue* q, char* item, int idx)
{
	qsi_assert(q);

	if (qsi_isempty_q(q)){
		fprintf(stderr, "q is empty\n");
		return -1;
	}

	if ( idx > ((q->rear - q->front + q->len_buf) % q->len_buf) - 1 ){
		fprintf(stderr, "peek overflow\n");
		return -1;
	}
	int peek = (q->front + idx + 1) % q->len_buf;

	*item = q->buf[peek];
	return 0;
}

size_t qsi_size_q(qsi_queue* q)
{
	return (size_t)((q->rear - q->front + q->len_buf) % q->len_buf);
}

void qsi_expand_q(qsi_queue* q)
{
	qsi_assert(q);

	char* buf;
	buf = (char *)qsi_calloc(2*(q->len_buf));

	int start = (q->front + 1) % q->len_buf;

	if (start < 2){
		memcpy(buf, &q->buf[start], q->len_buf-1);
	}
	else{
		memcpy(buf,&q->buf[start], q->len_buf - start);
		memcpy(buf + q->len_buf - start, q->buf, start - 1);
	}

	q->front = 2 * q->len_buf -1;
	q->rear = q->len_buf -2;
	q->len_buf *= 2;

	qsi_free(q->buf);
	q->buf = buf;
}

void qsi_show_q(qsi_queue* q)
{
	if (!qsi_isempty_q(q)){
		int i, len;
		printf("buf size:%d front: buf[%d] rear: buf[%d]\n",q->len_buf,q->front,q->rear);
		printf("buf: ");
		len = ((q->rear - q->front + q->len_buf) % q->len_buf);
		for(i = 0 ; i < len;  i++){
			printf("0x%x ",q->buf[(q->front + i + 1) % q->len_buf]);
		}
		printf("\n");
	}
}


