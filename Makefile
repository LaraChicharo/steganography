# Makefile
LDFLAGS=-lpng

open_write: open_write.o
	cc -o open_write open_write.o $(LDFLAGS)
open_write.o:
	cc -c open_write.c

clean:
	rm *.o open_write
