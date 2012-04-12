#!/bin/bash
LINUX=/home/hvolos/workspace/linux/linux-2.6.33
CWD=`pwd`
cd $LINUX
ctags `cat $CWD/linux-tagfiles`
mv tags $CWD/linux-tags
