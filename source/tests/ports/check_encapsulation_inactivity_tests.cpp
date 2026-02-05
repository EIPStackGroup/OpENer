/*******************************************************************************
 * Copyright (c) 2025, Martin Melik Merkumians
 * All rights reserved.
 *
 * Martin Melik Merkumians < Initial creation >
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "core/typedefs.h"
#include "enet_encap/encap.h"
#include "ports/socket_timer.h"
}

/* Forward declarations of external variables */
extern SocketTimer g_timestamps[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];
extern MilliSeconds g_actual_time;

/* Function being tested */
extern void CheckEncapsulationInactivity(int socket_handle);

/* External structure for network options */
struct TcpIpNetworkOptions {
  uint32_t encapsulation_inactivity_timeout;
};

extern TcpIpNetworkOptions g_tcpip;

TEST_GROUP(CheckEncapsulationInactivity){ void setup(){ mock().clear();

/* Initialize socket timers */
SocketTimerArrayInitialize(g_timestamps, OPENER_NUMBER_OF_SUPPORTED_SESSIONS);

/* Reset global time */
g_actual_time = 0;
}

void teardown() {
  mock().clear();
}
}
;

/**
 * Test 1: Encapsulation inactivity timeout disabled (timeout = 0)
 * Expected: No session/connection should be closed
 */
TEST(CheckEncapsulationInactivity, TimeoutDisabledDoesNothing) {
  g_tcpip.encapsulation_inactivity_timeout = 0;
  int test_socket                          = 5;

  /* Set up a socket timer */
  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 100);

  g_actual_time = 500;

  /* Call the function - should do nothing since timeout is disabled */
  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 2: Encapsulation inactivity timeout enabled, but time delta below
 * threshold Expected: No session/connection should be closed
 */
TEST(CheckEncapsulationInactivity, InactivityBelowThresholdDoesNothing) {
  g_tcpip.encapsulation_inactivity_timeout = 10; /* 10 seconds */
  int test_socket                          = 5;

  /* Set up a socket timer with recent update */
  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 1000);

  g_actual_time = 1500; /* Only 500ms elapsed, threshold is 10000ms */

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 3: Encapsulation inactivity timeout exceeded
 * Expected: CloseClass3ConnectionBasedOnSession, CloseTcpSocket, and
 * RemoveSession should all be called in sequence
 */
TEST(CheckEncapsulationInactivity,
     InactivityThresholdExceededClosesConnection) {
  g_tcpip.encapsulation_inactivity_timeout = 5; /* 5 seconds = 5000ms */
  int test_socket                          = 3;
  CipSessionHandle expected_session        = 2;

  /* Set up a socket timer with old update time */
  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 1000);

  g_actual_time = 7000; /* 6000ms elapsed, exceeds 5000ms threshold */

  /* Set up mock expectations in order */
  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", test_socket)
    .andReturnValue((int)expected_session);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", (int)expected_session);
  mock()
    .expectOneCall("CloseTcpSocket")
    .withIntParameter("socket_handle", test_socket);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", test_socket);

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 4: Socket with no timer found
 * Expected: No cleanup should be performed if socket timer is NULL
 */
TEST(CheckEncapsulationInactivity, InvalidSocketTimerDoesNothing) {
  g_tcpip.encapsulation_inactivity_timeout = 5;
  int invalid_socket = 999; /* Socket that doesn't have a timer */

  g_actual_time = 10000;

  CheckEncapsulationInactivity(invalid_socket);
}

/**
 * Test 5: Exact timeout boundary condition
 * Expected: Connection should be closed when delta equals timeout threshold
 */
TEST(CheckEncapsulationInactivity, ExactTimeoutBoundaryClosesConnection) {
  g_tcpip.encapsulation_inactivity_timeout = 3; /* 3 seconds = 3000ms */
  int test_socket                          = 7;
  CipSessionHandle expected_session        = 4;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 2000);

  g_actual_time = 5000; /* Exactly 3000ms elapsed */

  /* Set up mock expectations */
  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", test_socket)
    .andReturnValue((int)expected_session);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", (int)expected_session);
  mock()
    .expectOneCall("CloseTcpSocket")
    .withIntParameter("socket_handle", test_socket);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", test_socket);

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 6: Just below timeout boundary
 * Expected: Connection should NOT be closed when delta is one less than
 * threshold
 */
TEST(CheckEncapsulationInactivity, JustBelowTimeoutBoundaryDoesNotClose) {
  g_tcpip.encapsulation_inactivity_timeout = 3; /* 3 seconds = 3000ms */
  int test_socket                          = 8;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 2000);

  g_actual_time = 4999; /* 2999ms elapsed, just below 3000ms threshold */

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 7: Multiple socket timers - verify independent checking
 * Expected: Each socket should be evaluated independently
 */
TEST(CheckEncapsulationInactivity, MultipleSocketsIndependentEvaluation) {
  g_tcpip.encapsulation_inactivity_timeout = 2; /* 2 seconds = 2000ms */

  /* Set up first socket timer (inactive) */
  SocketTimerSetSocket(&g_timestamps[0], 10);
  SocketTimerSetLastUpdate(&g_timestamps[0], 100);

  /* Set up second socket timer (active) */
  SocketTimerSetSocket(&g_timestamps[1], 20);
  SocketTimerSetLastUpdate(&g_timestamps[1], 2500);

  g_actual_time = 3000;

  /* First socket inactive check: 3000 - 100 = 2900ms > 2000ms */
  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", 10)
    .andReturnValue(1);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", 1);
  mock().expectOneCall("CloseTcpSocket").withIntParameter("socket_handle", 10);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", 10);

  CheckEncapsulationInactivity(10);

  /* Clear mocks for next check */
  mock().clear();

  /* Second socket is still active: 3000 - 2500 = 500ms < 2000ms */
  CheckEncapsulationInactivity(20);
}

/**
 * Test 8: Zero timeout disables feature
 * Expected: Feature disabled when timeout is 0
 */
TEST(CheckEncapsulationInactivity, ZeroTimeoutDisablesFeature) {
  g_tcpip.encapsulation_inactivity_timeout = 0;
  int test_socket                          = 11;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 0);

  g_actual_time = 100000;

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 9: Very large timeout (1 hour)
 * Expected: Connection stays open even with significant elapsed time
 */
TEST(CheckEncapsulationInactivity, LargeTimeoutValue) {
  g_tcpip.encapsulation_inactivity_timeout = 3600; /* 1 hour */
  int test_socket                          = 12;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 1000);

  g_actual_time = 1800000; /* 30 minutes - still below 1 hour threshold */

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 10: One second timeout (minimum practical value)
 * Expected: Timeout closes connection after 1 second of inactivity
 */
TEST(CheckEncapsulationInactivity, MinimumOneSecondTimeout) {
  g_tcpip.encapsulation_inactivity_timeout = 1; /* 1 second = 1000ms */
  int test_socket                          = 13;
  CipSessionHandle expected_session        = 6;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 500);

  g_actual_time = 2000; /* 1500ms elapsed */

  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", test_socket)
    .andReturnValue((int)expected_session);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", (int)expected_session);
  mock()
    .expectOneCall("CloseTcpSocket")
    .withIntParameter("socket_handle", test_socket);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", test_socket);

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 11: Socket at array boundary
 * Expected: Works correctly at edge of timer array
 */
TEST(CheckEncapsulationInactivity, SocketAtArrayBoundary) {
  g_tcpip.encapsulation_inactivity_timeout = 2;

  int last_index  = OPENER_NUMBER_OF_SUPPORTED_SESSIONS - 1;
  int test_socket = 100 + last_index;

  SocketTimerSetSocket(&g_timestamps[last_index], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[last_index], 0);

  g_actual_time = 3000; /* 3000ms > 2000ms threshold */

  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", test_socket)
    .andReturnValue(7);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", 7);
  mock()
    .expectOneCall("CloseTcpSocket")
    .withIntParameter("socket_handle", test_socket);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", test_socket);

  CheckEncapsulationInactivity(test_socket);
}

/**
 * Test 12: Off-by-one boundary test
 * Expected: Connection closed when diff > threshold, not closed when diff <
 * threshold
 */
TEST(CheckEncapsulationInactivity, OffByOneBoundaryTest) {
  g_tcpip.encapsulation_inactivity_timeout = 5; /* 5000ms */
  int test_socket                          = 15;

  SocketTimerSetSocket(&g_timestamps[0], test_socket);
  SocketTimerSetLastUpdate(&g_timestamps[0], 0);

  /* Test just below: 4999ms should not close */
  g_actual_time = 4999;
  CheckEncapsulationInactivity(test_socket);

  /* Advance time - now 5001ms should close */
  g_actual_time = 5001;

  mock()
    .expectOneCall("GetSessionFromSocket")
    .withIntParameter("socket_handle", test_socket)
    .andReturnValue(9);
  mock()
    .expectOneCall("CloseClass3ConnectionBasedOnSession")
    .withIntParameter("encapsulation_session_handle", 9);
  mock()
    .expectOneCall("CloseTcpSocket")
    .withIntParameter("socket_handle", test_socket);
  mock().expectOneCall("RemoveSession").withIntParameter("socket", test_socket);

  CheckEncapsulationInactivity(test_socket);
}
