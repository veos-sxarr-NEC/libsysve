/* Copyright (C) 2017-2019 by NEC Corporation
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
 * @file libsysve.c
 * @brief VE side library named "libsysve".
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sysve.h>
#include <veos_defs.h>
#include <libsysve.h>
#include <pthread.h>
#include <sys/syscall.h>

/*! \mainpage Introduction
 *
 * The libsysve package contains three libraries which are used by VE
 * programs to invoke VE-specific system calls or enable an additional
 * feature.
 *
 *  * libsysve: VH call, VH-VE SHM, Misc. API
 *  * libveio:  VE DMA, VE AIO
 *  * libveaccio: Accelerated I/O
 *
 * To build VE programs using libraris in the libsysve package,
 * libsysve-devel package needs to be installed.
 *
 * \author NEC Corporation
 * \copyright 2017-2023. Licensed under the terms of the MIT license.
 *
 * @par Revision History
 *   Revision,
 *   Date,
 *   Updates/Remarks
 * @par 
 *   Rev.18,
 *   Mar. 2023, @n
 *   This revision covers libsysve-3.0.1 or later. @n
 *   Support VE3. @n
 * @par
 *   Rev.17,
 *   Mar. 2022, @n
 *   This revision covers libsysve-2.11.0 or later. @n
 *   Add descriptions of API which gets VE product name. @n
 *   Add descriptions of APIs for VESHM. @n
 * @par
 *   Rev.16,
 *   Dec. 2021, @n
 *   This revision covers libsysve-2.10.0 or later. @n
 *   Update the example of VE DMA to show restrictions relating to alignments @n
 *   Update the API reference of VE Asynchronous I/O. @n
 * @par
 *   Rev.15,
 *   Sep. 2021, @n
 *   This revision covers libsysve-2.9.0 or later. @n
 *   Improve performance of Acceleration I/O. @n
 *   Add descriptions of API which returns the enabled/disabled of @n
 *   Acceleration I/O.
 * @par
 *   Rev.14,
 *   Mar. 2021, @n
 *   This revision covers libsysve-2.7.6 or later. @n
 *   Added "Revision History".
 * @par
 *   Rev.13,
 *   Dec. 2020, @n
 *   This revision covers libsysve-2.7.1 or later. @n
 *   Update descriptions of how many hugepages accelerated I/O uses. @n
 *   Remove the restriction on simultaneous use of accelerated I/O and @n
 *   ScaTeFS direct I/O.
 * @par
 *   Rev.12,
 *   Oct. 2020, @n
 *   This revision covers libsysve-2.7.1 or later. @n
 *   Add descriptions of API which gets maximum size of non-swappable memory.
 * @par
 *   Rev.11,
 *   Jul. 2020, @n
 *   This revision covers libsysve-2.6.2 or later. @n
 *   Add descriptions of API which gets PID of VEOS.
 */
/**
 * @brief This function gets the setting of PCI synchronization from PCISYAR and
 * PCISYMR.
 *
 * @param[in] The pair number of PCISYAR and PCISYMR (0, 1, 2, 3)
 * @param[out] pcisyar Variable to store the value gotten from PCISYAR
 * @param[out] pcisymr Variable to store the value gotten from PCISYMR
 *
 * @return 0 on Success, -1 on Failure.
 *
 * @internal
 *
 * @author libsysve
 */
int ve_get_pci_sync(uint8_t syncnum, uint64_t *pcisyar, uint64_t *pcisymr)
{
	int ret = 0;
	uint64_t snum = (uint64_t)syncnum;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_PCISYNC, snum,
							(uint64_t)pcisyar,
							(uint64_t)pcisymr);
	return ret;
}

/**
 * @brief This function gets fixed VEHVA mapping.
 *
 * @param[in] region A region specified by macros.
 * @param[out] vehva Variable to store the start address in VEHVA space of the
 * specified region.
 *
 * @return 0 on Success, -1 on Failure.
 *
 * @internal
 *
 * @author libsysve
 */
int ve_get_fixed_vehva(uint64_t region, uint64_t *vehva)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_FIXED_VEHVA, region, vehva);
	return ret;
}

/**
 * @brief This function sets the specified user register of the current VE core.
 *
 * @param[in] reg Register specified by macros.
 * @param[in] value Value to be set to the register.
 * When VE_REGID_PSW is specified as a register, only bit 0 (advanced-off mode)
 * is set to PSM register. Other bits are ignored.
 *
 * @return 0 on Success, -1 on Failure.
 *
 * @internal
 *
 * @author libsysve
 */
int ve_set_user_reg(uint64_t reg, uint64_t value)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_SET_USER_REG, reg, value);
	return ret;
}

/**
 * @brief This function gets information from a sysfs file of current VE node.
 *
 * @param[in] name A null-terminated name of sysfs file.
 * @param[out] Buffer to store the information.
 * @param[in] size Buffer size.
 *
 * @return The number of bytes of the gotten information, excluding the null
 * byte used to end output to string on Success, -1 on Failure.
 *
 * @internal
 *
 * @author libsysve
 */
ssize_t ve_get_ve_info(char *name, char *buffer, size_t size)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_VE_INFO, (uint64_t)name,
							(uint64_t)buffer, size);
	return ret;
}

/**
 * @brief This function set the thread to be created next time to be a VE worker thread.
 *
 * @retval  0 on sucess.
 * @retval  -1 is returned and errno is set on failure.
 */
int ve_set_next_thread_worker()
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_SET_NEXT_THREAD_WORKER);
	return ret;
}

/**
 * @brief This function set all the flags “ve_sched_state” 
 * of all threads except worker to VE_SCHED_STOPPING.
 *
 * @note If you create a thread in a worker thread after running
 * ve_stop_user_threads(), The thread you created may not be stopped.
 * Please run ve_stop_user_threads() again after creating the thread.
 *
 * @retval 0 on sucess.
 * @retval -1 is returned and errno is set on failure.
 *     - ESRCH No such process.
 *     - EPERM Invoked by user thread.
 */
int ve_stop_user_threads(void)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_STOP_USER_THREADS);
	return ret;
}

/**
 * @brief This function set all the flags “ve_sched_state” 
 * of all threads except worker to VE_SCHED_STARTED.
 * 
 * @retval 0 on sucess.
 * @retval -1 is returned and errno is set on failure.
 *     - ESRCH No such process.
 *     - EPERM Invoked by user thread.
 */
int ve_start_user_threads(void)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_START_USER_THREADS);
	return ret;
}

/**
 * @brief This function check all the flags “ve_sched_state” 
 * of all threads except worker.
 * 
 * @retval 0 on sucess.
 * @retval -1 is returned and errno is set on failure.
 *     - ESRCH No such process or Unable to find user threads.
 */
int ve_get_user_threads_state(void)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_USER_THREADS_STATE);
	return ret;
}

/**
 * \defgroup misc MISC
 *
 * Miscellaneous APIs.
 * Please include "libsysve.h" in the source file.
 *
 */
/*@{*/


/**
 * @brief This function returns NUMA node number on which process is running
 *
 * @param[out] node NUMA node number is stored into the integer pointed by it.
 *
 * @retval  0 on sucess. 
 * @retval  -1 is returned and errno is set on failure.
 *  - EFAULT  Argument points an invalid address.
 *
 * @internal
 *
 * @author libsysve
 */
int ve_get_numa_node(unsigned *node)
{
	int ret = 0;
	ret = syscall(__NR_getcpu, NULL, node, NULL);
	return ret;
}

/**
 * @brief this functions returns VEOS pid managing this process.
 *
 * @retval pid of VEOS on success.
 * @retval -1 is returned and errno is set on failure.
 *
 * @internal
 *
 * @author libsysve
 **/
int ve_get_veos_pid(void)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_VEOS_PID);
	return ret;
}

/**
 * @brief This function gets maximum size of non-swappable memory of a caller
 *        process at that time.
 *
 * @param[out] size Pointer to the value in which maximum size in byte of
 *                  non-swappable memory will be stored.
 *
 * @retval  0 on sucess.
 * @retval  -1 is returned and errno is set on failure.
 */
int ve_get_nonswappable(uint64_t *size)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_MNS, (uint64_t)size);
	return ret;
}

/**
 * @brief This function returns RDMA_OFFSET, start address and size of VE memory
 *        virtual address region.
 *
 * @param[out] addr Pointer to a variable which stores start address of
 *                  VE memory virtual address region.
 * @param[out] size Pointer to a variable which stores size of VE memory virtual
 *                  address region.
 * @param[out] offset Pointer to a variable which stores RDMA_OFFSET.
 *
 * @retval  0 on sucess.
 * @retval  -1 is returned and errno is set on failure.
 */
int ve_get_vemva_region(uint64_t *addr, size_t *size, int64_t *offset)
{
	int ret;
	ret = syscall(SYS_sysve, VE_SYSVE_VEMVA_REGION,
			(uint64_t)addr, (uint64_t)size, (uint64_t)offset);
	return ret;
}

/**
 * @brief This function gets VE product neme from a sysfs file of current VE node.
 *
 * @param[out] name Address of buffer to store VE product name.
 *                  VE product name will be a null-terminated string even if buffer size 
 *                  is small or equal to the size of information.
 * @param[in] size Buffer (name) size.
 *
 * @retval  0 on sucess.
 * @retval  -1 is returned and errno is set on failure.
 *
 * @internal
 *
 * @author libsysve
 */
int ve_get_ve_product_name(char *name, size_t size)
{
	int ret = 0;
	ret = syscall(SYS_sysve, VE_SYSVE_GET_VE_PRODUCT_NAME, (uint64_t)name, size);
	return ret;
}

/**
 * \ingroup veaccio
 *
 * Accelerated IO APIs.
 * Please include "libsysve.h" in the source file.
 *
 */
/*@{*/

/**
 * @brief This is a function of libsysve which checks accelerated IO is enabled
 *        or disabled.
 *
 * @note Even if user sets the environment variable to enable accelerated IO,
 *       accelerated IO might be disabled in case of no enough resource.
 *       At that time, this function returns 0. @n
 *       If a multi-thread program requests IOs in parallel,
 *       all of them might not be handled as accelerated IOs.
 *       Some of them might be handled as normal IOs due to lack of resources.
 *       This function returns 1 if at least one IO is handled as an accelerated
 *       IO.
 *
 * @retval   0  Accelerated IO is disabled.
 * @retval   1  Accelerated IO is enabled.
 * @retval  -1  On failure
 */
int
ve_is_acc_io_enabled(void)
{
	int ret = -1;

	ret = syscall(SYS_sysve, VE_SYSVE_IS_ACC_IO_ENABLED);
	return ret;
}
/** @} */ 
/**
 * @brief This is a function of libsysve which is used to get the proginf
 * information.
 *
 * @param[in] version User specifies data structure version.
 * @param[out] buffer A buffer for storing requested proginf data.
 *
 * @retval 0 on success.
 * @retval -1 is returned and errno is set on failure.
 */
int get_proginf_data(int version, void *buffer)
{
	int ret = -1;

	ret = syscall(SYS_sysve, VE_SYSVE_GET_PROGINF_DATA, version, buffer);
	return ret;
}

/**
 * @brief This is a function of libsysve which  sends request 
 * to pseudo process to get original address from specified address.
 *
 * @param[in] Address of modified code
 *
 * @retval 0 over is the address of original code
 * @retval -1 is returned and errno is set on failure.
 */
int64_t ve_get_original_addr(uint64_t mod_vemva)
{
	int64_t ret = -1;
	ret = syscall(SYS_sysve, VE_SYSVE_GETORGADDR, mod_vemva);
	return ret;
}
