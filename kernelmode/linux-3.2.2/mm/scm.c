/*
 * mm/scm.c
 *
 * Written by Sankar
 *
 * Storage class memory		<sankarp@cs.wisc.edu>
 */

#include "scm.h"

/*
 * Variables used to maintain the mapping between virtual memory chunks
 * to physical memory chunks
 */
struct memory_block **mem_block;
unsigned long memblocksz = 0;
unsigned long memblocksz_mb = 0;

/*
 * Persistent page table that is shared across users
 */
ppgtable_user ppgtbl[MAXUSERS];
unsigned long ppgtbl_index = 0;
DEFINE_MUTEX(ppgtbl_lock);

/*
 * Simple lock implementation - This can be modified to a read write lock. This 
 * might be needed if features like reallocating chunks are done where the
 * status of memory sections can change significantly and more functions need to
 * be covered by the locks. But just the allocate_persistent_chunk() is the only
 * function protected by lock. 
*/
DEFINE_MUTEX(scm_lock);

/*
 * OS exposes 128 MB memory blocks (hotplug granularity). So, the bitmap size
 * should be physical memory size / 128 MB. The variable max_block_pindex 
 * tracks the block with the highest numbered physical index. The scm_init() 
 * defines the freememblocks as bitmap. 
 */
#define MEMBLOCKS_BMAPSZ BITS_TO_LONGS(max_block_pindex)
#define MAX_MEMBLOCKS_RND MEMBLOCKS_BMAPSZ*sizeof(long)*8
#define MAX_MEMBLOCKS    max_block_pindex+1
#define NOALIGN 0
unsigned long *freememblocks;
unsigned long max_block_pindex = 0;

/*
 * This was done to avoid introducing a read write lock. Page fault handler 
 * needs to grab read lock in order to avoid any inconsistent views in the 
 * mapping structure due to changes in page table structure. 
 * At any point passive_map1 and passive_map2 should have the same values 
 * except when the chunk allocator is currently working. The active_mapping
 * points to either of these structures and thereby offering a consitent view
 * to the readers (page fault handler). This is similar to RCU mechanism. 
 * The active_mapping pointer can be atomically modified to point to 
 * passive_map1 or passive_map2.
 */
virt_phys_map *active_mapping;
virt_phys_map passive_map1;
virt_phys_map passive_map2;
int current_map;
extern int sections_per_block;

/*
 * The removable state of the memory regions seem to be changing. So, before 
 * every call to allocate_persistent_chunks, the removable state of all memory 
 * region is checked. Since the chunk allocation is not in the critical path,
 * this does not cause any additional overhead. 
 */
static void update_removable_memblocks()
{
	unsigned long startbit = 0, scanbit;
	struct memory_block *mblock;
	unsigned long i;

	for(i = 0;i < MEMBLOCKS_BMAPSZ; i++)
		printk(KERN_ERR"before BITMAP %llx", freememblocks[i]);

	while(startbit < max_block_pindex)
	{
		scanbit = bitmap_find_next_zero_area(freememblocks,
					MAX_MEMBLOCKS, startbit, 1, NOALIGN);

		if(scanbit > max_block_pindex || scanbit == (unsigned long)-1)
			break;

		startbit = scanbit+1;
		mblock = mem_block[scanbit];

		if(!is_removable(mblock))
			bitmap_set(freememblocks, scanbit, 1);

		//printk(KERN_ERR"mblock state %d secct %d phys_index %d", mblock->state, mblock->section_count, mblock->start_section_nr / sections_per_block);
	}

	for(i = 0;i < MEMBLOCKS_BMAPSZ; i++)
		printk(KERN_ERR"after BITMAP %llx", freememblocks[i]);
}

/*
 * The initialization of scm related structures go here. This should be invoked
 * once during boot time.
 */
void init_scm(void)
{
	struct memory_block *mblock;
	unsigned long i, phys_index;
	unsigned long onegb = 1024*1024*1024;

	//printk(KERN_ERR"possible devices %d", MAX_MEMBLOCKS);

	freememblocks = (unsigned long *)kmalloc
		(sizeof(unsigned long) * MEMBLOCKS_BMAPSZ, GFP_KERNEL);
	bitmap_fill(freememblocks, MAX_MEMBLOCKS_RND); 
	mem_block = (struct memory_block **)kmalloc
		(sizeof(struct memory_block *) * MAX_MEMBLOCKS, GFP_KERNEL);
 
	for(i = 0;i < NR_MEM_SECTIONS;i++)
	{
		if(!present_section_nr(i))
			continue;

		mblock = find_memory_block(__nr_to_section(i));
		phys_index = mblock->start_section_nr/sections_per_block;
		mem_block[phys_index] = mblock;

		//printk(KERN_ERR"mblock state %d secct %d phys_index %d phy_dev %d", mblock->state, mblock->section_count, mblock->start_section_nr / sections_per_block, mblock->phys_device);

		if(is_removable(mblock))
			bitmap_clear(freememblocks, phys_index, 1);

		if(phys_index > max_block_pindex)
			panic("memblock index count mismatch");
	}

	//for(i = 0;i < MEMBLOCKS_BMAPSZ; i++)
	//	printk(KERN_ERR"BITMAP %lx", freememblocks[i]);

	memblocksz = get_memory_block_size();
	memblocksz_mb = memblocksz / 1024 / 1024;
	active_mapping = &passive_map1;
	current_map = 1;
	passive_map1.max_virtindex = passive_map2.max_virtindex = -1;
}

long allocate_persistent_chunk(unsigned long *p_index, unsigned long size_mb)
{
	//The constraints on the requested size should be checked by the caller
	int memblocks_req = size_mb / memblocksz_mb;
	unsigned long scanbitindex = 0;
	unsigned long freebitindex = 0; 
	unsigned long i, j = 0;
	int error = -EINVAL;

	printk(KERN_ERR"size_mb %d memblocks requested : %d", size_mb, memblocks_req);
	
	while(scanbitindex <= max_block_pindex)
	{	
		// Find free bitmaps that cover the requested size
		freebitindex = bitmap_find_next_zero_area(freememblocks, 
						MAX_MEMBLOCKS, scanbitindex, 
						memblocks_req, NOALIGN);
		scanbitindex = freebitindex + 1;
		printk(KERN_ERR"free bit : %d", freebitindex);

		if(freebitindex > max_block_pindex ||
			freebitindex == (unsigned long)-1)
		{
			return CONTIGERROR;	
		}

		// Hotplug-offline the covered memory blocks
		for(i = 0; i < memblocks_req; i++)
		{
			j = freebitindex+i;
			error = memory_block_change_state(mem_block[j], 
						MEM_OFFLINE, MEM_ONLINE);
			// 0 refers to success
			if(error != 0)
			{
				printk(KERN_ERR"hotplug failed");
				break;
			}
		}

		//Hotplug of contiguous memory blocks failed
		//Revert the state of the one's that were offlined  
		if(error != 0)
		{
			printk(KERN_ERR"trying to revert");
			for(i = j-1; i >= freebitindex; i--)
			{
				error = memory_block_change_state(
						mem_block[i], MEM_ONLINE,
						MEM_OFFLINE);
				printk(KERN_ERR"Reverting for mblock %d", i);
				if(error == -EINVAL)
				{ /* onlining failed */ }
			}
		}
		else
		{
			bitmap_set(freememblocks, freebitindex, memblocks_req);
			*p_index = freebitindex;
			break;
		}
	}


	if(error == -EINVAL)	
		return HOTPLUGERROR;
	return SUCCESS;
}

SYSCALL_DEFINE2(alloc_persistent, unsigned long, v_addr, unsigned long, size_mb)
{
	/*
	input 		: size, virtual address
	output 		: true if allocated else false
	constraints 	: size in granularity of 128
			  virtual address in the persistent region
			  virtual address not already allocated
	*/

	long ret;
	unsigned long p_index;
	long int fill_index, i;
	vpmemblock curmap;
	virt_phys_map *tobeactivemap, *tobelazymap, *tobeprintmap; 
	unsigned long block_sz_mb;

	update_removable_memblocks();

	/* Check for alignment - allocation done in granularity of block size */
	block_sz_mb = memblocksz/1024/1024;
	if(size_mb & (block_sz_mb-1) != 0)
		return SZALIGNERROR;

	/* check fo virtual address overlap */

	// Locking to avoid races and corrupting the data structures
	mutex_lock(&scm_lock);

	ret = allocate_persistent_chunk(&p_index, size_mb);

	if(ret != SUCCESS)
		return ret;

	printk(KERN_ERR"phys index returned %d", p_index);

	if(current_map != 1 && current_map != 2)
		return CURMAPERROR;

	tobeactivemap = &passive_map2;
	tobelazymap = &passive_map1;
	if(current_map == 2)
	{
		tobeactivemap = &passive_map1;
		tobelazymap = &passive_map2;
	}

	curmap.v_startaddr = v_addr;
	curmap.size = size_mb;
	curmap.p_index = p_index;
	curmap.nr_blocks = block_sz_mb; 

	fill_index = tobeactivemap->max_virtindex + 1;
	for(i = tobeactivemap->max_virtindex; i >= 0; i--)
	{
		if(tobeactivemap->vpblock[i].v_startaddr < v_addr)
			break;
		tobeactivemap->vpblock[i+1] =
			tobeactivemap->vpblock[i];
		fill_index = i;
	}
	tobeactivemap->vpblock[fill_index] = curmap;
	tobeactivemap->max_virtindex++;	

	//This is supposed to be an atomic assignment
	active_mapping = tobeactivemap;

	//Update the other list tobelazymap
	fill_index = tobelazymap->max_virtindex + 1;
	for(i = tobelazymap->max_virtindex; i >= 0; i--)
	{
		if(tobelazymap->vpblock[i].v_startaddr < v_addr)
			break;
		tobelazymap->vpblock[i+1] =
			tobelazymap->vpblock[i];
		fill_index = i;
	}
	tobelazymap->vpblock[fill_index] = curmap;
	tobelazymap->max_virtindex++;	

	//Assert to check if both the lists are the same
	tobeprintmap = &passive_map1;
	for(i = tobeprintmap->max_virtindex; i >= 0; i--)
		printk(KERN_ERR"v_addr %lx size %ld p_index %ld nr_blocks %ld",
			tobeprintmap->vpblock[i].v_startaddr,
			tobeprintmap->vpblock[i].size, 
			tobeprintmap->vpblock[i].p_index,
			tobeprintmap->vpblock[i].nr_blocks);
			
	tobeprintmap = &passive_map2;
	for(i = tobeprintmap->max_virtindex; i >= 0; i--)
		printk(KERN_ERR"v_addr %lx size %ld p_index %ld nr_blocks %ld",
			tobeprintmap->vpblock[i].v_startaddr,
			tobeprintmap->vpblock[i].size,
			tobeprintmap->vpblock[i].p_index,
			tobeprintmap->vpblock[i].nr_blocks);
	
	//unlock
	mutex_unlock(&scm_lock);

	return SUCCESS;
}

/*
 * Page fault handler for persistent region
 * 1. Establishes the page table entry
 * 2. Responsible for setting up the protection bits for every page based 
 * on the process
 * 
 * __do_fault() - lies in page fault handling code path - checks if the address
 * that faulted lies in persistent space by checking the persistent field
 * of the vma node. If so, the code path ends up here.
 */
unsigned long pgfault_serviced = 0;
bool ppg_tracker = false;

int do_persistent_fault(struct mm_struct *mm, struct vm_area_struct *vma, 
			unsigned long address, pte_t *pte, pmd_t *pmd,
			unsigned int flags)
{
	int i;
        spinlock_t *ptl;
        pte_t entry;
	pte_t *page_table;
	unsigned long page_nr, va_pg, va_off, p_start, p_w_off;
	virt_phys_map *vpmap = active_mapping;
	bool block_fnd = false;
	struct page* first_page;

	//printk(KERN_ERR"do_persistent_fault: faulting address %lx", address);

	for(i = vpmap->max_virtindex; i >= 0; i--)
	{
		unsigned long saddr, size;
		saddr = vpmap->vpblock[i].v_startaddr;
		size = vpmap->vpblock[i].size * 1024 * 1024;

		if(saddr <= address && address <= saddr+size)
		{
			block_fnd = true;
			break;
		}
	}

	if(block_fnd == true)
	{
		va_pg = address & PAGE_MASK;
		va_off = va_pg - vpmap->vpblock[i].v_startaddr;

		first_page = pfn_to_page(vpmap->vpblock[i].p_index << PFN_SECTION_SHIFT);
		p_start = page_to_pfn(first_page); 
		p_start = p_start << PAGE_SHIFT;
		p_w_off = p_start + va_off;

		page_nr = p_w_off >> PAGE_SHIFT;
		pgfault_serviced++;
		//printk(KERN_ERR"%s: va_pg %lx; va_off %lx; p_start %lx; p_w_off %lx; page_nr %lx;", current->comm, va_pg, va_off, p_start, p_w_off, page_nr);
	}

	page_table = pte_offset_map_lock(mm, pmd, address, &ptl);
	entry = pfn_pte(page_nr, vma->vm_page_prot);

	if (flags & FAULT_FLAG_WRITE)
		entry = pte_mkwrite(pte_mkdirty(entry));

	//printk(KERN_ERR"persistent pte entry %lx", entry);
	set_pte_at(mm, address, page_table, entry);
	update_mmu_cache(vma, address, page_table);

	pte_unmap_unlock(page_table, ptl);

	return 0;
}

void pud_assign(struct mm_struct *mm, pgd_t *pgd, pud_t *new)
{
	spin_lock(&mm->page_table_lock);
	pgd_populate(mm, pgd, new);
	spin_unlock(&mm->page_table_lock);
}

pud_t *find_shared_ppgtbl_entry(uid_t uid, bool create_new, bool incref)
{
	unsigned long i;
	pud_t *ppud = NULL;

	if(create_new == true)
		mutex_lock(&ppgtbl_lock);

	for(i = 0;i < ppgtbl_index; i++)
	{
		if(ppgtbl[i].uid == uid)
		{
			ppud = ppgtbl[i].ppud;
			goto ppgd_ret; 
		}
	}

	// Does not support more than MAXUSERS
	if(ppgtbl_index+1 >= MAXUSERS || create_new == false)
		goto ppgd_ret;

	ppg_tracker = true;
	ppgtbl[ppgtbl_index].uid = uid;
	ppud = ppgtbl[ppgtbl_index].ppud = pud_alloc_one(NULL, 0x0);
	ppgtbl_index++;

	for(i = 0;i < ppgtbl_index; i++)
	{
		ppud = ppgtbl[i].ppud;
		printk(KERN_ERR"USER SHARED %d.ppgtbl UID %lx PUD %lx pfn %lx", 
			i, ppgtbl[i].uid, ppud, __pa(ppud));
	}

ppgd_ret:
	if(create_new == true)
		mutex_unlock(&ppgtbl_lock);

	if(ppud != NULL && incref == true)
		get_page(pfn_to_page(__pa(ppud) >> PAGE_SHIFT));

	return ppud;
}

void clear_ppgd_from_mm(struct mm_struct *mm)
{
	pgd_t *pgd;
	pud_t *pud, *shared_pud;

	pgd = pgd_offset(mm, PERS_START);
	pud = pud_offset(pgd, PERS_START);
	shared_pud = find_shared_ppgtbl_entry(current->cred->uid, false, false);

	if(pud == shared_pud)
	{
	//printk(KERN_ERR"REVOKE %s pgd--%lx *pgd--%lx pud--%lx pid--%lx", 
	//		current->comm, pgd, *pgd, pud, current->cred->uid);
		spin_lock(&mm->page_table_lock);
		*(unsigned long *)pgd = 0x0;
		spin_unlock(&mm->page_table_lock);
	}
}

