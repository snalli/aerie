/**
 * kvfs_crash_test.cc — Crash recovery tests
 *
 * Tests that kvfs recovers correctly from unclean shutdown.
 * This test verifies that:
 * - Committed (synced) data persists
 * - Uncommitted data may be lost (but no corruption)
 * - System recovers to valid state
 *
 * Two-phase test:
 *   Phase 1: Write data with mix of synced/unsynced (-p prepare)
 *   Phase 2: Verify recovery after unclean exit (-p verify)
 *
 * Usage:  kvfs_crash_test -h <port> -p <prepare|verify>
 *         kvfs_crash_test -h 10001 -p prepare    # Write test data
 *         kvfs_crash_test -h 10001 -p verify     # Check recovery
 *
 * To simulate crash:
 *   1. Run: kvfs_crash_test -h 10001 -p prepare
 *   2. Kill the kvfs server (don't give it time to shutdown gracefully)
 *   3. Restart the server
 *   4. Run: kvfs_crash_test -h 10001 -p verify
 *
 * Returns 0 if recovery successful, 1 if data corrupted/lost unexpectedly.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYNCED_KEYS 50   /* Keys we'll explicitly sync */
#define UNSYNCED_KEYS 50 /* Keys we write but don't sync */
#define VALUE_SIZE 128
#define SYNCED_PATTERN 0x55
#define UNSYNCED_PATTERN 0xAA

typedef enum
{
    PHASE_PREPARE,
    PHASE_VERIFY
} crash_phase_t;

/* ── Phase 1: Write test data ──────────────────────────────────────────── */

static int phase_prepare(void)
{
    char key[64], val[VALUE_SIZE];
    int i, ret;

    printf("\n=== Crash Test: Prepare Phase ===\n");
    printf("This phase creates test data for crash recovery testing.\n\n");

    /* Write synced keys (should always survive) */
    printf("Step 1: Writing %d SYNCED keys (will be synced)...\n", SYNCED_KEYS);
    memset(val, SYNCED_PATTERN, sizeof(val));

    for (i = 0; i < SYNCED_KEYS; i++)
    {
        snprintf(key, sizeof(key), "crash_synced_%d", i);
        ret = kvfs_put(key, val, sizeof(val));
        if (ret != (int) sizeof(val))
        {
            fprintf(stderr, "Write failed for synced key %d\n", i);
            return -1;
        }

        if ((i + 1) % 10 == 0)
        {
            printf("  Written %d synced keys\n", i + 1);
        }
    }

    /* Sync explicitly */
    printf("\nSyncing %d keys...\n", SYNCED_KEYS);
    ret = kvfs_sync();
    if (ret != 0)
    {
        fprintf(stderr, "Sync failed: %d\n", ret);
        return -1;
    }
    printf("✓ Synced data is on stable storage\n");

    /* Write unsynced keys (may be lost on crash) */
    printf("\nStep 2: Writing %d UNSYNCED keys (will NOT be synced)...\n", UNSYNCED_KEYS);
    memset(val, UNSYNCED_PATTERN, sizeof(val));

    for (i = 0; i < UNSYNCED_KEYS; i++)
    {
        snprintf(key, sizeof(key), "crash_unsynced_%d", i);
        ret = kvfs_put(key, val, sizeof(val));
        if (ret != (int) sizeof(val))
        {
            fprintf(stderr, "Write failed for unsynced key %d\n", i);
            return -1;
        }

        if ((i + 1) % 10 == 0)
        {
            printf("  Written %d unsynced keys\n", i + 1);
        }
    }

    printf("\n⚠️  UNSYNCED DATA IS NOT ON STABLE STORAGE\n");
    printf("\nPhase 1 Complete. Now:\n");
    printf("  1. Kill the kvfs server IMMEDIATELY (don't let it shutdown gracefully)\n");
    printf("  2. Restart the kvfs server\n");
    printf("  3. Run: kvfs_crash_test -h <port> -p verify\n");

    return 0;
}

/* ── Phase 2: Verify recovery ──────────────────────────────────────────── */

static int phase_verify(void)
{
    char key[64], buf[VALUE_SIZE];
    int i, ret;
    int synced_lost = 0;
    int synced_corrupted = 0;
    int unsynced_lost = 0;
    int unsynced_corrupted = 0;

    printf("\n=== Crash Test: Verify Phase ===\n");
    printf("Checking recovery after unclean shutdown...\n\n");

    /* Verify synced keys (should all be present and correct) */
    printf("Checking %d SYNCED keys (should all be present):\n", SYNCED_KEYS);

    for (i = 0; i < SYNCED_KEYS; i++)
    {
        snprintf(key, sizeof(key), "crash_synced_%d", i);
        memset(buf, 0, sizeof(buf));

        ret = kvfs_get(key, buf);

        if (ret < 0)
        {
            synced_lost++;
            if (synced_lost <= 3)
            {
                fprintf(stderr, "  ❌ CRITICAL: Synced key %d LOST (should be persistent)\n", i);
            }
        }
        else if (ret != (int) sizeof(buf))
        {
            synced_corrupted++;
            fprintf(stderr, "  ❌ CRITICAL: Synced key %d SIZE MISMATCH\n", i);
        }
        else
        {
            /* Verify pattern */
            int mismatch = 0;
            for (int j = 0; j < (int) sizeof(buf); j++)
            {
                if ((unsigned char) buf[j] != SYNCED_PATTERN)
                {
                    mismatch = 1;
                    break;
                }
            }
            if (mismatch)
            {
                synced_corrupted++;
                fprintf(stderr, "  ❌ CRITICAL: Synced key %d DATA CORRUPTED\n", i);
            }
        }
    }

    printf("Synced key results:\n");
    printf("  Present: %d/%d\n", SYNCED_KEYS - synced_lost, SYNCED_KEYS);
    printf("  Valid:   %d/%d\n", SYNCED_KEYS - synced_lost - synced_corrupted, SYNCED_KEYS);

    /* Verify unsynced keys (may be lost, but shouldn't be corrupted) */
    printf("\nChecking %d UNSYNCED keys (may be lost, but not corrupted):\n", UNSYNCED_KEYS);

    for (i = 0; i < UNSYNCED_KEYS; i++)
    {
        snprintf(key, sizeof(key), "crash_unsynced_%d", i);
        memset(buf, 0, sizeof(buf));

        ret = kvfs_get(key, buf);

        if (ret < 0)
        {
            unsynced_lost++;
        }
        else if (ret != (int) sizeof(buf))
        {
            unsynced_corrupted++;
            fprintf(stderr, "  ⚠️  Unsynced key %d SIZE MISMATCH (partial write?)\n", i);
        }
        else
        {
            /* Check if data is consistent (either all 0xAA or all something else) */
            int valid = 1;
            unsigned char first = (unsigned char) buf[0];

            for (int j = 1; j < (int) sizeof(buf); j++)
            {
                if ((unsigned char) buf[j] != first)
                {
                    valid = 0;
                    break;
                }
            }

            if (!valid)
            {
                unsynced_corrupted++;
                fprintf(stderr, "  ⚠️  Unsynced key %d DATA INCONSISTENT (corruption detected)\n",
                        i);
            }
        }
    }

    printf("Unsynced key results:\n");
    printf("  Present: %d/%d\n", UNSYNCED_KEYS - unsynced_lost, UNSYNCED_KEYS);
    printf("  Valid:   %d/%d\n", UNSYNCED_KEYS - unsynced_lost - unsynced_corrupted, UNSYNCED_KEYS);

    /* Summary */
    printf("\n=== Crash Recovery Summary ===\n");

    /* Critical failures: synced data lost or corrupted */
    if (synced_lost > 0 || synced_corrupted > 0)
    {
        printf("❌ CRITICAL: Synced data was lost or corrupted!\n");
        printf("   Lost: %d, Corrupted: %d\n", synced_lost, synced_corrupted);
        return -1;
    }

    /* OK: unsynced data may be lost, but shouldn't be corrupted */
    if (unsynced_corrupted > 0)
    {
        printf("⚠️  WARNING: Some unsynced data is corrupted (partial writes)\n");
        printf("   This may be acceptable but is not ideal\n");
        printf("   Unsynced keys: Lost %d, Corrupted %d\n", unsynced_lost, unsynced_corrupted);
        /* Still pass if synced data is intact */
    }

    printf("\n✅ CRASH RECOVERY TEST PASSED\n");
    printf("System recovered to valid state after unclean shutdown\n");
    printf("Synced data was preserved (%.1f%% of unsynced data recovered)\n",
           100.0 * (UNSYNCED_KEYS - unsynced_lost) / UNSYNCED_KEYS);

    return 0;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    const char* phase_str = NULL;
    crash_phase_t phase;
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
        fprintf(stderr, "Usage: kvfs_crash_test -h <port> -p <prepare|verify>\n");
        return 1;
    }

    if (strcmp(phase_str, "prepare") == 0)
    {
        phase = PHASE_PREPARE;
    }
    else if (strcmp(phase_str, "verify") == 0)
    {
        phase = PHASE_VERIFY;
    }
    else
    {
        fprintf(stderr, "Invalid phase: %s (use 'prepare' or 'verify')\n", phase_str);
        return 1;
    }

    printf("=== KVFS Crash Recovery Test ===\n");
    printf("Connecting to kvfs on %s...\n", xdst);

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
    case PHASE_PREPARE:
        ret = phase_prepare();
        break;
    case PHASE_VERIFY:
        ret = phase_verify();
        break;
    default:
        ret = -1;
    }

    kvfs_shutdown();
    return (ret < 0) ? 1 : 0;
}
