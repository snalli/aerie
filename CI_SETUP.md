# CI/CD Integration Guide

## GitHub Actions Workflow

The project now includes automated code quality checks in CI:

### File: `.github/workflows/code-quality.yml`

**Triggers:**
- Push to `master`, `main`, `develop`
- Pull requests to `master`, `main`, `develop`
- Changes to C++ files or `.clang-format`

**Jobs:**

1. **Formatting Check** (code-quality/formatting)
   - Installs clang-format
   - Runs `scripts/format-code.sh --check`
   - Fails if any files need formatting

2. **Static Analysis** (code-quality/static-analysis)
   - Installs cppcheck
   - Runs `scripts/analyze-code.sh critical`
   - Reports critical issues (null pointers, memory leaks, etc.)

3. **Build Test** (code-quality/build)
   - Runs after formatting & analysis pass
   - Installs CMake and build tools
   - Configures and builds the project
   - Ensures formatted code still compiles

### View Results

After pushing/opening a PR, check results at:
```
https://github.com/snalli/aerie/actions
```

## Local Pre-commit Hooks

### Setup Pre-commit Hook

```bash
# Copy hook script to .git
cp .github/workflows/pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### What It Does

- Checks formatting for staged files only
- Runs cppcheck for critical issues on staged files
- Prevents commits with formatting violations
- Warns about critical static analysis issues

### Bypass Hook (if needed)

```bash
git commit --no-verify
```

## Local Quality Checks

### Before Committing

```bash
# Check all code
./scripts/quality-check.sh --all

# Fix formatting
./scripts/format-code.sh --fix

# Check critical issues
./scripts/analyze-code.sh critical
```

### After Formatting Changes

```bash
# Verify all formatted
./scripts/format-code.sh --check

# If issues found
./scripts/format-code.sh --fix

# Commit
git add -A
git commit -m "Fix: code formatting"
git push origin master
```

## CI Workflow Example

### When you push to master:

```mermaid
Push to master
     ↓
code-quality/formatting (runs)
     ↓
  Check passes? → No → Workflow fails ✗
     ↓ Yes
code-quality/static-analysis (runs)
     ↓
  Critical issues? → Log warnings
     ↓
code-quality/build (runs)
     ↓
  Build succeeds? → Yes → All checks pass ✅
                  → No → Workflow fails ✗
```

### When you open a PR:

Same workflow runs + branch protection requires all checks to pass before merge.

## Configuration

### Modifying GitHub Actions Workflow

Edit `.github/workflows/code-quality.yml`:

```yaml
on:
  push:
    branches: [ master, main, develop ]    # Add/remove branches
    paths:
      - 'libfs/**/*.cc'                    # Path patterns to monitor
      - 'libfs/**/*.h'
```

### Modifying Pre-commit Hook

Edit `.github/workflows/pre-commit-hook.sh`:

```bash
# Change check types
clang-format --style=file "$file"         # Formatting
cppcheck --quiet --enable=critical        # Static analysis
```

## Troubleshooting

### Formatting check fails locally but passes in CI

Ensure you have the same version of clang-format:
```bash
clang-format --version    # Should be 22.1.6 or compatible
```

### Pre-commit hook not running

```bash
# Check hook exists and is executable
ls -l .git/hooks/pre-commit

# If missing, reinstall
cp .github/workflows/pre-commit-hook.sh .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### CI reports formatting issues but local check passes

Push the latest version:
```bash
git status
git add -A
git commit -m "Fix: formatting"
git push origin master
```

## Next Steps

### Optional Enhancements

1. **Code Coverage**
   ```yaml
   - name: Generate coverage
     run: cmake -DCMAKE_BUILD_TYPE=Coverage . && make coverage
   ```

2. **Performance Benchmarks**
   ```yaml
   - name: Run benchmarks
     run: cd libfs/bench/ubench && make benchmark
   ```

3. **Clang-tidy (Advanced Linting)**
   ```yaml
   - name: Run clang-tidy
     run: clang-tidy libfs/src/**/*.cc
   ```

4. **ASAN (Memory Safety)**
   ```bash
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" .
   ```

## Files Reference

- **`.github/workflows/code-quality.yml`** - GitHub Actions workflow
- **`.github/workflows/pre-commit-hook.sh`** - Local pre-commit hook
- **`.clang-format`** - Code style configuration
- **`scripts/format-code.sh`** - Formatting tool
- **`scripts/analyze-code.sh`** - Static analysis tool
- **`CODE_QUALITY.md`** - Code quality documentation

## Status

✅ CI integration complete
✅ GitHub Actions workflow enabled
✅ Pre-commit hook available
✅ All local scripts integrated

---

**Setup Date**: 2026-05-30
**Status**: Ready for use
