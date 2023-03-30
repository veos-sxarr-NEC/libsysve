# Getting Started with VE AIO
Asynchronous I/O operation for VE (VE AIO) enables to develop VE programs which can do their own tasks during data transfer between VE and VH, and actual I/O.

## Introduction
SX-Aurora TSUBASA provides VE AIO function that enables VE programs to do following:
- VE programs can request to read or write an ordinary file.
- VE programs can do their own tasks while data transfer between VE and VH, and actual I/O are done asynchronously.

The files must be opened without O_DIRECT flag.
It cannot request to read or write an ordinary file opened with O_DIRECT flag.
It cannot also request to read or write socket and pipe.

## Prerequisite
To develop VE programs using VE AIO, please install libsysve-devel package, which has veaio.h header delcaring VE AIO API functions.

For example, execute the forllowing command as root.
On ve1
~~~
# yum install libsysve-ve1-devel
~~~
On ve3
~~~
# yum install libsysve-ve3-devel
~~~

If you execute a program using VE AIO only, no extra packages are required.

## APIs of VE AIO
A VE program using VE AIO needs to include "veaio.h".
In the header, the following API functions are declared.
- `ve_aio_init()` initializes and returns new context for VE AIO.
- `ve_aio_fini()` release the context for VE AIO.
- `ve_aio_read()` starts asynchronous read operation for VE.
- `ve_aio_write()` starts asynchronous write operation for VE.
- `ve_aio_query()` gets state of read/write operation. If state is complete, result of read/write request can be got.
- `ve_aio_wait()`  waits and gets result of read/write request.

Basic use of VE AIO read/write is following steps.
1. Initialize VE AIO context.
2. Start VE AIO read/write operation for VE AIO context.
3. Do something with checking whether the VE AIO state is cpmpleted.
4. If it is not completed after doing something, wait the completion of VE AIO operation.
5. Release VE AIO context after VE AIO operation completes.

## VE Code Using VE AIO
A simple program with VE AIO reading is following.
~~~c
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <veaio.h>

#define BUF_SIZE (1024*1024)

ssize_t do_something_and_reading(int fd, uint64_t *rbuf,
					size_t size) {
	struct ve_aio_ctx *ctx;
	int errnoval, ret, i;
	ssize_t retval;

	/* Initialize and get VE AIO context */
	ctx = ve_aio_init();
	if (ctx == NULL) {
		perror("ve_aio_init");
		return -1;
	}

	/* Start asynchronous read for context */
	ret = ve_aio_read(ctx, fd, size, rbuf, 0);
	if (ret == -1) {
		perror("ve_aio_read");
		retval = -1;
		goto completed;
	}

	for (i = 0; i < 10; i++) {
		/* Do something */

		/* Check state of asynchronous read */
		ret = ve_aio_query(ctx, &retval, &errnoval);
		if (ret == 0) {
			goto completed;
		} else if (ret == -1) {
			perror("ve_aio_query");
			retval = -1;
			goto completed;
		}
	}

	/* Wait completion of asynchronous read if it is not completed yet */
	ret = ve_aio_wait(ctx, &retval, &errnoval);
	if (ret == -1) {
		perror("ve_aio_wait");
		retval = -1;
	}
completed:
	ve_aio_fini(ctx);
	return retval;
}

int main (void) {
	int i;
	int fd;
	int ret;
	ssize_t retval;
	uint64_t buf[BUF_SIZE];
	uint64_t rbuf[BUF_SIZE];

	fd = open("/tmp/test_file", O_RDWR|O_CREAT, S_IRWXU);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	ret = ftruncate(fd, BUF_SIZE);
	if (ret == -1) {
		perror("ftruncate");
		exit(1);
	}

	for (i = 0; i < BUF_SIZE/8; i++)
		buf[i] = i;

	ret = pwrite(fd, buf, BUF_SIZE, 0);
	if (ret == -1) {
		perror("pwrite");
		exit(1);
	}

	retval = do_something_and_reading(fd, rbuf, BUF_SIZE);
	if (retval != BUF_SIZE) {
		fprintf(stderr, "do_something_and_reading fails: retval=%zd\n",
			retval);
		goto err;
	}

	for (i = 0; i < BUF_SIZE / 8; i++) {
		if (buf[i] != rbuf[i]) {
			fprintf(stderr, "diff buf[%d] = %lx, rbuf[%d] = %lx\n",
				i, buf[i], i, rbuf[i]);
			goto err;
		}
	}
	printf("OK\n");
err:
	close(fd);

	return 0;
}
~~~

## Compile and execute the VE program
Save the above code as simple_aio.c and compile it on VE side as shown below.
~~~
$ /opt/nec/ve/bin/ncc -o simple_aio simple_aio.c -lveio -pthread
~~~
Please note the VE AIO API functions are in libveio library. So, "-lveio -pthread" option is required.

Execute compiled VE program.
~~~
$ ./simple_aio
OK
~~~

There are some environment variables to change IO performance in VE AIO. Please find [Note](group__veaio.html#details) for more detail.
