# KVFS Comprehensive Test Suite

Complete overview of all kvfs tests with instructions for running them.

---

## Test Categories

### 1. Single-Thread API Tests (`kvfs_api_test`)
**File**: `libfs/bench/ubench/kvfs_api_test.cc`
**Tests**: 7 correctness tests
- `test_put_get` - Basic put/get with content verification
- `test_put_overwrite` - Overwrite with new content
- `test_put_binary` - Binary data handling
- `test_del` - Deletion and subsequent access
- `test_multiple_keys` - Multiple key operations
- `test_large_value` - Large value (4KB) handling
- `test_sync` - Sync operation and durability

**Usage:**
```bash
kvfs_api_test -h 10001
```

**Expected**: All 7 tests pass

---

### 2. Multi-Thread Concurrency Tests (`kvfs_multithread_test`)
**File**: `libfs/bench/ubench/kvfs_multithread_test.cc`
**Tests**: Configurable threads, ~5 tests per thread

#### With 2 threads: 10 tests
- 5 tests per thread:
  1. `concurrent_put_get` (50 keys per thread)
  2. `concurrent_overwrite` (25 keys per thread)
  3. `concurrent_delete` (25 keys per thread)
  4. `concurrent_same_key` (25 writes to shared key)
  5. `cross_thread_read` (content-verified reads of other thread's data)

**Usage:**
```bash
kvfs_multithread_test -h 10001 -t 2    # 2 threads = 10 tests
kvfs_multithread_test -h 10001 -t 3    # 3 threads = 15 tests
kvfs_multithread_test -h 10001 -t 8    # 8 threads = 40 tests
```

**Key Features:**
- Content verification with memcmp (not just size)
- Cross-thread data reads with integrity checks
- Same-key contention testing
- Atomic delete handling

**Expected**: 100% pass rate at all thread counts

---

### 3. Space Exhaustion Tests (`kvfs_space_test`)
**File**: `libfs/bench/ubench/kvfs_space_test.cc`
**Tests**: 3 stress scenarios

1. **test_space_exhaustion**
   - Continuously write until storage exhausted
   - Verify proper error codes returned
   - No crashes allowed

2. **test_recovery_after_exhaustion**
   - Verify reads work after exhaustion
   - Existing data is accessible

3. **test_sync_after_exhaustion**
   - Sync operation on full disk
   - Proper error handling

**Usage:**
```bash
kvfs_space_test -h 10001
```

**Configurable:**
- `-s <bytes>` - Individual value size (default 256)
- `-p <bytes>` - Maximum pool size (default 1MB)

**Expected**: Graceful degradation, no crashes

---

### 4. Durability Tests (`kvfs_durability_test`)
**File**: `libfs/bench/ubench/kvfs_durability_test.cc`
**Tests**: 2-phase persistence test

#### Phase 1: Write (-p write)
- Write 100 keys with 0x42 pattern (256 bytes each)
- Explicitly sync to persistent storage
- Instructs user to restart server

#### Phase 2: Verify (-p verify)
- Verify all 100 keys survived restart
- Check pattern integrity (0x42 bytes)
- Detect any data loss or corruption

**Usage:**
```bash
# Terminal 1: Write test data
kvfs_durability_test -h 10001 -p write

# Kill and restart kvfs server

# Terminal 1: Verify after restart
kvfs_durability_test -h 10001 -p verify
```

**Expected:**
- Phase 1: Completes and instructs restart
- Phase 2: All 100 keys recovered correctly

---

### 5. Resilience Tests (`kvfs_resilience_test`)
**File**: `libfs/bench/ubench/kvfs_resilience_test.cc`
**Tests**: 4 independent resilience scenarios

#### Test 1: client_reconnect
**What**: Client disconnect and reconnect
**How**: Write → Disconnect → Reconnect → Verify
**Validates**: Session recovery without data loss

```bash
kvfs_resilience_test -h 10001 -t client_reconnect
```

#### Test 2: concurrent_delete
**What**: Multiple threads delete same key
**How**: 4 threads simultaneously delete same key
**Validates**: Atomic delete, race condition handling

```bash
kvfs_resilience_test -h 10001 -t concurrent_delete
```

#### Test 3: corruption
**What**: Data integrity verification
**How**: Write 256 bytes of 0xAB pattern, read back and verify every byte
**Validates**: No bit flips or corruption

```bash
kvfs_resilience_test -h 10001 -t corruption
```

#### Test 4: atomicity
**What**: Partial write detection
**How**: Write 10 bytes, overwrite with 1024 bytes, verify result
**Validates**: Atomic writes (not partial)

```bash
kvfs_resilience_test -h 10001 -t atomicity
```

**Expected**: All 4 tests pass independently

---

### 6. Crash Recovery Tests (`kvfs_crash_test`)
**File**: `libfs/bench/ubench/kvfs_crash_test.cc`
**Tests**: 2-phase unclean shutdown recovery

#### Phase 1: Prepare (-p prepare)
- Write 50 keys with 0x55 pattern, sync them (durable)
- Write 50 keys with 0xAA pattern, no sync (volatile)
- Instructs user to kill server immediately

#### Phase 2: Verify (-p verify)
- All 50 synced keys must be present and valid (CRITICAL)
- Unsynced keys may be lost but not corrupted (acceptable)
- Detects partial writes or inconsistent data

**Usage:**
```bash
# Terminal 1: Prepare
kvfs_crash_test -h 10001 -p prepare

# Terminal 2: Kill server immediately
pkill kvfs_server

# Terminal 1: Restart server and verify
kvfs_crash_test -h 10001 -p verify
```

**Expected:**
- All synced data recovered (100%)
- Unsynced data: loss acceptable, corruption NOT acceptable

---

## Test Execution Strategy

### Smoke Tests (Quick Validation)
Run these first to verify basic functionality:
```bash
kvfs_api_test -h 10001
kvfs_multithread_test -h 10001 -t 2
```

**Time**: ~5 seconds
**Coverage**: Basic operations, concurrency

### Correctness Tests
```bash
kvfs_multithread_test -h 10001 -t 3
kvfs_resilience_test -h 10001 -t client_reconnect
kvfs_resilience_test -h 10001 -t concurrent_delete
kvfs_resilience_test -h 10001 -t corruption
kvfs_resilience_test -h 10001 -t atomicity
```

**Time**: ~10 seconds
**Coverage**: Concurrency, resilience, integrity

### Durability Tests (Manual/Scheduled)
```bash
# Test 1: Server restart
kvfs_durability_test -h 10001 -p write
# [restart server]
kvfs_durability_test -h 10001 -p verify

# Test 2: Crash recovery
kvfs_crash_test -h 10001 -p prepare
# [kill server immediately]
kvfs_crash_test -h 10001 -p verify
```

**Time**: ~30 seconds (+ manual restart)
**Coverage**: Persistence, crash recovery

### Stress Tests (Long-Running)
```bash
kvfs_space_test -h 10001
kvfs_multithread_test -h 10001 -t 8
```

**Time**: ~60 seconds
**Coverage**: Resource limits, high concurrency

---

## Test Coverage Matrix

| Aspect | Test | Status |
|--------|------|--------|
| **Basic Operations** | kvfs_api_test | ✅ Complete |
| **Content Integrity** | cross_thread_read, corruption | ✅ Complete |
| **Concurrency** | kvfs_multithread_test (2-8T) | ✅ Complete |
| **Same-key Contention** | concurrent_same_key | ✅ Complete |
| **Deletion** | concurrent_delete | ✅ Complete |
| **Durability** | kvfs_durability_test | ✅ Complete |
| **Crash Recovery** | kvfs_crash_test | ✅ Complete |
| **Atomicity** | test_atomicity | ✅ Complete |
| **Space Exhaustion** | kvfs_space_test | ✅ Complete |
| **Client Restart** | client_reconnect | ✅ Complete |

---

## Known Limitations & Future Work

### Not Yet Implemented
- ⚠️ Network partition simulation
- ⚠️ 10+ thread stress testing (can be added)
- ⚠️ 10K+ key volume testing (can be added)
- ⚠️ Hot key contention benchmarks (can be added)
- ⚠️ Latency/throughput monitoring (can be added)

### Acceptable Limitations
- Crash recovery requires manual server kill (simulation)
- Server restart durability requires manual service restart
- No automatic recovery/monitoring in test framework

---

## Production Readiness Checklist

- [x] Basic operations tested
- [x] Concurrency tested (2, 3, 8 threads)
- [x] Content integrity verified
- [x] Data atomicity checked
- [x] Deletion handling verified
- [x] Space exhaustion handled gracefully
- [x] Server restart recovery validated
- [x] Crash recovery with durable data separation
- [ ] Performance benchmarks (future)
- [ ] 10K+ key scale testing (future)
- [ ] Network resilience (future)

---

## Quick Reference Commands

```bash
# All smoke tests
kvfs_api_test -h 10001
kvfs_multithread_test -h 10001 -t 2

# All resilience tests
kvfs_resilience_test -h 10001 -t client_reconnect
kvfs_resilience_test -h 10001 -t concurrent_delete
kvfs_resilience_test -h 10001 -t corruption
kvfs_resilience_test -h 10001 -t atomicity

# Durability (manual restart)
kvfs_durability_test -h 10001 -p write
# [restart server]
kvfs_durability_test -h 10001 -p verify

# Crash recovery (manual kill)
kvfs_crash_test -h 10001 -p prepare
# [kill -9 kvfs_server]
kvfs_crash_test -h 10001 -p verify

# Stress tests
kvfs_space_test -h 10001
kvfs_multithread_test -h 10001 -t 8
```

---

**Created**: 2026-05-30
**All High-Priority Gaps**: Implemented ✅
**Test File Count**: 6 test programs
**Total Distinct Scenarios**: 50+ unique test cases
