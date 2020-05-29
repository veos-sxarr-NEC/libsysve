# Getting Started with VH Call
VH call is a mechanism to append functions in shared libraries on Vector Host
(VH) as system calls on Vector Engine (VE).

## Introduction
SX-Aurora TSUBASA provides VH call for invoking functions in shared libraries
on VH from a VE program as system calls.
VH call enables to develop a program using both VE and VH computing resources.
It would be useful for offloading I/O and procedures not well-vectorized.

## Prerequisite

#### For using VE APIs
To develop VE programs using VH call, please install libsysve-devel package,
which has libvhcall.h header declaring VH call API functions.

For example, execute the following command as root.
~~~
# yum install libsysve-devel
~~~

If you are using musl-libc as C library, APIs introduced here are not supported.

If you execute a program using VH call only, no extra packages are required.

#### For using VH APIs
To develop VH library invoked by VH Call, veos-devel package is available.

For example, execute the following command as root.
~~~
# yum install veos-devel
~~~

## APIs for getting started sample programs

#### VE APIs of VH Call to invoke function with passing arguments
A VE program using VH Call needs to include `"libvhcall.h"`.
In the header, the following API functions are declared.

- `vhcall_install()` loads a VH shared library.
- `vhcall_find()` searches an address of a symbol a function in the VH library.
- `vhcall_args_alloc()` allocates VHCall arguments object (vhcall_args).
- `vhcall_args_set_i8()` sets a 8-bit signed integer argument.
- `vhcall_args_set_u8()` sets a 8-bit unsigned integer argument.
- `vhcall_args_set_i16()` sets a 16-bit signed integer argument.
- `vhcall_args_set_u16()` sets a 16-bit unsigned integer argument.
- `vhcall_args_set_i32()` sets a 32-bit signed integer argument.
- `vhcall_args_set_u32()` sets a 32-bit unsigned integer argument.
- `vhcall_args_set_i64()` sets a 64-bit signed integer argument.
- `vhcall_args_set_u64()` sets a 64-bit unsigned integer argument.
- `vhcall_args_set_float()` sets a single precision floating point
                            number argument.
- `vhcall_args_set_double()` sets a double precision floating point
                             number argument.
- `vhcall_args_set_complex_float()` sets a single precision floating point
                                    complex object argument.
- `vhcall_args_set_complex_double()` sets a double precision floating point
                                     complex object argument.
- `vhcall_args_set_pointer()` sets VH function argument of pointer type.
- `vhcall_args_set_veoshandle()` sets VH function argument of pointer to
                                 VEOS handle.
- `vhcall_args_clear()` clears arguments set in VHCall arguments object.
- `vhcall_args_free()` frees VHCall arguments object.
- `vhcall_invoke_with_args()` invokes a function on VH side with passing arguments.
- `vhcall_uninstall()` unloads a VH shared library.

For VE side, sending data to VH is implemented as API of passing arguments. Receiving data from VH is also implemented as return value or pointer type argument which INTENT is OUT or INOUT. Please see [VH Call](group__vhcall.html#details) for more detail.

Any arguments passed to VH Fortran program is set by only using vhcall_args_set_pointer(), i.e. they are passed by reference.

#### VH APIs provided from VE OS to transfer data
A VH library invoked by VH Call can include `"libvepseudo.h"`.
In the header, the following API functions are declared.

#### Data transfer to caller VE thread from callee VH library function

- `ve_send_data(veos_handle *handle, uint64_t dst, size_t size, void *src)`
- `ve_recv_data(veos_handle *handle, uint64_t src, size_t size, void *dst)`

The first argument handle is an opaque VE OS handle passed as a library function argument by calling `vhcall_args_set_veoshandle()` on VE side. The second is address of VE virtual memory space.


#### VH shared library Function Prototype
VH call can call a function on VH with the following prototype:
~~~c
uint64_t vh_function(<T>, ...);
~~~
Argument type <T> is allowed to be following variation:
 - 8, 16, 32, 64 bit signed/unsigned integer type
 - (complex) single/double precision floating point type
 - pointer type
 - an opaque VE OS handle, used for data transfer API functions.


## Three example programs

#### First example

This is a simple program invoking VH shared library functions with passing arguments.

Program invokes VH C library function which has three types arguments, pointer type, int type and float type. The pointer one is used as input and output.


Steps of programs are below.
1. Load a VH C library by `vhcall_install()`, which returns a VH call handle,
 an identifier to specify the VH library loaded.
2. Get a symbol ID, an identifier of a function in a VH library,
 by `vhcall_find()` with the name of a function and a VH call handle.
3. Create an arguments object for a function in a VH library
 by `vhcall_args_alloc()` and `vhcall_args_set_<T>()`
4. Invoke a function `vhcall_invoke_with_args()`.
  - The function is specified by its symbol ID of 1st argument.
  - The arguments are passed as an arguments object of 2nd argument.
  - The return values is got as pointer of 3rd argument.
5. Free the arguments object by`vhcall_args_free()`.
6. Unload the VH library by `vhcall_uninstall()`.


#### Second example

This invokes VH Fortran library function and subroutine. For Fortran, all arguments are treated as pointer type.

All arguments of VH Fortran function requires to be set by vhcall_args_set_pointer from VE C program.

1. Load a VH Fortran library as step1 of first sample.
2. Get a Fortran function symbol ID as step2 of first sample.
3. Create the arguments object for a Fortran function
 by `vhcall_args_alloc()` and `vhcall_args_set_pointer()`.
4. Invoke a Fortran function as step4 of first sample.
5. Get a Fortran subroutine symbol ID as step2 of first sample.
6. Update 1st argument in arguments object by `vhcall_args_set_pointer()`.
7. Invoke a Fortran subroutine as step4 of first sample.
8. Free the arguments object by`vhcall_args_free()`.
9. Unload the VH library as step6 first sample.


#### Third example
This is simple program using `ve_recv_data()` and `ve_send_data()`.
Steps of programs are below.

1. Load a VH C library and get a symbol ID.
2. Create arguments object.
  - Set VE OS handle to 1st argument by `vhcall_args_set_veoshandle()`
  - Set VE virtual address to 2nd argument by `vhcall_args_set_u64()`
3. Invoke a VH function.
4. VH side get above two arguments and calls data transfer API with them.

#### Files

 - [examples/vhcall/sample1.c](vhcall_2sample1_8c-example.html) is VE program using VH Call for first example
 - [examples/vhcall/sample2.c](vhcall_2sample2_8c-example.html) is VE program using VH Call for second example
 - [examples/vhcall/libvhhello.c](vhcall_2libvhhello_8c-example.html) is VH C library invoked by VH Call for first example
 - [examples/vhcall/libvhhello.f90](vhcall_2libvhhello_8f90-example.html) is VH Fortran library invoked by VH Call for first example
 - [examples/vhcall/sample3.c](vhcall_2sample3_8c-example.html) is VE program using VH Call for third example
 - [examples/vhcall/libdatatransfer.c](vhcall_2libdatatransfer_8c-example.html) is VH C library using VH data transfer API for third example
 - [examples/vhcall/Makefile](vhcall_2Makefile-example.html) builds programs

#### Build programs

Put all files of this sample in the same directory.

Build the programs.
~~~
$ make
~~~

#### Execute programs

For first example

~~~
$ ./sample1
Test for C library
[VE] buffer: Hello, This is VE
[VH libvhhello.so:hello] 1starg: Hello, This is VE
[VH libvhhello.so:hello] 2ndarg: -1
[VH libvhhello.so:hello] 3rdarg: -1.111000
[VE] buffer: Hello VE, This is VH C library
~~~

For second example

~~~
$ ./sample2

Test for Fortran library

Invoke function passing string
 [VH libvhhello_f.so:hellofunc] 1starg:Hello, This is VE
 [VH libvhhello_f.so:hellofunc] 2ndarg:         100

Invoke subroutine passing matrix of double
[VE] a[0][0]=0.0000000000000000
[VE] a[0][1]=1.0000000000000000
[VE] a[0][2]=2.0000000000000000
[VE] a[1][0]=10.0000000000000000
[VE] a[1][1]=11.0000000000000000
[VE] a[1][2]=12.0000000000000000
[VE] a[2][0]=20.0000000000000000
[VE] a[2][1]=21.0000000000000000
[VE] a[2][2]=22.0000000000000000
 [VH] a(           1 ,           1 )=   0.0000000000000000
 [VH] a(           1 ,           2 )=   10.000000000000000
 [VH] a(           1 ,           3 )=   20.000000000000000
 [VH] a(           2 ,           1 )=   1.0000000000000000
 [VH] a(           2 ,           2 )=   11.000000000000000
 [VH] a(           2 ,           3 )=   21.000000000000000
 [VH] a(           3 ,           1 )=   2.0000000000000000
 [VH] a(           3 ,           2 )=   12.000000000000000
 [VH] a(           3 ,           3 )=   22.000000000000000
~~~

For third example

~~~
$ ./sample3
[VE] addr:0x60fffffff208, data:deadbeefbaadcafe
[VH] Success receiving msg! deadbeefbaadcafe
[VH] Success sending msg!   cafebaadbeefdead
[VE] addr:0x60fffffff208, data:cafebaadbeefdead
~~~

## Additional API introduction

#### Data transfer to caller VE thread from VH thread created by callee VH library function
One way to implement asynchronous VH Call, callee VH library function creates worker threads and these threads do their works with transferring data with caller VE thread. But such VH threads don't have their VEOS handler.
So they have to copy VEOS handler from parent's to transfer data with VE.

- `veos_handler_copy(veos_handler *handle)`
- `veos_handler_free(veos_handler *handle)`

Normally, caller VE thread and callee VH library function have same thread ID
and data transfer APIs don't have to take care of them.
But VH thread created by callee VH library function gets different thread ID,
and data transfer APIs have to take care of them to find caller VE thread.

- `ve_send_data_tid(veos_handle *handle, uint64_t dst, size_t size, void *src, pid_t tid)`
- `ve_recv_data_tid(veos_handle *handle, uint64_t src, size_t size, void *dst, pid_t tid)`

The third argument tid is VE thread ID which has invoked VH Call.
Then, these APIs can be used from worker thread created by VH library function of VH Call.

@n

@example vhcall/sample1.c
@example vhcall/sample2.c
@example vhcall/libvhhello.c
@example vhcall/libvhhello.f90
@example vhcall/sample3.c
@example vhcall/libdatatransfer.c
@example vhcall/Makefile
