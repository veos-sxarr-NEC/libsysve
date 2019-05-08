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
 * @file libvepci.c
 * @brief Library of  PCI operation on VE
 */

#include <veos_defs.h>
#include <sysve.h>
#include <vepci.h>
/**
 * \defgroup vepci VE PCI(EXPERIMENTAL)
 *
 * VE PCI is a experimental feature to make PCI device access to VE memory
 * or be accessed from VE core. VE memory is registered to PCI ATB and
 * PCI device can access the VE memory by accessing PCI ATB entry. On the other
 * hand, PCI device address is registered to DMA ATB and VE core can access
 * the PCI device by accessing VEHVA which is entry of DMA ATB. In actual
 * VE program, this is realized by calling ve_pci_load64() or ve_pci_store64()
 * with VEHVA.
 *
 * Please include "vepci.h".
 *
 * @note - Registration PCI device address to DMA ATB requires root privilege
 *       because it enables VE program to access any address in PCI space of VH
 * @note - VE PCI provides API to register/unregister each address to/from
 *       PCI ATB or DMA ATB. VE virtual memory address to be registered PCI ATB
 *       must be aligned with PCI ATB page size. PCI ATB page size is 64MB
 *       except for SX-Aurora TSUBASA A100. For A100 model, PCI ATB page size
 *       is 2MB. On the othre hand, PCI device address to be registered DMA
 *       ATB must be aligned with 2 or 64 MB size.
 * @note - If VE program terminates without calling
 * 	 ve_unregister_mem_from_pci(), PCI ATB entry allocated by
 * 	 ve_register_mem_to_pci() is never freed
 *       during VEOS running. Restarting VEOS is needed to free the entry. On
 *       the other hand, VEHVA is automatically freed when VE program
 *       terminates.
 * @note - For synchronization between VE core and PCI device,
 *       ve_pci_recv_sync() is needed. When PCI device writes data to VE memory
 *       and VE
 *       program reads it, PCI device must write any flag in VE memory after
 *       writing
 *       data and VE program must read the flag at first. After that, call
 *       ve_pci_recv_sync() and read data.
 */
/*@{*/

/**
 * @brief This function registers VE memory to PCI ATB.
 *
 * @param[in] mem VE memory virtual address to be registered. This is must be
 *            aligned with PCI ATB page size.
 * @param[in] size VE memory size to be registered. This is must be aligned with
 *            PCI ATB page size.
 *
 * @retval "Address of PCI ATB entry" on success
 * @retval -1 on failure and set errno
 */
uint64_t
ve_register_mem_to_pci(void *mem, size_t size)
{
	return (uint64_t)syscall((uint64_t)SYS_sysve,
			(uint64_t)VE_SYSVE_REGISTER_VEMVA_TO_PCIATB,
			(uint64_t)mem, (uint64_t)size);
}

/**
 * @brief This function unregisters PCI ATB entry.
 *
 *  @param[in] pci_addr Address of PCI ATB entry to be unregistered. This is
 *             must be aligned with PCI ATB page size.
 *  @param[in] size Memory size registered at its entry. This is  must be
 *             aligned with PCI ATB page size.
 *
 *  @retval  0 on success
 *  @retval -1 on failure and set errno
 */
int
ve_unregister_mem_from_pci(uint64_t pci_addr, size_t size)
{
        return (int)syscall((uint64_t)SYS_sysve,
			(uint64_t)VE_SYSVE_UNREGISTER_VHSAA_FROM_PCIATB,
                        pci_addr, (uint64_t)size);
}

/**
 * @brief This function registers PCI ATB address to DMAATB.
 *
 * @note This function requires root privilege
 *
 * @param[in] pci_addr PCI address to be registered to DMAATB. This is must be
 *            aligned with 2 or 64 MB size.
 * @param[in] size Memory size to be registered to DMAATB. This is must be
 *            aligned with 2 or 64 MB size.
 *
 * @retval "Address of VEHVA" on success
 * @retval -1 on failure and set errno
 */
uint64_t
ve_register_pci_to_vehva(uint64_t pci_addr, size_t size)
{
        return (uint64_t)syscall((uint64_t)SYS_sysve,
			(uint64_t)VE_SYSVE_REGISTER_VHSAA_TO_DMAATB,
                        pci_addr, (uint64_t)size);
}

/**
 * @brief This function unregisters VEHVA from DMAATB
 *
 * @param[in] vehva VEHVA address to be unregistered from DMAATB. This is must
 *            be aligned with 2 or 64 MB size.
 * @param[in] size Memory size of VEHVA registered at DMAATB. This is must be
 *            aligned with 2 or 64 MB size.
 *
 * @retval 0 on success
 * @retval -1 on failure and set errno
 */
int
ve_unregister_pci_from_vehva(uint64_t vehva, size_t size)
{
        return (int)syscall((uint64_t)SYS_sysve,
			(uint64_t)VE_SYSVE_UNREGISTER_VEHVA_FROM_DMAATB,
                        vehva, (uint64_t)size);
}
