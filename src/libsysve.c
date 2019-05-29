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
 * \copyright 2017-2019. Licensed under the terms of the MIT license.
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
