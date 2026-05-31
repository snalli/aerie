#!/bin/bash
# Pre-commit hook for code quality checks
#
# Install in .git/hooks/pre-commit:
#   cp .github/workflows/pre-commit-hook.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit
#
# This will run formatting and static analysis checks before allowing commits.

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Running pre-commit code quality checks..."
echo ""

# Only check files being committed (not all files)
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cc|h)$' || true)

if [ -z "$STAGED_FILES" ]; then
    echo "No C++ files to check."
    exit 0
fi

echo "Checking formatting..."
for file in $STAGED_FILES; do
    if [ -f "$file" ]; then
        if ! clang-format --style=file "$file" | diff -q "$file" - > /dev/null 2>&1; then
            echo "❌ Formatting check failed: $file"
            echo ""
            echo "Run to fix:"
            echo "  clang-format --style=file -i $file"
            echo "  git add $file"
            exit 1
        fi
    fi
done

echo "✅ Formatting check passed"
echo ""

echo "Checking for obvious errors with cppcheck..."
if command -v cppcheck &> /dev/null; then
    # Only check changed files
    for file in $STAGED_FILES; do
        if [ -f "$file" ]; then
            issues=$(cppcheck --quiet --enable=critical "$file" 2>&1 | grep -E "(nullPointer|memoryLeak|outOfBounds)" || true)
            if [ -n "$issues" ]; then
                echo "⚠️  Potential critical issues found in $file:"
                echo "$issues"
                echo ""
                echo "Consider running: bash scripts/analyze-code.sh critical"
                # Don't fail on warnings - just notify
            fi
        fi
    done
fi

echo "✅ Pre-commit checks passed"
