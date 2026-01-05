#!/bin/bash
################################################################################
# Run OpENer Tests with AddressSanitizer Output
#
# This script runs the OpENer test suite with AddressSanitizer and
# UndefinedBehaviorSanitizer configured for maximum error detection.
#
# Usage:
#   ./run_sanitizer_tests.sh [build_dir] [test_filter]
#
# Examples:
#   ./run_sanitizer_tests.sh              # Run all tests, auto-find build dir
#   ./run_sanitizer_tests.sh build_sanitizer
#   ./run_sanitizer_tests.sh build_sanitizer NetworkHandlerSecurity
#
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Determine build directory
if [ -n "$1" ] && [ -d "$1" ]; then
    BUILD_DIR="$1"
elif [ -d "${SCRIPT_DIR}/build_sanitizer" ]; then
    BUILD_DIR="${SCRIPT_DIR}/build_sanitizer"
elif [ -d "${SCRIPT_DIR}/build" ]; then
    BUILD_DIR="${SCRIPT_DIR}/build"
else
    echo "Error: Cannot find build directory"
    echo "Usage: $0 [build_dir] [test_filter]"
    exit 1
fi

TEST_FILTER="${2:-}"
TEST_EXECUTABLE="${BUILD_DIR}/tests/OpENer_Tests"

if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Error: Test executable not found at $TEST_EXECUTABLE"
    echo "Build with: ./build_with_sanitizers.sh"
    exit 1
fi

echo "=============================================="
echo "OpENer Test Execution with Sanitizers"
echo "=============================================="
echo "Build Directory: $BUILD_DIR"
echo "Test Executable: $TEST_EXECUTABLE"
echo ""

# Configure sanitizer options
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:verbosity=2:log_path=asan.log"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:verbosity=2:log_path=ubsan.log"

echo "Environment:"
echo "  ASAN_OPTIONS=$ASAN_OPTIONS"
echo "  UBSAN_OPTIONS=$UBSAN_OPTIONS"
echo ""

# Run tests
TEST_ARGS="-v -c"
if [ -n "$TEST_FILTER" ]; then
    TEST_ARGS="$TEST_ARGS -g $TEST_FILTER"
    echo "Running tests matching: $TEST_FILTER"
else
    echo "Running all tests..."
fi

echo ""
"$TEST_EXECUTABLE" $TEST_ARGS
TEST_RESULT=$?

echo ""
echo "=============================================="
if [ "$TEST_RESULT" -eq 0 ]; then
    echo "✓ All tests passed!"
else
    echo "✗ Tests failed with exit code: $TEST_RESULT"
fi
echo "=============================================="
echo ""

# Check for sanitizer logs
if [ -f "asan.log" ]; then
    echo "AddressSanitizer detected issues:"
    echo "See asan.log for details"
    echo ""
fi

if [ -f "ubsan.log" ]; then
    echo "UndefinedBehaviorSanitizer detected issues:"
    echo "See ubsan.log for details"
    echo ""
fi

exit $TEST_RESULT
