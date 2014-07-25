CC=gcc

EXES=lfs_cleanerd
INCDIR=../../include

all: $(EXES) 

lfs_cleanerd:cleanerd.o library.o coalesce.o print.o
	gcc -o $@ $^

%.o: %.c $(INCDIR)/lfs.h cleaner.h
	gcc -DDEBUG -g -I $(INCDIR) -o $@ -c $<
#	gcc -Wall -DDEBUG -g -I $(INCDIR) -o $@ -c $<

clean:
	rm -f $(EXES)
	rm -f *.o
