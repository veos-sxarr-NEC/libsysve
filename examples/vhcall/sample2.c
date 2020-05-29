#include <stddef.h>
#include <stdlib.h>
#include <libvhcall.h>
#include <stdio.h>

#define MAXLEN 100
#define N      3
const char *msg = "Hello, This is VE";

int main() {
	int ret=1;
	uint64_t len = MAXLEN;
	char buf[MAXLEN];
	double a[N][N];
	int64_t symid;
	vhcall_handle handle;
	vhcall_args *ca;
	uint64_t retval = -1;
	int i,j;
	/* Load VH Fortran library */
	printf("Test for Fortran library\n");
	handle = vhcall_install("./libvhhello_f.so");
        if (handle == (vhcall_handle)-1) {
                perror("vhcall_install");
                vhcall_args_free(ca);
                goto ret;
        }
	printf("\nInvoke function passing string\n");
	/* Find VH Fortran library function */
        symid  = vhcall_find(handle, "hellofunc_");
        if (symid == -1 ) {
                perror("vhcall_find");
                goto ret3;
        }
	/* Create arguments */
	ca=vhcall_args_alloc();
	memcpy(buf, msg, strlen(msg));
        memset(buf+strlen(msg)+1, '\0', MAXLEN-strlen(msg));
        /* 1st */
        ret = vhcall_args_set_pointer(ca, VHCALL_INTENT_IN, 0, buf, MAXLEN);
        if (ret) {
                perror("vhcall_args_set_pointer");
                goto ret3;
        }
        /* 2nd */
        ret = vhcall_args_set_pointer(ca, VHCALL_INTENT_IN, 1, &len, sizeof(uint64_t));
        if (ret) {
                perror("vhcall_args_set_pointer");
                goto ret3;
        }
	/* Invoke VH Fortran library function */
        ret = vhcall_invoke_with_args(symid, ca, &retval);
        if (ret) {
                perror("vhcall_invoke_with_args");
                goto ret3;
        }
	/* Check return value of function */
        if (retval != 999) {
                printf("VH function returns unexpected value(%ld)\n", retval);
                goto ret3;
        }
	printf("\nInvoke subroutine passing matrix of double\n");
	/* Find VH Fortran library subroutine */
        symid  = vhcall_find(handle, "hellosubr_");
        if (symid == -1 ) {
                perror("vhcall_find");
                goto ret3;
        }
        /* Update argument */
	for(i=0;i<N;i++)for(j=0;j<N;j++)
		a[i][j]=10*i+j;
	len = N;
        ret = vhcall_args_set_pointer(ca, VHCALL_INTENT_INOUT, 0, a, len*len*sizeof(double));
        if (ret) {
                perror("vhcall_args_set_pointer");
                goto ret3;
        }
	/* Invoke VH Fortran library subroutine */
	for(i=0;i<N;i++)for(j=0;j<N;j++)
		printf("[VE] a[%d][%d]=%.16f\n", i,j,a[i][j]);
	ret = vhcall_invoke_with_args(symid, ca, NULL);
        if (ret) {
                perror("vhcall_invoke_with_args");
                goto ret3;
        }
ret3:
	/* Free arguments object */
	vhcall_args_free(ca);
ret2:
	/* Unload VH library */
	if (vhcall_uninstall(handle))
                perror("vhcall_uninstall");
ret:
	return ret;
}
