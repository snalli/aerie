#!/bin/bash
if test -z "$1"
then
  LINUX=../../../../kernelmode/linux-3.2.2
else
  LINUX=$1
fi
echo 'Processing kernel tree' ${LINUX}
CWD=`pwd`
cd $LINUX
ctags `cat $CWD/linux-tagfiles`
mv tags $CWD/linux-tags
