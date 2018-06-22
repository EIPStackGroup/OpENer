/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "enipmessage.h"
#include "string.h"

void InitializeENIPMessage(ENIPMessage *const message) {
  memset(message, 0, sizeof(ENIPMessage) );
  message->current_message_position = message->message_buffer;
}
