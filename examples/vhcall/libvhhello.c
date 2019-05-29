#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define MAXLEN 100
const char *msg="Hello VE, This is VH C library";
uint64_t hello(char *p, int i, float f)
{
	printf("[VH libvhhello.so:hello] 1starg: %s\n", p);
	printf("[VH libvhhello.so:hello] 2ndarg: %d\n", i);
	printf("[VH libvhhello.so:hello] 3rdarg: %f\n", f);
	memcpy(p, msg, strlen(msg));
	memset(p+strlen(msg)+1, '\0', MAXLEN-strlen(msg));
	return 0;
}
