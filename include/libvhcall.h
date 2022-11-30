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
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __VE_LIBVHCALL_H
#define __VE_LIBVHCALL_H
#include <vhcall.h>

typedef struct vhcall_args {
        int num;
        vhcall_data data;
        struct vhcall_args *next;
        int args_num;
} vhcall_args;

vhcall_handle vhcall_install(const char *);
int64_t vhcall_find(vhcall_handle, const char *);
long vhcall_invoke(int64_t, const void *, size_t, void *, size_t);
int vhcall_uninstall(vhcall_handle);
#ifndef VHCALLNOENHANCE
vhcall_args *vhcall_args_alloc(void);
vhcall_args *vhcall_args_alloc_num(int num);
int vhcall_args_set_i8(vhcall_args *, int, int8_t);
int vhcall_args_set_u8(vhcall_args *, int, uint8_t);
int vhcall_args_set_i16(vhcall_args *, int, int16_t);
int vhcall_args_set_u16(vhcall_args *, int, uint16_t);
int vhcall_args_set_i32(vhcall_args *, int, int32_t);
int vhcall_args_set_u32(vhcall_args *, int, uint32_t);
int vhcall_args_set_i64(vhcall_args *, int, int64_t);
int vhcall_args_set_u64(vhcall_args *, int, uint64_t);
int vhcall_args_set_float(vhcall_args *, int, float);
int vhcall_args_set_double(vhcall_args *, int, double);
int vhcall_args_set_complex_float(vhcall_args *, int, _Complex float);
int vhcall_args_set_complex_double(vhcall_args *, int, _Complex double);
int vhcall_args_set_pointer(vhcall_args *, enum vhcall_args_intent,
				int, void *, size_t);
int vhcall_args_set_veoshandle(vhcall_args *, int);
int vhcall_invoke_with_args(int64_t, vhcall_args *, uint64_t*);
void vhcall_args_clear(vhcall_args *);
void vhcall_args_free(vhcall_args *);
#endif

#endif

#ifdef __cplusplus
}
#endif
