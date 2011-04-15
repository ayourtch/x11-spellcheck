all: sample
sample:
	gcc -lX11 /usr/lib/libXtst.so.6 sample.c 
