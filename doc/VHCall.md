# Getting Started with VH Call
VH call is a mechanism to append functions in shared libraries on Vector Host
(VH) as system calls on Vector Engine (VE).

## Introduction
SX-Aurora TSUBASA provides VH call for invoking functions in shared libraries
on VH from a VE program as system calls.
VH call enables to develop a program using both VE and VH computing resources.
It would be useful for offloading I/O and procedures not well-vectorized.

## Prerequisite
To develop VE programs using VH call, please install libsysve-devel package,
which has libvhcall.h header declaring VH call API functions.

For example, execute the following command as root.
~~~
# yum install libsysve-devel
~~~

If you execute a program using VH call only, no extra packages are required.

## Hello VH Call
In this chapter, a simple "Hello world" like program is introduced.

### Function Prototype
VH call can call a function on VH with the following prototype:
~~~c
long vh_function(veos_handle *handle, const void *inptr, size_t insize,
                 void *outptr, size_t outsize);
~~~
 - `handle`: an opaque VE OS handle, used for data transfer API functions.
 - `inptr`: pointer to input data buffer.
 - `insize`: size of input data buffer.
 - `outptr`: pointer to output data buffer.
 - `outsize`: size of output data buffer.

### VH Code
Code to run on VH is shown below.

~~~c
#include <stdio.h>
long hello(veos_handle *handle, const void *inp, size_t insize,
           void *outptr, size_t outsize)
{
  printf("Hello, world\n");
  return 0;
}
~~~

Save the above code as libvhhello.c.

### Compile VH Code
To execute a function on VH using VH call, compile and link the souce file
into a shared library.

~~~
$ gcc -shared -fpic -o libvhhello.so libvhhello.c
~~~

### VE Code to Use VH Call
A VE program to call the function hello using VH call is shown below.

~~~c
#include <libvhcall.h>
int main()
{
  vhcall_handle handle = vhcall_install("./libvhhello.so");
  int64_t symid = vhcall_find(handle, "hello");
  vhcall_invoke(symid, NULL, 0, NULL, 0);
  return 0;
}
~~~

Save the above code as hello.c.

A program using VH call needs to include "libvhcall.h".
In the header, the following API functions are declared.
- `vhcall_install()` loads a VH shared library.
- `vhcall_find()` searches an address of a symbol a function in the VH library.
- `vhcall_invoke()` invokes a function on VH side.
- `vhcall_uninstall()` unloads a VH shared library.

To execute a VH function with VH call:
1. Load a library by `vhcall_install()`, which returns a VH call handle,
 an identifier to specify the VH library loaded.
2. Get a symbol ID, an identifier of a function in a VH library,
 by `vhcall_find()` with the name of a function and a VH call handle.
3. Invoke a function `vhcall_invoke()`. The function is specified by its
 symbol ID.
4. When the VH library is no longer used, unload the VH library by
 `vhcall_uninstall()` (optional).

### Compile and Run VE Program
Compile source code on VE side as shown below.

~~~
$ /opt/nec/ve/bin/musl-ncc -o hello hello.c
~~~

The VH call API functions are in libsysve library, linked by default.

Execute the VE program.

~~~
$ ./hello
Hello, world
~~~

## Data Transfer
Since VH and VE are different memory address space,
data need to be transferred on call.
VH call provides a mechanism to transfer data for an argument and a result
automatically between VE and VH.
VH call also provides API functions for explicit data transfer.

### Input and Output Arguments
VH call copies input data specified by the second argument from VE to VH on
vhcall_invoke(). The size of input data is specified by the third argument
in bytes.
VH call copies output data specified by the fourth argument from VH to VE on
return from vhcall_invoke(). The size of output data is specified by the
fifth argument in bytes.

For example, to invoke `func()` shown below,
~~~c
long func(type1 arg1, type2 arg2, type3 arg3, struct abc *ret);
~~~

Declare the following structure for input:
~~~
struct func_input {
  type1 arg1;
  type2 arg2;
  type3 arg3;
};
~~~

An example of a wrapper function for VH call is shown below:

~~~
long func_wrapper(veos_handle *handle, void *inptr, size_t insize,
                  void *outptr, size_t outsize)
{
  assert(insize == sizeof(struct func_input));
  assert(outsize == sizeof(struct abc));
  struct func_input *ip = inptr;
  return func(ip->arg1, ip->arg2, ip->arg3, (struct abc *)outptr);
}
~~~

The code above is compiled into libfuncwrapper.so.

From VE side,
~~~
vhcall_handle handle = vhcall_install("libfuncwrapper.so");
int64_t sym = vhcall_find(handle, "func_wrapper");
struct func_input input = {
  .arg1 = arg1,
  .arg2 = arg2,
  .arg3 = arg3,
};
struct abc ret;
vhcall_invoke(sym, &input, sizeof(input), &ret, sizeof(ret));
...
~~~

### API functions on VH
VH call provides API functions to transfer data between VE and VH explicitly.

 - `ve_recv_data(veos_handle *handle, uint64_t src, size_t size, void *dst)`
 - `ve_send_data(veos_handle *handle, uint64_t dst, size_t size, void *src)`

The first argument `handle`is VE OS handle passed from VH call as the first
argument on VH side.

## Return Value from VH
The return value of `vhcall_invoke()` is the return value of VH function
basically. However, when the return value of VH function is
between -4095 and -1, the return value on VE is -1 and `errno` is set.
This is because VH call is implemented as a system call.

For example, a VH function returns -22 (= `-EINVAL`), vhcall_invoke()
on VE side returns -1 and `errno` is set to 22.
