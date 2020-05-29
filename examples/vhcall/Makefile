NCC = /opt/nec/ve/bin/ncc
GCC = gcc
GFORT = gfortran

ALL = sample1 sample2 libvhhello.so libvhhello_f.so sample3 libdatatransfer.so

all: $(ALL)

sample1: sample1.c
	$(NCC) -o $@ $^

sample2: sample2.c
	$(NCC) -o $@ $^

libvhhello.so: libvhhello.c
	$(GCC) -o $@ $^ -fpic -shared

libvhhello_f.so: libvhhello.f
	$(GFORT) -o $@ $^ -fpic -shared

sample3: sample3.c
	$(NCC) -o $@ $^

libdatatransfer.so: libdatatransfer.c
	$(GCC) -o $@ $^ -fpic -shared -I/opt/nec/ve/veos/include

clean:
	rm -f $(ALL) *.o
