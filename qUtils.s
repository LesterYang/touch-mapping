	.file	"qUtils.c"
	.text
	.type	q_atomic_set, @function
q_atomic_set:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	12(%ebp), %edx
	movl	%edx, (%eax)
	popl	%ebp
	ret
	.size	q_atomic_set, .-q_atomic_set
	.type	q_atomic_sub, @function
q_atomic_sub:
	pushl	%ebp
	movl	%esp, %ebp
	movl	12(%ebp), %eax
	movl	8(%ebp), %edx
#APP
# 252 "qUtils.h" 1
	subl %edx,(%eax)
# 0 "" 2
#NO_APP
	popl	%ebp
	ret
	.size	q_atomic_sub, .-q_atomic_sub
	.type	q_atomic_inc, @function
q_atomic_inc:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
#APP
# 258 "qUtils.h" 1
	incl (%eax)
# 0 "" 2
#NO_APP
	popl	%ebp
	ret
	.size	q_atomic_inc, .-q_atomic_inc
	.type	q_isempty_queue, @function
q_isempty_queue:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	(%eax), %edx
	movl	8(%ebp), %eax
	movl	4(%eax), %eax
	cmpl	%eax, %edx
	sete	%al
	movzbl	%al, %eax
	popl	%ebp
	ret
	.size	q_isempty_queue, .-q_isempty_queue
	.type	q_isfull_queue, @function
q_isfull_queue:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$4, %esp
	movl	8(%ebp), %eax
	movl	(%eax), %ecx
	movl	8(%ebp), %eax
	movl	4(%eax), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -8(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-8(%ebp)
	movl	%edx, %eax
	cmpl	%eax, %ecx
	sete	%al
	movzbl	%al, %eax
	addl	$4, %esp
	popl	%ebx
	popl	%ebp
	ret
	.size	q_isfull_queue, .-q_isfull_queue
	.section	.rodata
.LC0:
	.string	"qsiFinc : malloc error"
	.align 4
.LC1:
	.string	"qsiFinc : allocation size error"
	.text
.globl q_malloc
	.type	q_malloc, @function
q_malloc:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	cmpl	$0, 8(%ebp)
	je	.L12
	cmpl	$67108863, 8(%ebp)
	ja	.L12
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	malloc
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jne	.L13
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC0, %eax
	movl	%edx, 12(%esp)
	movl	$22, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$0, %eax
	jmp	.L14
.L13:
	movl	-12(%ebp), %eax
	jmp	.L14
.L12:
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC1, %eax
	movl	%edx, 12(%esp)
	movl	$31, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$0, %eax
.L14:
	leave
	ret
	.size	q_malloc, .-q_malloc
	.section	.rodata
.LC2:
	.string	"qsiFinc : calloc error"
	.text
.globl q_calloc
	.type	q_calloc, @function
q_calloc:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	cmpl	$0, 8(%ebp)
	je	.L17
	cmpl	$67108863, 8(%ebp)
	ja	.L17
	movl	8(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$1, (%esp)
	call	calloc
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jne	.L18
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC2, %eax
	movl	%edx, 12(%esp)
	movl	$22, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$0, %eax
	jmp	.L19
.L18:
	movl	-12(%ebp), %eax
	jmp	.L19
.L17:
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC1, %eax
	movl	%edx, 12(%esp)
	movl	$31, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$0, %eax
.L19:
	leave
	ret
	.size	q_calloc, .-q_calloc
.globl q_free
	.type	q_free, @function
q_free:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	cmpl	$0, 8(%ebp)
	je	.L25
.L22:
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	free
	jmp	.L24
.L25:
	nop
.L24:
	leave
	ret
	.size	q_free, .-q_free
.globl q_strdup
	.type	q_strdup, @function
q_strdup:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	cmpl	$0, 8(%ebp)
	jne	.L27
	movl	$0, %eax
	jmp	.L28
.L27:
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	strlen
	addl	$1, %eax
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	q_calloc
	movl	%eax, -16(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	8(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	-16(%ebp), %eax
	movl	%eax, (%esp)
	call	memcpy
	movl	-16(%ebp), %eax
.L28:
	leave
	ret
	.size	q_strdup, .-q_strdup
.globl q_strlcpy
	.type	q_strlcpy, @function
q_strlcpy:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	12(%ebp), %eax
	movl	%eax, (%esp)
	call	strlen
	movl	%eax, -12(%ebp)
	movl	16(%ebp), %eax
	subl	$1, %eax
	cmpl	-12(%ebp), %eax
	jae	.L31
	movl	16(%ebp), %eax
	subl	$1, %eax
	movl	%eax, -12(%ebp)
.L31:
	movl	-12(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	memcpy
	movl	-12(%ebp), %eax
	movl	8(%ebp), %edx
	leal	(%edx,%eax), %eax
	movb	$0, (%eax)
	movl	8(%ebp), %eax
	leave
	ret
	.size	q_strlcpy, .-q_strlcpy
.globl q_read
	.type	q_read, @function
q_read:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	jmp	.L35
.L37:
	nop
.L35:
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	read
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jns	.L34
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$4, %eax
	je	.L37
.L34:
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_read, .-q_read
.globl q_write
	.type	q_write, @function
q_write:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	cmpl	$0, 20(%ebp)
	je	.L39
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	testl	%eax, %eax
	jne	.L40
	jmp	.L39
.L49:
	nop
.L39:
	movl	$16384, 12(%esp)
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	send
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jns	.L41
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$4, %eax
	je	.L49
.L42:
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$88, %eax
	jne	.L43
	jmp	.L48
.L41:
	movl	-12(%ebp), %eax
	jmp	.L45
.L43:
	movl	-12(%ebp), %eax
	jmp	.L45
.L48:
	cmpl	$0, 20(%ebp)
	je	.L40
	movl	20(%ebp), %eax
	movl	$1, (%eax)
	jmp	.L40
.L50:
	nop
.L40:
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	write
	movl	%eax, -16(%ebp)
	cmpl	$0, -16(%ebp)
	jns	.L46
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$4, %eax
	je	.L50
.L46:
	movl	-16(%ebp), %eax
.L45:
	leave
	ret
	.size	q_write, .-q_write
.globl q_loop_read
	.type	q_loop_read, @function
q_loop_read:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$0, -12(%ebp)
	jmp	.L52
.L57:
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_read
	movl	%eax, -16(%ebp)
	cmpl	$0, -16(%ebp)
	jns	.L53
	movl	-16(%ebp), %eax
	jmp	.L54
.L53:
	cmpl	$0, -16(%ebp)
	je	.L59
.L55:
	movl	-16(%ebp), %eax
	addl	%eax, -12(%ebp)
	movl	-16(%ebp), %eax
	addl	%eax, 12(%ebp)
	movl	-16(%ebp), %eax
	subl	%eax, 16(%ebp)
.L52:
	cmpl	$0, 16(%ebp)
	jne	.L57
	jmp	.L56
.L59:
	nop
.L56:
	movl	-12(%ebp), %eax
.L54:
	leave
	ret
	.size	q_loop_read, .-q_loop_read
.globl q_loop_write
	.type	q_loop_write, @function
q_loop_write:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$0, -12(%ebp)
	cmpl	$0, 20(%ebp)
	jne	.L62
	movl	$0, -16(%ebp)
	leal	-16(%ebp), %eax
	movl	%eax, 20(%ebp)
	jmp	.L62
.L67:
	movl	20(%ebp), %eax
	movl	%eax, 12(%esp)
	movl	16(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_write
	movl	%eax, -20(%ebp)
	cmpl	$0, -20(%ebp)
	jns	.L63
	movl	-20(%ebp), %eax
	jmp	.L64
.L63:
	cmpl	$0, -20(%ebp)
	je	.L69
.L65:
	movl	-20(%ebp), %eax
	addl	%eax, -12(%ebp)
	movl	-20(%ebp), %eax
	addl	%eax, 12(%ebp)
	movl	-20(%ebp), %eax
	subl	%eax, 16(%ebp)
.L62:
	cmpl	$0, 16(%ebp)
	jne	.L67
	jmp	.L66
.L69:
	nop
.L66:
	movl	-12(%ebp), %eax
.L64:
	leave
	ret
	.size	q_loop_write, .-q_loop_write
.globl q_close
	.type	q_close, @function
q_close:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	jmp	.L72
.L74:
	nop
.L72:
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	close
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jns	.L71
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$4, %eax
	je	.L74
.L71:
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_close, .-q_close
	.type	internal_thread_func, @function
internal_thread_func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	8(%ebp), %eax
	movl	%eax, -12(%ebp)
	call	pthread_self
	movl	-12(%ebp), %edx
	movl	%eax, (%edx)
	movl	-12(%ebp), %eax
	addl	$16, %eax
	movl	%eax, (%esp)
	call	q_atomic_inc
	movl	-12(%ebp), %eax
	movl	4(%eax), %edx
	movl	-12(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, (%esp)
	call	*%edx
	movl	-12(%ebp), %eax
	addl	$16, %eax
	movl	%eax, 4(%esp)
	movl	$2, (%esp)
	call	q_atomic_sub
	movl	$0, %eax
	leave
	ret
	.size	internal_thread_func, .-internal_thread_func
	.section	.rodata
.LC3:
	.string	"qsiFunc : thread_new error\n"
	.text
.globl q_thread_new
	.type	q_thread_new, @function
q_thread_new:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$20, (%esp)
	call	q_malloc
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	movl	8(%ebp), %edx
	movl	%edx, 4(%eax)
	movl	-12(%ebp), %eax
	movl	12(%ebp), %edx
	movl	%edx, 8(%eax)
	movl	-12(%ebp), %eax
	movl	$0, 12(%eax)
	movl	-12(%ebp), %eax
	addl	$16, %eax
	movl	$0, 4(%esp)
	movl	%eax, (%esp)
	call	q_atomic_set
	movl	-12(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%edx, 12(%esp)
	movl	$internal_thread_func, 8(%esp)
	movl	$0, 4(%esp)
	movl	%eax, (%esp)
	call	pthread_create
	testl	%eax, %eax
	jns	.L78
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC3, %eax
	movl	%edx, 12(%esp)
	movl	$27, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$0, %eax
	jmp	.L79
.L78:
	movl	-12(%ebp), %eax
	addl	$16, %eax
	movl	%eax, (%esp)
	call	q_atomic_inc
	movl	-12(%ebp), %eax
.L79:
	leave
	ret
	.size	q_thread_new, .-q_thread_new
.globl q_thread_delet
	.type	q_thread_delet, @function
q_thread_delet:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	movl	%eax, (%esp)
	call	pthread_detach
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	$0, 8(%ebp)
	leave
	ret
	.size	q_thread_delet, .-q_thread_delet
.globl q_thread_free
	.type	q_thread_free, @function
q_thread_free:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_thread_join
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	$0, 8(%ebp)
	leave
	ret
	.size	q_thread_free, .-q_thread_free
.globl q_thread_join
	.type	q_thread_join, @function
q_thread_join:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	12(%eax), %eax
	testl	%eax, %eax
	je	.L86
	movl	$-1, %eax
	jmp	.L87
.L86:
	movl	8(%ebp), %eax
	movl	$1, 12(%eax)
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	movl	$0, 4(%esp)
	movl	%eax, (%esp)
	call	pthread_join
.L87:
	leave
	ret
	.size	q_thread_join, .-q_thread_join
.globl q_thread_get_data
	.type	q_thread_get_data, @function
q_thread_get_data:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	popl	%ebp
	ret
	.size	q_thread_get_data, .-q_thread_get_data
.globl q_mutex_new
	.type	q_mutex_new, @function
q_mutex_new:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$4, 8(%esp)
	movl	$0, 4(%esp)
	leal	-16(%ebp), %eax
	movl	%eax, (%esp)
	call	memset
	movl	$24, (%esp)
	call	q_calloc
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	leal	-16(%ebp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	pthread_mutex_init
	movl	%eax, -20(%ebp)
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_mutex_new, .-q_mutex_new
.globl q_mutex_free
	.type	q_mutex_free, @function
q_mutex_free:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	$0, 8(%ebp)
	leave
	ret
	.size	q_mutex_free, .-q_mutex_free
.globl q_mutex_lock
	.type	q_mutex_lock, @function
q_mutex_lock:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
	.size	q_mutex_lock, .-q_mutex_lock
.globl q_mutex_try_lock
	.type	q_mutex_try_lock, @function
q_mutex_try_lock:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	pthread_mutex_trylock
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	je	.L98
	movl	$0, %eax
	jmp	.L99
.L98:
	movl	$1, %eax
.L99:
	leave
	ret
	.size	q_mutex_try_lock, .-q_mutex_try_lock
.globl q_mutex_unlock
	.type	q_mutex_unlock, @function
q_mutex_unlock:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
	.size	q_mutex_unlock, .-q_mutex_unlock
.globl q_cond_new
	.type	q_cond_new, @function
q_cond_new:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$48, (%esp)
	call	q_malloc
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_cond_new, .-q_cond_new
.globl q_cond_free
	.type	q_cond_free, @function
q_cond_free:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	leave
	ret
	.size	q_cond_free, .-q_cond_free
.globl q_cond_signal
	.type	q_cond_signal, @function
q_cond_signal:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
	.size	q_cond_signal, .-q_cond_signal
.globl q_cond_wait
	.type	q_cond_wait, @function
q_cond_wait:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	12(%ebp), %edx
	movl	8(%ebp), %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	pthread_cond_wait
	leave
	ret
	.size	q_cond_wait, .-q_cond_wait
.globl q_semaphore_new
	.type	q_semaphore_new, @function
q_semaphore_new:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$16, (%esp)
	call	q_malloc
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_semaphore_new, .-q_semaphore_new
.globl q_semaphore_free
	.type	q_semaphore_free, @function
q_semaphore_free:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	$0, 8(%ebp)
	leave
	ret
	.size	q_semaphore_free, .-q_semaphore_free
.globl q_semaphore_post
	.type	q_semaphore_post, @function
q_semaphore_post:
	pushl	%ebp
	movl	%esp, %ebp
	popl	%ebp
	ret
	.size	q_semaphore_post, .-q_semaphore_post
.globl q_semaphore_wait
	.type	q_semaphore_wait, @function
q_semaphore_wait:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
.L119:
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	sem_wait
	movl	%eax, -12(%ebp)
	cmpl	$0, -12(%ebp)
	jns	.L120
	call	__errno_location
	movl	(%eax), %eax
	cmpl	$4, %eax
	je	.L119
.L120:
	leave
	ret
	.size	q_semaphore_wait, .-q_semaphore_wait
.globl q_create_queue
	.type	q_create_queue, @function
q_create_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$16, (%esp)
	call	q_malloc
	movl	%eax, -12(%ebp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_calloc
	movl	%eax, %edx
	movl	-12(%ebp), %eax
	movl	%edx, 12(%eax)
	movl	-12(%ebp), %eax
	movl	$-1, (%eax)
	movl	-12(%ebp), %eax
	movl	$-1, 4(%eax)
	movl	-12(%ebp), %eax
	movl	8(%ebp), %edx
	movl	%edx, 8(%eax)
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_create_queue, .-q_create_queue
.globl q_destroy_queue
	.type	q_destroy_queue, @function
q_destroy_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	12(%eax), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	$0, 8(%ebp)
	leave
	ret
	.size	q_destroy_queue, .-q_destroy_queue
.globl q_get_queue
	.type	q_get_queue, @function
q_get_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$0, -12(%ebp)
	jmp	.L126
.L129:
	movl	-12(%ebp), %eax
	movl	12(%ebp), %edx
	leal	(%edx,%eax), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_pop_queue
	testl	%eax, %eax
	jne	.L131
.L127:
	addl	$1, -12(%ebp)
.L126:
	movl	-12(%ebp), %eax
	cmpl	16(%ebp), %eax
	jb	.L129
	jmp	.L128
.L131:
	nop
.L128:
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_get_queue, .-q_get_queue
.globl q_set_queue
	.type	q_set_queue, @function
q_set_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$0, -12(%ebp)
	jmp	.L133
.L134:
	movl	-12(%ebp), %eax
	movl	12(%ebp), %edx
	addl	%eax, %edx
	movl	20(%ebp), %eax
	movl	%eax, 8(%esp)
	movl	%edx, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_add_queue
	addl	$1, -12(%ebp)
.L133:
	movl	-12(%ebp), %eax
	cmpl	16(%ebp), %eax
	jb	.L134
	movl	-12(%ebp), %eax
	leave
	ret
	.size	q_set_queue, .-q_set_queue
.globl q_add_queue
	.type	q_add_queue, @function
q_add_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_isfull_queue
	testl	%eax, %eax
	je	.L137
	cmpl	$0, 16(%ebp)
	je	.L138
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_expand_queue
	jmp	.L137
.L138:
	movl	$-1, %eax
	jmp	.L139
.L137:
	movl	8(%ebp), %eax
	movl	4(%eax), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-12(%ebp)
	movl	8(%ebp), %eax
	movl	%edx, 4(%eax)
	movl	8(%ebp), %eax
	movl	12(%eax), %edx
	movl	8(%ebp), %eax
	movl	4(%eax), %eax
	addl	%eax, %edx
	movl	12(%ebp), %eax
	movzbl	(%eax), %eax
	movb	%al, (%edx)
	movl	$0, %eax
.L139:
	leave
	ret
	.size	q_add_queue, .-q_add_queue
	.section	.rodata
.LC4:
	.string	"q is empty\n"
	.text
.globl q_pop_queue
	.type	q_pop_queue, @function
q_pop_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_isempty_queue
	testl	%eax, %eax
	je	.L142
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC4, %eax
	movl	%edx, 12(%esp)
	movl	$11, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$-1, %eax
	jmp	.L143
.L142:
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-12(%ebp)
	movl	8(%ebp), %eax
	movl	%edx, (%eax)
	movl	8(%ebp), %eax
	movl	12(%eax), %edx
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	leal	(%edx,%eax), %eax
	movzbl	(%eax), %edx
	movl	12(%ebp), %eax
	movb	%dl, (%eax)
	movl	$0, %eax
.L143:
	leave
	ret
	.size	q_pop_queue, .-q_pop_queue
	.section	.rodata
.LC5:
	.string	"peek overflow\n"
	.text
.globl q_peek_queue
	.type	q_peek_queue, @function
q_peek_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_isempty_queue
	testl	%eax, %eax
	je	.L146
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC4, %eax
	movl	%edx, 12(%esp)
	movl	$11, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$-1, %eax
	jmp	.L147
.L146:
	movl	8(%ebp), %eax
	movl	4(%eax), %edx
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	subl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -28(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-28(%ebp)
	movl	%edx, %eax
	subl	$1, %eax
	cmpl	16(%ebp), %eax
	jge	.L148
	movl	stderr, %eax
	movl	%eax, %edx
	movl	$.LC5, %eax
	movl	%edx, 12(%esp)
	movl	$14, 8(%esp)
	movl	$1, 4(%esp)
	movl	%eax, (%esp)
	call	fwrite
	movl	$-1, %eax
	jmp	.L147
.L148:
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	addl	16(%ebp), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -28(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-28(%ebp)
	movl	%edx, -12(%ebp)
	movl	8(%ebp), %eax
	movl	12(%eax), %edx
	movl	-12(%ebp), %eax
	leal	(%edx,%eax), %eax
	movzbl	(%eax), %edx
	movl	12(%ebp), %eax
	movb	%dl, (%eax)
	movl	$0, %eax
.L147:
	leave
	ret
	.size	q_peek_queue, .-q_peek_queue
.globl q_size_queue
	.type	q_size_queue, @function
q_size_queue:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	movl	8(%ebp), %eax
	movl	4(%eax), %edx
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	subl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -4(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-4(%ebp)
	movl	%edx, %eax
	leave
	ret
	.size	q_size_queue, .-q_size_queue
.globl q_expand_queue
	.type	q_expand_queue, @function
q_expand_queue:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%ebx
	subl	$48, %esp
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%eax, %eax
	movl	%eax, (%esp)
	call	q_calloc
	movl	%eax, -12(%ebp)
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -28(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-28(%ebp)
	movl	%edx, -16(%ebp)
	cmpl	$1, -16(%ebp)
	jg	.L153
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	subl	$1, %eax
	movl	8(%ebp), %edx
	movl	12(%edx), %ecx
	movl	-16(%ebp), %edx
	leal	(%ecx,%edx), %edx
	movl	%eax, 8(%esp)
	movl	%edx, 4(%esp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	memcpy
	jmp	.L154
.L153:
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	subl	-16(%ebp), %eax
	movl	8(%ebp), %edx
	movl	12(%edx), %ecx
	movl	-16(%ebp), %edx
	leal	(%ecx,%edx), %edx
	movl	%eax, 8(%esp)
	movl	%edx, 4(%esp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	memcpy
	movl	-16(%ebp), %eax
	subl	$1, %eax
	movl	%eax, %ecx
	movl	8(%ebp), %eax
	movl	12(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, %ebx
	movl	-16(%ebp), %eax
	movl	%ebx, %esi
	subl	%eax, %esi
	movl	%esi, %eax
	addl	-12(%ebp), %eax
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	memcpy
.L154:
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%eax, %eax
	leal	-1(%eax), %edx
	movl	8(%ebp), %eax
	movl	%edx, (%eax)
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	leal	-2(%eax), %edx
	movl	8(%ebp), %eax
	movl	%edx, 4(%eax)
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	leal	(%eax,%eax), %edx
	movl	8(%ebp), %eax
	movl	%edx, 8(%eax)
	movl	8(%ebp), %eax
	movl	12(%eax), %eax
	movl	%eax, (%esp)
	call	q_free
	movl	8(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%edx, 12(%eax)
	addl	$48, %esp
	popl	%ebx
	popl	%esi
	popl	%ebp
	ret
	.size	q_expand_queue, .-q_expand_queue
	.section	.rodata
	.align 4
.LC6:
	.string	"buf size:%d front: buf[%d] rear: buf[%d]\n"
.LC7:
	.string	"buf: "
.LC8:
	.string	"0x%x "
	.text
.globl q_show_queue
	.type	q_show_queue, @function
q_show_queue:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$52, %esp
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	q_isempty_queue
	testl	%eax, %eax
	jne	.L160
	movl	8(%ebp), %eax
	movl	4(%eax), %ebx
	movl	8(%ebp), %eax
	movl	(%eax), %ecx
	movl	8(%ebp), %eax
	movl	8(%eax), %edx
	movl	$.LC6, %eax
	movl	%ebx, 12(%esp)
	movl	%ecx, 8(%esp)
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	$.LC7, %eax
	movl	%eax, (%esp)
	call	printf
	movl	8(%ebp), %eax
	movl	4(%eax), %edx
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	subl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	addl	%eax, %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -28(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-28(%ebp)
	movl	%edx, -16(%ebp)
	movl	$0, -12(%ebp)
	jmp	.L158
.L159:
	movl	8(%ebp), %eax
	movl	12(%eax), %ecx
	movl	8(%ebp), %eax
	movl	(%eax), %eax
	addl	-12(%ebp), %eax
	leal	1(%eax), %edx
	movl	8(%ebp), %eax
	movl	8(%eax), %eax
	movl	%eax, -28(%ebp)
	movl	%edx, %eax
	sarl	$31, %edx
	idivl	-28(%ebp)
	movl	%edx, %eax
	leal	(%ecx,%eax), %eax
	movzbl	(%eax), %eax
	movsbl	%al,%edx
	movl	$.LC8, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	addl	$1, -12(%ebp)
.L158:
	movl	-12(%ebp), %eax
	cmpl	-16(%ebp), %eax
	jl	.L159
	movl	$10, (%esp)
	call	putchar
.L160:
	addl	$52, %esp
	popl	%ebx
	popl	%ebp
	ret
	.size	q_show_queue, .-q_show_queue
	.ident	"GCC: (Ubuntu 4.4.3-4ubuntu5) 4.4.3"
	.section	.note.GNU-stack,"",@progbits
