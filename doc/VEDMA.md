# Getting Started with VE DMA and VH-VE SHM
VE DMA is a feature which registers VE memory to DMAATB, and transfer data by DMA between the memories registered in DMAATB.

VH-VE SHM is a feature for VE programs to register System V shared memory created at VH side to DMAATB of VE. 

So, VE programs can use VE DMA and VH-VE SHM to transfer data by DMA between System V shared memory and VE memory registered to DMAATB.  

## Introduction
SX-Aurora TSUBASA provides VE DMA functions that enables VE programs to transfer data by DMA between memories.

A source and a destination of DMA data transfer are specified by VE host virtual address.
To obtain VE host virtual address, you need to register memory to DMAATB.

A source and a destination of DMA data transfer need to be aligned on a 4 byte boundary.
Data transfer size needs to be less than 128MB. 

Please see [VE DMA](group__vedma.html#details) for more detail.

SX-Aurora TSUBASA provides VH-VE SHM functions that enables VE programs to register System V shared memory created at VH side to DMAATB of VE.

VH-VE SHM feature does not provide a function to create System V shared memory. A user program at VH side needs to create System V shared memory. 

Please see [VH-VE SHM](group__vhshm.html#details) for more detail.

By combining VE DMA and VH-VE SHM, data communication between VE program and VH program is possible via System V shared memory on VH.

## Prerequisite
To develop VE programs using VE DMA and VH-VE SHM, please install libsysve-devel package, 
which has "vedma.h" header delcaring VE DMA API functions, "vhshm.h" header delcaring VH-VE SHM API functions

For example, execute the following command as root.
~~~
# yum install libsysve-devel
~~~
System V shared memory will be created with SHM_HUGETLB flag to allocate huge pages (2MB pages).
The kernel parameter "vm.nr_hugepages" needs to be set.
For example, the sample program in this page creates a 2 MB system V shared memory which consists of huge pages. So, "vm.nr_hugepages" needs to be set to 1 or more.
If the value of "vm.nr_hugepages" is already more than zero, other software might use huge pages. In this case, please add the required number of huge pages(1) to the value of "vm.nr_hugepages".

Please see [VH-VE SHM](group__vhshm.html#details) for more detail.

## APIs of VE DMA
A VE program using VE DMA needs to include "vedma.h".
Please specify "-lveio -lpthread" option to the compiler driver to link libveio,

In the header, the following API functions are declared.
- `ve_dma_init()` Initializes VE DMA feature.
- `ve_dma_post()` Issues asynchronous DMA.
- `ve_dma_poll()` Inquiries the completion of asynchronous DMA.
- `ve_dma_post_wait()` Issues synchronous DMA.
- `ve_dma_read_ctrl_reg()` Gets the value of DMA Control Register.
- `ve_register_mem_to_dmaatb()`  Registers VE local memory to DMAATB.
- `ve_unregister_mem_from_dmaatb()`  Unregisters VE local memory from DMAATB.

## APIs of VH-VE SHM
A VE program using VH-VE SHM needs to include "vhshm.h".

In the header, the following API functions are declared.
- `vh_shmget()` Gets the identifier of system V shared memory on VH.
- `vh_shmat()` Attaches system V shared memory on VH and register it with DMAATB.
- `vh_shmdt()` Detaches system V shared memory on VH and releases DMAATB entry.

## Example programs
These are simple example programs which transfer data between VE program and VH program via System V shared memory on VH by combining VE DMA and VH-VE SHM. The example programs consist of a main VE program and helper x86 programs.

Steps of the VE program are below.

1. Get the identifier of system V shared memory on VH. The size is 2MB as an example.
2. Attach the system V shared memory and register it to DMAATB on VE.
3. Initialize VE DMA.
4. Allocate VE memory. The size is 4 bytes as an example.
5. Register VE memory to DMAATB. The program calculates the address and the size to be aligned with 64MB boundary (page boundary).
6. Post DMA to read the system V shared memory. The size is 4 bytes as an example.
7. Post DMA to write the system V shared memory. The size is 4 bytes as an example.
8. Detach the system V shared memory.
9. Unregister VE memory from DMAATB.

## Files

 - [examples/vedma/dma.c](vedma_2dma_8c-example.html) transfers data using VE DMA (VE program)
 - [examples/vedma/mkshm.c](vedma_2mkshm_8c-example.html) creates System V shared memory on VH (x86 program)
 - [examples/vedma/rdshm.c](vedma_2rdshm_8c-example.html) prints data on System V shared memory (x86 program)
 - [examples/vedma/Makefile](vedma_2Makefile-example.html) builds programs


## Compile and execute the programs

Put all files of this sample in the same directory.

Command executed by the root user starts with "#".

Build the programs.
~~~
$ make
~~~
Check the current number of huge pages on VH.
~~~
$ sysctl vm.nr_hugepages
~~~

Set the number of huge pages on VH.
This sample creates a 2 MB system V shared memory. 1 huge pages are required because the size of a huge page is 2MB.
Add 1 to the current number of huge pages. This value will be reset after reboot. To set the number of huge pages permanently, see [VH-VE SHM](group__vhshm.html#details).
~~~
# sysctl -w vm.nr_hugepages=[page number]
~~~
Check the "HugePages_Free" is 1 or more.
If "HugePages_Free" is less than 1, please set "vm.nr_hugepages" to a more larger value as in the previous step.
~~~
$ cat /proc/meminfo | grep HugePages_Free
~~~
Execute mkshm.x86 to create system V shared memory on VH.
~~~
$ ./mkshm.x86
~~~
Show system V shared memory.
~~~
$ ipcs -m
~~~
You will see the created system V shared memory with key=0x19761215

Execute ./dma.ve to transfer data using VE DMA.

~~~
$ ./dma.ve
~~~
Execute ./rdshm.x86 to print data on system V shared memory.

~~~
$ ./rdshm.x86
~~~
Run ipcrm command to remove the system V shared memory.
~~~
# ipcrm -M 0x19761215
~~~
@n

@example vedma/dma.c
@example vedma/mkshm.c
@example vedma/rdshm.c
@example vedma/Makefile
