#ifndef SLAB_H
#define SLAB_H


#define D_PRINT(err, format, ...)

class slab_free_list
{
   public: slab_free_list *n;
};

template <int size> class slab
{
	slab_free_list *tail;		// free cell list
	slab_free_list *slices;		// list of malloced blocks
	int fragment_size;		// number of slots to be allocated
   public:
	inline slab(int frag_size);
	inline ~slab();
	inline void shutdown();
	inline void *alloc();
	inline void release(void *ptr);
};

template <int size>
inline slab<size>::slab(int fs)
{
	tail = NULL;
	slices = NULL;
	fragment_size = fs;
}

/*
 *	The destructor releases any memory previously allocated,
 *	and checks whether the space really was free.
 */

template <int size>
inline slab<size>::~slab()
{
	shutdown();
}

template <int size>
inline void slab<size>::shutdown()
{
	slab_free_list *tmp, *tmp2;
	int cells = 0;

	for (tmp = tail; tmp; tmp = tmp->n) cells++;
	for (tmp = slices; tmp; ) {
		cells -= fragment_size - 1;
		tmp2 = tmp;
		tmp = tmp->n;
		free(tmp2);
	}
	tail = NULL;
	slices = NULL;
	//if (cells && !fatal_bugs) shriek(862, fmt("Slab<%d> lost %d cells", size, -cells));
}

template <int size>
inline void *slab<size>::alloc()
{
	void *slot;

	if (!tail) {
		D_PRINT(0, "Metaallocating for slab\n");
		void *more = malloc(size * fragment_size);
		for (int i=1; i < fragment_size; i++) this->release(i*size +(char *)more);
		((slab_free_list *) more)->n = slices;
		slices = (slab_free_list *) more;
	}
	slot = tail;
	tail = tail->n;
	D_PRINT(0, "Allocating %p from slab\n", slot);
	return slot;
}

template <int size>
inline void slab<size>::release(void *ptr)
{
	D_PRINT(0, "Releasing %p to slab\n", ptr);
	slab_free_list *slot = (slab_free_list *)ptr;
	slot->n = tail;
	tail = slot;
}

#define SLABIFY(type, slab_name, slice_sz, shutdown_handler)	\
					\
slab<sizeof(type)> slab_name(slice_sz);	\
					\
void * type::operator new(size_t)	\
{					\
	return slab_name.alloc();	\
}					\
					\
void type::operator delete(void *ptr)	\
{					\
	slab_name.release(ptr);		\
}					\
					\
void shutdown_handler()			\
{					\
	slab_name.shutdown();		\
}

#endif		// SLAB_H
