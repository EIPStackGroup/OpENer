/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPEPATH_H_
#define SRC_CIP_CIPEPATH_H_

#include <stdbool.h>

#include "ciptypes.h"

typedef enum {
  kNetworkSegmentSubtypeReserved,
  kNetworkSegmentSubtypeScheduleSegment,
  kNetworkSegmentSubtypeFixedTagSegment,
  kNetworkSegmentSubtypeProductionInhibitTimeInMilliseconds,
  kNetworkSegmentSubtypeSafetySegment,
  kNetworkSegmentSubtypeProductionInhibitTimeInMicroseconds,
  kNetworkSegmentSubtypeExtendedNetworkSegment
} NetworkSegmentSubtype;

SegmentType GetPathSegmentType(const char *const cip_path);

void SetPathSegmentType(SegmentType segment_type, char *const cip_path);

bool GetPathPortSegmentExtendedLinkAddressSizeBit(const char *const cip_path);


#endif /* SRC_CIP_CIPEPATH_H_ */
