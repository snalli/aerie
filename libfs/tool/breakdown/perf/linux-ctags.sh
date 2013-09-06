#!/bin/bash
LINUX=../../../../kernelmode/linux-3.9
CWD=`pwd`
cd $LINUX
ctags `cat $CWD/linux-tagfiles`
mv tags $CWD/linux-tags
