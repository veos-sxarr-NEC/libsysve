/**
 * Copyright (C) 2018 by NEC Corporation
 * This file is part of the VEOS.
 *
 * The VEOS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 * The VEOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the VEOS; if not, see * <http://www.gnu.org/licenses/>.
 */
/**
 * @file vepci.h
 * @brief Header file for pci operation on VE
 */

#ifndef __VE_VEPCI_H
#define __VE_VEPCI_H

#include <stdint.h>

/** \defgroup vepci VE PCI(EXPERIMENTAL)
 */
/*@{*/
uint64_t ve_register_mem_to_pci(void *mem, size_t size);
int ve_unregister_mem_from_pci(uint64_t pci_addr, size_t size);
uint64_t ve_register_pci_to_vehva(uint64_t pci_addr, size_t size);
int ve_unregister_pci_from_vehva(uint64_t vehva, size_t size);

/**
 * @brief This function loads 64 bit data from address mapped to VEHVA
 *
 * @param[in] vehva Address of data
 *
 * @return "64bit size data"
 */
inline uint64_t ve_pci_load64(uint64_t vehva) __attribute__((always_inline));
inline uint64_t ve_pci_load64(uint64_t vehva)
{
	uint64_t ret;
	asm volatile(
		"       lhm.l   %0, 0(%1)\n"
		"	fencem 2\n"
			: "=r"(ret)
			: "r"(vehva));
	return ret;
}

/**
 * @brief This function synchronizes VE core and PCI device when PCI device
 *        writes some data to VE memory and VE core reads the data. For usage,
 *        please refer to Note.
 */
inline void ve_pci_recv_sync(void) __attribute__((always_inline));
inline void ve_pci_recv_sync(void)
{
	ve_pci_load64(0x1400); /* PSIRCVSYC */
}

/**
 * @brief This function stores 64 bit data to address mapped to VEHVA
 *
 * @param[in] vehva Address of data
 * @param[in] value 64 bit size data
 */
inline void ve_pci_store64(uint64_t vehva, uint64_t value) __attribute__((always_inline));
inline void ve_pci_store64(uint64_t vehva, uint64_t value)
{
	asm volatile(
		"	fencem 1\n"
		"       shm.l   %0, 0(%1)\n"
			:: "r"(value), "r"(vehva));
}

#endif
