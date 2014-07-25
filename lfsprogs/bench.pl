#!/usr/bin/perl
use Time::HiRes qw(gettimeofday tv_interval);

$DO = 1;

if(@ARGV != 1) {
	print "Usage: $0 <lfs|ext2|ext3|reiserfs>\n";
	exit(1);
}

$fs_type = $ARGV[0];
$dev = "/dev/hda11";
$mnt = "/mnt/foo";

$file = "test.c";

&init;
for($i = 2; $i < 1024 * 128; $i = $i * 2) {
	&write_test($i);
}

&exit;

sub init 
{
	if($fs_type eq "lfs") {
		$mkfs = "./mklfs";
	}
	elsif ($fs_type eq "ext2" || $fs_type eq "ext3") {
		$mkfs = "mkfs.$fs_type";
	}
	else {
		$mkfs = "mkreiserfs";
	}
	run("$mkfs $dev");
	run("mount -t $fs_type $dev $mnt");
}

sub exit
{
	run("umount $mnt");
}

sub write_test
{
	$nblocks = $_[0];
	$start = [gettimeofday];
	run("./test 2 $mnt/$file $nblocks");
	$elapsed = tv_interval($start);
	print "Nblocks: $nblocks Time:$elapsed\n";
	#run("rm $mnt/$file");
}

sub run
{
	$command = $_[0];

	if($DO) {
		system($command);
	}
	else {
		print($command, "\n");
	}
}
