/**
 * kvfs_space_test.cc — Space exhaustion test for kvfs
 *
 * Tests kvfs behavior when storage space is exhausted.
 * Monitors error handling and recovery mechanisms.
 *
 * Usage:  kvfs_space_test -h <port> [-v <value_size>] [-m <max_size_mb>]
 *         kvfs_space_test -h 10001                       # default: 1KB values
 *         kvfs_space_test -h 10001 -v 1024 -m 50        # 1KB values, 50MB limit
 *
 * Returns 0 if space exhaustion is handled gracefully, 1 if errors occur.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── test configuration ───────────────────────────────────────────────────── */

#define DEFAULT_VALUE_SIZE (1024) /* 1 KB per value */
#define DEFAULT_MAX_SIZE_MB (128) /* Stop at 128 MB */
#define VALUE_PATTERN 0xAB

static int g_failures = 0;
static int g_total_tests = 0;

typedef struct
{
    size_t value_size;
    size_t max_size_mb;
    size_t bytes_written;
    int num_keys;
    int failed_writes;
    int successful_writes;
} space_test_stats_t;

#define CHECK(expr, msg)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "CHECK FAILED: %s\n", msg);                                            \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

/* ── space exhaustion test ──────────────────────────────────────────────── */

static int test_space_exhaustion(space_test_stats_t* stats)
{
    char* value_buf;
    char key[64];
    int key_num = 0;
    size_t bytes_to_write = stats->max_size_mb * 1024 * 1024;
    ssize_t ret;

    printf("\n=== Space Exhaustion Test ===\n");
    printf("Value size: %zu bytes\n", stats->value_size);
    printf("Max space: %zu MB (%zu bytes)\n", stats->max_size_mb, bytes_to_write);
    printf("\nWriting data until space is exhausted...\n\n");

    /* Allocate value buffer */
    value_buf = (char*) malloc(stats->value_size);
    if (!value_buf)
    {
        fprintf(stderr, "Failed to allocate value buffer\n");
        return -1;
    }

    /* Fill with pattern */
    memset(value_buf, VALUE_PATTERN, stats->value_size);

    /* Write until space exhausted */
    while (stats->bytes_written < bytes_to_write)
    {
        snprintf(key, sizeof(key), "space_test_%d", key_num);

        ret = kvfs_put(key, value_buf, stats->value_size);

        if (ret > 0)
        {
            /* Successful write */
            stats->bytes_written += ret;
            stats->num_keys++;
            stats->successful_writes++;

            /* Progress indicator */
            if (stats->successful_writes % 10 == 0)
            {
                size_t percent = (stats->bytes_written * 100) / bytes_to_write;
                printf("  [%3zu%%] Written: %zu bytes, %d keys\n", percent, stats->bytes_written,
                       stats->num_keys);
            }
        }
        else
        {
            /* Failed write - space exhausted or error */
            stats->failed_writes++;

            if (ret < 0)
            {
                printf("\n❌ Write failed at key %d: error %zd\n", key_num, ret);
                printf("   Bytes written before failure: %zu bytes\n", stats->bytes_written);
                printf("   Number of successful writes: %d\n", stats->successful_writes);
                printf("   Failure is expected (space exhausted)\n\n");
                break;
            }
            else
            {
                /* Partial write? */
                printf("⚠️  Partial write: requested %zu, got %zd\n", stats->value_size, ret);
                stats->bytes_written += ret;
                stats->num_keys++;
                stats->successful_writes++;
            }
        }

        key_num++;

        /* Safety limit to prevent infinite loops */
        if (key_num > 1000000)
        {
            printf("❌ ERROR: Too many keys written (>1M), aborting\n");
            free(value_buf);
            return -1;
        }
    }

    free(value_buf);
    return 0;
}

/* ── recovery test after space exhaustion ──────────────────────────────── */

static int test_recovery_after_exhaustion(space_test_stats_t* stats __attribute__((unused)))
{
    char key[64], val[256], buf[256];
    int attempts = 0;
    ssize_t ret;

    printf("\n=== Recovery Test After Exhaustion ===\n");
    printf("Attempting to write new keys after space exhaustion...\n\n");

    /* Try to write a few more keys (should fail) */
    for (attempts = 0; attempts < 5; attempts++)
    {
        snprintf(key, sizeof(key), "recovery_fail_%d", attempts);
        snprintf(val, sizeof(val), "should_fail_%d", attempts);

        ret = kvfs_put(key, val, strlen(val));

        if (ret > 0)
        {
            printf("  [Attempt %d] ✓ Write succeeded (unexpected)\n", attempts);
        }
        else if (ret < 0)
        {
            printf("  [Attempt %d] ✗ Write failed (expected): error %zd\n", attempts, ret);
        }
        else
        {
            printf("  [Attempt %d] ? Partial write\n", attempts);
        }
    }

    /* Try to read existing keys (should work) */
    printf("\nTesting reads on existing keys...\n");
    snprintf(key, sizeof(key), "space_test_0");
    memset(buf, 0, sizeof(buf));

    ret = kvfs_get(key, buf);
    if (ret > 0)
    {
        printf("  ✓ Read existing key succeeded (ret=%zd bytes)\n", ret);

        /* Verify pattern */
        int verified = 1;
        for (int i = 0; i < ret; i++)
        {
            if ((unsigned char) buf[i] != VALUE_PATTERN)
            {
                verified = 0;
                break;
            }
        }

        if (verified)
        {
            printf("  ✓ Data pattern verified correctly\n");
        }
        else
        {
            printf("  ❌ Data pattern CORRUPTED\n");
            return -1;
        }
    }
    else
    {
        printf("  ❌ Failed to read existing key\n");
        return -1;
    }

    return 0;
}

/* ── sync/durability test ──────────────────────────────────────────────── */

static int test_sync_after_exhaustion(space_test_stats_t* stats __attribute__((unused)))
{
    int ret;

    printf("\n=== Sync/Durability Test ===\n");
    printf("Testing sync after space exhaustion...\n\n");

    ret = kvfs_sync();
    if (ret == 0)
    {
        printf("  ✓ kvfs_sync() succeeded\n");
    }
    else
    {
        printf("  ❌ kvfs_sync() failed: %d\n", ret);
        return -1;
    }

    return 0;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    char ch;
    int ret;
    space_test_stats_t stats = {.value_size = DEFAULT_VALUE_SIZE,
                                .max_size_mb = DEFAULT_MAX_SIZE_MB,
                                .bytes_written = 0,
                                .num_keys = 0,
                                .failed_writes = 0,
                                .successful_writes = 0};

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:v:m:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            xdst = optarg;
            break;
        case 'v':
            stats.value_size = atoi(optarg);
            break;
        case 'm':
            stats.max_size_mb = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (stats.value_size < 1 || stats.value_size > 1024 * 1024)
    {
        fprintf(stderr, "Invalid value size: %zu (1 - 1MB)\n", stats.value_size);
        return 1;
    }

    if (stats.max_size_mb < 1)
    {
        fprintf(stderr, "Invalid max size: %zu MB (min 1)\n", stats.max_size_mb);
        return 1;
    }

    /* Initialize kvfs */
    printf("=== KVFS Space Exhaustion Test ===\n\n");
    printf("Connecting to kvfs on host %s...\n", xdst);

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

    printf("✓ Connected and mounted\n");

    /* Run tests */
    ret = test_space_exhaustion(&stats);
    if (ret != 0)
    {
        g_failures++;
    }
    g_total_tests++;

    ret = test_recovery_after_exhaustion(&stats);
    if (ret != 0)
    {
        g_failures++;
    }
    g_total_tests++;

    ret = test_sync_after_exhaustion(&stats);
    if (ret != 0)
    {
        g_failures++;
    }
    g_total_tests++;

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Value size: %zu bytes\n", stats.value_size);
    printf("Max space: %zu MB\n", stats.max_size_mb);
    printf("Total bytes written: %zu bytes (%.2f MB)\n", stats.bytes_written,
           (double) stats.bytes_written / (1024 * 1024));
    printf("Total keys written: %d\n", stats.num_keys);
    printf("Successful writes: %d\n", stats.successful_writes);
    printf("Failed writes: %d\n", stats.failed_writes);
    printf("\nTest results: %d/%d passed\n", g_total_tests - g_failures, g_total_tests);

    kvfs_shutdown();
    return (g_failures > 0) ? 1 : 0;
}
