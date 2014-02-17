#!/bin/bash

mountpoint="/mnt/scmfs"
scmdevice0="/dev/scm0"
objdir=`pwd`"/build"

function mkfs {
	if [ ! -b "$scmdevice0" ]
	then 
	mknod $scmdevice0 b 240 0
	fi
	/sbin/insmod $objdir/scmdisk.ko
	#/sbin/mke2fs -m 0 $scmdevice0
	#/sbin/mkfs.ext3 -m 0 $scmdevice0 -j
	/sbin/mkfs.ext4 -m 0 $scmdevice0 -j
	rm -rf $mountpoint
	if [ ! -d "$mountpoint" ]
	then 
	mkdir $mountpoint
	fi
	mount $scmdevice0 /mnt/scmfs -o noatime,nodiratime -o data=ordered
	#mount $scmdevice0 /mnt/scmfs -o noatime,nodiratime
	chmod a+wr /mnt/scmfs
}

function rmfs {
	umount $mountpoint
	/sbin/rmmod scmdisk.ko
}

if [ "$1" = "-c" ]
then
mkfs
df -T
fi

if [ "$1" = "-r" ]
then
rmfs
fi
