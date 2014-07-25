ifneq ($(KERNELRELEASE),)
    obj-m	:= lfs.o
    lfs-objs	:= util.o segment.o inode.o dir.o file.o super.o \
    		   tree.o symlink.o ifile.o seguse.o segsum.o ioctl.o
    EXTRA_CFLAGS := -I$(src)/../include
else
    KDIR	:= /lib/modules/`uname -r`/build
    PWD		:= $(shell pwd)

    default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
    clean:
	rm *.o *.ko *.mod.c
endif
