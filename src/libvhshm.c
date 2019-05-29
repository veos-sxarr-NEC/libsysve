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
 * @file libvhshm.c
 * @brief VE side library functions for VH-VE shared memory.
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <sysve.h>
#include <unistd.h>
#include <inttypes.h>
#include <veos_defs.h>
#include <vhshm_defs.h>

#include "vhshm.h"

/**
 * \defgroup vhshm VH-VE SHM
 *
 * VH-VE SHM is a feature for VE programs to register System V shared
 * memory created at VH side to DMAATB of VE. 
 *
 * Please include "vhshm.h".
 *
 * @note VH-VE SHM feature does not provide a function to create
 *       System V shared memory. A user program at VH side needs to
 *       create System V shared memory.
 * @note System V shared memory needs to be created with SHM_HUGETLB
 *       flag to allocate huge pages (2MB pages).
 *       The kernel parameter "vm.nr_hugepages" needs to be set, in
 *       order to allocate huge pages.
 *        1. Check whether "vm.nr_hugepages" exists in /etc/sysctl.conf and files under /etc/sysctl.d and /usr/lib/sysctl.d.
 *        2. If "vm.nr_hugepages" does not exists, add "vm.nr_hugepages = 4096" to /etc/sysctl.conf. 4096 is tentative value. Please set required value for your program.
 *        3. If "vm.nr_hugepages" exists, add the required value to the current value of "vm.nr_hugepages"
 *        4. Execute "sysctl --system" command
 * @note VE programs can use VE DMA feature to transfer data between
 *       System V shared memory and VE memory registered to DMAATB.
 */
/*@{*/
/**
 * @brief This function gets the identifier of system V shared memory on VH.
 *
 * @note Argument is similar to shmget(2). Different points are shown below.
 * @note If a specified size is smaller than actual shared memory
 *       size, actual shared memory size is used.
 * @note Invoking this function is not required if the program knows
 *       the shared memory identifier.
 *
 * @param[in] key A key value associated with System V shared memory
 *            segment on VH. Don't specify IPC_PRIVATE.
 * @param[in] size Size of System V shared memory segment on VH
 * @param[in] shmflag SHM_HUGETLB must be specified. Don't specify
 *            SHM_NORESERVE IPC_EXCL and IPC_CREAT.
 *
 * @return A valid shared memory identifier on Success, -1 on Failure.
 * - EINVAL SHM_HUGETLB is not specified as 3rd argument.
 *          SHM_NORESERVE or IPC_EXCL or IPC_CREAT are specified as 3rd
 *          argument.
 * - EINVAL IPC_PRIVATE is specified as 1st argument.
 * - EINVAL A segment with given key existed, but size is greater than
 *          the size of the segment.
 * - EACCES The user does not have permission to access the shared
 *          memory segment, and does not have the CAP_IPC_OWNER capability.
 * - ENOENT No segment exists for the given key.
 *
 * @internal
 * @author VHSHM
 */
int vh_shmget(key_t key, size_t size, int shmflag)
{
	return syscall(SYS_sysve, VE_SYSVE_VHSHM_CTL, VHSHM_GET, (uint64_t)key,
					(uint64_t)size, (uint64_t)shmflag);
}

/**
 * @brief This function attaches system V shared memory on VH and register it with DMAATB.
 *
 * @note Argument is similar to shmat(2). Different points are shown below.
 * @note On Linux, it is possible to attach a shared memory segment even if it is already marked to be deleted. vh_shmat() follows it.
 *
 * @param[in] shmid System V shared memory segment identifier.
 * @param[in] shmaddr This argument must be NULL.
 * @param[in] shmflag SHM_RDONLY can be specified. Don't specify SHM_EXEC, SHM_REMAP and SHM_RND.
 * @param[in] vehva An address of pointer to store VEHVA.
 *
 * @return A valid shared memory identifier on Success, (void *)-1 on Failure.
 * - EINVAL    shmaddr is not NULL.
 * - EINVAL    SHM_EXEC, SHM_REMAP or SHM_RND are specified.
 * - EINVAL    Invalid shmid value.
 * - EFAULT    vehva is invalid or the specified segment is not huge page.
 * - ENOMEM    Can't allocate DMAATB more. No enough memory at VH side.
 * - ECANCELED Failed to update resource information (VEOS internal error).
 * - EACCES    The calling process does not have the required permissions
 *             for the requested attach type, and does not have the
 *             CAP_IPC_OWNER capability.
 * - ENOTSUP   VEOS does not connect to IVED.
 * - EIDRM     shmid points to a removed identifier.
 *
 * @internal
 * @author VHSHM
 */
void *vh_shmat(int shmid, const void *shmaddr, int shmflag, void **vehva)
{
	return (void *)syscall(SYS_sysve, VE_SYSVE_VHSHM_CTL, VHSHM_AT,
					(uint64_t)shmid, (uint64_t)shmaddr,
					(uint64_t)shmflag, (uint64_t)vehva);
}

/**
 * @brief This function detaches system V shared memory on VH and releases DMAATB entry.
 *
 * @param[in] shmaddr An address which is attached to System V shared memory on
 * VH
 *
 * @return 0 on Success, -1 on Failure.
 * - ECANCELED Failed to update resource information (VEOS internal error).
 * - EINVAL    shmaddr is invalid. There is no shared memory segment
 *             attached at shmaddr, or shmaddr is not aligned on a page
 *             boundary.
 *
 * @internal
 * @author VHSHM
 */
int vh_shmdt(const void *shmaddr)
{
	return syscall(SYS_sysve, VE_SYSVE_VHSHM_CTL,
						VHSHM_DT, (uint64_t)shmaddr);
}
