/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipethernetlink.h"

EipStatus ConfigureNetworkInterface(const char *ip_address,
                                    const char *subnet_mask,
                                    const char *gateway);

void ConfigureDomainName(const char *domain_name);

void ConfigureHostName(const char *const RESTRICT hostname);
