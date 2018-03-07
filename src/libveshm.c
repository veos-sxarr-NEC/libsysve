/* Copyright (C) 2017-2018 by NEC Corporation
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
 * @file  libveshm.c
 * @brief Library of VESHM API 
 */
#include <unistd.h>
#include <inttypes.h>
#include <veos_defs.h>
#include <sysve.h>
#include "veshm.h"

#define VESHM_MAX_ARGS		5

/**
 * @brief Register a VESHM area
 *
 * @parm [in] vemva	Virtual address of VESHM area
 * @parm [in] size	Size in byte
 * @parm [in] syncnum   Pair number of PCISYR/PCISMR
 * @parm [in] mode_flag Mode flag
 *
 * @return 0 on success, -1 and set errno on failure
 */
int 
ve_shared_mem_open(void *vemva, size_t size, int syncnum, int mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_OPEN, vemva, size, 
		       (uint64_t)syncnum, (uint64_t)mode_flag);
}

/**
 * @brief Attach a VESHM area
 *
 * @parm [in] pid	Pid of an owner process
 * @parm [in] veshm_vemva Virtual address of VESHM area
 * @parm [in] size	Size in byte
 * @parm [in] syncnum   Pair number of PCISYR/PCISMR
 * @parm [in] mode_flag Mode flag
 *
 * @return Attached address on success, -1 and set errno on failure
 */
void *
ve_shared_mem_attach(pid_t pid, void *veshm_vemva, size_t size, 
		     int syncnum, int mode_flag)
{
	uint64_t arg[VESHM_MAX_ARGS];
	uint64_t ret;

	/* struct veshm_args args; */
	arg[0] = (uint64_t) pid;
	arg[1] = (uint64_t) veshm_vemva;
	arg[2] = (uint64_t) size;
	arg[3] = (uint64_t) syncnum;
	arg[4] = (uint64_t) mode_flag;

	ret = syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		      (uint64_t)VESHM_ATTACH, arg);
	return((void *)ret);
}

/**
 * @brief Detach a VESHM area
 *
 * @parm [in] veshm_addr Virtual address of an attached VESHM area
 * @parm [in] mode_flag Mode flag
 *
 * @return 0 on success, -1 and set errno on failure
 */
int 
ve_shared_mem_detach(void *veshm_addr, int mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_DETACH, (uint64_t)veshm_addr, 
		       (uint64_t)mode_flag);
}

/**
 * @brief Erace a VESHM area (Not free)
 *
 * @parm [in] vemva	Virtual address of VESHM area
 * @parm [in] size	Size in byte
 * @parm [in] syncnum   Pair number of PCISYR/PCISMR
 * @parm [in] mode_flag Mode flag
 *
 * @return 0 on success, -1 and set errno on failure
 */
int 
ve_shared_mem_close(void *vemva, size_t size, int syncnum, int mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_CLOSE, vemva, 
		       (uint64_t)size, (uint64_t)syncnum, 
		       (uint64_t)mode_flag);
}

/**
 * @brief Get page size of a specified address.
 *
 * @param [in] mode_flag  Mode flag
 * @param [in] pid	  Target pid
 * @param [in] address	  Target address
 *
 * @return Page size or -1 on failure
 * @retval 0x200000 (2MB)
 * @retval 0x4000000 (64MB)
 */
int
ve_get_pgmode(int mode_flag, int pid, void *address)
{
	return (int)syscall((uint64_t)SYS_sysve, 
			    (uint64_t)VE_SYSVE_VESHM_CTL, 
			    VESHM_PGSIZE, mode_flag, pid, address);
}


