/**
 * kvfs_multithread_test.cc — Multi-threaded unit tests for kvfs
 *
 * Tests concurrent put/get/delete operations across multiple threads
 *
 * Usage:  kvfs_multithread_test -h <port> -t <num_threads>
 *         kvfs_multithread_test -h 10001 -t 2   # Run with 2 threads
 *         kvfs_multithread_test -h 10001 -t 3   # Run with 3 threads
 *
 * Returns 0 if all tests pass, 1 if any fail.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ── test configuration ───────────────────────────────────────────────────── */

#define NUM_KEYS_PER_THREAD 50
#define MAX_THREADS 8

static int g_failures = 0;
static int g_total = 0;
static int g_num_threads = 2;

typedef struct
{
    int thread_id;
    int failures;
    int total;
} thread_stats_t;

#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "  [T%d] CHECK FAILED at %s:%d: %s\n", pthread_self(), __FILE__,       \
                    __LINE__, #expr);                                                              \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

/* ── thread-safe counter ────────────────────────────────────────────────── */

static pthread_mutex_t g_stats_lock = PTHREAD_MUTEX_INITIALIZER;

static void report_result(int thread_id, const char* name, int rc)
{
    pthread_mutex_lock(&g_stats_lock);
    g_total++;
    if (rc == 0)
    {
        printf("[T%d] PASS  %s\n", thread_id, name);
    }
    else
    {
        printf("[T%d] FAIL  %s\n", thread_id, name);
        g_failures++;
    }
    pthread_mutex_unlock(&g_stats_lock);
}

#define REPORT(thread_id, name, rc) report_result(thread_id, name, rc)

/* ── multi-threaded tests ───────────────────────────────────────────────── */

static int test_concurrent_put_get(int thread_id)
{
    char key[64], val[64], buf[64];
    int i;

    for (i = 0; i < NUM_KEYS_PER_THREAD; i++)
    {
        snprintf(key, sizeof(key), "thread%d_key_%d", thread_id, i);
        snprintf(val, sizeof(val), "thread%d_val_%d", thread_id, i);
        memset(buf, 0, sizeof(buf));

        /* Put value */
        if (kvfs_put(key, val, strlen(val)) != (ssize_t) strlen(val))
        {
            fprintf(stderr, "[T%d] Put failed for key %s\n", thread_id, key);
            return -1;
        }

        /* Get value immediately */
        if (kvfs_get(key, buf) != (ssize_t) strlen(val))
        {
            fprintf(stderr, "[T%d] Get failed for key %s\n", thread_id, key);
            return -1;
        }

        /* Verify value */
        if (memcmp(buf, val, strlen(val)) != 0)
        {
            fprintf(stderr, "[T%d] Value mismatch for key %s\n", thread_id, key);
            return -1;
        }
    }

    return 0;
}

static int test_concurrent_overwrite(int thread_id)
{
    char key[64], val1[64], val2[64], buf[64];
    int i;

    for (i = 0; i < NUM_KEYS_PER_THREAD / 2; i++)
    {
        snprintf(key, sizeof(key), "thread%d_overwrite_%d", thread_id, i);
        snprintf(val1, sizeof(val1), "value1_t%d_%d", thread_id, i);
        snprintf(val2, sizeof(val2), "value2_longer_t%d_%d", thread_id, i);
        memset(buf, 0, sizeof(buf));

        /* Put first value */
        if (kvfs_put(key, val1, strlen(val1)) != (ssize_t) strlen(val1))
        {
            return -1;
        }

        /* Overwrite with longer value */
        if (kvfs_put(key, val2, strlen(val2)) != (ssize_t) strlen(val2))
        {
            return -1;
        }

        /* Get and verify new value */
        if (kvfs_get(key, buf) != (ssize_t) strlen(val2))
        {
            return -1;
        }

        if (memcmp(buf, val2, strlen(val2)) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int test_concurrent_delete(int thread_id)
{
    char key[64], val[64], buf[64];
    int i;

    /* First, put some keys */
    for (i = 0; i < NUM_KEYS_PER_THREAD / 2; i++)
    {
        snprintf(key, sizeof(key), "thread%d_delete_%d", thread_id, i);
        snprintf(val, sizeof(val), "to_delete_t%d_%d", thread_id, i);

        if (kvfs_put(key, val, strlen(val)) != (ssize_t) strlen(val))
        {
            return -1;
        }
    }

    /* Now delete and verify deletion */
    for (i = 0; i < NUM_KEYS_PER_THREAD / 2; i++)
    {
        snprintf(key, sizeof(key), "thread%d_delete_%d", thread_id, i);
        memset(buf, 0, sizeof(buf));

        if (kvfs_del(key) != 0)
        {
            fprintf(stderr, "[T%d] Delete failed for key %s\n", thread_id, key);
            return -1;
        }

        /* Get after delete should fail */
        ssize_t r = kvfs_get(key, buf);
        if (r >= 0)
        {
            fprintf(stderr, "[T%d] Get after delete should fail for key %s\n", thread_id, key);
            return -1;
        }
    }

    return 0;
}

static int test_cross_thread_read(int thread_id)
{
    char key[64], val[64], buf[64];
    int i, j;

    /* Each thread reads keys written by all other threads */
    for (j = 0; j < g_num_threads; j++)
    {
        if (j == thread_id)
            continue; /* Skip own keys */

        for (i = 0; i < NUM_KEYS_PER_THREAD; i++)
        {
            snprintf(key, sizeof(key), "thread%d_key_%d", j, i);
            snprintf(val, sizeof(val), "thread%d_val_%d", j, i);
            memset(buf, 0, sizeof(buf));

            ssize_t ret = kvfs_get(key, buf);
            if (ret <= 0)
            {
                fprintf(stderr, "[T%d] Failed to read key from thread %d: %s\n", thread_id, j, key);
                return -1;
            }

            /* CRITICAL FIX: Verify content matches what was written */
            if (ret != (ssize_t) strlen(val))
            {
                fprintf(stderr,
                        "[T%d] Size mismatch for key from thread %d: expected %zu, got %zd\n",
                        thread_id, j, strlen(val), ret);
                return -1;
            }

            if (memcmp(buf, val, strlen(val)) != 0)
            {
                fprintf(stderr, "[T%d] Content mismatch for key from thread %d: %s\n", thread_id, j,
                        key);
                return -1;
            }
        }
    }

    return 0;
}

static int test_concurrent_same_key(int thread_id)
{
    char key[64], val[64], buf[64];
    int i;

    /* All threads write to the same key with their own values */
    snprintf(key, sizeof(key), "shared_key");

    for (i = 0; i < NUM_KEYS_PER_THREAD / 2; i++)
    {
        snprintf(val, sizeof(val), "thread%d_value_%d", thread_id, i);
        memset(buf, 0, sizeof(buf));

        /* Write shared key */
        if (kvfs_put(key, val, strlen(val)) != (ssize_t) strlen(val))
        {
            fprintf(stderr, "[T%d] Put failed for shared key\n", thread_id);
            return -1;
        }

        /* Immediately read back */
        ssize_t ret = kvfs_get(key, buf);
        if (ret <= 0)
        {
            fprintf(stderr, "[T%d] Get failed for shared key\n", thread_id);
            return -1;
        }

        /* Verify we got back what we just wrote (or another thread's write) */
        /* Since multiple threads write the same key, we just verify the read succeeded */
        if (ret != (ssize_t) strlen(val) && memcmp(buf, val, strlen(val)) != 0)
        {
            /* It's OK if we read another thread's value, but we should read SOMETHING valid */
            int valid = 0;
            for (int t = 0; t < g_num_threads; t++)
            {
                for (int v = 0; v < NUM_KEYS_PER_THREAD / 2; v++)
                {
                    char expected[64];
                    snprintf(expected, sizeof(expected), "thread%d_value_%d", t, v);
                    if (ret == (ssize_t) strlen(expected) &&
                        memcmp(buf, expected, strlen(expected)) == 0)
                    {
                        valid = 1;
                        break;
                    }
                }
                if (valid)
                    break;
            }
            if (!valid)
            {
                fprintf(stderr, "[T%d] Invalid content read from shared key\n", thread_id);
                return -1;
            }
        }
    }

    return 0;
}

/* ── thread entry point ─────────────────────────────────────────────────── */

static void* thread_worker(void* arg)
{
    int thread_id = (intptr_t) arg;
    int rc;

    printf("[T%d] Thread started, running tests...\n", thread_id);

    /* Test 1: Concurrent put/get */
    rc = test_concurrent_put_get(thread_id);
    REPORT(thread_id, "concurrent_put_get", rc);
    if (rc != 0)
        return NULL;

    /* Test 2: Concurrent overwrite */
    rc = test_concurrent_overwrite(thread_id);
    REPORT(thread_id, "concurrent_overwrite", rc);
    if (rc != 0)
        return NULL;

    /* Test 3: Concurrent delete */
    rc = test_concurrent_delete(thread_id);
    REPORT(thread_id, "concurrent_delete", rc);
    if (rc != 0)
        return NULL;

    /* Test 4: Concurrent access to same key */
    rc = test_concurrent_same_key(thread_id);
    REPORT(thread_id, "concurrent_same_key", rc);
    if (rc != 0)
        return NULL;

    /* Test 5: Cross-thread reads (with content verification) */
    sleep(1); /* Wait for all threads to complete their writes */
    rc = test_cross_thread_read(thread_id);
    REPORT(thread_id, "cross_thread_read", rc);

    printf("[T%d] Thread completed\n", thread_id);
    return NULL;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    char ch;
    int ret;
    int i;
    pthread_t threads[MAX_THREADS];

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:t:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            xdst = optarg;
            break;
        case 't':
            g_num_threads = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (g_num_threads < 1 || g_num_threads > MAX_THREADS)
    {
        fprintf(stderr, "Invalid thread count: %d (1-%d allowed)\n", g_num_threads, MAX_THREADS);
        return 1;
    }

    /* Initialize kvfs */
    ret = kvfs_init2(xdst);
    if (ret < 0)
    {
        fprintf(stderr, "kvfs_init2 failed: %d\n", ret);
        return 1;
    }

    ret = kvfs_mount("/tmp/stamnos_pool", 0);
    if (ret < 0)
    {
        fprintf(stderr, "kvfs_mount failed: %d\n", ret);
        return 1;
    }

    printf("\n=== kvfs Multi-Threaded Tests ===\n");
    printf("Running with %d thread(s)\n", g_num_threads);
    printf("Operations per thread: %d\n", NUM_KEYS_PER_THREAD);
    printf("\n");

    /* Create and join threads */
    for (i = 0; i < g_num_threads; i++)
    {
        ret = pthread_create(&threads[i], NULL, thread_worker, (void*) (intptr_t) i);
        if (ret != 0)
        {
            fprintf(stderr, "Failed to create thread %d: %d\n", i, ret);
            return 1;
        }
    }

    /* Wait for all threads to complete */
    for (i = 0; i < g_num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* Print results */
    printf("\n=== Test Results ===\n");
    printf("Total tests: %d\n", g_total);
    printf("Passed: %d\n", g_total - g_failures);
    printf("Failed: %d\n", g_failures);
    printf("\n%d/%d tests passed\n", g_total - g_failures, g_total);

    kvfs_shutdown();
    return (g_failures > 0) ? 1 : 0;
}
