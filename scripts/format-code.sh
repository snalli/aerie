#!/bin/bash
# format-code.sh - Run clang-format on all C++ source files
# Usage: ./scripts/format-code.sh [--check|--fix]

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Determine mode
MODE="${1:---check}"

if [[ "$MODE" != "--check" && "$MODE" != "--fix" ]]; then
    echo "Usage: $0 [--check|--fix]"
    echo ""
    echo "  --check    Dry-run: show files that need formatting"
    echo "  --fix      Apply formatting changes to all files"
    exit 1
fi

# Find all C++ source files
FILES=$(find libfs -name "*.cc" -o -name "*.h" | grep -v ".build\|.generated\|postmark")

FILE_COUNT=$(echo "$FILES" | wc -l)
BAD_FILES=0

echo "=========================================="
echo "clang-format Code Quality Check"
echo "=========================================="
echo "Mode: $MODE"
echo "Total files: $FILE_COUNT"
echo ""

if [[ "$MODE" == "--check" ]]; then
    echo "Checking formatting (dry-run)..."
    echo ""

    while read -r file; do
        if ! clang-format --style=file "$file" | diff -q "$file" - > /dev/null 2>&1; then
            echo "  ✗ $file"
            ((BAD_FILES++))
        fi
    done <<< "$FILES"

    GOOD_FILES=$((FILE_COUNT - BAD_FILES))
    PERCENTAGE=$(( (GOOD_FILES * 100) / FILE_COUNT ))

    echo ""
    echo "=========================================="
    echo "Summary:"
    echo "  Files OK: $GOOD_FILES / $FILE_COUNT ($PERCENTAGE%)"
    echo "  Files needing formatting: $BAD_FILES"
    echo "=========================================="

    if [ $BAD_FILES -gt 0 ]; then
        echo ""
        echo "To fix formatting, run:"
        echo "  $0 --fix"
        exit 1
    fi

elif [[ "$MODE" == "--fix" ]]; then
    echo "Applying formatting to all files..."
    echo ""

    while read -r file; do
        clang-format --style=file -i "$file"
        echo "  ✓ $file"
    done <<< "$FILES"

    echo ""
    echo "=========================================="
    echo "Formatting complete!"
    echo "=========================================="
fi
