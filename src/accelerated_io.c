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
 * "Accelerated I/O" is a feature which improves I/O performance by
 * efficient data transfer between VE and VH.
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
 * Please set environment variable VE_ACC_IO=1 to enable accelerated
 * I/O.
 *
 * ~~~
 * $ export VE_ACC_IO=1
 * $ ./a.out
 * ~~~
 * @note A VE process uses 32 huge pages (64MB huge pages memory),
 *       when Accelerated I/O is enabled.
 * @note Data is transferred every 4MB when accelerated I/O is
 *       enabled. So, read/write family system calls will not be
 *       atomic when the size is more than 4MB.
 *
 * Users can set the environment variable VE_ACC_IO_VERBOSE=1 to display
 * whether accelerated IO is enabled or disabled to standard error when
 * exit VE process.
 * ~~~
 * $ export VE_ACC_IO=1
 * $ export VE_ACC_IO_VERBOSE=1
 * $ ./a.out
 *   Accelerated IO is enabled
 *
 * $ export -n VE_ACC_IO
 * $ export VE_ACC_IO_VERBOSE=1
 * $ ./a.out
 *   Accelerated IO is disabled
 * ~~~
 */
/*@{*/

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
#define ENV_KEY_VE_ACC_IO "VE_ACC_IO"

#define VE_BUFF_SIZE (8*1024*1024)

#define VE_BUFF_USING 1
#define VE_BUFF_NOT_USING 0

#define BUFF_NPARAS 2
#define PARAS_SIZE VE_BUFF_SIZE/BUFF_NPARAS

#define SUCCESS 0
#define FAIL -1
#define NOBUF 1

#define DEFAULT_CORE_NUM 8
#define SIZE_BY_CORE 2

typedef struct {
	uint64_t vh_buff_and_flag[BUFF_NPARAS];	/*!< VH buffer adress and bit flag */
	uint64_t ve_buff[BUFF_NPARAS];	/*!< VE buffer */
	uint64_t vh_vehva[BUFF_NPARAS];	/*!< vehva of VH buffer */
	uint64_t ve_vehva[BUFF_NPARAS];	/*!< vehva of VE buffer */
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
			vh_resources.vh_buff[i]	= (void *)((uint64_t)vhva
							+ VE_BUFF_SIZE * i);
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
	char *ptr_env;
	char *evn_ve_acc_io;
	int ve_acc_io_value = 0;
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

	evn_ve_acc_io = getenv(ENV_KEY_VE_ACC_IO);
	if (evn_ve_acc_io != NULL) {
		ve_acc_io_value = (int)strtol(evn_ve_acc_io, &ptr_env, 3);
	}

	if (ve_acc_io_value == SIZE_BY_CORE) {
		if (FAIL == (int)ve_get_ve_info("num_of_core",
					core_num_buff, CORE_NUM_BUFF_SIZE)) {
			retval = FAIL;
			goto error_unlock_vh;
		}
		vh_resources.ve_core_num = atoi(core_num_buff);
	} else {
		vh_resources.ve_core_num = DEFAULT_CORE_NUM;
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
	
	for (i = 0; i < BUFF_NPARAS; i++) {
		io_info->ve_buff[i] = (uint64_t)&ve_resources.ve_io_buff[0]
							+ PARAS_SIZE * i;
		io_info->vh_vehva[i] = vh_resources.vehva[vh_buff_num]
							+ PARAS_SIZE * i;
		io_info->ve_vehva[i] = ve_resources.local_vehva
							+ PARAS_SIZE * i;
	}

	if (pthread_spin_unlock(&vh_resources.lock) != 0) {
		abort();
	}
	/* Set accelerated io bit flag to VHVA */
	for (i = 0; i < BUFF_NPARAS; i++) {
		io_info->vh_buff_and_flag[i]
			= (((uint64_t)vh_buff) | VE_ACCELERATED_IO_FLAG)
							+ PARAS_SIZE * i;
	}
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
 * @brief The continued processing after DMA transmission is processed for each
 * "8MB / BUFF_NPARAS".
 * This function finds the min number in these continued processing that has
 * not been executed yet.This function ,
 *
 * @param[in] point of array
 *
 * @retval the element number of array.
 */
static inline int __ve_find_min_posted(uint64_t *posted)
{
	int k;
	int min_k = 0;
	uint64_t min_v = UINT64_MAX;
	for (k = 0; k < BUFF_NPARAS; k++) {
		if (posted[k] == -1) {
			continue;
		}
		if (posted[k] < min_v) {
			min_v = posted[k];
			min_k = k;
		}
	}
	return min_k;
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
	int i,j,k;
	int ret = 0;
	int dma_ret = 0;
	int errno_bak = 0;
	uint64_t posted[BUFF_NPARAS];
	uint64_t read_num = 0;
	int min_k = 0;
	int data_err = 0;
	acc_io_info io_info;
	ssize_t transfer_size = PARAS_SIZE;
	ssize_t read_out_size[BUFF_NPARAS];
	ssize_t exit_result = 0;
	ve_dma_handle_t vedma_handle[BUFF_NPARAS];

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

	memset(posted, -1, sizeof(posted));
	read_num = (count + transfer_size - 1)/ transfer_size;
	for (i = 0; i < read_num; i++) {
		if (i == 1) {
			for (j = 0; j < BUFF_NPARAS; j++) {
				io_info.vh_buff_and_flag[j]
					= io_info.vh_buff_and_flag[j]
						| VE_SECOND_SYS_CALL_FLAG;
			}
		}
		if (i == read_num - 1) {
			transfer_size = count - transfer_size * i;
		}

		/* If not last 8MB */
		j = i % BUFF_NPARAS;
		if (i >= BUFF_NPARAS) {
			/* Wait DMA */
			dma_ret = ve_dma_wait(&vedma_handle[j]);
			if (dma_ret >= 1) {
				exit_result = FAIL;
				errno_bak = EIO;
				/* Data cannot be processed correctly
				 * because "ve_dma_wait()" is failed,
				 * but "ve_dma_wait()" of other "ve_dma_post()"
				 * are need.
				 */
				data_err = 1;
				posted[j] = -1;
				break;
			}
			/* Copy data from ve buffer to user buffer */
			__libsysve_vec_memcpy(buf,
					(void *)((uint64_t)io_info.ve_buff[j]),
							read_out_size[j]);
			buf = (void *)((uint64_t)buf + read_out_size[j]);
			exit_result += read_out_size[j];
		}

		/* Execute syscall call and data transfer in 1 set
		 * and BUFF_NPARAS sets in parallel
		 */
		/* Call syscall */
		read_out_size[j] = syscall(syscall_num, fd,
				io_info.vh_buff_and_flag[j], transfer_size, ofs);
		if (FAIL == read_out_size[j]) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = read_out_size[j];
			}
			errno = 0;
			posted[j] = -1;
			break;
		} else if (0 == read_out_size[j]) {
			posted[j] = -1;
			break;
		}
		/* Transfer data from VH buffer to VE buffer by VE DMA */
		ret = ve_dma_post(io_info.ve_vehva[j], io_info.vh_vehva[j],
					GET_DMA_SIZE(read_out_size[j]),
					&vedma_handle[j]);
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			ofs = ofs + transfer_size;
			posted[j] = -1;
			data_err = 1;
			break;
		}

		ofs = ofs + read_out_size[j];
		posted[j] = i;
		if (transfer_size > read_out_size[j]) {
			break;
		}
	}

	/* Last 8MB */
	min_k =  __ve_find_min_posted(posted);
	for (k = 0; k < BUFF_NPARAS; k++) {
		i = posted[(min_k + k) % BUFF_NPARAS];

		if (i == -1) {
			continue;
		}
		j = i % BUFF_NPARAS;
		/* Wait DMA */
		dma_ret = ve_dma_wait(&vedma_handle[j]);
		if (dma_ret >= 1) {
			exit_result = FAIL;
			errno_bak = EIO;
			/* Data cannot be processed correctly
 			 * because "ve_dma_wait()" is failed,
			 * but "ve_dma_wait()" of other "ve_dma_post()"
			 * are need.
			 */
			data_err = 1;
		}
		if (!data_err) {
		/* Copy data from ve buffer to user buffer */
			__libsysve_vec_memcpy(buf,
					(void *)((uint64_t)io_info.ve_buff[j]),
							read_out_size[j]);

			buf = (void *)((uint64_t)buf + read_out_size[j]);
			exit_result += read_out_size[j];
		}
	}

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
	int i,j,k,n;
	int ret = 0;
	int dma_ret = 0;
	int errno_bak = 0;
	uint64_t posted[BUFF_NPARAS];
	uint64_t read_num = 0;
	int min_k = 0;
	int data_err = 0;
	acc_io_info io_info;
	ssize_t total_size = 0;
	ssize_t transfer_size = PARAS_SIZE;
	ssize_t read_out_size[BUFF_NPARAS];
	ssize_t exit_result = 0;
	int read_syscall_type = SYS_read;

	int next_iov_num = 0;
	ssize_t iov_len_done = 0;
	ssize_t iov_len_less = 0;
	ssize_t ve_buff_size_less = 0;
	ssize_t iov_copy_size = 0;
	uint64_t source_buff;
	ve_dma_handle_t vedma_handle[BUFF_NPARAS];

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

	memset(posted, -1, sizeof(posted));
	read_num = (total_size + transfer_size - 1)/ transfer_size;
	for (i = 0; i < read_num; i++) {
		if (i == 1) {
			for (j = 0; j < BUFF_NPARAS; j++) {
				io_info.vh_buff_and_flag[j]
					= io_info.vh_buff_and_flag[j]
						| VE_SECOND_SYS_CALL_FLAG;
			}
		}
		if (i == read_num - 1) {
			transfer_size = total_size - transfer_size * i;
		}

		j = i % BUFF_NPARAS;
		/* If not last 8MB */
		if (i >= BUFF_NPARAS) {
			/* Wait DMA */
			dma_ret = ve_dma_wait(&vedma_handle[j]);
			if (dma_ret >= 1) {
				exit_result = FAIL;
				errno_bak = EIO;
				/* Data cannot be processed correctly
				 * because "ve_dma_wait()" is failed,
				 * but "ve_dma_wait()" of other "ve_dma_post()"
				 * are need.
				 */
				data_err = 1;
				posted[j] = -1;
				break;
			}

			ve_buff_size_less = read_out_size[j];
			source_buff = io_info.ve_buff[j];
			for (n = next_iov_num; n < count; n++) {
				iov_len_less = iov[n].iov_len - iov_len_done;
				iov_copy_size = MIN (iov_len_less, ve_buff_size_less);
				__libsysve_vec_memcpy(
					(void *)((uint64_t)iov[n].iov_base
							+ iov_len_done),
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
			exit_result += read_out_size[j];
		}

		/* Execute syscall call and data transfer in 1 set
		 * and BUFF_NPARAS sets in parallel
		 */
		/* Call syscall */
		read_out_size[j] = syscall(read_syscall_type, fd,
				io_info.vh_buff_and_flag[j], transfer_size, ofs);
		if (FAIL == read_out_size[j]) {
			if (0 == i) {
				errno_bak = errno;
				exit_result = read_out_size[j];
			}
			errno = 0;
			posted[j] = -1;
			break;
		} else if (0 == read_out_size[j]) {
			posted[j] = -1;
			break;
		}
		/* Transfer data from VH buffer to VE buffer by VE DMA */
		ret = ve_dma_post(io_info.ve_vehva[j], io_info.vh_vehva[j],
					GET_DMA_SIZE(read_out_size[j]),
					&vedma_handle[j]);

		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			ofs = ofs + transfer_size;
			posted[j] = -1;
			data_err = 1;
			break;
		}

		ofs = ofs + read_out_size[j];
		posted[j] = i;
		if (transfer_size > read_out_size[j]) {
			break;
		}
	}

	/* Last 8MB */
	min_k =  __ve_find_min_posted(posted);
	for (k = 0; k < BUFF_NPARAS; k++) {
		i = posted[(min_k + k) % BUFF_NPARAS];
		if (i == -1) {
			continue;
		}
		j = i % BUFF_NPARAS;

		/* Wait DMA */
		dma_ret = ve_dma_wait(&vedma_handle[j]);
		if (dma_ret >= 1) {
			exit_result = FAIL;
			errno_bak = EIO;
			/* Data cannot be processed correctly
			 * because "ve_dma_wait()" is failed,
			 * but "ve_dma_wait()" of other "ve_dma_post()"
			 * are need.
			 */
			data_err = 1;
		}

		if (!data_err) {
			ve_buff_size_less = read_out_size[j];
			source_buff = io_info.ve_buff[j];
			for (n = next_iov_num; n < count; n++) {
				iov_len_less = iov[n].iov_len - iov_len_done;
				iov_copy_size = MIN (iov_len_less,
							ve_buff_size_less);
				__libsysve_vec_memcpy(
					(void *)((uint64_t)iov[n].iov_base
								+ iov_len_done),
							(void *)source_buff,
							iov_copy_size);
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
				ve_buff_size_less = ve_buff_size_less
							- iov_copy_size;
				source_buff = source_buff + iov_copy_size;
			}
			exit_result += read_out_size[j];
		}
	}

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
	int i,j,k;
	int ret = 0;
	int dma_ret = 0;
	int errno_bak = 0;
	uint64_t posted[BUFF_NPARAS];
	uint64_t write_num = 0;
	int min_k = 0;
	int data_err = 0;
	acc_io_info io_info;
	ssize_t transfer_size = PARAS_SIZE;
	ssize_t need_write_in_size[BUFF_NPARAS] = {0};
	ssize_t write_in_size;
	ssize_t exit_result = 0;
	ve_dma_handle_t vedma_handle[BUFF_NPARAS];

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

	memset(posted, -1, sizeof(posted));
	write_num = (count + transfer_size - 1)/ transfer_size;
	for (i = 0; i < write_num; i++) {
		if (i == 1) {
			for (j = 0; j < BUFF_NPARAS; j++) {
				io_info.vh_buff_and_flag[j]
					= io_info.vh_buff_and_flag[j]
						| VE_SECOND_SYS_CALL_FLAG;
			}
		}
		if (i == write_num - 1) {
			transfer_size = count - transfer_size * i;
		}
		j = i % BUFF_NPARAS;
		/* If not last 8MB */
		if (i >= BUFF_NPARAS) {
			/* Wait DMA */
			dma_ret = ve_dma_wait(&vedma_handle[j]);
			if (dma_ret >= 1) {
				exit_result = FAIL;
				errno_bak = EIO;
				/* Data cannot be processed correctly
				 * because "ve_dma_wait()" is failed,
				 * but "ve_dma_wait()" of other "ve_dma_post()"
				 * are need.
				 */
				data_err = 1;
				posted[j] = -1;
				break;
			}

			/* Call syscall */
			write_in_size = syscall(syscall_num, fd,
						io_info.vh_buff_and_flag[j],
						PARAS_SIZE, ofs);
			if (FAIL == write_in_size) {
				if (0 == i) {
					errno_bak = errno;
					exit_result = write_in_size;
				}
				/* Data cannot be processed correctly
				 * because "ve_dma_wait()" is failed,
				 * but "ve_dma_wait()" of other "ve_dma_post()"
				 * are need.
				 */
				errno = 0;
				data_err = 1;
				posted[j] = -1;
				break;
			} else if (0 == write_in_size) {
				data_err = 1;
				posted[j] = -1;
				break;
			}
			exit_result += write_in_size;
			ofs += write_in_size;
			if (PARAS_SIZE > write_in_size) {
				data_err = 1;
				posted[j] = -1;
				break;
			}
		}
		/* Execute memory copy and data transfer in 1 set
		 * and BUFF_NPARAS sets in parallel
		 */
		/* Copy data from user buffer to VE buffer */
		__libsysve_vec_memcpy((void *)io_info.ve_buff[j],
						(void *)buf, transfer_size);

		/* Transfer data from VE buffer to VH buffer by VE DMA */
		ret = ve_dma_post(io_info.vh_vehva[j], io_info.ve_vehva[j],
					GET_DMA_SIZE(transfer_size),
					&vedma_handle[j]);
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			posted[j] = -1;
			buf = (void *)((uint64_t)buf + transfer_size);
			data_err = 1;
			break;
		}
		buf = (void *)((uint64_t)buf + transfer_size);
		posted[j] = i;
		need_write_in_size[j] = transfer_size;
	}

	/* Last 8MB */
	min_k =  __ve_find_min_posted(posted);
	for (k = 0; k < BUFF_NPARAS; k++) {
		i = posted[(min_k + k) % BUFF_NPARAS];
		if (i == -1) {
			continue;
		}
		j = i % BUFF_NPARAS;
		/* Wait DMA */
		dma_ret = ve_dma_wait(&vedma_handle[j]);
		if (dma_ret >= 1) {
			exit_result = FAIL;
			errno_bak = EIO;
			/* Data cannot be processed correctly
			 * because "ve_dma_wait()" is failed,
			 * but "ve_dma_wait()" of other "ve_dma_post()"
			 * are need.
			 */
			data_err = 1;
		}
		if (!data_err) {
			/* Call syscall */
			write_in_size = syscall(syscall_num, fd,
						io_info.vh_buff_and_flag[j],
						need_write_in_size[j], ofs);
			if (FAIL == write_in_size) {
				if (0 == i) {
					errno_bak = errno;
					exit_result = write_in_size;
				}
				errno = 0;
				data_err = 1;
				continue;
			} else if (0 == write_in_size) {
				data_err = 1;
				continue;
			}
			exit_result += write_in_size;
			ofs += write_in_size;
			if (need_write_in_size[j] > write_in_size) {
				data_err = 1;
				continue;
			}
		}
	}

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
	int i,j,k,n;
	int ret = 0;
	int dma_ret = 0;
	int errno_bak = 0;
	uint64_t posted[BUFF_NPARAS];
	uint64_t write_num = 0;
	int min_k = 0;
	int data_err = 0;
	acc_io_info io_info;
	ssize_t total_size = 0;
	ssize_t transfer_size = PARAS_SIZE;
	ssize_t need_write_in_size[BUFF_NPARAS] = {0};
	ssize_t write_in_size;
	ssize_t exit_result = 0;
	int write_syscall_type = SYS_write;

	int next_iov_num = 0;
	ssize_t iov_len_done = 0;
	ssize_t iov_len_less = 0;
	ssize_t ve_buff_size_less = 0;
	ssize_t iov_copy_size = 0;
	uint64_t target_buff;
	ve_dma_handle_t vedma_handle[BUFF_NPARAS];

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

	memset(posted, -1, sizeof(posted));
	write_num = (total_size + transfer_size - 1)/ transfer_size;
	for (i = 0; i < write_num; i++) {
		if (i == 1) {
			for (j = 0; j < BUFF_NPARAS; j++) {
				io_info.vh_buff_and_flag[j]
					= io_info.vh_buff_and_flag[j]
						| VE_SECOND_SYS_CALL_FLAG;
			}
		}
		if (i == write_num - 1) {
			transfer_size = total_size - transfer_size * i;
		}

		/* If not last 8MB */
		j = i % BUFF_NPARAS;
		if (i >= BUFF_NPARAS) {
			/* Wait DMA */
			dma_ret = ve_dma_wait(&vedma_handle[j]);
			if (dma_ret >= 1) {
				exit_result = FAIL;
				errno_bak = EIO;
				/* Data cannot be processed correctly
				 * because "ve_dma_wait()" is failed,
				 * but "ve_dma_wait()" of other "ve_dma_post()"
				 * are need.
				 */
				data_err = 1;
				posted[j] = -1;
				break;
			}

			/* Call syscall */
			write_in_size = syscall(write_syscall_type, fd,
						io_info.vh_buff_and_flag[j],
						PARAS_SIZE, ofs);
			if (FAIL == write_in_size) {
				if (0 == i) {
					errno_bak = errno;
					exit_result = write_in_size;
				}
				errno = 0;
				data_err = 1;
				posted[j] = -1;
				break;
			} else if (0 == write_in_size) {
				data_err = 1;
				posted[j] = -1;
				break;
			}
			exit_result += write_in_size;
			ofs += write_in_size;
			if (PARAS_SIZE > write_in_size) {
				data_err = 1;
				posted[j] = -1;
				break;
			}			
		}
		/* Execute memory copy and data transfer in 1 set
		 * and BUFF_NPARAS sets in parallel
		 */
		ve_buff_size_less = transfer_size;
		target_buff = io_info.ve_buff[j];
		for (n = next_iov_num; n < count; n++) {
			iov_len_less = iov[n].iov_len - iov_len_done;
			iov_copy_size = MIN (iov_len_less, ve_buff_size_less);
			__libsysve_vec_memcpy((void *)target_buff,
					(void *)((uint64_t)iov[n].iov_base
						+ iov_len_done),
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
				/* This loop breaks always here
				 * because the user-specified buffer should be
				 * larger than the data in the VE buffer
				 */
				ve_buff_size_less = ve_buff_size_less
							- iov_copy_size;
				break;
			}
			iov_len_done = 0;
			ve_buff_size_less = ve_buff_size_less - iov_copy_size;
			target_buff = target_buff + iov_copy_size;
		}

		/* Transfer data from VE buffer to VH buffer by VE DMA */
		ret = ve_dma_post(io_info.vh_vehva[j], io_info.ve_vehva[j],
					GET_DMA_SIZE(transfer_size),
					&vedma_handle[j]);
		if (SUCCESS != ret) {
			exit_result = FAIL;
			errno_bak = EIO;
			posted[j] = -1;
			data_err = 1;
			break;
		}

		posted[j] = i;
		need_write_in_size[j] = transfer_size - ve_buff_size_less;
	}

	/* Last 8MB */
	min_k =  __ve_find_min_posted(posted);
	for (k = 0; k < BUFF_NPARAS; k++) {
		i = posted[(min_k + k) % BUFF_NPARAS];
		if (i == -1) {
			continue;
		}
		j = i % BUFF_NPARAS;
		/* Wait DMA */
		dma_ret = ve_dma_wait(&vedma_handle[j]);
		if (dma_ret >= 1) {
			exit_result = FAIL;
			errno_bak = EIO;
			/* Data cannot be processed correctly
			 * because "ve_dma_wait()" is failed,
			 * but "ve_dma_wait()" of other "ve_dma_post()"
			 * are need.
			 */
			data_err = 1;
		}
		/* Call syscall */
		if (!data_err) {
			write_in_size = syscall(write_syscall_type, fd,
						io_info.vh_buff_and_flag[j],
						need_write_in_size[j], ofs);
			if (FAIL == write_in_size) {
				if (0 == i) {
					errno_bak = errno;
					exit_result = write_in_size;
				}
				errno = 0;
				data_err = 1;
				continue;
			} else if (0 == write_in_size) {
				data_err = 1;
				continue;
			}
			exit_result += write_in_size;
			ofs += write_in_size;
			if (need_write_in_size[j] > write_in_size) {
				data_err = 1;
				continue;
			}
		}
	}

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
