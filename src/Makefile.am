lib_LTLIBRARIES = libsysve.la
libsysve_la_SOURCES = rodata.s libvhcall.c libveshm.c libsysve.c libvecr.c \
			libvhshm.c
libsysve_la_LDFLAGS = -version-info 1:0:0 -Wl,--build-id=sha1
libsysve_la_CFLAGS = -I$(top_srcdir)/include -I@LIBC_INC@/include 
include_HEADERS = $(top_srcdir)/include/libvhcall.h \
					$(top_srcdir)/include/veshm.h \
					$(top_srcdir)/include/vecr.h \
					$(top_srcdir)/include/libsysve.h \
					$(top_srcdir)/include/vhshm.h
