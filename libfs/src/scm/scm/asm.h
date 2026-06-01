#ifndef __STAMNOS_SCM_ASM_H
#define __STAMNOS_SCM_ASM_H

#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
#include <mmintrin.h>
#endif

// ---------------------------------------------------------------------------
// CPU serialising barrier (used before rdtsc to prevent instruction reorder)
// ---------------------------------------------------------------------------
static inline void asm_cpuid()
{
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("cpuid" ::: "rax", "rbx", "rcx", "rdx");
#elif defined(__aarch64__)
    asm volatile("isb" ::: "memory");
#else
    __sync_synchronize();
#endif
}

// ---------------------------------------------------------------------------
// High-resolution cycle counter
// ---------------------------------------------------------------------------
#if defined(__i386__)

static inline unsigned long long asm_rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile(".byte 0x0f, 0x31" : "=A"(x));
    return x;
}

static inline unsigned long long asm_rdtscp(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"ecx");
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#elif defined(__x86_64__)

static inline unsigned long long asm_rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

static inline unsigned long long asm_rdtscp(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"rcx");
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#elif defined(__aarch64__)

static inline unsigned long long asm_rdtsc(void)
{
    unsigned long long val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

static inline unsigned long long asm_rdtscp(void)
{
    asm volatile("isb" ::: "memory");
    unsigned long long val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

#else

#include <time.h>
static inline unsigned long long asm_rdtsc(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline unsigned long long asm_rdtscp(void)
{
    return asm_rdtsc();
}

#endif

// ---------------------------------------------------------------------------
// Non-temporal / SCM write primitives
// ---------------------------------------------------------------------------
static inline void asm_sse_write_block64(volatile scm_word_t* addr, scm_word_t* val)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[0]) : "r"(val[0]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[1]) : "r"(val[1]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[2]) : "r"(val[2]));
    // insight : SEGFAULT occurs in the third movnti when fileserver profile
    // is run with two threads and the cache
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[3]) : "r"(val[3]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[4]) : "r"(val[4]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[5]) : "r"(val[5]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[6]) : "r"(val[6]));
    __asm__ __volatile__("movnti %1, %0" : "=m"(*&addr[7]) : "r"(val[7]));
#elif defined(__aarch64__)
    // stnp = store non-temporal pair (bypasses cache, equivalent to movnti)
    asm volatile("stnp %1, %2, [%0]"     :: "r"(addr+0), "r"(val[0]), "r"(val[1]) : "memory");
    asm volatile("stnp %1, %2, [%0]"     :: "r"(addr+2), "r"(val[2]), "r"(val[3]) : "memory");
    asm volatile("stnp %1, %2, [%0]"     :: "r"(addr+4), "r"(val[4]), "r"(val[5]) : "memory");
    asm volatile("stnp %1, %2, [%0]"     :: "r"(addr+6), "r"(val[6]), "r"(val[7]) : "memory");
#else
    for (int i = 0; i < 8; i++) addr[i] = val[i];
    __sync_synchronize();
#endif
}

static inline void asm_sse_write(volatile void* dst, uint64_t val)
{
#if defined(__x86_64__) || defined(__i386__)
    uint64_t* daddr = (uint64_t*) dst;
    __asm__ __volatile__("movnti %1, %0" : "=m"(*daddr) : "r"(val));
#elif defined(__aarch64__)
    asm volatile("stlr %1, [%0]" :: "r"(dst), "r"(val) : "memory");
#else
    *(volatile uint64_t*) dst = val;
    __sync_synchronize();
#endif
}

static inline void asm_movnti(volatile scm_word_t* addr, scm_word_t val)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("movnti %1, %0" : "=m"(*addr) : "r"(val));
#elif defined(__aarch64__)
    asm volatile("stlr %1, [%0]" :: "r"(addr), "r"(val) : "memory");
#else
    *addr = val;
    __sync_synchronize();
#endif
}

// ---------------------------------------------------------------------------
// Cache control
// ---------------------------------------------------------------------------
static inline void asm_clflush(volatile scm_word_t* addr)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("clflush %0" : : "m"(*addr));
#elif defined(__aarch64__)
    asm volatile("dc civac, %0" :: "r"(addr) : "memory");
#else
    (void) addr;
    __sync_synchronize();
#endif
}

static inline void asm_mfence(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("mfence");
#elif defined(__aarch64__)
    asm volatile("dmb ish" ::: "memory");
#else
    __sync_synchronize();
#endif
}

static inline void asm_sfence(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("sfence");
#elif defined(__aarch64__)
    asm volatile("dmb ishst" ::: "memory");
#else
    __sync_synchronize();
#endif
}

#endif // __STAMNOS_SCM_ASM_H
