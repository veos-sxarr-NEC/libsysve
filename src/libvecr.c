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
 * @file  libvecr.c
 * @brief Library of CR API
 */
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <veos_defs.h>
#include <sysve.h>
#include "vecr.h"

/**
 * @brief This function allocates a local CR page to a caller process.
 *
 * @param [in] mode_flag Mode flag to specify the purpose of CR.
 *
 * @return CR directory number on success, -1 and set errno on failure
 */
int64_t ve_cr_alloc_crset(uint64_t mode_flag)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_CR_CTL,
					(uint64_t)CR_ALLOC, mode_flag);
}

/**
 * @brief This function attaches the local CR page specified by
 *        arguments to a CR directory entry of a caller process.
 *
 * @param [in] pid Pid of the existing user process.
 *
 * @param [in] crd_number CR directory number of the CR page.
 *
 * @return CR directory number on success, -1 and set errno on failure
 */
int64_t ve_cr_attach_dir(pid_t pid, uint64_t crd_number)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_CR_CTL,
		(uint64_t)CR_ATTACH_LOCAL, (uint64_t)pid, crd_number);
}

/**
 * @brief This function attaches the remote CR page specified by
 *        arguments to VEHVA of a caller process.
 *
 * @param [in] pid Pid of the existing user process.
 *
 * @param [in] crd_number CR directory number of the CR page.
 *
 * @return VEHVA on success, -1 and set errno on failure
 */
int64_t ve_cr_attach_dma(pid_t pid, uint64_t crd_number)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_CR_CTL,
		(uint64_t)CR_ATTACH_REMOTE, (uint64_t)pid, crd_number);
}

/**
 * @brief This function releases the local CR page specified by CR
 *        directory number
 *
 * @param [in] crd_number CR directory number of the CR page.
 *
 * @return 0 on success, -1 and set errno on failure
 */
int ve_cr_release_dir(uint64_t crd_number)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_CR_CTL,
				(uint64_t)CR_RELEASE_LOCAL, crd_number);
}

/**
 * @brief This function detaches the remote CR page specified by the
 *        argument.
 *
 * @param [in] vehva Start address of VEHVA mapped a target CR page.
 *
 * @return 0 on success, -1 and set errno on failure
 */
int ve_cr_release_dma(uint64_t vehva)
{
	return syscall((uint64_t)SYS_sysve, (uint64_t)VE_SYSVE_CR_CTL,
				(uint64_t)CR_RELEASE_REMOTE, vehva);
}
