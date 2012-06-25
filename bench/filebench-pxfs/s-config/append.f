#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"%Z%%M%	%I%	%E% SMI"

# Single threaded sequential reads (1MB I/Os) on a 1G file.

set $dir=/pxfs
set $cached=false
set $filesize=0k
set $nthreads=1
set $io4k=4k
set $io8k=8k
set $io16k=16k
set $io32k=32k

define file name=largefile,path=$dir,size=$filesize,prealloc,reuse,cached=$cached

define process name=fileappender,instances=1
{
  thread name=fileappendthread,memsize=10m,instances=$nthreads
  {
    flowop appendfile name=append-file,filename=largefile,iosize=$io4k
    flowop appendfile name=append-file,filename=largefile,iosize=$io8k
    flowop appendfile name=append-file,filename=largefile,iosize=$io16k
    flowop appendfile name=append-file,filename=largefile,iosize=$io32k
  }
}

echo  "FileMicro-SeqRead Version 2.1 personality successfully loaded"
usage "Usage: set \$dir=<dir>"
usage "       set \$cached=<bool>    defaults to $cached"
usage "       set \$filesize=<size>  defaults to $filesize"
usage "       set \$iosize=<size>    defaults to $iosize"
usage "       set \$nthreads=<value> defaults to $nthreads"
usage " "
usage "       run runtime (e.g. run 60)"

run 60
