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
 * @file  libvedma.c
 * @brief Library of register/unregister VE local memory to DMAATB  
 */
#include <unistd.h>
#include <inttypes.h>
#include <veos_defs.h>
#include <sysve.h>
#include "veshm.h"


/**
 * @brief This function registers VE local memory to DMAATB
 *
 * @note The default value of VE page size is 64MB. @n
 * 	In this case, values of vemva and size need to be aligned with 64MB boundary. @n
 * 	If VE page size is 2MB, values of vemva and size need to be aligned with 2MB boundary.
 *
 * @param[in] vemva	An address of memory to be registered @n
 * 	The address needs to be aligned with 64MB boundary.
 * @param[in] size	Size of memory aligned aligned with 64MB boundary
 *
 * @retval vehva VE host virtual address on success
 * @retval  0xffffffffffffffff On failure
 * - EINVAL Invalid argument
 * - EFAULT Bad address
 * - ESRCH No such process
 * - ENOENT No such memory
 * - EACCES Permission denied
 * - ECANCEL Operation canceled
 */
uint64_t
ve_register_mem_to_dmaatb(void *vemva, size_t size)
{
	pid_t pid = getpid();
	int syncnum = 0;
	int mode_flag = VE_MEM_LOCAL;

	return (uint64_t)ve_shared_mem_attach(pid, vemva, size, syncnum, mode_flag);
}

/**
 * @brief This function unregisters VE local memory from DMAATB 
 *
 * @param [in] vehva VE host virtual address of memory to be unregistered
 *
 * @retval 0 On success
 * @retval -1 On failure
 * - EINVAL Invalid argument
 * - ECANCELED No such memory
 */
int
ve_unregister_mem_from_dmaatb(uint64_t vehva) 
{
	int mode_flag = VE_MEM_LOCAL;

	return ve_shared_mem_detach((void *)vehva, mode_flag);
}


