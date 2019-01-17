if SEPARATEDLIBS
lib_LTLIBRARIES =	libsysve.la libveio.la libveaccio.la
libveio_la_SOURCES =	libveaio.c \
			libvedma.c vedma_init.c vedma_impl.h vedma_main.S \
			libsysve_vec_memcpy.S libsysve_atomic.s libsysve_utils.h
libveaccio_la_SOURCES = accelerated_io.c
libsysve_la_SOURCES =	libvhcall.c libveshm.c libsysve.c libvecr.c \
			libvhshm.c libuserdma.c
libveio_la_LDFLAGS = -version-info 1:0:0 -Wl,--build-id=sha1 -lpthread
libveio_la_CFLAGS = -I$(top_srcdir)/include -I@LIBC_INC@/include
libveaccio_la_LDFLAGS = -version-info 1:0:0 -Wl,--build-id=sha1 -lpthread -lveio
libveaccio_la_CFLAGS = -I$(top_srcdir)/include -I@LIBC_INC@/include
else
lib_LTLIBRARIES = libsysve.la
libsysve_la_SOURCES =	rodata.s \
			libvhcall.c libveshm.c libsysve.c libvecr.c \
			libvhshm.c libuserdma.c \
			libveaio.c \
			libvedma.c vedma_init.c vedma_impl.h vedma_main.S
endif
libsysve_la_LDFLAGS = -version-info 1:0:0 -Wl,--build-id=sha1
libsysve_la_CFLAGS = -I$(top_srcdir)/include -I@LIBC_INC@/include
include_HEADERS = $(top_srcdir)/include/libvhcall.h \
					$(top_srcdir)/include/veshm.h \
					$(top_srcdir)/include/vecr.h \
					$(top_srcdir)/include/libsysve.h \
					$(top_srcdir)/include/vhshm.h \
					$(top_srcdir)/include/userdma.h \
					$(top_srcdir)/include/veaio.h \
					$(top_srcdir)/include/vedma.h

