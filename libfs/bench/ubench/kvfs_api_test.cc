/**
 * kvfs_api_test.cc — correctness unit tests for every kvfs_* API
 *
 * Usage:  kvfs_api_test -h <port>
 *
 * Returns 0 if all tests pass, 1 if any fail.
 */

#include "common/util.h"
#include "kvfs/client/c_api.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── helpers ────────────────────────────────────────────────────────────── */

static int g_failures = 0;
static int g_total = 0;

#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            fprintf(stderr, "  CHECK FAILED at %s:%d: %s\n", __FILE__, __LINE__, #expr);           \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static void report(const char* name, int rc)
{
    g_total++;
    if (rc == 0)
    {
        printf("PASS  %s\n", name);
    }
    else
    {
        printf("FAIL  %s\n", name);
        g_failures++;
    }
}

#define RUN(fn) report(#fn, fn())

/* ── tests ──────────────────────────────────────────────────────────────── */

static int test_put_get()
{
    const char* key = "key1";
    const char* val = "value1";
    char buf[64] = {0};

    CHECK(kvfs_put(key, val, strlen(val)) == (ssize_t) strlen(val));
    CHECK(kvfs_get(key, buf) > 0);
    CHECK(memcmp(buf, val, strlen(val)) == 0);
    return 0;
}

static int test_put_overwrite()
{
    const char* key = "key_ow";
    const char* val1 = "first";
    const char* val2 = "second_longer";
    char buf[64] = {0};

    CHECK(kvfs_put(key, val1, strlen(val1)) == (ssize_t) strlen(val1));
    CHECK(kvfs_put(key, val2, strlen(val2)) == (ssize_t) strlen(val2));
    CHECK(kvfs_get(key, buf) == (ssize_t) strlen(val2));
    CHECK(memcmp(buf, val2, strlen(val2)) == 0);
    return 0;
}

static int test_put_binary()
{
    const char key[] = "binkey";
    const unsigned char val[] = {0x00, 0x01, 0x02, 0xFF, 0xFE};
    char buf[8] = {0};

    CHECK(kvfs_put(key, (const char*) val, sizeof(val)) == (ssize_t) sizeof(val));
    CHECK(kvfs_get(key, buf) == (ssize_t) sizeof(val));
    CHECK(memcmp(buf, val, sizeof(val)) == 0);
    return 0;
}

static int test_del()
{
    const char* key = "key_del";
    const char* val = "to_be_deleted";
    char buf[64] = {0};

    CHECK(kvfs_put(key, val, strlen(val)) == (ssize_t) strlen(val));
    CHECK(kvfs_del(key) == 0);
    /* get after del should fail */
    ssize_t r = kvfs_get(key, buf);
    CHECK(r < 0);
    return 0;
}

static int test_multiple_keys()
{
    char buf[64] = {0};
    for (int i = 0; i < 10; i++)
    {
        char key[16], val[16];
        snprintf(key, sizeof(key), "mkey_%d", i);
        snprintf(val, sizeof(val), "mval_%d", i);
        CHECK(kvfs_put(key, val, strlen(val)) == (ssize_t) strlen(val));
    }
    for (int i = 0; i < 10; i++)
    {
        char key[16], val[16];
        snprintf(key, sizeof(key), "mkey_%d", i);
        snprintf(val, sizeof(val), "mval_%d", i);
        memset(buf, 0, sizeof(buf));
        CHECK(kvfs_get(key, buf) == (ssize_t) strlen(val));
        CHECK(memcmp(buf, val, strlen(val)) == 0);
    }
    return 0;
}

static int test_large_value()
{
    const char* key = "bigkey";
    const int size = 4096;
    char* val = (char*) malloc(size);
    char* buf = (char*) malloc(size);
    int rc = 0;

    if (val == NULL || buf == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        rc = -ENOMEM;
        goto done;
    }

    memset(val, 0xAB, size);
    if (kvfs_put(key, val, size) != size)
    {
        rc = -1;
        goto done;
    }
    if (kvfs_get(key, buf) != size)
    {
        rc = -1;
        goto done;
    }
    if (memcmp(buf, val, size) != 0)
    {
        rc = -1;
        goto done;
    }

done:
    free(val);
    free(buf);
    return rc;
}

static int test_sync()
{
    CHECK(kvfs_put("synckey", "syncval", 7) == 7);
    CHECK(kvfs_sync() == 0);
    return 0;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int opterr;
    const char* xdst = "10000";
    char ch;

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:")) != -1)
    {
        if (ch == 'h')
            xdst = optarg;
    }

    int ret = kvfs_init2(xdst);
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

    printf("=== kvfs API unit tests ===\n");
    RUN(test_put_get);
    RUN(test_put_overwrite);
    RUN(test_put_binary);
    RUN(test_del);
    RUN(test_multiple_keys);
    RUN(test_large_value);
    RUN(test_sync);

    printf("\n%d/%d passed\n", g_total - g_failures, g_total);
    kvfs_shutdown();
    return (g_failures > 0) ? 1 : 0;
}
