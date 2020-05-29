#include <stddef.h>
#include <stdlib.h>
#include <libvhcall.h>
#include <stdio.h>

int main() {
	int ret=1;
	int64_t symid;
	vhcall_handle handle;
	vhcall_args *ca;
	uint64_t retval = -1;
        uint64_t data = 0xdeadbeefbaadcafe;

	handle = vhcall_install("./libdatatransfer.so");
	if (handle == (vhcall_handle)-1) {
		perror("vhcall_install");
                goto ret;
        }

	symid  = vhcall_find(handle, "func");
        if (symid == -1 ) {
                perror("vhcall_find");
                goto ret2;
        }
	/* Allocate arguments object for VH library function */
	ca  = vhcall_args_alloc();
        if (!ca) {
                perror("vhcall_args_alloc");
                goto ret2;
        }
	/* Set each arguments */
        printf("[VE] addr:%p, data:%lx\n", &data, data);
	/* 1st */
	ret = vhcall_args_set_veoshandle(ca, 0);
	if (ret) {
		perror("vhcall_args_set_veoshandle");
		goto ret3;
	}
	/* 2nd */
	ret = vhcall_args_set_u64(ca, 1, (uint64_t)&data);
	if (ret) {
                perror("vhcall_args_set_u64");
                goto ret3;
        }
	/* Invoke VH library function */
	ret = vhcall_invoke_with_args(symid, ca, &retval);
	if (ret) {
                perror("vhcall_invoke_with_args");
                goto ret3;
        }
	if (retval) {
		printf("VH function returns unexpected value(%ld)\n", retval);
                goto ret3;
        }
        printf("[VE] addr:%p, data:%lx\n", &data, data);
	ret = 0;
ret3:
	vhcall_args_free(ca);
ret2:
	if (vhcall_uninstall(handle))
                perror("vhcall_uninstall");
ret:
	return ret;
}
