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
 * @file    veaio.h
 * @brief   Header file for VEAIO API.
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __VE_AIO_H
#define __VE_AIO_H

#include <sys/types.h>


struct ve_aio2_ctx;
#define ve_aio_ctx ve_aio2_ctx

struct ve_aio_ctx *ve_aio_init(void);
int ve_aio_fini(struct ve_aio_ctx *ctx);
int ve_aio_write(struct ve_aio_ctx *ctx, int fd, ssize_t count, void *buf,
		off_t offset);
int ve_aio_read(struct ve_aio_ctx *ctx, int fd, ssize_t count, void *buf,
		off_t offset);
int ve_aio_wait(struct ve_aio_ctx *ctx, ssize_t *retval, int *errnoval);
int ve_aio_query(struct ve_aio_ctx *ctx, ssize_t *retval, int *errnoval);

#endif

#ifdef __cplusplus
}
#endif
