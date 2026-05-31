# Code Quality Framework

## Overview

This project uses automated tools for code quality assurance:
- **clang-format**: Code formatting consistency
- **cppcheck**: Static analysis for potential bugs

## Quick Start

### Run Quality Checks
```bash
# Check all
./scripts/quality-check.sh --all

# Check only formatting
./scripts/quality-check.sh --format

# Check only static analysis
./scripts/quality-check.sh --analyze
```

### Fix Formatting Issues
```bash
# Dry-run (show what will change)
./scripts/format-code.sh --check

# Apply fixes
./scripts/format-code.sh --fix
```

### Detailed Static Analysis
```bash
# All issues
./scripts/analyze-code.sh all

# Warnings and errors only
./scripts/analyze-code.sh warnings

# Critical issues only (null pointers, memory leaks, etc.)
./scripts/analyze-code.sh critical
```

## Current Status

### Formatting (clang-format)
- **Files needing formatting**: 466 / 483 (97%)
- **Configuration**: `.clang-format` (LLVM-based style)
- **Action**: Run `scripts/format-code.sh --fix` to apply

### Static Analysis (cppcheck)
- **Total issues found**: 3,757
- **Critical issues**: Potential null pointers, memory leaks
- **Warning issues**: Format string mismatches, type conversions

### Top Issue Categories

| Issue Type | Count | Severity | Example |
|-----------|-------|----------|---------|
| nullPointerOutOfMemory | High | Critical | malloc() without null check |
| invalidPrintfArgType | Medium | Warning | %d format with wrong type |
| variableScope | Low | Style | Variable declared too early |
| constParameterCallback | Low | Style | Parameter could be const |

## Configuration Files

### .clang-format
Defines code style for the project:
- **Base style**: LLVM
- **Indent**: 4 spaces
- **Line length**: 100 characters
- **Braces**: Allman style
- **Includes**: Auto-sorted

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
PointerAlignment: Left
BreakBeforeBraces: Allman
ColumnLimit: 100
```

## Integration Points

### Pre-commit Hook (Optional)
To enforce formatting before commits:

```bash
# Create .git/hooks/pre-commit
#!/bin/bash
bash scripts/format-code.sh --check
```

### CI/CD Integration
Include in GitHub Actions:

```yaml
- name: Check code formatting
  run: bash scripts/format-code.sh --check

- name: Static analysis
  run: bash scripts/analyze-code.sh critical
```

## Scripts Reference

### format-code.sh
**Purpose**: Enforce consistent code style
**Usage**: `./scripts/format-code.sh [--check|--fix]`

**Flags**:
- `--check`: Dry-run, show files that need formatting (default)
- `--fix`: Apply formatting to all files

**Output**: Lists files that need formatting or confirms all are formatted

### analyze-code.sh
**Purpose**: Find potential bugs and code quality issues
**Usage**: `./scripts/analyze-code.sh [all|warnings|critical]`

**Levels**:
- `all`: All issues (default)
- `warnings`: Warnings and errors only
- `critical`: Critical issues (null pointers, memory leaks)

**Suppressed Warnings**:
- `missingIncludeSystem`: System header warnings
- `unusedFunction`: Library functions may not be used by all
- `missingInclude`: Project-specific paths

### quality-check.sh
**Purpose**: Run all quality checks
**Usage**: `./scripts/quality-check.sh [--format|--analyze|--all]`

**Targets**:
- `--format`: Formatting check only
- `--analyze`: Static analysis only
- `--all`: Run all checks (default)

## Troubleshooting

### clang-format not found
```bash
brew install clang-format
```

### cppcheck not found
```bash
brew install cppcheck
```

### Too many formatting issues
This is normal for a mature codebase. Start by:
1. Run format-code.sh --fix to apply all fixes at once
2. Commit the formatting changes
3. Configure IDE to use .clang-format for auto-formatting

### cppcheck showing too many false positives
- Use `--suppress` flags to exclude known non-issues
- Configuration in `analyze-code.sh` already excludes system headers
- Critical issues (nullPointer, memoryLeak) are more reliable

## Next Steps

### For Developers
1. Set up IDE integration for clang-format
2. Run `quality-check.sh` before committing
3. Fix critical issues (null pointers, memory leaks)

### For CI/CD
1. Add pre-commit hook
2. Run formatting check in CI pipeline
3. Run static analysis on critical code paths
4. Archive results for trend analysis

### For Project
1. Create epic to address critical cppcheck issues
2. Integrate formatting into development workflow
3. Review and suppress false positives over time
4. Consider adding more lint rules (clang-tidy)

## Related Files

- `.clang-format` - Code style configuration
- `scripts/format-code.sh` - Formatting tool
- `scripts/analyze-code.sh` - Static analysis tool
- `scripts/quality-check.sh` - Unified quality check

---

**Setup Date**: 2026-05-30
**Tools**: clang-format 22.1.6, cppcheck 2.20.0
**Status**: ✅ Ready for use
