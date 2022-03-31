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
 * \defgroup veoveshmapi VESHM (low-level)
 *
 * VESHM is a feature to share VE memory between VE processes.
 *
 * The APIs of VESHM are low-level and are intended to be called by
 * upper layer software such as NEC MPI. They are not intended to be
 * called by a user program. Please use NEC MPI to share data on VE
 * memory between VE processes.
 * 
 * If you want to use VESHM APIs directly to implement upper layer
 * software, include "veshm.h" header.
 *
 * @note User programs linked with NEC MPI should not use these APIs
 *       directly, because the MPI library use them internally.
 */

//@{

/**
 * @brief Register a VESHM area
 *
 * @param [in] vemva	Virtual address of VESHM area
 * @param [in] size	Size in byte
 * @param [in] syncnum   Pair number of PCISYR/PCISMR
 * -    Physical register number: 0-3 (Supported 0 only)
 * @param [in] mode_flag Mode flag which is ORed value of the following macros
 * -    VE_REGISTER_PCI:	Set up a memory as VESHM and 
 * 				register the memory with PCIATB.
 * 				The values of vemva and size need to
 * 				be aligned on the PCIATB page size (the
 * 				PCIATB page size of almost all of the
 * 				models of VEs is 64 MB).
 *				A caller process specifies this flag
 *				to allow VE processes on remote VEs
 *				and the local VE to access the specified
 *				memory.
 * -    VE_REGISTER_NONE: 	Set up a memory as VESHM and 
 * 				NOT register the memory with PCIATB.
 * 				The values of vemva and size need to
 * 				be aligned on the page size of VE
 * 				memory(default:64MB).
 *				A caller process specifies this flag
 *				to allow VE processes on the local VE
 *				to access the specified memory.
 * -    VE_PCISYNC: 		Enable synchronization.
 *                              VE_REGISTER_PCI must be specified.
 * -    VE_SHM_RO: 		Set "Read Only" permission.
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid value. (E.g., it is negative value, or too big)
 * - EINVAL	Invalid value. (Different paze size from PCIATB)
 * - ENOMEM	Creating a VESHM failed.
 * - EACCESS	Permission denied.
 * - ECANCELED	Operation canceled.
 */
int 
ve_shared_mem_open(void *vemva, size_t size, int syncnum, long long mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_OPEN, vemva, size, 
		       (uint64_t)syncnum, (uint64_t)mode_flag);
}

/**
 * @brief Attach a VESHM area
 *
 * @param [in] pid	Pid of an owner process
 * @param [in] veshm_vemva Virtual address of VESHM area
 * @param [in] size	Size in byte
 * @param [in] syncnum   Pair number of PCISYR/PCISMR
 * -    Physical register number: 0-3 (Supported 0 only)
 * @param [in] mode_flag Mode flag which is ORed value of one of the
 *             following macros and the same value as the argument of
 *             ve_shared_mem_open()
 * -	VE_REGISTER_VEHVA:	VESHM on a remote VE will be attached to 
 *  				VEHVA (using DMAATB).
 *				A caller process specifies this flag to
 *				access the memory registered by a VE process
 *				running on a remote VE.
 *				The caller process can transfer data using
 *				ve_dma_post() or ve_dma_post_wait() with
 *				returned address.
 * -	VE_REGISTER_VEMVA:	VESHM on the local VE will be attached to 
 *				VEMVA (using ATB).
 *				A caller process specifies this flag to
 *				access the memory registered by a VE process
 *				running on the local VE.
 *				The caller process can transfer data using
 *				memcpy() with returned address.
 * -	VE_MEM_LOCAL:		An own memory will be attached to VEHVA
 *                              (using DMAATB).
 *
 * @note A VESHM area is recognized by a combination of (vemva, size,
 *       syncnum, mode_flag) which are arguments of this function.
 *
 * @retval Attached-address On success
 * @retval 0xffffffffffffffff On failure (set errno)
 * - EINVAL	Invalid value.
 * - EFAULT	Bad address.
 * - ESRCH	No such process.
 * - ENOENT	No such memory.
 * - ENOMEM	Cannot attach VESHM.
 * - EACCESS	Cannot attach VESHM.
 * - EACCESS	Permission denied.
 */
void *
ve_shared_mem_attach(pid_t pid, void *veshm_vemva, size_t size, 
		     int syncnum, long long mode_flag)
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
 * @param [in] veshm_addr Virtual address of an attached VESHM area
 * @param [in] mode_flag Mode flag which is one of the following macros
 * -	VE_REGISTER_VEHVA:	VESHM on a remote VE will be detached from 
 *  				VEHVA (using DMAATB).
 * -	VE_REGISTER_VEMVA:	VESHM on the local VE will be detached from 
 *  				VEMVA (using ATB).
 * -	VE_MEM_LOCAL:		An own memory will be detached from VEHVA
 *                              (using DMAATB).
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid argument.
 * - ECANCELED	No such memory.
 */
int 
ve_shared_mem_detach(void *veshm_addr, long long mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_DETACH, (uint64_t)veshm_addr, 
		       (uint64_t)mode_flag);
}

/**
 * @brief Unregister a VESHM area
 *
 * @param [in] vemva	Virtual address of VESHM area
 * @param [in] size	Size in byte
 * @param [in] syncnum   Pair number of PCISYR/PCISMR
 * @param [in] mode_flag Mode flag which is the same value as the
 *                       argument of ve_shared_mem_open().
 *
 * @retval 0 On success
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid value.
 * - ECANCELED	Operation canceled.
 */
int 
ve_shared_mem_close(void *vemva, size_t size, int syncnum, long long mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_VESHM_CTL, 
		       (uint64_t)VESHM_CLOSE, vemva, 
		       (uint64_t)size, (uint64_t)syncnum, 
		       (uint64_t)mode_flag);
}

/**
 * @brief Get the page size of a specified process and a specified address.
 *
 * @param [in] mode_flag  Mode flag which is one of the following macros
 * - VE_ADDR_VEMVA: The page size of VE memory mapped to VEMVA using ATB.
 * - VE_ADDR_VEHVA: The page size of VE memory mapped to VEHVA using DMAATB.
 * - VE_ADDR_PCI:   The page size of PCIATB. The value of "address" will be
 *                  ignored.
 * @param [in] pid	  Target pid
 * @param [in] address	  Target address
 *
 * @retval 0x200000 (2MB)
 * @retval 0x4000000 (64MB)
 * @retval -1 On failure (set errno)
 * - EINVAL	Invalid argument.
 * - EFAULT     Bad address.
 * - ESRCH      No such process.
 * - ENOTSUP	Operation not supported.
 * - ECANCELED	Operation canceled.
 */
int
ve_get_pgmode(long long mode_flag, int pid, void *address)
{
	return (int)syscall((uint64_t)SYS_sysve, 
			    (uint64_t)VE_SYSVE_VESHM_CTL, 
			    VESHM_PGSIZE, mode_flag, pid, address);
}
