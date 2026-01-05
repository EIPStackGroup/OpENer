#!/bin/bash
################################################################################
# AddressSanitizer Build Script for OpENer
#
# This script builds OpENer with AddressSanitizer and UndefinedBehaviorSanitizer
# enabled for comprehensive security vulnerability detection.
#
# Usage:
#   ./build_with_sanitizers.sh [asan|ubsan|both]
#
# Environment Variables:
#   ASAN_OPTIONS: Configure AddressSanitizer behavior
#   UBSAN_OPTIONS: Configure UndefinedBehaviorSanitizer behavior
#
# Examples:
#   ./build_with_sanitizers.sh asan     # Only AddressSanitizer
#   ./build_with_sanitizers.sh ubsan    # Only UndefinedBehaviorSanitizer
#   ./build_with_sanitizers.sh both     # Both sanitizers
#
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="${PROJECT_ROOT}/build_sanitizer"

# Parse arguments
SANITIZER_TYPE="${1:-both}"

if [[ ! "$SANITIZER_TYPE" =~ ^(asan|ubsan|both)$ ]]; then
    echo "Usage: $0 [asan|ubsan|both]"
    exit 1
fi

echo "=============================================="
echo "OpENer Security Build with Sanitizers"
echo "=============================================="
echo "Sanitizer Type: $SANITIZER_TYPE"
echo "Build Directory: $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake with sanitizer options
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug -DOpENer_PLATFORM=POSIX -DOpENer_TESTS=ON"

if [[ "$SANITIZER_TYPE" == "asan" || "$SANITIZER_TYPE" == "both" ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DENABLE_ADDRESS_SANITIZER=ON"
    echo "[+] AddressSanitizer enabled"
fi

if [[ "$SANITIZER_TYPE" == "ubsan" || "$SANITIZER_TYPE" == "both" ]]; then
    CMAKE_ARGS="$CMAKE_ARGS -DENABLE_UNDEFINED_SANITIZER=ON"
    echo "[+] UndefinedBehaviorSanitizer enabled"
fi

echo ""
echo "Configuring CMake..."
cmake "$CMAKE_ARGS" "$PROJECT_ROOT/source"

echo ""
echo "Building..."
cmake --build . --config Debug --parallel "$(nproc)"

echo ""
echo "=============================================="
echo "Build Complete!"
echo "=============================================="
echo ""
echo "Run tests with:"
echo "  cd $BUILD_DIR"
echo "  ASAN_OPTIONS='detect_leaks=1:halt_on_error=1' ./tests/OpENer_Tests -v -c"
echo ""
