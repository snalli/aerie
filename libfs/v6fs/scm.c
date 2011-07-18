#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"
#include "buf.h"

typedef unsigned long uintptr_t;

#define PAGE_SIZE          512
#define NUM_PAGES(size)    ((((size) % PAGE_SIZE) == 0? 0 : 1) + (size)/PAGE_SIZE)
#define SIZEOF_PAGES(size) (NUM_PAGES((size)) * PAGE_SIZE)


int scm_size = 4096*512;

int scm_fd;
uintptr_t scm_mmap_addr;

struct scmroot {
	void *sector_region;
};

struct scmroot *scmroot_ptr;

scminit()
{
	scmalloc_init(&scmroot_ptr, sizeof(struct scmroot));
	scm_mmap_addr = (uintptr_t) scmroot_ptr->sector_region;
	assert(scm_mmap_addr != -1);
}


// Sync buf with SCM.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
scmrw(struct buf *b)
{
  struct buf **pp;

  if(!(b->flags & B_BUSY))
    panic("scmrw: buf not busy");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("scmrw: nothing to do");

  if(b->flags & B_DIRTY) {
    // TODO: write buf to SCM identified by device number dev
	// we need to (pre)-init the SCM device via a new function scminit
	//pwrite(scm_fd, b->data, 512, 512*b->sector);
	//printf("write to SCM: %p\n", scm_mmap_addr);
	//printf("write to SCM: ptr = %p\n", (void *) (scm_mmap_addr + 512*b->sector));
	assert(512*b->sector < scm_size*1024);
	memcpy((void *) (scm_mmap_addr + 512*b->sector), b->data, 512);
	//printf("write to SCM: DONE\n");
  } else {
    // TODO: read buf from SCM
	//pread(scm_fd, b->data, 512, 512*b->sector);
	//printf("read from SCM: base = %p\n", scm_mmap_addr);
	//printf("read from SCM: ptr = %p\n", (void *) (scm_mmap_addr + 512*b->sector));
	assert(512*b->sector < scm_size*1024);
	memcpy(b->data, (void *) (scm_mmap_addr + 512*b->sector), 512);
	//printf("read from SCM: DONE\n");
  }

  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
}


