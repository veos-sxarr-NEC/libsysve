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
 * @file accelerated_io.c
 * @brief VE side library functions for VE Accelerated IO.
 * @internal
 * @author accelerated io
 */

/**
 * \defgroup veaccio Accelerated I/O
 *
 * "Accelerated I/O" improves I/O performance by efficient data
 * transfer between VE and VH.
 *
 * The throughput and the latency of the below read/write family
 * system calls will be improved.
 *
 * * read
 * * pread
 * * readv
 * * preadv
 * * write
 * * pwrite
 * * writev
 * * pwritev
 * 
 * This feature uses huge pages as buffers, and the same I/O path
 * which is used when data is transferred via InfiniBand. Because of
 * this, this feature can be enabled when the below requirements are
 * met.
 * 
 * * The kernel parameter "vm.nr_hugepages" is more than or equal to
 *   256/per VE (Please see sysctl(8) man page to set the kernel
 *   parameter)
 * * VH isn't connected to the other VH by InfiniBand
 * * VH doesn't use ScaTeFS
 *
 * "Accelerated I/O" is enabled when 'libveaccio' is loaded at runtime.
 * Please set environment variable VE_LD_PRELOAD to load it.
 *
 * ~~~
 * (For bash)
 * $ export VE_LD_PRELOAD=libveaccio.so.1
 * $ ./a.out
 * (For csh)
 * % setenv VE_LD_PRELOAD libveaccio.so.1
 * $ ./a.out
 * ~~~
 *
 * @note Data is transferred every 8MB. So, read/write family system
 *       calls will not be atomic when the size is more than 8MB.
 * @note No API is provided.
 */

#ifdef HAVE_IO_HOOK_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sysve.h>
#include <pthread.h>
#include <errno.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <veshm.h>
#include "vedma.h"
#include <io_hook.h>
#include <veacc_io_defs.h>
#include "libsysve.h"
#include "libsysve_utils.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define GET_DMA_SIZE(a) ((a + 0x3) & 0xfffffffffffffffc)

#define SYSCALL_CANCEL(a,...) \
  do {\
    pthread_testcancel();\
    a = syscall(__VA_ARGS__);\
    pthread_testcancel();\
  } while(0)

#define ACCELERATED_IO  0
#define PDMA_IO  1

#define VE_CORE_MAX 16
#define CORE_NUM_BUFF_SIZE 32

#define ENV_KEY_PDMA "VE_PDMA_IO"
#define ENV_KEY_ATOMIC "VE_ATOMIC_IO"

#define VE_BUFF_SIZE (8*1024*1024)

#define VE_BUFF_USING 1
#define VE_BUFF_NOT_USING 0

#define SUCCESS 0
#define FAIL -1
#define NOBUF 1

typedef struct {
	uint64_t vh_buff_and_flag;	/*!< VH buffer adress and bit flag */
	uint64_t ve_buff;	/*!< VE buffer */
	uint64_t vh_vehva;	/*!< vehva of VH buffer */
	uint64_t ve_vehva;	/*!< vehva of VE buffer */
	int vh_mask;	/*!< mask for get flag status of VH buffer */
} acc_io_info;

typedef struct {
	int ve_buff_using_flag;	/*!< Flag status of VE buffer */
	uint64_t local_vehva;	/*!< VEHVA of IO buffer memory on VE */
	uint64_t ve_io_buff[VE_BUFF_SIZE/sizeof(uint64_t)];	/*!< VE buffer */
} ve_acc_io_ve_resources;

typedef struct {
	int ve_core_num;	/*!< Number of ve cores */
	int vh_buff_using_flag;	/*!< Bit flag of lock status of VH buffer */
	void *vh_buff[VE_CORE_MAX];	/*!< Array of start address of VH buffer */
	uint64_t vehva[VE_CORE_MAX];	/*!< Array of the VEHVA of VH buffer*/
	pthread_spinlock_t lock;	/*!< spin lock for vh resources*/
} ve_acc_io_vh_resources;

static int constructor_result = ACCELERATED_IO;

/**
 * @brief This global variable hold the information of VE/VH resources of
 * Accelerated IO
 */
static __thread ve_acc_io_ve_resources ve_resources = {0, 0, {0}};
static ve_acc_io_vh_resources vh_resources = {0, 0, {NULL}, {0}, 0};

/**
 * @brief This function check if environment that can use accelerated IO.
 *
 * @retval ACCELERATED_IO accelerated IO is enable
 * @retval  PDMA_IO accelerated IO is disable
 * @retval  -EIO On Failure
 */
static int ve_accelerated_io_chk_env_init_dma(void)
{
	int ret = -EIO;
	int dma_init_ret;
	char *env_val_pdma;
	char *env_val_atomic;
	char *ptr_pdma;
	char *ptr_atomic;

	env_val_pdma = getenv(ENV_KEY_PDMA);
	env_val_atomic = getenv(ENV_KEY_ATOMIC);

	if ((env_val_pdma != NULL)
		&& ((int)strtol(env_val_pdma, &ptr_pdma, 2) != ACCELERATED_IO
		|| strlen(ptr_pdma) != 0)) {
		return PDMA_IO;
	}
	if ((env_val_atomic != NULL)
		&& ((int)strtol(env_val_atomic, &ptr_atomic, 2) != ACCELERATED_IO
		|| strlen(ptr_atomic) != 0)) {
		return PDMA_IO;
	}

	dma_init_ret = ve_dma_init();
	if (SUCCESS == dma_init_ret) {
		ret = ACCELERATED_IO;
	}
	return ret;
}

/**
 * @brief This function get following information of accelerated IO resources
 *   from Libsysve global variable vh_resources.
 *   VH IO buffer
 *   VE IO buffer
 *   VEHVA of VH IO buffer
 *   VEHVA of VE IO buffer
 *
 * @retval 0 on success, -1 on failure.
 */
static int ve_accelerated_io_getinfo(void)
{
	void *vhva;
	uint64_t vehva = 0;
	void *local_vemva;
	uint64_t local_vehva = 0;
	void *vhva_register = NULL;
	void *local_vemva_register = NULL;
	int ret;
	int i;

	if (NULL == vh_resources.vh_buff[0]) {
		/* Invoke Pseudo function sys_accelerated_io_init() */
		ret = (int)syscall(SYS_sysve,
				VE_SYSVE_ACCELERATED_IO_INIT,
				&vhva, &vehva,
				&ve_resources.ve_io_buff[0], &local_vehva,
				vh_resources.ve_core_num);
		if (SUCCESS != ret) {
			return FAIL;
		}

		/* Put information into global variable */
		vh_resources.vh_buff[0] = vhva;
		vh_resources.vehva[0] = vehva;
		ve_resources.local_vehva = local_vehva;

		for (i = 0; i < vh_resources.ve_core_num; i++) {
			vh_resources.vh_buff[i]	= vhva + VE_BUFF_SIZE * i;
			vh_resources.vehva[i] = vehva + VE_BUFF_SIZE * i;
	        }
		return SUCCESS;
	}

	if (0 == vh_resources.vehva[0]){
		vhva_register = vh_resources.vh_buff[0];
	}
	if (0 == ve_resources.local_vehva) {
		local_vemva_register = &ve_resources.ve_io_buff[0];
	}
	if ((NULL != vhva_register)
		|| (NULL != local_vemva_register)) {
		/* Invoke sys_accelerated_io_register_dmaatb() */
		ret = (int)syscall(SYS_sysve,
				VE_SYSVE_ACCELERATED_IO_REGISTER_DMAATB,
				vhva_register, &vehva,
				local_vemva_register, &local_vehva,
				vh_resources.ve_core_num);
		if (SUCCESS != ret) {
			return NOBUF;
		}

		if (0 != local_vehva) {
			ve_resources.local_vehva = local_vehva;
		}

		if (0 != vehva) {
			vh_resources.vehva[0] = vehva;
			for (i = 0; i < vh_resources.ve_core_num; i++) {
				vh_resources.vehva[i] = vehva + VE_BUFF_SIZE * i;
			}
		}
	}
	return SUCCESS;
}

/**
 * @brief This function handle pre-processing of IO request.
 *
 * @param [out] struct acc_io_info
 *
 * @retval 0 on success, -1 on failure.
 */
static int ve_accelerated_io_pre(acc_io_info *io_info)
{
	int i;
	int ret = SUCCESS;
	int retval = SUCCESS;
	int vh_buff_num = 0;
	int errno_bak = errno;
	void *vh_buff;
	ssize_t transfer_size = VE_BUFF_SIZE;
	ssize_t read_out_size = 0;
	char core_num_buff[CORE_NUM_BUFF_SIZE] = {'\0'};
	ve_dma_handle_t vedma_handle;
	io_info->vh_mask = 1;

	pthread_testcancel();

	if (PDMA_IO == constructor_result) {
		return FAIL;
	}

	/* Checks VE IO buffer is using or not by flag ve_buff_using_flag */
	ret = __libsysve_a_swap(&ve_resources.ve_buff_using_flag,
				VE_BUFF_USING);
	if(ret != VE_BUFF_NOT_USING){
		errno = errno_bak;
		return NOBUF;
	}

	if (pthread_spin_lock(&vh_resources.lock) != 0) {
		errno = errno_bak;
		return FAIL;
	}
	if (0 == vh_resources.ve_core_num) {
		if (FAIL == (int)ve_get_ve_info("num_of_core",
					core_num_buff, CORE_NUM_BUFF_SIZE)) {
			retval = FAIL;
			goto error_unlock_vh;
		}
		vh_resources.ve_core_num = atoi(core_num_buff);
	}
	/* When first IO request, check environment */
	if ((0 == vh_resources.vehva[0])
		&& (ACCELERATED_IO != ve_accelerated_io_chk_env_init_dma())) {
		retval = FAIL;
		goto error_unlock_vh;
	}
	ret = ve_accelerated_io_getinfo();
	if (SUCCESS != ret) {
		retval = ret;
		goto error_unlock_vh;
	}
	/* Find the free VH buffer */
	for (i = 0; i < vh_resources.ve_core_num; i++) {
		io_info->vh_mask = 1 << i;
		if (0 == (vh_resources.vh_buff_using_flag & io_info->vh_mask)) {
			vh_resources.vh_buff_using_flag
				= vh_resources.vh_buff_using_flag
				| io_info->vh_mask;
			vh_buff_num = i;
			break;
		}
		if (i == vh_resources.ve_core_num - 1) {
			retval = NOBUF;
			goto error_unlock_vh;
		}
	}
	vh_buff = vh_resources.vh_buff[vh_buff_num];
	io_info->ve_buff = (uint64_t)&ve_resources.ve_io_buff[0];
	io_info->vh_vehva = vh_resources.vehva[vh_buff_num];
	io_info->ve_vehva = ve_resources.local_vehva;

	if (pthread_spin_unlock(&vh_resources.lock) != 0) {
		abort();
	}
	/* Set accelerated io bit flag to VHVA */
	io_info->vh_buff_and_flag
		= ((uint64_t)vh_buff) | VE_ACCELERATED_IO_FLAG;
	return SUCCESS;

error_unlock_vh:
	if (pthread_spin_unlock(&vh_resources.lock) != 0) {
		abort();
	}

error_unlock_ve:
	/* Free flag ve_buff_using_flag */
	__libsysve_a_swap(&ve_resources.ve_buff_using_flag,
				VE_BUFF_NOT_USING);
	errno = errno_bak;
	return retval;
}

/**
 * @brief This function handle post-processing of IO request.
 *
 * @param [in] mask for get flag status of VH buffer
 */
static void ve_accelerated_io_post(int mask)
{
	int ret;

	/* Free flag ve_buff_using_flag */
	__libsysve_a_swap(&ve_resources.ve_buff_using_flag,
				VE_BUFF_NOT_USING);

	if (pthread_spin_lock(&vh_resources.lock) != 0) {
		abort();
	}
	vh_resources.vh_buff_using_flag
		= vh_resources.vh_buff_using_flag & (~mask);
	if (pthread_spin_unlock(&vh_resources.lock) != 0) {
		abort();
	}
	pthread_testcancel();
}

/**
 * @brief This function free the io hook
 */
static void ve_accelerated_io_free_io_hook(void)
{
	__read_hook = NULL;
	__pread_hook = NULL;
	__readv_hook = NULL;
	__preadv_hook = NULL;
	__write_hook = NULL;
	__pwrite_hook = NULL;
	__writev_hook = NULL;
	__pwritev_hook = NULL;
}

/**
 * @brief This function starts accelerated read or pread.
 *
 * @param[in] The number of system call
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of bytes read
 * @param[in] File offset, it is 0 when read
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_read_pread(int syscall_num, int fd, void *buf,
					size_t count, off_t ofs)
{
	int i;
	int ret = 0;
	int errno_bak = 0;
	uint64_t read_num = 0;
	acc_io_info io_info;
	ssize_t transfer_size = VE_BUFF_SIZE;
	ssize_t read_out_size = 0;
	ssize_t exit_result = 0;
	ve_dma_handle_t vedma_handle;

	if (0 == count) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	}
	if (NULL == buf) {
		exit_result = FAIL;
		errno = EFAULT;
		return exit_result;
	}
	/* Pre processing of IO request */
	ret = ve_accelerated_io_pre(&io_info);
	
	if (FAIL == ret) {
		ve_accelerated_io_free_io_hook();
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	} else if (NOBUF == ret) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	}

	read_num = (count + transfer_size - 1)/ transfer_size;
	for (i = 0; i < read_num; i++) {
		if (i == 1) {
			io_info.vh_buff_and_flag
				= io_info.vh_buff_and_flag
					| VE_SECOND_SYS_CALL_FLAG;
		}
		if (i == read_num - 1) {
			transfer_size = count - transfer_size * i;
		}

		/* Call read syscall */
		read_out_size = syscall(syscall_num, fd,
				io_info.vh_buff_and_flag, transfer_size, ofs);
		if (FAIL == read_out_size) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = read_out_size;
			}
			errno = 0;
			break;
		} else if (0 == read_out_size) {
			break;
		}
		exit_result = exit_result + read_out_size;
		/* Transfer data from VH buffer to VE buffer by VE DMA */
		ret = ve_dma_post_wait(io_info.ve_vehva, io_info.vh_vehva,
					GET_DMA_SIZE(transfer_size));
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			goto hndl_return;
		}
		/* Copy data from VE buffer to user buffer */
		__libsysve_vec_memcpy(buf, (void *)io_info.ve_buff,
					transfer_size);

		buf = buf + transfer_size;
		ofs = ofs + transfer_size;
		if (transfer_size > read_out_size) {
			break;
		}
	}
hndl_return:
	ve_accelerated_io_post(io_info.vh_mask);
	if (0 != errno_bak) {
		errno = errno_bak;
	}
	return exit_result;
}

/**
 * @brief This function starts accelerated read.
 *
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of bytes read
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_read(int fd, void *buf, size_t count)
{
	return ve_accelerated_io_read_pread(SYS_read, fd, buf, count, 0);
}

/**
 * @brief This function starts accelerated pread.
 *
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of bytes read
 * @param[in] File offset
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_pread(int fd, void *buf, size_t count,
					off_t ofs)
{
        return ve_accelerated_io_read_pread(SYS_pread64, fd, buf, count, ofs);
}

/**
 * @brief This function starts accelerated readv or preadv.
 *
 * @param[in] The number of system call
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of read buffer
 * @param[in] File offset, it is 0 when readv
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_readv_preadv(int syscall_num, int fd,
				const struct iovec *iov, int count, off_t ofs)
{
	int i, n;
	int ret = 0;
	int errno_bak = 0;
	uint64_t read_num = 0;
	acc_io_info io_info;
	ssize_t total_size = 0;
	ssize_t transfer_size = VE_BUFF_SIZE;
	ssize_t read_out_size = 0;
	ssize_t exit_result = 0;
	int read_syscall_type = SYS_read;

	int next_iov_num = 0;
	ssize_t iov_len_done = 0;
	ssize_t iov_len_less = 0;
	ssize_t ve_buff_size_less = 0;
	ssize_t iov_copy_size = 0;
	uint64_t source_buff;
	ve_dma_handle_t vedma_handle;

	if (0 == count) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	}
	if (NULL == iov) {
		exit_result = FAIL;
		errno = EFAULT;
		return exit_result;
	}
	/* Pre processing of IO request */
	ret = ve_accelerated_io_pre(&io_info);

	if (FAIL == ret) {
		ve_accelerated_io_free_io_hook();
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	} else if (NOBUF == ret) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	}

	if (SYS_preadv == syscall_num) {
		read_syscall_type = SYS_pread64;
	}
	
	/* get total size */
	for (i = 0; i < count; i++) {
		total_size = total_size + iov[i].iov_len; 
	}

	read_num = (total_size + transfer_size - 1)/ transfer_size;
	for (i = 0; i < read_num; i++) {
		if (i == 1) {
			io_info.vh_buff_and_flag
				= io_info.vh_buff_and_flag
					| VE_SECOND_SYS_CALL_FLAG;
		}
		if (i == read_num - 1) {
			transfer_size = total_size - transfer_size * i;
		}

		/* Call read syscall */
		read_out_size = syscall(read_syscall_type, fd,
				io_info.vh_buff_and_flag, transfer_size, ofs);
		if (FAIL == read_out_size) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = read_out_size;
			}
			errno = 0;
			break;
		} else if (0 == read_out_size) {
			break;
		}
		exit_result = exit_result + read_out_size;
		/* Transfer data from VH buffer to VE buffer by VE DMA */
		ret = ve_dma_post_wait(io_info.ve_vehva, io_info.vh_vehva,
					GET_DMA_SIZE(transfer_size));
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			goto hndl_return;
		}
		/* Copy data from VE buffer to user buffer */
		ve_buff_size_less = read_out_size;
		source_buff = io_info.ve_buff;
		for (n = next_iov_num; n < count; n++) {
			iov_len_less = iov[n].iov_len - iov_len_done;
			iov_copy_size = MIN (iov_len_less, ve_buff_size_less);
			__libsysve_vec_memcpy((void *)iov[n].iov_base + iov_len_done,
                                        (void *)source_buff, iov_copy_size);
			if (0 >= (ve_buff_size_less - iov_copy_size)) {
				/* We have copied All data from the VE buffer to
				 * the user buffers.
				*/
				if (iov_len_less > ve_buff_size_less) {
					iov_len_done = iov_len_done
							+ ve_buff_size_less;
					next_iov_num = n;
				} else {
					iov_len_done = 0;
					next_iov_num = n + 1;
				}
				break;
			}
			iov_len_done = 0;
			ve_buff_size_less = ve_buff_size_less - iov_copy_size;
			source_buff = source_buff + iov_copy_size;
		}

		ofs = ofs + transfer_size;
		if (transfer_size > read_out_size) {
			break;
		}
	}
hndl_return:
	ve_accelerated_io_post(io_info.vh_mask);
	if (0 != errno_bak) {
		errno = errno_bak;
	}
	return exit_result;
}

/**
 * @brief This function starts accelerated readv.
 *
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of read buffer
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_readv(int fd, const struct iovec *iov,
					int count)
{
        return ve_accelerated_io_readv_preadv(SYS_readv, fd, iov, count, 0);
}

/**
 * @brief This function starts accelerated preadv.
 *
 * @param[in] File descriptor which refer to a file this function reads from
 * @param[in] Buffer into which this function stores the read data
 * @param[in] Number of read buffer
 * @param[in] File offset
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_preadv(int fd, const struct iovec *iov,
					int count, off_t ofs)
{
	return ve_accelerated_io_readv_preadv(SYS_preadv, fd, iov, count, ofs);
}

/**
 * @brief This function starts accelerated write.
 *
 * @param[in] The number of system call
 * @param[in] File descriptor which refer to a file this function writes to
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of bytes written
 * @param[in] File offset, it is 0 when read
 *
 * @return Total number of write bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_write_pwrite(int syscall_num, int fd,
						const void *buf, size_t count,
						off_t ofs)
{
	int i;
	int ret = 0;
	int errno_bak = 0;
	uint64_t write_num = 0;
	acc_io_info io_info;
	ssize_t transfer_size = VE_BUFF_SIZE;
	ssize_t write_in_size = 0;
	ssize_t exit_result = 0;
	ve_dma_handle_t vedma_handle;

	if (0 == count) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	}
	if (NULL == buf) {
		exit_result = FAIL;
		errno = EFAULT;
		return exit_result;
	}
	/* Pre processing of IO request */
	ret = ve_accelerated_io_pre(&io_info);
	
	if (FAIL == ret) {
		ve_accelerated_io_free_io_hook();
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	} else if (NOBUF == ret) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, buf, count, ofs);
		return exit_result;
	}

	write_num = (count + transfer_size - 1)/ transfer_size;
	for (i = 0; i < write_num; i++) {
		if (i == 1) {
			io_info.vh_buff_and_flag
				= io_info.vh_buff_and_flag
					| VE_SECOND_SYS_CALL_FLAG;
		}
		if (i == write_num - 1) {
			transfer_size = count - transfer_size * i;
		}

		/* Copy data from user buffer to VE buffer */
		__libsysve_vec_memcpy((void *)io_info.ve_buff, (void *)buf,
					transfer_size);

		/* Transfer data from VE buffer to VH buffer by VE DMA */
		ret = ve_dma_post_wait(io_info.vh_vehva, io_info.ve_vehva,
					GET_DMA_SIZE(transfer_size));
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			goto hndl_return;
		}

		/* Call write syscall */
		write_in_size = syscall(syscall_num, fd,
					io_info.vh_buff_and_flag,
					transfer_size, ofs);
		if (FAIL == write_in_size) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = write_in_size;
			}
			errno = 0;
			break;
		} else if (0 == write_in_size) {
			break;
		}
		exit_result = exit_result + write_in_size;
		buf = buf + transfer_size;
		ofs = ofs + transfer_size;
		if (transfer_size > write_in_size) {
			break;
		}
	}
hndl_return:
	ve_accelerated_io_post(io_info.vh_mask);
	if (0 != errno_bak) {
		errno = errno_bak;
	}
	return exit_result;
}

/**
 * @brief This function starts accelerated write.
 *
 * @param[in] File descriptor which refer to a file this function writes to
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of bytes written
 *
 * @return Total number of write bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_write(int fd, const void *buf, size_t count)
{
	return ve_accelerated_io_write_pwrite(SYS_write, fd, buf, count, 0);
}

/**
 * @brief This function starts accelerated write.
 *
 * @param[in] File descriptor which refer to a file this function writes to
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of bytes written
 * @param[in] File offset
 *
 * @return Total number of write bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_pwrite(int fd, const void *buf, size_t count,
					off_t ofs)
{
	return ve_accelerated_io_write_pwrite(SYS_pwrite64, fd, buf, count, ofs);
}

/**
 * @brief This function starts accelerated writev or pwritev.
 *
 * @param[in] The number of system call
 * @param[in] File descriptor which refer to a file this function writes to
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of write iuffer
 * @param[in] File offset, it is 0 when writev
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_writev_pwritev(int syscall_num, int fd,
				const struct iovec *iov, int count, off_t ofs)
{
	int i, n;
	int ret = 0;
	int errno_bak = 0;
	uint64_t write_num = 0;
	acc_io_info io_info;
	ssize_t total_size = 0;
	ssize_t transfer_size = VE_BUFF_SIZE;
	ssize_t write_in_size = 0;
	ssize_t exit_result = 0;
	int write_syscall_type = SYS_write;

	int next_iov_num = 0;
	ssize_t iov_len_done = 0;
	ssize_t iov_len_less = 0;
	ssize_t ve_buff_size_less = 0;
	ssize_t iov_copy_size = 0;
	uint64_t target_buff;
	ve_dma_handle_t vedma_handle;

	if (0 == count) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	}
	if (NULL == iov) {
		exit_result = FAIL;
		errno = EFAULT;
		return exit_result;
	}
	/* Pre processing of IO request */
	ret = ve_accelerated_io_pre(&io_info);

	if (FAIL == ret) {
		ve_accelerated_io_free_io_hook();
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	} else if (NOBUF == ret) {
		SYSCALL_CANCEL(exit_result, syscall_num, fd, iov, count, ofs);
		return exit_result;
	}

	if (SYS_pwritev == syscall_num) {
		write_syscall_type = SYS_pwrite64;
	}
	
	/* get total size */
	for (i = 0; i < count; i++) {
		total_size = total_size + iov[i].iov_len; 
	}

	write_num = (total_size + transfer_size - 1)/ transfer_size;
	for (i = 0; i < write_num; i++) {
		if (i == 1) {
			io_info.vh_buff_and_flag
				= io_info.vh_buff_and_flag
					| VE_SECOND_SYS_CALL_FLAG;
		}
		if (i == write_num - 1) {
			transfer_size = total_size - transfer_size * i;
		}

		/* Copy data from VE buffer to user buffer */
		ve_buff_size_less = transfer_size;
		target_buff = io_info.ve_buff;
		for (n = next_iov_num; n < count; n++) {
			iov_len_less = iov[n].iov_len - iov_len_done;
			iov_copy_size = MIN (iov_len_less, ve_buff_size_less);
			__libsysve_vec_memcpy((void *)target_buff,
					(void *)iov[n].iov_base + iov_len_done,
					iov_copy_size);
			if (0 >= (ve_buff_size_less - iov_copy_size)) {
				/* We have copied All data from the user buffers
				 * to the VE buffer.
				*/
				if (iov_len_less > ve_buff_size_less) {
					iov_len_done = iov_len_done
							+ ve_buff_size_less;
					next_iov_num = n;
				} else {
					iov_len_done = 0;
					next_iov_num = n + 1;
				}
				break;
			}
			iov_len_done = 0;
			ve_buff_size_less = ve_buff_size_less - iov_copy_size;
			target_buff = target_buff + iov_copy_size;
		}

		/* Transfer data from VE buffer to VH buffer by VE DMA */
		ret = ve_dma_post_wait(io_info.vh_vehva, io_info.ve_vehva,
					GET_DMA_SIZE(transfer_size));
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			goto hndl_return;
		}

		/* Call write syscall */
		write_in_size = syscall(write_syscall_type, fd,
				io_info.vh_buff_and_flag, transfer_size, ofs);
		if (FAIL == write_in_size) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = write_in_size;
			}
			errno = 0;
			break;
		} else if (0 == write_in_size) {
			break;
		}
		exit_result = exit_result + write_in_size;
		ofs = ofs + transfer_size;
		if (transfer_size > write_in_size) {
			break;
		}
	}
hndl_return:
	ve_accelerated_io_post(io_info.vh_mask);
	if (0 != errno_bak) {
		errno = errno_bak;
	}
	return exit_result;
}

/**
 * @brief This function starts accelerated writev.
 *
 * @param[in] File descriptor which refer to a file this function s from
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of write buffer
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_writev(int fd, const struct iovec *iov,
					int count)
{
        return ve_accelerated_io_writev_pwritev(SYS_writev, fd, iov, count, 0);
}

/**
 * @brief This function starts accelerated pwritev.
 *
 * @param[in] File descriptor which refer to a file this function s from
 * @param[in] Buffer from which this function gets the data to write
 * @param[in] Number of write buffer
 * @param[in] File offset
 *
 * @return Total number of read bytes on Success, -1 on Failure.
 */
static ssize_t ve_accelerated_io_pwritev(int fd, const struct iovec *iov,
					int count, off_t ofs)
{
        return ve_accelerated_io_writev_pwritev(SYS_pwritev, fd, iov, count, ofs);
}

/**
 * @brief This function clear the result of DMAATB register and vh buffer using
 * flag.
 */
static void ve_accelerated_io_clear_acc_io_info(void)
{
	vh_resources.vh_buff[0] = NULL;
	vh_resources.vehva[0] = 0;
	vh_resources.vh_buff_using_flag = 0;
	ve_resources.local_vehva = 0;
}

/**
 * @brief This function load call back function, initialize spin lock,
 * and load function ve_accelerated_io_clear_acc_io_info().
 */
__attribute__((constructor)) void ve_accelerated_io_init(void)
{
	__read_hook = (__read_hook) ? __read_hook : ve_accelerated_io_read;
	__pread_hook = (__pread_hook) ? __pread_hook : ve_accelerated_io_pread;
	__readv_hook = (__readv_hook) ? __readv_hook : ve_accelerated_io_readv;
	__preadv_hook = (__preadv_hook) ? __preadv_hook : ve_accelerated_io_preadv;
	__write_hook = (__write_hook) ? __write_hook : ve_accelerated_io_write;
	__pwrite_hook = (__pwrite_hook) ? __pwrite_hook : ve_accelerated_io_pwrite;
	__writev_hook = (__writev_hook) ? __writev_hook : ve_accelerated_io_writev;
	__pwritev_hook = (__pwritev_hook) ? __pwritev_hook : ve_accelerated_io_pwritev;

	/* initialize spin lock */
	if (pthread_spin_init(&vh_resources.lock, 0) != 0) {
		constructor_result = PDMA_IO;
	}
	/* clear vehva information when fork or vfork */
	if (pthread_atfork (NULL, NULL, ve_accelerated_io_clear_acc_io_info)) {
		constructor_result = PDMA_IO;
	}
}

#endif
