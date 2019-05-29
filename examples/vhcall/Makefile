NCC = /opt/nec/ve/bin/ncc
GCC = gcc
GFORT = gfortran

ALL = sample1 libvhhello.so libvhhello_f.so sample2 libdatatransfer.so

all: $(ALL)

sample1: sample1.c
	$(NCC) -o $@ $^

libvhhello.so: libvhhello.c
	$(GCC) -o $@ $^ -fpic -shared

libvhhello_f.so: libvhhello.f90
	$(GFORT) -o $@ $^ -fpic -shared

sample2: sample2.c
	$(NCC) -o $@ $^

libdatatransfer.so: libdatatransfer.c
	$(GCC) -o $@ $^ -fpic -shared -I/opt/nec/ve/veos/include

clean:
	rm -f $(ALL) *.o
