/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {

#include "socket_timer.h"

}

TEST_GROUP(SocketTimer) {

};

TEST(SocketTimer, GetAvailableEmptySocketTimer) {
  SocketTimer timers[10];
  SocketTimerArrayInitialize(timers, 10);
  POINTERS_EQUAL( &timers[0], SocketTimerArrayGetEmptySocketTimer(timers, 10) );
}

TEST(SocketTimer, NoEmptySocketTimerAvailable) {
  SocketTimer timers[10];
  memset( timers, 0, sizeof(timers) );
  POINTERS_EQUAL( NULL, SocketTimerArrayGetEmptySocketTimer(timers, 10) );
}

TEST(SocketTimer, SetSocket) {
  SocketTimer timer = {
    socket : -1,
    last_update : 0
  };
  SocketTimerSetSocket(&timer, 1);
  CHECK_EQUAL( 1, timer.socket );
}


TEST(SocketTimer, UpdateSocketTimer) {
  SocketTimer timer = {
    socket : -1,
    last_update : 0
  };
  SocketTimerSetLastUpdate(&timer, 10);
  CHECK_EQUAL( 10, SocketTimerGetLastUpdate(&timer) );
}

TEST(SocketTimer, ClearSocketTimer) {
  SocketTimer timer = {
    socket : 5,
    last_update : 100
  };
  SocketTimerClear(&timer);
  CHECK_EQUAL(-1, timer.socket);
  CHECK_EQUAL(0, timer.last_update);
}
