/*******************************************************************************
 * Copyright (c) 2018, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef SRC_CIP_ENIPMESSAGE_H_
#define SRC_CIP_ENIPMESSAGE_H_

#include "opener_user_conf.h"

typedef struct enip_message {
  CipOctet message_buffer[PC_OPENER_ETHERNET_BUFFER_SIZE];
  CipOctet *current_message_position;
  size_t used_message_length;
} ENIPMessage;

void InitializeENIPMessage(ENIPMessage *const message);

#endif /* SRC_CIP_ENIPMESSAGE_H_ */
