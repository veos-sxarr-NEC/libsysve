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
 * @brief This function gets a System V shred memory on VH.
 *
 * @param[in] key A key value associated with System V shared memory segment on
 * VH
 * @param[in] size Size of System V shared memory segment on VH
 * @param[in] shmflag Flags
 *
 * @return A valid shared memory identifier on Success, -1 on Failure.
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
 * @brief This function attach a System V shred memory on VH.
 *
 * @param[in] shmid System V shared memory segment identifier which is returned
 * by vh_shmget()
 * @param[in] shmaddr This argument must be NULL.
 * @param[in] shmflag Flags
 * @param[in] vehva An address of pointer to store VEHVA.
 *
 * @return A valid shared memory identifier on Success, (void *)-1 on Failure.
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
 * @brief This function detach a System V shred memory on VH.
 *
 * @param[in] shmaddr An address which is attached to System V shared memory on
 * VH
 *
 * @return 0 on Success, -1 on Failure.
 *
 * @internal
 * @author VHSHM
 */
int vh_shmdt(const void *shmaddr)
{
	return syscall(SYS_sysve, VE_SYSVE_VHSHM_CTL,
						VHSHM_DT, (uint64_t)shmaddr);
}
