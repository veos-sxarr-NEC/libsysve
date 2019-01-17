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

.section	.data
.set max_fg,    255     #MAXIMUM FLAG use in ts1am.l instruction

# int __libsysve_a_swap(volatile int *Sz, int Sx)
# For ts1am.w, the address indicated by pointer *Sz must be
# aligned to 4-byte boundary.
# implementation: ts1am.w newval,pointer,cond_fg
#	newval(Sx)	: value stored after swap, at address pointed by Sz
#	pointer(Sz)	: address from where old value is to be swapped and
#			  new value is to be stored.
#	cond_fg(Sy)	: condition bits Sy[60:63] to perform swap operation
#			  on memory with 4-byte data size of Sx.
#	Old ← M(Sz, 4)
#	for (i = 0 to 3)
#		if (Sy[60+i] = 1) {M(Sz+3-i) ← Sx[8*(i+4):8*(i+4)+7]}
#	Sx[32:63] ← Old
#	Sx[0:31] ← 00…0
.text
.global __libsysve_a_swap
.type	__libsysve_a_swap,@function
__libsysve_a_swap:
	fencem 1		# FENCE(ST=1)
	ts1am.w	%s1, %s0,15
	fencem 2		# FENCE(LF=1)
	or	%s0, %s1, (0)1
	beq.l.t 0, 0x0(,%s10)

# void a_spin()	NOT IMPLEMENTED
# void a_crash() NOT IMPLEMENTED
