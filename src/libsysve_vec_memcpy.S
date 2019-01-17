#
# Copyright (C) 2017 NEC Corporation.
#

# void *__libsysve_vec_memcpy(void *dest, void *src, size_t n)

	.text
	.balign	16
	.globl	__libsysve_vec_memcpy
	.type	__libsysve_vec_memcpy,@function
__libsysve_vec_memcpy:
# prologue
	st	%fp,0x0(,%sp)
	st	%lr,0x8(,%sp)
	st	%got,0x18(,%sp)
	st	%plt,0x20(,%sp)
	or	%fp,0,%sp
.L_EoP:
	brgt.l	40,%s2,.L_scalar	# if n<40, call memcpy
	or	%s63,0,%s0		# p = dest
	or	%s62,0,%s1		# q = src
	or	%s61,0,%s2		# i = n
	and	%s55,%s0,(61)0		# p & 0x07
	and	%s54,%s1,(61)0		# q & 0x07
 	brne.l	%s55,%s54,.L_4byte	# (p&0x7) != (q&0x7)
	breq.l	%s55,0,.L_8vector	# (p&0x7) == 0
.L_pre8:
	ld1b	%s60,(,%s62)
	st1b	%s60,(,%s63)		# *p = *q
	addu.l	%s62,0x1,%s62		# q++
	addu.l	%s63,0x1,%s63		# p++
	subu.l	%s61,%s61,(63)0		# n--
	and	%s59,%s63,(61)0		# p&0x7
	brne.l	%s59,0,.L_pre8
.L_8vector:
	smvl	%s57			# MVL
	sla.l	%s56,%s57,3		# mvl * 8
	brlt.l	%s61,%s56,.L_rem8_1
	lvl	%s57
.L_loop8:
	vld	%v63,8,%s62
	vst	%v63,8,%s63
	subu.l	%s61,%s61,%s56		# n -= mvl*8
	addu.l	%s63,%s63,%s56		# p += mvl*8
	addu.l	%s62,%s62,%s56		# q += mvl*8
	brge.l	%s61,%s56,.L_loop8
.L_rem8_1:
	sra.l	%s58,%s61,3		# n/8
	breq.l	%s58,0,.L_rem8_2
	lvl	%s58
	vld	%v63,8,%s62
	vst	%v63,8,%s63
	sla.l	%s58,%s58,3
	addu.l	%s63,%s63,%s58
	addu.l	%s62,%s62,%s58
	subu.l	%s61,%s61,%s58
.L_rem8_2:
	and	%s58,%s61,(61)0
	breq.l	%s58,0,.L_epilogue	# if ((n&0x7)==0)
.L_post8:
	ld1b	%s53,(,%s62)
	st1b	%s53,(,%s63)
	addu.l	%s62,0x1,%s62
	addu.l	%s63,0x1,%s63
	subu.l	%s61,%s61,(63)0
	brgt.l	%s61,0,.L_post8
	br.l.t	.L_epilogue
.L_4byte:
	and	%s55,%s0,(62)0		# p&0x03
	and	%s54,%s1,(62)0		# q&0x03
	brne.l	%s55,%s54,.L_scalar	# (p&0x3) != (q&0x3)
	breq.l	%s55,0,.L_4vector	# (p&0x3)==0
.L_pre4:
	ld1b	%s60,(,%s62)
	st1b	%s60,(,%s63)
	addu.l	%s62,0x1,%s62
	addu.l	%s63,0x1,%s63
	subu.l	%s61,%s61,(63)0		# n--
	and	%s59,%s63,(62)0		# p&0x3
	brne.l	%s59,0,.L_pre4
.L_4vector:
	smvl	%s57
	sla.l	%s56,%s57,2		# MVL*4
	brlt.l	%s61,%s56,.L_rem4_1
	lvl	%s57
.L_loop4:
	vldl	%v63,4,%s62
	vstl	%v63,4,%s63
	subu.l	%s61,%s61,%s56		# n -= MVL*4
	addu.l	%s62,%s62,%s56		# p += MVL*4
	addu.l	%s63,%s63,%s56		# q += MVL*4
	brge.l	%s61,%s56,.L_loop4
.L_rem4_1:
	sra.l	%s58,%s61,2		# n/4
	breq.l	%s58,0,.L_rem4_2
	lvl	%s58
	vldl	%v63,4,%s62
	vstl	%v63,4,%s63
	sla.l	%s58,%s58,2
	addu.l	%s63,%s63,%s58
	addu.l	%s62,%s62,%s58
	subu.l	%s61,%s61,%s58
.L_rem4_2:
	and	%s58,%s61,(62)0
	breq.l	%s58,0,.L_epilogue
.L_post4:
	ld1b	%s53,(,%s62)
	st1b	%s53,(,%s63)
	addu.l	%s62,0x1,%s62
	addu.l	%s63,0x1,%s63
	subu.l	%s61,%s61,(63)0
	brgt.l	%s61,0,.L_post4
	br.l.t	.L_epilogue
.L_scalar:
	or	%s63,0,%s0		# p = dest
	or	%s62,0,%s1		# q = src
	or	%s61,0,%s2		# n
	and	%s59,%s61,(61)1
	breq.l	%s59,0,.L_rem8_2
.L_scalar_loop:
	ld	%s58,(,%s62)
	st	%s58,(,%s63)
	addu.l	%s62,8,%s62
	addu.l	%s63,8,%s63
	adds.l	%s61,-8,%s61
	and	%s59,%s61,(61)1
	brne.l	%s59,0,.L_scalar_loop
	and	%s59,%s61,(61)0
	brne.l	%s59,0,.L_post8
.L_epilogue:
	or	%sp,0,%fp
	ld	%got,0x18(,%sp)
	ld	%plt,0x20(,%sp)
	ld	%lr,0x8(,%sp)
	ld	%fp,0x0(,%sp)
	b.l	(,%lr)
	.size	__libsysve_vec_memcpy,	.-__libsysve_vec_memcpy

