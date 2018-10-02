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
#ifndef _VEDMA_IMPL_H_
#define _VEDMA_IMPL_H_

#include "vedma.h"
#include <sys/types.h>

#define VEDMA_NDESC	128
#define VEDMA_DESC_SIZE	32

#define vedma_spin_lock(p)					\
do {								\
	uint64_t	*lp = (p);				\
	asm volatile(						\
		"# vedma_spin_lock\n"				\
		"	or	%%s63, 0, %%sl\n"		\
		"	cas.l	%%s63, 0(%0), 0\n"		\
		"	brne.l.t	%%s63, 0, -8\n"		\
		"	fencem	2\n"				\
		:: "r"(lp)					\
		: "s63", "memory");				\
} while(0)

#define vedma_spin_unlock(p)					\
do {								\
	uint64_t	*lp = (p);				\
	asm volatile(						\
		"# vedma_spin_unlock\n"				\
		"	fencem	1\n"				\
		"	or	%%s63, 0, (0)1\n"		\
		"	st	%%s63, 0(%0)\n"			\
		:: "r"(lp)					\
		: "s63", "memory");				\
} while(0)

#define vedma_write_dmadesc(desc, dst, src, size)		\
do {								\
	asm volatile(						\
		" # vedma_write_dmadesc\n"			\
		"	shm.l	%0, 0x08(%3)\n"			\
		"	shm.l	%1, 0x10(%3)\n"			\
		"	shm.l	%2, 0x18(%3)\n"			\
		"	or	%%s63, 0, (63)0\n"		\
		"	shm.l	%%s63, 0(%3)\n"			\
		:: "r"(dst), "r"(src), "r"(size), "r"(desc)	\
		: "s63");					\
} while(0)

static inline uint64_t vedma_lhm64(uint64_t p) __attribute__((always_inline));
static inline uint64_t vedma_lhm64(uint64_t p)
{
	asm volatile(
		"	lhm.l	%0, 0(%0)\n"
		: "+r"(p));
	return p;
}

#endif /* _VEDMA_H_ */
