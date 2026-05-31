/**
 * kvfs_resilience_test.cc — Resilience and recovery tests
 *
 * Tests kvfs behavior under adverse conditions:
 * - Client disconnect/reconnect
 * - Concurrent deletes on same key
 * - Data corruption detection
 * - Partial write detection
 *
 * Usage:  kvfs_resilience_test -h <port> -t <test_name>
 *         kvfs_resilience_test -h 10001 -t client_reconnect
 *         kvfs_resilience_test -h 10001 -t concurrent_delete
 *         kvfs_resilience_test -h 10001 -t corruption
 *         kvfs_resilience_test -h 10001 -t atomicity
 *
 * Returns 0 if test passes, 1 if failure detected.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_KEY "resilience_test_key"
#define TEST_VALUE "test_value_data"
#define EXPECTED_SIZE strlen(TEST_VALUE)

static int g_failures __attribute__((unused)) = 0;

/* ── Test 1: Client Disconnect/Reconnect ───────────────────────────────── */

static int test_client_reconnect(void)
{
    char buf[256];
    int ret;

    printf("\n=== Test: Client Disconnect/Reconnect ===\n");

    /* Initial write */
    printf("Phase 1: Initial write...\n");
    ret = kvfs_put(TEST_KEY, TEST_VALUE, EXPECTED_SIZE);
    if (ret != (int) EXPECTED_SIZE)
    {
        fprintf(stderr, "Initial write failed\n");
        return -1;
    }
    printf("✓ Data written\n");

    /* Read to verify */
    memset(buf, 0, sizeof(buf));
    ret = kvfs_get(TEST_KEY, buf);
    if (ret != (int) EXPECTED_SIZE || memcmp(buf, TEST_VALUE, EXPECTED_SIZE) != 0)
    {
        fprintf(stderr, "Initial read verification failed\n");
        return -1;
    }
    printf("✓ Verified after write\n");

    /* Disconnect (shutdown client) */
    printf("\nPhase 2: Client shutdown...\n");
    kvfs_shutdown();
    printf("✓ Client disconnected\n");

    /* Reconnect (reinitialize) */
    printf("\nPhase 3: Client reconnect...\n");
    ret = kvfs_init2("10001"); /* Hardcoded for this test */
    if (ret < 0)
    {
        fprintf(stderr, "Reconnect init failed: %d\n", ret);
        return -1;
    }

    ret = kvfs_mount("/tmp/stamnos_pool", 0);
    if (ret < 0)
    {
        fprintf(stderr, "Reconnect mount failed: %d\n", ret);
        return -1;
    }
    printf("✓ Client reconnected\n");

    /* Read after reconnect */
    printf("\nPhase 4: Read after reconnect...\n");
    memset(buf, 0, sizeof(buf));
    ret = kvfs_get(TEST_KEY, buf);
    if (ret != (int) EXPECTED_SIZE || memcmp(buf, TEST_VALUE, EXPECTED_SIZE) != 0)
    {
        fprintf(stderr, "Post-reconnect read failed\n");
        return -1;
    }
    printf("✓ Data available after reconnect\n");

    printf("\n✅ Client reconnect test PASSED\n");
    return 0;
}

/* ── Test 2: Concurrent Delete on Same Key ────────────────────────────── */

typedef struct
{
    int thread_id;
    int result;
} delete_thread_t;

static void* delete_thread(void* arg)
{
    delete_thread_t* dt = (delete_thread_t*) arg;
    int ret = kvfs_del(TEST_KEY);
    dt->result = ret;
    return NULL;
}

static int test_concurrent_delete(void)
{
    char buf[256];
    int i, ret;
    pthread_t threads[4];
    delete_thread_t delete_threads[4];

    printf("\n=== Test: Concurrent Delete on Same Key ===\n");

    /* Write initial data */
    printf("Writing test data...\n");
    ret = kvfs_put(TEST_KEY, TEST_VALUE, EXPECTED_SIZE);
    if (ret != (int) EXPECTED_SIZE)
    {
        fprintf(stderr, "Initial write failed\n");
        return -1;
    }
    printf("✓ Data written\n");

    /* Spawn multiple threads to delete same key */
    printf("\nSpawning 4 threads to delete same key...\n");
    for (i = 0; i < 4; i++)
    {
        delete_threads[i].thread_id = i;
        delete_threads[i].result = -999;
        pthread_create(&threads[i], NULL, delete_thread, &delete_threads[i]);
    }

    /* Wait for all threads */
    for (i = 0; i < 4; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* Check results */
    printf("\nDelete results from 4 threads:\n");
    int successful_deletes = 0;
    for (i = 0; i < 4; i++)
    {
        printf("  Thread %d: %s\n", i, delete_threads[i].result == 0 ? "SUCCESS (0)" : "ERROR");
        if (delete_threads[i].result == 0)
            successful_deletes++;
    }

    /* At least one should succeed, others should fail or succeed consistently */
    if (successful_deletes == 0)
    {
        fprintf(stderr, "❌ No threads successfully deleted key\n");
        return -1;
    }
    printf("✓ %d thread(s) successfully deleted key\n", successful_deletes);

    /* Verify key is gone */
    printf("\nVerifying key is deleted...\n");
    memset(buf, 0, sizeof(buf));
    ret = kvfs_get(TEST_KEY, buf);
    if (ret >= 0)
    {
        fprintf(stderr, "❌ Key still exists after delete\n");
        return -1;
    }
    printf("✓ Key confirmed deleted\n");

    printf("\n✅ Concurrent delete test PASSED\n");
    return 0;
}

/* ── Test 3: Corruption Detection ──────────────────────────────────────── */

static int test_corruption_detection(void)
{
    char key[64], val[256], buf[256];
    int i;

    printf("\n=== Test: Data Corruption Detection ===\n");

    /* Write data with specific pattern */
    printf("Writing data with checksum pattern...\n");
    snprintf(key, sizeof(key), "corruption_test_1");
    memset(val, 0xAB, sizeof(val));

    int ret = kvfs_put(key, val, sizeof(val));
    if (ret != (int) sizeof(val))
    {
        fprintf(stderr, "Write failed\n");
        return -1;
    }
    printf("✓ Data written (pattern 0xAB)\n");

    /* Read back and verify pattern */
    printf("\nReading and verifying pattern...\n");
    memset(buf, 0, sizeof(buf));
    ret = kvfs_get(key, buf);

    if (ret != (int) sizeof(val))
    {
        fprintf(stderr, "❌ Size mismatch: expected %zu, got %d\n", sizeof(val), ret);
        return -1;
    }

    /* Check pattern */
    int corrupted = 0;
    for (i = 0; i < (int) sizeof(val); i++)
    {
        if ((unsigned char) buf[i] != 0xAB)
        {
            if (corrupted == 0)
            {
                fprintf(stderr, "❌ Corruption detected at byte %d: expected 0xAB, got 0x%02X\n", i,
                        (unsigned char) buf[i]);
            }
            corrupted++;
        }
    }

    if (corrupted > 0)
    {
        fprintf(stderr, "Total corrupted bytes: %d/%zu\n", corrupted, sizeof(val));
        return -1;
    }

    printf("✓ All %zu bytes verified (pattern 0xAB)\n", sizeof(val));

    printf("\n✅ Corruption detection test PASSED\n");
    return 0;
}

/* ── Test 4: Atomicity (Partial Write Detection) ────────────────────────── */

static int test_atomicity(void)
{
    char key[64], small_val[10], large_val[1024], buf[1024];
    int ret;

    printf("\n=== Test: Write Atomicity ===\n");

    snprintf(key, sizeof(key), "atomicity_test");

    /* Write small value */
    printf("Phase 1: Writing small value (10 bytes)...\n");
    memset(small_val, 0x11, sizeof(small_val));
    ret = kvfs_put(key, small_val, sizeof(small_val));
    if (ret != (int) sizeof(small_val))
    {
        fprintf(stderr, "Small write failed\n");
        return -1;
    }
    printf("✓ Small value written\n");

    /* Overwrite with large value */
    printf("\nPhase 2: Overwriting with large value (1024 bytes)...\n");
    memset(large_val, 0x22, sizeof(large_val));
    ret = kvfs_put(key, large_val, sizeof(large_val));
    if (ret != (int) sizeof(large_val))
    {
        fprintf(stderr, "Large write failed\n");
        return -1;
    }
    printf("✓ Large value written\n");

    /* Read and verify - should be either all small or all large, never partial */
    printf("\nPhase 3: Reading and verifying atomicity...\n");
    memset(buf, 0, sizeof(buf));
    ret = kvfs_get(key, buf);

    if (ret == (int) sizeof(small_val))
    {
        fprintf(stderr, "⚠️  WARNING: Got small value after large write (possible partial write)\n");
        return -1;
    }

    if (ret != (int) sizeof(large_val))
    {
        fprintf(stderr, "❌ Unexpected size: expected %zu, got %d\n", sizeof(large_val), ret);
        return -1;
    }

    /* Verify all bytes are 0x22 (large value pattern) */
    int mismatches = 0;
    for (int i = 0; i < (int) sizeof(large_val); i++)
    {
        if ((unsigned char) buf[i] != 0x22)
        {
            mismatches++;
        }
    }

    if (mismatches > 0)
    {
        fprintf(stderr, "❌ Data pattern mismatch: %d bytes incorrect\n", mismatches);
        return -1;
    }

    printf("✓ Verified atomic write (all 1024 bytes are new value)\n");

    printf("\n✅ Atomicity test PASSED\n");
    return 0;
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    const char* test_name = NULL;
    char ch;
    int ret;

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:t:")) != -1)
    {
        switch (ch)
        {
        case 'h':
            xdst = optarg;
            break;
        case 't':
            test_name = optarg;
            break;
        default:
            break;
        }
    }

    if (!test_name)
    {
        fprintf(stderr, "Usage: kvfs_resilience_test -h <port> -t <test_name>\n");
        fprintf(stderr, "Tests: client_reconnect, concurrent_delete, corruption, atomicity\n");
        return 1;
    }

    printf("=== KVFS Resilience Tests ===\n");
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

    /* Run selected test */
    if (strcmp(test_name, "client_reconnect") == 0)
    {
        ret = test_client_reconnect();
    }
    else if (strcmp(test_name, "concurrent_delete") == 0)
    {
        ret = test_concurrent_delete();
    }
    else if (strcmp(test_name, "corruption") == 0)
    {
        ret = test_corruption_detection();
    }
    else if (strcmp(test_name, "atomicity") == 0)
    {
        ret = test_atomicity();
    }
    else
    {
        fprintf(stderr, "Unknown test: %s\n", test_name);
        ret = -1;
    }

    kvfs_shutdown();
    return (ret < 0) ? 1 : 0;
}
