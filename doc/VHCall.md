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

#### VH APIs provided from VE OS to transfer data
A VH library invoked by VH Call can include `"libvepseudo.h"`.
In the header, the following API functions are declared.

- `ve_send_data(veos_handle *handle, uint64_t dst, size_t size, void *src)`
- `ve_recv_data(veos_handle *handle, uint64_t src, size_t size, void *dst)`

The first argument handle is an opaque VE OS handle passed as a library function argument by calling `vhcall_args_set_veoshandle()` on VE side. The second is address of VE virtual memory space.

For VH side, sending data to VE and receiving data from VE are implemented as these APIs.

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


## Two example programs

#### First example

This is a simple program invoking VH shared library functions with passing arguments.

Program invokes VH C library function which has three types arguments, pointer type, int type and float type. The pointer one is used as input and output.

Also it invokes VH Fortran library function and subroutine. For Fortran, all arguments are treated as pointer type.

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
5. Unload the VH C library by `vhcall_uninstall()`.
6. Load a Fortran library as step1.
7. Get a Fortran function symbol ID as step2.
8. Recreate the arguments object for a Fortran function
 by `vhcall_args_clear()` and `vhcall_args_set_pointer()`.
9. Invoke a Fortran function as step4.
10. Get a Fortran subroutine symbol ID as step2.
11. Update 1st argument in arguments object by `vhcall_args_set_pointer()`.
12. Invoke a Fortran subroutine as step4.
13. Free the arguments object by`vhcall_args_free()`.
14. Unload the VH library as step6.


#### Second example
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
 - [examples/vhcall/libvhhello.c](vhcall_2libvhhello_8c-example.html) is VH C library invoked by VH Call for first example
 - [examples/vhcall/libvhhello.f90](vhcall_2libvhhello_8f90-example.html) is VH Fortran library invoked by VH Call for first example
 - [examples/vhcall/sample2.c](vhcall_2sample2_8c-example.html) is VE program using VH Call for second example
 - [examples/vhcall/libdatatransfer.c](vhcall_2libdatatransfer_8c-example.html) is VH C library using VH data transfer API for second example
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

Test for Fortran library
 [VH libvhhello_f.so:hellofunc] 1starg:Hello, This is VE
 [VH libvhhello_f.so:hellofunc] 2ndarg:         100
[VE] buffer: Hello, This is VE
 [VH libvhhello_f.so:hellosubr] 1starg:Hello, This is VE
 [VH libvhhello_f.so:hellosubr] 2ndarg:         100
[VE] buffer: Hello VE, This is VH Fortran library
~~~

For second example

~~~
$ ./sample2
[VE] addr:0x60fffffff208, data:deadbeefbaadcafe
[VH] Success receiving msg! deadbeefbaadcafe
[VH] Success sending msg!   cafebaadbeefdead
[VE] addr:0x60fffffff208, data:cafebaadbeefdead
~~~
@n

@example vhcall/sample1.c
@example vhcall/libvhhello.c
@example vhcall/libvhhello.f90
@example vhcall/sample2.c
@example vhcall/libdatatransfer.c
@example vhcall/Makefile
