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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include "vedma_impl.h"

#if !defined(NO_VE_MAP_DMADES)
#include <userdma.h>
#else

static inline int
ve_map_dmades(uint64_t *vehva_dmades, uint64_t *vehva_dmactl)
{
	*vehva_dmades = 0x4000UL;
	*vehva_dmactl = 0x3010UL;
	return 0;
}
#endif

#define SUCCESS 0
#define FAIL -1

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static int ve_dma_initialized = 0;

/**
 * @struct vedma_vars
 * @brief This structure hold the state of the DMA descriptor table.
 */
struct vedma_vars {
	uint64_t	vedma_desc; /*! VE host virtual address of DMA
				      descriptor table */
	uint64_t	vedma_lock; /*! A spin lock for exclusive control.
				      Set to 0 when it is not locked 
				      and non-0 to lock it */
	uint64_t	vedma_index; /*! The index number of the DMA
				       descriptor to be used next.
				       The value is 0-127. */
	int		*vedma_status[VEDMA_NDESC]; /*! DMA status */
} vedma_vars;
uint64_t	vedma_ctrl;

int 
ve_dma_init(void)
{
	int	ret = SUCCESS;
	int	i;

	if (pthread_mutex_lock(&init_lock) != 0) {
		return FAIL;
	}

	if(ve_dma_initialized){
		goto init_unlock;
	}

	ret = ve_map_dmades(&vedma_vars.vedma_desc, &vedma_ctrl);
	if (ret != 0) {
		return ret;
	}
	vedma_vars.vedma_index = 0;
	vedma_vars.vedma_lock = 0;
	for (i = 0; i < VEDMA_NDESC; i++) {
		vedma_vars.vedma_status[i] = NULL;
	}
	ve_dma_initialized = 1;

init_unlock:
	if (pthread_mutex_unlock(&init_lock) != 0) {
		return FAIL;
	}

	return ret;
}

static void
ve_dma_clear_skip_flag(void)
{
	ve_dma_initialized = 0;
}

static __attribute__((constructor)) void
init(void)
{
	if (pthread_atfork(NULL, NULL, ve_dma_clear_skip_flag)) {
		exit(1);
	}
}
