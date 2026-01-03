/*******************************************************************************
 * Copyright (c) 2021, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_CIP_CIPSTRINGI_H_
#define SRC_CIP_CIPSTRINGI_H_

#include "cip/ciptypes.h"

void CipStringIDelete(CipStringI* const string);

bool CipStringIsAnyStringEmpty(const CipStringI* const string);

void CipStringICopy(CipStringI* const to, const CipStringI* const from);

void CipStringIDecodeFromMessage(
  CipStringI* data_to, CipMessageRouterRequest* const message_router_request);

bool CipStringICompare(const CipStringI* const stringI_1,
                       const CipStringI* const stringI_2);

#endif  // SRC_CIP_CIPSTRINGI_H_
