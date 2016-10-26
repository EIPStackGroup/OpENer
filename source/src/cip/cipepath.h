/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPEPATH_H_
#define SRC_CIP_CIPEPATH_H_

#include <stdbool.h>

#include "ciptypes.h"

SegmentType GetPathSegementType(const char *const cip_path);

void SetPathSegementType(SegmentType segment_type, char *const cip_path);

bool GetPathPortSegementExtendedLinkAddressSizeBit(const char *const cip_path);


#endif /* SRC_CIP_CIPEPATH_H_ */
