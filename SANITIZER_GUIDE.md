# AddressSanitizer Integration Guide for OpENer

## Overview

AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) have been integrated into OpENer's build system to provide comprehensive runtime detection of memory safety and undefined behavior issues. This is critical for security-hardened automation infrastructure.

## What Gets Detected

### AddressSanitizer (ASAN)
- **Heap buffer overflows** - Writing beyond allocated memory
- **Stack buffer overflows** - Writing beyond stack arrays
- **Global buffer overflows** - Writing beyond global arrays
- **Use-after-free** - Accessing freed memory
- **Double-free** - Freeing memory twice
- **Memory leaks** - Allocated memory never freed
- **Invalid memory access** - Accessing unmapped/invalid memory

### UndefinedBehaviorSanitizer (UBSAN)
- **Integer overflows** - Signed integer overflow
- **Signed integer underflow**
- **Shift out of bounds** - Shift amount >= width
- **Division by zero**
- **Type mismatches** - Invalid pointer casts
- **Array bounds violations**
- **Null pointer dereference**

## Building with Sanitizers

### Option 1: Automatic Build Script (Recommended)

```bash
# Build with both ASAN and UBSAN
./build_with_sanitizers.sh both

# Build with only ASAN
./build_with_sanitizers.sh asan

# Build with only UBSAN
./build_with_sanitizers.sh ubsan
```

This creates a `build_sanitizer` directory with instrumented binaries.

### Option 2: Manual CMake Configuration

```bash
mkdir build_debug
cd build_debug

# Enable AddressSanitizer
cmake -DENABLE_ADDRESS_SANITIZER=ON ../source

# Or enable both sanitizers
cmake -DENABLE_ADDRESS_SANITIZER=ON -DENABLE_UNDEFINED_SANITIZER=ON ../source

cmake --build . --parallel $(nproc)
```

## Running Tests

### Using the Test Runner Script

```bash
# Run all security tests
./run_sanitizer_tests.sh

# Run specific test group
./run_sanitizer_tests.sh build_sanitizer NetworkHandlerSecurity

# Run tests matching pattern
./run_sanitizer_tests.sh build_sanitizer CheckEncapsulation
```

### Manual Test Execution

```bash
cd build_sanitizer
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:verbosity=2"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
./tests/OpENer_Tests -v -c
```

## Understanding ASAN Output

When ASAN detects an error, it produces output like:

```sh
=================================================================
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x61000001fbfc
  WRITE of size 4 at 0x61000001fbfc thread T0
  #0 0x46f0b0 in SendUdpData /home/user/OpENer/source/src/ports/generic_networkhandler.c:850:5
  #1 0x46a1bc in HandleDataOnTcpSocket /home/user/OpENer/source/src/ports/generic_networkhandler.c:920:3
```

**Key Information:**
- **ERROR TYPE**: heap-buffer-overflow, use-after-free, memory-leak, etc.
- **LOCATION**: File, function, and line number
- **STACK TRACE**: Call stack showing how the error was reached
- **Address**: Memory address involved

### Common ASAN Error Types

| Error                   | Cause                                | Fix                               |
|-------------------------|--------------------------------------|-----------------------------------|
| `heap-buffer-overflow`  | Writing beyond allocated heap buffer | Validate buffer size before write |
| `stack-buffer-overflow` | Writing beyond stack array           | Check array bounds                |
| `use-after-free`        | Accessing freed memory               | Don't use pointer after free      |
| `memory-leak`           | Allocated memory never freed         | Add appropriate free/cleanup      |
| `double-free`           | Freeing same pointer twice           | Track free() calls                |

## Security Test Suite

New security-focused tests have been added to detect vulnerabilities:

### Location
`source/tests/security_tests.cpp`

### Test Groups
- **SocketHandleValidation**: Validate socket descriptor ranges
- **MaxSocketBoundary**: Detect integer overflow in socket calculations
- **NegativeSocketHandle**: Handle invalid socket descriptors
- **ReceivedSizeValidation**: Validate network-received data sizes
- **EncapsulationHeaderCalculations**: Detect header parsing overflows
- **MessageLengthValidation**: Validate untrusted message lengths
- **ASANHeapBufferDetection**: Verify ASAN infrastructure
- **ASANUseAfterFreeDetection**: Verify ASAN detects UAF
- **ASANStackBufferDetection**: Verify ASAN detects stack overflow

Run security tests:
```bash
./run_sanitizer_tests.sh build_sanitizer NetworkHandlerSecurity
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Security Tests

on: [push, pull_request]

jobs:
  sanitizer-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install Dependencies
        run: apt-get install -y clang cmake

      - name: Build with Sanitizers
        run: ./build_with_sanitizers.sh both

      - name: Run Security Tests
        run: ./run_sanitizer_tests.sh build_sanitizer
```

## Configuration Options

### AddressSanitizer Options

Set via `ASAN_OPTIONS` environment variable:

```bash
# Halt on first error
export ASAN_OPTIONS="halt_on_error=1"

# Enable leak detection
export ASAN_OPTIONS="detect_leaks=1"

# Verbose output
export ASAN_OPTIONS="verbosity=2"

# Log to file
export ASAN_OPTIONS="log_path=asan.log"

# Combined
export ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:verbosity=2:log_path=asan.log"
```

### UndefinedBehaviorSanitizer Options

Set via `UBSAN_OPTIONS` environment variable:

```bash
# Print stack trace
export UBSAN_OPTIONS="print_stacktrace=1"

# Halt on error
export UBSAN_OPTIONS="halt_on_error=1"

# Verbose output
export UBSAN_OPTIONS="verbosity=2"

# Combined
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:verbosity=2"
```

## Performance Impact

Sanitizers add significant overhead:

| Metric          | Normal | ASAN        | UBSAN           | Both          |
|-----------------|--------|-------------|-----------------|---------------|
| Execution Speed | 1x     | 2-4x slower | 1.1-1.5x slower | 2.5-5x slower |
| Memory Overhead | 1x     | 2-3x larger | ~1x             | 2-3x larger   |
| Compile Time    | Normal | +30%        | +20%            | +50%          |

**Recommendation**: Use sanitizers during development and CI, use optimized builds for production.

## Workflow

### Development Cycle

1. **Edit code** in source files
2. **Build with sanitizers**: `./build_with_sanitizers.sh both`
3. **Run tests**: `./run_sanitizer_tests.sh`
4. **Fix any detected issues** before committing
5. **Run full test suite** in CI pipeline

### Pre-Commit Checklist

```bash
#!/bin/bash
# .git/hooks/pre-commit

./build_with_sanitizers.sh both
./run_sanitizer_tests.sh build_sanitizer
if [ $? -ne 0 ]; then
    echo "Sanitizer tests failed - commit blocked"
    exit 1
fi
```

## Suppressing False Positives

If a sanitizer detects a false positive, you can suppress it:

### ASAN Suppression

Create `asan.supp`:
```sh
leak:function_name_here
addr:file_name:line_number
```

Use it:
```sh
export ASAN_OPTIONS="suppressions=$(pwd)/asan.supp"
```

### UBSAN Suppression

```bash
export UBSAN_OPTIONS="suppressions=$(pwd)/ubsan.supp"
```

## Troubleshooting

### Build Fails with Sanitizer Flags

Ensure compiler supports sanitizers:
```bash
clang --version
gcc --version
```

Update to recent versions (GCC 9+, Clang 10+)

### Tests Run Very Slowly

This is expected with sanitizers enabled. For normal development:
```bash
# Use optimized build for iteration
cmake -DCMAKE_BUILD_TYPE=Release ../source
cmake --build .
```

Only use sanitizer builds for security validation.

### Memory Leak Detection False Positives

Some third-party libraries trigger false positives. Create suppressions:
```bash
export ASAN_OPTIONS="detect_leaks=0"  # Disable leak detection
```

## Related Files

- **CMake Configuration**: `source/CMakeLists.txt`
- **Security Tests**: `source/tests/security_tests.cpp`
- **Build Script**: `build_with_sanitizers.sh`
- **Test Runner**: `run_sanitizer_tests.sh`

## Further Reading

- [AddressSanitizer Documentation](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [ASAN Flag Reference](https://github.com/google/sanitizers/wiki/AddressSanitizerFlags)
