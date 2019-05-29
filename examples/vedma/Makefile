NCC = /opt/nec/ve/bin/ncc
GCC = gcc

ALL = mkshm.x86 rdshm.x86 dma.ve

all: $(ALL)

mkshm.x86: mkshm.c
	$(GCC) -o $@ $^

rdshm.x86: rdshm.c
	$(GCC) -o $@ $^

dma.ve: dma.c
	$(NCC) -o $@ $^ -lveio -lpthread

clean:
	rm -f $(ALL) *.o
