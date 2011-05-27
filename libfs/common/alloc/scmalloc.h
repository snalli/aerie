#ifndef _SCMALLOC_H_AKL178
#define _SCMALLOC_H_AKL178

extern void scmalloc_init(void**, size_t);
int scmalloc(void **ptr, size_t size);
extern void scmfree(void*, size_t);
int scmrealloc(void **, size_t, size_t);

#endif
