#!/bin/bash
#
# do cleanup

./build/bench/ubench/ubench_pxfs -h 10000 -d 0 +fs_create -p /pxfs/ -n 10 -s 512

