#!/bin/bash
#LINUX=/home/hvolos/workspace/linux/linux-2.6.33
LINUX=/scratch/nvm/stamnos/kernelmode/linux-3.2.2
CWD=`pwd`
cd $LINUX
ctags `cat $CWD/linux-tagfiles`
mv tags $CWD/linux-tags
