all:
	cd kernel; make
	cd lfsprogs; make
clean:
	cd kernel; make clean
	cd lfsprogs; make clean
