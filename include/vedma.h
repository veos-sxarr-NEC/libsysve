/* Copyright (C) 2018 by NEC Corporation
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
 * @file    vedma.h
 * @brief   Header file for VE_DMA API.
 */

#ifndef _LIBVEDMA_H_
#define _LIBVEDMA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>

/**
 * \defgroup vedma VE DMA
 *
 * VE DMA is a DMA data transfer feature for VE programs.
 *
 * Please include "vedma.h" in the source file.
 * Please specify "-lveio -pthread" option to the compiler driver to
 * link libveio.
 *
 * @note This feature is unavailable when system software such as MPI
 *       and ScaTeFS provided by NEC uses the DMA descriptor table. In
 *       this case, ve_dma_init() returns failure.
 * @note A source and a destination of DMA data transfer are specified
 *       by VE host virtual address. To obtain VE host virtual
 *       address, you need to register memory to DMAATB.
 * @note A source and a destination of DMA data transfer need to be
 *       aligned on a 4 byte boundary.
 * @note Data transfer size needs to be less than 128MB.
 */
/*@{*/

/**
 * @struct ve_dma_handle_t
 * @brief This structure holds the state of the asynchronous DMA.
 * @note This structure holds internal data. Please do not access
 *       member variables directly.
*/
typedef struct ve_dma_handle {
	int	status;
	int	index;
} ve_dma_handle_t;

/**
 * @brief This function initializes VE DMA feature
 *
 * @note When the VE program invokes this function multiple times, this
 *       function simply returns 0.
 * @note Before using VE DMA, invoke this function to initialize VE DMA. @n
 * 	When a child process is created by fork(), vfork(), the state
 * 	of VE DMA is not inherited to the child process.
 * 	This function should be invoked from the child process. 
 *
 * @retval 0 On success
 * @retval -1 On failure
 */
int ve_dma_init(void);

/**
 * @brief This function issues asynchronous DMA
 *
 * @note This function writes the DMA transfer request to the DMA descriptor table.
 *
 * @param[in] dst 4 byte aligned VE host virtual address of destination
 * @param[in] src 4 byte aligned VE host virtual address of source
 * @param[in] size Transfer size less than 128MB
 * @param[out] handle Handle used to inquire DMA completion
 *
 * @retval 0 On success
 * @retval -EAGAIN The DMA using the DMA descriptor to be used next is not yet completed @n 
 * 	Need to call vedma_post() again.
 */
int ve_dma_post(uint64_t dst, uint64_t src, int size, ve_dma_handle_t *handle);

/**
 * @brief This function inquiries the completion of asynchronous DMA
 *
 * @note This function inquiries the completion of DMA transfer request issued by vedma_post().
 *
 * @param[in] handle DMA transfer request issued by vedma_post()
 * 
 * @retval 0 DMA completed normally
 * @retval 1-65535 DMA failed @n
 * 	Exception value of DMA descriptor is returned. @n
 * 	DMA failed. The value is bitwise ORed combination of the values described below.
 * -	0x8000: Memory protection exception
 * -	0x4000: Missing page exception
 * -	0x2000: Missing space exception
 * -	0x1000: Memory access exception
 * -	0x0800: I/O access exception
 * @retval -EAGAIN DMA has not completed yet @n
 * 	Need to call vedma_poll() again
 */
int ve_dma_poll(ve_dma_handle_t *handle);

/**
 * @brief This function issues synchronous DMA
 *
 * @note This function writes the DMA transfer request to the DMA descriptor table, and waits for finish.
 *
 * @param[in] dst 4 byte aligned VE host virtual address of destination
 * @param[in] src 4 byte aligned VE host virtual address of source
 * @param[in] size Transfer size less than 128MB
 *
 * @retval 0 On success
 * @retval 1-65535 DMA failed @n
 * 	Exception value of DMA descriptor is returned. @n
 * 	DMA failed. The value is bitwise ORed combination of the values described below. @n
 * -    0x8000: Memory protection exception
 * -    0x4000: Missing page exception
 * -	0x2000: Missing space exception
 * -	0x1000: Memory access exception
 * -	0x0800: I/O access exception
 */
int ve_dma_post_wait(uint64_t dst, uint64_t src, int size);

/**
 * @brief This function gets the value of DMA Control Register
 *
 * @param[out] regs The value of DMA Control Register
 *
 * @retval None
 */
void ve_dma_read_ctrl_reg(uint64_t *regs);

uint64_t ve_register_mem_to_dmaatb(void *vemva, size_t size);
int ve_unregister_mem_from_dmaatb(uint64_t vehva);

#ifdef __cplusplus
}
#endif

#endif /* _LIBVEDMA_H_ */
