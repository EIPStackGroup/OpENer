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
  POINTERS_EQUAL( &timers[0], GetEmptySocketTimer(timers, 10) );
}

TEST(SocketTimer, NoEmptySocketTimerAvailable) {
  SocketTimer timers[10];
  memset( timers, 0, sizeof(timers) );
  POINTERS_EQUAL( NULL, GetEmptySocketTimer(timers, 10) );
}
