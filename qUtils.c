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
#include "qUtils.h"

void* q_malloc(size_t size)
{
    void *p;

    if (size > 0 && size < Q_MAX_ALLOC_SIZE){
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

void* q_calloc(size_t size)
{
    void *p;

    if (size > 0 && size < Q_MAX_ALLOC_SIZE){
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

void q_free(void *p)
{
    if (!p)
        return;
    free(p);
}

char* q_strdup(const char *s)
{
    if (!s)
        return NULL;
    else {
    	int l;
    	l=strlen(s)+1;
        char *r = (char*)q_calloc(l);
        memcpy(r, s, l);
        return r;
    }
}

char *q_strlcpy(char *b, const char *s, size_t l)
{
    size_t k;
    k = strlen(s);
    if (k > l-1)
        k = l-1;
    memcpy(b, s, k);
    b[k] = 0;
    return b;
}
ssize_t q_read(int fd, void *buf, size_t count)
{
    for (;;) {
        ssize_t r;
        if ((r = read(fd, buf, count)) < 0)
            if (errno == EINTR)
                continue;
        return r;
    }
}

ssize_t q_write(int fd, const void *buf, size_t count, int *type)
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

ssize_t q_loop_read(int fd, void *data, size_t size)
{
    ssize_t ret = 0;

    while (size > 0) {
        ssize_t r;
        if ((r = q_read(fd, data, size)) < 0)
            return r;
        if (r == 0)
            break;
        ret += r;
        data = (uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}

ssize_t q_loop_write(int fd, const void *data, size_t size, int *type)
{
    ssize_t ret = 0;
    int _type;

    if (!type) {
        _type = 0;
        type = &_type;
    }
    while (size > 0) {
        ssize_t r;
        if ((r = q_write(fd, data, size, type)) < 0)
            return r;

        if (r == 0)
            break;
        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}

int q_close(int fd)
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
    q_thread *t = (q_thread *)userdata;
    q_assert(t);

    t->id = pthread_self();
    q_atomic_inc(&t->running);
    t->thread_func(t->userdata);
    q_atomic_sub(2, &t->running);
    return NULL;
}

q_thread* q_thread_new(q_thread_func_t thread_func, void *userdata)
{
	q_assert(thread_func);

	q_thread *t;
	t=(q_thread *)q_malloc(sizeof(q_thread));
	t->thread_func = thread_func;
	t->userdata = userdata;
	t->joined = q_false;

	q_atomic_set(&t->running,0);
	if (pthread_create(&t->id, NULL, internal_thread_func, t) < 0) {
        q_free(t);
        fprintf (stderr, "qsiFunc : thread_new error\n");
        return NULL;
    }
    q_atomic_inc(&t->running);
	return t;
}

void q_thread_delet(q_thread *t)
{
	q_assert(t);
	pthread_detach(t->id);
	q_free(t);
	t = NULL;
}

void q_thread_free(q_thread *t)
{
	q_assert(t);
	q_thread_join(t);
	q_free(t);
	t = NULL;
}

int q_thread_join(q_thread *t)
{
	if (t->joined)
	        return -1;
	t->joined = q_true;
	return pthread_join(t->id, NULL);
}

void* q_thread_get_data(q_thread *t)
{
    q_assert(t);
    return t->userdata;
}

q_mutex* q_mutex_new(q_bool recursive, q_bool inherit_priority)
{
	q_mutex *m;
	pthread_mutexattr_t attr;
	int r;

	memset(&attr, 0, sizeof(pthread_mutexattr_t));

	m = (q_mutex *)q_calloc(sizeof(q_mutex));

	q_assert(pthread_mutexattr_init(&attr) == 0);

	if (recursive)
		q_assert(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0);

	if (inherit_priority)
		q_assert(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT) == 0);

    if ((r = pthread_mutex_init(&m->mutex, &attr))) {
        //back to normal mutexes.
        q_assert((r == ENOTSUP) && inherit_priority);

        q_assert(pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE) == 0);
        q_assert(pthread_mutex_init(&m->mutex, &attr) == 0);
    }

    return m;
}

void q_mutex_free(q_mutex *m)
{
	q_assert(m);
    q_assert(pthread_mutex_destroy(&m->mutex) == 0);
    q_free(m);
    m = NULL;
}

void q_mutex_lock(q_mutex *m)
{
	q_assert(m);
	q_assert(pthread_mutex_lock(&m->mutex) == 0);
}

q_bool q_mutex_try_lock(q_mutex *m)
{
    int r;
    q_assert(m);

    if ((r = pthread_mutex_trylock(&m->mutex)) != 0) {
        q_assert(r == EBUSY);
        return q_false;
    }
    return q_true;
}

void q_mutex_unlock(q_mutex *m)
{
	q_assert(m);
	q_assert(pthread_mutex_unlock(&m->mutex) == 0);
}

q_cond *q_cond_new()
{
	q_cond *c;
	c = (q_cond *)q_malloc(sizeof(q_cond));
	q_assert(pthread_cond_init(&c->cond, NULL) == 0);
	return c;
}

void q_cond_free(q_cond *c)
{
	q_assert(c);
	q_assert(pthread_cond_destroy(&c->cond) == 0);
	q_free(c);
}

void q_cond_signal(q_cond *c, int broadcast)
{
	q_assert(c);

	if (broadcast)
		q_assert(pthread_cond_broadcast(&c->cond) == 0);
	else
		q_assert(pthread_cond_signal(&c->cond) == 0);
}

int q_cond_wait(q_cond *c, q_mutex *m)
{
    q_assert(c);
    q_assert(m);
    return pthread_cond_wait(&c->cond, &m->mutex);
}

q_semaphore* 	q_semaphore_new(unsigned value)
{
	q_semaphore* s;
	s = (q_semaphore*)q_malloc(sizeof(q_semaphore));
	q_assert(sem_init(&s->sem, 0, value) == 0);
	return s;
}

void q_semaphore_free(q_semaphore *s)
{
	q_assert(s);
	q_assert(sem_destroy(&s->sem) == 0);
	q_free(s);
	s = NULL;
}

void q_semaphore_post(q_semaphore *s)
{
	q_assert(s);
	q_assert(sem_post(&s->sem) == 0);
}

void q_semaphore_wait(q_semaphore *s)
{
    int ret;
    q_assert(s);

    do {
        ret = sem_wait(&s->sem);
    } while (ret < 0 && errno == EINTR);

    q_assert(ret == 0);
}

q_queue* q_create_queue(int size)
{
	q_queue *q;

	q = (q_queue *)q_malloc(sizeof(q_queue));
	q->buf = (char *)q_calloc(size);
	
	q->front = -1;
	q->rear = -1;
	q->len_buf = size;
	
	return q;
}

void q_destroy_queue(q_queue* q)
{
	q_assert(q);

	q_free(q->buf);
	q_free(q);
	q = NULL;
}

int q_get_queue(q_queue* q, char* buf, size_t len)
{
	q_assert(q);
	q_assert(buf);

	uint32_t i;
	
	for (i=0 ;i<len ;i++){
		if(q_pop_queue(q, buf+i))
			break;
	}

	return i;
}

int q_set_queue(q_queue* q, char* buf, size_t len, q_bool expand)
{
	q_assert(q);
	q_assert(buf);

	uint32_t i;

	for (i=0 ;i<len ;i++){
		q_add_queue(q, buf+i, expand);
	}

	return i;
}

int q_add_queue(q_queue* q, char* item, q_bool expand)
{
	q_assert(q);

	if (q_isfull_queue(q)){
		if (expand)
			q_expand_queue(q);
		else
			return -1;
	}

	q->rear = (q->rear + 1 ) % q->len_buf;
	q->buf[q->rear] = *item;
	return 0;
}

int q_pop_queue(q_queue* q, char* item)
{
	q_assert(q);

	if (q_isempty_queue(q)){
		fprintf(stderr, "q is empty\n");
		return -1;
	}

	q->front = (q->front  + 1 ) % q->len_buf;

	*item = q->buf[q->front];
	return 0;
}

int q_peek_queue(q_queue* q, char* item, int idx)
{
	q_assert(q);

	if (q_isempty_queue(q)){
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

size_t q_size_queue(q_queue* q)
{
	return (size_t)((q->rear - q->front + q->len_buf) % q->len_buf);
}

void q_expand_queue(q_queue* q)
{
	q_assert(q);

	char* buf;
	buf = (char *)q_calloc(2*(q->len_buf));

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

	q_free(q->buf);
	q->buf = buf;
}

void q_show_queue(q_queue* q)
{
	if (!q_isempty_queue(q)){
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


