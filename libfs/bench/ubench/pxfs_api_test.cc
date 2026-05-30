/**
 * pxfs_api_test.cc — correctness unit tests for every libfs_* API
 *
 * Usage:  pxfs_api_test -h <port>
 *
 * Returns 0 if all tests pass, 1 if any fail.
 * Each test prints PASS or FAIL with a short description.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include "pxfs/client/libfs.h"
#include "common/util.h"

/* ── helpers ────────────────────────────────────────────────────────────── */

static int g_failures = 0;
static int g_total    = 0;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "  CHECK FAILED at %s:%d: %s\n", \
                    __FILE__, __LINE__, #expr); \
            return -1; \
        } \
    } while (0)

static void report(const char* name, int rc)
{
    g_total++;
    if (rc == 0) {
        printf("PASS  %s\n", name);
    } else {
        printf("FAIL  %s\n", name);
        g_failures++;
    }
}

#define RUN(fn) report(#fn, fn())

/* ── individual tests ───────────────────────────────────────────────────── */

static int test_open_create()
{
    int fd = libfs_open2("/pxfs/t_open.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_write_read()
{
    const char *msg = "hello aerie";
    char buf[64]    = {0};
    int  fd         = libfs_open2("/pxfs/t_rw.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    CHECK(libfs_fsync(fd) == 0);   /* flush transaction so read-back sees the data */
    CHECK(libfs_lseek(fd, 0, SEEK_SET) == 0);
    CHECK(libfs_read(fd, buf, strlen(msg)) == (ssize_t)strlen(msg));
    CHECK(memcmp(buf, msg, strlen(msg)) == 0);  /* content validation */
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_pread_pwrite()
{
    const char *a = "AAAA", *b = "BBBB";
    char buf[8]   = {0};
    int  fd       = libfs_open2("/pxfs/t_prw.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_pwrite(fd, a, 4, 0) == 4);
    CHECK(libfs_pwrite(fd, b, 4, 4) == 4);
    CHECK(libfs_fsync(fd) == 0);   /* flush before read-back */
    CHECK(libfs_pread(fd, buf,   4, 0) == 4);
    CHECK(memcmp(buf, a, 4) == 0);
    CHECK(libfs_pread(fd, buf,   4, 4) == 4);
    CHECK(memcmp(buf, b, 4) == 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_lseek()
{
    const char *msg = "0123456789";
    char buf[4]     = {0};
    int  fd         = libfs_open2("/pxfs/t_seek.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, msg, 10) == 10);
    CHECK(libfs_lseek(fd, 5, SEEK_SET) == 5);
    CHECK(libfs_read(fd, buf, 3) == 3);
    CHECK(memcmp(buf, "567", 3) == 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_unlink()
{
    int fd = libfs_open2("/pxfs/t_unlink.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_close(fd) == 0);
    CHECK(libfs_unlink("/pxfs/t_unlink.dat") == 0);
    /* re-open after unlink should create a fresh file */
    fd = libfs_open2("/pxfs/t_unlink.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_mkdir_rmdir()
{
    CHECK(libfs_mkdir("/pxfs/t_dir", 0755) == 0);
    CHECK(libfs_rmdir("/pxfs/t_dir") == 0);
    return 0;
}

static int test_rename()
{
    int fd = libfs_open2("/pxfs/t_rename_src.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, "data", 4) == 4);
    CHECK(libfs_close(fd) == 0);
    CHECK(libfs_rename("/pxfs/t_rename_src.dat", "/pxfs/t_rename_dst.dat") == 0);
    /* dst should now be readable */
    fd = libfs_open("/pxfs/t_rename_dst.dat", O_RDONLY);
    CHECK(fd > 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_link()
{
    int fd = libfs_open2("/pxfs/t_link_orig.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, "link", 4) == 4);
    CHECK(libfs_close(fd) == 0);
    CHECK(libfs_link("/pxfs/t_link_orig.dat", "/pxfs/t_link_hard.dat") == 0);
    fd = libfs_open("/pxfs/t_link_hard.dat", O_RDONLY);
    CHECK(fd > 0);
    CHECK(libfs_close(fd) == 0);
    return 0;
}

static int test_stat()
{
    int fd = libfs_open2("/pxfs/t_stat.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, "statme", 6) == 6);
    CHECK(libfs_close(fd) == 0);
    struct stat st;
    CHECK(libfs_stat("/pxfs/t_stat.dat", &st) == 0);
    return 0;
}

static int test_dup()
{
    int fd  = libfs_open2("/pxfs/t_dup.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    int fd2 = libfs_dup(fd);
    CHECK(fd2 > 0 && fd2 != fd);
    CHECK(libfs_write(fd2, "dup", 3) == 3);
    CHECK(libfs_close(fd)  == 0);
    CHECK(libfs_close(fd2) == 0);
    return 0;
}

static int test_chdir_getcwd()
{
    char buf[256];
    CHECK(libfs_chdir("/pxfs") == 0);
    char *cwd = libfs_getcwd(buf, sizeof(buf));
    CHECK(cwd != NULL);
    return 0;
}

static int test_sync_fsync()
{
    int fd = libfs_open2("/pxfs/t_sync.dat", O_CREAT|O_RDWR|O_TRUNC, 0644);
    CHECK(fd > 0);
    CHECK(libfs_write(fd, "sync", 4) == 4);
    CHECK(libfs_fsync(fd) == 0);
    CHECK(libfs_close(fd) == 0);
    CHECK(libfs_sync() == 0);
    return 0;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    extern int  opterr;
    const char* xdst = "10000";
    int         debug = 0;
    char        ch;

    opterr = 0;
    while ((ch = getopt(argc, argv, "h:d:")) != -1) {
        switch (ch) {
            case 'h': xdst  = optarg;       break;
            case 'd': debug = atoi(optarg);  break;
            default:  break;
        }
    }

    int ret = libfs_init3(xdst, debug);
    if (ret < 0) { fprintf(stderr, "libfs_init3 failed: %d\n", ret); return 1; }
    ret = libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
    if (ret < 0) { fprintf(stderr, "libfs_mount failed: %d\n", ret); return 1; }
    ret = libfs_chdir("/pxfs");
    if (ret < 0) { fprintf(stderr, "libfs_chdir failed: %d\n", ret); return 1; }

    printf("=== pxfs API unit tests ===\n");
    RUN(test_open_create);
    RUN(test_write_read);
    RUN(test_pread_pwrite);
    RUN(test_lseek);
    RUN(test_unlink);
    RUN(test_mkdir_rmdir);
    RUN(test_rename);
    RUN(test_link);
    RUN(test_stat);
    RUN(test_dup);
    RUN(test_chdir_getcwd);
    RUN(test_sync_fsync);

    printf("\n%d/%d passed\n", g_total - g_failures, g_total);
    libfs_shutdown();
    return (g_failures > 0) ? 1 : 0;
}
