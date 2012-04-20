#ifndef __STAMNOS_SCM_ASM_H
#define __STAMNOS_SCM_ASM_H

#include <mmintrin.h>

static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}

#if defined(__i386__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

static inline unsigned long long asm_rdtscp(void)
{
		unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"ecx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );

}
#elif defined(__x86_64__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static inline unsigned long long asm_rdtscp(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"rcx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#else
#error "What architecture is this???"
#endif


static inline void asm_sse_write_block64(volatile scm_word_t *addr, scm_word_t *val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[0]): "r" (val[0]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[1]): "r" (val[1]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[2]): "r" (val[2]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[3]): "r" (val[3]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[4]): "r" (val[4]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[5]): "r" (val[5]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[6]): "r" (val[6]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[7]): "r" (val[7]));
}


static inline void asm_movnti(volatile scm_word_t *addr, scm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_clflush(volatile scm_word_t *addr)
{
	__asm__ __volatile__ ("clflush %0" : : "m"(*addr));
}


static inline void asm_mfence(void)
{
	__asm__ __volatile__ ("mfence");
}


static inline void asm_sfence(void)
{
	__asm__ __volatile__ ("sfence");
}

#endif // __STAMNOS_SCM_ASM_H
