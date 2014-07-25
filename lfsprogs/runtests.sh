#!/bin/sh
#############################################################################
#
# Modified for testing LFS by Pradeep Padala
# Original Script from Justin Piszcz
# Refer to http://linuxgazette.net/102/piszcz.html for details
#
#############################################################################

# TEST PARAMETERS
MAKE_TEN_THOUSAND_FILES=1000
MAKE_TEN_THOUSAND_DIRECTORIES=1000
SLEEP=2
KERNEL_NUM=1
COUNT=10
RUNS=3
#COUNT=100

# REAL PARAMETERS
#MAKE_TEN_THOUSAND_FILES=10000
#MAKE_TEN_THOUSAND_DIRECTORIES=10000
#SLEEP=10
#KERNEL_NUM=10
#COUNT=1024
#RUNS=3

MNTDIR=/mnt/foo		# where the file systems are mounted

remove()
{
rm -rf $MNTDIR/* > /dev/null 2>&1
find $MNTDIR -type f -exec rm -f {} \;  > /dev/null 2>&1
find $MNTDIR/* -type d -exec rmdir {} \; > /dev/null 2>&1
}

echo "runtests.sh: rm -rf $MNTDIR/*"
remove
#rm -f /x/k*tar
echo "runtests.sh: creating $MNTDIR/ten_thousand_file_test{1,2,3} directory"
mkdir -p $MNTDIR/ten_thousand_file_test{1,2,3}
echo "runtests.sh: creating $MAKE_TEN_THOUSAND_FILES files 3 times and taking the average." 
echo "runtests.sh: sync will be run and it will sleep 10 seconds afterwards."
echo "runtests.sh: between each run."

for i in `seq 1 $RUNS`
do
  echo '#!/bin/sh' > "/tmp/mft$i.sh"
  echo " " >> "/tmp/mft$i.sh"
  echo "for a in \`seq 1 $MAKE_TEN_THOUSAND_FILES\`" >> "/tmp/mft$i.sh"
  echo "do" >> "/tmp/mft$i.sh"
  echo "  touch $MNTDIR/ten_thousand_file_test$i/\$a" >> "/tmp/mft$i.sh"
  echo "done" >> "/tmp/mft$i.sh"
  chmod +x "/tmp/mft$i.sh"
done


TOTALTIME=0

rm -f /tmp/001-create_ten_thousand_file_tests

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Making $MAKE_TEN_THOUSAND_FILES files, run $i/3."
  /usr/bin/time /tmp/mft$i.sh 2>> /tmp/001-create_ten_thousand_file_tests 
  echo "runtests.sh: Test $i complete, syncing filesystem."
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" > /tmp/report.txt
echo "TEST 001 (make_ten_thousand_files)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/001-create_ten_thousand_file_tests >> /tmp/report.txt

rm -f /tmp/002-find_ten_thousand_files

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running find $MNTDIR/ten_thousand_file_test$i and timing it."
  /usr/bin/time find $MNTDIR/ten_thousand_file_test$i 1> /dev/null 2>>/tmp/002-find_ten_thousand_files
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

exit

echo "=====================================================" >> /tmp/report.txt
echo "TEST 002 (find_ten_thousand_files)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/002-find_ten_thousand_files >> /tmp/report.txt


rm -f /tmp/003-remove_ten_thousand_files

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running rm -rf $MNTDIR/ten_thousand_file_test$i and timing it."
  /usr/bin/time rm -rf $MNTDIR/ten_thousand_file_test$i 1>/dev/null 2>>/tmp/003-remove_ten_thousand_files
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 003 (remove_ten_thousand_files)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/003-remove_ten_thousand_files >> /tmp/report.txt

echo "runtests.sh: creating $MNTDIR/ten_thousand_directory_test{1,2,3} directories"
mkdir -p $MNTDIR/ten_thousand_directory_test{1,2,3}
echo "runtests.sh: creating $MAKE_TEN_THOUSAND_FILES directories 3 times and taking the average." 
echo "runtests.sh: sync will be run and it will sleep 10 seconds afterwards."
echo "runtests.sh: between each run."

for i in `seq 1 $RUNS`
do
  echo '#!/bin/sh' > "/tmp/mft$i.sh"
  echo " " >> "/tmp/mft$i.sh"
  echo "mkdir -p $MNTDIR/ten_thousand_directory_test$i" >> "/tmp/mft$i.sh"
  echo "for a in \`seq 1 $MAKE_TEN_THOUSAND_DIRECTORIES\`" >> "/tmp/mft$i.sh"
  echo "do" >> $ten_thousand_directory_test"$i" >> "/tmp/mft$i.sh"
  echo "  mkdir $MNTDIR/ten_thousand_directory_test$i/\$a" >> "/tmp/mft$i.sh"
  echo "done" >> $ten_thousand_directory_test"$i" >> "/tmp/mft$i.sh"
  chmod +x "/tmp/mft$i.sh"
done

rm -f /tmp/004-create_ten_thousand_directory_tests

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Making $MAKE_TEN_THOUSAND_FILES directories, run $i/3."
  /usr/bin/time /tmp/mft$i.sh 2>> /tmp/004-create_ten_thousand_directory_tests
  echo "runtests.sh: Test $i complete, syncing directoriesystem."
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 004 (make_ten_thousand_directories)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/004-create_ten_thousand_directory_tests >> /tmp/report.txt

rm -f /tmp/005-find_ten_thousand_directories

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running find $MNTDIR/ten_thousand_directory_test$i and timing it."
  /usr/bin/time find $MNTDIR/ten_thousand_directory_test$i 1> /dev/null 2>>/tmp/005-find_ten_thousand_directories
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done
 
echo "=====================================================" >> /tmp/report.txt
echo "TEST 005 (find_ten_thousand_directories)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/005-find_ten_thousand_directories >> /tmp/report.txt


rm -f /tmp/006-remove_ten_thousand_directories

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running rm -rf $MNTDIR/ten_thousand_directory_test$i and timing it."
  /usr/bin/time rm -rf $MNTDIR/ten_thousand_directory_test$i 1>/dev/null 2>>/tmp/006-remove_ten_thousand_directories
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 006 (remove_ten_thousand_directories)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/006-remove_ten_thousand_directories >> /tmp/report.txt

rm -f /tmp/copytime.txt

echo "runtests.sh: Making kernel tarball test directores."
echo "runtests.sh: Runnng mkdir -p $MNTDIR/k{1,2,3}"
mkdir -p $MNTDIR/k{1,2,3}

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Copying kernel tarball from other other and timing it."
  echo "runtests.sh: Running cp /x/linux-2.4.26.tar $MNTDIR/k$i"
  /usr/bin/time cp /x/linux-2.4.26.tar $MNTDIR/k$i 2>>/tmp/copytime.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 007 (copy_165mb_tarball_from_remote_disk_to_disk)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/copytime.txt >> /tmp/report.txt

rm -f /tmp/copytime.txt
rm -f /x/k{1,2,3}.tar

echo "runtests.sh: Creating $MNTDIR/k{1,2,3}"
mkdir -p $MNTDIR/k{1,2,3}
echo "runtests.sh: Copying kernel tarball from current filesystem to other disk and timing it."
for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running cp $MNTDIR/k$i/linux-2.4.26.tar /x/k$i.tar"
  /usr/bin/time cp $MNTDIR/k$i/linux-2.4.26.tar /x/k$i.tar 2>>/tmp/copytime.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 008 (copy_165mb_tarball_from_disk_to_remote_disk)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/copytime.txt >> /tmp/report.txt

rm -f /tmp/untar.txt

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Untarring with tar xf $MNTDIR/k$i/linux-2.4.26.tar and timing."
  echo "runtests.sh: Changing directory to $MNTDIR/k$i"
  cd $MNTDIR/k$i
  /usr/bin/time tar xf $MNTDIR/k$i/linux-2.4.26.tar 2>>/tmp/untar.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "runtests.sh: Changing directory back to /root"
cd


echo "=====================================================" >> /tmp/report.txt
echo "TEST 009 (untar_kernel_2.4.26_source_tree)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/untar.txt >> /tmp/report.txt


echo "runtests.sh: Preparing to tar kernel source directories."

rm -f /tmp/tar.txt

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running tar cf $MNTDIR/k$i/linux-2.4.26-tartest$i.tar $MNTDIR/k$i/linux-2.4.26"
  /usr/bin/time tar cf $MNTDIR/k$i/linux-2.4.26-tartest$i.tar $MNTDIR/k$i/linux-2.4.26 2>>/tmp/tar.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 010 (tar_kernel_2.4.26_source_tree)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/tar.txt >> /tmp/report.txt


echo "runtests.sh: Preparing to remove kernel source trees and time it."

rm -f /tmp/rmtree.txt

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Removing $MNTDIR/k$i/linux-2.4.26 and timing it"
  /usr/bin/time rm -rf $MNTDIR/k$i/linux-2.4.26 2>>/tmp/rmtree.txt
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 011 (rm_kernel_2.4.26_source_tree)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/rmtree.txt >> /tmp/report.txt


echo "runtests.sh: Cleaning current mess, rm -rf $MNTDIR/*"
remove
echo "runtests.sh: syncing disk"
sync
echo "runtests.sh: Sleeping for $SLEEP seconds."
sleep $SLEEP

echo "runtests.sh: Copying kernel tarball from remote disk (not timed)"
echo "runtests.sh: Running cp /x/linux-2.4.26.tar $MNTDIR"
cp /x/linux-2.4.26.tar $MNTDIR
echo "runtests.sh: Copying kernel tarball 10 times to 10 different filenames"

rm -f /tmp/cptime.txt

echo "runtests.sh: Creating copytest{1,2,3} directories."
mkdir -p $MNTDIR/copytest{1,2,3}

for i in `seq 1 $RUNS`
do
  echo '#!/bin/sh' > "/tmp/mft$i.sh"
  echo " " >> "/tmp/mft$i.sh"
  echo "for a in \`seq 1 $KERNEL_NUM\`" >> "/tmp/mft$i.sh"
  echo "do" >> "/tmp/mft$i.sh"
  echo "  cp $MNTDIR/linux-2.4.26.tar $MNTDIR/copytest$i/$a.tar" >> "/tmp/mft$i.sh"
  echo "done" >> "/tmp/mft$i.sh"
  chmod +x "/tmp/mft$i.sh"
done

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Copying kernel tarball 10 times, run $i/3." 
  /usr/bin/time /tmp/mft$i.sh 2>> /tmp/cptime.txt
  echo "runtests.sh: Test $i complete, syncing filesystem."
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done


echo "=====================================================" >> /tmp/report.txt
echo "TEST 012 (copy_2.4.26_tarball_10_times)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/cptime.txt >> /tmp/report.txt


echo "runtests.sh: Cleaning current mess, rm -rf $MNTDIR/*"
remove
echo "runtests.sh: syncing disk"
sync
echo "runtests.sh: Sleeping for $SLEEP seconds."
sleep $SLEEP

rm -f /tmp/1gb.txt

echo "runtests.sh: Preparing to create 10GB file from /dev/zero and time it."
for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Running dd if=/dev/zero of=$MNTDIR/1gb-$i bs=1M count=10240000"
  /usr/bin/time dd if=/dev/zero of=$MNTDIR/1gb-"$i" bs=1M count=$COUNT 2>> /tmp/1gb.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 013 (create_10_gb_file_from_zeros)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/1gb.txt >> /tmp/report.txt


rm -f /tmp/fcopy.txt
echo "runtests.sh: Copying a 10GB file."
for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Copying a 1gb file, on run $i/3"
  /usr/bin/time cp $MNTDIR/1gb-$i $MNTDIR/1gb-copy-$i 2>>/tmp/fcopy.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 014 (copy_a_1gb_file)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/fcopy.txt >> /tmp/report.txt


for i in `seq 1 $RUNS`
do
  dd if=/dev/zero of=$MNTDIR/10mb-$i bs=1M count=10 
done

rm -f /tmp/piecetest.txt

echo "runtests.sh: Splitting a 10mb file into 1000 byte pieces."
mkdir -p $MNTDIR/chunk{1,2,3}
for i in `seq 1 $RUNS`
do
  cd $MNTDIR/chunk$i
  /usr/bin/time split -a 32 -b 1000 $MNTDIR/10mb-$i 2>>/tmp/piecetest.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done


echo "=====================================================" >> /tmp/report.txt
echo "TEST 015 (split -a 32_10mb_file_into_1000b_pieces)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/piecetest.txt >> /tmp/report.txt

rm -f /tmp/piecetest.txt
echo "runtests.sh: removing old piece tests."
find $MNTDIR/chunk1 -type f -exec rm {} \;
find $MNTDIR/chunk2 -type f -exec rm {} \;
find $MNTDIR/chunk3 -type f -exec rm {} \;

echo "runtests.sh: Splitting a 10mb file into 1024 byte pieces."
mkdir -p $MNTDIR/chunk{1,2,3}
for i in `seq 1 $RUNS`
do
  cd $MNTDIR/chunk$i
  /usr/bin/time split -a 32 -b 1024 $MNTDIR/10mb-$i 2>>/tmp/piecetest.txt
  echo "runtests.sh: syncing disk"
  sync 
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 016 (split -a 32_10mb_file_into_1024b_pieces)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/piecetest.txt >> /tmp/report.txt

rm -f /tmp/piecetest.txt
find $MNTDIR/chunk1 -type f -exec rm {} \;
find $MNTDIR/chunk2 -type f -exec rm {} \;
find $MNTDIR/chunk3 -type f -exec rm {} \;



echo "runtests.sh: Splitting a 10mb file into 2048 byte pieces."
mkdir -p $MNTDIR/chunk{1,2,3}
for i in `seq 1 $RUNS`
do
  cd $MNTDIR/chunk$i
  /usr/bin/time split -a 32 -b 2048 $MNTDIR/10mb-$i 2>>/tmp/piecetest.txt
  echo "runtests.sh: syncing disk"
  sync 
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 017 (split -a 32_10mb_file_into_2048b_pieces)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/piecetest.txt >> /tmp/report.txt

rm -f /tmp/piecetest.txt
echo "runtests.sh: removing old piece tests."
find $MNTDIR/chunk1 -type f -exec rm {} \;
find $MNTDIR/chunk2 -type f -exec rm {} \;
find $MNTDIR/chunk3 -type f -exec rm {} \;




echo "runtests.sh: Splitting a 10mb file into 4096 byte pieces."
mkdir -p $MNTDIR/chunk{1,2,3}
for i in `seq 1 $RUNS`
do
  cd $MNTDIR/chunk$i
  /usr/bin/time split -a 32 -b 4096 $MNTDIR/10mb-$i 2>>/tmp/piecetest.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 018 (split -a 32_10mb_file_into_4096b_pieces)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/piecetest.txt >> /tmp/report.txt

rm -f /tmp/piecetest.txt
echo "runtests.sh: removing old piece tests."
find $MNTDIR/chunk1 -type f -exec rm {} \;
find $MNTDIR/chunk2 -type f -exec rm {} \;
find $MNTDIR/chunk3 -type f -exec rm {} \;




echo "runtests.sh: Splitting a 10mb file into 8192 byte pieces."
mkdir -p $MNTDIR/chunk{1,2,3}
for i in `seq 1 $RUNS`
do
  cd $MNTDIR/chunk$i
  /usr/bin/time split -a 32 -b 8192 $MNTDIR/10mb-$i 2>>/tmp/piecetest.txt
  echo "runtests.sh: syncing disk"
  sync
  echo "runtests.sh: Sleeping for $SLEEP seconds."
  sleep $SLEEP
done 
  
echo "=====================================================" >> /tmp/report.txt
echo "TEST 019 (split -a 32_10mb_file_into_8192b_pieces)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/piecetest.txt >> /tmp/report.txt



echo "runtests.sh: Cleaning current mess, rm -rf $MNTDIR/*"
remove
echo "runtests.sh: syncing disk"
sync
echo "runtests.sh: Sleeping for $SLEEP seconds."
sleep $SLEEP

echo "runtests.sh: Copying kernel tarball 3 times."
rm -f /tmp/krnsrc.txt
mkdir -p $MNTDIR/{1,2,3}
cp /x/linux-2.4.26.tar $MNTDIR
cd $MNTDIR
tar xf linux-2.4.26.tar
cd $MNTDIR
echo "runtests.sh: syncing disk"
sync
echo "runtests.sh: Sleeping for $SLEEP seconds."
sleep $SLEEP

for i in `seq 1 $RUNS`
do 
  echo "runtests.sh: Copying kernel tarball , run $i/3 times."
  /usr/bin/time cp -r $MNTDIR/linux-2.4.26 $MNTDIR/$i 2>>/tmp/krnsrc.txt
done

echo "=====================================================" >> /tmp/report.txt
echo "TEST 020 (copy kernel source tarball)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt

cat /tmp/krnsrc.txt >> /tmp/report.txt

echo "runtests.sh: Cleaning current mess, rm -rf $MNTDIR/*"
remove
echo "runtests.sh: syncing disk"
sync
echo "runtests.sh: Sleeping for $SLEEP seconds."
sleep $SLEEP

dd if=/dev/zero of=$MNTDIR/1Gigabyte bs=1M count=$COUNT
rm -f /tmp/null.txt

for i in `seq 1 $RUNS`
do
  echo "runtests.sh: Catting a 1gb file to /dev/null and timing it."
  /usr/bin/time cat $MNTDIR/1Gigabyte 1> /dev/null 2>>/tmp/null.txt
done


echo "=====================================================" >> /tmp/report.txt
echo "TEST 021 (cat_a_1gb_file_to_dev_null)" >> /tmp/report.txt
echo "=====================================================" >> /tmp/report.txt
cat /tmp/null.txt >> /tmp/report.txt


