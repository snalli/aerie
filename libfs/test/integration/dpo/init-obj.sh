#!/bin/bash
#
# prepare an empty file

TEST_FILE=/tmp/stamnos_test_object_file
rm $TEST_FILE
truncate -s 1024K $TEST_FILE 

exit 0 # ignore any failed commands 
