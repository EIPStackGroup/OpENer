# CheckEncapsulationInactivity Test Suite

## Overview
Comprehensive test suite for the `CheckEncapsulationInactivity()` function from [source/src/ports/generic_networkhandler.c](source/src/ports/generic_networkhandler.c). This function monitors TCP socket encapsulation activity and closes inactive connections that exceed a configured timeout threshold.

## Function Under Test
```c
void CheckEncapsulationInactivity(int socket_handle)
```

**Purpose**: Checks if a socket has exceeded the encapsulation inactivity timeout and closes the connection if necessary.

**Parameters**:
- `socket_handle`: The socket to check for inactivity

**Dependencies**:
- `g_tcpip.encapsulation_inactivity_timeout`: Global timeout configuration (in seconds)
- `g_actual_time`: Current time in milliseconds
- `g_timestamps[]`: Array of socket timers tracking last update times
- `GetSessionFromSocket()`: Maps socket to session handle
- `CloseClass3ConnectionBasedOnSession()`: Closes Class 3 connections
- `CloseTcpSocket()`: Closes the TCP socket
- `RemoveSession()`: Removes the session from tracking

## Test Suite Details

### Location
[source/tests/ports/check_encapsulation_inactivity_tests.cpp](source/tests/ports/check_encapsulation_inactivity_tests.cpp)

### Test Count
**12 comprehensive tests**

### Test Cases

#### 1. `TimeoutDisabledDoesNothing`
**Purpose**: Verify that when encapsulation inactivity timeout is disabled (value = 0), no connection cleanup occurs regardless of elapsed time.

**Setup**:
- Timeout: 0 (disabled)
- Elapsed time: 400ms
- Socket: 5

**Expected**: No mock calls (no cleanup)

---

#### 2. `InactivityBelowThresholdDoesNothing`
**Purpose**: Verify that connections remain open when elapsed time is below the timeout threshold.

**Setup**:
- Timeout: 10 seconds (10000ms)
- Elapsed time: 500ms
- Socket: 5

**Expected**: No cleanup (elapsed < threshold)

---

#### 3. `InactivityThresholdExceededClosesConnection`
**Purpose**: Verify complete cleanup sequence when inactivity timeout is exceeded.

**Setup**:
- Timeout: 5 seconds (5000ms)
- Elapsed time: 6000ms
- Socket: 3
- Expected session: 2

**Expected Behavior**:
1. `GetSessionFromSocket(3)` called → returns session 2
2. `CloseClass3ConnectionBasedOnSession(2)` called
3. `CloseTcpSocket(3)` called
4. `RemoveSession(3)` called

---

#### 4. `InvalidSocketTimerDoesNothing`
**Purpose**: Verify graceful handling when socket has no timer allocated.

**Setup**:
- Socket: 999 (no timer)
- Timeout: 5 seconds

**Expected**: Function returns early, no cleanup (socket_timer is NULL)

---

#### 5. `ExactTimeoutBoundaryClosesConnection`
**Purpose**: Verify connection closes at exact timeout boundary (>= comparison).

**Setup**:
- Timeout: 3 seconds (3000ms)
- Elapsed time: 3000ms (exactly at threshold)
- Socket: 7

**Expected**: Connection closed (diff >= threshold)

---

#### 6. `JustBelowTimeoutBoundaryDoesNotClose`
**Purpose**: Verify off-by-one handling - no close when just below threshold.

**Setup**:
- Timeout: 3 seconds (3000ms)
- Elapsed time: 2999ms
- Socket: 8

**Expected**: No cleanup (diff < threshold)

---

#### 7. `MultipleSocketsIndependentEvaluation`
**Purpose**: Verify that multiple sockets are evaluated independently with correct timeout semantics.

**Setup**:
- Timeout: 2 seconds
- Socket 10: Last update at 100ms, current time 3000ms → 2900ms > 2000ms → CLOSED
- Socket 20: Last update at 2500ms, current time 3000ms → 500ms < 2000ms → OPEN

**Expected**:
- Socket 10: cleanup sequence executed
- Socket 20: no cleanup

---

#### 8. `ZeroTimeoutDisablesFeature`
**Purpose**: Verify timeout = 0 disables the feature.

**Setup**:
- Timeout: 0
- Elapsed time: 100000ms (very large)

**Expected**: Feature disabled, no cleanup

---

#### 9. `LargeTimeoutValue`
**Purpose**: Verify handling of very large timeout values (e.g., 1 hour).

**Setup**:
- Timeout: 3600 seconds (1 hour = 3600000ms)
- Elapsed time: 1800000ms (30 minutes)
- Socket: 12

**Expected**: Connection remains open (30 min < 1 hour)

---

#### 10. `MinimumOneSecondTimeout`
**Purpose**: Verify behavior with minimum practical timeout value.

**Setup**:
- Timeout: 1 second (1000ms)
- Elapsed time: 1500ms
- Socket: 13

**Expected**: Connection closed (1500ms > 1000ms)

---

#### 11. `SocketAtArrayBoundary`
**Purpose**: Verify correct behavior for sockets at the boundary of the timer array.

**Setup**:
- Socket at last valid index (OPENER_NUMBER_OF_SUPPORTED_SESSIONS - 1)
- Timeout: 2 seconds
- Elapsed time: 3000ms

**Expected**: Works correctly at array edge

---

#### 12. `OffByOneBoundaryTest`
**Purpose**: Comprehensive boundary testing with two threshold checks.

**Setup**:
- Timeout: 5 seconds (5000ms)
- Socket: 15

**Sequence**:
1. g_actual_time = 4999ms → No cleanup
2. g_actual_time = 5001ms → Cleanup executed

**Expected**: Demonstrates exact boundary behavior

---

## Mock Objects Used

The tests use CppUTest mocking framework to verify the function's interaction with dependencies:

### Mocked Functions
1. **GetSessionFromSocket(int socket_handle)**
   - Returns: CipSessionHandle (session ID for socket)
   - Verified for: correct socket parameter

2. **CloseClass3ConnectionBasedOnSession(CipSessionHandle session)**
   - Verified for: called with correct session handle

3. **CloseTcpSocket(int socket)**
   - Verified for: called with correct socket handle

4. **RemoveSession(int socket)**
   - Verified for: called with correct socket handle

### Test Framework Global State
- **g_actual_time**: Set to simulate time progression
- **g_timestamps[]**: Initialized and managed for socket timer tracking
- **g_tcpip.encapsulation_inactivity_timeout**: Configured per test scenario

## Test Results

```sh
OK (246 tests, 12 ran, 24 checks, 0 ignored, 234 filtered out, 1 ms)
```

**All 12 tests PASSED**

## Key Insights

1. **Timeout Semantics**: The function uses `>=` comparison (delta >= threshold), closing connections at or after the threshold.

2. **Feature Enablement**: Timeout must be > 0 to enable the feature. A value of 0 disables all inactivity checking.

3. **Time Unit**: Timeout is in seconds, converted to milliseconds internally (1000 * g_tcpip.encapsulation_inactivity_timeout).

4. **Graceful Degradation**: If socket timer lookup fails (socket not found), the function safely returns without attempting cleanup.

5. **Call Sequence**: When timeout is exceeded, the function always calls in this order:
   1. GetSessionFromSocket()
   2. CloseClass3ConnectionBasedOnSession()
   3. CloseTcpSocket()
   4. RemoveSession()

## Running the Tests

Run all CheckEncapsulationInactivity tests:
```bash
cd /home/mmm/OpENer/bin/posix
./tests/OpENer_Tests -v -g CheckEncapsulationInactivity
```

Run with detailed output:
```bash
./tests/OpENer_Tests -v -c -g CheckEncapsulationInactivity
```

Run all tests:
```bash
./tests/OpENer_Tests
```

## Files Modified

1. [source/tests/ports/check_encapsulation_inactivity_tests.cpp](source/tests/ports/check_encapsulation_inactivity_tests.cpp) - New test file
2. [source/tests/ports/CMakeLists.txt](source/tests/ports/CMakeLists.txt) - Added test file to build
3. [source/tests/OpENerTests.h](source/tests/OpENerTests.h) - Added test group import
4. [source/src/ports/generic_networkhandler.h](source/src/ports/generic_networkhandler.h) - Added function declaration

## Design Notes

The test suite covers:
- **Happy path**: Timeout exceeded → proper cleanup
- **Edge cases**: Exact boundary conditions, off-by-one scenarios
- **Error conditions**: Invalid sockets, disabled feature
- **Configuration changes**: Different timeout values, large/small values
- **State management**: Multiple sockets, independent evaluation
- **Array boundaries**: Sockets at edge of timer array

Tests are designed to be deterministic and independent, with each test's setup and teardown managed through the TEST_GROUP setup/teardown methods.
