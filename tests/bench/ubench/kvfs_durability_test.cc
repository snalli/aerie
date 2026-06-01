/**
 * kvfs_durability_test.cc — Data durability and persistence tests
 *
 * Tests that data persists across server restarts and client reconnects.
 * This test is designed to be run in two phases:
 *   Phase 1: Write and sync data (-p write)
 *   Phase 2: Verify data survived restart (-p verify)
 *
 * Usage:  kvfs_durability_test -h <port> -p <write|verify>
 *         kvfs_durability_test -h 10001 -p write    # Write test data
 *         kvfs_durability_test -h 10001 -p verify   # Verify after restart
 *
 * Returns 0 if successful, 1 if data lost or corrupted.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_TEST_KEYS 100
#define VALUE_SIZE 256
#define VALUE_PATTERN 0x42

typedef enum
{
    PHASE_WRITE,
    PHASE_VERIFY
} test_phase_t;

static int g_failures = 0;

/* ── write phase: create test data ──────────────────────────────────────── */

static int phase_write(void)
{
    char key[64], val[VALUE_SIZE];
    int i;

    printf("\n=== Write Phase ===\n");
    printf("Writing %d test keys with pattern 0x%02X...\n", NUM_TEST_KEYS, VALUE_PATTERN);

    memset(val, VALUE_PATTERN, sizeof(val));

    for (i = 0; i < NUM_TEST_KEYS; i++)
    {
        snprintf(key, sizeof(key), "durability_test_%d", i);

        ssize_t ret = kvfs_put(key, val, sizeof(val));
        if (ret != (ssize_t) sizeof(val))
        {
            fprintf(stderr, "Write failed at key %d: %zd\n", i, ret);
            return -1;
        }

        if ((i + 1) % 25 == 0)
        {
            printf("  Written %d keys\n", i + 1);
        }
    }

    printf("✓ All %d keys written successfully\n", NUM_TEST_KEYS);

    /* Sync to ensure durability */
    printf("\nSyncing data to persistent storage...\n");
    int ret = kvfs_sync();
    if (ret != 0)
    {
        fprintf(stderr, "Sync failed: %d\n", ret);
        return -1;
    }
    printf("✓ Sync completed\n");

    printf("\n*** SERVER RESTART REQUIRED ***\n");
    printf("Kill the kvfs server and restart it, then run:\n");
    printf("  kvfs_durability_test -h <port> -p verify\n");

    return 0;
}

/* ── verify phase: check data survived restart ──────────────────────────── */

static int phase_verify(void)
{
    char key[64], buf[VALUE_SIZE];
    int i;
    int corrupted = 0;

    printf("\n=== Verify Phase ===\n");
    printf("Verifying %d test keys survived server restart...\n", NUM_TEST_KEYS);

    for (i = 0; i < NUM_TEST_KEYS; i++)
    {
        snprintf(key, sizeof(key), "durability_test_%d", i);
        memset(buf, 0, sizeof(buf));

        ssize_t ret = kvfs_get(key, buf);

        if (ret < 0)
        {
            fprintf(stderr, "❌ Key %d LOST: not found after restart\n", i);
            g_failures++;
            corrupted++;
            continue;
        }

        if (ret != (ssize_t) sizeof(buf))
        {
            fprintf(stderr, "❌ Key %d SIZE MISMATCH: expected %zu, got %zd\n", i, sizeof(buf),
                    ret);
            g_failures++;
            corrupted++;
            continue;
        }

        /* Verify pattern */
        int pattern_ok = 1;
        for (size_t j = 0; j < sizeof(buf); j++)
        {
            if ((unsigned char) buf[j] != VALUE_PATTERN)
            {
                pattern_ok = 0;
                break;
            }
        }

        if (!pattern_ok)
        {
            fprintf(stderr, "❌ Key %d CORRUPTED: pattern mismatch\n", i);
            g_failures++;
            corrupted++;
        }

        if ((i + 1) % 25 == 0)
        {
            printf("  Verified %d keys\n", i + 1);
        }
    }

    printf("\n=== Durability Test Results ===\n");
    printf("Total keys verified: %d\n", NUM_TEST_KEYS);
    printf("Keys survived: %d\n", NUM_TEST_KEYS - corrupted);
    printf("Data lost/corrupted: %d\n", corrupted);

    if (corrupted == 0)
    {
        printf("\n✅ DURABILITY TEST PASSED: All data survived restart\n");
        return 0;
    }
    else
    {
        printf("\n❌ DURABILITY TEST FAILED: Data loss or corruption detected\n");
        return -1;
    }
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    const char* phase_str = NULL;
    test_phase_t phase;
    char ch;
    int ret;

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:p:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            xdst = optarg;
            break;
        case 'p':
            phase_str = optarg;
            break;
        default:
            break;
        }
    }

    if (!phase_str)
    {
        fprintf(stderr, "Usage: kvfs_durability_test -h <port> -p <write|verify>\n");
        return 1;
    }

    if (strcmp(phase_str, "write") == 0)
    {
        phase = PHASE_WRITE;
    }
    else if (strcmp(phase_str, "verify") == 0)
    {
        phase = PHASE_VERIFY;
    }
    else
    {
        fprintf(stderr, "Invalid phase: %s (use 'write' or 'verify')\n", phase_str);
        return 1;
    }

    printf("=== KVFS Durability Test ===\n");
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

    /* Run selected phase */
    switch (phase)
    {
    case PHASE_WRITE:
        ret = phase_write();
        break;
    case PHASE_VERIFY:
        ret = phase_verify();
        break;
    default:
        ret = -1;
    }

    kvfs_shutdown();

    return (ret < 0 || g_failures > 0) ? 1 : 0;
}
