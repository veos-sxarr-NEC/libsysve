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
 * @file libvhcall.c
 * @brief VE side library for VH call
 */
#include <sys/types.h>
#include <unistd.h>
#include <sysve.h>
#include <veos_defs.h>
#include <libvhcall.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/**
 * \defgroup vhcall VH call
 *
 * VH call is a feature for VE programs to invoke functions on VH.
 *
 * Please include "libvhcall.h".
 *
 * @note vhcall_invoke_with_args() is expanded API of vhcall_invoke() for
 *       passing arguments. vhcall_args_alloc() and functions in
 *       vhcall_args_set_double() family (they are found in API list with prefix
 *       "vhcall_args_set_") are
 *       available for generating VH function arguments from VE side by
 *       following two steps:
 *         1. vhcall_args_alloc() allocates new list for arguments of VH
 *         function. The list is represented by "vhcall_args".
 *         2. For each argument, function of vhcall_args_set_double() family sets its
 *         value to vhcall_args.
 *       vhcall_args_set_double() family is corresponding to each fundamental
 *       type of argument (int_8t, float, pointer and etc...)
 *       but long double.
 */
/*@{*/

/**
 * @brief Load a VH library
 *
 * @param[in] filename the file name to load on VH
 *
 * @return VH call handle of the VH library
 * @retval -1 upon failure and the following errno is set:
 *          - ENOENT  filename cannot be loaded;
 *          - EFAULT  filename is not accessible;
 *          - ENOMEM  not enough VH memory.
 */
vhcall_handle vhcall_install(const char *filename)
{
	return syscall(SYS_sysve, VE_SYSVE_VHCALL_INSTALL, filename);
}

/**
 * @brief Find a symbol in VH library
 *
 * @param hdl VH call handle of VH library
 * @param[in] symname the name of a function
 *
 * @return symbol ID of the function upon success; negative upon failure.
 * @retval -1 upon failure and the following errno is set:
 *          - EINVAL  symname is not found in hdl.
 *          - EFAULT  symname is not accessible.
 *          - ENOMEM  not enough VH memory.
 */
int64_t vhcall_find(vhcall_handle hdl, const char *symname)
{
	return syscall(SYS_sysve, VE_SYSVE_VHCALL_FIND, hdl, symname);
}

#ifndef VHCALLNOENHANCE
/**
 * @brief Allocate VHCall arguments object (vhcall_args)
 *
 * @return pointer to vehcall_args
 * @retval NULL the allocation of vhcall_args failed and following errno is set:
 *          - ENOMEM not enough VE memory.
 */
vhcall_args *vhcall_args_alloc(void)
{
        vhcall_args *p;
        p = (vhcall_args *)calloc(1, sizeof(vhcall_args));
        if (p == NULL) {
                errno = ENOMEM;
                goto ret;
        }

        p->num = -1;
        p->next = NULL;
ret:
        return p;
}

/**
 * @brief Append VHCall arguments object
 *
 * @return pointer to vehcall_args
 * @retval NULL the allocation of vhcall_args failed.
 */
static vhcall_args *vhcall_args_append(vhcall_args *list_head, int num)
{
        vhcall_args *p, *tmp;
        p = list_head;
        while (p->num < num) {
                if (p->next == NULL) {
                        tmp = calloc(1, sizeof(vhcall_args));
                        if (tmp == NULL)
                                return NULL;
			tmp->num = num;
			tmp->next = NULL;
			p->next = tmp;
			p = p->next;
                        break;
                } else {
			tmp = p;
                        p = p->next;
                }
        }

        if (p->num > num) {
		p = tmp;
                tmp = calloc(1, sizeof(vhcall_args));
                if (tmp == NULL)
                        return NULL;
		tmp->num = num;
		tmp->next = p->next;
                p->next = tmp;
                p = p->next;
        }

        return p;
}

/**
 * @brief Set value to VHCall arguments object
 *
 * @param ca vhcall_args
 * @param inout intent of argument
 * @param argnum the argnum-th argument
 * @param val pointer to value to be set
 * @param size size of value
 * @param class type of value
 * @return zero upon success; -1 upon failure and following errno is set:
 *          - EINVAL some of arguments are invalid.
 *          - ENOMEM not enough VE memory.
 */
static int vhcall_args_set(vhcall_args *ca,
		enum vhcall_args_intent inout,
		int argnum, void *val, size_t size,
		enum vhcall_args_class class) {
        int ret = -1;
        vhcall_args *p;

        if (ca == NULL || argnum < 0 || inout < VHCALL_INTENT_IN
			|| inout > VHCALL_INTENT_OUT) {
                errno = EINVAL;
                goto ret;
        }

        p = vhcall_args_append(ca, argnum);
        if (p == NULL) {
                errno = ENOMEM;
                goto ret;
        }

	if (class == VHCALL_CLASS_PTR)
		p->data.val[0] = (uint64_t)val;
	else
		memcpy(&p->data.val, val, size);
	p->data.inout = inout;
        p->data.size = size;
	p->data.cl = class;
        ret = 0;
ret:
        return ret;
}

/**
 * @brief Set a 8-bit signed integer argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 *         - EINVAL ca is NULL or argnum is negative.
 *         - ENOMEM not enough VE memory.
 */
int vhcall_args_set_i8(vhcall_args *ca, int argnum, int8_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
			&val, sizeof(int8_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 8-bit unsigned integer argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_u8(vhcall_args *ca, int argnum, uint8_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(uint8_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 16-bit signed integer argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_i16(vhcall_args *ca, int argnum, int16_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(int16_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 16-bit unsigned integer argument
 *
 * @param ca vhcall_arg
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 *         - EINVAL ca is NULL or argnum is negative.
 *         - ENOMEM not enough VE memory.
 */
int vhcall_args_set_u16(vhcall_args *ca, int argnum, uint16_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(uint16_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 32-bit signed integer argument
 *
 * @param ca vhcall_arg
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_i32(vhcall_args *ca, int argnum, int32_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(int32_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 32-bit unsigned integer argument
 *
 * @param ca vhcall_arg
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_u32(vhcall_args *ca, int argnum, uint32_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(uint32_t), VHCALL_CLASS_INT);
}

/**
 *@brief Set a 64-bit signed integer argument
 *
 * @param ca vhcall_arg
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_i64(vhcall_args *ca, int argnum, int64_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(int64_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a 64-bit unsigned integer argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_u64(vhcall_args *ca, int argnum, uint64_t val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(uint64_t), VHCALL_CLASS_INT);
}

/**
 * @brief Set a single precision floating point number argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_float(vhcall_args *ca, int argnum, float val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(float), VHCALL_CLASS_DBL);
}

/**
 * @brief Set a double precision floating point number argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_double(vhcall_args *ca, int argnum, double val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(double), VHCALL_CLASS_DBL);
}

/**
 * @brief Set a single precision floating point complex object argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and following errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_complex_float(vhcall_args *ca, int argnum,
			_Complex float val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(_Complex float), VHCALL_CLASS_DBL);
}

/**
 * @brief Set a double precision floating point complex object argument
 *
 * @param ca vhcall_args
 * @param argnum the argnum-th argument
 * @param val value to be set
 * @return zero upon success; negative upon failure and errno is set.
 * 	   - EINVAL ca is NULL or argnum is negative.
 * 	   - ENOMEM not enough VE memory.
 */
int vhcall_args_set_complex_double(vhcall_args *ca, int argnum,
                        _Complex double val) {
        return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
                        &val, sizeof(_Complex double), VHCALL_CLASS_CDB);
}

/**
 * @brief Set VH function argument of pointer type
 *
 * @param ca pointer to vhcall_args object
 * @param inout intent of argument. VHCALL_INTENT_IN, VHCALL_INTENT_OUT,
 *        VHCALL_INTENT_INOUT are supported
 * @param argnum argument number that is being set
 * @param[inout] buff char pointer to be set as argument
 * @param len length of buffer that is specified as argument
 * @retval  0 argnum is successfully set
 * @retval -1 an error occurred and following errno is set
 * 	  - EINVAL ca is NULL, intent is not vhcall_args_intent type or
 * 	           argnum is negative.
 * 	  - ENOMEM not enough VE memory.
 *
 * Buffer is allocated on VH side and its pointer is given as actual
 * argument of VH function. The VH buffer is copy-in/copy-out from/to buff
 * depending on intent. Use this in following cases:
 *   - pass by pointer (eg. C language functions)
 *   - pass by reference (eg. GNU Fortran functions)
 */
int vhcall_args_set_pointer(vhcall_args *ca,
		enum vhcall_args_intent inout,
		int argnum, void *buff, size_t len) {
        return vhcall_args_set(ca, inout, argnum, buff, len, VHCALL_CLASS_PTR);
}

/**
 * @brief Set VH function argument of pointer to VEOS handle
 *
 * @note Please include libvepseudo.h to use VEOS handle in VH function.
 *
 * @param ca pointer to vhcall_args object
 * @param argnum argument number that is being set
 *
 * @retval  0 argumen is successfully set.
 * @retval -1 an error occurred and following errno is set.
 *          - EINVAL ca is NULL or argnum is negative.
 *          - ENOMEM not enough VE memory.
 *
 * VEOS handle is used to send/receive data to/from VE in VHCall function.
 */
int vhcall_args_set_veoshandle(vhcall_args *ca, int argnum) {
	return vhcall_args_set(ca, VHCALL_INTENT_IN, argnum,
			NULL, 0, VHCALL_CLASS_HDL);
}

/**
 * @brief Clear arguments set in VHCall arguments object
 *
 * @param ca vhcall_args object
 */
void vhcall_args_clear(vhcall_args *ca) {
	vhcall_args *p, *prev;
	if (!ca)
		return;
	p = ca->next;
	while (p != NULL) {
		prev = p;
		p = p->next;
		free(prev);
	}
	ca->next = NULL;
	return;
}

/**
 * @brief Free VHCall arguments object
 *
 * @param ca vhcall_args object
 */
void vhcall_args_free(vhcall_args *ca) {
	vhcall_args_clear(ca);
	free(ca);
	return ;
}

/**
 * @brief Invoke a function in VH library with passing arguments. 
 *
 * @note  "vhcall_args" passed to vhcall_invoke_with_args() must have all of
 * 	  arguments of VH function to be invoked. If not, result is unexpected.
 * @note  Return value of VH function to be invoked must be uint64_t. It can be
 * 	  got through pointer passed to vhcall_invoke_with_args().
 *
 * @param symid symbol id of VH function to call
 * @param args arguments to be passed to the VH function
 * @param[out] retval pointer to buffer storing return value of VH library function
 *
 * @return 0 upon success, -1 upon failure and following errno is set.
 *         - EINVAL args is not complete i.e. some list elements of args are missing.
 *         - ENOMEM not enough VE memory or not enough VH memory.
 *         - EFAULT failure to send/receive data to/from VE on VH side.
 */
int vhcall_invoke_with_args(int64_t symid, vhcall_args *args, uint64_t *retval)
{
	uint64_t num_args;
	int ret = -1;
	vhcall_args *p;
	vhcall_data data;
	void *inptr = NULL;
	size_t insize = 0;

	if (args == NULL) {
		errno = EINVAL;
		return -1;
	}

	p = args;
	while (p != NULL) {
                if (p->next != NULL)
                        if (p->num+1 != p->next->num) {
                                errno = EINVAL;
				return -1;
                        }
		num_args = p->num+1;
		if (p->data.cl == VHCALL_CLASS_CDB)
			insize += 2*sizeof(vhcall_data);
		else if (num_args != 0)
			insize += sizeof(vhcall_data);
                p = p->next;
	}

	if (insize > 0) {
		inptr = calloc(1, insize);
		if (inptr == NULL) {
			errno = ENOMEM;
			return -1;
		}
	}

	for (p = args->next; p != NULL; p = p->next) {
		if (p->data.cl == VHCALL_CLASS_CDB) {
			data.cl = VHCALL_CLASS_DBL;
			data.inout = VHCALL_INTENT_IN;
			data.size = sizeof(double);
			memcpy(data.val, &(p->data.val[0]), sizeof(double));
			memcpy(inptr, &data, sizeof(vhcall_data));
			inptr = (void *)((char *)inptr + sizeof(vhcall_data));
			memcpy(data.val, &(p->data.val[1]), sizeof(double));
			memcpy(inptr, &data, sizeof(vhcall_data));
			inptr = (void *)((char *)inptr + sizeof(vhcall_data));
		} else {
			memcpy(inptr, &p->data, sizeof(vhcall_data));
			inptr = (void *)((char *)inptr + sizeof(vhcall_data));
		}
	}
	inptr = (void *)((char *)inptr - insize);

	ret = syscall(SYS_sysve, VE_SYSVE_VHCALL_INVOKE_WITH_ARGS, symid,
			inptr, insize, retval);

	free(inptr);
	return ret;
}
#endif
/**
* @brief Invoke a function in VH library
*
* @param symid symbol ID to invoke
* @param[in] inptr input buffer
* @param insize size of input buffer
* @param[out] outptr output buffer
* @param outsize size of output buffer
*
* @return return value from the function specified by symid.
* @retval -1 and errno is set when the return value is between -4095 and -1.
*/
long vhcall_invoke(int64_t symid, const void *inptr, size_t insize,
		   void *outptr, size_t outsize)
{
	return syscall(SYS_sysve, VE_SYSVE_VHCALL_INVOKE, symid, inptr,
		       insize, outptr, outsize);
}

/**
 * @brief Unload a VH library
 *
 * @param hdl VH call handle to close
 *
 * @retval 0 on success.
 * @retval non-zero value on error: the value returned from dlclose() on VH.
 */
int vhcall_uninstall(vhcall_handle hdl)
{
	return syscall(SYS_sysve, VE_SYSVE_VHCALL_UNINSTALL, hdl);
}
