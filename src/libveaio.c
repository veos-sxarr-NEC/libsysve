/* Copyright (C) 2018 by NEC Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
/**
 *  @file libveaio.c
 *  @brief Library of VEAIO API
 */
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sysve.h>
#include <veaio_defs.h>
#include "veaio.h"
#include <stdio.h>
#include <veos_defs.h>
#include <unistd.h>

static pthread_mutex_t ve_aio_setup_lock = PTHREAD_MUTEX_INITIALIZER;
static int ve_aio_setup = 0;

/**
 * \defgroup veaio VE AIO
 *
 * VE AIO is a asynchronous I/O feature for VE programs.
 * VE programs can do their own tasks while data transfer between VE
 * and VH and actual I/O are done asynchronously.
 *
 * Please include "veaio.h".
 * Please specify "-lveio -pthread" option to the compiler driver to
 * link libveio.
 *
 * @note VE AIO expects an ordinary file opened without O_DIRECT flag.
 *       Socket, pipe or fifo can't be used.
 * @note Calling fork() or vfork() when read/write operation status is
 *       in progress, invoking AIO API results in undefined
 *       behavior. Check read/write operation status by using
 *       ve_aio_query() or synchronize read/write operation by using
 *       ve_aio_wait().
 * @note Pseudo process(ve_exec) creates IO worker threads on VH and each 
 *       thread allocates IO buffer and opens two files to communicate 
 *       with VEOS. IO request is divided and processed non-atomically 
 *       under multithreading by default. Following environment 
 *       variables can be set to change from default.
 *         - VE_ASYNC_IO_THREAD To specify the number of VEAIO worker 
 *         threads per VE process. This is valid only when 
 *         VE_ASYNC_IO_ATOMIC is 0. Default is 4.
 *         - VE_ASYNC_IO_BUFSIZE To specify IO buffer size(Byte) per 
 *         worker thread. This is valid only when VE_ASYNC_IO_ATOMIC is 
 *         0. Default is 8MB.
 *         - VE_ASYNC_IO_ATOMIC Set this to 1 and VEAIO worker threads 
 *         process IO operation atomically. Default is 0. The number of 
 *         bytes read or written at once is restricted to 
 *         2147479552Byte(2GB - 4KB) same as pread64/pwrite64.
 * @note If VE thread which has submitted read/write request terminates 
 *       before completion of read/write operation, then VE process 
 *       gets abnormal termination.
 */
/*@{*/

/**
 * @brief This function returns a new ve_aio_ctx managing a read/write request.
 *
 * @note User must initialize a ve_aio_ctx for AIO operation
 * @note User must invoke this function as many as multiplicity
 *       of read/write requests
 * @note For example, initialize two contexts to submit two read/write requests
 *       at a time
 * @note Context can be reused after completion of previous request
 * @retval ve_aio_ctx on success
 * @retval NULL on failure.
 */
struct ve_aio_ctx *
ve_aio_init(void)
{
	int err;
	struct ve_aio_ctx *ctx;
	pthread_mutex_lock(&ve_aio_setup_lock);
	if (ve_aio_setup == 0) {
		int ret = syscall(SYS_sysve, VE_SYSVE_AIO2_INIT);
		if (ret) {
			err = errno;
			pthread_mutex_unlock(&ve_aio_setup_lock);
			errno = err;
			return NULL;
		}
		ve_aio_setup++;
	}
	pthread_mutex_unlock(&ve_aio_setup_lock);


	ctx = calloc(1, sizeof(struct ve_aio_ctx));
	if (NULL == ctx) {
		errno = ENOMEM;
		return NULL;
	}

	if (0 != pthread_mutex_init(&ctx->ve_aio_status_lock, NULL)) {
		errno = ENOMEM;
		free(ctx);
		return NULL;
	}
	ctx->status = VE_AIO_COMPLETE;
	ctx->result.retval = 0;
	ctx->result.errnoval = 0;

	return ctx;
}

/**
 * @brief This function releases context managing a read/write request.
 *
 * @param[in] ctx Context to be released
 *
 * @retval  0 on success
 * @retval -1 on failure and following errno is set
 * - EINVAL  Context in arguments is invalid
 * - EBUSY  Request for this context is in progress
 */
int
ve_aio_fini(struct ve_aio_ctx *ctx)
{
	if (NULL == ctx) {
		/* set error EINVAL  */
		errno = EINVAL;
		return -1;
	}
	if (ctx->result.binary < 0) {
		/* set error EBUSY */
		errno = EBUSY;
		return -1;
	}
	pthread_mutex_destroy(&ctx->ve_aio_status_lock);
	free(ctx);

	return 0;
}

/**
 * @brief This function starts asynchronous read.
 *
 * @note This function internally invokes pread() system call at VH side
 * @note Context can be reused after completion of previous request
 *
 * @param[in] ctx Context managing this request
 * @param[in] fd File descriptor which refer to a file this function reads to
 * @param[in] count Number of bytes read
 * @param[out] buf Buffer this function reads from
 * @param[in] offset File offset
 *
 * @retval  0 on success
 * @retval -1 on failure and following errno is set
 * - EINVAL  Context in arguments is invalid
 * - EBUSY  Not complete the previous read request for this context
 * - EAGAIN  No resource to accept this request
 * - ENOMEM  No memory to accept this request on host
 */
int
ve_aio_read(struct ve_aio_ctx *ctx, int fd, ssize_t count, void *buf,
		off_t offset)
{
	if (NULL == ctx) {
		errno = EINVAL;
		return -1;
	}

	pthread_mutex_lock(&ctx->ve_aio_status_lock);
	if (ctx->result.binary < 0) {
		pthread_mutex_unlock(&ctx->ve_aio_status_lock);
		errno = EBUSY;
		return -1;
	}
	ctx->result.active = 1;
	pthread_mutex_unlock(&ctx->ve_aio_status_lock);

	if (syscall(SYS_sysve, VE_SYSVE_AIO2_READ, ctx, fd, count, buf,
			offset)) {
		pthread_mutex_lock(&ctx->ve_aio_status_lock);
		ctx->result.active = 0;
		pthread_mutex_unlock(&ctx->ve_aio_status_lock);
		return -1;
	}

	return 0;
}

/**
 * @brief This function starts asynchronous write.
 *
 * @note This function internally invokes pwrite() system call at VH side
 * @note Context can be reused after completion of previous request
 *
 * @param[in] ctx Context managing this request
 * @param[in] fd File descriptor which refer to a file function writes to
 * @param[in] count Number of bytes written
 * @param[in] buf Buffer this function writes from
 * @param[in] offset File offset
 *
 * @retval  0 on success
 * @retval -1 on failure and following errno is set
 * - EINVAL  Context in arguments is invalid
 * - EBUSY  Not complete the previous read request for this context
 * - EAGAIN  No resource to accept this request
 * - ENOMEM  No memory to accept this request on host
 */
int
ve_aio_write(struct ve_aio_ctx *ctx, int fd, ssize_t count, void *buf,
                off_t offset)
{
	if (NULL == ctx) {
                errno = EINVAL;
                return -1;
        }

        pthread_mutex_lock(&ctx->ve_aio_status_lock);
        if (ctx->result.binary < 0) {
                pthread_mutex_unlock(&ctx->ve_aio_status_lock);
                errno = EBUSY;
                return -1;
        }
        ctx->result.active = 1;
        pthread_mutex_unlock(&ctx->ve_aio_status_lock);

        if (syscall(SYS_sysve, VE_SYSVE_AIO2_WRITE, ctx, fd, count, buf,
                        offset)) {
                pthread_mutex_lock(&ctx->ve_aio_status_lock);
                ctx->result.active = 0;
                pthread_mutex_unlock(&ctx->ve_aio_status_lock);
                return -1;
        }

	return 0;
}

/**
 * @brief This function gets state of read/write operation for the context.
 *
 * @note If argument *retval or *errnoval below is NULL, status of read/write
 *       can be got but return value or error number can not be got
 *
 * @param[in] ctx Context which this function gets some information from
 * @param[out] retval Pointer to get return value of pread()/pwrite()
 * @param[out] errnoval Pointer to get error number of pread()/pwrite()
 *
 * @retval  0 on completion of read/write and set retval and errnoval
 * @retval  1 on incompletion read/write
 * @retval -1 on failure of query and following errno is set
 * - EINVAL  Context in arguments is invalid
 */
int
ve_aio_query(struct ve_aio_ctx *ctx, ssize_t *retval, int *errnoval)
{
	if (NULL == ctx) {
		/* set error EINVAL  */
		errno = EINVAL;
		return -1;
	}
	if (ctx->result.binary < 0)
		return 1;
	if (NULL != retval)
		*retval = ctx->result.retval;
	if (NULL != errnoval)
		*errnoval = ctx->result.errnoval;
	return 0;
}

/**
 * @brief This function waits read/write request for the context.
 *
 * @note When read or write is failure (actually, pread or pwrite is used
 *       internally), its error number is set
 * @note This function blocks VE process so VEOS finds other process to schedule
 * @note After context was reused, this function gets result of latest
 *        request for ve_aio_ctx
 * @note When this function fails, retval and errnoval are not set
 *
 * @param[in] ctx Context managing read/write request
 * @param[out] retval Pointer to get return value of pread()/pwrite()
 * @param[out] errnoval Pointer to get error number of pread()/pwrite()
 *
 * @retval  0 on completion of read/write and set retval and errnoval
 * @retval -1 on failure of wait and following errno is set
 * - EINVAL  Context in arguments is invalid
 * - EBUSY  Context in arguments has already managed new read/write request and
 *   failure to get result of previous request
 */
int
ve_aio_wait(struct ve_aio_ctx *ctx, ssize_t *retval, int *errnoval)
{
	int ret;
	if (NULL == ctx) {
		errno = EINVAL;
		return -1;
	}
	if (ctx->result.binary >= 0)
		goto hndl_ret;

	ret = syscall(SYS_sysve, VE_SYSVE_AIO2_WAIT, ctx);
	if (ret != 0)
		return ret;

	if (ctx->result.binary < 0) {
		errno = EBUSY;
		return -1;
	}
hndl_ret:
	if (NULL != retval)
		*retval = ctx->result.retval;
	if (NULL != errnoval)
                        *errnoval = ctx->result.errnoval;
	return 0;
}
