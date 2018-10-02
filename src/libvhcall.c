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
 * @file libvhcall.c
 * @brief VE side library for VH call
 */
#include <sys/types.h>
#include <unistd.h>
#include <sysve.h>
#include <veos_defs.h>
#include <libvhcall.h>

/**
 * \defgroup vhcall VH call
 *
 * VH call is a feature for VE programs to invoke functions on VH.
 *
 * Please include "libvhcall.h".
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
