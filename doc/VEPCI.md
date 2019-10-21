# Getting Started with VE PCI
VE PCI is a API for make PCI device access to VE memory or be accessed from VE core. This is experimental feature of VEOS.

## Introduction
SX-Aurora TSUBASA provides experimental function that enables VE programs to do following:
- VE programs can register own memory to PCI ATB. This function makes PCI devices access to VE memory.
- VE programs can unregister entry form PCI ATB. If VE program terminates without calling this function, registered memory to PCI ATB remains until VEOS is restarted.
- VE programs can map arbitrary PCI address to own virtual address called VE Host Virtual Address(VEHVA). This function enables VE program to access PCI device through VEHVA.
- VE programs can load/store 64bit size data from/to PCI device specified by VEHVA.
- VE programs can unmap VEHVA. If VE program terminates without calling this function, VEHVA is freed automatically because VEHVA space is assigned per a VE program.

## Prerequisite
To develop VE programs using VE PCI please install libsysve-devel package, which has vepci.h header declearing VE PCI API functions.

For example, execute the following command as root.
~~~
# yum install libsysve-devel
~~~

If you execute a program using VE PCI only, no extra packages are required.

## APIs of VE PCI
A VE program using VE PCI needs to include "vepci.h"
In the header, the following API functions are declared.
- `ve_register_mem_to_pci()` register VE memory to PCI ATB.
- `ve_unregister_mem_from_pci()` unregsiter VE memory mapped to PCI ATB.
- `ve_register_pci_to_vehva()` register PCI address to VEHVA.
- `ve_pci_load64()` load 64bit data from PCI address mapped VEHVA.
- `ve_pci_recv_sync()` synchronize PCI device and VE core.
- `ve_pci_store64()` store 64bit data to PCI address mapped VEHVA.
- `ve_unregister_pci_from()` unregister PCI address from VEHVA.

Basic use of VE PCI.
1. Register own VE memory to PCI ATB entry which can be accessed by PCI device.
2. Check PCI BAR address of device and register it to VEHVA.
3. Communicate with device through VEHVA and VE memory registered to PCI ATB.
4. No need to communicate any more, unregister VE memory from PCI ATB and unmap PCI address from VEHVA.

## VE code Using VE PCI
A sample program with VE PCI is following.
This sample requires two VE nodes. One VE is used as PCI device by registering own VE memory to PCI ATB. Another VE maps the PCI ATB entry address to VEHVA and communicates between two VEs. Following program is for VE used as PCI device.
~~~c
/* sample1.c */
#include <stdio.h>
#include <stdlib.h>
#include <vepci.h>
#include <sys/mman.h>
#include <unistd.h>

#define PCIATB_PAGESIZE (1UL << 26)
#define TIMEOUT 100

int main()
{
	int i;
	int time = 0;
	int ret = -1;
	ssize_t size = PCIATB_PAGESIZE;
	void *vemva;
	uint64_t flag_data = 0xbeefbeefbeefbeef;
	uint64_t vehva, pci_vhsaa;

	/* allocate memory by malloc() */
	vemva = malloc(2*size);
	if (NULL == vemva) {
		perror("Fail to allocate memory");
		return 1;
	}
	/* align memory */
	vemva = (void *)(((uint64_t)vemva + size) & ~(size - 1));

	/* If you want to allocate aligned memory, use mmap() following */
	/*
	vemva = mmap(NULL, size, PROT_READ|PROT_WRITE,
			MAP_ANONYMOUS|MAP_SHARED|MAP_64MB, -1, 0);
	if (NULL == vemva) {
		perror("Fail to allocate memory");
		return 1;
	}
	*/

	/* register own VE memory to PCI ATB */
	pci_vhsaa = ve_register_mem_to_pci(vemva, size);
	if (pci_vhsaa == (uint64_t)-1) {
		perror("Fail to ve_register_mem_to_pci()");
		return 1;
	}

	printf("vemva:0x%lx is registered PCI ATB entry as vhsaa:0x%lx\n",
			vemva, pci_vhsaa);

	/* write data to be read by another VE */
	for (i=1; i<size/8; i++)
		*(uint64_t *)((char *)vemva + i*8) = 0xdeadbeefbaadcafe;

	/* write flag after writing data */
	*(uint64_t *)vemva = flag_data;

	/* check and wait for another VE replying */
	while(*((uint64_t *)vemva) == flag_data && time < TIMEOUT){
		sleep(10);
		printf(".");
		fflush(stdout);
		time++;
	}
	printf("\n");
	if (time >= TIMEOUT) {
                printf("timeout\n");
                return 1;
        }
	printf("get message from another VE: 0x%lx\n",
			*((uint64_t *)vemva));

	/* unregister own VE memory from PCI ATB */
	ret = ve_unregister_mem_from_pci(pci_vhsaa, size);
	if (ret) {
		perror("Fail to ve_unregister_mem_from_pci()");
		return 1;
	}

	return 0;
}
~~~
Then, program for another VE node is following.
~~~c
/* sample2.c */
#include <stdio.h>
#include <stdlib.h>
#include <vepci.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define SIZE_64M (1UL << 26)
#define TIMEOUT 100

uint64_t cto_ui64(char c)
{
	switch(c) {
	case '0': return 0x0;
	case '1': return 0x1;
	case '2': return 0x2;
	case '3': return 0x3;
	case '4': return 0x4;
	case '5': return 0x5;
	case '6': return 0x6;
	case '7': return 0x7;
	case '8': return 0x8;
	case '9': return 0x9;
	case 'a': return 0xa;
	case 'b': return 0xb;
	case 'c': return 0xc;
	case 'd': return 0xd;
	case 'e': return 0xe;
	case 'f': return 0xf;
	default:
		printf("Invalid argument of main: %c\n", c);
		exit(1);
	}
}

uint64_t ato_ui64(char *str)
{
	int i;
	uint64_t ret=0;

	for(i=0; i<strlen(str); i++)
		ret = ret*0x10 + cto_ui64(str[i]);

	return ret;
}

int main(int argc, void *argv[])
{
	int i;
	int time = 0;
	int ret = -1;
	uint64_t vehva;
	uint64_t pci_vhsaa;
	size_t size = SIZE_64M;
	uint64_t check_flag = 0xbeefbeefbeefbeef;
	uint64_t check_data = 0xdeadbeefbaadcafe;
	uint64_t reply_data = 0xbaadbaadbaadbaad;
	uint64_t load_data;

	if (argc != 2) {
		printf("Invalid argument: %d\n", argc);
		return 1;
	}

	/* get PCI ATB entry address from another VE node */
	pci_vhsaa = ato_ui64(argv[1]);

	/* map PCI ATB entry to VEHVA */
	vehva = ve_register_pci_to_vehva(pci_vhsaa, size);
	if (vehva == (uint64_t)-1) {
		perror("Fail to ve_register_pci_to_vehva()");
		return 1;
	}

	/* at first, read flag set by another VE node */
	do {
		load_data = ve_pci_load64(vehva);
		printf("\rread flag: %lx", load_data);
		fflush(stdout);
		time++;
		sleep(2);
	} while (load_data != check_flag && time < TIMEOUT);
	if (time >= TIMEOUT) {
		printf("timeout\n");
		return 1;
	}
	printf("\rread flag correctly        \n");

	/* synchronization */
	ve_pci_recv_sync();

	/* read data */
	for (i=1; i<size/8; i++) {
		load_data = ve_pci_load64(vehva + i * 8);
		if (load_data != check_data) {
			printf("data(offset: 0x%d) is not collect 0x%lx\n",
					i * 8, load_data);
			return 1;
		}
	}
	printf("read data correctly\n");

	/* reply to another VE node */
	ve_pci_store64(vehva, reply_data);
	printf("reply message to another VE: 0x%lx\n", reply_data);

	/* unmap PCI ATB entry from VEHVA */
	ret = ve_unregister_pci_from_vehva(vehva, size);
	if (ret) {
		perror("Fail to ve_unregister_pci_to_vehva");
		return 1;
	}

	return 0;
}
~~~

## Compile and execute the VE program
Prepare two terminal for each sample program.

For sample1.c, execute test and check "vhsaa" address
~~~c
$ /opt/nec/ve/bin/ncc -o sample1 sample1.c
$ VE_NODE_NUMBER=0 ./sample1
vemva:0x7fb000000000 is registered PCI ATB entry as vhsaa:0x1004000000
got message from another VE: 0xbaadbaadbaadbaad
~~~
For sample2.c, execute test with "vhsaa" address
~~~c
$ /opt/nec/ve/bin/ncc -o sample2 sample2.c
$ sudo VE_NODE_NUMBER=1 ./sample2 1004000000
read flag correctly
read data correctly
reply message to another VE: 0xbaadbaadbaadbaad
~~~

