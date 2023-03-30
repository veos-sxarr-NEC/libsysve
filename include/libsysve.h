/* Copyright (C) 2017-2023 by NEC Corporation
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
 * @file libsysve.h
 * @brief Header file for libsysve
 */

#ifndef __VE_LIBSYSVE_H
#define __VE_LIBSYSVE_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <veos_defs.h>

struct proginf_v1 {
	short              version;        /* Always set to PROGINF_VERSION */
	short              arch;           /* Architecuture VE1:0, VE3:1 */
	unsigned int       padding_1[3];
	unsigned long long ac_ex;          /* Execution count */
	unsigned long long ac_vx;          /* Vector execution count */
	unsigned long long ac_fpec;        /* Floating point data element count */
	unsigned long long ac_ve;          /* Vector elements count */
	unsigned long long ac_l1imc;       /* L1 instruction cache miss count */
	unsigned long long ac_vecc;        /* Vector execution in microseconds */
	unsigned long long ac_l1iac;       /* L1 instruction cache access count */
	unsigned long long ac_l1mcc;       /* L1 cache miss in microseconds */
	unsigned long long ac_l2mcc;       /* L2 cache miss in microseconds */
	unsigned long long ac_l1omc;       /* L1 operand cache miss count */
	unsigned long long ac_uxc;         /* User-specified instruction execution count */
	unsigned long long ac_ve2;         /* Vector elements count 2 */
	unsigned long long ac_l1oac;       /* L1 operand cache access count */
        unsigned long long ac_uec;         /* User-specified instruction element count */
	unsigned long long ac_varec;       /* Vector arithmetic execution in microseconds */
	unsigned long long ac_l1imcc;      /* L1 instruction cache miss in microseconds */
	unsigned long long ac_l2mc;        /* L2 cache miss count */
	unsigned long long ac_vldec;       /* Vector load execution in microseconds */
	unsigned long long ac_l1omcc;      /* L1 operand cache miss in microseconds */
	unsigned long long ac_l2ac;        /* L2 cache access count  */
	unsigned long long ac_pccc;        /* Port conflict in microseconds */
	unsigned long long ac_ltrc;        /* Load instruction traffic count */
	unsigned long long ac_brec;        /* Branch execution count */
        unsigned long long ac_sarcc;       /* Shared resource access clock count in microseconds */
	unsigned long long ac_vlpc;        /* Vector load delayed count */
	unsigned long long ac_strc;        /* Store instruction traffic count */
	unsigned long long ac_bpfc;        /* Branch prediction failure count */
	unsigned long long ac_iphcc;       /* Instruction pipeline hold clock count in microseconds */
	unsigned long long ac_vlec;        /* Vector load element count */
	unsigned long long ac_vlxc;        /* Vector load instruction execution count */
	unsigned long long ac_llvml;       /* LLC vector load cache fill line count */
	unsigned long long ac_llvme;       /* LLC vector load cache miss element count */
	unsigned long long ac_llvxc;       /* LLC vector load cache miss instruction 
					      execution count */
	unsigned long long ac_llvxc2;      /* LLC vector load cache miss/miss 
					      on fill instruction execution count */
	unsigned long long ac_fmaec;       /* Fused multiply add element count */
	unsigned long long ac_fmaxc;       /* Fused multiply add execution count */
	unsigned long long ac_ptcc;        /* Power throttling in microseconds */
	unsigned long long ac_ttcc;        /* Thermal throttling in microseconds */
	unsigned long long ac_corebitmap;  /* core bitmap */
	unsigned long long ac_max_nthread; /* Maximum number of threads whose
					      state are "RUNNING" or "WAIT" at
					      the same time */
	unsigned long long ac_l3vsac;      /* VLD+SLD elements accessing L3 count */
	unsigned long long ac_l3vsme;      /* L3 VLD+SLD miss-hit element count */
	unsigned long long ac_l3vsml;      /* L3 VLD+SLD miss-hit cache line count */
	unsigned long long ac_llvsme;      /* LLC miss-hit element count */
	unsigned long long ac_llvsml;      /* LLC miss-hit cache line count */
	unsigned long long padding_2[2];
};

int ve_get_pci_sync(uint8_t, uint64_t *, uint64_t *);
ssize_t ve_get_ve_info(char *, char *, size_t);
int ve_set_user_reg(uint64_t, uint64_t);
int ve_get_fixed_vehva(uint64_t, uint64_t *);
int ve_get_numa_node(unsigned *);
int ve_get_veos_pid(void);
int ve_get_nonswappable(uint64_t *);
int ve_get_vemva_region(uint64_t *, size_t *, int64_t *);
int ve_is_acc_io_enabled(void);
int ve_get_ve_product_name(char *, size_t);
int get_proginf_data(int, void *);

#endif
