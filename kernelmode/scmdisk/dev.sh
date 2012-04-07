#!/bin/bash

scmctldevice0="/dev/scm0-ctl"

mknod $scmctldevice0 c 241 0
chmod a+wr $scmctldevice0
