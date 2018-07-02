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
 * @file libuserdma.c
 * @brief DMA functions for VE library
 */
#include <stdint.h>
#include <unistd.h>
#include <sysve.h>
#include <veos_defs.h>


/**
 * @brief This function maps DMA descriptor H to VEHVA
 *
 * @param[out] vehva_dmades VEHVA which DMA descriptor table H is mapped to
 * @param[out] vehva_dmactl VEHVA which DMA control register H is mapped to
 *
 * @return 0 on success, -1 and set errno on failure.
 */
int ve_map_dmades(uint64_t *vehva_dmades, uint64_t *vehva_dmactl)
{
	return syscall(SYS_sysve, VE_SYSVE_MAP_DMADES, 
			vehva_dmades, vehva_dmactl);
}

/**
 * @brief This function halts user mode DMA correspoinding to descriptor H
 *        and unmaps it from VEHVA.
 *
 * @param[in] vehva_dmades VEHVA which DMA descriptor table H is mapped to
 *
 * @return 0 on success, -1 and set errno on failure.
 */
int ve_unmap_dmades(uint64_t vehva_dmades)
{
	return syscall(SYS_sysve, VE_SYSVE_UNMAP_DMADES, 
			vehva_dmades);
}

