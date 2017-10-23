# Makefile
LDFLAGS=-lpng

esteganografo: esteganografo.o
	cc -o esteganografo esteganografo.o $(LDFLAGS)
esteganografo.o:
	cc -c esteganografo.c

clean:
	rm *.o esteganografo
