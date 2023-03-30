# Copyright (C) 2018 by NEC Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

#define vedma_desc	0
#define vedma_lock	0x8
#define vedma_index	0x10
#define vedma_status	0x18
#define EAGAIN		11

#ifdef __PIC__
# use %s40 as %got, %s41 as %plt
#define GET_GOT	\
	lea	%s40, _GLOBAL_OFFSET_TABLE_@PC_LO(-24); \
	and	%s40, %s40, (32)0; \
	sic	%s41; \
	lea.sl	%s40, _GLOBAL_OFFSET_TABLE_@PC_HI(%s40,%s41)
#endif

#
# int ve_dma_post(uint64_t dst, uint64_t src, int size,
#		 ve_dma_handle_t *handle)
#
	.text
	.balign 256
#ifndef __PIC__
	.extern	_vedma_vars
#endif

	.global	ve_dma_post
	.type	ve_dma_post, @function

ve_dma_post:
#ifdef __PIC__
	# get vedma_vars (s37)
	GET_GOT
	lea		%s37, vedma_vars@GOT_LO
	and		%s37, %s37, (32)0
	lea.sl		%s37, vedma_vars@GOT_HI(%s37,%s40)
	ld		%s37, 0(%s37)
#else
        # get vedma_vars (s37)
        lea             %s37, vedma_vars@LO
        and             %s37, %s37, (32)0
        lea.sl          %s37, vedma_vars@HI(,%s37)
#endif

	# lock
	or		%s63, 0, %sl
	cas.l		%s63, vedma_lock(%s37), 0
	fencem		3
	brne.l.nt       0, %s63, -16

	# get index (s34)
	ld		%s34, vedma_index(%s37)

	# get descriptor address (s35)
	ld		%s35, vedma_desc(%s37)
	sll		%s63, %s34, 5
	addu.l		%s35, %s35, %s63

	# get &vedma_status[index] (s36)
	lea		%s36, vedma_status(%s37)
	sll		%s63, %s34, 3
	addu.l		%s36, %s36, %s63

	# check previous DMA
	ld		%s63, 0(%s36)
	breq.l.nt	1, %s63, 4f
	brne.l.nt	0, %s63, 3f

1:
	# set SYNC flag
	and		%s2, %s2, (32)0
	lea.sl		%s2, 8(, %s2)

	# start DMA
	shm.l		%s0, 0x18(%s35)
	shm.l		%s1, 0x10(%s35)
	shm.l		%s2, 0x08(%s35)
	or		%s63, 0, (63)0
	shm.l		%s63, 0(%s35)

	# initialize handle
	or		%s63, 0, (32)0
	stl		%s63, 0(%s3)
	stl		%s34, 4(%s3)

	# update vedma_status[index]
	st		%s3, 0(%s36)

	# update vedma_index
	addu.l		%s34, 1, %s34
	and		%s34, %s34, (57)0
	st		%s34, vedma_index(%s37)

	# set 0 to return value
	or		%s0, 0, (0)1
2:
	# unlock
	fencem		1
	or		%s63, 0, (0)1
	st		%s63, vedma_lock(%s37)

	b.l.t		0(, %s10)	# return

3:
	# check completion of previous DMA
	lhm.l		%s63, %s35
	and		%s62, 2, %s63
	breq.l.nt	0, %s62, 4f

	# save DMA status
	srl		%s62, %s63, 48
	ld		%s61, 0(%s36)
	stl		%s62, 0(%s61)

	# clear vedma_status[index]
	or		%s63, 0, (0)1
	st		%s63, 0(%s36)

	br.l.t		1b

4:
	# set -EAGAIN to return value
	or		%s0, -EAGAIN, (0)1
	br.l.t		2b

#
# int ve_dma_poll(ve_dma_handle_t *handle)
#
	.text
	.balign 256
#ifndef __PIC__
	.extern	_vedma_vars
#endif

	.global	ve_dma_poll
	.type	ve_dma_poll, @function

ve_dma_poll:
#ifdef __PIC__
	# get vedma_vars (s37)
	GET_GOT
	lea		%s37, vedma_vars@GOT_LO
	and		%s37, %s37, (32)0
	lea.sl		%s37, vedma_vars@GOT_HI(%s37,%s40)
	ld		%s37, 0(%s37)
#else
        # get vedma_vars (s37)
        lea             %s37, vedma_vars@LO
        and             %s37, %s37, (32)0
        lea.sl          %s37, vedma_vars@HI(,%s37)
#endif

	# lock
	or		%s63, 0, %sl
	cas.l		%s63, vedma_lock(%s37), 0
	fencem		2
	brne.l.nt       0, %s63, -16

	# check handle->status
	ldl		%s63, 0(%s0)
	brne.w.nt	-1, %s63, 2f

	# get index (s34)
	ldl		%s34, 4(%s0)

	# get descriptor address (s35)
	ld		%s35, vedma_desc(%s37)
	sll		%s63, %s34, 5
	addu.l		%s35, %s35, %s63

	# get &vedma_status[index] (s36)
	lea		%s36, vedma_status(%s37)
	sll		%s63, %s34, 3
	addu.l		%s36, %s36, %s63

	# check DMA completion
	lhm.l		%s63, %s35
	and		%s62, 2, %s63
	breq.l.nt	0, %s62, 3f

	# clear vedma_status[index]
	or		%s62, 0, (0)1
	st		%s62, 0(%s36)

	# set return value
	srl		%s0, %s63, 48

1:
	# unlock
	fencem		1
	or		%s63, 0, (0)1
	st		%s63, vedma_lock(%s37)

	fencem          2
	b.l.t		0(, %s10)	# return

2:
	# set handle->status to return value
	or		%s0, 0, %s63
	br.l.t		1b

3:
	# set -EAGAIN to return value
	or		%s0, -EAGAIN, (0)1
	br.l.t		1b

#
# int ve_dma_post_wait(uint64_t dst, uint64_t src, int size)
#
	.text
	.balign 256
#ifndef __PIC__
	.extern	_vedma_vars
#endif

	.global	ve_dma_post_wait
	.type	ve_dma_post_wait, @function

ve_dma_post_wait:
#ifdef __PIC__
	# get vedma_vars (s37)
	GET_GOT
	lea		%s37, vedma_vars@GOT_LO
	and		%s37, %s37, (32)0
	lea.sl		%s37, vedma_vars@GOT_HI(%s37,%s40)
	ld		%s37, 0(%s37)
#else
        # get vedma_vars (s37)
        lea             %s37, vedma_vars@LO
        and             %s37, %s37, (32)0
        lea.sl          %s37, vedma_vars@HI(,%s37)
#endif

	# lock
	or		%s63, 0, %sl
	cas.l		%s63, vedma_lock(%s37), 0
	fencem		3
	brne.l.nt       0, %s63, -16

	# get index (s34)
	ld		%s34, vedma_index(%s37)

	# get descriptor address (s35)
	ld		%s35, vedma_desc(%s37)
	sll		%s63, %s34, 5
	addu.l		%s35, %s35, %s63

	# get &vedma_status[index] (s36)
	lea		%s36, vedma_status(%s37)
	sll		%s63, %s34, 3
	addu.l		%s36, %s36, %s63

	# check previous DMA
	ld		%s63, 0(%s36)
	breq.l.nt	1, %s63, -8
	brne.l.nt	0, %s63, 2f

1:
	# set SYNC flag
	and		%s2, %s2, (32)0
	lea.sl		%s2, 8(, %s2)

	# start DMA
	shm.l		%s0, 0x18(%s35)
	shm.l		%s1, 0x10(%s35)
	shm.l		%s2, 0x08(%s35)
	or		%s63, 0, (63)0
	shm.l		%s63, 0(%s35)

	# set vedma_status[index]
	or		%s63, 1, (0)1
	st		%s63, 0(%s36)

	# update vedma_index
	addu.l		%s34, 1, %s34
	and		%s34, %s34, (57)0
	st		%s34, vedma_index(%s37)

	# unlock
	fencem		1
	or		%s63, 0, (0)1
	st		%s63, vedma_lock(%s37)

	# wait DMA
	lhm.l		%s63, %s35
	and		%s62, 2, %s63
	breq.l.nt	0, %s62, -0x10

	# clear vedma_status[index]
	or		%s61, 0, (0)1
	st		%s61, 0(%s36)

	# set return value
	srl		%s0, %s63, 48

	fencem          2
	b.l.t		0(, %s10)	# return

2:
	# wait previous DMA
	lhm.l		%s63, %s35
	and		%s62, 2, %s63
	breq.l.nt	0, %s62, -0x10

	# save DMA status
	srl		%s62, %s63, 48
	ld		%s61, 0(%s36)
	stl		%s62, 0(%s61)

	# clear vedma_status[index]
	or		%s63, 0, (0)1
	st		%s63, 0(%s36)

	br.l.t		1b

#
# int ve_dma_read_ctrl_reg(uint64_t *regs);
#
	.text
	.balign 256
#ifndef __PIC__
	.extern	_vedma_ctrl
#endif

	.global	ve_dma_read_ctrl_reg
	.type	ve_dma_read_ctrl_reg, @function

ve_dma_read_ctrl_reg:
#ifdef __PIC__
	# get vedma_ctrl (s34)
	GET_GOT
	lea		%s34, vedma_ctrl@GOT_LO
	and		%s34, %s34, (32)0
	lea.sl		%s34, vedma_ctrl@GOT_HI(%s34,%s40)
	ld		%s34, (%s34)
	ld		%s34, 0(%s34)
#else
        # get vedma_ctrl (s34)
        lea             %s34, vedma_ctrl@LO
        and             %s34, %s34, (32)0
        lea.sl          %s34, vedma_ctrl@HI(,%s34)
        ld              %s34, (%s34)
#endif

	# read DMA control register
	lhm.l		%s35, 0(%s34)
	lhm.l		%s36, 8(%s34)

	# store register value
	st		%s35, 0(%s0)
	st		%s36, 8(%s0)

	b.l.t		0(, %s10)	# return
