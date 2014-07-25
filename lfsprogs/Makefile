CC=gcc

EXES=mklfs lfsread test
INCDIR=../include

all: $(EXES) 
	cd cleanerd; $(MAKE)

mklfs:mklfs.o
lfsread:lfsread.o
test:test.o

%.o: %.c $(INCDIR)/lfs.h
	gcc -Wall -I $(INCDIR) -o $@ -c $<

clean:
	cd cleanerd; $(MAKE) clean
	rm -f $(EXES)
	rm -f *.o
