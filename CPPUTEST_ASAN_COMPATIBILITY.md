# CppUTest and AddressSanitizer Compatibility

## The Issue

CppUTest and AddressSanitizer (ASAN) both instrument memory allocation functions (`malloc`, `free`, `new`, `delete`). When used together without proper configuration, they can conflict:

### CppUTest's Memory Leak Detection
- Overrides `malloc`/`free` to track allocations
- Uses memory leak detector macros (`MemoryLeakDetectorNewMacros.h`)
- Maintains internal hash tables of allocated memory
- Good for detecting leaks within test scope

### AddressSanitizer's Memory Instrumentation
- Wraps `malloc`/`free` at compile time
- Tracks shadow memory for overflow detection
- Monitors use-after-free, double-free, and leaks
- More comprehensive than CppUTest's detector

### Potential Conflicts
1. **Double instrumentation**: Both tools wrap the same functions
2. **Conflicting metadata**: Different memory tracking approaches interfere
3. **False positives**: One tool's cleanup can confuse the other
4. **Performance**: Double overhead from both tools

## Our Solution

We've configured the build system to **disable CppUTest's memory leak detection when ASAN is enabled**:

### Configuration Changes

**1. CMakeLists.txt (tests/)**
```cmake
if(ENABLE_ADDRESS_SANITIZER OR ENABLE_UNDEFINED_SANITIZER)
  add_compile_definitions(CPPUTEST_MEMORY_LEAK_DETECTION_DISABLED)
  message(STATUS "CppUTest memory leak detection disabled (using AddressSanitizer instead)")
endif()
```

This tells CppUTest to use standard C/C++ library functions without wrapping them.

**2. security_tests.cpp**
```cpp
#ifdef CPPUTEST_MEMORY_LEAK_DETECTION_DISABLED
  #define CPPUTEST_USE_STD_CPP_LIB
  #define CPPUTEST_USE_STD_C_LIB
#endif
```

These macros instruct CppUTest to use the standard library instead of its custom memory wrappers.

## Why This Works

- **When ASAN is OFF**: CppUTest's memory checks work normally
- **When ASAN is ON**:
  - CppUTest doesn't instrument memory
  - Only ASAN does, avoiding conflicts
  - ASAN provides more comprehensive checking anyway

## Testing Both Ways

### Without ASAN (CppUTest memory checks only)
```bash
cd bin/posix
cmake .
make -j$(nproc)
./tests/OpENer_Tests -g NetworkHandlerSecurity
```

### With ASAN (full memory safety)
```bash
cd bin/posix
cmake -DENABLE_ADDRESS_SANITIZER=ON .
make -j$(nproc)
ASAN_OPTIONS="verbosity=0" ./tests/OpENer_Tests -g NetworkHandlerSecurity
```

## Memory Safety Coverage

| Feature | CppUTest | ASAN | Both |
|---------|----------|------|------|
| Leak detection | ✓ | ✓ | ASAN only |
| Buffer overflow | - | ✓ | ✓ |
| Use-after-free | - | ✓ | ✓ |
| Double-free | - | ✓ | ✓ |
| Integer overflow | - | ✓ (UBSAN) | ✓ |
| Stack issues | - | ✓ | ✓ |
| Uninitialized reads | - | Limited | Limited |

**Recommendation**: Use ASAN for comprehensive memory safety testing.

## Verification

To verify there are no conflicts:

```bash
# Build with ASAN
cd bin/posix && cmake -DENABLE_ADDRESS_SANITIZER=ON . && make -j$(nproc)

# Run tests - should see no conflicts
ASAN_OPTIONS="verbosity=0" ./tests/OpENer_Tests -g NetworkHandlerSecurity

# Check for ASAN errors (exit code 1 = error found)
echo "Exit code: $?"
```

## References

- **CppUTest Memory Management**: Uses `CHECK_EQUAL_TEXT`, `CHECK_EQUAL_NOCASE_TEXT` for memory checks
- **ASAN Documentation**: https://github.com/google/sanitizers/wiki/AddressSanitizer
- **Best Practices**: Disable CppUTest's memory checking when using compiler sanitizers

## Future Improvements

1. Could implement custom ASAN suppressions file if false positives occur
2. Could run both tools separately for comprehensive coverage
3. Could add environment variable to control behavior at runtime

---

**Status**: ✅ CppUTest and ASAN are now compatible and won't conflict
