#Viziru Luciana - 332 CA
CFLAGS = -Wall -g -Wno-unused

.PHONY: all clean

build: libvmsim.so

libvmsim.so: vmsim.o common_lin.o linkedlist.o
	gcc -shared vmsim.o common_lin.o linkedlist.o -o libvmsim.so

vmsim.o: vmsim.h vmsim.c common.h util.h linkedlist.h helpers.h
	gcc -c -fPIC $^

common_lin.o: common_lin.c common.h util.h debug.h
	gcc -c -fPIC $^

linkedlist.o: linkedlist.h linkedlist.c
	gcc -c -fPIC $^

clean:
	-rm -f *~ *.o *.so

