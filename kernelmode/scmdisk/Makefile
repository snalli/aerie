# build modules
obj-m = scmdisk.o
scmdisk-objs = brd.o scm.o
#scmdisk-objs = brd26.o scm.o   # linux 2.6.33

# use the kernel build system
KERNEL_VERSION := `uname -r`
KERNEL_SOURCE := /lib/modules/$(KERNEL_VERSION)/build

SRCDIR=`pwd`
OBJDIR=`pwd`/build

all: create_build_dir build_kernelmodule build_userapp

clean: clean_kernelmodule clean_userapp

create_build_dir:
	if test ! -d $(OBJDIR); then                             \
		mkdir $(OBJDIR);                                     \
		cp Makefile $(OBJDIR)/Makefile;                      \
		ln -s $(SRCDIR)/brd.c $(OBJDIR)/brd.c;               \
		ln -s $(SRCDIR)/brd26.c $(OBJDIR)/brd26.c;           \
		ln -s $(SRCDIR)/scm.c $(OBJDIR)/scm.c;               \
		ln -s $(SRCDIR)/scm.h $(OBJDIR)/scm.h;               \
		ln -s $(SRCDIR)/../scmmodel $(OBJDIR)/scmmodel;      \
		ln -s $(SRCDIR)/scmdisk.h $(OBJDIR)/scmdisk.h;       \
		ln -s $(SRCDIR)/scmdiskctl.c $(OBJDIR)/scmdiskctl.c; \
	fi

build_kernelmodule: 
	make -C $(KERNEL_SOURCE)  M=$(OBJDIR) modules

clean_kernelmodule:
	make -C $(KERNEL_SOURCE) M=$(OBJDIR) clean

build_userapp:
	gcc $(OBJDIR)/scmdiskctl.c -o $(OBJDIR)/scmdiskctl

clean_userapp:
	rm $(OBJDIR)/scmdiskctl
