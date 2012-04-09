#include <linux/memory.h>
#include <linux/mmzone.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <asm/pgtable.h>

#define PERS_START 0x5000000000
#define PERS_SPACE 1024*1024*1024*4
#define PAGESIZE   4*1024*1024

#define SUCCESS		0x1
#define VIRTADDROLAP 	-0x1
#define HOTPLUGERROR 	-0x2
#define CURMAPERROR	-0x4 
#define CONTIGERROR	-0x8
#define SZALIGNERROR	-0x16 

#define MAXVIRTBLOCKS 128

/* 
Structure to map address between virtual address and physical address 
 
The chunk allocator uses this structure to identify if the virtual address 
with the provided range has already overlapped with previous mapping. 
 
The page fault handler uses this structure to establish a mapping  
 
Linear array is used to maintain the mapping which is sorted. Thus trying to  
optimize lookup during page fault handling. 
*/
typedef struct {
	unsigned long v_startaddr;
	unsigned long size;
	unsigned long p_index;
	unsigned long nr_blocks;	
}vpmemblock;

typedef struct {
	vpmemblock vpblock[MAXVIRTBLOCKS];
	long int max_virtindex;
}virt_phys_map;

extern unsigned long get_memory_block_size(void);
extern int memory_block_change_state(struct memory_block *,
		unsigned long, unsigned long);
extern struct memory_block *find_memory_block(struct mem_section *);
extern int is_removable(struct memory_block *);
