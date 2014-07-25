Introduction
------------
This is an implementation of the log structured file system for Linux.

Compilation
-----------
Currently compiles on 2.6.11. It may or may not compile on other kernel
versions. Kernel headers are required for compilation.

        - Run top level make

Usage
-----
        - Create a lfs partition
          lfsprogs/mklfs <device name>
        - Insert kernel module
          insmod lfs/lfs.ko
        - Mount the device
          mount -t lfs <device name> <target dir name>

Testing
-------
LFS is still an experimental file system, so do not put important data on a
LFS file system. For testing, please check doc/cases to see a list of cases
that are known to be working.

Contact
-------
Any queries regarding LFS should be directed to the devel mailing list at
logfs-devel@lists.sourceforge.net (You do not have to be subscribed to post to
the list).
