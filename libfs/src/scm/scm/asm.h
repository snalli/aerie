#ifndef __STAMNOS_SCM_ASM_H
#define __STAMNOS_SCM_ASM_H

#include <stdint.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Serialising barrier (prevents instruction/memory reordering around timing)
// ---------------------------------------------------------------------------
static inline void asm_cpuid()
{
    __sync_synchronize();
}

// ---------------------------------------------------------------------------
// High-resolution cycle/nanosecond counter
// ---------------------------------------------------------------------------
static inline unsigned long long asm_rdtsc(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static inline unsigned long long asm_rdtscp(void)
{
    __sync_synchronize();
    return asm_rdtsc();
}

// ---------------------------------------------------------------------------
// Non-temporal / SCM write primitives
// Use release semantics — visible to other cores, avoids RFO like movnti
// ---------------------------------------------------------------------------
static inline void asm_sse_write_block64(volatile scm_word_t* addr, scm_word_t* val)
{
    for (int i = 0; i < 8; i++)
        __atomic_store_n(&addr[i], val[i], __ATOMIC_RELEASE);
}

static inline void asm_sse_write(volatile void* dst, uint64_t val)
{
    __atomic_store_n((volatile uint64_t*) dst, val, __ATOMIC_RELEASE);
}

static inline void asm_movnti(volatile scm_word_t* addr, scm_word_t val)
{
    __atomic_store_n(addr, val, __ATOMIC_RELEASE);
}

// ---------------------------------------------------------------------------
// Cache control
// ---------------------------------------------------------------------------
static inline void asm_clflush(volatile scm_word_t* addr)
{
    (void) addr;
    __sync_synchronize();
}

static inline void asm_mfence(void)
{
    __sync_synchronize();
}

static inline void asm_sfence(void)
{
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

#endif // __STAMNOS_SCM_ASM_H
